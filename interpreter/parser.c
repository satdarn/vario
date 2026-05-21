#include "parser.h"
#include "nodes.h"

int main() {
	return 0;
}
Node *parse(char *data) {
	Tokens tokens = { 0 };
	tokens.data = data;
	tokens.size = strlen(data);
	return parse_program(&tokens);
}

Node *parse_program(Tokens *tokens) {
	Node *root = create_node(NODE_PROGRAM, "Root");
	Node *curr = parse_top_level_decl(tokens);
	while (curr) {
		append_child(root, curr);
		curr = parse_top_level_decl(tokens);
	}
	return curr;
}

Node *parse_top_level_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_func_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_obj_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_enum_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_union_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_const_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_module_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_import_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	return NULL;
}

Node *parse_module_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "module")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "module");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		destroy_node(identifier);
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_MODULE_DECL, "");
	append_child(curr, identifier);
	return curr;
}

Node *parse_import_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "import")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "import");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_IMPORT_DECL, "");
	if (consume_if(tokens, ';')) {
		append_child(curr, identifier);
		return curr;
	}
	if (!peek_string(tokens, "as")) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	consume_string(tokens, "as");
	identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	append_child(curr, identifier);
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_visiblity(Tokens *tokens) {
	Node *curr = create_node(NODE_VISIBLITY, "");
	curr->data.visiblity = false;
	if (peek_string(tokens, "pub")) {
		consume_string(tokens, "pub");
		curr->data.visiblity = true;
	}
	return curr;
}

Node *parse_func_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_string(tokens, "fn")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '(')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *parameter_list = parse_parameter_list(tokens);
	if (!consume_if(tokens, ')')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *return_type = parse_return_type(tokens);
	if (!return_type) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(parameter_list);
		destroy_node(return_type);
		return NULL;
	}
	Node *curr = create_node(NODE_FUNC_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, parameter_list);
	append_child(curr, return_type);
	append_child(curr, block);
	return curr;
}

Node *parse_parameter_list(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = create_node(NODE_PARAMETER_LIST, "");
	Node *node = parse_parameter(tokens);
	while (node) {
		append_child(curr, node);
		if (!consume_if(tokens, ',')) {
			break;
		}
		node = parse_parameter(tokens);
	}
	if (!curr->first_child) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_parameter(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *identifier = parse_identifier(tokens);
	if (!identifier)
		return NULL;
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node(NODE_PARAMETER, "");
	append_child(curr, identifier);
	append_child(curr, type);
	return curr;
}

Node *parse_return_type(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "->")) {
		return NULL;
	}
	consume_string(tokens, "->");
	Node *type = parse_type(tokens);
	if (!type) {
		tokens->position = start_pos;
	}
	type->type = NODE_RETURN_TYPE;
	return type;
}

Node *parse_block(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!consume_if(tokens, '{')) {
		return NULL;
	}
	Node *curr = create_node(NODE_BLOCK, "");
	Node *node = parse_statement(tokens);
	while (node) {
		append_child(curr, node);
		node = parse_statement(tokens);
	}
	if (!consume_if(tokens, '}')) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_type(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_primitive_type(tokens);
	if (curr) {
		return curr;
	}
	curr = parse_pointer_type(tokens);
	if (curr) {
		return curr;
	}
	curr = parse_slice_type(tokens);
	if (curr) {
		return curr;
	}
	/* #TODO
	 * Currently there is no check if an identifier is either and
	 * obj, enum or union... in terms of types.
	 * if we build a symbol table of sorts for theses declared types
	 * at parse time all of these would have to be forward declared...
	 * i dont like that. it can be remedied with a second pass when
	 * we make the symbol table. so when we make the symbol table,
	 * we will have to traverse the tree and change the types from
	 * obj to either obj enum or union... this does mean that
	 * parse_enum_type and parse_union_type are dead code...
	 */
	curr = parse_obj_type(tokens);
	if (curr) {
		return curr;
	}
	curr = parse_enum_type(tokens);
	if (curr) {
		return curr;
	}
	curr = parse_union_type(tokens);
	if (curr) {
		return curr;
	}
	return NULL;
}

