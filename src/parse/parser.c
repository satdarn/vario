#include "../parse/parser.h"

// ============================================================
// This parser is predictive by default: every dispatch point resolves
// with bounded peek()/peek_n(), not backtracking. There is no
// ParserCheckpoint / parser_save / parser_restore anywhere in this file.
// The one place that historically needed backtracking (casts) was
// removed by switching to infix "as" casts — see parse_cast_expression.
//
// Invariant: once a parse_X function has consumed its own leading/
// committing token, it must never return NULL again. NULL is reserved
// exclusively for "this isn't an X, caller should try something else" —
// a decision made via peek(), before consuming anything. See
// context.md for the full design rationale.
// ============================================================

static const TokenKind TOP_LEVEL_SYNC[] = {
	TKN_KW_FN,	  TKN_KW_OBJ,	TKN_KW_INTER,  TKN_KW_ENUM,
	TKN_KW_UNION, TKN_KW_CONST, TKN_KW_MODULE, TKN_KW_IMPORT,
};
#define TOP_LEVEL_SYNC_N (sizeof(TOP_LEVEL_SYNC) / sizeof(TOP_LEVEL_SYNC[0]))

static const TokenKind BLOCK_SYNC[] = {
	TKN_RBRACE,	   TKN_KW_VAR,	 TKN_KW_LET,	  TKN_KW_IF,	TKN_KW_WHILE,  TKN_KW_FOR,
	TKN_KW_RETURN, TKN_KW_BREAK, TKN_KW_CONTINUE, TKN_KW_DEFER, TKN_KW_SWITCH,
};
#define BLOCK_SYNC_N (sizeof(BLOCK_SYNC) / sizeof(BLOCK_SYNC[0]))

static const TokenKind SWITCH_BODY_SYNC[] = {
	TKN_KW_CASE,
	TKN_KW_DEFAULT,
	TKN_RBRACE,
};
#define SWITCH_BODY_SYNC_N (sizeof(SWITCH_BODY_SYNC) / sizeof(SWITCH_BODY_SYNC[0]))

static bool is_primitive_type_token(TokenKind k) {
	switch (k) {
	case TKN_KW_U8:
	case TKN_KW_U32:
	case TKN_KW_U64:
	case TKN_KW_I32:
	case TKN_KW_I64:
	case TKN_KW_F32:
	case TKN_KW_F64:
	case TKN_KW_BOOL:
	case TKN_KW_VOID:
	case TKN_KW_USIZE:
	case TKN_KW_ISIZE:
		return true;
	default:
		return false;
	}
}

static bool is_assignment_operator_token(TokenKind k) {
	switch (k) {
	case TKN_EQ:
	case TKN_PLUSEQ:
	case TKN_MINUSEQ:
	case TKN_STAREQ:
	case TKN_SLASHEQ:
	case TKN_PERCENTEQ:
	case TKN_AMPEQ:
	case TKN_PIPEEQ:
	case TKN_CARROTEQ:
	case TKN_LTLTEQ:
	case TKN_GTGTEQ:
		return true;
	default:
		return false;
	}
}

// Shared by statement-level assignment/expression and for-loop init/update,
// which need the same "expression, optionally followed by an assignment
// operator and rhs" shape but differ in who owns the trailing ';'.
static Node *build_assignment_or_expr(ParseState *state) {
	Node *expr = parse_expression(state);
	if (!expr) {
		report_error(state, "expected an expression");
		return create_node(state->arena, NODE_ERROR, "");
	}
	if (!is_assignment_operator_token(peek(state))) {
		return expr;
	}
	Node *op = parse_assignment_operator(state);
	Node *rhs = parse_expression(state);
	if (!rhs)
		report_error(state, "expected an expression after assignment operator");
	Node *curr = create_node(state->arena, NODE_ASSIGNMENT_STMT, "");
	append_child(curr, expr);
	append_child(curr, op);
	append_child(curr, rhs);
	return curr;
}

// ============================================================
// TOP LEVEL
// ============================================================

Node *parse(Arena *arena, TokenStream *ts, bool *had_errors) {
	ParseState state = {0};
	state.ts = ts;
	state.arena = arena;

	Node *root = parse_program(&state);

	bool errored = arrlen(state.errors) > 0;
	for (int i = 0; i < arrlen(state.errors); i++) {
		fprintf(stderr, "parse error at %zu:%zu: %s\n", state.errors[i].line,
				state.errors[i].col, state.errors[i].msg);
	}
	arrfree(state.errors);
	destroy_arena(&state.error_arena);

	if (had_errors)
		*had_errors = errored;
	return root;
}

Node *parse_program(ParseState *state) {
	Node *root = create_node(state->arena, NODE_PROGRAM, "Root");
	while (peek(state) != TKN_EOF) {
		Node *curr = parse_top_level_decl(state);
		if (!curr) {
			report_error(state, "expected a top-level declaration");
			synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
			continue;
		}
		append_child(root, curr);
	}
	return root;
}

Node *parse_top_level_decl(ParseState *state) {
	TokenKind k = (peek(state) == TKN_KW_PUB) ? peek_n(state, 1) : peek(state);
	switch (k) {
	case TKN_KW_MODULE:
		return parse_module_decl(state);
	case TKN_KW_IMPORT:
		return parse_import_decl(state);
	case TKN_KW_FN:
		return parse_func_decl(state);
	case TKN_KW_OBJ:
		return parse_obj_decl(state);
	case TKN_KW_INTER:
		return parse_inter_decl(state);
	case TKN_KW_ENUM:
		return parse_enum_decl(state);
	case TKN_KW_UNION:
		return parse_union_decl(state);
	case TKN_KW_CONST:
		return parse_const_decl(state);
	default:
		return NULL; // parse_program's loop handles this
	}
}

// module_decl = "module" identifier ";" ;
Node *parse_module_decl(ParseState *state) {
	advance(state); // consume 'module'
	Node *curr = create_node(state->arena, NODE_MODULE_DECL, "");
	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'module'");
	if (!expect(state, TKN_SEMI, "';' after module name")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
	}
	return curr;
}

// import_decl = "import" identifier ( "as" identifier )? ";" ;
Node *parse_import_decl(ParseState *state) {
	advance(state); // consume 'import'
	Node *curr = create_node(state->arena, NODE_IMPORT_DECL, "");
	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'import'");

	if (peek(state) == TKN_KW_AS) {
		advance(state);
		Node *alias = parse_identifier(state);
		if (alias)
			append_child(curr, alias);
		else
			report_error(state, "expected an identifier after 'as'");
	}
	if (!expect(state, TKN_SEMI, "';' after import")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
	}
	return curr;
}

// visibility = [ "pub" ] ;
Node *parse_visiblity(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_VISIBLITY, "");
	curr->data.visiblity = (peek(state) == TKN_KW_PUB);
	if (curr->data.visiblity)
		advance(state);
	return curr;
}

// ============================================================
// FUNCTIONS
// ============================================================

