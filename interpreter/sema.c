#include "sema.h"

// hmput(table, key, value);
// value = hmget(table, key);
// hmdel(table, key);
// hmfree(table);

void dump_type_table(TypeTable *table) {
	if (!table) {
		printf("Table is NULL\n");
		return;
	}

	int len = shlen(table);
	printf("Table size: %d entries\n", len);

	for (int i = 0; i < len; i++) {
		printf("  [%d] key='%s' (ptr=%p), value=%p\n",
			   i,				// index
			   table[i].key,	// key string (for %s)
			   table[i].key,	// key pointer (for %p)
			   table[i].value); // value pointer
		if (table[i].value) {
			Type *t = (Type *)table[i].value;
			printf("      kind=%d, name='%s'\n", t->kind, t->name);
		}
	}
}
void dump_symbol_table(SymbolTable *table) {
	if (!table) {
		printf("Symbol table is NULL\n");
		return;
	}

	int len = shlen(table);
	printf("Global Symbol Table: %d entries\n", len);
	printf("+------------------+------------------+----------------------+\n");
	printf("| %-16s | %-16s | %-20s |\n", "Name", "Kind", "Type");
	printf("+------------------+------------------+----------------------+\n");

	for (int i = 0; i < len; i++) {
		Symbol *sym = table[i].value;
		if (!sym)
			continue;

		const char *kind_str = (sym->kind == function) ? "function" : "variable";
		const char *type_name = sym->type ? sym->type->name : "unknown";

		printf("| %-16s | %-16s | %-20s |\n",
			   sym->name ? sym->name : "(null)",
			   kind_str,
			   type_name);
	}
	printf("+------------------+------------------+----------------------+\n");
}

void add_prim_type(TypeTable **table, char *name, PrimitiveType _prim) {
	Type *type = (Type *)malloc(sizeof(Type));
	memset(type, 0, sizeof(Type));
	type->kind = prim;
	type->name = name;
	type->data.prim = _prim;
	shput(*table, name, type);
}
Type *add_usr_type(TypeTable **table, char *name, TypeKind kind) {
	Type *type = (Type *)malloc(sizeof(Type));
	if (!type) {
		perror("Failed allocation in add_usr_type");
	}
	memset(type, 0, sizeof(Type));
	type->kind = kind;
	type->name = name;
	shput(*table, name, type);
	return type;
}

void add_prims_types(TypeTable **table) {
	add_prim_type(table, "u8", u8);
	add_prim_type(table, "u32", u32);
	add_prim_type(table, "u64", u64);
	add_prim_type(table, "i32", i32);
	add_prim_type(table, "i64", i64);
	add_prim_type(table, "f32", f32);
	add_prim_type(table, "f64", f64);
	add_prim_type(table, "bool", boolean);
	add_prim_type(table, "void", voidian);
	add_prim_type(table, "usize", usize);
	add_prim_type(table, "isize", isize);
}

