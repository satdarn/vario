#include "../type/types.h"

Type *resolve_type(Arena *arena, TypeTable *tbl, Node *type_node) {
	if (!type_node)
		return NULL;
	switch (type_node->type) {
	case NODE_PRIMITIVE_TYPE: {
		const char *name = primitive_name(type_node->data.primitive_type);
		Type *t = shget(*tbl, name);
		return t;
	}

	case NODE_POINTER_TYPE: {
		Type *inner = resolve_type(arena, tbl, get_first_child(type_node));
		if (!inner)
			return NULL;
		size_t len = strlen(inner->name) + 2;
		char *name = alloc(arena, len);
		snprintf(name, len, "*%s", inner->name);
		Type *existing = shget(*tbl, name);
		if (existing) {

			return existing;
		}
		Type *t = alloc(arena, sizeof(Type));
		t->kind = TYPE_POINTER;
		t->name = name;
		t->data.pointer.base = inner;
		shput(*tbl, name, t);
		return t;
	}

	case NODE_SLICE_TYPE: {
		Type *elem = resolve_type(arena, tbl, get_first_child(type_node));
		if (!elem)
			return NULL;
		size_t len = strlen(elem->name) + 3;
		char *name = alloc(arena, len);
		snprintf(name, len, "[]%s", elem->name);
		Type *existing = shget(*tbl, name);
		if (existing) {
			return existing;
		}
		Type *t = alloc(arena, sizeof(Type));
		t->kind = TYPE_SLICE;
		t->name = name;
		t->data.slice.element = elem;
		shput(*tbl, name, t);
		return t;
	}

	case NODE_OBJ_TYPE:
	case NODE_ENUM_TYPE:
	case NODE_UNION_TYPE: {
		char *name = slice_string(arena, type_node->data.literal);
		Type *t = shget(*tbl, name);
		return t;
	}

	default:
		return NULL;
	}
}

Type *resolve_decl_type(Arena *arena, TypeTable *tbl, Node *decl_node) {
	if (!decl_node)
		return NULL;
	if (decl_node->resolved_type) {
		return decl_node->resolved_type;
	}

	switch (decl_node->type) {
	case NODE_CONST_DECL: {
		Node *ident = get_first_child_of_type(decl_node, NODE_IDENTIFER);
		Node *type_ann = get_next_sibling(ident);
		return resolve_type(arena, tbl, type_ann);
	}
	case NODE_METHOD_DECL:
	case NODE_FUNC_DECL: {
		Type **param_types = NULL;
		Node *param_list = get_first_child_of_type(decl_node, NODE_PARAMETER_LIST);
		if (!param_list)
			printf("NO PARMATER LIST\n");
		for (Node *param = get_first_child(param_list); param;
			 param = get_next_sibling(param)) {
			Node *ident = get_first_child(param);
			Node *type_ann = get_next_sibling(ident);
			Type *pt = resolve_type(arena, tbl, type_ann);
			if (!pt) {
				char *str = slice_string(arena, ident->data.literal);
				printf("sema: parameter '%s' has unknown type at %d:%d\n", str,
					   type_ann ? type_ann->line : 0, type_ann ? type_ann->col : 0);

				arrfree(param_types);
				return NULL;
			}
			arrpush(param_types, pt);
		}

		Node *ret_node = get_first_child_of_type(decl_node, NODE_RETURN_TYPE);
		Type *ret = ret_node ? resolve_type(arena, tbl, get_first_child(ret_node)) : NULL;
		if (!ret)
			ret = shget(*tbl, "void");

		size_t cap = 256;
		char *name = alloc(arena, cap);
		strcpy(name, "fn(");
		size_t n = (size_t) arrlen(param_types);
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

			arrfree(param_types);
			return existing;
		}

		Type *ft = alloc(arena, sizeof(Type));
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

void print_type_table(TypeTable table) {
	int len = shlen(table);
	printf("\n╔══════════════════════════════════════════════════════════════════"
		   "╗\n");
	printf("║                         TYPE TABLE                              "
		   "║\n");
	printf("╠══════════════════════════════════════════════════════════════════"
		   "╣\n");
	printf("║ %-30s │ %-10s │ %-20s ║\n", "Type Name", "Kind", "Details");
	printf("╠══════════════════════════════════════════════════════════════════"
		   "╣\n");

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
			snprintf(details, sizeof(details), "%zu variants",
					 t->data.enum_.variant_count);
			break;
		case TYPE_UNION:
			kind_str = "Union";
			snprintf(details, sizeof(details), "%zu variants",
					 t->data.union_.variant_count);
			break;
		case TYPE_POINTER:
			kind_str = "Pointer";
			snprintf(details, sizeof(details), "-> %s",
					 t->data.pointer.base ? t->data.pointer.base->name : "?");
			break;
		case TYPE_SLICE:
			kind_str = "Slice";
			snprintf(details, sizeof(details), "[]%s",
					 t->data.slice.element ? t->data.slice.element->name : "?");
			break;
		case TYPE_FUNC:
			kind_str = "Function";
			snprintf(details, sizeof(details), "(%zu params) -> %s",
					 t->data.func.param_count,
					 t->data.func.return_type ? t->data.func.return_type->name : "?");
			break;
		default:
			kind_str = "Unknown";
			snprintf(details, sizeof(details), "???");
			break;
		}

		printf("║ %-30s │ %-10s │ %-20s ║\n", t->name, kind_str, details);
	}
	printf("╚══════════════════════════════════════════════════════════════════"
		   "╝\n");
	printf("Total types: %d\n\n", len);
}

void type_error(Node *node, const char *format, ...) {
	va_list args;
	va_start(args, format);
	fprintf(stderr, "type error at %d:%d: ", node->line, node->col);
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
	va_end(args);
}

static void free_type(Type *t) {
	if (!t)
		return;
	switch (t->kind) {
	case TYPE_PRIM:
		break;
	case TYPE_OBJ:
		for (int i = 0; i < (int) arrlen(t->data.obj.fields); i++)
			arrfree(t->data.obj.fields);
		break;
	case TYPE_UNION:
		for (int i = 0; i < (int) arrlen(t->data.union_.variants); i++)
			arrfree(t->data.union_.variants);
		break;
	case TYPE_ENUM:
		for (int i = 0; i < (int) arrlen(t->data.enum_.variants); i++)
			arrfree(t->data.enum_.variants);
		break;
	case TYPE_FUNC:
		arrfree(t->data.func.params);
		break;
	case TYPE_POINTER:
	case TYPE_SLICE:
	case TYPE_NON:
		break;
	}
}

void free_type_table(TypeTable *table) {
	if (!table || !*table)
		return;
	int len = shlen(*table);
	for (int i = 0; i < len; i++)
		free_type((*table)[i].value);
	shfree(*table);
}
