#include "../parse/parser.h"

Node *parse(Arena *arena, TokenStream *ts) {
	ParseState state = {0};
	state.ts = ts;
	state.arena = arena;
	Node *root = parse_program(&state);
	for (int i = 0; i < arrlen(state.errors); i++) {
		printf("<PARSE ERROR> %s @ %ld: %ld;\n", state.errors[i].msg,
			   state.errors[i].line, state.errors[i].col);
	}
	return root;
}

Node *parse_program(ParseState *state) {
	Node *root = create_node(state->arena, NODE_PROGRAM, "Root");
	Node *curr = parse_top_level_decl(state);
	while (curr) {
		append_child(root, curr);
		curr = parse_top_level_decl(state);
	}
	return root;
}

Node *parse_top_level_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_func_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_obj_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_enum_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_union_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_const_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_module_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_import_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	return NULL;
}

Node *parse_module_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (ts_advance(state->ts)->kind != TKN_KW_MODULE) {
		parser_restore(cp);
		return NULL;
	}
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		// parse error
		// parser recovery
		parser_restore(cp);
		return NULL;
	}
	if (ts_advance(state->ts)->kind != TKN_SEMI) {
		// parse error
		// parser recovery
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_MODULE_DECL, "");
	append_child(curr, identifier);
	return curr;
}

Node *parse_import_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (ts_advance(state->ts)->kind != TKN_KW_IMPORT) {
		parser_restore(cp);
		return NULL;
	}
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		// parse error
		// parse recovery
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_IMPORT_DECL, "");
	append_child(curr, identifier);
	if (ts_peek(state->ts)->kind == TKN_KW_AS) {
		ts_advance(state->ts);
		Node *alias = parse_identifier(state);
		if (!alias) {
			// parse error
			// parse recovery
			parser_restore(cp);
			return NULL;
		}
		append_child(curr, alias);
	}
	if (ts_advance(state->ts)->kind != TKN_SEMI) {
		// parse error
		// parse recovery
		parser_restore(cp);
		return NULL;
	}
	return curr;
}

Node *parse_visiblity(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_VISIBLITY, "");
	curr->data.visiblity = false;
	if (ts_peek(state->ts)->kind == TKN_KW_PUB) {
		ts_advance(state->ts);
		curr->data.visiblity = true;
	}
	return curr;
}

Node *parse_func_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	if (ts_advance(state->ts)->kind != TKN_KW_FN) {
		parser_restore(cp);
		return NULL;
	}
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		// send the following formated error;
		// parse error: function deceleration not followed by identifier
		// move to next appropirate character
		parser_restore(cp);
		return NULL;
	}
	if (ts_advance(state->ts)->kind != TKN_LPAREN) {
		// send the following formated error;
		// parse error: function name not followed by '('
		// move to next appropirate character
		parser_restore(cp);
		return NULL;
	}
	Node *parameter_list = parse_parameter_list(state);
	if (ts_advance(state->ts)->kind != TKN_RPAREN) {
		// send the following formated error;
		// parse error: function name not followed by ')'
		// move to next appropirate character
		parser_restore(cp);
		return NULL;
	}
	Node *return_type = parse_return_type(state);
	if (!return_type) {
		// send the following formated error;
		// parse error: function definition type
		// move to next appropirate character
		parser_restore(cp);
		return NULL;
	}
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_FUNC_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, parameter_list);
	append_child(curr, return_type);
	append_child(curr, block);
	return curr;
}

Node *parse_parameter_list(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_PARAMETER_LIST, "");
	Node *node = parse_parameter(state);
	while (node) {
		append_child(curr, node);
		if (ts_peek(state->ts)->kind != TKN_SEMI)
			break;
		ts_advance(state->ts);
		node = parse_parameter(state);
	}
	return curr;
}

Node *parse_parameter(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		return NULL;
	}
	if (ts_advance(state->ts)->kind != TKN_COLON) {
		parser_restore(cp);
		return NULL;
	}
	Node *type = parse_type(state);
	if (!type) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_PARAMETER, "");
	append_child(curr, identifier);
	append_child(curr, type);
	return curr;
}

Node *parse_return_type(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (ts_advance(state->ts)->kind != TKN_ARROW) {
		return NULL;
	}
	Node *type = parse_type(state);
	if (!type) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_RETURN_TYPE, "");
	append_child(curr, type);
	return curr;
}

Node *parse_block(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!consume_if(ts, '{')) {
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_BLOCK, "");
	Node *node = parse_statement(state);
	while (node) {
		append_child(curr, node);
		node = parse_statement(state);
	}
	if (ts_advance(state->ts)->kind != TKN_RBRACE) {
		parser_restore(cp);
		return NULL;
	}
	return curr;
}

Node *parse_type(ParseState *state) {
	Node *curr = parse_primitive_type(state);
	if (curr) {
		return curr;
	}
	curr = parse_pointer_type(state);
	if (curr) {
		return curr;
	}
	curr = parse_slice_type(state);
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
	curr = parse_obj_type(state);
	if (curr) {
		return curr;
	}
	curr = parse_enum_type(state);
	if (curr) {
		return curr;
	}
	curr = parse_union_type(state);
	if (curr) {
		return curr;
	}
	return NULL;
}

