#include "parser.h"
#include "util.h"

Node *parse(char *data) {
	Tokens tokens = { 0 };
	tokens.line = 1;
	tokens.col = 0;
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
	return root;
}

Node *parse_top_level_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_func_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_obj_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_enum_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_union_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_const_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_module_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_import_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	return NULL;
}

Node *parse_module_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "module")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "module");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		destroy_node(identifier);
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_MODULE_DECL, "", cp);
	append_child(curr, identifier);
	return curr;
}

Node *parse_import_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "import")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "import");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_IMPORT_DECL, "", cp);
	append_child(curr, identifier);
	if (peek_keyword(tokens, "as")) {
		consume_string(tokens, "as");
		Node *alias = parse_identifier(tokens);
		if (alias) {
			append_child(curr, alias);
		}
	}
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_visiblity(Tokens *tokens) {
	Node *curr = create_node(NODE_VISIBLITY, "");
	curr->data.visiblity = false;
	if (peek_keyword(tokens, "pub")) {
		consume_string(tokens, "pub");
		curr->data.visiblity = true;
	}
	return curr;
}

Node *parse_func_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "fn")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '(')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *parameter_list = parse_parameter_list(tokens);
	if (!consume_if(tokens, ')')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *return_type = parse_return_type(tokens);
	if (!return_type) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(parameter_list);
		destroy_node(return_type);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_FUNC_DECL, "", cp);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, parameter_list);
	append_child(curr, return_type);
	append_child(curr, block);
	return curr;
}

Node *parse_parameter_list(Tokens *tokens) {
	Node *curr = create_node(NODE_PARAMETER_LIST, "");
	Node *node = parse_parameter(tokens);
	while (node) {
		append_child(curr, node);
		if (!consume_if(tokens, ','))
			break;
		node = parse_parameter(tokens);
	}
	return curr;
}

Node *parse_parameter(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_PARAMETER, "", cp);
	append_child(curr, identifier);
	append_child(curr, type);
	return curr;
}

Node *parse_return_type(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_string(tokens, "->")) {
		return NULL;
	}
	consume_string(tokens, "->");
	Node *type = parse_type(tokens);
	if (!type) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_RETURN_TYPE, "", cp);
	append_child(curr, type);
	return curr;
}

