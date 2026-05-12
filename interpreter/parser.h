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

static inline char peek(Tokens *tokens) {
	if (tokens->position == tokens->size)
		return '\0';
	return tokens->data[tokens->position];
}
static inline char consume(Tokens *tokens) {
	if (tokens->position == tokens->size)
		return '\0';
	return tokens->data[tokens->position++];
}
 
static inline bool consume_if(Tokens *tokens, char c) {
	if (peek(tokens) == c) {
		consume(tokens);
		return true;
	}
	return false;
}

static inline bool peek_string(Tokens *tokens, char* s ) {
	int len = strlen(s);
	for (int i = 0; i < len; i++) {
		if (s[i] != tokens->data[tokens->position + i ]) return false;
	}
	return true;
}
static inline void consume_string(Tokens *tokens, char* s ) {
	int len = strlen(s);
	tokens->position += len;
}
#endif
