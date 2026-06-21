#include "sema.h"

void dump_scopes_from_ast(Node *node, int depth) {
	if (!node)
		return;
	if (node->scope) {
		for (int i = 0; i < depth; i++)
			printf("  ");
		const char *kind_str;
		switch (node->scope->kind) {
		case SCOPE_GLOBAL:
			kind_str = "GLOBAL";
			break;
		case SCOPE_FUNCTION:
			kind_str = "FUNCTION";
			break;
		case SCOPE_BLOCK:
			kind_str = "BLOCK";
			break;
		case SCOPE_OBJECT:
			kind_str = "OBJECT";
			break;
		case SCOPE_ENUM:
			kind_str = "ENUM";
			break;
		case SCOPE_UNION:
			kind_str = "UNION";
			break;
		default:
			kind_str = "UNKNOWN";
			break;
		}
		printf("[%s] at line %d\n", kind_str, node->line);

		// Print symbols
		if (node->scope->symbols) {
			int count = shlen(node->scope->symbols);
			for (int i = 0; i < count; i++) {
				Symbol *sym = node->scope->symbols[i].value;
				if (sym) {
					for (int j = 0; j < depth + 1; j++)
						printf("  ");
					const char *kind_names[] = {"VAR", "CONST", "PARAM", "FIELD", "METHOD", "VARIANT", "TYPE", "FUNC"};
					printf("  ├─ %s : %s\n", sym->name ? sym->name : "(null)",
						   sym->type ? sym->type->name : "(unknown)");
				}
			}
		}
	}
	Node *child = node->first_child;
	while (child) {
		dump_scopes_from_ast(child, depth + 1);
		child = child->next_sibling;
	}
}
void dump_type_table(TypeTable table) {
	if (!table) {
		printf("Type table is NULL\n");
		return;
	}
	int len = shlen(table);
	printf("Type table: %d entries\n", len);
	for (int i = 0; i < len; i++) {
		Type *t = table[i].value;
		printf("  [%d] '%s'  kind=%d\n", i, table[i].key, t ? t->kind : -1);
	}
}

void dump_symbol_table(SymbolTable table) {
	if (!table) {
		printf("Symbol table is NULL\n");
		return;
	}
	int len = shlen(table);
	printf("Symbol table: %d entries\n", len);
	printf("+------------------+------------------+----------------------+\n");
	printf("| %-16s | %-16s | %-20s |\n", "Name", "Kind", "Type");
	printf("+------------------+------------------+----------------------+\n");
	for (int i = 0; i < len; i++) {
		Symbol *sym = table[i].value;
		if (!sym)
			continue;
		const char *kind_str;
		switch (sym->kind) {
		case SYM_FUNCTION:
			kind_str = "function";
			break;
		case SYM_CONSTANT:
			kind_str = "constant";
			break;
		case SYM_VARIABLE:
			kind_str = "variable";
			break;
		case SYM_PARAMETER:
			kind_str = "parameter";
			break;
		case SYM_FIELD:
			kind_str = "field";
			break;
		case SYM_METHOD:
			kind_str = "method";
			break;
		default:
			kind_str = "other";
			break;
		}
		const char *type_name = sym->type ? sym->type->name : "(unknown)";
		printf("| %-16s | %-16s | %-20s |\n", sym->name ? sym->name : "(null)", kind_str, type_name);
	}
	printf("+------------------+------------------+----------------------+\n");
}

static TypeTable add_prim_type(TypeTable table, const char *name, PrimitiveType prim) {
	Type *t = calloc(1, sizeof(Type));
	if (!t) {
		perror("add_prim_type: calloc");
		return table;
	}
	t->kind = TYPE_PRIM;
	t->name = (char *)name;
	t->data.prim = prim;
	shput(table, name, t);
	return table;
}

static TypeTable add_prim_types(TypeTable table) {
	table = add_prim_type(table, "u8", u8);
	table = add_prim_type(table, "u32", u32);
	table = add_prim_type(table, "u64", u64);
	table = add_prim_type(table, "i32", i32);
	table = add_prim_type(table, "i64", i64);
	table = add_prim_type(table, "f32", f32);
	table = add_prim_type(table, "f64", f64);
	table = add_prim_type(table, "bool", boolean);
	table = add_prim_type(table, "void", voidian);
	table = add_prim_type(table, "usize", usize);
	table = add_prim_type(table, "isize", isize);
	return table;
}

static Type *add_usr_type(TypeTable *table, char *name, TypeKind kind) {
	Type *t = calloc(1, sizeof(Type));
	if (!t) {
		perror("add_usr_type: calloc");
		return NULL;
	}
	t->kind = kind;
	t->name = name;
	shput(*table, name, t);
	return t;
}