Node *parse_block(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!consume_if(tokens, '{')) {
		return NULL;
	}
	Node *curr = create_node_cp(NODE_BLOCK, "", cp);
	Node *node = parse_statement(tokens);
	while (node) {
		append_child(curr, node);
		node = parse_statement(tokens);
	}
	if (!consume_if(tokens, '}')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_type(Tokens *tokens) {
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
	if (peek_keyword(tokens, "u8")) {
		consume_string(tokens, "u8");
		curr->data.primitive_type = u8;
		return curr;
	}
	if (peek_keyword(tokens, "u32")) {
		consume_string(tokens, "u32");
		curr->data.primitive_type = u32;
		return curr;
	}
	if (peek_keyword(tokens, "u64")) {
		consume_string(tokens, "u64");
		curr->data.primitive_type = u64;
		return curr;
	}
	if (peek_keyword(tokens, "i32")) {
		consume_string(tokens, "i32");
		curr->data.primitive_type = i32;
		return curr;
	}
	if (peek_keyword(tokens, "i64")) {
		consume_string(tokens, "i64");
		curr->data.primitive_type = i64;
		return curr;
	}
	if (peek_keyword(tokens, "f32")) {
		consume_string(tokens, "f32");
		curr->data.primitive_type = f32;
		return curr;
	}
	if (peek_keyword(tokens, "f64")) {
		consume_string(tokens, "f64");
		curr->data.primitive_type = f64;
		return curr;
	}
	if (peek_keyword(tokens, "bool")) {
		consume_string(tokens, "bool");
		curr->data.primitive_type = boolean;
		return curr;
	}
	if (peek_keyword(tokens, "void")) {
		consume_string(tokens, "void");
		curr->data.primitive_type = voidian;
		return curr;
	}
	if (peek_keyword(tokens, "usize")) {
		consume_string(tokens, "usize");
		curr->data.primitive_type = usize;
		return curr;
	}
	if (peek_keyword(tokens, "isize")) {
		consume_string(tokens, "isize");
		curr->data.primitive_type = isize;
		return curr;
	}
	destroy_node(curr);
	return NULL;
}

Node *parse_pointer_type(Tokens *tokens) {
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
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!consume_if(tokens, '[')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *integer = parse_integer_literal(tokens);

	if (!consume_if(tokens, ']')) {
		parser_restore(tokens, cp);
		destroy_node(integer);
		return NULL;
	}

	Node *type = parse_type(tokens);
	if (!type) {
		parser_restore(tokens, cp);
		destroy_node(integer);
		return NULL;
	}

	Node *curr = create_node_cp(NODE_SLICE_TYPE, "", cp);
	append_child(curr, type);
	append_child(curr, integer);
	return curr;
}

Node *parse_obj_type(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	identifier->type = NODE_OBJ_TYPE;
	return identifier;
}

Node *parse_enum_type(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	identifier->type = NODE_ENUM_TYPE;
	return identifier;
}

Node *parse_union_type(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	identifier->type = NODE_UNION_TYPE;
	return identifier;
}

Node *parse_obj_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visibility = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "obj")) {
		parser_restore(tokens, cp);
		destroy_node(visibility);
		return NULL;
	}

	consume_string(tokens, "obj");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		destroy_node(visibility);
		return NULL;
	}

	if (!consume_if(tokens, '{')) {
		parser_restore(tokens, cp);
		destroy_node(visibility);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_OBJ_DECL, "", cp);
	append_child(curr, visibility);
	append_child(curr, identifier);
	Node *node = parse_obj_member(tokens);
	while (node) {
		append_child(curr, node);
		node = parse_obj_member(tokens);
	}
	if (!consume_if(tokens, '}')) {
		parser_restore(tokens, cp);
		destroy_node(visibility);
		destroy_node(identifier);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_obj_member(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_field_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_method_decl(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	return NULL;
	//  constructors and destructors are not syntaxically different from methods, no distinction need now
	//	curr = parse_constructor_decl(tokens);
	//	if (curr)
	//		return curr;
	//	parser_restore(tokens, cp);
	//	curr = parse_destructor_decl(tokens);
	//	if (curr)
	//		return curr;
}

Node *parse_field_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	Node *var_decl = parse_var_decl(tokens);
	if (!var_decl) {
		destroy_node(visiblity);
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		destroy_node(visiblity);
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_FIELD_DECL, "", cp);
	append_child(curr, visiblity);
	append_child(curr, var_decl);
	return curr;
}

Node *parse_method_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "fn")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '(')) {
		parser_restore(tokens, cp);
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
				parser_restore(tokens, cp);
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
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *return_type = parse_return_type(tokens);
	if (!return_type) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		destroy_node(return_type);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_METHOD_DECL, "", cp);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, self_param);
	append_child(curr, parameter_list);
	append_child(curr, return_type);
	append_child(curr, block);
	return curr;
}

Node *parse_self_param(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "self")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "self");
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, '*')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *obj_type = parse_obj_type(tokens);
	if (!obj_type) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_SELF_PARAM, "", cp);
	append_child(curr, obj_type);
	return curr;
}

Node *parse_constructor_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "fn")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	if (!peek_keyword(tokens, "init")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "init");
	if (!consume_if(tokens, '(')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	Node *parameter_list = parse_parameter_list(tokens);

	if (!consume_if(tokens, ')')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(parameter_list);
		return NULL;
	}
	parse_return_type(tokens);
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_CONSTRUCTOR_DECL, "", cp);
	append_child(curr, visiblity);
	append_child(curr, parameter_list);
	append_child(curr, block);
	return curr;
}

Node *parse_destructor_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "fn")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "fn");
	if (!peek_keyword(tokens, "deinit")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "deinit");
	if (!consume_if(tokens, '(')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	Node *self_param = parse_self_param(tokens);

	if (!consume_if(tokens, ')')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(self_param);
		return NULL;
	}

	parse_return_type(tokens);
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(self_param);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_DESTRUCTOR_DECL, "", cp);
	append_child(curr, visiblity);
	append_child(curr, self_param);
	append_child(curr, block);
	return curr;
}