Node *parse_primitive_type(Tokens *tokens) {
	Node *curr = create_node(NODE_PRIMITIVE_TYPE, "");
	if (peek_string(tokens, "u8")) {
		consume_string(tokens, "u8");
		curr->data.primitive_type = u8;
		return curr;
	}
	if (peek_string(tokens, "u32")) {
		consume_string(tokens, "u32");
		curr->data.primitive_type = u32;
		return curr;
	}
	if (peek_string(tokens, "u64")) {
		consume_string(tokens, "u64");
		curr->data.primitive_type = u64;
		return curr;
	}
	if (peek_string(tokens, "i32")) {
		consume_string(tokens, "i32");
		curr->data.primitive_type = i32;
		return curr;
	}
	if (peek_string(tokens, "i64")) {
		consume_string(tokens, "i64");
		curr->data.primitive_type = i64;
		return curr;
	}
	if (peek_string(tokens, "f32")) {
		consume_string(tokens, "f32");
		curr->data.primitive_type = f32;
		return curr;
	}
	if (peek_string(tokens, "f32")) {
		consume_string(tokens, "f32");
		curr->data.primitive_type = f32;
		return curr;
	}
	if (peek_string(tokens, "bool")) {
		consume_string(tokens, "bool");
		curr->data.primitive_type = boolean;
		return curr;
	}
	if (peek_string(tokens, "void")) {
		consume_string(tokens, "void");
		curr->data.primitive_type = voidian;
		return curr;
	}
	if (peek_string(tokens, "usize")) {
		consume_string(tokens, "usize");
		curr->data.primitive_type = usize;
		return curr;
	}
	if (peek_string(tokens, "isize")) {
		consume_string(tokens, "isize");
		curr->data.primitive_type = isize;
		return curr;
	}
	destroy_node(curr);
	return NULL;
}

Node *parse_pointer_type(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!consume_if(tokens, '*')) {
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		return NULL;
	}
	Node *curr = create_node(NODE_POINTER_TYPE, "");
	append_child(curr, type);
	return curr;
}

Node *parse_slice_type(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!consume_if(tokens, '[')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *integer = parse_integer_literal(tokens);
	if (!consume_if(tokens, ']')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		tokens->position = start_pos;
		destroy_node(integer);
		return NULL;
	}
	Node *curr = create_node(NODE_SLICE_TYPE, "");
	append_child(curr, type);
	append_child(curr, integer);
	return curr;
}

Node *parse_obj_type(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	identifier->type = NODE_OBJ_TYPE;
	return identifier;
}

Node *parse_enum_type(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	identifier->type = NODE_ENUM_TYPE;
	return identifier;
}

Node *parse_union_type(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	identifier->type = NODE_UNION_TYPE;
	return identifier;
}

Node *parse_obj_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visibility = parse_visiblity(tokens);
	if (!peek_string(tokens, "obj")) {
		tokens->position = start_pos;
		destroy_node(visibility);
		return NULL;
	}
	consume_string(tokens, "obj");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		destroy_node(visibility);
		return NULL;
	}
	if (!consume_if(tokens, '{')) {
		tokens->position = start_pos;
		destroy_node(visibility);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node(NODE_OBJ_DECL, "");
	Node *node = parse_obj_member(tokens);
	while (node) {
		append_child(curr, node);
		node = parse_obj_member(tokens);
	}
	if (!consume_if(tokens, '}')) {
		tokens->position = start_pos;
		destroy_node(visibility);
		destroy_node(identifier);
		destroy_node(curr);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(visibility);
		destroy_node(identifier);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_obj_member(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_field_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_constructor_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_destructor_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_method_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	return NULL;
}

Node *parse_field_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	Node *var_decl = parse_var_decl(tokens);
	if (!var_decl) {
		destroy_node(visiblity);
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		destroy_node(visiblity);
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_FIELD_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, var_decl);
	return curr;
}

Node *parse_method_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_string(tokens, "fn")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '(')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *self_param = parse_self_param(tokens);
	Node *parameter_list = NULL;
	if (self_param) {
		if (consume_if(tokens, ',')) {
			parameter_list = parse_parameter_list(tokens);
			if (!parameter_list) {
				tokens->position = start_pos;
				destroy_node(visiblity);
				destroy_node(identifier);
				destroy_node(self_param);
				return NULL;
			}
		} else {
			parameter_list = parse_parameter_list(tokens);
		}
	} else {
		parameter_list = parse_parameter_list(tokens);
	}

	if (!consume_if(tokens, ')')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *return_type = parse_return_type(tokens);
	if (!return_type) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		destroy_node(return_type);
		return NULL;
	}
	Node *curr = create_node(NODE_METHOD_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, self_param);
	append_child(curr, parameter_list);
	append_child(curr, return_type);
	append_child(curr, block);
	return curr;
}