Type *resolve_type(Sema *sema, Node *type_node) {
	if (!type_node)
		return NULL;
	TypeTable *tbl = &sema->types;
	switch (type_node->type) {
	case NODE_PRIMITIVE_TYPE: {
		const char *name = primitive_name(type_node->data.primitive_type);
		return shget(*tbl, name);
	}

	case NODE_POINTER_TYPE: {
		Type *inner = resolve_type(sema, get_first_child(type_node));
		if (!inner)
			return NULL;
		size_t len = strlen(inner->name) + 2;
		char *name = malloc(len);
		snprintf(name, len, "*%s", inner->name);
		Type *existing = shget(*tbl, name);
		if (existing) {
			free(name);
			return existing;
		}
		Type *t = calloc(1, sizeof(Type));
		t->kind = TYPE_POINTER;
		t->name = name;
		t->data.pointer.base = inner;
		shput(*tbl, name, t);
		return t;
	}

	case NODE_SLICE_TYPE: {
		Type *elem = resolve_type(sema, get_first_child(type_node));
		if (!elem)
			return NULL;
		size_t len = strlen(elem->name) + 3;
		char *name = malloc(len);
		snprintf(name, len, "[]%s", elem->name);
		Type *existing = shget(*tbl, name);
		if (existing) {
			free(name);
			return existing;
		}
		Type *t = calloc(1, sizeof(Type));
		t->kind = TYPE_SLICE;
		t->name = name;
		t->data.slice.element = elem;
		shput(*tbl, name, t);
		return t;
	}

	case NODE_OBJ_TYPE:
	case NODE_ENUM_TYPE:
	case NODE_UNION_TYPE: {
		char *name = slice_string(type_node->data.literal);
		Type *t = shget(*tbl, name);
		free(name);
		return t;
	}

	default:
		return NULL;
	}
}

Type *resolve_decl_type(Sema *sema, Node *decl_node) {
	if (!decl_node)
		return NULL;
	TypeTable *tbl = &sema->types;

	switch (decl_node->type) {
	case NODE_CONST_DECL: {
		Node *ident = get_first_child_of_type(decl_node, NODE_IDENTIFER);
		Node *type_ann = get_next_sibling(ident);
		return resolve_type(sema, type_ann);
	}

	case NODE_FUNC_DECL: {
		Type **param_types = NULL;
		Node *param_list = get_first_child_of_type(decl_node, NODE_PARAMETER_LIST);
		for (Node *param = get_first_child(param_list); param; param = get_next_sibling(param)) {
			Node *ident = get_first_child(param);
			Node *type_ann = get_next_sibling(ident);
			Type *pt = resolve_type(sema, type_ann);
			if (!pt) {
				char *str = slice_string(ident->data.literal);
				printf("sema: parameter '%s' has unknown type at %d:%d\n", str, type_ann ? type_ann->line : 0,
					   type_ann ? type_ann->col : 0);
				free(str);
				arrfree(param_types);
				return NULL;
			}
			arrpush(param_types, pt);
		}

		Node *ret_node = get_first_child_of_type(decl_node, NODE_RETURN_TYPE);
		Type *ret = ret_node ? resolve_type(sema, get_first_child(ret_node)) : NULL;
		if (!ret)
			ret = shget(*tbl, "void");

		size_t cap = 256;
		char *name = malloc(cap);
		strcpy(name, "fn(");
		size_t n = (size_t)arrlen(param_types);
		for (size_t i = 0; i < n; i++) {
			if (i > 0)
				strcat(name, ",");
			size_t needed = strlen(name) + strlen(param_types[i]->name) + 8;
			if (needed > cap) {
				cap = needed * 2;
				name = realloc(name, cap);
			}
			strcat(name, param_types[i]->name);
		}
		strcat(name, ") -> ");
		strcat(name, ret->name);

		Type *existing = shget(*tbl, name);
		if (existing) {
			free(name);
			arrfree(param_types);
			return existing;
		}

		Type *ft = calloc(1, sizeof(Type));
		ft->kind = TYPE_FUNC;
		ft->name = name;
		ft->data.func.params = param_types;
		ft->data.func.param_count = n;
		ft->data.func.return_type = ret;
		shput(*tbl, name, ft);
		return ft;
	}

	default:
		return NULL;
	}
}