Node *parse_enum_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "enum")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "enum");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '{')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_ENUM_DECL, "", cp);
	Node *node = parse_enum_variant(tokens);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	while (node) {
		append_child(curr, node);
		node = parse_enum_variant(tokens);
	}
	if (!consume_if(tokens, '}')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_enum_variant(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *integer = NULL;
	if (consume_if(tokens, '=')) {
		integer = parse_integer_literal(tokens);
		if (!integer) {
			parser_restore(tokens, cp);
			destroy_node(identifier);
			return NULL;
		}
	}
	consume_if(tokens, ',');
	Node *curr = create_node_cp(NODE_ENUM_VARIANT, "", cp);
	append_child(curr, identifier);
	append_child(curr, integer);
	return curr;
}

Node *parse_union_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "union")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "union");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, '{')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_UNION_DECL, "", cp);
	Node *node = parse_union_variant(tokens);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	while (node) {
		append_child(curr, node);
		node = parse_union_variant(tokens);
	}

	if (!consume_if(tokens, '}')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

// union_variant = identifier ":" type [ "," ] ;
Node *parse_union_variant(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	consume_if(tokens, ',');
	Node *curr = create_node_cp(NODE_UNION_VARIANT, "", cp);
	append_child(curr, identifier);
	append_child(curr, type);
	return curr;
}

Node *parse_const_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *visiblity = parse_visiblity(tokens);
	if (!peek_keyword(tokens, "const")) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(tokens, "const");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	if (!consume_if(tokens, '=')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_CONST_DECL, "", cp);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, type);
	append_child(curr, expression);
	return curr;
}

Node *parse_var_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "var")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "var");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_VAR_DECL, "", cp);
	append_child(curr, identifier);
	append_child(curr, type);
	if (consume_if(tokens, '=')) {
		Node *expression = parse_expression(tokens);
		if (!expression) {
			parser_restore(tokens, cp);
			destroy_node(identifier);
			destroy_node(type);
			return NULL;
		}
		append_child(curr, expression);
	}
	return curr;
}
Node *parse_let_decl(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "let")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "let");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(tokens);
	if (!type) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	if (!consume_if(tokens, '=')) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_LET_DECL, "", cp);
	append_child(curr, identifier);
	append_child(curr, type);
	append_child(curr, expression);
	return curr;
}

Node *parse_statement(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = NULL;
	curr = parse_declaration_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_assignment_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_expression_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_conditional_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_loop_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_switch_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_return_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_break_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_continue_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_defer_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_block_stmt(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	return NULL;
}

Node *parse_declaration_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_var_decl(tokens);
	if (!curr) {
		curr = parse_let_decl(tokens);
	}
	if (!curr) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_assignment_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_assignment(tokens);
	if (!curr) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}
Node *parse_assignment(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *left_expression = parse_expression(tokens);
	if (!left_expression) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *assignment = parse_assignment_operator(tokens);
	if (!assignment) {
		parser_restore(tokens, cp);
		destroy_node(left_expression);
		return NULL;
	}
	Node *right_expression = parse_expression(tokens);
	if (!right_expression) {
		parser_restore(tokens, cp);
		destroy_node(left_expression);
		destroy_node(assignment);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_ASSIGNMENT_STMT, "", cp);
	append_child(curr, left_expression);
	append_child(curr, assignment);
	append_child(curr, right_expression);
	return curr;
}

Node *parse_assignment_operator(Tokens *tokens) {
	Node *curr = create_node(NODE_ASSIGNMENT_OPERATOR, "");
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
	if (peek_string(tokens, "=")) {
		curr->data.op = equals;
		consume_string(tokens, "=");
		return curr;
	}
	destroy_node(curr);
	return NULL;
}

Node *parse_expression_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_expression(tokens);
	if (!curr) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	return curr;
}