Node *parse_self_param(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "self")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "self");
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, '*')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *obj_type = parse_obj_type(tokens);
	if (!obj_type) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_SELF_PARAM, "");
	append_child(curr, obj_type);
	return curr;
}

Node *parse_constructor_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_string(tokens, "fn")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	if (!peek_string(tokens, "init")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "init");
	if (!consume_if(tokens, '(')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	Node *parameter_list = parse_parameter_list(tokens);

	if (!consume_if(tokens, ')')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *curr = create_node(NODE_CONSTRUCTOR_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, parameter_list);
	append_child(curr, block);
	return curr;
}

Node *parse_destructor_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_string(tokens, "fn")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	if (!peek_string(tokens, "deinit")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "deinit");
	if (!consume_if(tokens, '(')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	Node *self_param = parse_self_param(tokens);

	if (!consume_if(tokens, ')')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(self_param);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(self_param);
		return NULL;
	}
	Node *curr = create_node(NODE_DESTRUCTOR_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, self_param);
	append_child(curr, block);
	return curr;
}

Node *parse_enum_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_string(tokens, "enum")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "enum");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '{')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node(NODE_ENUM_DECL, "");
	Node *node = parse_enum_variant(tokens);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	while (node) {
		append_child(curr, node);
		node = parse_enum_variant(tokens);
	}
	if (!consume_if(tokens, '}')) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_enum_variant(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *integer = NULL;
	if (consume_if(tokens, '=')) {
		integer = parse_integer_literal(tokens);
		if (!integer) {
			tokens->position = start_pos;
			destroy_node(identifier);
			return NULL;
		}
	}
	consume_if(tokens, ',');
	Node *curr = create_node(NODE_ENUM_VARIANT, "");
	append_child(curr, identifier);
	append_child(curr, integer);
	return curr;
}

Node *parse_union_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_string(tokens, "union")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "union");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '{')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node(NODE_UNION_DECL, "");
	Node *node = parse_enum_variant(tokens);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	while (node) {
		append_child(curr, node);
		node = parse_union_variant(tokens);
	}

	if (!consume_if(tokens, '}')) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_const_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_string(tokens, "const")) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "const");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	if (!consume_if(tokens, '=')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node(NODE_CONST_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, type);
	append_child(curr, expression);
	return curr;
}

Node *parse_var_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "var")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "var");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	if (!consume_if(tokens, '=')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		tokens->position = start_pos;
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		destroy_node(type);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node(NODE_VAR_DECL, "");
	append_child(curr, identifier);
	append_child(curr, type);
	append_child(curr, expression);
	return curr;
}
Node *parse_let_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "let")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "let");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	if (!consume_if(tokens, '=')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		tokens->position = start_pos;
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		destroy_node(type);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node(NODE_LET_DECL, "");
	append_child(curr, identifier);
	append_child(curr, type);
	append_child(curr, expression);
	return curr;
}

Node *parse_statement(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = NULL;
	curr = parse_declaration_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_assignment_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_expression_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_conditional_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_loop_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_switch_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_return_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_break_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_continue_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_defer_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_block_stmt(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	return NULL;
}

Node *parse_declaration_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_var_decl(tokens);
	if (!curr) {
		curr = parse_let_decl(tokens);
	}
	if (!curr) {
		tokens->position = start_pos;
		return NULL;
	}
	return curr;
}