static void fill_obj_fields(Sema *sema, Type *obj_type, Node *decl_node) {
	uint32_t index = 0;
	for (Node *field = get_first_child_of_type(decl_node, NODE_FIELD_DECL); field;
		 field = get_next_sibling_of_type(field, NODE_FIELD_DECL)) {

		Node *var_decl = get_first_child_of_type(field, NODE_VAR_DECL);
		Node *ident = get_first_child_of_type(var_decl, NODE_IDENTIFER);
		Node *type_ann = get_next_sibling(ident);
		Type *field_type = resolve_type(sema, type_ann);

		if (!field_type) {
			char *tname = slice_string(type_ann->data.literal);
			printf("sema: unknown field type '%s' at %d:%d\n", tname, type_ann->line, type_ann->col);
			free(tname);
			index++;
			continue;
		}
		Field f = {
			.name = slice_string(ident->data.literal),
			.type = field_type,
			.index = index++,
		};
		arrpush(obj_type->data.obj.fields, f);
	}
	obj_type->data.obj.field_count = (size_t)arrlen(obj_type->data.obj.fields);
}

static void fill_union_variants(Sema *sema, Type *union_type, Node *decl_node) {
	uint32_t tag = 0;
	for (Node *variant = get_first_child_of_type(decl_node, NODE_UNION_VARIANT); variant;
		 variant = get_next_sibling_of_type(variant, NODE_UNION_VARIANT)) {

		Node *ident = get_first_child(variant);
		Node *type_ann = get_next_sibling(ident);
		Type *var_type = resolve_type(sema, type_ann);

		if (!var_type) {
			char *tname = slice_string(type_ann->data.literal);
			printf("sema: unknown union variant type '%s' at %d:%d\n", tname, type_ann->line, type_ann->col);
			free(tname);
			tag++;
			continue;
		}
		UnionVariant v = {
			.name = slice_string(ident->data.literal),
			.type = var_type,
			.tag = tag++,
		};
		arrpush(union_type->data.union_.variants, v);
	}
	union_type->data.union_.variant_count = (size_t)arrlen(union_type->data.union_.variants);
}

static void fill_enum_variants(Type *enum_type, Node *decl_node) {
	int64_t next_value = 0;
	for (Node *variant = get_first_child_of_type(decl_node, NODE_ENUM_VARIANT); variant;
		 variant = get_next_sibling_of_type(variant, NODE_ENUM_VARIANT)) {

		Node *ident = get_first_child(variant);
		Node *value_node = get_next_sibling(ident);
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
			.name = slice_string(ident->data.literal),
			.value = value,
		};
		arrpush(enum_type->data.enum_.variants, v);
	}
	enum_type->data.enum_.variant_count = (size_t)arrlen(enum_type->data.enum_.variants);
}

static void build_type_table(Sema *sema) {
	TypeTable table = NULL;
	sh_new_strdup(table);
	table = add_prim_types(table);

	for (Node *curr = get_first_child(sema->root); curr; curr = get_next_sibling(curr)) {
		TypeKind kind;
		if (curr->type == NODE_OBJ_DECL)
			kind = TYPE_OBJ;
		else if (curr->type == NODE_UNION_DECL)
			kind = TYPE_UNION;
		else if (curr->type == NODE_ENUM_DECL)
			kind = TYPE_ENUM;
		else
			continue;

		Node *ident = get_first_child_of_type(curr, NODE_IDENTIFER);
		char *key = slice_string(ident->data.literal);

		if (shget(table, key)) {
			printf("sema: type '%s' already defined (duplicate at %d:%d)\n", key, curr->line, curr->col);
			free(key);
			continue;
		}

		Type *type = add_usr_type(&table, key, kind);
		curr->resolved_type = type;
	}

	sema->types = table;

	for (Node *curr = get_first_child(sema->root); curr; curr = get_next_sibling(curr)) {
		Type *user_type = (Type *)curr->resolved_type;
		if (!user_type)
			continue;

		switch (user_type->kind) {
		case TYPE_OBJ:
			fill_obj_fields(sema, user_type, curr);
			break;
		case TYPE_UNION:
			fill_union_variants(sema, user_type, curr);
			break;
		case TYPE_ENUM:
			fill_enum_variants(user_type, curr);
			break;
		default:
			break;
		}
	}
}

Scope *create_scope(Scope *parent, Node *node, ScopeKind kind) {
	Scope *scope = calloc(1, sizeof(Scope));
	if (!scope) {
		perror("create_scope: calloc");
		return NULL;
	}
	sh_new_strdup(scope->symbols);
	scope->parent = parent;
	scope->node = node;
	scope->kind = kind;
	scope->depth = parent ? parent->depth + 1 : 0;
	return scope;
}