Node *parse_primitive_type(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = create_node(state->arena, NODE_PRIMITIVE_TYPE, "");
	if (peek_keyword(ts, "u8")) {
		consume_string(ts, "u8");
		curr->data.primitive_type = u8;
		return curr;
	}
	if (peek_keyword(ts, "u32")) {
		consume_string(ts, "u32");
		curr->data.primitive_type = u32;
		return curr;
	}
	if (peek_keyword(ts, "u64")) {
		consume_string(ts, "u64");
		curr->data.primitive_type = u64;
		return curr;
	}
	if (peek_keyword(ts, "i32")) {
		consume_string(ts, "i32");
		curr->data.primitive_type = i32;
		return curr;
	}
	if (peek_keyword(ts, "i64")) {
		consume_string(ts, "i64");
		curr->data.primitive_type = i64;
		return curr;
	}
	if (peek_keyword(ts, "f32")) {
		consume_string(ts, "f32");
		curr->data.primitive_type = f32;
		return curr;
	}
	if (peek_keyword(ts, "f64")) {
		consume_string(ts, "f64");
		curr->data.primitive_type = f64;
		return curr;
	}
	if (peek_keyword(ts, "bool")) {
		consume_string(ts, "bool");
		curr->data.primitive_type = boolean;
		return curr;
	}
	if (peek_keyword(ts, "void")) {
		consume_string(ts, "void");
		curr->data.primitive_type = voidian;
		return curr;
	}
	if (peek_keyword(ts, "usize")) {
		consume_string(ts, "usize");
		curr->data.primitive_type = usize;
		return curr;
	}
	if (peek_keyword(ts, "isize")) {
		consume_string(ts, "isize");
		curr->data.primitive_type = isize;
		return curr;
	}
	destroy_node(curr);
	return NULL;
}

Node *parse_pointer_type(ParseState *state) {
	if (!consume_if(ts, '*')) {
		return NULL;
	}
	Node *type = parse_type(state);
	if (!type) {
		return NULL;
	}
	Node *curr = create_node(state, NODE_POINTER_TYPE, "");
	append_child(curr, type);
	return curr;
}

Node *parse_slice_type(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!consume_if(ts, '[')) {
		parser_restore(cp);
		return NULL;
	}
	Node *integer = parse_integer_literal(state);

	if (!consume_if(ts, ']')) {
		parser_restore(cp);
		destroy_node(integer);
		return NULL;
	}

	Node *type = parse_type(state);
	if (!type) {
		parser_restore(cp);
		destroy_node(integer);
		return NULL;
	}

	Node *curr = create_node_parse(state, NODE_SLICE_TYPE, "");
	append_child(curr, type);
	append_child(curr, integer);
	return curr;
}

Node *parse_obj_type(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	identifier->type = NODE_OBJ_TYPE;
	return identifier;
}

Node *parse_enum_type(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	identifier->type = NODE_ENUM_TYPE;
	return identifier;
}

Node *parse_union_type(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	identifier->type = NODE_UNION_TYPE;
	return identifier;
}

Node *parse_obj_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visibility = parse_visiblity(state);
	if (!peek_keyword(ts, "obj")) {
		parser_restore(cp);
		destroy_node(visibility);
		return NULL;
	}

	consume_string(ts, "obj");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		destroy_node(visibility);
		return NULL;
	}

	if (!consume_if(ts, '{')) {
		parser_restore(cp);
		destroy_node(visibility);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_OBJ_DECL, "");
	append_child(curr, visibility);
	append_child(curr, identifier);
	Node *node = parse_obj_member(state);
	while (node) {
		append_child(curr, node);
		node = parse_obj_member(state);
	}
	if (!consume_if(ts, '}')) {
		parser_restore(cp);
		destroy_node(visibility);
		destroy_node(identifier);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_obj_member(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_field_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_method_decl(state);
	if (curr)
		return curr;
	parser_restore(cp);
	return NULL;
	//  constructors and destructors are not syntaxically different from
	//  methods, no distinction need now
	//	curr = parse_constructor_decl(state);
	//	if (curr)
	//		return curr;
	//	parser_restore(cp);
	//	curr = parse_destructor_decl(state);
	//	if (curr)
	//		return curr;
}

Node *parse_field_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	Node *var_decl = parse_var_decl(state);
	if (!var_decl) {
		destroy_node(visiblity);
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ';')) {
		destroy_node(visiblity);
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_FIELD_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, var_decl);
	return curr;
}

Node *parse_method_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	if (!peek_keyword(ts, "fn")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "fn");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(ts, '(')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *self_param = parse_self_param(state);
	Node *parameter_list = NULL;
	if (self_param) {
		if (consume_if(ts, ',')) {
			parameter_list = parse_parameter_list(state);
			if (!parameter_list) {
				parser_restore(cp);
				destroy_node(visiblity);
				destroy_node(identifier);
				destroy_node(self_param);
				return NULL;
			}
		} else {
			parameter_list = parse_parameter_list(state);
		}
	} else {
		parameter_list = parse_parameter_list(state);
	}

	if (!consume_if(ts, ')')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *return_type = parse_return_type(state);
	if (!return_type) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(self_param);
		destroy_node(parameter_list);
		destroy_node(return_type);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_METHOD_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, self_param);
	append_child(curr, parameter_list);
	append_child(curr, return_type);
	append_child(curr, block);
	return curr;
}