Node *parse_assignment_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *left_expression = parse_expression(tokens);
	if (!left_expression) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *assignment = parse_assignment_operator(tokens);
	if (!assignment) {
		tokens->position = start_pos;
		destroy_node(left_expression);
		return NULL;
	}
	Node *right_expression = parse_expression(tokens);
	if (!right_expression) {
		tokens->position = start_pos;
		destroy_node(left_expression);
		destroy_node(assignment);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(left_expression);
		destroy_node(assignment);
		destroy_node(right_expression);
		return NULL;
	}
	Node *curr = create_node(NODE_ASSIGNMENT_STMT, "");
	append_child(curr, left_expression);
	append_child(curr, assignment);
	append_child(curr, right_expression);
	return curr;
}

Node *parse_assignment_operator(Tokens *tokens) {
	Node *curr = create_node(NODE_ASSIGNMENT_OPERATOR, "");
	if (peek_string(tokens, "=")) {
		curr->data.op = equals;
		consume_string(tokens, "=");
		return curr;
	}
	if (peek_string(tokens, "+=")) {
		curr->data.op = plus_equals;
		consume_string(tokens, "+=");
		return curr;
	}
	if (peek_string(tokens, "*=")) {
		curr->data.op = star_equals;
		consume_string(tokens, "*=");
		return curr;
	}
	if (peek_string(tokens, "/=")) {
		curr->data.op = slash_equals;
		consume_string(tokens, "/=");
		return curr;
	}
	if (peek_string(tokens, "%=")) {
		curr->data.op = percent_equals;
		consume_string(tokens, "%=");
		return curr;
	}
	if (peek_string(tokens, "&=")) {
		curr->data.op = and_equals;
		consume_string(tokens, "&=");
		return curr;
	}
	if (peek_string(tokens, "|=")) {
		curr->data.op = or_equals;
		consume_string(tokens, "|=");
		return curr;
	}
	if (peek_string(tokens, "^=")) {
		curr->data.op = xor_equals;
		consume_string(tokens, "^=");
		return curr;
	}
	if (peek_string(tokens, "<<=")) {
		curr->data.op = lshift_equals;
		consume_string(tokens, "<<=");
		return curr;
	}
	if (peek_string(tokens, ">>=")) {
		curr->data.op = rshift_equals;
		consume_string(tokens, ">>=");
		return curr;
	}
	destroy_node(curr);
	return NULL;
}

Node *parse_expression_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_expression(tokens);
	if (!curr) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		return NULL;
	}
	return curr;
}

Node *parse_return_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "return")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "return");
	Node *expression = parse_expression(tokens);
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node(NODE_RETURN_STMT, "");
	append_child(curr, expression);
	return curr;
}
Node *parse_break_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "break")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "break");
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_BREAK_STMT, "");
	return curr;
}
Node *parse_continue_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "continue")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "continue");
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_CONTINUE_STMT, "");
	return curr;
}

Node *parse_defer_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "defer")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "defer");
	Node *statement = parse_statement(tokens);
	if (!statement) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_DEFER_STMT, "");
	append_child(curr, statement);
	return curr;
}

Node *parse_block_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_block(tokens);
	if (!curr) {
		tokens->position = start_pos;
		return NULL;
	}
	return curr;
}

Node *parse_conditional_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "if")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "if");
	Node *expression = parse_expression(tokens);
	if (!expression) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node(NODE_CONDITIONAL_STMT, "");
	append_child(curr, expression);
	append_child(curr, block);
	if (peek_string(tokens, "else")) {
		consume_string(tokens, "else");
		Node *conditional_stmt = parse_conditional_stmt(tokens);
		if (conditional_stmt) {
			append_child(curr, conditional_stmt);
			return curr;
		}
		Node *else_block = parse_block(tokens);
		if (else_block) {
			append_child(curr, else_block);
			return curr;
		}
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_loop_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_while_loop(tokens);
	if (curr) {
		return curr;
	}
	tokens->position = start_pos;
	curr = parse_for_loop(tokens);
	if (curr) {
		return curr;
	}
	tokens->position = start_pos;
	curr = parse_range_for_loop(tokens);
	if (curr) {
		return curr;
	}
	tokens->position = start_pos;
	return NULL;
}

Node *parse_while_loop(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "while")) {
		return NULL;
	}
	consume_string(tokens, "while");
	Node *expression = parse_expression(tokens);
	if (!expression) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node(NODE_WHILE_LOOP, "");
	append_child(curr, expression);
	append_child(curr, block);
	return curr;
}

