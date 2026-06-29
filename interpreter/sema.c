#include "sema.h"

void print_type_table(TypeTable table) {
	int len = shlen(table);
	printf("\n╔══════════════════════════════════════════════════════════════════╗\n");
	printf("║                         TYPE TABLE                              ║\n");
	printf("╠══════════════════════════════════════════════════════════════════╣\n");
	printf("║ %-30s │ %-10s │ %-20s ║\n", "Type Name", "Kind", "Details");
	printf("╠══════════════════════════════════════════════════════════════════╣\n");

	for (int i = 0; i < len; i++) {
		Type *t = table[i].value;
		if (!t)
			continue;

		const char *kind_str;
		char details[50] = "";

		switch (t->kind) {
		case TYPE_PRIM:
			kind_str = "Primitive";
			snprintf(details, sizeof(details), "%s", primitive_name(t->data.prim));
			break;
		case TYPE_OBJ:
			kind_str = "Object";
			snprintf(details, sizeof(details), "%zu fields", t->data.obj.field_count);
			break;
		case TYPE_ENUM:
			kind_str = "Enum";
			snprintf(details, sizeof(details), "%zu variants", t->data.enum_.variant_count);
			break;
		case TYPE_UNION:
			kind_str = "Union";
			snprintf(details, sizeof(details), "%zu variants", t->data.union_.variant_count);
			break;
		case TYPE_POINTER:
			kind_str = "Pointer";
			snprintf(details, sizeof(details), "-> %s", t->data.pointer.base ? t->data.pointer.base->name : "?");
			break;
		case TYPE_SLICE:
			kind_str = "Slice";
			snprintf(details, sizeof(details), "[]%s", t->data.slice.element ? t->data.slice.element->name : "?");
			break;
		case TYPE_FUNC:
			kind_str = "Function";
			snprintf(details, sizeof(details), "(%zu params) -> %s", t->data.func.param_count,
					 t->data.func.return_type ? t->data.func.return_type->name : "?");
			break;
		default:
			kind_str = "Unknown";
			snprintf(details, sizeof(details), "???");
			break;
		}

		printf("║ %-30s │ %-10s │ %-20s ║\n", t->name, kind_str, details);
	}
	printf("╚══════════════════════════════════════════════════════════════════╝\n");
	printf("Total types: %d\n\n", len);
}

