#ifndef PARSER_H
#define PARSER_H
#include "../parse/nodes.h"
#include "../shared/util.h"

typedef struct {
	char *data;
	size_t size;
	size_t position;
	size_t line;
	size_t col;
} Tokens;

static inline void token_dump(Tokens *tokens) {
	printf("====================\n\n");
	printf("%s\n", tokens->data + tokens->position);
	printf("====================\n\n");
}
static inline void advance_one(Tokens *tokens) {
	if (tokens->position >= tokens->size)
		return;

	char c = tokens->data[tokens->position];
	tokens->position++;

	if (c == '\n') {
		tokens->line++;
		tokens->col = 1;
	} else {
		tokens->col++;
	}
}

static inline void skip_whitespace(Tokens *tokens) {
	while (tokens->position < tokens->size) {
		char c = tokens->data[tokens->position];
		if (isspace(c)) {
			advance_one(tokens);
			continue;
		}
		if (c == '/' && tokens->position + 1 < tokens->size &&
			tokens->data[tokens->position + 1] == '/') {
			tokens->position += 2;
			while (tokens->position < tokens->size &&
				   tokens->data[tokens->position] != '\n')
				advance_one(tokens);
			continue;
		}
		if (c == '/' && tokens->position + 1 < tokens->size &&
			tokens->data[tokens->position + 1] == '*') {
			tokens->position += 2;
			while (tokens->position + 1 < tokens->size) {
				if (tokens->data[tokens->position] == '*' &&
					tokens->data[tokens->position + 1] == '/') {
					advance_one(tokens);
					advance_one(tokens);
					break;
				}
				tokens->position++;
			}
			continue;
		}
		break;
	}
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
		consume_raw(tokens);
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
	for (int i = 0; i < len; i++) {
		advance_one(tokens);
	}
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
	char after = (tokens->position + len < tokens->size)
					 ? tokens->data[tokens->position + len]
					 : '\0';
	return !isalnum(after) && after != '_';
}

typedef struct {
	size_t position;
	size_t line;
	size_t col;
	ArenaCheckpoint arena_cp;
} ParserCheckpoint;

static inline ParserCheckpoint parser_save(Arena *arena, Tokens *tokens) {
	ParserCheckpoint cp = {
		.position = tokens->position,
		.line = tokens->line,
		.col = tokens->col,
		.arena_cp = arena_save(arena),
	};
	return cp;
}

static inline void parser_restore(Arena *arena, Tokens *tokens, ParserCheckpoint cp) {
	tokens->position = cp.position;
	tokens->line = cp.line;
	tokens->col = cp.col;
	arena_restore(arena, cp.arena_cp);
}

static inline Node *create_node_cp(Arena *arena, NodeType node_type, char *node_name,
								   ParserCheckpoint cp) {
	Node *node = create_node(arena, node_type, node_name);
	if (node) {
		node->line = cp.line;
		node->col = cp.col;
	}
	return node;
}

static inline Node *create_node_with_line_col(Arena *arena, NodeType node_type, char *node_name,
											  size_t line, size_t col) {
	Node *node = create_node(arena, node_type, node_name);
	if (node) {
		node->line = line;
		node->col = col;
	}
	return node;
}