Node *parse_for_loop(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "for")) {
		return NULL;
	}
	consume_string(tokens, "for");
	Node *for_init = parse_for_init(tokens);
	if (!for_init) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(for_init);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		tokens->position = start_pos;
		destroy_node(for_init);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(for_init);
		destroy_node(expression);
		return NULL;
	}
	Node *for_update = parse_for_update(tokens);
	if (!for_update) {
		tokens->position = start_pos;
		destroy_node(for_init);
		destroy_node(expression);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(for_init);
		destroy_node(expression);
		destroy_node(for_update);
		return NULL;
	}
	Node *curr = create_node(NODE_FOR_LOOP, "");
	append_child(curr, for_init);
	append_child(curr, block);
	return curr;
}

Node *parse_for_init(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = create_node(NODE_FOR_INIT, "");
	Node *var_decl = parse_var_decl(tokens);
	if (var_decl) {
		append_child(curr, var_decl);
		return curr;
	}
	tokens->position = start_pos;
	Node *assignment_stmt = parse_assignment_stmt(tokens);
	if (assignment_stmt) {
		append_child(curr, assignment_stmt);
		return curr;
	}
	tokens->position = start_pos;
	Node *expression_stmt = parse_expression_stmt(tokens);
	if (expression_stmt) {
		append_child(curr, expression_stmt);
		return curr;
	}
	tokens->position = start_pos;
	return NULL;
}

Node *parse_for_update(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = create_node(NODE_FOR_UPDATE, "");
	Node *assignment_stmt = parse_assignment_stmt(tokens);
	if (assignment_stmt) {
		append_child(curr, assignment_stmt);
		return curr;
	}
	tokens->position = start_pos;
	Node *expression_stmt = parse_expression_stmt(tokens);
	if (expression_stmt) {
		append_child(curr, expression_stmt);
		return curr;
	}
	tokens->position = start_pos;
	Node *expression = parse_expression(tokens);
	if (expression) {
		append_child(curr, expression);
		return curr;
	}
	tokens->position = start_pos;
	return NULL;
}

Node *parse_range_for_loop(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "for")) {
		return NULL;
	}
	consume_string(tokens, "for");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
	}
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		tokens->position = start_pos;
		destroy_node(identifier);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(identifier);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node(NODE_RANGE_FOR_LOOP, "");
	append_child(curr, identifier);
	append_child(curr, expression);
	append_child(curr, block);
	return curr;
}

Node *parse_switch_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "switch")) {
		return NULL;
	}
	consume_string(tokens, "switch");
	if (!consume_if(tokens, '{')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_SWITCH_STMT, "");
	Node *node = parse_switch_case(tokens);
	while (node) {
		append_child(curr, node);
		node = parse_switch_case(tokens);
	}
	Node *switch_default = parse_switch_default(tokens);
	append_child(curr, switch_default);
	if (!consume_if(tokens, '}')) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_switch_case(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "case")) {
		return NULL;
	}
	consume_string(tokens, "case");
	Node *case_pattern = parse_case_pattern(tokens);
	if (!case_pattern) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_SWITCH_CASE, "");
	while (case_pattern) {
		append_child(curr, case_pattern);
		if (!consume_if(tokens, ',')) {
			break;
		}
		case_pattern = parse_case_pattern(tokens);
	}
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	append_child(curr, block);
	return curr;
}

Node *parse_case_pattern(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *expression = parse_expression(tokens);
	if (expression) {
		return expression;
	}
	tokens->position = start_pos;
	Node *literal = parse_expression(tokens);
	if (literal) {
		return literal;
	}
	tokens->position = start_pos;
	Node *identifier = parse_expression(tokens);
	if (identifier) {
		return identifier;
	}
	tokens->position = start_pos;
	return NULL;
}

Node *parse_switch_default(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "default")) {
		return NULL;
	}
	consume_string(tokens, "default");
	if (!consume_if(tokens, ':')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_SWITCH_DEFAULT, "");
	append_child(curr, block);
	return curr;
}

Node *parse_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *logical_or_expression = parse_logical_or_expression(tokens);
	if (!logical_or_expression) {
		tokens->position = start_pos;
		return NULL;
	}
	return logical_or_expression;
}