Node *parse_return_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "return")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "return");
	Node *expression = parse_expression(tokens);
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_RETURN_STMT, "", cp);
	append_child(curr, expression);
	return curr;
}
Node *parse_break_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "break")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "break");
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_BREAK_STMT, "", cp);
	return curr;
}
Node *parse_continue_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "continue")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "continue");
	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_CONTINUE_STMT, "", cp);
	return curr;
}

Node *parse_defer_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "defer")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "defer");
	Node *statement = parse_statement(tokens);
	if (!statement) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_DEFER_STMT, "", cp);
	append_child(curr, statement);
	return curr;
}

Node *parse_block_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_block(tokens);
	if (!curr) {
		parser_restore(tokens, cp);
		return NULL;
	}
	return curr;
}

Node *parse_conditional_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "if")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "if");
	Node *expression = parse_expression(tokens);
	if (!expression) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_CONDITIONAL_STMT, "", cp);
	append_child(curr, expression);
	append_child(curr, block);
	if (peek_keyword(tokens, "else")) {
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
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_loop_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_while_loop(tokens);
	if (curr) {
		return curr;
	}
	parser_restore(tokens, cp);
	curr = parse_for_loop(tokens);
	if (curr) {
		return curr;
	}
	parser_restore(tokens, cp);
	curr = parse_range_for_loop(tokens);
	if (curr) {
		return curr;
	}
	parser_restore(tokens, cp);
	return NULL;
}

Node *parse_while_loop(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "while")) {
		return NULL;
	}
	consume_string(tokens, "while");
	Node *expression = parse_expression(tokens);
	if (!expression) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_WHILE_LOOP, "", cp);
	append_child(curr, expression);
	append_child(curr, block);
	return curr;
}

Node *parse_for_loop(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);

	if (!peek_keyword(tokens, "for")) {
		parser_restore(tokens, cp);
		return NULL;
	}
	consume_string(tokens, "for");
	if (!consume_if(tokens, '(')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *for_init = parse_for_init(tokens);

	if (!for_init) {
		parser_restore(tokens, cp);
		return NULL;
	}

	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		destroy_node(for_init);
		return NULL;
	}
	Node *expression = parse_expression(tokens);

	if (!expression) {
		parser_restore(tokens, cp);
		destroy_node(for_init);
		return NULL;
	}

	if (!consume_if(tokens, ';')) {
		parser_restore(tokens, cp);
		destroy_node(for_init);
		destroy_node(expression);
		return NULL;
	}

	Node *for_update = parse_for_update(tokens);
	if (!for_update) {
		parser_restore(tokens, cp);
		destroy_node(for_init);
		destroy_node(expression);
		return NULL;
	}
	if (!consume_if(tokens, ')')) {
		parser_restore(tokens, cp);
		destroy_node(for_init);
		destroy_node(expression);
		destroy_node(for_update);
		return NULL;
	}
	Node *block = parse_block(tokens);

	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(for_init);
		destroy_node(expression);
		destroy_node(for_update);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_FOR_LOOP, "", cp);
	append_child(curr, for_init);
	append_child(curr, expression);
	append_child(curr, for_update);
	append_child(curr, block);
	return curr;
}

Node *parse_for_init(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = create_node_cp(NODE_FOR_INIT, "", cp);
	Node *var_decl = parse_var_decl(tokens);
	if (var_decl) {
		append_child(curr, var_decl);
		return curr;
	}
	parser_restore(tokens, cp);
	Node *assignment_stmt = parse_assignment_stmt(tokens);
	if (assignment_stmt) {
		append_child(curr, assignment_stmt);
		return curr;
	}
	parser_restore(tokens, cp);
	Node *expression_stmt = parse_expression_stmt(tokens);
	if (expression_stmt) {
		append_child(curr, expression_stmt);
		return curr;
	}
	parser_restore(tokens, cp);
	destroy_node(curr);
	return NULL;
}

Node *parse_for_update(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = create_node_cp(NODE_FOR_UPDATE, "", cp);
	Node *assignment_stmt = parse_assignment(tokens);
	if (assignment_stmt) {
		append_child(curr, assignment_stmt);
		return curr;
	}
	parser_restore(tokens, cp);
	Node *expression = parse_expression(tokens);
	if (expression) {
		append_child(curr, expression);
		return curr;
	}
	parser_restore(tokens, cp);
	destroy_node(curr);
	return NULL;
}

