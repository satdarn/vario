#ifndef PARSER_H
#define PARSER_H
#include "nodes.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
	char *data;
	size_t size;
	size_t position;
} Tokens;
static inline void skip_whitespace(Tokens *tokens) {
	while (tokens->position < tokens->size && isspace(tokens->data[tokens->position]))
		tokens->position++;
}

static inline char peek_raw(Tokens *tokens) {
	if (tokens->position == tokens->size)
		return '\0';
	return tokens->data[tokens->position];
}
static inline char peek(Tokens *tokens) {
	skip_whitespace(tokens);
	return peek_raw(tokens);
}

static inline char consume_raw(Tokens *tokens) {
	if (tokens->position == tokens->size)
		return '\0';
	return tokens->data[tokens->position++];
}

static inline char consume(Tokens *tokens) {
	skip_whitespace(tokens);
	return consume_raw(tokens);
}
static inline bool consume_if(Tokens *tokens, char c) {
	skip_whitespace(tokens);
	if (peek(tokens) == c) {
		consume(tokens);
		return true;
	}
	return false;
}

static inline bool peek_string(Tokens *tokens, char *s) {
	skip_whitespace(tokens);
	int len = strlen(s);
	for (int i = 0; i < len; i++) {
		if (s[i] != tokens->data[tokens->position + i])
			return false;
	}
	return true;
}
static inline void consume_string(Tokens *tokens, char *s) {
	skip_whitespace(tokens);
	int len = strlen(s);
	tokens->position += len;
}
static inline bool peek_keyword(Tokens *tokens, const char *s) {
	skip_whitespace(tokens);
	int len = strlen(s);
	for (int i = 0; i < len; i++) {
		if (tokens->position + i >= tokens->size)
			return false;
		if (tokens->data[tokens->position + i] != s[i])
			return false;
	}
	char after = (tokens->position + len < tokens->size) ? tokens->data[tokens->position + len] : '\0';
	return !isalnum(after) && after != '_';
}

Node *parse_program(Tokens *tokens);
Node *parse_top_level_decl(Tokens *tokens);
Node *parse_module_decl(Tokens *tokens);
Node *parse_import_decl(Tokens *tokens);
Node *parse_visibility(Tokens *tokens);
Node *parse_func_decl(Tokens *tokens);
Node *parse_parameter_list(Tokens *tokens);
Node *parse_parameter(Tokens *tokens);
Node *parse_return_type(Tokens *tokens);
Node *parse_block(Tokens *tokens);
Node *parse_type(Tokens *tokens);
Node *parse_primitive_type(Tokens *tokens);
Node *parse_pointer_type(Tokens *tokens);
Node *parse_slice_type(Tokens *tokens);
Node *parse_obj_type(Tokens *tokens);
Node *parse_enum_type(Tokens *tokens);
Node *parse_union_type(Tokens *tokens);
Node *parse_obj_decl(Tokens *tokens);
Node *parse_obj_member(Tokens *tokens);
Node *parse_field_decl(Tokens *tokens);
Node *parse_method_decl(Tokens *tokens);
Node *parse_self_param(Tokens *tokens);
Node *parse_constructor_decl(Tokens *tokens);
Node *parse_destructor_decl(Tokens *tokens);
Node *parse_enum_decl(Tokens *tokens);
Node *parse_enum_variant(Tokens *tokens);
Node *parse_union_decl(Tokens *tokens);
Node *parse_union_variant(Tokens *tokens);
Node *parse_const_decl(Tokens *tokens);
Node *parse_var_decl(Tokens *tokens);
Node *parse_let_decl(Tokens *tokens);
Node *parse_statement(Tokens *tokens);
Node *parse_declaration_stmt(Tokens *tokens);
Node *parse_assignment_stmt(Tokens *tokens);
Node *parse_assignment_operator(Tokens *tokens);
Node *parse_expression_stmt(Tokens *tokens);
Node *parse_return_stmt(Tokens *tokens);
Node *parse_break_stmt(Tokens *tokens);
Node *parse_continue_stmt(Tokens *tokens);
Node *parse_defer_stmt(Tokens *tokens);
Node *parse_block_stmt(Tokens *tokens);
Node *parse_conditional_stmt(Tokens *tokens);
Node *parse_loop_stmt(Tokens *tokens);
Node *parse_while_loop(Tokens *tokens);
Node *parse_for_loop(Tokens *tokens);
Node *parse_for_init(Tokens *tokens);
Node *parse_for_update(Tokens *tokens);
Node *parse_range_for_loop(Tokens *tokens);
Node *parse_switch_stmt(Tokens *tokens);
Node *parse_switch_case(Tokens *tokens);
Node *parse_case_pattern(Tokens *tokens);
Node *parse_switch_default(Tokens *tokens);
Node *parse_expression(Tokens *tokens);
Node *parse_logical_or_expression(Tokens *tokens);
Node *parse_logical_and_expression(Tokens *tokens);
Node *parse_equality_expression(Tokens *tokens);
Node *parse_relational_expression(Tokens *tokens);
Node *parse_additive_expression(Tokens *tokens);
Node *parse_multiplicative_expression(Tokens *tokens);
Node *parse_unary_expression(Tokens *tokens);
Node *parse_postfix_expression(Tokens *tokens);
Node *parse_primary_expression(Tokens *tokens);
Node *parse_argument_list(Tokens *tokens);
Node *parse_literal(Tokens *tokens);
Node *parse_integer_literal(Tokens *tokens);
Node *parse_float_literal(Tokens *tokens);
Node *parse_string_literal(Tokens *tokens);
Node *parse_boolean_literal(Tokens *tokens);
Node *parse_array_literal(Tokens *tokens);
Node *parse_identifier(Tokens *tokens);
Node *parse_decimal_literal(Tokens *tokens);
Node *parse_hex_literal(Tokens *tokens);
Node *parse_octal_literal(Tokens *tokens);
Node *parse_binary_literal(Tokens *tokens);
Node *parse_decimal_digits(Tokens *tokens);
Node *parse_hex_digits(Tokens *tokens);
Node *parse_octal_digits(Tokens *tokens);
Node *parse_binary_digits(Tokens *tokens);
Node *parse_letter(Tokens *tokens);
Node *parse_digit(Tokens *tokens);
Node *parse_hex_digit(Tokens *tokens);
Node *parse_octal_digit(Tokens *tokens);
Node *parse_binary_digit(Tokens *tokens);
Node *parse_string_char(Tokens *tokens);
Node *parse_comment(Tokens *tokens);
Node *parse_whitespace(Tokens *tokens);
#endif
