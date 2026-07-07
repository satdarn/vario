#include "../type/types.h"

static TypeTable add_prim_type(Arena *arena, TypeTable table, const char *name,
							   PrimitiveType prim) {
	Type *t = alloc(arena, sizeof(Type));
	if (!t) {
		perror("add_prim_type: calloc");
		return table;
	}
	t->kind = TYPE_PRIM;
	t->name = (char *) name;
	t->data.prim = prim;
	shput(table, name, t);
	return table;
}

static TypeTable add_prim_types(Arena *arena, TypeTable table) {
	table = add_prim_type(arena, table, "u8", u8);
	table = add_prim_type(arena, table, "u32", u32);
	table = add_prim_type(arena, table, "u64", u64);
	table = add_prim_type(arena, table, "i32", i32);
	table = add_prim_type(arena, table, "i64", i64);
	table = add_prim_type(arena, table, "f32", f32);
	table = add_prim_type(arena, table, "f64", f64);
	table = add_prim_type(arena, table, "bool", boolean);
	table = add_prim_type(arena, table, "void", voidian);
	table = add_prim_type(arena, table, "usize", usize);
	table = add_prim_type(arena, table, "isize", isize);
	return table;
}

static Type *add_usr_type(Arena *arena, TypeTable *table, char *name, TypeKind kind) {
	Type *t = alloc(arena, sizeof(Type));
	if (!t) {
		perror("add_usr_type: calloc");
		return NULL;
	}
	t->kind = kind;
	t->name = name;
	shput(*table, name, t);
	return t;
}

static void fill_obj_fields(Arena *arena, TypeTable tbl, Type *obj_type,
							Node *decl_node) {
	uint32_t index = 0;
	for (Node *field = get_first_child_of_type(decl_node, NODE_FIELD_DECL); field;
		 field = get_next_sibling_of_type(field, NODE_FIELD_DECL)) {

		Node *var_decl = get_first_child_of_type(field, NODE_VAR_DECL);
		Node *ident = get_first_child_of_type(var_decl, NODE_IDENTIFER);
		Node *type_ann = get_next_sibling(ident);
		Type *field_type = resolve_type(arena, &tbl, type_ann);

		if (!field_type) {
			char *tname = slice_string(arena, type_ann->data.literal);
			printf("sema: unknown field type '%s' at %d:%d\n", tname, type_ann->line,
				   type_ann->col);
			free(tname);
			index++;
			continue;
		}
		Field f = {
			.name = slice_string(arena, ident->data.literal),
			.type = field_type,
			.index = index++,
		};
		arrpush(obj_type->data.obj.fields, f);
	}
	obj_type->data.obj.field_count = (size_t) arrlen(obj_type->data.obj.fields);
}

static void fill_union_variants(Arena *arena, TypeTable tbl, Type *union_type,
								Node *decl_node) {
	uint32_t tag = 0;
	for (Node *variant = get_first_child_of_type(decl_node, NODE_UNION_VARIANT); variant;
		 variant = get_next_sibling_of_type(variant, NODE_UNION_VARIANT)) {

		Node *ident = get_first_child(variant);
		Node *type_ann = get_next_sibling(ident);
		Type *var_type = resolve_type(arena, &tbl, type_ann);

		if (!var_type) {
			char *tname = slice_string(arena, type_ann->data.literal);
			printf("sema: unknown union variant type '%s' at %d:%d\n", tname,
				   type_ann->line, type_ann->col);
			free(tname);
			tag++;
			continue;
		}
		UnionVariant v = {
			.name = slice_string(arena, ident->data.literal),
			.type = var_type,
			.tag = tag++,
		};
		arrpush(union_type->data.union_.variants, v);
	}
	union_type->data.union_.variant_count =
		(size_t) arrlen(union_type->data.union_.variants);
}

static void fill_enum_variants(Arena *arena, Type *enum_type, Node *decl_node) {
	int64_t next_value = 0;
	Arena temp = {0};
	for (Node *variant = get_first_child_of_type(decl_node, NODE_ENUM_VARIANT); variant;
		 variant = get_next_sibling_of_type(variant, NODE_ENUM_VARIANT)) {

		Node *ident = get_first_child(variant);
		Node *value_node = get_next_sibling(ident);
		int64_t value;
		if (value_node) {
			char *val_str = slice_string(&temp, value_node->data.literal);
			value = strtoll(val_str, NULL, 10);
			next_value = value + 1;
		} else {
			value = next_value++;
		}
		EnumVariant v = {
			.name = slice_string(arena, ident->data.literal),
			.value = value,
		};
		arrpush(enum_type->data.enum_.variants, v);
	}
	destroy_arena(&temp);
}

TypeTable build_type_table(Arena *arena, Node *root) {
	TypeTable table = NULL;
	sh_new_strdup(table);
	table = add_prim_types(arena, table);

	for (Node *curr = get_first_child(root); curr; curr = get_next_sibling(curr)) {
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
		char *key = slice_string(arena, ident->data.literal);

		if (shget(table, key)) {
			printf("sema: type '%s' already defined (duplicate at %d:%d)\n", key,
				   curr->line, curr->col);
			continue;
		}

		Type *type = add_usr_type(arena, &table, key, kind);
		curr->resolved_type = type;
	}

	for (Node *curr = get_first_child(root); curr; curr = get_next_sibling(curr)) {
		Type *user_type = (Type *) curr->resolved_type;
		if (!user_type)
			continue;

		switch (user_type->kind) {
		case TYPE_OBJ:
			fill_obj_fields(arena, table, user_type, curr);
			break;
		case TYPE_UNION:
			fill_union_variants(arena, table, user_type, curr);
			break;
		case TYPE_ENUM:
			fill_enum_variants(arena, user_type, curr);
			break;
		default:
			break;
		}
	}
	return table;
}