// func_decl = visibility "fn" identifier [ generic_param_list ] "(" parameter_list ")"
// return_type block ;
Node *parse_func_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	advance(state); // consume 'fn'
	Node *curr = create_node(state->arena, NODE_FUNC_DECL, "");
	append_child(curr, visiblity);

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected a function name after 'fn'");

	Node *generics =
		(peek(state) == TKN_LBRACKET) ? parse_generic_param_list(state) : NULL;
	append_child(curr, generics);

	if (!expect(state, TKN_LPAREN, "'(' to begin parameter list")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
		return curr;
	}
	append_child(curr, parse_parameter_list(state));
	if (!expect(state, TKN_RPAREN, "')' to close parameter list")) {
		const TokenKind sync[] = {TKN_ARROW, TKN_LBRACE};
		synchronize(state, sync, 2);
	}
	append_child(curr, parse_return_type(state));
	append_child(curr, parse_block(state));
	return curr;
}

// generic_param_list = "[" generic_param { "," generic_param } "]" ;
Node *parse_generic_param_list(ParseState *state) {
	advance(state); // consume '['
	Node *curr = create_node(state->arena, NODE_GENERIC_PARAM_LIST, "");
	Node *param = parse_generic_param(state);
	if (param)
		append_child(curr, param);
	while (peek(state) == TKN_COMMA) {
		advance(state);
		param = parse_generic_param(state);
		if (param)
			append_child(curr, param);
	}
	if (!expect(state, TKN_RBRACKET, "']' to close generic parameter list")) {
		const TokenKind sync[] = {TKN_LPAREN, TKN_LBRACE};
		synchronize(state, sync, 2);
	}
	return curr;
}

// generic_param = identifier [ ":" bound_list ] ;
Node *parse_generic_param(ParseState *state) {
	Node *identifier = parse_identifier(state);
	if (!identifier) {
		report_error(state, "expected a type parameter name");
		const TokenKind sync[] = {TKN_COMMA, TKN_RBRACKET};
		synchronize(state, sync, 2);
		return create_node(state->arena, NODE_ERROR, "");
	}
	Node *curr = create_node(state->arena, NODE_GENERIC_PARAM, "");
	append_child(curr, identifier);
	if (peek(state) == TKN_COLON) {
		advance(state);
		append_child(curr, parse_bound_list(state));
	}
	return curr;
}

// bound_list = identifier { "+" identifier } ;
Node *parse_bound_list(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_BOUND_LIST, "");
	Node *bound = parse_identifier(state);
	if (!bound) {
		report_error(state, "expected an interface name after ':'");
		return curr;
	}
	append_child(curr, bound);
	while (peek(state) == TKN_PLUS) {
		advance(state);
		bound = parse_identifier(state);
		if (!bound) {
			report_error(state, "expected an interface name after '+'");
			break;
		}
		append_child(curr, bound);
	}
	return curr;
}

// type_args = "[" type { "," type } "]" ;
Node *parse_type_args(ParseState *state) {
	advance(state); // consume '['
	Node *curr = create_node(state->arena, NODE_TYPE_ARGS, "");
	Node *arg = parse_type(state);
	if (arg)
		append_child(curr, arg);
	else
		report_error(state, "expected a type");
	while (peek(state) == TKN_COMMA) {
		advance(state);
		arg = parse_type(state);
		if (arg)
			append_child(curr, arg);
		else
			report_error(state, "expected a type after ','");
	}
	if (!expect(state, TKN_RBRACKET, "']' to close type argument list")) {
		const TokenKind sync[] = {TKN_LPAREN, TKN_LBRACE, TKN_SEMI};
		synchronize(state, sync, 3);
	}
	return curr;
}

// parameter_list = [ parameter { "," parameter } ] ;
Node *parse_parameter_list(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_PARAMETER_LIST, "");
	if (peek(state) != TKN_IDENT)
		return curr; // empty list
	Node *node = parse_parameter(state);
	if (node)
		append_child(curr, node);
	while (peek(state) == TKN_COMMA) {
		advance(state);
		node = parse_parameter(state);
		if (node)
			append_child(curr, node);
	}
	return curr;
}

// parameter = identifier ":" type ;
Node *parse_parameter(ParseState *state) {
	Node *identifier = parse_identifier(state);
	if (!identifier)
		return NULL; // speculative — caller already checked TKN_IDENT
	Node *curr = create_node(state->arena, NODE_PARAMETER, "");
	append_child(curr, identifier);
	if (!expect(state, TKN_COLON, "':' after parameter name")) {
		const TokenKind sync[] = {TKN_COMMA, TKN_RPAREN};
		synchronize(state, sync, 2);
		return curr;
	}
	Node *type = parse_type(state);
	if (!type)
		report_error(state, "expected a parameter type");
	append_child(curr, type);
	return curr;
}

// return_type = "->" type ;  (mandatory everywhere it's called — never optional)
Node *parse_return_type(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_RETURN_TYPE, "");
	if (!expect(state, TKN_ARROW, "'->' before return type")) {
		return curr;
	}
	Node *type = parse_type(state);
	if (!type)
		report_error(state, "expected a return type");
	append_child(curr, type);
	return curr;
}

// block = "{" { statement } "}" ;  (mandatory everywhere it's called)
Node *parse_block(ParseState *state) {
	if (!expect(state, TKN_LBRACE, "'{' to begin block")) {
		return create_node(state->arena, NODE_BLOCK, "");
	}
	Node *curr = create_node(state->arena, NODE_BLOCK, "");
	while (peek(state) != TKN_RBRACE && peek(state) != TKN_EOF) {
		Node *stmt = parse_statement(state);
		if (stmt) {
			append_child(curr, stmt);
		} else {
			report_error(state, "expected a statement");
			synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
		}
	}
	expect(state, TKN_RBRACE, "'}' to close block");
	return curr;
}

// ============================================================
// TYPES
// ============================================================

// type = primitive_type | pointer_type | slice_type | obj_type | self_type ;
Node *parse_type(ParseState *state) {
	switch (peek(state)) {
	case TKN_STAR:
		return parse_pointer_type(state);
	case TKN_LBRACKET:
		return parse_slice_type(state);
	case TKN_KW_SELF_TYPE:
		return parse_self_type(state);
	case TKN_IDENT:
		return parse_obj_type(state);
	default:
		if (is_primitive_type_token(peek(state)))
			return parse_primitive_type(state);
		return NULL;
	}
}

static const struct {
	TokenKind kind;
	PrimitiveType pt;
} PRIMITIVE_TYPE_MAP[] = {
	{TKN_KW_U8, u8},	   {TKN_KW_U32, u32},	   {TKN_KW_U64, u64},
	{TKN_KW_I32, i32},	   {TKN_KW_I64, i64},	   {TKN_KW_F32, f32},
	{TKN_KW_F64, f64},	   {TKN_KW_BOOL, boolean}, {TKN_KW_VOID, voidian},
	{TKN_KW_USIZE, usize}, {TKN_KW_ISIZE, isize},
};
#define PRIMITIVE_TYPE_MAP_N (sizeof(PRIMITIVE_TYPE_MAP) / sizeof(PRIMITIVE_TYPE_MAP[0]))

Node *parse_primitive_type(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_PRIMITIVE_TYPE, "");
	for (size_t i = 0; i < PRIMITIVE_TYPE_MAP_N; i++) {
		if (PRIMITIVE_TYPE_MAP[i].kind == peek(state)) {
			curr->data.primitive_type = PRIMITIVE_TYPE_MAP[i].pt;
			advance(state);
			return curr;
		}
	}
	return curr; // unreachable via parse_type's dispatch, but stay safe
}