void print_symbol_table(SymbolTable table) {

	int len = shlen(table);
	printf("                              SYMBOL TABLE                                      \n");
	printf("║ %-25s │ %-12s │ %-30s │ %-20s ║\n", "Name", "Kind", "Type", "Scope");

	for (int i = 0; i < len; i++) {
		Symbol *sym = table[i].value;
		if (!sym)
			continue;

		const char *kind_str;
		switch (sym->kind) {
		case SYM_VARIABLE:
			kind_str = "Variable";
			break;
		case SYM_CONSTANT:
			kind_str = "Constant";
			break;
		case SYM_SELF_PARAMETER:
		case SYM_PARAMETER:
			kind_str = "Parameter";
			break;
		case SYM_FIELD:
			kind_str = "Field";
			break;
		case SYM_METHOD:
			kind_str = "Method";
			break;
		case SYM_VARIANT:
			kind_str = "Variant";
			break;
		case SYM_TYPE:
			kind_str = "Type";
			break;
		case SYM_FUNCTION:
			kind_str = "Function";
			break;
		default:
			kind_str = "Unknown";
			break;
		}

		char type_name[20] = "?";
		if (sym->type && sym->type->name) {
			snprintf(type_name, sizeof(type_name), "%s", sym->type->name);
		}

		char scope_name[20] = "?";
		if (sym->scope) {
			switch (sym->scope->kind) {
			case SCOPE_GLOBAL:
				snprintf(scope_name, sizeof(scope_name), "global");
				break;
			case SCOPE_FUNCTION:
				snprintf(scope_name, sizeof(scope_name), "function");
				break;
			case SCOPE_BLOCK:
				snprintf(scope_name, sizeof(scope_name), "block");
				break;
			case SCOPE_OBJECT:
				snprintf(scope_name, sizeof(scope_name), "object");
				break;
			case SCOPE_ENUM:
				snprintf(scope_name, sizeof(scope_name), "enum");
				break;
			case SCOPE_UNION:
				snprintf(scope_name, sizeof(scope_name), "union");
				break;
			default:
				snprintf(scope_name, sizeof(scope_name), "?");
				break;
			}
		}

		printf("║ %-25s │ %-12s │ %-30s │ %-20s ║\n", sym->name ? sym->name : "<unnamed>", kind_str, type_name,
			   scope_name);
	}
	printf("Total symbols: %d\n\n", len);
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
	case NODE_METHOD_DECL:
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
		if (sym) {
			curr->resolved_symbol = sym;
			add_symbol(sema, sym);
		}
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
	case TYPE_NON:
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
			if (sym->kind != SYM_SELF_PARAMETER && sym->name) {
				free(sym->name);
			}
			free(sym);
		}
	}
	shfree(*table);
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
			node->resolved_symbol = sym;
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

			Symbol *sym = make_symbol(SYM_SELF_PARAMETER, "self", ptr_type, self_param, obj_scope);
			self_param->resolved_symbol = sym;
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

	case NODE_RANGE_FOR_LOOP: {
		Node *element = get_first_child_of_type(node, NODE_IDENTIFER);
		char *name = slice_string(element->data.literal);
		Symbol *sym = make_symbol(SYM_VARIABLE, name, NULL, node, sema->current_scope);
		element->resolved_symbol = sym;
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
		return;
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

// Add a helper for error reporting
static void type_error(Node *node, const char *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stderr, "type error at %d:%d: ", node->line, node->col);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

Type *validate_type(Sema *sema, Node *node) {
	if (!node)
		return NULL;

	switch (node->type) {
	// ============ DECLARATIONS ============
	case NODE_VAR_DECL: {
		Node *ident = get_first_child(node);
		Node *type_node = get_next_sibling(ident);
		Node *expr = get_next_sibling(type_node);

		Type *decl_type = resolve_type(sema, type_node);
		if (!decl_type) {
			type_error(node, "unknown type in variable declaration");
			return NULL;
		}
		node->resolved_type = decl_type;

		if (expr) {
			Type *expr_type = validate_type(sema, expr);
			if (!expr_type) {
				return NULL; // error already reported
			}
			if (expr_type != decl_type) {
				type_error(node, "type mismatch in var declaration: expected '%s', got '%s'", decl_type->name,
						   expr_type->name);
				return NULL;
			}
		}
		return decl_type;
	}

	case NODE_LET_DECL: {
		Node *ident = get_first_child(node);
		Node *type_node = get_next_sibling(ident);
		Node *expr = get_next_sibling(type_node);

		Type *decl_type = resolve_type(sema, type_node);
		if (!decl_type) {
			type_error(node, "unknown type in let declaration");
			return NULL;
		}
		node->resolved_type = decl_type;

		if (!expr) {
			type_error(node, "let declaration requires an initializer");
			return NULL;
		}

		Type *expr_type = validate_type(sema, expr);
		if (!expr_type) {
			return NULL;
		}
		if (expr_type != decl_type) {
			type_error(node, "type mismatch in let declaration: expected '%s', got '%s'", decl_type->name,
					   expr_type->name);
			return NULL;
		}
		return decl_type;
	}

	case NODE_CONST_DECL: {
		Node *vis = get_first_child(node);
		Node *ident = get_next_sibling(vis);
		Node *type_node = get_next_sibling(ident);
		Node *expr = get_next_sibling(type_node);

		Type *decl_type = resolve_type(sema, type_node);
		if (!decl_type) {
			type_error(node, "unknown type in const declaration");
			return NULL;
		}
		node->resolved_type = decl_type;

		Type *expr_type = validate_type(sema, expr);
		if (!expr_type) {
			return NULL;
		}
		if (expr_type != decl_type) {
			type_error(node, "type mismatch in const declaration: expected '%s', got '%s'", decl_type->name,
					   expr_type->name);
			return NULL;
		}
		return decl_type;
	}

	// ============ ASSIGNMENTS ============
	case NODE_ASSIGNMENT_STMT: {
		Node *left = get_first_child(node);
		Node *op = get_next_sibling(left);
		Node *right = get_next_sibling(op);

		Type *left_type = validate_type(sema, left);
		if (!left_type) {
			return NULL;
		}

		// Check if left is assignable (not a constant)
		if (left->type == NODE_IDENTIFER) {
			char *name = slice_string(left->data.literal);
			Symbol *sym = lookup_symbol(sema->current_scope, name);
			free(name);
			if (sym && sym->kind == SYM_CONSTANT) {
				type_error(node, "cannot assign to constant '%s'", sym->name);
				return NULL;
			}
		}

		Type *right_type = validate_type(sema, right);
		if (!right_type) {
			return NULL;
		}

		if (left_type != right_type) {
			type_error(node, "type mismatch in assignment: left is '%s', right is '%s'", left_type->name,
					   right_type->name);
			return NULL;
		}

		node->resolved_type = left_type;
		return left_type;
	}

	// ============ STATEMENTS ============
	case NODE_EXPRESSION_STMT: {
		Node *expr = get_first_child(node);
		if (expr) {
			return validate_type(sema, expr);
		}
		return shget(sema->types, "void");
	}

	case NODE_RETURN_STMT: {
		Node *expr = get_first_child(node);
		Type *expr_type = NULL;

		if (expr) {
			expr_type = validate_type(sema, expr);
			if (!expr_type) {
				return NULL;
			}
		}

		// Find enclosing function/method
		Node *func = get_parent_of_type(node, NODE_FUNC_DECL);
		if (!func) {
			func = get_parent_of_type(node, NODE_METHOD_DECL);
		}

		if (func) {
			Node *return_type_node = get_first_child_of_type(func, NODE_RETURN_TYPE);
			if (return_type_node) {
				Node *type_node = get_first_child(return_type_node);
				Type *func_return_type = resolve_type(sema, type_node);
				if (!func_return_type) {
					type_error(node, "unknown return type in function");
					return NULL;
				}

				// void return type means no expression allowed
				if (func_return_type == shget(sema->types, "void")) {
					if (expr) {
						type_error(node, "void function cannot return a value");
						return NULL;
					}
				} else {
					if (!expr) {
						type_error(node, "expected return value of type '%s'", func_return_type->name);
						return NULL;
					}
					if (expr_type != func_return_type) {
						type_error(node, "return type mismatch: expected '%s', got '%s'", func_return_type->name,
								   expr_type->name);
						return NULL;
					}
				}
			}
		}

		node->resolved_type = expr_type ? expr_type : shget(sema->types, "void");
		return node->resolved_type;
	}

	case NODE_BLOCK: {
		Type *last_type = shget(sema->types, "void");
		for (Node *child = get_first_child(node); child; child = get_next_sibling(child)) {
			Type *child_type = validate_type(sema, child);
			if (child_type && child_type != shget(sema->types, "void")) {
				last_type = child_type;
			}
		}
		node->resolved_type = last_type;
		return last_type;
	}

	case NODE_CONDITIONAL_STMT: {
		Node *condition = get_first_child(node);
		Node *then_block = get_next_sibling(condition);
		Node *else_block = get_next_sibling(then_block);

		Type *cond_type = validate_type(sema, condition);
		if (!cond_type) {
			return NULL;
		}

		Type *bool_type = shget(sema->types, "bool");
		if (cond_type != bool_type) {
			type_error(node, "if condition must be boolean, got '%s'", cond_type->name);
			return NULL;
		}

		Type *then_type = validate_type(sema, then_block);
		Type *else_type = NULL;
		if (else_block) {
			else_type = validate_type(sema, else_block);
		}

		// If both branches exist, they should have the same type
		if (then_type && else_type && then_type != else_type) {
			type_error(node, "if/else branches have different types: then='%s', else='%s'", then_type->name,
					   else_type->name);
			return NULL;
		}

		node->resolved_type = then_type ? then_type : shget(sema->types, "void");
		return node->resolved_type;
	}

	// ============ EXPRESSIONS ============
	case NODE_IDENTIFER: {
		char *name = slice_string(node->data.literal);
		Symbol *sym = lookup_symbol(sema->current_scope, name);
		free(name);

		if (!sym) {
			type_error(node, "undeclared identifier");
			return NULL;
		}

		node->resolved_symbol = sym;
		node->resolved_type = sym->type;
		return sym->type;
	}

	// ============ LITERALS ============
	case NODE_DECIMAL_LITERAL:
	case NODE_HEX_LITERAL:
	case NODE_OCTAL_LITERAL:
	case NODE_BINARY_LITERAL: {
		Type *int_type = shget(sema->types, "i32"); // default integer type
		node->resolved_type = int_type;
		return int_type;
	}

	case NODE_FLOAT_LITERAL: {
		Type *float_type = shget(sema->types, "f32");
		node->resolved_type = float_type;
		return float_type;
	}

	case NODE_BOOLEAN_LITERAL: {
		Type *bool_type = shget(sema->types, "bool");
		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_STRING_LITERAL: {
		Type *u8_type = shget(sema->types, "u8");
		char *slice_name = malloc(strlen(u8_type->name) + 3);
		sprintf(slice_name, "[]%s", u8_type->name);
		Type *slice_type = shget(sema->types, slice_name);
		free(slice_name);
		node->resolved_type = slice_type;
		return slice_type;
	}

	case NODE_ARRAY_LITERAL: {
		// Type will be determined by first element, all elements must match
		Node *first_expr = get_first_child(node);
		if (!first_expr) {
			type_error(node, "empty array literal");
			return NULL;
		}

		Type *elem_type = validate_type(sema, first_expr);
		if (!elem_type) {
			return NULL;
		}

		// Check all elements have the same type
		for (Node *expr = get_next_sibling(first_expr); expr; expr = get_next_sibling(expr)) {
			Type *expr_type = validate_type(sema, expr);
			if (!expr_type) {
				return NULL;
			}
			if (expr_type != elem_type) {
				type_error(node, "array literal elements must have same type: expected '%s', got '%s'", elem_type->name,
						   expr_type->name);
				return NULL;
			}
		}

		// Build slice type: []T
		char *slice_name = malloc(strlen(elem_type->name) + 3);
		sprintf(slice_name, "[]%s", elem_type->name);
		Type *slice_type = shget(sema->types, slice_name);
		free(slice_name);
		node->resolved_type = slice_type;
		return slice_type;
	}

	// ============ BINARY OPERATIONS ============
	case NODE_ADDITIVE_EXPRESSION:
	case NODE_MULTIPLICTIVE_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		// Both must be the same primitive numeric type
		if (left_type->kind != TYPE_PRIM || right_type->kind != TYPE_PRIM) {
			type_error(node, "arithmetic requires numeric types, got '%s' and '%s'", left_type->name, right_type->name);
			return NULL;
		}

		if (left_type != right_type) {
			type_error(node, "arithmetic operands must have same type: got '%s' and '%s'", left_type->name,
					   right_type->name);
			return NULL;
		}

		node->resolved_type = left_type;
		return left_type;
	}

	case NODE_EQUALITY_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		// Both must be the same type (any type is fine for equality)
		if (left_type != right_type) {
			type_error(node, "equality operands must have same type: got '%s' and '%s'", left_type->name,
					   right_type->name);
			return NULL;
		}

		Type *bool_type = shget(sema->types, "bool");
		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_RELATIONAL_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		// Both must be primitive numeric types
		if (left_type->kind != TYPE_PRIM || right_type->kind != TYPE_PRIM) {
			type_error(node, "relational requires numeric types, got '%s' and '%s'", left_type->name, right_type->name);
			return NULL;
		}

		if (left_type != right_type) {
			type_error(node, "relational operands must have same type: got '%s' and '%s'", left_type->name,
					   right_type->name);
			return NULL;
		}

		Type *bool_type = shget(sema->types, "bool");
		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_LOGICAL_AND_EXPRESSION:
	case NODE_LOGICAL_OR_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		Type *bool_type = shget(sema->types, "bool");
		if (left_type != bool_type || right_type != bool_type) {
			type_error(node, "logical operation requires boolean operands, got '%s' and '%s'", left_type->name,
					   right_type->name);
			return NULL;
		}

		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_UNARY_EXPRESSION: {
		Node *operand = get_first_child(node);
		Type *op_type = validate_type(sema, operand);
		if (!op_type)
			return NULL;

		Op op = node->data.op;

		// Check operation validity based on operator
		switch (op) {
		case minus:
			// Unary minus requires numeric type
			if (op_type->kind != TYPE_PRIM) {
				type_error(node, "unary minus requires numeric type, got '%s'", op_type->name);
				return NULL;
			}
			break;
		case log_not:
			// Logical NOT requires boolean
			if (op_type != shget(sema->types, "bool")) {
				type_error(node, "logical not requires boolean, got '%s'", op_type->name);
				return NULL;
			}
			break;
		case bit_not:
			// Bitwise NOT requires integer type
			if (op_type->kind != TYPE_PRIM) {
				type_error(node, "bitwise not requires integer type, got '%s'", op_type->name);
				return NULL;
			}
			// Check it's not a float
			char *type_name = op_type->name;
			if (type_name[0] == 'f') {
				type_error(node, "bitwise not cannot be used on float, got '%s'", op_type->name);
				return NULL;
			}
			break;
		case star: // dereference
			if (op_type->kind != TYPE_POINTER) {
				type_error(node, "dereference requires pointer type, got '%s'", op_type->name);
				return NULL;
			}
			// Result type is the base type of the pointer
			node->resolved_type = op_type->data.pointer.base;
			return op_type->data.pointer.base;
		case and_perc: // address-of
			// Can take address of anything
			// Result type is pointer to operand type
			// We need to create or find the pointer type
			{
				char *ptr_name = malloc(strlen(op_type->name) + 2);
				sprintf(ptr_name, "*%s", op_type->name);
				Type *ptr_type = shget(sema->types, ptr_name);
				free(ptr_name);
				if (!ptr_type) {
					// Pointer type should have been created during type resolution
					type_error(node, "pointer type not found for '%s'", op_type->name);
					return NULL;
				}
				node->resolved_type = ptr_type;
				return ptr_type;
			}
		default:
			type_error(node, "unknown unary operator");
			return NULL;
		}

		node->resolved_type = op_type;
		return op_type;
	}

	case NODE_INC_DEC: {
		Node *operand = get_first_child(node);
		Type *op_type = validate_type(sema, operand);
		if (!op_type)
			return NULL;

		// Increment/decrement requires numeric type
		if (op_type->kind != TYPE_PRIM) {
			type_error(node, "increment/decrement requires numeric type, got '%s'", op_type->name);
			return NULL;
		}

		// Also check it's assignable (variable, not constant)
		if (operand->type == NODE_IDENTIFER) {
			char *name = slice_string(operand->data.literal);
			Symbol *sym = lookup_symbol(sema->current_scope, name);
			free(name);
			if (sym && sym->kind == SYM_CONSTANT) {
				type_error(node, "cannot increment/decrement constant '%s'", sym->name);
				return NULL;
			}
		}

		node->resolved_type = op_type;
		return op_type;
	}

	case NODE_ACCESS: {
		Node *obj = get_first_child(node);
		Node *field = get_next_sibling(obj);

		Type *obj_type = validate_type(sema, obj);
		if (!obj_type)
			return NULL;

		Type *actual_obj_type = obj_type;
		if (obj_type->kind == TYPE_POINTER) {
			actual_obj_type = obj_type->data.pointer.base;
		}

		if (actual_obj_type->kind != TYPE_OBJ) {
			type_error(node, "field access on non-object type '%s'", actual_obj_type->name);
			return NULL;
		}

		char *field_name = slice_string(field->data.literal);

		Scope *obj_scope = actual_obj_type->scope;
		if (!obj_scope) {
			type_error(node, "object '%s' has no scope", actual_obj_type->name);
			free(field_name);
			return NULL;
		}

		Symbol *field_sym = lookup_symbol_local(obj_scope, field_name);

		if (!field_sym) {
			type_error(node, "field '%s' not found in object '%s'", field_name, actual_obj_type->name);
			return NULL;
		}
		free(field_name);

		node->resolved_symbol = field_sym;
		node->resolved_type = field_sym->type;
		return field_sym->type;
	}

	case NODE_FUNC_CALL: {
		Node *callee = get_first_child(node);
		Node *args = get_next_sibling(callee);

		Type *func_type = validate_type(sema, callee);
		if (!func_type)
			return NULL;

		if (func_type->kind != TYPE_FUNC) {
			type_error(node, "calling non-function type '%s'", func_type->name);
			return NULL;
		}

		// Validate arguments match parameters
		size_t arg_count = get_child_count(args);
		if (arg_count != func_type->data.func.param_count) {
			type_error(node, "wrong number of arguments: expected %zu, got %zu", func_type->data.func.param_count,
					   arg_count);
			return NULL;
		}

		Node *arg = get_first_child(args);
		for (size_t i = 0; i < arg_count && arg; i++) {
			Type *arg_type = validate_type(sema, arg);
			if (!arg_type)
				return NULL;

			Type *param_type = func_type->data.func.params[i];
			if (arg_type != param_type) {
				type_error(node, "argument %zu type mismatch: expected '%s', got '%s'", i + 1, param_type->name,
						   arg_type->name);
				return NULL;
			}
			arg = get_next_sibling(arg);
		}

		node->resolved_type = func_type->data.func.return_type;
		return func_type->data.func.return_type;
	}

	case NODE_INDEX: {
		// Array/slice indexing: arr[index] or slice[from:to]
		Node *obj = get_first_child(node);
		Node *index = get_next_sibling(obj);
		Node *slice_end = get_next_sibling(index); // optional for slicing

		Type *obj_type = validate_type(sema, obj);
		if (!obj_type)
			return NULL;

		// Object must be a slice or pointer to slice
		Type *actual_type = obj_type;
		if (obj_type->kind == TYPE_POINTER) {
			actual_type = obj_type->data.pointer.base;
		}

		if (actual_type->kind != TYPE_SLICE) {
			type_error(node, "indexing non-slice type '%s'", actual_type->name);
			return NULL;
		}

		if (slice_end) {
			// Slicing: arr[start:end]
			Type *start_type = validate_type(sema, index);
			if (!start_type)
				return NULL;

			Type *end_type = validate_type(sema, slice_end);
			if (!end_type)
				return NULL;

			// Both must be integers
			Type *int_type = shget(sema->types, "i64");
			if (start_type != int_type || end_type != int_type) {
				type_error(node, "slice indices must be integers");
				return NULL;
			}

			// Result is same type as the slice
			node->resolved_type = obj_type;
			return obj_type;
		} else {
			// Single index: arr[i]
			Type *index_type = validate_type(sema, index);
			if (!index_type)
				return NULL;

			Type *int_type = shget(sema->types, "i64");
			if (index_type != int_type) {
				type_error(node, "array index must be integer, got '%s'", index_type->name);
				return NULL;
			}

			// Result is the element type of the slice
			node->resolved_type = actual_type->data.slice.element;
			return actual_type->data.slice.element;
		}
	}

	case NODE_CAST_EXPRESSION: {
		Node *type_node = get_first_child(node);
		Node *expr = get_next_sibling(type_node);

		Type *target_type = resolve_type(sema, type_node);
		if (!target_type) {
			type_error(node, "unknown target type in cast");
			return NULL;
		}

		Type *expr_type = validate_type(sema, expr);
		if (!expr_type)
			return NULL;

		// Casts require both types to be primitive numeric types for now
		if (expr_type->kind != TYPE_PRIM || target_type->kind != TYPE_PRIM) {
			type_error(node, "cast only supports primitive types, got '%s' -> '%s'", expr_type->name,
					   target_type->name);
			return NULL;
		}

		node->resolved_type = target_type;
		return target_type;
	}

	case NODE_SIZE_OF_EXPRESSION: {
		Node *type_node = get_first_child(node);
		Type *type = resolve_type(sema, type_node);
		if (!type) {
			type_error(node, "unknown type in sizeof");
			return NULL;
		}

		// sizeof returns usize
		Type *usize_type = shget(sema->types, "usize");
		node->resolved_type = usize_type;
		return usize_type;
	}

	case NODE_GROUPED_EXPRESSION: {
		Node *expr = get_first_child(node);
		Type *expr_type = validate_type(sema, expr);
		if (!expr_type)
			return NULL;

		node->resolved_type = expr_type;
		return expr_type;
	}

	// ============ LOOPS ============
	case NODE_WHILE_LOOP: {
		Node *condition = get_first_child(node);
		Node *block = get_next_sibling(condition);

		Type *cond_type = validate_type(sema, condition);
		if (!cond_type)
			return NULL;

		Type *bool_type = shget(sema->types, "bool");
		if (cond_type != bool_type) {
			type_error(node, "while condition must be boolean, got '%s'", cond_type->name);
			return NULL;
		}

		validate_type(sema, block);
		node->resolved_type = shget(sema->types, "void");
		return node->resolved_type;
	}

	case NODE_FOR_LOOP: {
		Node *init = get_first_child(node);
		Node *condition = get_next_sibling(init);
		Node *update = get_next_sibling(condition);
		Node *block = get_next_sibling(update);

		validate_type(sema, init);

		Type *cond_type = validate_type(sema, condition);
		if (!cond_type)
			return NULL;

		Type *bool_type = shget(sema->types, "bool");
		if (cond_type != bool_type) {
			type_error(node, "for condition must be boolean, got '%s'", cond_type->name);
			return NULL;
		}

		validate_type(sema, update);
		validate_type(sema, block);

		node->resolved_type = shget(sema->types, "void");
		return node->resolved_type;
	}

	case NODE_RANGE_FOR_LOOP: {
		Node *ident = get_first_child(node);
		Node *expr = get_next_sibling(ident);
		Node *block = get_next_sibling(expr);
		Type *range_type = validate_type(sema, expr);
		if (!range_type)
			return NULL;
		if (range_type->kind != TYPE_SLICE) {
			type_error(node, "range for requires slice type, got '%s'", range_type->name);
			return NULL;
		}
		Type *elem_type = range_type->data.slice.element;
		char *name = slice_string(ident->data.literal);
		Symbol *sym = lookup_symbol(sema->current_scope, name);
		free(name);
		if (sym) {
			if (sym->type != elem_type) {
				type_error(node, "range variable type mismatch: expected '%s', got '%s'", elem_type->name,
						   sym->type->name);
				return NULL;
			}
		}
		sema->current_scope = block->scope;
		validate_type(sema, block);
		node->resolved_type = shget(sema->types, "void");
		return node->resolved_type;
	}
	case NODE_OBJ_DECL: {
		Node *visablity = get_first_child(node);
		Node *identfier = get_next_sibling(visablity);
		for (Node *child = get_first_child(identfier); child; child = get_next_sibling(child)) {
			validate_type(sema, child);
		}
		node->resolved_type = shget(sema->types, "void");
		return node->resolved_type;
	}
	case NODE_FUNC_DECL: {
		sema->current_scope = node->scope;
		validate_type(sema, get_first_child_of_type(node, NODE_BLOCK));
		return shget(sema->types, "void");
		break;
	}
	case NODE_UNION_DECL:
	case NODE_ENUM_DECL:
	case NODE_MODULE_DECL: {
		return shget(sema->types, "void");
		break;
	}

	default: {
		for (Node *child = get_first_child(node); child; child = get_next_sibling(child)) {
			validate_type(sema, child);
		}
		node->resolved_type = shget(sema->types, "void");
		return node->resolved_type;
	}
	}
}

void type_check(Sema *sema) {
	bool has_errors = false;
	Node *root = sema->root;
	for (Node *curr = get_first_child(root); curr; curr = get_next_sibling(curr)) {
		Type *type = validate_type(sema, curr);
		if (!type) {
			has_errors = true;
			print_node_inline(curr, sema->source);
			printf(" -> ERROR\n");
		}
	}
	if (has_errors) {
		printf("\nType checking completed with errors.\n");
	} else {
		printf("Type checking completed successfully.\n");
	}
}
void free_scopes(Node *root) {
	if (!root)
		return;
	for (Node *curr = get_first_child(root); curr; curr = get_next_sibling(curr)) {
		free_scopes(curr);
	}
	if (root->scope) {
		if (root->scope->symbols) {
			free_symbol_table(&(root->scope->symbols));
		}
		free(root->scope);
		root->scope = NULL;
	}
}

bool analyize(Node *root, char *source) {
	Sema sema = {0};
	sema.source = source;
	sema.root = root;
	build_type_table(&sema);
	global_symbol_registration(&sema);
	sema.current_scope = sema.global;
	sema.root->scope = sema.global;
	build_scopes(&sema, root);
	type_check(&sema);
	free_type_table(&sema.types);
	free_scopes(root);
	return false;
}
