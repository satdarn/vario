#ifndef PARSER_H
#define PARSER_H
#include "../parse/nodes.h"
#include "../shared/util.h"
#include "../token/tokens.h"

typedef struct {
	char *msg;
	size_t line;
	size_t col;
} ParseError;

typedef struct {
	TokenStream *ts;
	Arena *arena;
	ArenaCheckpoint arena_cp;
	size_t ts_cp;
	ParseError *errors;
} ParseState;

typedef struct {
	size_t position;
	ParseState *state;
	ArenaCheckpoint arena_cp;
} ParserCheckpoint;

static inline ParserCheckpoint parser_save(ParseState *state) {
	ParserCheckpoint cp = {
		.position = ts_mark(state->ts),
		.state = state,
		.arena_cp = arena_save(state->arena),
	};
	return cp;
}

static inline void parser_restore(ParserCheckpoint cp) {
	ts_reset(cp.state->ts, cp.position);
	arena_restore(cp.state->arena, cp.arena_cp);
}

static inline Node *create_node_parse(ParseState *state, NodeType node_type,
									  char *node_name) {
	Node *node = create_node(state->arena, node_type, node_name);
	if (node) {
		node->line = ts_peek(state->ts)->line;
		node->col = ts_peek(state->ts)->col;
	}
	return node;
}
static inline Node *create_node_cp(Arena *arena, NodeType node_type, char *node_name,
								   ParserCheckpoint cp) {
	TokenStream *ts = cp.state->ts;
	Node *node = create_node(arena, node_type, node_name);
	if (node) {
		node->line = ts->tokens[cp.position].line;
		node->col = ts->tokens[cp.position].col;
	}
	return node;
}

Node *parse(Arena *arena, TokenStream *state);
Node *parse_program(ParseState *state);
Node *parse_top_level_decl(ParseState *state);
Node *parse_module_decl(ParseState *state);
Node *parse_import_decl(ParseState *state);
Node *parse_visibility(ParseState *state);
Node *parse_func_decl(ParseState *state);
Node *parse_parameter_list(ParseState *state);
Node *parse_parameter(ParseState *state);
Node *parse_return_type(ParseState *state);
Node *parse_block(ParseState *state);
Node *parse_type(ParseState *state);
Node *parse_primitive_type(ParseState *state);
Node *parse_pointer_type(ParseState *state);
Node *parse_slice_type(ParseState *state);
Node *parse_obj_type(ParseState *state);
Node *parse_enum_type(ParseState *state);
Node *parse_union_type(ParseState *state);
Node *parse_obj_decl(ParseState *state);
Node *parse_obj_member(ParseState *state);
Node *parse_field_decl(ParseState *state);
Node *parse_method_decl(ParseState *state);
Node *parse_self_param(ParseState *state);
Node *parse_constructor_decl(ParseState *state);
Node *parse_destructor_decl(ParseState *state);
Node *parse_enum_decl(ParseState *state);
Node *parse_enum_variant(ParseState *state);
Node *parse_union_decl(ParseState *state);
Node *parse_union_variant(ParseState *state);
Node *parse_const_decl(ParseState *state);
Node *parse_var_decl(ParseState *state);
Node *parse_let_decl(ParseState *state);
Node *parse_statement(ParseState *state);
Node *parse_declaration_stmt(ParseState *state);
Node *parse_assignment(ParseState *state);
Node *parse_assignment_stmt(ParseState *state);
Node *parse_assignment_operator(ParseState *state);
Node *parse_expression_stmt(ParseState *state);
Node *parse_return_stmt(ParseState *state);
Node *parse_break_stmt(ParseState *state);
Node *parse_continue_stmt(ParseState *state);
Node *parse_defer_stmt(ParseState *state);
Node *parse_block_stmt(ParseState *state);
Node *parse_conditional_stmt(ParseState *state);
Node *parse_loop_stmt(ParseState *state);
Node *parse_while_loop(ParseState *state);
Node *parse_for_loop(ParseState *state);
Node *parse_for_init(ParseState *state);
Node *parse_for_update(ParseState *state);
Node *parse_range_for_loop(ParseState *state);
Node *parse_switch_stmt(ParseState *state);
Node *parse_switch_case(ParseState *state);
Node *parse_case_pattern(ParseState *state);
Node *parse_switch_default(ParseState *state);
Node *parse_expression(ParseState *state);
Node *parse_logical_or_expression(ParseState *state);
Node *parse_logical_and_expression(ParseState *state);
Node *parse_equality_expression(ParseState *state);
Node *parse_relational_expression(ParseState *state);
Node *parse_additive_expression(ParseState *state);
Node *parse_multiplicative_expression(ParseState *state);
Node *parse_unary_expression(ParseState *state);
Node *parse_postfix_expression(ParseState *state);
Node *parse_primary_expression(ParseState *state);
Node *parse_argument_list(ParseState *state);
Node *parse_literal(ParseState *state);
Node *parse_integer_literal(ParseState *state);
Node *parse_float_literal(ParseState *state);
Node *parse_string_literal(ParseState *state);
Node *parse_char_literal(ParseState *state);
Node *parse_boolean_literal(ParseState *state);
Node *parse_array_literal(ParseState *state);
Node *parse_identifier(ParseState *state);
Node *parse_decimal_literal(ParseState *state);
Node *parse_hex_literal(ParseState *state);
Node *parse_octal_literal(ParseState *state);
Node *parse_binary_literal(ParseState *state);
Node *parse_decimal_digits(ParseState *state);
Node *parse_hex_digits(ParseState *state);
Node *parse_octal_digits(ParseState *state);
Node *parse_binary_digits(ParseState *state);
Node *parse_letter(ParseState *state);
Node *parse_digit(ParseState *state);
Node *parse_hex_digit(ParseState *state);
Node *parse_octal_digit(ParseState *state);
Node *parse_binary_digit(ParseState *state);
Node *parse_string_char(ParseState *state);
Node *parse_comment(ParseState *state);
Node *parse_whitespace(ParseState *state);
#endif