// pointer_type = "*" type ;
Node *parse_pointer_type(ParseState *state) {
	advance(state); // consume '*'
	Node *curr = create_node(state->arena, NODE_POINTER_TYPE, "");
	Node *type = parse_type(state);
	if (!type)
		report_error(state, "expected a type after '*'");
	append_child(curr, type);
	return curr;
}

// slice_type = "[" [ integer_literal ] "]" type ;
Node *parse_slice_type(ParseState *state) {
	advance(state); // consume '['
	Node *curr = create_node(state->arena, NODE_SLICE_TYPE, "");
	Node *integer =
		(peek(state) == TKN_INT_LITERAL) ? parse_integer_literal(state) : NULL;
	if (!expect(state, TKN_RBRACKET, "']' in array/slice type")) {
		const TokenKind sync[] = {TKN_LPAREN, TKN_LBRACE, TKN_SEMI, TKN_COMMA};
		synchronize(state, sync, 4);
	}
	Node *type = parse_type(state);
	if (!type)
		report_error(state, "expected an element type after ']'");
	append_child(curr, type);
	append_child(curr, integer);
	return curr;
}

// obj_type = identifier [ type_args ] ;
Node *parse_obj_type(ParseState *state) {
	Node *identifier = parse_identifier(state);
	if (!identifier)
		return NULL; // speculative — dispatcher already confirmed TKN_IDENT
	identifier->type = NODE_OBJ_TYPE;
	if (peek(state) == TKN_LBRACKET) {
		append_child(identifier, parse_type_args(state));
	}
	return identifier;
}

// self_type = "Self" ;
Node *parse_self_type(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_SELF_TYPE, "Self");
	advance(state);
	return curr;
}

// ============================================================
// INTERFACES
// ============================================================

// inter_decl = visibility "inter" identifier "{" { inter_method_sig } "}" ";" ;
Node *parse_inter_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	advance(state); // consume 'inter'
	Node *curr = create_node(state->arena, NODE_INTER_DECL, "");
	append_child(curr, visiblity);

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'inter'");

	if (!expect(state, TKN_LBRACE, "'{' to begin interface body")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
		return curr;
	}
	while (peek(state) != TKN_RBRACE && peek(state) != TKN_EOF) {
		if (peek(state) == TKN_KW_FN) {
			append_child(curr, parse_inter_method_sig(state));
		} else {
			report_error(state, "expected a method signature");
			const TokenKind sync[] = {TKN_RBRACE, TKN_KW_FN};
			synchronize(state, sync, 2);
		}
	}
	expect(state, TKN_RBRACE, "'}' to close interface body");
	if (!expect(state, TKN_SEMI, "';' after interface declaration")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
	}
	return curr;
}

// inter_method_sig = "fn" identifier "(" [ self_param [ "," parameter_list ] |
// parameter_list ] ")" return_type ";" ;
Node *parse_inter_method_sig(ParseState *state) {
	advance(state); // consume 'fn'
	Node *curr = create_node(state->arena, NODE_INTER_METHOD_SIG, "");

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected a method name after 'fn'");

	if (!expect(state, TKN_LPAREN, "'(' to begin parameter list")) {
		const TokenKind sync[] = {TKN_RBRACE, TKN_KW_FN};
		synchronize(state, sync, 2);
		return curr;
	}

	Node *self_param = parse_self_param(state);
	Node *params;
	if (self_param && peek(state) == TKN_COMMA) {
		advance(state);
		params = parse_parameter_list(state);
	} else if (!self_param) {
		params = parse_parameter_list(state);
	} else {
		params = NULL;
	}
	append_child(curr, self_param);
	append_child(curr, params);

	if (!expect(state, TKN_RPAREN, "')' to close parameter list")) {
		const TokenKind sync[] = {TKN_ARROW, TKN_SEMI, TKN_RBRACE};
		synchronize(state, sync, 3);
	}
	append_child(curr, parse_return_type(state));

	if (!expect(state, TKN_SEMI, "';' after interface method signature")) {
		const TokenKind sync[] = {TKN_RBRACE, TKN_KW_FN};
		synchronize(state, sync, 2);
	}
	return curr;
}

// ============================================================
// OBJECTS
// ============================================================

// obj_decl = visibility "obj" identifier [ generic_param_list ] "{" { obj_member } "}"
// ";" ;
Node *parse_obj_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	advance(state); // consume 'obj'
	Node *curr = create_node(state->arena, NODE_OBJ_DECL, "");
	append_child(curr, visiblity);

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'obj'");

	Node *generics =
		(peek(state) == TKN_LBRACKET) ? parse_generic_param_list(state) : NULL;
	append_child(curr, generics);

	if (!expect(state, TKN_LBRACE, "'{' to begin obj body")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
		return curr;
	}
	while (peek(state) != TKN_RBRACE && peek(state) != TKN_EOF) {
		Node *member = parse_obj_member(state);
		if (member) {
			append_child(curr, member);
		} else {
			report_error(state, "expected a field or method declaration");
			const TokenKind sync[] = {TKN_RBRACE, TKN_KW_PUB, TKN_KW_VAR, TKN_KW_FN};
			synchronize(state, sync, 4);
		}
	}
	expect(state, TKN_RBRACE, "'}' to close obj body");
	if (!expect(state, TKN_SEMI, "';' after obj declaration")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
	}
	return curr;
}

// obj_member = field_decl | method_decl ;
// "var" vs "fn" (with an optional leading "pub") is fully resolved by
// bounded lookahead — no backtracking needed.
Node *parse_obj_member(ParseState *state) {
	bool has_pub = peek(state) == TKN_KW_PUB;
	TokenKind next = has_pub ? peek_n(state, 1) : peek(state);
	if (next == TKN_KW_FN)
		return parse_method_decl(state);
	if (next == TKN_KW_VAR)
		return parse_field_decl(state);
	return NULL;
}

// field_decl = visibility var_decl ";" ;
Node *parse_field_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	Node *var_decl = parse_var_decl(state);
	Node *curr = create_node(state->arena, NODE_FIELD_DECL, "");
	append_child(curr, visiblity);
	append_child(curr, var_decl);
	if (!expect(state, TKN_SEMI, "';' after field declaration")) {
		const TokenKind sync[] = {TKN_COMMA, TKN_RBRACE, TKN_KW_VAR, TKN_KW_FN,
								  TKN_KW_PUB};
		synchronize(state, sync, 5);
	}
	return curr;
}