Node *parse(Arena *arena, char *data);
Node *parse_program(Arena *arena, Tokens *tokens);
Node *parse_top_level_decl(Arena *arena, Tokens *tokens);
Node *parse_module_decl(Arena *arena, Tokens *tokens);
Node *parse_import_decl(Arena *arena, Tokens *tokens);
Node *parse_visibility(Arena *arena, Tokens *tokens);
Node *parse_func_decl(Arena *arena, Tokens *tokens);
Node *parse_parameter_list(Arena *arena, Tokens *tokens);
Node *parse_parameter(Arena *arena, Tokens *tokens);
Node *parse_return_type(Arena *arena, Tokens *tokens);
Node *parse_block(Arena *arena, Tokens *tokens);
Node *parse_type(Arena *arena, Tokens *tokens);
Node *parse_primitive_type(Arena *arena, Tokens *tokens);
Node *parse_pointer_type(Arena *arena, Tokens *tokens);
Node *parse_slice_type(Arena *arena, Tokens *tokens);
Node *parse_obj_type(Arena *arena, Tokens *tokens);
Node *parse_enum_type(Arena *arena, Tokens *tokens);
Node *parse_union_type(Arena *arena, Tokens *tokens);
Node *parse_obj_decl(Arena *arena, Tokens *tokens);
Node *parse_obj_member(Arena *arena, Tokens *tokens);
Node *parse_field_decl(Arena *arena, Tokens *tokens);
Node *parse_method_decl(Arena *arena, Tokens *tokens);
Node *parse_self_param(Arena *arena, Tokens *tokens);
Node *parse_constructor_decl(Arena *arena, Tokens *tokens);
Node *parse_destructor_decl(Arena *arena, Tokens *tokens);
Node *parse_enum_decl(Arena *arena, Tokens *tokens);
Node *parse_enum_variant(Arena *arena, Tokens *tokens);
Node *parse_union_decl(Arena *arena, Tokens *tokens);
Node *parse_union_variant(Arena *arena, Tokens *tokens);
Node *parse_const_decl(Arena *arena, Tokens *tokens);
Node *parse_var_decl(Arena *arena, Tokens *tokens);
Node *parse_let_decl(Arena *arena, Tokens *tokens);
Node *parse_statement(Arena *arena, Tokens *tokens);
Node *parse_declaration_stmt(Arena *arena, Tokens *tokens);
Node *parse_assignment(Arena *arena, Tokens *tokens);
Node *parse_assignment_stmt(Arena *arena, Tokens *tokens);
Node *parse_assignment_operator(Arena *arena, Tokens *tokens);
Node *parse_expression_stmt(Arena *arena, Tokens *tokens);
Node *parse_return_stmt(Arena *arena, Tokens *tokens);
Node *parse_break_stmt(Arena *arena, Tokens *tokens);
Node *parse_continue_stmt(Arena *arena, Tokens *tokens);
Node *parse_defer_stmt(Arena *arena, Tokens *tokens);
Node *parse_block_stmt(Arena *arena, Tokens *tokens);
Node *parse_conditional_stmt(Arena *arena, Tokens *tokens);
Node *parse_loop_stmt(Arena *arena, Tokens *tokens);
Node *parse_while_loop(Arena *arena, Tokens *tokens);
Node *parse_for_loop(Arena *arena, Tokens *tokens);
Node *parse_for_init(Arena *arena, Tokens *tokens);
Node *parse_for_update(Arena *arena, Tokens *tokens);
Node *parse_range_for_loop(Arena *arena, Tokens *tokens);
Node *parse_switch_stmt(Arena *arena, Tokens *tokens);
Node *parse_switch_case(Arena *arena, Tokens *tokens);
Node *parse_case_pattern(Arena *arena, Tokens *tokens);
Node *parse_switch_default(Arena *arena, Tokens *tokens);
Node *parse_expression(Arena *arena, Tokens *tokens);
Node *parse_logical_or_expression(Arena *arena, Tokens *tokens);
Node *parse_logical_and_expression(Arena *arena, Tokens *tokens);
Node *parse_equality_expression(Arena *arena, Tokens *tokens);
Node *parse_relational_expression(Arena *arena, Tokens *tokens);
Node *parse_additive_expression(Arena *arena, Tokens *tokens);
Node *parse_multiplicative_expression(Arena *arena, Tokens *tokens);
Node *parse_unary_expression(Arena *arena, Tokens *tokens);
Node *parse_postfix_expression(Arena *arena, Tokens *tokens);
Node *parse_primary_expression(Arena *arena, Tokens *tokens);
Node *parse_argument_list(Arena *arena, Tokens *tokens);
Node *parse_literal(Arena *arena, Tokens *tokens);
Node *parse_integer_literal(Arena *arena, Tokens *tokens);
Node *parse_float_literal(Arena *arena, Tokens *tokens);
Node *parse_string_literal(Arena *arena, Tokens *tokens);
Node *parse_char_literal(Arena *arena, Tokens *tokens);
Node *parse_boolean_literal(Arena *arena, Tokens *tokens);
Node *parse_array_literal(Arena *arena, Tokens *tokens);
Node *parse_identifier(Arena *arena, Tokens *tokens);
Node *parse_decimal_literal(Arena *arena, Tokens *tokens);
Node *parse_hex_literal(Arena *arena, Tokens *tokens);
Node *parse_octal_literal(Arena *arena, Tokens *tokens);
Node *parse_binary_literal(Arena *arena, Tokens *tokens);
Node *parse_decimal_digits(Arena *arena, Tokens *tokens);
Node *parse_hex_digits(Arena *arena, Tokens *tokens);
Node *parse_octal_digits(Arena *arena, Tokens *tokens);
Node *parse_binary_digits(Arena *arena, Tokens *tokens);
Node *parse_letter(Arena *arena, Tokens *tokens);
Node *parse_digit(Arena *arena, Tokens *tokens);
Node *parse_hex_digit(Arena *arena, Tokens *tokens);
Node *parse_octal_digit(Arena *arena, Tokens *tokens);
Node *parse_binary_digit(Arena *arena, Tokens *tokens);
Node *parse_string_char(Arena *arena, Tokens *tokens);
Node *parse_comment(Arena *arena, Tokens *tokens);
Node *parse_whitespace(Arena *arena, Tokens *tokens);
#endif