Type *resolve_type(TypeTable **table, Node *type_node) {
	if (!type_node)
		return NULL;
	switch (type_node->type) {
	case NODE_CONST_DECL: {
		Node *identifier = get_first_child_of_type(type_node, NODE_IDENTIFER);
		Node *const_type = get_next_sibling(identifier);
		Type *type = resolve_type(table, const_type);
		return type;
	}

	case NODE_FUNC_DECL: {
		Type **param_types = NULL;
		Node *param_list = get_first_child_of_type(type_node, NODE_PARAMETER_LIST);
		for (Node *param = get_first_child(param_list); param != NULL; param = get_next_sibling(param)) {
			Node *identifier = get_first_child(param);
			Node *param_type = get_next_sibling(identifier);
			Type *type = resolve_type(table, param_type);
			if (!type) {
				arrfree(param_types);
				char *str = slice_string(identifier->data.literal);
				printf("Resolving type faild, parameter %s is of non-resolvable type", str);
				free(str);
				return NULL;
			}
			arrpush(param_types, type);
		}
		Node *return_type = get_first_child_of_type(type_node, NODE_RETURN_TYPE);
		Type *type = resolve_type(table, get_first_child(return_type));
		if (!type) {
			type = shget(*table, "void");
		}
		size_t name_capacity = 256;
		char *name_buf = malloc(name_capacity);
		strcpy(name_buf, "fn(");
		size_t num_params = arrlen(param_types);
		for (size_t i = 0; i < num_params; i++) {
			if (i > 0)
				strcat(name_buf, ",");
			if (strlen(name_buf) + strlen(param_types[i]->name) + 32 > name_capacity) {
				name_capacity *= 2;
				name_buf = realloc(name_buf, name_capacity);
			}
			strcat(name_buf, param_types[i]->name);
		}
		strcat(name_buf, ") -> ");
		strcat(name_buf, type->name);
		Type *existing = shget(*table, name_buf);
		if (existing) {
			free(name_buf);
			arrfree(param_types);
			return existing;
		}
		Type *new_func = malloc(sizeof(Type));
		memset(new_func, 0, sizeof(Type));
		new_func->kind = func;
		new_func->name = name_buf;
		new_func->data.func.params = param_types;
		new_func->data.func.param_count = num_params;
		new_func->data.func.return_type = type;
		shput(*table, name_buf, new_func);
		return new_func;
	}
	case NODE_PRIMITIVE_TYPE: {
		const char *name = primitive_name(type_node->data.primitive_type);
		return shget(*table, name);
	}
	case NODE_POINTER_TYPE: {
		Node *inner_node = get_first_child(type_node);
		Type *inner = resolve_type(table, inner_node);
		if (!inner)
			return NULL;
		size_t len = strlen(inner->name) + 2;
		char *name = malloc(len);
		snprintf(name, len, "*%s", inner->name);
		Type *existing = shget(*table, name);
		if (existing) {
			free(name);
			return existing;
		}
		Type *new_ptr = malloc(sizeof(Type));
		memset(new_ptr, 0, sizeof(Type));
		new_ptr->kind = pointer;
		new_ptr->name = name;
		new_ptr->data.pointer.base = inner;
		shput(*table, name, new_ptr);
		return new_ptr;
	}
	case NODE_SLICE_TYPE: {
		Node *inner_node = get_first_child(type_node);
		Type *element = resolve_type(table, inner_node);
		if (!element)
			return NULL;
		size_t len = strlen(element->name) + 3;
		char *name = malloc(len);
		snprintf(name, len, "[]%s", element->name);
		Type *existing = shget(*table, name);
		if (existing) {
			free(name);
			return existing;
		}
		Type *new_slice = malloc(sizeof(Type));
		memset(new_slice, 0, sizeof(Type));
		new_slice->kind = slice;
		new_slice->name = name;
		new_slice->data.slice.element = element;
		shput(*table, name, new_slice);
		return new_slice;
	}
	case NODE_OBJ_TYPE:
	case NODE_ENUM_TYPE:
	case NODE_UNION_TYPE: {
		char *name = slice_string(type_node->data.literal);
		Type *t = shget(*table, name);
		free(name);
		return t;
	}
	default:
		return NULL;
	}
}
TypeTable *build_type_table(Node *root) {
	TypeTable *table = NULL;
	sh_new_strdup(table);
	add_prims_types(&table);

	for (Node *curr = get_first_child(root); curr != NULL; curr = get_next_sibling(curr)) {
		if (curr->type == NODE_OBJ_DECL) {
			Node *identifier = get_first_child_of_type(curr, NODE_IDENTIFER);
			char *key = slice_string(identifier->data.literal);
			if (!shget(table, key)) {
				Type *type = add_usr_type(&table, key, obj);
				curr->resolved_type = type;
				continue;
			}
			printf("obj type %s already defined, remove duplicate at %d:%d\n", key, curr->line, curr->col);
			free(key);
		}
		if (curr->type == NODE_UNION_DECL) {
			Node *identifier = get_first_child_of_type(curr, NODE_IDENTIFER);
			char *key = slice_string(identifier->data.literal);
			if (!shget(table, key)) {
				Type *type = add_usr_type(&table, key, _union);
				curr->resolved_type = type;
				continue;
			}
			printf("union type %s already defined, remove duplicate at %d:%d\n", key, curr->line, curr->col);
			free(key);
		}
		if (curr->type == NODE_ENUM_DECL) {
			Node *identifier = get_first_child_of_type(curr, NODE_IDENTIFER);
			char *key = slice_string(identifier->data.literal);
			if (!shget(table, key)) {
				Type *type = add_usr_type(&table, key, _enum);
				curr->resolved_type = type;
				continue;
			}
			printf("enum type %s already defined, remove duplicate at %d:%d\n", key, curr->line, curr->col);
			free(key);
		}
	}

	for (Node *curr = get_first_child(root); curr != NULL; curr = get_next_sibling(curr)) {
		if (curr->type == NODE_OBJ_DECL) {
			Type *obj_type = (Type *)curr->resolved_type;
			if (!obj_type)
				continue;
			uint32_t index = 0;
			for (
				Node *field = get_first_child_of_type(curr, NODE_FIELD_DECL);
				field != NULL;
				field = get_next_sibling_of_type(field, NODE_FIELD_DECL)) {
				Node *var_decl = get_first_child_of_type(field, NODE_VAR_DECL);
				Node *identifier = get_first_child_of_type(var_decl, NODE_IDENTIFER);
				Node *type_node = get_next_sibling(identifier);
				Type *field_type = resolve_type(&table, type_node);
				if (!field_type) {
					char *type_name = slice_string(type_node->data.literal);
					printf("unknown type '%s' for field at %d:%d\n", type_name, type_node->line, type_node->col);
					free(type_name);
					index++;
					continue;
				}
				Field f = {
					.name = slice_string(identifier->data.literal),
					.type = field_type,
					.index = index++,
				};
				arrpush(obj_type->data.obj.fields, f);
			}
			obj_type->data.obj.field_count = arrlen(obj_type->data.obj.fields);
		}

		if (curr->type == NODE_UNION_DECL) {
			Type *union_type = (Type *)curr->resolved_type;
			if (!union_type)
				continue;
			uint32_t tag = 0;
			for (
				Node *variant = get_first_child_of_type(curr, NODE_UNION_VARIANT);
				variant != NULL;
				variant = get_next_sibling_of_type(variant, NODE_UNION_VARIANT)) {
				Node *identifier = get_first_child(variant);
				Node *type_node = get_next_sibling(identifier);
				Type *variant_type = resolve_type(&table, type_node);
				if (!variant_type) {
					char *type_name = slice_string(type_node->data.literal);
					printf("unknown type '%s' for union variant at %d:%d\n", type_name, type_node->line, type_node->col);
					free(type_name);
					tag++;
					continue;
				}
				UnionVariant v = {
					.type = variant_type,
					.tag = tag++,
				};
				arrpush(union_type->data.union_.variants, v);
			}
			union_type->data.union_.variant_count = arrlen(union_type->data.union_.variants);
		}

		if (curr->type == NODE_ENUM_DECL) {
			Type *enum_type = (Type *)curr->resolved_type;
			if (!enum_type)
				continue;
			int64_t next_value = 0;
			for (
				Node *variant = get_first_child_of_type(curr, NODE_ENUM_VARIANT);
				variant != NULL;
				variant = get_next_sibling_of_type(variant, NODE_ENUM_VARIANT)) {
				Node *identifier = get_first_child(variant);
				Node *value_node = get_next_sibling(identifier);
				int64_t value;
				if (value_node) {
					char *val_str = slice_string(value_node->data.literal);
					value = strtoll(val_str, NULL, 10);
					free(val_str);
					next_value = value + 1;
				} else {
					value = next_value++;
				}
				EnumVariant v = {
					.name = slice_string(identifier->data.literal),
					.value = value,
				};
				arrpush(enum_type->data.enum_.variants, v);
			}
			enum_type->data.enum_.variant_count = arrlen(enum_type->data.enum_.variants);
		}
	}

	return table;
}