Node *parse_logical_or_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *logical_and_expression = parse_logical_and_expression(tokens);
	if (!logical_and_expression) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!peek_string(tokens, "||")) {
		return logical_and_expression;
	}
	Node *curr = create_node(NODE_LOGICAL_OR_EXPRESSION, "");
	while (peek_string(tokens, "||")) {
		append_child(curr, logical_and_expression);
		consume_string(tokens, "||");
		logical_and_expression = parse_logical_and_expression(tokens);
		if (!logical_and_expression) {
			destroy_node(curr);
			destroy_node(logical_and_expression);
			return NULL;
		}
	}
	append_child(curr, logical_and_expression);
	return curr;
}

Node *parse_logical_and_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *equality_expression = parse_equality_expression(tokens);
	if (!equality_expression) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!peek_string(tokens, "&&")) {
		return equality_expression;
	}
	Node *curr = create_node(NODE_LOGICAL_AND_EXPRESSION, "");
	while (peek_string(tokens, "&&")) {
		append_child(curr, equality_expression);
		consume_string(tokens, "&&");
		equality_expression = parse_equality_expression(tokens);
		if (!equality_expression) {
			destroy_node(curr);
			destroy_node(equality_expression);
			return NULL;
		}
	}
	append_child(curr, equality_expression);
	return curr;
}

//equality_expression = relational_expression { ("==" | "!=") relational_expression } ;
Node *parse_equality_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *left = parse_relational_expression(tokens);
	if (!left) {
		tokens->position = start_pos;
		return NULL;
	}
	while (peek_string(tokens, "==") || peek_string(tokens, "!=")) {
		Node *curr = create_node(NODE_EQUALITY_EXPRESSION, "");
		if (peek_string(tokens, "==")) {
			consume_string(tokens, "==");
			curr->data.op = eq_equals;
		} else {
			consume_string(tokens, "!=");
			curr->data.op = not_equals;
		}
		Node *right = parse_relational_expression(tokens);
		if (!right) {
			destroy_node(left);
			destroy_node(curr);
			return NULL;
		}
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}

//relational_expression = additive_expression { ("<" | "<=" | ">" | ">=") additive_expression } ;
Node *parse_relational_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *left = parse_additive_expression(tokens);
	if (!left) {
		tokens->position = start_pos;
		return NULL;
	}
	while (peek_string(tokens, "<") || peek_string(tokens, "<=") || peek_string(tokens, ">") || peek_string(tokens, ">=")) {
		Node *curr = create_node(NODE_RELATIONAL_EXPRESSION, "");
		if (peek_string(tokens, "<")) {
			consume_string(tokens, "<");
			curr->data.op = eq_equals;
		} else if (peek_string(tokens, "<=")) {
			consume_string(tokens, "<=");
			curr->data.op = not_equals;
		} else if (peek_string(tokens, ">")) {
			consume_string(tokens, ">");
			curr->data.op = not_equals;
		} else if (peek_string(tokens, ">=")) {
			consume_string(tokens, ">=");
			curr->data.op = not_equals;
		}
		Node *right = parse_additive_expression(tokens);
		if (!right) {
			destroy_node(left);
			destroy_node(curr);
			return NULL;
		}
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}

//additive_expression = multiplicative_expression { ("+" | "-") multiplicative_expression } ;
Node *parse_additive_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *left = parse_multiplicative_expression(tokens);
	if (!left) {
		tokens->position = start_pos;
		return NULL;
	}
	while (peek_string(tokens, "+") || peek_string(tokens, "-")) {
		Node *curr = create_node(NODE_ADDITIVE_EXPRESSION, "");
		if (peek_string(tokens, "+")) {
			consume_string(tokens, "+");
			curr->data.op = plus;
		} else if (peek_string(tokens, "-")) {
			consume_string(tokens, "-");
			curr->data.op = minus;
		}
		Node *right = parse_multiplicative_expression(tokens);
		if (!right) {
			destroy_node(left);
			destroy_node(curr);
			return NULL;
		}
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}