Node *parse_range_for_loop(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "for")) {
		return NULL;
	}
	consume_string(tokens, "for");
	Node *identifier = parse_identifier(tokens);
	if (!identifier) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *expression = parse_expression(tokens);
	if (!expression) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(identifier);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_RANGE_FOR_LOOP, "", cp);
	append_child(curr, identifier);
	append_child(curr, expression);
	append_child(curr, block);
	return curr;
}

Node *parse_switch_stmt(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "switch")) {
		return NULL;
	}
	consume_string(tokens, "switch");

	Node *expression = parse_expression(tokens);

	if (!expression) {
		parser_restore(tokens, cp);
		return NULL;
	}

	if (!consume_if(tokens, '{')) {
		parser_restore(tokens, cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_SWITCH_STMT, "", cp);
	append_child(curr, expression);
	Node *node = parse_switch_case(tokens);
	while (node) {
		append_child(curr, node);
		node = parse_switch_case(tokens);
	}
	Node *switch_default = parse_switch_default(tokens);
	append_child(curr, switch_default);
	if (!consume_if(tokens, '}')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_switch_case(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "case")) {
		return NULL;
	}
	consume_string(tokens, "case");
	Node *case_pattern = parse_case_pattern(tokens);
	if (!case_pattern) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_SWITCH_CASE, "", cp);
	while (case_pattern) {
		append_child(curr, case_pattern);
		if (!consume_if(tokens, ',')) {
			break;
		}
		case_pattern = parse_case_pattern(tokens);
	}
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	append_child(curr, block);
	return curr;
}

Node *parse_case_pattern(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *expression = parse_expression(tokens);
	if (expression) {
		return expression;
	}
	parser_restore(tokens, cp);
	Node *literal = parse_literal(tokens);
	if (literal) {
		return literal;
	}
	parser_restore(tokens, cp);
	Node *identifier = parse_identifier(tokens);
	if (identifier) {
		return identifier;
	}
	parser_restore(tokens, cp);
	return NULL;
}

Node *parse_switch_default(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!peek_keyword(tokens, "default")) {
		return NULL;
	}
	consume_string(tokens, "default");
	if (!consume_if(tokens, ':')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *block = parse_block(tokens);
	if (!block) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_SWITCH_DEFAULT, "", cp);
	append_child(curr, block);
	return curr;
}

Node *parse_expression(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *logical_or_expression = parse_logical_or_expression(tokens);
	if (!logical_or_expression) {
		parser_restore(tokens, cp);
		return NULL;
	}
	return logical_or_expression;
}