// method_decl = visibility "fn" identifier "(" [ self_param [ "," parameter_list ] |
// parameter_list ] ")" return_type block ;
Node *parse_method_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	advance(state); // consume 'fn'
	Node *curr = create_node(state->arena, NODE_METHOD_DECL, "");
	append_child(curr, visiblity);

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected a method name after 'fn'");

	if (!expect(state, TKN_LPAREN, "'(' to begin parameter list")) {
		const TokenKind sync[] = {TKN_RBRACE, TKN_KW_VAR, TKN_KW_FN, TKN_KW_PUB};
		synchronize(state, sync, 4);
		return curr;
	}

	Node *self_param = parse_self_param(state);
	Node *params;
	if (self_param && peek(state) == TKN_COMMA) {
		advance(state);
		params = parse_parameter_list(state);
	} else if (!self_param) {
		params = parse_parameter_list(state);
	} else {
		params = NULL;
	}
	append_child(curr, self_param);
	append_child(curr, params);

	if (!expect(state, TKN_RPAREN, "')' to close parameter list")) {
		const TokenKind sync[] = {TKN_ARROW, TKN_LBRACE};
		synchronize(state, sync, 2);
	}
	append_child(curr, parse_return_type(state));
	append_child(curr, parse_block(state));
	return curr;
}

// self_param = "self" ":" "*" ( obj_type | self_type ) ;
Node *parse_self_param(ParseState *state) {
	if (peek(state) != TKN_KW_SELF)
		return NULL; // speculative
	advance(state);
	Node *curr = create_node(state->arena, NODE_SELF_PARAM, "");
	if (!expect(state, TKN_COLON, "':' after 'self'"))
		return curr;
	if (!expect(state, TKN_STAR, "'*' — self is always a pointer"))
		return curr;
	Node *type = (peek(state) == TKN_KW_SELF_TYPE) ? parse_self_type(state)
												   : parse_obj_type(state);
	if (!type)
		report_error(state, "expected a type after 'self: *'");
	append_child(curr, type);
	return curr;
}

// ============================================================
// ENUMS
// ============================================================

// enum_decl = visibility "enum" identifier "{" { enum_variant } "}" ";" ;
Node *parse_enum_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	advance(state); // consume 'enum'
	Node *curr = create_node(state->arena, NODE_ENUM_DECL, "");
	append_child(curr, visiblity);

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'enum'");

	if (!expect(state, TKN_LBRACE, "'{' to begin enum body")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
		return curr;
	}
	while (peek(state) != TKN_RBRACE && peek(state) != TKN_EOF) {
		if (peek(state) != TKN_IDENT) {
			report_error(state, "expected an enum variant name");
			const TokenKind sync[] = {TKN_COMMA, TKN_RBRACE};
			synchronize(state, sync, 2);
			continue;
		}
		append_child(curr, parse_enum_variant(state));
	}
	expect(state, TKN_RBRACE, "'}' to close enum body");
	if (!expect(state, TKN_SEMI, "';' after enum declaration")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
	}
	return curr;
}

// enum_variant = identifier ( "=" integer_literal )? [ "," ] ;
Node *parse_enum_variant(ParseState *state) {
	Node *identifier = parse_identifier(state);
	if (!identifier)
		return NULL; // speculative — caller already checked TKN_IDENT
	Node *curr = create_node(state->arena, NODE_ENUM_VARIANT, "");
	append_child(curr, identifier);
	if (peek(state) == TKN_EQ) {
		advance(state);
		Node *integer = parse_integer_literal(state);
		if (!integer)
			report_error(state, "expected an integer literal after '='");
		append_child(curr, integer);
	}
	if (peek(state) == TKN_COMMA)
		advance(state);
	return curr;
}

// ============================================================
// TAGGED UNIONS
// ============================================================

// union_decl = visibility "union" identifier "{" { union_variant } "}" ";" ;
Node *parse_union_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	advance(state); // consume 'union'
	Node *curr = create_node(state->arena, NODE_UNION_DECL, "");
	append_child(curr, visiblity);

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'union'");

	if (!expect(state, TKN_LBRACE, "'{' to begin union body")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
		return curr;
	}
	while (peek(state) != TKN_RBRACE && peek(state) != TKN_EOF) {
		if (peek(state) != TKN_IDENT) {
			report_error(state, "expected a union variant name");
			const TokenKind sync[] = {TKN_COMMA, TKN_RBRACE};
			synchronize(state, sync, 2);
			continue;
		}
		append_child(curr, parse_union_variant(state));
	}
	expect(state, TKN_RBRACE, "'}' to close union body");
	if (!expect(state, TKN_SEMI, "';' after union declaration")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
	}
	return curr;
}

// union_variant = identifier ":" type [ "," ] ;
Node *parse_union_variant(ParseState *state) {
	Node *identifier = parse_identifier(state);
	if (!identifier)
		return NULL; // speculative — caller already checked TKN_IDENT
	Node *curr = create_node(state->arena, NODE_UNION_VARIANT, "");
	append_child(curr, identifier);
	if (!expect(state, TKN_COLON, "':' after union variant name")) {
		const TokenKind sync[] = {TKN_COMMA, TKN_RBRACE};
		synchronize(state, sync, 2);
		return curr;
	}
	Node *type = parse_type(state);
	if (!type)
		report_error(state, "expected a type");
	append_child(curr, type);
	if (peek(state) == TKN_COMMA)
		advance(state);
	return curr;
}

// ============================================================
// CONSTANTS & VARIABLES
// ============================================================

// const_decl = visibility "const" identifier ":" type "=" expression ";" ;
Node *parse_const_decl(ParseState *state) {
	Node *visiblity = parse_visiblity(state);
	advance(state); // consume 'const'
	Node *curr = create_node(state->arena, NODE_CONST_DECL, "");
	append_child(curr, visiblity);

	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'const'");

	if (!expect(state, TKN_COLON, "':' after const name")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
		return curr;
	}
	Node *type = parse_type(state);
	if (!type)
		report_error(state, "expected a type");
	append_child(curr, type);

	if (!expect(state, TKN_EQ, "'=' — const must be initialized")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
		return curr;
	}
	Node *expr = parse_expression(state);
	if (!expr)
		report_error(state, "expected an expression");
	append_child(curr, expr);

	if (!expect(state, TKN_SEMI, "';' after const declaration")) {
		synchronize(state, TOP_LEVEL_SYNC, TOP_LEVEL_SYNC_N);
	}
	return curr;
}

// var_decl = "var" identifier ":" type ( "=" expression )? ;
// let_decl = "let" identifier ":" type ( "=" expression )? ;
// Neither consumes a trailing ';' — declaration_stmt and for_init own that,
// since var_decl is reused bare inside for-loop init clauses.
static Node *parse_var_or_let(ParseState *state, NodeType kind) {
	advance(state); // consume 'var' / 'let'
	Node *curr = create_node(state->arena, kind, "");
	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier");

	if (!expect(state, TKN_COLON, "':' after variable name")) {
		return curr;
	}
	Node *type = parse_type(state);
	if (!type)
		report_error(state, "expected a type after ':'");
	append_child(curr, type);

	if (peek(state) == TKN_EQ) {
		advance(state);
		Node *expr = parse_expression(state);
		if (!expr)
			report_error(state, "expected an expression after '='");
		append_child(curr, expr);
	}
	return curr;
}
Node *parse_var_decl(ParseState *state) { return parse_var_or_let(state, NODE_VAR_DECL); }
Node *parse_let_decl(ParseState *state) { return parse_var_or_let(state, NODE_LET_DECL); }

// ============================================================
// STATEMENTS
// ============================================================

