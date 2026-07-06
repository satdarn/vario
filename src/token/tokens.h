#ifndef TOKENS_H
#define TOKENS_H
#include "../shared/common.h"

typedef enum {
	TKN_KEYWORD = 256,
	TKN_IDENT,
	TKN_INT_LITERAL,
	TKN_FLOAT_LITERAL,
	TKN_STRING_LITERAL,
	TKN_CHAR_LITERAL,
} TokenKind;

typedef struct {
    TokenKind kind;
} Token;

typedef struct {
    char *data;
    size_t size;
    size_t position;
    size_t line;
    size_t col;
    Token current;
} Lexer;

#endif