//multiplicative_expression = unary_expression { ("*" | "/" | "%") unary_expression } ;
Node *parse_multiplicative_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *left = parse_unary_expression(tokens);
	if (!left) {
		tokens->position = start_pos;
		return NULL;
	}
	while (peek_string(tokens, "*") || peek_string(tokens, "/") || peek_string(tokens, "%")) {
		Node *curr = create_node(NODE_MULTIPLICTIVE_EXPRESSION, "");
		if (peek_string(tokens, "*")) {
			consume_string(tokens, "*");
			curr->data.op = star;
		} else if (peek_string(tokens, "/")) {
			consume_string(tokens, "/");
			curr->data.op = slash;
		} else if (peek_string(tokens, "%")) {
			consume_string(tokens, "%");
			curr->data.op = percent;
		}

		Node *right = parse_unary_expression(tokens);
		if (!right) {
			destroy_node(left);
			destroy_node(curr);
			return NULL;
		}
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}
//unary_expression = ("-" | "!" | "~" | "*" | "&") unary_expression
//                 | postfix_expression
//                 ;
Node *parse_unary_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	if (!peek_string(tokens, "-") &&
		!peek_string(tokens, "!") &&
		!peek_string(tokens, "~") &&
		!peek_string(tokens, "*") &&
		!peek_string(tokens, "&")) {
		Node *curr = create_node(NODE_UNARY_EXPRESSION, "");
		if (peek_string(tokens, "-")) {
			consume_string(tokens, "-");
			curr->data.op = minus;
		} else if (peek_string(tokens, "!")) {
			consume_string(tokens, "!");
			curr->data.op = log_not;
		} else if (peek_string(tokens, "~")) {
			curr->data.op = bit_not;
			consume_string(tokens, "~");
		} else if (peek_string(tokens, "*")) {
			curr->data.op = star;
			consume_string(tokens, "*");
		} else {
			curr->data.op = and_perc;
			consume_string(tokens, "&");
		}
		return curr;
	}
	return parse_postfix_expression(tokens);
}

//postfix_expression = primary_expression
//                   { "(" argument_list ")"      (* function call *)
//                   | "." identifier             (* field/method access *)
//                   | "[" expression "]"         (* indexing *)
//                   | "[" expression? ":" expression? "]"  (* slicing *)
//                   | "++" | "--"                (* postfix inc/dec *)
//                   } ;
Node *parse_postfix_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *left = parse_primary_expression(tokens);
	if (!left) {
		tokens->position = start_pos;
		return NULL;
	}
	while (1) {
		if (consume_if(tokens, '(')) {
			Node *args = parse_argument_list(tokens);
			if (consume_if(tokens, ')')) {
				destroy_node(args);
				destroy_node(left);
				tokens->position = start_pos;
				return NULL;
			}
			Node *curr = create_node(NODE_FUNC_CALL, "");
			append_child(curr, left);
			append_child(curr, args);
			left = curr;
			continue;
		}
		if (consume_if(tokens, '.')) {
			Node *identifier = parse_identifier(tokens);
			if (!identifier) {
				destroy_node(left);
				tokens->position = start_pos;
				return NULL;
			}
			Node *curr = create_node(NODE_ACCESS, "");
			append_child(curr, left);
			append_child(curr, identifier);
			left = curr;
			continue;
		}
		if (consume_if(tokens, '[')) {
			Node *curr = create_node(NODE_INDEX, "");
			append_child(curr, left);
			Node *expr = parse_expression(tokens);
			append_child(curr, expr);
			if (consume_if(tokens, ':')) {
				expr = parse_expression(tokens);
				append_child(curr, expr);
			}
			if (consume_if(tokens, ']')) {
				destroy_node(curr);
				tokens->position = start_pos;
				return NULL;
			}
			append_child(curr, expr);
			left = curr;
			continue;
		}
		if (peek_string(tokens, "++")) {
			consume_string(tokens, "++");
			Node *curr = create_node(NODE_INC_DEC, "");
			curr->data.op = plus_plus;
			append_child(left, curr);
			continue;
		}
		if (peek_string(tokens, "--")) {
			consume_string(tokens, "--");
			Node *curr = create_node(NODE_INC_DEC, "");
			curr->data.op = minus_minus;
			append_child(left, curr);
			continue;
		}
	}
}