Node *parse_self_param(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "self")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "self");
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, '*')) {
		parser_restore(cp);
		return NULL;
	}
	Node *obj_type = parse_obj_type(state);
	if (!obj_type) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_SELF_PARAM, "");
	append_child(curr, obj_type);
	return curr;
}

Node *parse_constructor_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	if (!peek_keyword(ts, "fn")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "fn");
	if (!peek_keyword(ts, "init")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "init");
	if (!consume_if(ts, '(')) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	Node *parameter_list = parse_parameter_list(state);

	if (!consume_if(ts, ')')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(parameter_list);
		return NULL;
	}
	parse_return_type(state);
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(parameter_list);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_CONSTRUCTOR_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, parameter_list);
	append_child(curr, block);
	return curr;
}

Node *parse_destructor_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	if (!peek_keyword(ts, "fn")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "fn");
	if (!peek_keyword(ts, "deinit")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "deinit");
	if (!consume_if(ts, '(')) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	Node *self_param = parse_self_param(state);

	if (!consume_if(ts, ')')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(self_param);
		return NULL;
	}

	parse_return_type(state);
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(self_param);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_DESTRUCTOR_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, self_param);
	append_child(curr, block);
	return curr;
}

Node *parse_enum_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	if (!peek_keyword(ts, "enum")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "enum");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(ts, '{')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_ENUM_DECL, "");
	Node *node = parse_enum_variant(state);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	while (node) {
		append_child(curr, node);
		node = parse_enum_variant(state);
	}
	if (!consume_if(ts, '}')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_enum_variant(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	Node *integer = NULL;
	if (consume_if(ts, '=')) {
		integer = parse_integer_literal(state);
		if (!integer) {
			parser_restore(cp);
			destroy_node(identifier);
			return NULL;
		}
	}
	consume_if(ts, ',');
	Node *curr = create_node_parse(state, NODE_ENUM_VARIANT, "");
	append_child(curr, identifier);
	append_child(curr, integer);
	return curr;
}

Node *parse_union_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	if (!peek_keyword(ts, "union")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "union");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(ts, '{')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_UNION_DECL, "");
	Node *node = parse_union_variant(state);
	append_child(curr, visiblity);
	append_child(curr, identifier);
	while (node) {
		append_child(curr, node);
		node = parse_union_variant(state);
	}

	if (!consume_if(ts, '}')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

// union_variant = identifier ":" type [ "," ] ;
Node *parse_union_variant(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(state);
	if (!type) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	consume_if(ts, ',');
	Node *curr = create_node_parse(state, NODE_UNION_VARIANT, "");
	append_child(curr, identifier);
	append_child(curr, type);
	return curr;
}

Node *parse_const_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *visiblity = parse_visiblity(state);
	if (!peek_keyword(ts, "const")) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	consume_string(ts, "const");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		destroy_node(visiblity);
		return NULL;
	}
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(state);
	if (!type) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		return NULL;
	}
	if (!consume_if(ts, '=')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *expression = parse_expression(state);
	if (!expression) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		destroy_node(visiblity);
		destroy_node(identifier);
		destroy_node(type);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_CONST_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, identifier);
	append_child(curr, type);
	append_child(curr, expression);
	return curr;
}

Node *parse_var_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "var")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "var");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(state);
	if (!type) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_VAR_DECL, "");
	append_child(curr, identifier);
	append_child(curr, type);
	if (consume_if(ts, '=')) {
		Node *expression = parse_expression(state);
		if (!expression) {
			parser_restore(cp);
			destroy_node(identifier);
			destroy_node(type);
			return NULL;
		}
		append_child(curr, expression);
	}
	return curr;
}
Node *parse_let_decl(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "let")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "let");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *type = parse_type(state);
	if (!type) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	if (!consume_if(ts, '=')) {
		parser_restore(cp);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *expression = parse_expression(state);
	if (!expression) {
		parser_restore(cp);
		destroy_node(identifier);
		destroy_node(type);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_LET_DECL, "");
	append_child(curr, identifier);
	append_child(curr, type);
	append_child(curr, expression);
	return curr;
}

Node *parse_statement(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = NULL;
	curr = parse_declaration_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);

	curr = parse_return_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_assignment_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_expression_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_conditional_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_loop_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_switch_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_break_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_continue_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_defer_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_block_stmt(state);
	if (curr)
		return curr;
	parser_restore(cp);
	return NULL;
}

Node *parse_declaration_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_var_decl(state);
	if (!curr) {
		curr = parse_let_decl(state);
	}
	if (!curr) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_assignment_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_assignment(state);
	if (!curr) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}
Node *parse_assignment(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *left_expression = parse_expression(state);
	if (!left_expression) {
		parser_restore(cp);
		return NULL;
	}
	Node *assignment = parse_assignment_operator(state);
	if (!assignment) {
		parser_restore(cp);
		destroy_node(left_expression);
		return NULL;
	}
	Node *right_expression = parse_expression(state);
	if (!right_expression) {
		parser_restore(cp);
		destroy_node(left_expression);
		destroy_node(assignment);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_ASSIGNMENT_STMT, "");
	append_child(curr, left_expression);
	append_child(curr, assignment);
	append_child(curr, right_expression);
	return curr;
}