SymbolTable *global_symbol_registration(Node *root, TypeTable **type_table) {
	SymbolTable *table = NULL;
	sh_new_strdup(table);
	for (Node *curr = get_first_child(root); curr != NULL; curr = get_next_sibling(curr)) {
		if (curr->type == NODE_FUNC_DECL || curr->type == NODE_CONST_DECL) {
			Symbol *symbol = (Symbol *)malloc(sizeof(Symbol));
			Node *identifier = get_first_child_of_type(curr, NODE_IDENTIFER);
			char *key = slice_string(identifier->data.literal);
			Type *type = resolve_type(type_table, curr);
			symbol->name = key;
			symbol->type = type;
			symbol->decl = curr;
			symbol->scope = NULL;
			symbol->kind = function;
			if (curr->type == NODE_CONST_DECL)
				symbol->kind = variable;
			shput(table, key, symbol);
		}
	}
	return table;
}

void free_type_table(TypeTable *table) {
	if (!table)
		return;
	for (int i = 0; i < shlen(table); i++) {
		Type *t = table[i].value;
		if (t) {
			if (t->kind == obj)
				arrfree(t->data.obj.fields);
			if (t->kind == _union)
				arrfree(t->data.union_.variants);
			if (t->kind == _enum)
				arrfree(t->data.enum_.variants);
			if (t->kind != prim && t->name)
				free(t->name);
			free(t);
		}
	}
	shfree(table);
}

void free_symbol_table(SymbolTable *table) {
	if (!table)
		return;
	for (int i = 0; i < shlen(table); i++) {
		Symbol *sym = table[i].value;
		if (sym) {
			free(sym->name);
			free(sym);
		}
	}
	shfree(table);
}

bool analyize(Node *root, char *source) {
	print_ast(root, source, 0, false);
	TypeTable *table = build_type_table(root);
	SymbolTable *global = global_symbol_registration(root, &table);
	dump_symbol_table(global);
	free_type_table(table);
	free_symbol_table(global);
	return false;
}