Node *parse_logical_or_expression(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *logical_and_expression = parse_logical_and_expression(tokens);
	if (!logical_and_expression) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!peek_string(tokens, "||")) {
		return logical_and_expression;
	}
	Node *curr = create_node_cp(NODE_LOGICAL_OR_EXPRESSION, "", cp);
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
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *equality_expression = parse_equality_expression(tokens);
	if (!equality_expression) {
		parser_restore(tokens, cp);
		return NULL;
	}
	if (!peek_string(tokens, "&&")) {
		return equality_expression;
	}
	Node *curr = create_node_cp(NODE_LOGICAL_AND_EXPRESSION, "", cp);
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
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *left = parse_relational_expression(tokens);
	if (!left) {
		parser_restore(tokens, cp);
		return NULL;
	}
	while (peek_string(tokens, "==") || peek_string(tokens, "!=")) {
		Node *curr = create_node_cp(NODE_EQUALITY_EXPRESSION, "", cp);
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
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *left = parse_additive_expression(tokens);
	if (!left) {
		parser_restore(tokens, cp);
		return NULL;
	}
	while (peek_string(tokens, "<") || peek_string(tokens, "<=") || peek_string(tokens, ">") || peek_string(tokens, ">=")) {
		Node *curr = create_node_cp(NODE_RELATIONAL_EXPRESSION, "", cp);
		if (peek_string(tokens, "<=")) {
			consume_string(tokens, "<=");
			curr->data.op = less_than_eq;
		} else if (peek_string(tokens, ">=")) {
			consume_string(tokens, ">=");
			curr->data.op = greater_than_eq;
		} else if (peek_string(tokens, "<")) {
			consume_string(tokens, "<");
			curr->data.op = less_than;
		} else if (peek_string(tokens, ">")) {
			consume_string(tokens, ">");
			curr->data.op = greater_than;
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
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *left = parse_multiplicative_expression(tokens);
	if (!left) {
		parser_restore(tokens, cp);
		return NULL;
	}
	while (peek_string(tokens, "+") || peek_string(tokens, "-")) {
		Node *curr = create_node_cp(NODE_ADDITIVE_EXPRESSION, "", cp);
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
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *left = parse_unary_expression(tokens);
	if (!left) {
		parser_restore(tokens, cp);
		return NULL;
	}
	while (peek_string(tokens, "*") || peek_string(tokens, "/") || peek_string(tokens, "%")) {
		Node *curr = create_node_cp(NODE_MULTIPLICTIVE_EXPRESSION, "", cp);
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
	if (peek_string(tokens, "-") ||
		peek_string(tokens, "!") ||
		peek_string(tokens, "~") ||
		peek_string(tokens, "*") ||
		peek_string(tokens, "&")) {
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
		Node *operand = parse_unary_expression(tokens);
		if (!operand) {
			destroy_node(curr);
			return NULL;
		}
		append_child(curr, operand);
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
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *left = parse_primary_expression(tokens);
	if (!left) {
		parser_restore(tokens, cp);
		return NULL;
	}
	while (1) {
		if (consume_if(tokens, '(')) {
			Node *args = parse_argument_list(tokens);
			if (!consume_if(tokens, ')')) {
				destroy_node(args);
				destroy_node(left);
				parser_restore(tokens, cp);
				return NULL;
			}
			Node *curr = create_node_cp(NODE_FUNC_CALL, "", cp);
			append_child(curr, left);
			append_child(curr, args);
			left = curr;
			continue;
		}
		if (consume_if(tokens, '.')) {
			Node *identifier = parse_identifier(tokens);
			if (!identifier) {
				destroy_node(left);
				parser_restore(tokens, cp);
				return NULL;
			}
			Node *curr = create_node_cp(NODE_ACCESS, "", cp);
			append_child(curr, left);
			append_child(curr, identifier);
			left = curr;
			continue;
		}
		if (consume_if(tokens, '[')) {
			Node *curr = create_node_cp(NODE_INDEX, "", cp);
			append_child(curr, left);
			Node *expr_a = parse_expression(tokens);
			append_child(curr, expr_a);
			if (consume_if(tokens, ':')) {
				Node *expr_b = parse_expression(tokens);
				append_child(curr, expr_b);
			}
			if (!consume_if(tokens, ']')) {
				destroy_node(curr);
				parser_restore(tokens, cp);
				return NULL;
			}
			left = curr;
			continue;
		}
		if (peek_string(tokens, "++")) {
			consume_string(tokens, "++");
			Node *curr = create_node_cp(NODE_INC_DEC, "", cp);
			curr->data.op = plus_plus;
			append_child(curr, left);
			left = curr;
			continue;
		}
		if (peek_string(tokens, "--")) {
			consume_string(tokens, "--");
			Node *curr = create_node_cp(NODE_INC_DEC, "", cp);
			curr->data.op = minus_minus;
			append_child(curr, left);
			left = curr;
			continue;
		}
		break;
	}
	return left;
}

//primary_expression = literal
//                   | identifier
//                   | "(" expression ")"
//                   | "sizeof" "(" type ")"
//                   | "(" type ")" expression    (* cast *)
//                   ;
Node *parse_primary_expression(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_identifier(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	if (peek_keyword(tokens, "sizeof")) {
		consume_string(tokens, "sizeof");
		if (!consume_if(tokens, '(')) {
			parser_restore(tokens, cp);
			return NULL;
		}
		Node *type = parse_type(tokens);
		if (!type) {
			parser_restore(tokens, cp);
			return NULL;
		}
		if (!consume_if(tokens, ')')) {
			destroy_node(type);
			parser_restore(tokens, cp);
			return NULL;
		}
		curr = create_node_cp(NODE_SIZE_OF_EXPRESSION, "", cp);
		append_child(curr, type);
		return curr;
	}
	parser_restore(tokens, cp);
	if (consume_if(tokens, '(')) {
		Node *type = parse_type(tokens);
		if (type && consume_if(tokens, ')')) {
			Node *expr = parse_unary_expression(tokens);
			if (expr) {
				Node *cast = create_node_cp(NODE_CAST_EXPRESSION, "", cp);
				append_child(cast, type);
				append_child(cast, expr);
				return cast;
			}
		}
		parser_restore(tokens, cp);
		consume_if(tokens, '(');
		Node *expr = parse_expression(tokens);
		if (!expr) {
			parser_restore(tokens, cp);
			return NULL;
		}
		if (!consume_if(tokens, ')')) {
			destroy_node(expr);
			parser_restore(tokens, cp);
			return NULL;
		}
		Node *group = create_node_cp(NODE_GROUPED_EXPRESSION, "", cp);
		append_child(group, expr);
		return group;
	}
	destroy_node(curr);
	return NULL;
}

//argument_list = [ expression { "," expression } ] ;
Node *parse_argument_list(Tokens *tokens) {
	Node *curr = create_node(NODE_ARGUMENT_LIST, "");
	Node *expression = parse_expression(tokens);
	while (expression) {
		append_child(curr, expression);
		if (!consume_if(tokens, ',')) {
			break;
		}
		expression = parse_expression(tokens);
	}
	return curr;
}

//literal = integer_literal
//        | float_literal
//        | string_literal
//        | boolean_literal
//        | array_literal
//        ;
Node *parse_literal(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_integer_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_float_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_string_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_boolean_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_array_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	return NULL;
}
//integer_literal = decimal_literal | hex_literal | octal_literal | binary_literal ;
Node *parse_integer_literal(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_binary_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_hex_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_octal_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	curr = parse_decimal_literal(tokens);
	if (curr)
		return curr;
	parser_restore(tokens, cp);
	return NULL;
}

//float_literal = decimal_digits "." decimal_digits [ "e" [ "+" "-" ] decimal_digits ] ;
Node *parse_float_literal(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *right = parse_decimal_digits(tokens);
	if (!right) {
		parser_restore(tokens, cp);
		return NULL;
	}
	destroy_node(right);
	if (!consume_if(tokens, '.')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *left = parse_decimal_digits(tokens);
	if (!left) {
		parser_restore(tokens, cp);
		return NULL;
	}
	destroy_node(left);
	Node *curr = create_node_cp(NODE_FLOAT_LITERAL, "", cp);
	if (consume_if(tokens, 'e')) {
		if (!consume_if(tokens, '+'))
			consume_if(tokens, '-');
		Node *exponent = parse_decimal_digits(tokens);
		if (!exponent) {
			parser_restore(tokens, cp);
			destroy_node(curr);
			return NULL;
		}
		destroy_node(exponent);
	}
	curr->data.literal.start = cp.position;
	curr->data.literal.end = tokens->position;
	return curr;
}

//string_literal = '"' { string_char } '"' ;  (* yields []u8 *)
Node *parse_string_literal(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!consume_if(tokens, '"')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	while (peek_raw(tokens) != '"' && peek_raw(tokens) != '\0') {
		if (peek_raw(tokens) == '\\') {
			consume_raw(tokens);
			if (peek_raw(tokens) == '\0')
				break;
		}
		consume_raw(tokens);
	}
	if (!consume_if(tokens, '"')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_STRING_LITERAL, "", cp);
	curr->data.literal.start = cp.position + 1;
	curr->data.literal.end = tokens->position - 1;
	return curr;
}
//boolean_literal = "true" | "false" ;
Node *parse_boolean_literal(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (peek_keyword(tokens, "true")) {
		cp.position = tokens->position;
		consume_string(tokens, "true");
	} else if (peek_keyword(tokens, "false")) {
		cp.position = tokens->position;
		consume_string(tokens, "false");
	} else {
		return NULL;
	}
	Node *curr = create_node_cp(NODE_BOOLEAN_LITERAL, "", cp);
	curr->data.literal.start = cp.position;
	curr->data.literal.end = tokens->position;
	return curr;
}

// array_literal = "[" [ expression { "," expression } ] "]" ;  (* slice literal *)
Node *parse_array_literal(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	if (!consume_if(tokens, '[')) {
		parser_restore(tokens, cp);
		return NULL;
	}
	Node *curr = create_node_cp(NODE_ARRAY_LITERAL, "", cp);
	Node *expression = parse_expression(tokens);
	while (expression) {
		append_child(curr, expression);
		if (!consume_if(tokens, ',')) {
			break;
		}
		expression = parse_expression(tokens);
	}
	if (!consume_if(tokens, ']')) {
		parser_restore(tokens, cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

//identifier = letter { letter | digit | "_" } ;
Node *parse_identifier(Tokens *tokens) {
	if (!isalpha(peek(tokens))) {
		return NULL;
	}
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	size_t size = 0;
	for (; isalnum(peek_raw(tokens)) || peek_raw(tokens) == '_'; size++) {
		consume_raw(tokens);
	}
	Node *curr = create_node_cp(NODE_IDENTIFER, "", cp);
	curr->data.literal.source = tokens->data;
	curr->data.literal.start = cp.position;
	curr->data.literal.end = cp.position + size;
	return curr;
}

//decimal_literal = decimal_digits ;
Node *parse_decimal_literal(Tokens *tokens) {
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	Node *curr = parse_decimal_digits(tokens);
	if (!curr) {
		parser_restore(tokens, cp);
		return NULL;
	}
	curr->type = NODE_DECIMAL_LITERAL;
	return curr;
}

//hex_literal = "0x" hex_digits ;
Node *parse_hex_literal(Tokens *tokens) {
	if (!peek_string(tokens, "0x")) {
		return NULL;
	}
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	consume_string(tokens, "0x");
	while (isxdigit(peek_raw(tokens))) {
		consume_raw(tokens);
	}
	Node *curr = create_node_cp(NODE_HEX_LITERAL, "", cp);
	curr->data.literal.start = cp.position;
	curr->data.literal.end = tokens->position;
	curr->data.literal.source = tokens->data;
	return curr;
}

//octal_literal = "0o" octal_digits ;
Node *parse_octal_literal(Tokens *tokens) {
	if (!peek_string(tokens, "0o")) {
		return NULL;
	}
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	consume_string(tokens, "0o");
	while (peek_raw(tokens) >= '0' && peek_raw(tokens) <= '7') {
		consume_raw(tokens);
	}
	Node *curr = create_node_cp(NODE_OCTAL_LITERAL, "", cp);
	curr->data.literal.start = cp.position;
	curr->data.literal.end = tokens->position;
	curr->data.literal.source = tokens->data;
	return curr;
}

//binary_literal = "0b" binary_digits ;
Node *parse_binary_literal(Tokens *tokens) {
	if (!peek_string(tokens, "0b")) {
		return NULL;
	}
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	consume_string(tokens, "0b");
	while (peek_raw(tokens) == '1' || peek_raw(tokens) == '0') {
		consume_raw(tokens);
	}
	Node *curr = create_node_cp(NODE_BINARY_LITERAL, "", cp);
	curr->data.literal.start = cp.position;
	curr->data.literal.end = tokens->position;
	curr->data.literal.source = tokens->data;
	return curr;
}

//decimal_digits = digit { digit } ;
Node *parse_decimal_digits(Tokens *tokens) {
	if (!isdigit(peek(tokens))) {
		return NULL;
	}
	skip_whitespace(tokens);
	ParserCheckpoint cp = parser_save(tokens);
	while (isdigit(peek_raw(tokens))) {
		consume_raw(tokens);
	}
	Node *curr = create_node_cp(NODE_DECIMAL_DIGIT, "", cp);
	curr->data.literal.start = cp.position;
	curr->data.literal.end = tokens->position;
	curr->data.literal.source = tokens->data;

	return curr;
}