Node *parse_assignment_operator(ParseState *state) {
	Node *curr = create_node(state, NODE_ASSIGNMENT_OPERATOR, "");
	if (peek_string(ts, "+=")) {
		curr->data.op = plus_equals;
		consume_string(ts, "+=");
		return curr;
	}
	if (peek_string(ts, "*=")) {
		curr->data.op = star_equals;
		consume_string(ts, "*=");
		return curr;
	}
	if (peek_string(ts, "/=")) {
		curr->data.op = slash_equals;
		consume_string(ts, "/=");
		return curr;
	}
	if (peek_string(ts, "%=")) {
		curr->data.op = percent_equals;
		consume_string(ts, "%=");
		return curr;
	}
	if (peek_string(ts, "&=")) {
		curr->data.op = and_equals;
		consume_string(ts, "&=");
		return curr;
	}
	if (peek_string(ts, "|=")) {
		curr->data.op = or_equals;
		consume_string(ts, "|=");
		return curr;
	}
	if (peek_string(ts, "^=")) {
		curr->data.op = xor_equals;
		consume_string(ts, "^=");
		return curr;
	}
	if (peek_string(ts, "<<=")) {
		curr->data.op = lshift_equals;
		consume_string(ts, "<<=");
		return curr;
	}
	if (peek_string(ts, ">>=")) {
		curr->data.op = rshift_equals;
		consume_string(ts, ">>=");
		return curr;
	}
	if (peek_string(ts, "=")) {
		curr->data.op = equals;
		consume_string(ts, "=");
		return curr;
	}
	destroy_node(curr);
	return NULL;
}

Node *parse_expression_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_expression(state);
	if (!curr) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_return_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "return")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "return");
	Node *expression = parse_expression(state);

	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_RETURN_STMT, "");
	append_child(curr, expression);
	return curr;
}
Node *parse_break_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "break")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "break");
	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_BREAK_STMT, "");
	return curr;
}
Node *parse_continue_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "continue")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "continue");
	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_CONTINUE_STMT, "");
	return curr;
}

Node *parse_defer_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "defer")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "defer");
	Node *statement = parse_statement(state);
	if (!statement) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_DEFER_STMT, "");
	append_child(curr, statement);
	return curr;
}

Node *parse_block_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_block(state);
	if (!curr) {
		parser_restore(cp);
		return NULL;
	}
	return curr;
}

Node *parse_conditional_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "if")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "if");
	Node *expression = parse_expression(state);
	if (!expression) {
		parser_restore(cp);
		return NULL;
	}
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_CONDITIONAL_STMT, "");
	append_child(curr, expression);
	append_child(curr, block);
	if (peek_keyword(ts, "else")) {
		consume_string(ts, "else");
		Node *conditional_stmt = parse_conditional_stmt(state);
		if (conditional_stmt) {
			append_child(curr, conditional_stmt);
			return curr;
		}
		Node *else_block = parse_block(state);
		if (else_block) {
			append_child(curr, else_block);
			return curr;
		}
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_loop_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_while_loop(state);
	if (curr) {
		return curr;
	}
	parser_restore(cp);
	curr = parse_for_loop(state);
	if (curr) {
		return curr;
	}
	parser_restore(cp);
	curr = parse_range_for_loop(state);
	if (curr) {
		return curr;
	}
	parser_restore(cp);
	return NULL;
}

Node *parse_while_loop(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "while")) {
		return NULL;
	}
	consume_string(ts, "while");
	Node *expression = parse_expression(state);
	if (!expression) {
		parser_restore(cp);
		return NULL;
	}
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_WHILE_LOOP, "");
	append_child(curr, expression);
	append_child(curr, block);
	return curr;
}

Node *parse_for_loop(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);

	if (!peek_keyword(ts, "for")) {
		parser_restore(cp);
		return NULL;
	}
	consume_string(ts, "for");
	if (!consume_if(ts, '(')) {
		parser_restore(cp);
		return NULL;
	}
	Node *for_init = parse_for_init(state);

	if (!for_init) {
		parser_restore(cp);
		return NULL;
	}

	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		destroy_node(for_init);
		return NULL;
	}
	Node *expression = parse_expression(state);

	if (!expression) {
		parser_restore(cp);
		destroy_node(for_init);
		return NULL;
	}

	if (!consume_if(ts, ';')) {
		parser_restore(cp);
		destroy_node(for_init);
		destroy_node(expression);
		return NULL;
	}

	Node *for_update = parse_for_update(state);
	if (!for_update) {
		parser_restore(cp);
		destroy_node(for_init);
		destroy_node(expression);
		return NULL;
	}
	if (!consume_if(ts, ')')) {
		parser_restore(cp);
		destroy_node(for_init);
		destroy_node(expression);
		destroy_node(for_update);
		return NULL;
	}
	Node *block = parse_block(state);

	if (!block) {
		parser_restore(cp);
		destroy_node(for_init);
		destroy_node(expression);
		destroy_node(for_update);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_FOR_LOOP, "");
	append_child(curr, for_init);
	append_child(curr, expression);
	append_child(curr, for_update);
	append_child(curr, block);
	return curr;
}