//primary_expression = literal
//                   | identifier
//                   | "(" expression ")"
//                   | "sizeof" "(" type ")"
//                   | "(" type ")" expression    (* cast *)
//                   ;
Node *parse_primary_expression(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_literal(tokens);
	if (curr)
		return curr;
	curr = parse_identifier(tokens);
	tokens->position = start_pos;
	if (curr)
		return curr;
	tokens->position = start_pos;
	if (peek_string(tokens, "sizeof")) {
		consume_string(tokens, "sizeof");
		if (!consume_if(tokens, '(')) {
			tokens->position = start_pos;
			return NULL;
		}
		Node *type = parse_type(tokens);
		if (!type) {
			tokens->position = start_pos;
			return NULL;
		}
		if (!consume_if(tokens, ')')) {
			destroy_node(type);
			tokens->position = start_pos;
			return NULL;
		}
		curr = create_node(NODE_SIZE_OF_EXPRESSION, "");
		append_child(curr, type);
		return curr;
	}
	tokens->position = start_pos;
	if (consume_if(tokens, '(')) {
		Node *type = parse_type(tokens);
		if (type && consume_if(tokens, ')')) {
			Node *expr = parse_unary_expression(tokens);
			if (expr) {
				Node *cast = create_node(NODE_CAST_EXPRESSION, "");
				append_child(cast, type);
				append_child(cast, expr);
				return cast;
			}
		}
		tokens->position = start_pos;
		consume_if(tokens, '(');
		Node *expr = parse_expression(tokens);
		if (!expr) {
			tokens->position = start_pos;
			return NULL;
		}
		if (!consume_if(tokens, ')')) {
			destroy_node(expr);
			tokens->position = start_pos;
			return NULL;
		}
		Node *group = create_node(NODE_GROUPED_EXPRESSION, "");
		append_child(group, expr);
		return group;
	}
	return NULL;
}

//argument_list = [ expression { "," expression } ] ;
Node *parse_argument_list(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//literal = integer_literal
//        | float_literal
//        | string_literal
//        | boolean_literal
//        | array_literal
//        ;
Node *parse_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}
//integer_literal = decimal_literal | hex_literal | octal_literal | binary_literal ;
Node *parse_integer_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//float_literal = decimal_digits "." decimal_digits [ "e" [ "+" "-" ] decimal_digits ] ;
Node *parse_float_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//string_literal = '"' { string_char } '"' ;  (* yields []u8 *)
Node *parse_string_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}
//boolean_literal = "true" | "false" ;
Node *parse_boolean_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}
// array_literal = "[" [ expression { "," expression } ] "]" ;  (* slice literal *)
Node *parse_array_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//identifier = letter { letter | digit | "_" } ;
Node *parse_identifier(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//decimal_literal = decimal_digits ;
Node *parse_decimal_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//hex_literal = "0x" hex_digits ;
Node *parse_hex_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//octal_literal = "0o" octal_digits ;
Node *parse_octal_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//binary_literal = "0b" binary_digits ;
Node *parse_binary_literal(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//decimal_digits = digit { digit } ;
Node *parse_decimal_digits(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//hex_digits = hex_digit { hex_digit } ;
Node *parse_hex_digits(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//octal_digits = octal_digit { octal_digit } ;
Node *parse_octal_digits(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//binary_digits = binary_digit { binary_digit } ;
Node *parse_binary_digits(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//letter = "A" | "B" | ... | "Z" | "a" | "b" | ... | "z" ;
Node *parse_letter(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//digit = "0" | "1" | ... | "9" ;
Node *parse_digit(Tokens *tokens) {
	size_t start_pos = tokens->position;


//hex_digit = digit | "A" | "B" | "C" | "D" | "E" | "F" | "a" | "b" | "c" | "d" | "e" | "f" ;
Node *parse_hex_digit(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//octal_digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" ;
Node *parse_octal_digit(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//binary_digit = "0" | "1" ;
Node *parse_binary_digit(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//string_char = ? any character except " or newline, with escape sequences ? ;
Node *parse_string_char(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//comment = "//" { ? any character except newline ? } newline
//        | "/*" { ? any character ? } "*/"
//        ;
Node *parse_comment(Tokens *tokens) {
	size_t start_pos = tokens->position;
}

//whitespace = { space | tab | newline } ;
Node *parse_whitespace(Tokens *tokens) {
	size_t start_pos = tokens->position;
}