Node *parse_statement(ParseState *state) {
	switch (peek(state)) {
	case TKN_KW_VAR:
	case TKN_KW_LET:
		return parse_declaration_stmt(state);
	case TKN_KW_RETURN:
		return parse_return_stmt(state);
	case TKN_KW_IF:
		return parse_conditional_stmt(state);
	case TKN_KW_WHILE:
	case TKN_KW_FOR:
		return parse_loop_stmt(state);
	case TKN_KW_SWITCH:
		return parse_switch_stmt(state);
	case TKN_KW_BREAK:
		return parse_break_stmt(state);
	case TKN_KW_CONTINUE:
		return parse_continue_stmt(state);
	case TKN_KW_DEFER:
		return parse_defer_stmt(state);
	case TKN_LBRACE:
		return parse_block_stmt(state);
	default:
		return parse_assignment_or_expression_stmt(state);
	}
}

// declaration_stmt = ( var_decl | let_decl ) ";" ;
Node *parse_declaration_stmt(ParseState *state) {
	Node *decl =
		(peek(state) == TKN_KW_VAR) ? parse_var_decl(state) : parse_let_decl(state);
	if (!expect(state, TKN_SEMI, "';' after declaration")) {
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
	}
	return decl;
}

Node *parse_assignment_or_expression_stmt(ParseState *state) {
	Node *curr = build_assignment_or_expr(state);
	if (!expect(state, TKN_SEMI, "';' after statement")) {
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
	}
	return curr;
}

// assignment_operator = "=" | "+=" | "-=" | "*=" | "/=" | "%="
//                     | "&=" | "|=" | "^=" | "<<=" | ">>=" ;
Node *parse_assignment_operator(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_ASSIGNMENT_OPERATOR, "");
	Op op;
	switch (peek(state)) {
	case TKN_EQ:
		op = equals;
		break;
	case TKN_PLUSEQ:
		op = plus_equals;
		break;
	case TKN_MINUSEQ:
		op = minus_equals;
		break;
	case TKN_STAREQ:
		op = star_equals;
		break;
	case TKN_SLASHEQ:
		op = slash_equals;
		break;
	case TKN_PERCENTEQ:
		op = percent_equals;
		break;
	case TKN_AMPEQ:
		op = and_equals;
		break;
	case TKN_PIPEEQ:
		op = or_equals;
		break;
	case TKN_CARROTEQ:
		op = xor_equals;
		break;
	case TKN_LTLTEQ:
		op = lshift_equals;
		break;
	case TKN_GTGTEQ:
		op = rshift_equals;
		break;
	default:
		report_error(state, "expected an assignment operator");
		return curr;
	}
	curr->data.op = op;
	advance(state);
	return curr;
}

// return_stmt = "return" expression? ";" ;
Node *parse_return_stmt(ParseState *state) {
	advance(state); // consume 'return'
	Node *curr = create_node(state->arena, NODE_RETURN_STMT, "");
	if (peek(state) != TKN_SEMI) {
		append_child(curr, parse_expression(state));
	}
	if (!expect(state, TKN_SEMI, "';' after return statement")) {
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
	}
	return curr;
}

// break_stmt = "break" ";" ;
Node *parse_break_stmt(ParseState *state) {
	advance(state);
	Node *curr = create_node(state->arena, NODE_BREAK_STMT, "");
	if (!expect(state, TKN_SEMI, "';' after 'break'"))
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
	return curr;
}

// continue_stmt = "continue" ";" ;
Node *parse_continue_stmt(ParseState *state) {
	advance(state);
	Node *curr = create_node(state->arena, NODE_CONTINUE_STMT, "");
	if (!expect(state, TKN_SEMI, "';' after 'continue'"))
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
	return curr;
}

// defer_stmt = "defer" statement ;
Node *parse_defer_stmt(ParseState *state) {
	advance(state); // consume 'defer'
	Node *curr = create_node(state->arena, NODE_DEFER_STMT, "");
	Node *stmt = parse_statement(state);
	if (!stmt)
		report_error(state, "expected a statement after 'defer'");
	append_child(curr, stmt);
	return curr;
}

// block_stmt = block ;
Node *parse_block_stmt(ParseState *state) { return parse_block(state); }

// conditional_stmt = "if" expression block ( "else" ( conditional_stmt | block ) )? ;
Node *parse_conditional_stmt(ParseState *state) {
	advance(state); // consume 'if'
	Node *curr = create_node(state->arena, NODE_CONDITIONAL_STMT, "");
	Node *cond = parse_expression(state);
	if (!cond)
		report_error(state, "expected a condition after 'if'");
	append_child(curr, cond);
	append_child(curr, parse_block(state));

	if (peek(state) == TKN_KW_ELSE) {
		advance(state);
		if (peek(state) == TKN_KW_IF) {

			append_child(curr, parse_conditional_stmt(state));
		} else {
			append_child(curr, parse_block(state));
		}
	}
	return curr;
}

// loop_stmt = while_loop | for_loop | range_for_loop ;
// "for (" is a classic for-loop, "for identifier :" is a range-for —
// resolved with one token of extra lookahead, no backtracking.
Node *parse_loop_stmt(ParseState *state) {
	if (peek(state) == TKN_KW_WHILE)
		return parse_while_loop(state);
	return (peek_n(state, 1) == TKN_LPAREN) ? parse_for_loop(state)
											: parse_range_for_loop(state);
}

// while_loop = "while" expression block ;
Node *parse_while_loop(ParseState *state) {
	advance(state); // consume 'while'
	Node *curr = create_node(state->arena, NODE_WHILE_LOOP, "");
	Node *cond = parse_expression(state);
	if (!cond)
		report_error(state, "expected a condition after 'while'");
	append_child(curr, cond);
	append_child(curr, parse_block(state));
	return curr;
}

// for_loop = "for" "(" for_init ";" expression ";" for_update ")" block ;
Node *parse_for_loop(ParseState *state) {
	advance(state); // consume 'for'
	Node *curr = create_node(state->arena, NODE_FOR_LOOP, "");
	if (!expect(state, TKN_LPAREN, "'(' after 'for'")) {
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
		return curr;
	}
	append_child(curr, parse_for_init(state));
	if (!expect(state, TKN_SEMI, "';' after for-loop init")) {
		const TokenKind sync[] = {TKN_SEMI, TKN_RPAREN};
		synchronize(state, sync, 2);
	}
	Node *cond = parse_expression(state);
	if (!cond)
		report_error(state, "expected a condition in for-loop");
	append_child(curr, cond);
	if (!expect(state, TKN_SEMI, "';' after for-loop condition")) {
		const TokenKind sync[] = {TKN_SEMI, TKN_RPAREN};
		synchronize(state, sync, 2);
	}
	append_child(curr, parse_for_update(state));
	if (!expect(state, TKN_RPAREN, "')' to close for-loop header")) {
		const TokenKind sync[] = {TKN_LBRACE};
		synchronize(state, sync, 1);
	}
	append_child(curr, parse_block(state));
	return curr;
}

// for_init = var_decl | assignment_stmt | expression_stmt ;  (no trailing ';' — the loop
// owns it)
Node *parse_for_init(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_FOR_INIT, "");
	Node *inner = (peek(state) == TKN_KW_VAR) ? parse_var_decl(state)
											  : build_assignment_or_expr(state);
	append_child(curr, inner);
	return curr;
}

