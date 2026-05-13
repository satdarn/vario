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
	if (peek_string(tokens, "pub")) {
		consume_string(tokens, "pub");
		curr->data.visiblity = true;
	}
	curr->data.visiblity = false;
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

Node *parse_deconstructor_decl(Tokens *tokens) {
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
	Node *curr = create_node(NODE_DECONSTRUCTOR_DECL, "");
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
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_assignment_stmt(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *left_expression = parse_expression(tokens);
	if (!left_expression) {
	}
	Node *assignment = parse_assignment_stmt(tokens);
	if (!assignment) {
	}
	Node *right_expression = parse_expression(tokens);
	if (!right_expression) {
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		return NULL;
	}
	Node *curr = create_node(NODE_ASSIGNMENT_STMT, "");
	append_child(curr, left_expression);
	append_child(curr, assignment);
	append_child(curr, right_expression);
}