Node *parse_for_init(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = create_node_parse(state, NODE_FOR_INIT, "");
	Node *var_decl = parse_var_decl(state);
	if (var_decl) {
		append_child(curr, var_decl);
		return curr;
	}
	parser_restore(cp);
	Node *assignment_stmt = parse_assignment_stmt(state);
	if (assignment_stmt) {
		append_child(curr, assignment_stmt);
		return curr;
	}
	parser_restore(cp);
	Node *expression_stmt = parse_expression_stmt(state);
	if (expression_stmt) {
		append_child(curr, expression_stmt);
		return curr;
	}
	parser_restore(cp);
	destroy_node(curr);
	return NULL;
}

Node *parse_for_update(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = create_node_parse(state, NODE_FOR_UPDATE, "");
	Node *assignment_stmt = parse_assignment(state);
	if (assignment_stmt) {
		append_child(curr, assignment_stmt);
		return curr;
	}
	parser_restore(cp);
	Node *expression = parse_expression(state);
	if (expression) {
		append_child(curr, expression);
		return curr;
	}
	parser_restore(cp);
	destroy_node(curr);
	return NULL;
}

Node *parse_range_for_loop(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "for")) {
		return NULL;
	}
	consume_string(ts, "for");
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		parser_restore(cp);
		return NULL;
	}
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *expression = parse_expression(state);
	if (!expression) {
		parser_restore(cp);
		destroy_node(identifier);
		return NULL;
	}
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		destroy_node(identifier);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_RANGE_FOR_LOOP, "");
	append_child(curr, identifier);
	append_child(curr, expression);
	append_child(curr, block);
	return curr;
}

Node *parse_switch_stmt(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "switch")) {
		return NULL;
	}
	consume_string(ts, "switch");

	Node *expression = parse_expression(state);

	if (!expression) {
		parser_restore(cp);
		return NULL;
	}

	if (!consume_if(ts, '{')) {
		parser_restore(cp);
		destroy_node(expression);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_SWITCH_STMT, "");
	append_child(curr, expression);
	Node *node = parse_switch_case(state);
	while (node) {
		append_child(curr, node);
		node = parse_switch_case(state);
	}
	Node *switch_default = parse_switch_default(state);
	append_child(curr, switch_default);
	if (!consume_if(ts, '}')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

Node *parse_switch_case(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "case")) {
		return NULL;
	}
	consume_string(ts, "case");
	Node *case_pattern = parse_case_pattern(state);
	if (!case_pattern) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_SWITCH_CASE, "");
	while (case_pattern) {
		append_child(curr, case_pattern);
		if (!consume_if(ts, ',')) {
			break;
		}
		case_pattern = parse_case_pattern(state);
	}
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	append_child(curr, block);
	return curr;
}

Node *parse_case_pattern(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *expression = parse_expression(state);
	if (expression) {
		return expression;
	}
	parser_restore(cp);
	Node *literal = parse_literal(state);
	if (literal) {
		return literal;
	}
	parser_restore(cp);
	Node *identifier = parse_identifier(state);
	if (identifier) {
		return identifier;
	}
	parser_restore(cp);
	return NULL;
}

Node *parse_switch_default(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!peek_keyword(ts, "default")) {
		return NULL;
	}
	consume_string(ts, "default");
	if (!consume_if(ts, ':')) {
		parser_restore(cp);
		return NULL;
	}
	Node *block = parse_block(state);
	if (!block) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_SWITCH_DEFAULT, "");
	append_child(curr, block);
	return curr;
}

Node *parse_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *logical_or_expression = parse_logical_or_expression(state);
	if (!logical_or_expression) {
		parser_restore(cp);
		return NULL;
	}
	return logical_or_expression;
}

Node *parse_logical_or_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *logical_and_expression = parse_logical_and_expression(state);
	if (!logical_and_expression) {
		parser_restore(cp);
		return NULL;
	}
	if (!peek_string(ts, "||")) {
		return logical_and_expression;
	}
	Node *curr = create_node_parse(state, NODE_LOGICAL_OR_EXPRESSION, "");
	while (peek_string(ts, "||")) {
		append_child(curr, logical_and_expression);
		consume_string(ts, "||");
		logical_and_expression = parse_logical_and_expression(state);
		if (!logical_and_expression) {
			destroy_node(curr);
			destroy_node(logical_and_expression);
			return NULL;
		}
	}
	append_child(curr, logical_and_expression);
	return curr;
}

Node *parse_logical_and_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *equality_expression = parse_equality_expression(state);
	if (!equality_expression) {
		parser_restore(cp);
		return NULL;
	}
	if (!peek_string(ts, "&&")) {
		return equality_expression;
	}
	Node *curr = create_node_parse(state, NODE_LOGICAL_AND_EXPRESSION, "");
	while (peek_string(ts, "&&")) {
		append_child(curr, equality_expression);
		consume_string(ts, "&&");
		equality_expression = parse_equality_expression(state);
		if (!equality_expression) {
			destroy_node(curr);
			destroy_node(equality_expression);
			return NULL;
		}
	}
	append_child(curr, equality_expression);
	return curr;
}