// for_update = assignment_stmt | expression_stmt | expression ;  (no trailing ';')
Node *parse_for_update(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_FOR_UPDATE, "");
	append_child(curr, build_assignment_or_expr(state));
	return curr;
}

// range_for_loop = "for" identifier ":" expression block ;
Node *parse_range_for_loop(ParseState *state) {
	advance(state); // consume 'for'
	Node *curr = create_node(state->arena, NODE_RANGE_FOR_LOOP, "");
	Node *identifier = parse_identifier(state);
	if (identifier)
		append_child(curr, identifier);
	else
		report_error(state, "expected an identifier after 'for'");
	if (!expect(state, TKN_COLON, "':' in range-for loop")) {
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
		return curr;
	}
	Node *expr = parse_expression(state);
	if (!expr)
		report_error(state, "expected an expression after ':'");
	append_child(curr, expr);
	append_child(curr, parse_block(state));
	return curr;
}

// ============================================================
// SWITCH STATEMENT
// ============================================================

// switch_stmt = "switch" expression "{" { switch_case } [ switch_default ] "}" ;
Node *parse_switch_stmt(ParseState *state) {
	advance(state); // consume 'switch'
	Node *curr = create_node(state->arena, NODE_SWITCH_STMT, "");
	Node *expr = parse_expression(state);
	if (!expr)
		report_error(state, "expected an expression after 'switch'");
	append_child(curr, expr);
	if (!expect(state, TKN_LBRACE, "'{' to begin switch body")) {
		synchronize(state, BLOCK_SYNC, BLOCK_SYNC_N);
		return curr;
	}
	while (peek(state) != TKN_RBRACE && peek(state) != TKN_EOF) {
		if (peek(state) == TKN_KW_CASE) {
			append_child(curr, parse_switch_case(state));
		} else if (peek(state) == TKN_KW_DEFAULT) {
			append_child(curr, parse_switch_default(state));
		} else {
			report_error(state, "expected 'case' or 'default'");
			synchronize(state, SWITCH_BODY_SYNC, SWITCH_BODY_SYNC_N);
		}
	}
	expect(state, TKN_RBRACE, "'}' to close switch body");
	return curr;
}

// switch_case = "case" case_pattern { "," case_pattern } ":" block ;
Node *parse_switch_case(ParseState *state) {
	advance(state); // consume 'case'
	Node *curr = create_node(state->arena, NODE_SWITCH_CASE, "");
	Node *pattern = parse_case_pattern(state);
	if (!pattern) {
		report_error(state, "expected a pattern after 'case'");
		synchronize(state, SWITCH_BODY_SYNC, SWITCH_BODY_SYNC_N);
		return curr;
	}
	while (pattern) {
		append_child(curr, pattern);
		if (peek(state) != TKN_COMMA)
			break;
		advance(state); // consume the comma
		pattern = parse_case_pattern(state);
		if (!pattern)
			report_error(state, "expected a pattern after ','");
	}
	if (!expect(state, TKN_COLON, "':' after case pattern(s)")) {
		synchronize(state, SWITCH_BODY_SYNC, SWITCH_BODY_SYNC_N);
		return curr;
	}
	append_child(curr, parse_block(state));
	return curr;
}

// case_pattern = expression | literal | identifier ;
// literal and identifier are already reachable through primary_expression,
// so a single parse_expression call covers the whole rule.
Node *parse_case_pattern(ParseState *state) { return parse_expression(state); }

// switch_default = "default" ":" block ;
Node *parse_switch_default(ParseState *state) {
	advance(state); // consume 'default'
	Node *curr = create_node(state->arena, NODE_SWITCH_DEFAULT, "");
	if (!expect(state, TKN_COLON, "':' after 'default'")) {
		synchronize(state, SWITCH_BODY_SYNC, SWITCH_BODY_SYNC_N);
		return curr;
	}
	append_child(curr, parse_block(state));
	return curr;
}

// ============================================================
// EXPRESSIONS
// ============================================================

Node *parse_expression(ParseState *state) { return parse_logical_or_expression(state); }

// logical_or_expression = logical_and_expression { "||" logical_and_expression } ;
// Flat n-ary node (matches the existing sema convention for this level —
// there's only one operator here, so no per-child Op tagging is needed).
Node *parse_logical_or_expression(ParseState *state) {
	Node *left = parse_logical_and_expression(state);
	if (!left || peek(state) != TKN_PIPEPIPE)
		return left;
	Node *curr = create_node(state->arena, NODE_LOGICAL_OR_EXPRESSION, "");
	while (peek(state) == TKN_PIPEPIPE) {
		append_child(curr, left);
		advance(state);
		left = parse_logical_and_expression(state);
		if (!left) {
			report_error(state, "expected an expression after '||'");
			left = create_node(state->arena, NODE_ERROR, "");
		}
	}
	append_child(curr, left);
	return curr;
}

// logical_and_expression = equality_expression { "&&" equality_expression } ;
Node *parse_logical_and_expression(ParseState *state) {
	Node *left = parse_equality_expression(state);
	if (!left || peek(state) != TKN_AMPAMP)
		return left;
	Node *curr = create_node(state->arena, NODE_LOGICAL_AND_EXPRESSION, "");
	while (peek(state) == TKN_AMPAMP) {
		append_child(curr, left);
		advance(state);
		left = parse_equality_expression(state);
		if (!left) {
			report_error(state, "expected an expression after '&&'");
			left = create_node(state->arena, NODE_ERROR, "");
		}
	}
	append_child(curr, left);
	return curr;
}