void push_scope(Sema *sema, Scope *scope) { sema->current_scope = scope; }

void pop_scope(Sema *sema) {
	if (sema->current_scope)
		sema->current_scope = sema->current_scope->parent;
}

Symbol *make_symbol(SymbolKind kind, char *name, Type *type, Node *decl, Scope *scope) {
	Symbol *sym = calloc(1, sizeof(Symbol));
	if (!sym) {
		perror("make_symbol: calloc");
		return NULL;
	}
	sym->kind = kind;
	sym->name = name;
	sym->type = type;
	sym->decl = decl;
	sym->scope = scope;
	return sym;
}

bool add_symbol(Sema *sema, Symbol *symbol) {
	Scope *scope = sema->current_scope;
	if (shget(scope->symbols, symbol->name)) {
		printf("sema: '%s' already declared in this scope\n", symbol->name);
		return false;
	}
	shput(scope->symbols, symbol->name, symbol);
	return true;
}

Symbol *lookup_symbol(Scope *scope, const char *name) {
	for (Scope *s = scope; s; s = s->parent) {
		Symbol *sym = shget(s->symbols, name);
		if (sym)
			return sym;
	}
	return NULL;
}

Symbol *lookup_symbol_local(Scope *scope, const char *name) {
	if (!scope)
		return NULL;
	return shget(scope->symbols, name);
}

static void global_symbol_registration(Sema *sema) {
	Scope *global = create_scope(NULL, sema->root, SCOPE_GLOBAL);
	if (!global)
		return;

	sema->root->scope = global;
	sema->global = global;
	sema->current_scope = global;

	for (Node *curr = get_first_child(sema->root); curr; curr = get_next_sibling(curr)) {
		SymbolKind kind;
		if (curr->type == NODE_FUNC_DECL)
			kind = SYM_FUNCTION;
		else if (curr->type == NODE_CONST_DECL)
			kind = SYM_CONSTANT;
		else
			continue;

		Node *ident = get_first_child_of_type(curr, NODE_IDENTIFER);
		char *name = slice_string(ident->data.literal);
		Type *type = resolve_decl_type(sema, curr);

		Symbol *sym = make_symbol(kind, name, type, curr, global);
		if (sym)
			add_symbol(sema, sym);
	}
}

static void free_type(Type *t) {
	if (!t)
		return;
	switch (t->kind) {
	case TYPE_PRIM:
		break;
	case TYPE_OBJ:
		for (int i = 0; i < (int)arrlen(t->data.obj.fields); i++)
			free(t->data.obj.fields[i].name);
		arrfree(t->data.obj.fields);
		free(t->name);
		break;
	case TYPE_UNION:
		for (int i = 0; i < (int)arrlen(t->data.union_.variants); i++)
			free(t->data.union_.variants[i].name);
		arrfree(t->data.union_.variants);
		free(t->name);
		break;
	case TYPE_ENUM:
		for (int i = 0; i < (int)arrlen(t->data.enum_.variants); i++)
			free(t->data.enum_.variants[i].name);
		arrfree(t->data.enum_.variants);
		free(t->name);
		break;
	case TYPE_FUNC:
		arrfree(t->data.func.params);
		free(t->name);
		break;
	case TYPE_POINTER:
	case TYPE_SLICE:
		free(t->name);
		break;
	}
	free(t);
}

static void free_type_table(TypeTable *table) {
	if (!table || !*table)
		return;
	int len = shlen(*table);
	for (int i = 0; i < len; i++)
		free_type((*table)[i].value);
	shfree(*table);
}

static void free_symbol_table(SymbolTable *table) {
	if (!table || !*table)
		return;
	int len = shlen(*table);
	for (int i = 0; i < len; i++) {
		Symbol *sym = (*table)[i].value;
		if (sym) {
			free(sym->name);
			free(sym);
		}
	}
	shfree(*table);
}

static void free_scope(Scope *scope) {
	if (!scope)
		return;
	free_symbol_table(&scope->symbols);
	free(scope);
}

Scope *find_nearest_scope_of_kind(Scope *scope, ScopeKind kind) {
	for (Scope *s = scope; s; s = s->parent) {
		if (s->kind == kind)
			return s;
	}
	return NULL;
}