// equality_expression = relational_expression { ("==" | "!=")
// relational_expression } ;
Node *parse_equality_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *left = parse_relational_expression(state);
	if (!left) {
		parser_restore(cp);
		return NULL;
	}
	while (peek_string(ts, "==") || peek_string(tokens, "!=")) {
		Node *curr = create_node_parse(state, NODE_EQUALITY_EXPRESSION, "");
		if (peek_string(ts, "==")) {
			consume_string(ts, "==");
			curr->data.op = eq_equals;
		} else {
			consume_string(ts, "!=");
			curr->data.op = not_equals;
		}
		Node *right = parse_relational_expression(state);
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

// relational_expression = additive_expression { ("<" | "<=" | ">" | ">=")
// additive_expression } ;
Node *parse_relational_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *left = parse_additive_expression(state);
	if (!left) {
		parser_restore(cp);
		return NULL;
	}
	while (peek_string(ts, "<") || peek_string(tokens, "<=") || peek_string(ts, ">") ||
		   peek_string(tokens, ">=")) {
		Node *curr = create_node_parse(state, NODE_RELATIONAL_EXPRESSION, "");
		if (peek_string(ts, "<=")) {
			consume_string(ts, "<=");
			curr->data.op = less_than_eq;
		} else if (peek_string(ts, ">=")) {
			consume_string(ts, ">=");
			curr->data.op = greater_than_eq;
		} else if (peek_string(ts, "<")) {
			consume_string(ts, "<");
			curr->data.op = less_than;
		} else if (peek_string(ts, ">")) {
			consume_string(ts, ">");
			curr->data.op = greater_than;
		}
		Node *right = parse_additive_expression(state);
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

// additive_expression = multiplicative_expression { ("+" | "-")
// multiplicative_expression } ;
Node *parse_additive_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *left = parse_multiplicative_expression(state);
	if (!left) {
		parser_restore(cp);
		return NULL;
	}
	while (peek_string(ts, "+") || peek_string(tokens, "-")) {
		Node *curr = create_node_parse(state, NODE_ADDITIVE_EXPRESSION, "");
		if (peek_string(ts, "+")) {
			consume_string(ts, "+");
			curr->data.op = plus;
		} else if (peek_string(ts, "-")) {
			consume_string(ts, "-");
			curr->data.op = minus;
		}
		Node *right = parse_multiplicative_expression(state);
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

// multiplicative_expression = unary_expression { ("*" | "/" | "%")
// unary_expression } ;
Node *parse_multiplicative_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *left = parse_unary_expression(state);
	if (!left) {
		parser_restore(cp);
		return NULL;
	}
	while (peek_string(ts, "*") || peek_string(tokens, "/") || peek_string(ts, "%")) {
		Node *curr = create_node_parse(state, NODE_MULTIPLICTIVE_EXPRESSION, "");
		if (peek_string(ts, "*")) {
			consume_string(ts, "*");
			curr->data.op = star;
		} else if (peek_string(ts, "/")) {
			consume_string(ts, "/");
			curr->data.op = slash;
		} else if (peek_string(ts, "%")) {
			consume_string(ts, "%");
			curr->data.op = percent;
		}

		Node *right = parse_unary_expression(state);
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
// unary_expression = ("-" | "!" | "~" | "*" | "&") unary_expression
//                  | postfix_expression
//                  ;
Node *parse_unary_expression(ParseState *state) {
	if (peek_string(ts, "-") || peek_string(tokens, "!") || peek_string(ts, "~") ||
		peek_string(tokens, "*") || peek_string(ts, "&")) {
		Node *curr = create_node(state, NODE_UNARY_EXPRESSION, "");
		if (peek_string(ts, "-")) {
			consume_string(ts, "-");
			curr->data.op = minus;
		} else if (peek_string(ts, "!")) {
			consume_string(ts, "!");
			curr->data.op = log_not;
		} else if (peek_string(ts, "~")) {
			curr->data.op = bit_not;
			consume_string(ts, "~");
		} else if (peek_string(ts, "*")) {
			curr->data.op = star;
			consume_string(ts, "*");
		} else {
			curr->data.op = and_perc;
			consume_string(ts, "&");
		}
		Node *operand = parse_unary_expression(state);
		if (!operand) {
			destroy_node(curr);
			return NULL;
		}
		append_child(curr, operand);
		return curr;
	}
	return parse_postfix_expression(state);
}

// postfix_expression = primary_expression
//                    { "(" argument_list ")"      (* function call *)
//                    | "." identifier             (* field/method access *)
//                    | "[" expression "]"         (* indexing *)
//                    | "[" expression? ":" expression? "]"  (* slicing *)
//                    | "++" | "--"                (* postfix inc/dec *)
//                    } ;
Node *parse_postfix_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *left = parse_primary_expression(state);
	if (!left) {
		parser_restore(cp);
		return NULL;
	}
	while (1) {
		if (consume_if(ts, '(')) {
			Node *args = parse_argument_list(state);
			if (!consume_if(ts, ')')) {
				destroy_node(args);
				destroy_node(left);
				parser_restore(cp);
				return NULL;
			}
			Node *curr = create_node_parse(state, NODE_FUNC_CALL, "");
			append_child(curr, left);
			append_child(curr, args);
			left = curr;
			continue;
		}
		if (consume_if(ts, '.')) {
			Node *identifier = parse_identifier(state);
			if (!identifier) {
				destroy_node(left);
				parser_restore(cp);
				return NULL;
			}
			Node *curr = create_node_parse(state, NODE_ACCESS, "");
			append_child(curr, left);
			append_child(curr, identifier);
			left = curr;
			continue;
		}
		if (consume_if(ts, '[')) {
			Node *curr = create_node_parse(state, NODE_INDEX, "");
			append_child(curr, left);
			Node *expr_a = parse_expression(state);
			append_child(curr, expr_a);
			if (consume_if(ts, ':')) {
				Node *expr_b = parse_expression(state);
				append_child(curr, expr_b);
			}
			if (!consume_if(ts, ']')) {
				destroy_node(curr);
				parser_restore(cp);
				return NULL;
			}
			left = curr;
			continue;
		}
		if (peek_string(ts, "++")) {
			consume_string(ts, "++");
			Node *curr = create_node_parse(state, NODE_INC_DEC, "");
			curr->data.op = plus_plus;
			append_child(curr, left);
			left = curr;
			continue;
		}
		if (peek_string(ts, "--")) {
			consume_string(ts, "--");
			Node *curr = create_node_parse(state, NODE_INC_DEC, "");
			curr->data.op = minus_minus;
			append_child(curr, left);
			left = curr;
			continue;
		}
		break;
	}
	return left;
}

// primary_expression = literal
//                    | identifier
//                    | "(" expression ")"
//                    | "sizeof" "(" type ")"
//                    | "(" type ")" expression    (* cast *)
//                    ;
Node *parse_primary_expression(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_identifier(state);
	if (curr)
		return curr;
	parser_restore(cp);
	if (peek_keyword(ts, "sizeof")) {
		consume_string(ts, "sizeof");
		if (!consume_if(ts, '(')) {
			parser_restore(cp);
			return NULL;
		}
		Node *type = parse_type(state);
		if (!type) {
			parser_restore(cp);
			return NULL;
		}
		if (!consume_if(ts, ')')) {
			destroy_node(type);
			parser_restore(cp);
			return NULL;
		}
		curr = create_node_parse(state, NODE_SIZE_OF_EXPRESSION, "");
		append_child(curr, type);
		return curr;
	}
	parser_restore(cp);
	if (consume_if(ts, '(')) {
		Node *type = parse_type(state);
		if (type && consume_if(ts, ')')) {
			Node *expr = parse_unary_expression(state);
			if (expr) {
				Node *cast = create_node_parse(state, NODE_CAST_EXPRESSION, "");
				append_child(cast, type);
				append_child(cast, expr);
				return cast;
			}
		}
		parser_restore(cp);
		consume_if(ts, '(');
		Node *expr = parse_expression(state);
		if (!expr) {
			parser_restore(cp);
			return NULL;
		}
		if (!consume_if(ts, ')')) {
			destroy_node(expr);
			parser_restore(cp);
			return NULL;
		}
		Node *group = create_node_parse(state, NODE_GROUPED_EXPRESSION, "");
		append_child(group, expr);
		return group;
	}
	destroy_node(curr);
	return NULL;
}

// argument_list = [ expression { "," expression } ] ;
Node *parse_argument_list(ParseState *state) {
	Node *curr = create_node(state, NODE_ARGUMENT_LIST, "");
	Node *expression = parse_expression(state);
	while (expression) {
		append_child(curr, expression);
		if (!consume_if(ts, ',')) {
			break;
		}
		expression = parse_expression(state);
	}
	return curr;
}

// literal = integer_literal
//         | float_literal
//         | string_literal
//         | char_literal
//         | boolean_literal
//         | array_literal
//         ;
Node *parse_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_integer_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_float_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_string_literal(state);
	if (curr)
		return curr;
	curr = parse_char_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_boolean_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_array_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	return NULL;
}
// integer_literal = decimal_literal | hex_literal | octal_literal |
// binary_literal ;
Node *parse_integer_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_binary_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_hex_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_octal_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	curr = parse_decimal_literal(state);
	if (curr)
		return curr;
	parser_restore(cp);
	return NULL;
}

// float_literal = decimal_digits "." decimal_digits [ "e" [ "+" "-" ]
// decimal_digits ] ;
Node *parse_float_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *right = parse_decimal_digits(state);
	if (!right) {
		parser_restore(cp);
		return NULL;
	}
	destroy_node(right);
	if (!consume_if(ts, '.')) {
		parser_restore(cp);
		return NULL;
	}
	Node *left = parse_decimal_digits(state);
	if (!left) {
		parser_restore(cp);
		return NULL;
	}
	destroy_node(left);
	Node *curr = create_node_parse(state, NODE_FLOAT_LITERAL, "");
	if (consume_if(ts, 'e')) {
		if (!consume_if(ts, '+'))
			consume_if(ts, '-');
		Node *exponent = parse_decimal_digits(state);
		if (!exponent) {
			parser_restore(cp);
			destroy_node(curr);
			return NULL;
		}
		destroy_node(exponent);
	}
	curr->data.literal.start = cp.position;
	curr->data.literal.end = ts->position;
	return curr;
}