// equality_expression = relational_expression { ("==" | "!=") relational_expression } ;
Node *parse_equality_expression(ParseState *state) {
	Node *left = parse_relational_expression(state);
	if (!left)
		return NULL;
	while (peek(state) == TKN_EQEQ || peek(state) == TKN_BANGEQ) {
		Node *curr = create_node(state->arena, NODE_EQUALITY_EXPRESSION, "");
		curr->data.op = (peek(state) == TKN_EQEQ) ? eq_equals : not_equals;
		advance(state);
		Node *right = parse_relational_expression(state);
		if (!right)
			report_error(state, "expected an expression");
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}

// relational_expression = additive_expression { ("<" | "<=" | ">" | ">=")
// additive_expression } ;
Node *parse_relational_expression(ParseState *state) {
	Node *left = parse_additive_expression(state);
	if (!left)
		return NULL;
	while (peek(state) == TKN_LT || peek(state) == TKN_LTEQ || peek(state) == TKN_GT ||
		   peek(state) == TKN_GTEQ) {
		Node *curr = create_node(state->arena, NODE_RELATIONAL_EXPRESSION, "");
		switch (peek(state)) {
		case TKN_LTEQ:
			curr->data.op = less_than_eq;
			break;
		case TKN_GTEQ:
			curr->data.op = greater_than_eq;
			break;
		case TKN_LT:
			curr->data.op = less_than;
			break;
		default:
			curr->data.op = greater_than;
			break;
		}
		advance(state);
		Node *right = parse_additive_expression(state);
		if (!right)
			report_error(state, "expected an expression");
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}

// additive_expression = multiplicative_expression { ("+" | "-") multiplicative_expression
// } ;
Node *parse_additive_expression(ParseState *state) {
	Node *left = parse_multiplicative_expression(state);
	if (!left)
		return NULL;
	while (peek(state) == TKN_PLUS || peek(state) == TKN_MINUS) {
		Node *curr = create_node(state->arena, NODE_ADDITIVE_EXPRESSION, "");
		curr->data.op = (peek(state) == TKN_PLUS) ? plus : minus;
		advance(state);
		Node *right = parse_multiplicative_expression(state);
		if (!right)
			report_error(state, "expected an expression");
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}

// multiplicative_expression = cast_expression { ("*" | "/" | "%") cast_expression } ;
Node *parse_multiplicative_expression(ParseState *state) {
	Node *left = parse_cast_expression(state);
	if (!left)
		return NULL;
	while (peek(state) == TKN_STAR || peek(state) == TKN_SLASH ||
		   peek(state) == TKN_PERCENT) {
		Node *curr = create_node(state->arena, NODE_MULTIPLICTIVE_EXPRESSION, "");
		switch (peek(state)) {
		case TKN_STAR:
			curr->data.op = star;
			break;
		case TKN_SLASH:
			curr->data.op = slash;
			break;
		default:
			curr->data.op = percent;
			break;
		}
		advance(state);
		Node *right = parse_cast_expression(state);
		if (!right)
			report_error(state, "expected an expression");
		append_child(curr, left);
		append_child(curr, right);
		left = curr;
	}
	return left;
}

// cast_expression = unary_expression { "as" type } ;
// Replaces the old "(" type ")" expression cast, which required trying a
// type parse and backtracking to a grouped-expression parse on failure —
// the one genuine ambiguity in the grammar. Infix "as" removes it entirely.
Node *parse_cast_expression(ParseState *state) {
	Node *expr = parse_unary_expression(state);
	if (!expr)
		return NULL;
	while (peek(state) == TKN_KW_AS) {
		advance(state); // consume 'as'
		Node *type = parse_type(state);
		if (!type)
			report_error(state, "expected a type after 'as'");
		Node *curr = create_node(state->arena, NODE_CAST_EXPRESSION, "");
		append_child(curr, type);
		append_child(curr, expr);
		expr = curr; // left-associative: (x as i32) as i64 chains correctly
	}
	return expr;
}

// unary_expression = ("-" | "!" | "~" | "*" | "&") unary_expression | postfix_expression ;
Node *parse_unary_expression(ParseState *state) {
	Op op;
	switch (peek(state)) {
	case TKN_MINUS:
		op = minus;
		break;
	case TKN_BANG:
		op = log_not;
		break;
	case TKN_TILDA:
		op = bit_not;
		break;
	case TKN_STAR:
		op = star;
		break;
	case TKN_AMP:
		op = and_perc;
		break;
	default:
		return parse_postfix_expression(state);
	}
	Node *curr = create_node(state->arena, NODE_UNARY_EXPRESSION, "");
	curr->data.op = op;
	advance(state);
	Node *operand = parse_unary_expression(state);
	if (!operand)
		report_error(state, "expected an expression after unary operator");
	append_child(curr, operand);
	return curr;
}

// postfix_expression = primary_expression
//                    { "(" argument_list ")" | "." identifier
//                    | "[" expression "]" | "[" expression ":" expression? "]"
//                    | "++" | "--" } ;
Node *parse_postfix_expression(ParseState *state) {
	Node *left = parse_primary_expression(state);
	if (!left)
		return NULL;
	for (;;) {
		if (peek(state) == TKN_LPAREN) {
			advance(state);
			Node *args = parse_argument_list(state);
			if (!expect(state, TKN_RPAREN, "')' after arguments")) {
				const TokenKind sync[] = {TKN_RPAREN};
				synchronize(state, sync, 1);
				expect(state, TKN_RPAREN, "')'");
			}
			Node *curr = create_node(state->arena, NODE_FUNC_CALL, "");
			append_child(curr, left);
			append_child(curr, args);
			left = curr;
			continue;
		}
		if (peek(state) == TKN_DOT) {
			advance(state);
			Node *identifier = parse_identifier(state);
			if (!identifier)
				report_error(state, "expected a field or method name after '.'");
			Node *curr = create_node(state->arena, NODE_ACCESS, "");
			append_child(curr, left);
			append_child(curr, identifier);
			left = curr;
			continue;
		}
		if (peek(state) == TKN_LBRACKET) {
			advance(state); // consume '['

			Node *start = NULL;
			bool is_slice = false;

			// Parse start if present
			if (peek(state) != TKN_COLON) {
				start = parse_expression(state);
				if (!start) {
					report_error(state, "expected an index expression");
					start = create_node(state->arena, NODE_ERROR, "");
				}
			} else {
				// start omitted → create a marker
				start = create_node(state->arena, NODE_SLICE_START, "");
			}

			// Check for slice colon
			if (peek(state) == TKN_COLON) {
				is_slice = true;
				advance(state); // consume ':'
			}

			Node *end = NULL;
			if (is_slice) {
				if (peek(state) != TKN_RBRACKET) {
					end = parse_expression(state);
					if (!end) {
						report_error(state, "expected an expression after ':'");
						end = create_node(state->arena, NODE_ERROR, "");
					}
				} else {
					// end omitted → create a marker
					end = create_node(state->arena, NODE_SLICE_END, "");
				}
			}

			if (!expect(state, TKN_RBRACKET, "']' to close index/slice")) {
				const TokenKind sync[] = {TKN_RBRACKET};
				synchronize(state, sync, 1);
				expect(state, TKN_RBRACKET, "']'");
			}

			Node *curr;
			if (is_slice) {
				curr = create_node(state->arena, NODE_SLICE_EXPR, "");
				append_child(curr, left);
				append_child(curr, start); // always present (maybe marker)
				append_child(curr, end);   // always present (maybe marker)
			} else {
				// Simple index: we have a start expression, no colon
				curr = create_node(state->arena, NODE_INDEX, "");
				append_child(curr, left);
				append_child(curr, start);
			}
			left = curr;
			continue;
		}
		// if (peek(state) == TKN_LBRACKET) {
		// 	advance(state);
		// 	Node *curr = create_node(state->arena, NODE_INDEX, "");
		// 	append_child(curr, left);
		// 	Node *expr_a = parse_expression(state);
		// 	if (!expr_a)
		// 		report_error(state, "expected an index expression");
		// 	append_child(curr, expr_a);
		// 	if (peek(state) == TKN_COLON) {
		// 		advance(state);
		// 		Node *expr_b = parse_expression(state);
		// 		append_child(curr, expr_b); // absent upper bound is valid: arr[i:]
		// 	}
		// 	if (!expect(state, TKN_RBRACKET, "']' to close index/slice")) {
		// 		const TokenKind sync[] = {TKN_RBRACKET};
		// 		synchronize(state, sync, 1);
		// 		expect(state, TKN_RBRACKET, "']'");
		// 	}
		// 	left = curr;
		// 	continue;
		// }
		if (peek(state) == TKN_PLUSPLUS || peek(state) == TKN_MINUSMINUS) {
			Node *curr = create_node(state->arena, NODE_INC_DEC, "");
			curr->data.op = (peek(state) == TKN_PLUSPLUS) ? plus_plus : minus_minus;
			advance(state);
			append_child(curr, left);
			left = curr;
			continue;
		}
		break;
	}
	return left;
}

// primary_expression = literal | identifier | "(" expression ")" | "sizeof" "(" type ")" ;
Node *parse_primary_expression(ParseState *state) {
	if (peek(state) == TKN_KW_SIZEOF) {
		advance(state);
		Node *curr = create_node(state->arena, NODE_SIZE_OF_EXPRESSION, "");
		if (!expect(state, TKN_LPAREN, "'(' after 'sizeof'"))
			return curr;
		Node *type = parse_type(state);
		if (!type)
			report_error(state, "expected a type");
		append_child(curr, type);
		if (!expect(state, TKN_RPAREN, "')' after sizeof type")) {
			const TokenKind sync[] = {TKN_RPAREN};
			synchronize(state, sync, 1);
			expect(state, TKN_RPAREN, "')'");
		}
		return curr;
	}
	if (peek(state) == TKN_LPAREN) {
		advance(state);
		Node *expr = parse_expression(state);
		if (!expr)
			report_error(state, "expected an expression");
		if (!expect(state, TKN_RPAREN, "')' to close expression")) {
			const TokenKind sync[] = {TKN_RPAREN};
			synchronize(state, sync, 1);
			expect(state, TKN_RPAREN, "')'");
		}
		Node *curr = create_node(state->arena, NODE_GROUPED_EXPRESSION, "");
		append_child(curr, expr);
		return curr;
	}
	if (peek(state) == TKN_KW_SELF) {
		Token *tok = advance(state);
		Node *curr = create_node(state->arena, NODE_IDENTIFER, "");
		curr->line = tok->line;
		curr->col = tok->col;
		curr->data.literal.start = tok->offset;
		curr->data.literal.end = tok->offset + tok->length;
		curr->data.literal.source = (char *) state->ts->lex.src;
		return curr;
	}
	if (peek(state) == TKN_IDENT)
		return parse_identifier(state);
	return parse_literal(state);
}

// argument_list = [ expression { "," expression } ] ;
Node *parse_argument_list(ParseState *state) {
	Node *curr = create_node(state->arena, NODE_ARGUMENT_LIST, "");
	if (peek(state) == TKN_RPAREN)
		return curr; // empty list
	Node *expr = parse_expression(state);
	if (expr)
		append_child(curr, expr);
	while (peek(state) == TKN_COMMA) {
		advance(state); // consume the comma
		if (peek(state) == TKN_RPAREN) {
			report_error(state, "expected an expression after ','");
			break;
		}
		expr = parse_expression(state);
		if (expr)
			append_child(curr, expr);
	}
	return curr;
}

// ============================================================
// LITERALS
// ============================================================

// literal = integer_literal | float_literal | string_literal
//         | char_literal | boolean_literal | array_literal ;
Node *parse_literal(ParseState *state) {
	switch (peek(state)) {
	case TKN_INT_LITERAL:
		return parse_integer_literal(state);
	case TKN_FLOAT_LITERAL:
		return parse_float_literal(state);
	case TKN_STRING_LITERAL:
		return parse_string_literal(state);
	case TKN_CHAR_LITERAL:
		return parse_char_literal(state);
	case TKN_KW_TRUE:
	case TKN_KW_FALSE:
		return parse_boolean_literal(state);
	case TKN_LBRACKET:
		return parse_array_literal(state);
	default:
		return NULL;
	}
}

Node *parse_integer_literal(ParseState *state) {
	Token *tok = advance(state);
	Node *curr = create_node(state->arena, NODE_DECIMAL_LITERAL, "");
	curr->line = tok->line;
	curr->col = tok->col;
	curr->data.literal.start = tok->offset;
	curr->data.literal.end = tok->offset + tok->length;
	curr->data.literal.source = (char *) state->ts->lex.src;
	return curr;
}

Node *parse_float_literal(ParseState *state) {
	Token *tok = advance(state);
	Node *curr = create_node(state->arena, NODE_FLOAT_LITERAL, "");
	curr->line = tok->line;
	curr->col = tok->col;
	curr->data.literal.start = tok->offset;
	curr->data.literal.end = tok->offset + tok->length;
	curr->data.literal.source = (char *) state->ts->lex.src;
	return curr;
}

Node *parse_string_literal(ParseState *state) {
	Token *tok = advance(state);
	Node *curr = create_node(state->arena, NODE_STRING_LITERAL, "");
	curr->line = tok->line;
	curr->col = tok->col;
	curr->data.literal.start = tok->offset;
	curr->data.literal.end = tok->offset + tok->length;
	curr->data.literal.source = (char *) state->ts->lex.src;
	return curr;
}

Node *parse_char_literal(ParseState *state) {
	Token *tok = advance(state);
	Node *curr = create_node(state->arena, NODE_CHAR_LITERAL, "");
	curr->line = tok->line;
	curr->col = tok->col;
	curr->data.literal.start = tok->offset;
	curr->data.literal.end = tok->offset + tok->length;
	curr->data.literal.source = (char *) state->ts->lex.src;
	return curr;
}

Node *parse_boolean_literal(ParseState *state) {
	Token *tok = advance(state);
	Node *curr = create_node(state->arena, NODE_BOOLEAN_LITERAL, "");
	curr->line = tok->line;
	curr->col = tok->col;
	curr->data.literal.start = tok->offset;
	curr->data.literal.end = tok->offset + tok->length;
	curr->data.literal.source = (char *) state->ts->lex.src;
	return curr;
}

// array_literal = "[" [ expression { "," expression } ] "]" ;
Node *parse_array_literal(ParseState *state) {
	advance(state); // consume '['
	Node *curr = create_node(state->arena, NODE_ARRAY_LITERAL, "");
	if (peek(state) == TKN_RBRACKET) {
		advance(state);
		return curr;
	}
	Node *expr = parse_expression(state);
	if (expr)
		append_child(curr, expr);
	while (peek(state) == TKN_COMMA) {
		advance(state);
		if (peek(state) == TKN_RBRACKET) {
			report_error(state, "expected an expression after ','");
			break;
		}
		expr = parse_expression(state);
		if (expr)
			append_child(curr, expr);
	}
	if (!expect(state, TKN_RBRACKET, "']' to close array literal")) {
		const TokenKind sync[] = {TKN_RBRACKET};
		synchronize(state, sync, 1);
		expect(state, TKN_RBRACKET, "']'");
	}
	return curr;
}

// identifier = letter { letter | digit | "_" } ;
Node *parse_identifier(ParseState *state) {
	if (peek(state) != TKN_IDENT)
		return NULL; // speculative — callers check this
	Token *tok = advance(state);
	Node *curr = create_node(state->arena, NODE_IDENTIFER, "");
	curr->line = tok->line;
	curr->col = tok->col;
	curr->data.literal.start = tok->offset;
	curr->data.literal.end = tok->offset + tok->length;
	curr->data.literal.source = (char *) state->ts->lex.src;
	return curr;
}