void build_scopes(Sema *sema, Node *node) {
	if (!node)
		return;
	switch (node->type) {
	case NODE_OBJ_DECL: {
		Scope *scope = create_scope(sema->current_scope, node, SCOPE_OBJECT);
		push_scope(sema, scope);
		node->scope = scope;
		break;
	}
	case NODE_METHOD_DECL: {
		Scope *obj_scope = find_nearest_scope_of_kind(sema->current_scope, SCOPE_OBJECT);
		Scope *func_scope = create_scope(sema->current_scope, node, SCOPE_FUNCTION);
		push_scope(sema, func_scope);
		node->scope = func_scope;
		if (obj_scope) {
			Node *ident = get_first_child_of_type(node, NODE_IDENTIFER);
			char *name = slice_string(ident->data.literal);
			Type *method_type = resolve_decl_type(sema, node);
			Symbol *sym = make_symbol(SYM_METHOD, name, method_type, node, obj_scope);
			shput(obj_scope->symbols, name, sym);
		}
		Node *self_param = get_first_child_of_type(node, NODE_SELF_PARAM);
		if (self_param) {
			Node *obj_type = get_first_child(self_param);
			char *type_name = slice_string(obj_type->data.literal);
			Type *self_type = shget(sema->types, type_name);
			free(type_name);
			char *ptr_name = malloc(strlen(self_type->name) + 2);
			sprintf(ptr_name, "*%s", self_type->name);
			Type *ptr_type = shget(sema->types, ptr_name);
			free(ptr_name);

			Symbol *sym = make_symbol(SYM_PARAMETER, "self", ptr_type, self_param, obj_scope);
			add_symbol(sema, sym);
		}
		break;
	}
	case NODE_FUNC_DECL:
	case NODE_BLOCK: {
		ScopeKind kind;
		if (node->type == NODE_FUNC_DECL || node->type == NODE_METHOD_DECL)
			kind = SCOPE_FUNCTION;
		else
			kind = SCOPE_BLOCK;
		Scope *scope = create_scope(sema->current_scope, node, kind);
		push_scope(sema, scope);
		node->scope = scope;
		break;
	}
	case NODE_PARAMETER: {
		Node *identfier = get_first_child_of_type(node, NODE_IDENTIFER);
		Node *type_node = get_next_sibling(identfier);
		Type *type = resolve_type(sema, type_node);
		char *name = slice_string(identfier->data.literal);
		Symbol *sym = make_symbol(SYM_PARAMETER, name, type, node, sema->current_scope);
		if (sym)
			add_symbol(sema, sym);
		break;
	}
	case NODE_VAR_DECL:
	case NODE_LET_DECL: {
		Node *identfier = get_first_child_of_type(node, NODE_IDENTIFER);
		Node *type_node = get_next_sibling(identfier);
		Type *type = resolve_type(sema, type_node);
		char *name = slice_string(identfier->data.literal);
		Symbol *sym = make_symbol(SYM_VARIABLE, name, type, node, sema->current_scope);
		node->resolved_symbol = sym;
		if (sym)
			add_symbol(sema, sym);
		break;
	}
	case NODE_FIELD_DECL: {
		Scope *obj_scope = find_nearest_scope_of_kind(sema->current_scope, SCOPE_OBJECT);
		if (obj_scope) {
			Node *var_decl = get_first_child_of_type(node, NODE_VAR_DECL);
			Node *ident = get_first_child_of_type(var_decl, NODE_IDENTIFER);
			Node *type_ann = get_next_sibling(ident);
			Type *field_type = resolve_type(sema, type_ann);
			char *name = slice_string(ident->data.literal);
			Symbol *sym = make_symbol(SYM_FIELD, name, field_type, node, obj_scope);
			// Add to object's symbol table, not current scope
			shput(obj_scope->symbols, name, sym);
		}
		break;
	}
	default: {
		break;
	}
	}
	for (Node *child = node->first_child; child; child = child->next_sibling) {
		build_scopes(sema, child);
	}
	if (node->type == NODE_FUNC_DECL || node->type == NODE_BLOCK || node->type == NODE_METHOD_DECL ||
		node->type == NODE_OBJ_DECL) {
		pop_scope(sema);
	}
}

bool analyize(Node *root, char *source) {
	printf("%s\n", source);
	print_ast(root, source, 0, false);
	Sema sema = {0};
	sema.source = source;
	sema.root = root;
	build_type_table(&sema);
	global_symbol_registration(&sema);
	sema.current_scope = sema.global;
	sema.root->scope = sema.global;
	build_scopes(&sema, root); // — walk all functions/blocks, build scope tree
							   // TODO: type_check(&sema);    — bottom-up type checking + annotation
	dump_type_table(sema.types);
	dump_symbol_table(sema.global ? sema.global->symbols : NULL);
	dump_scopes_from_ast(root, 0);
	free_type_table(&sema.types);
	if (sema.global)
		free_scope(sema.global);
	return false;
}