// string_literal = '"' { string_char } '"' ;  (* yields []u8 *)
Node *parse_string_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!consume_if(ts, '"')) {
		parser_restore(cp);
		return NULL;
	}
	while (peek_raw(ts) != '"' && peek_raw(tokens) != '\0') {
		if (peek_raw(ts) == '\\') {
			consume_raw(ts);
			if (peek_raw(ts) == '\0')
				break;
		}
		consume_raw(ts);
	}
	if (!consume_if(ts, '"')) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_STRING_LITERAL, "");
	curr->data.literal.start = cp.position + 1;
	curr->data.literal.end = ts->position - 1;
	return curr;
}

// char_literal = ' string_char ' ;  (* yields u8 *)
Node *parse_char_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!consume_if(ts, '\'')) {
		parser_restore(cp);
		return NULL;
	}
	if (peek_raw(ts) != '\'' && peek_raw(tokens) != '\0') {
		if (peek_raw(ts) == '\\') {
			consume_raw(ts);
			if (peek_raw(ts) == '\0') {
				parser_restore(cp);
				return NULL;
			}
		}
		consume_raw(ts);
	}
	if (!consume_if(ts, '\'')) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_CHAR_LITERAL, "");
	curr->data.literal.start = cp.position + 1;
	curr->data.literal.end = ts->position - 1;
	return curr;
}
// boolean_literal = "true" | "false" ;
Node *parse_boolean_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (peek_keyword(ts, "true")) {
		cp.position = ts->position;
		consume_string(ts, "true");
	} else if (peek_keyword(ts, "false")) {
		cp.position = ts->position;
		consume_string(ts, "false");
	} else {
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_BOOLEAN_LITERAL, "");
	curr->data.literal.start = cp.position;
	curr->data.literal.end = ts->position;
	return curr;
}

