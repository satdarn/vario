#ifndef TOKENS_H
#define TOKENS_H
#include "../shared/common.h"

typedef enum {
	TKN_EOF = 0,
	// literals
	TKN_IDENT = 256,
	TKN_INT_LITERAL,
	TKN_FLOAT_LITERAL,
	TKN_STRING_LITERAL,
	TKN_CHAR_LITERAL,
	// keywords get their OWN kinds, not TKN_KEYWORD + strcmp
	TKN_KW_FN,
	TKN_KW_OBJ,
	TKN_KW_ENUM,
	TKN_KW_UNION,
	TKN_KW_CONST,
	TKN_KW_VAR,
	TKN_KW_LET,
	TKN_KW_IF,
	TKN_KW_ELSE,
	TKN_KW_WHILE,
	TKN_KW_FOR,
	TKN_KW_RETURN,
	TKN_KW_BREAK,
	TKN_KW_CONTINUE,
	TKN_KW_MODULE,
	TKN_KW_IMPORT,
	TKN_KW_PUB,
	TKN_KW_SELF,
	TKN_KW_INIT,
	TKN_KW_DEINIT,
	TKN_KW_DEFER,
	TKN_KW_SWITCH,
	TKN_KW_CASE,
	TKN_KW_DEFAULT,
	TKN_KW_TRUE,
	TKN_KW_FALSE,
	TKN_KW_AS,
	// punctuation / operators as distinct kinds too
	TKN_LPAREN = '(',
	TKN_RPAREN = ')',
	TKN_LBRACE = '{',
	TKN_RBRACE = '}',
	TKN_LBRACKET = '[',
	TKN_RBRACKET = ']',
	TKN_ARROW = ('-' << 8) + '>',
	TKN_COLON = ':',
	TKN_SEMI = ';',
	TKN_COMMA = ',',
	TKN_DOT = '.',
	TKN_PLUS = '+',
	TKN_MINUS = '-',
	TKN_STAR = '*',
	TKN_SLASH = '/',
	TKN_PERCENT = '%',
	TKN_EQ = '=',
	TKN_EQEQ = ('=' << 8) + '=',
	TKN_BANGEQ = ('!' << 8) + '=',
	TKN_LT = '<',
	TKN_LTEQ = ('<'<< 8) + '=',
	TKN_GT = '>',
	TKN_GTEQ = ('>'<< 8) + '=',
	TKN_AMPAMP = ('&' << 8) + '&',
	TKN_PIPEPIPE = ('|'<<8) + '|',
	TKN_BANG = '!',
	TKN_PLUSPLUS = ('+'<< 8) + '+',
	TKN_ERROR, // lexer-level error token (unterminated string, bad char, etc.)
} TokenKind;

typedef struct {
	TokenKind kind;
	uint32_t line, col;
	uint32_t offset, length; // slice into source buffer (no copy needed for most kinds)
	union {
		int64_t int_val;
		double float_val;
		struct {
			const char *ptr;
			size_t len;
		} str_val; // arena-processed (escapes resolved)
	} value;
} Token;

typedef struct {
	const char *src;
	size_t src_len, pos;
	uint32_t line, col;
} Lexer;

typedef struct {
	Lexer lex;
	Token *tokens;	  // stb_ds dynamic array, arrsetcap(ts->tokens, 10) up front
	size_t cursor;	  // parser's current index into tokens
	Arena *str_arena; // for processed string/char literal payloads
	bool at_eof;
} TokenStream;


void lexize(TokenStream *ts);
Token *ts_peek(TokenStream *ts,
			   size_t lookahead); // lex more if cursor+lookahead >= arrlen
Token *ts_curr(TokenStream *ts);
Token *ts_advance(TokenStream *ts); // Advances and returns the previous
size_t ts_mark(TokenStream *ts);			 // just returns cursor
void ts_reset(TokenStream *ts, size_t mark); // just sets cursor — O(1), no re-lexing
void ts_dump(TokenStream *ts);
#endif