// array_literal = "[" [ expression { "," expression } ] "]" ;  (* slice literal
// *)
Node *parse_array_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	if (!consume_if(ts, '[')) {
		parser_restore(cp);
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_ARRAY_LITERAL, "");
	Node *expression = parse_expression(state);
	while (expression) {
		append_child(curr, expression);
		if (!consume_if(ts, ',')) {
			break;
		}
		expression = parse_expression(state);
	}
	if (!consume_if(ts, ']')) {
		parser_restore(cp);
		destroy_node(curr);
		return NULL;
	}
	return curr;
}

// identifier = letter { letter | digit | "_" } ;
Node *parse_identifier(ParseState *state) {
	if (ts_peek(ts)->kind != TKN_IDENT) {
		return NULL;
	}
	Node *curr = create_node_parse(state, NODE_IDENTIFER, "");
	curr->data.literal.source = ts->lex.src;
	curr->data.literal.start = ident_tkn->offset;
	curr->data.literal.end = ident_tkn->offset + ident_tkn->length;
	return curr;
}

// decimal_literal = decimal_digits ;
Node *parse_decimal_literal(ParseState *state) {
	ParserCheckpoint cp = parser_save(state);
	Node *curr = parse_decimal_digits(state);
	if (!curr) {
		parser_restore(cp);
		return NULL;
	}
	curr->type = NODE_DECIMAL_LITERAL;
	return curr;
}

// hex_literal = "0x" hex_digits ;
Node *parse_hex_literal(ParseState *state) {
	if (!peek_string(ts, "0x")) {
		return NULL;
	}

	ParserCheckpoint cp = parser_save(state);
	consume_string(ts, "0x");
	while (isxdigit(peek_raw(ts))) {
		consume_raw(ts);
	}
	Node *curr = create_node_parse(state, NODE_HEX_LITERAL, "");
	curr->data.literal.start = cp.position;
	curr->data.literal.end = ts->position;
	curr->data.literal.source = ts->data;
	return curr;
}

// octal_literal = "0o" octal_digits ;
Node *parse_octal_literal(ParseState *state) {
	if (!peek_string(ts, "0o")) {
		return NULL;
	}

	ParserCheckpoint cp = parser_save(state);
	consume_string(ts, "0o");
	while (peek_raw(ts) >= '0' && peek_raw(tokens) <= '7') {
		consume_raw(ts);
	}
	Node *curr = create_node_parse(state, NODE_OCTAL_LITERAL, "");
	curr->data.literal.start = cp.position;
	curr->data.literal.end = ts->position;
	curr->data.literal.source = ts->data;
	return curr;
}

// binary_literal = "0b" binary_digits ;
Node *parse_binary_literal(ParseState *state) {
	if (!peek_string(ts, "0b")) {
		return NULL;
	}

	ParserCheckpoint cp = parser_save(state);
	consume_string(ts, "0b");
	while (peek_raw(ts) == '1' || peek_raw(tokens) == '0') {
		consume_raw(ts);
	}
	Node *curr = create_node_parse(state, NODE_BINARY_LITERAL, "");
	curr->data.literal.start = cp.position;
	curr->data.literal.end = ts->position;
	curr->data.literal.source = ts->data;
	return curr;
}

// decimal_digits = digit { digit } ;
Node *parse_decimal_digits(ParseState *state) {
	if (!isdigit(ts_peek(ts))) {
		return NULL;
	}

	ParserCheckpoint cp = parser_save(state);
	while (isdigit(peek_raw(ts))) {
		consume_raw(ts);
	}
	Node *curr = create_node_parse(state, NODE_DECIMAL_DIGIT, "");
	curr->data.literal.start = cp.position;
	curr->data.literal.end = ts->position;
	curr->data.literal.source = ts->data;
	return curr;
}
