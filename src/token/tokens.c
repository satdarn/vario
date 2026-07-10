#include "../token/tokens.h"

// Helper to match keywords
static TokenKind keyword_kind(const char *start, size_t len) {
#define MATCH(s, kind)                                   \
	if (len == strlen(s) && strncmp(start, s, len) == 0) \
	return kind

	MATCH("fn", TKN_KW_FN);
	MATCH("obj", TKN_KW_OBJ);
	MATCH("enum", TKN_KW_ENUM);
	MATCH("union", TKN_KW_UNION);
	MATCH("const", TKN_KW_CONST);
	MATCH("var", TKN_KW_VAR);
	MATCH("let", TKN_KW_LET);
	MATCH("if", TKN_KW_IF);
	MATCH("else", TKN_KW_ELSE);
	MATCH("while", TKN_KW_WHILE);
	MATCH("for", TKN_KW_FOR);
	MATCH("return", TKN_KW_RETURN);
	MATCH("break", TKN_KW_BREAK);
	MATCH("continue", TKN_KW_CONTINUE);
	MATCH("module", TKN_KW_MODULE);
	MATCH("import", TKN_KW_IMPORT);
	MATCH("pub", TKN_KW_PUB);
	MATCH("self", TKN_KW_SELF);
	MATCH("init", TKN_KW_INIT);
	MATCH("deinit", TKN_KW_DEINIT);
	MATCH("defer", TKN_KW_DEFER);
	MATCH("switch", TKN_KW_SWITCH);
	MATCH("case", TKN_KW_CASE);
	MATCH("default", TKN_KW_DEFAULT);
	MATCH("true", TKN_KW_TRUE);
	MATCH("false", TKN_KW_FALSE);
	MATCH("as", TKN_KW_AS);

#undef MATCH
	return TKN_IDENT;
}

// Helper to append a token
static void emit_token(TokenStream *ts, TokenKind kind, uint32_t offset, uint32_t len, uint32_t line, uint32_t col) {
	Token t = {
		.kind = kind,
		.line = line,
		.col = col,
		.offset = offset,
		.length = len,
		.value = { 0 },
	};
	arrpush(ts->tokens, t);
}

void lexize(TokenStream *ts) {
	// Pre-allocate some tokens
	arrsetcap(ts->tokens, 1024);

	while (ts->lex.pos < ts->lex.src_len) {
		char c = ts->lex.src[ts->lex.pos];

		// Skip whitespace
		if (isspace(c)) {
			if (c == '\n') {
				ts->lex.line++;
				ts->lex.col = 0;
			}
			ts->lex.pos++;
			ts->lex.col++;
			continue;
		}

		// Comments
		if (c == '/' && ts->lex.pos + 1 < ts->lex.src_len) {
			if (ts->lex.src[ts->lex.pos + 1] == '/') {
				// Line comment - skip to newline
				ts->lex.pos += 2;
				while (ts->lex.pos < ts->lex.src_len &&
					   ts->lex.src[ts->lex.pos] != '\n') {
					ts->lex.pos++;
				}
				continue;
			}
			if (ts->lex.src[ts->lex.pos + 1] == '*') {
				// Block comment - skip to */
				ts->lex.pos += 2;
				while (ts->lex.pos + 1 < ts->lex.src_len) {
					if (ts->lex.src[ts->lex.pos] == '*' &&
						ts->lex.src[ts->lex.pos + 1] == '/') {
						ts->lex.pos += 2;
						break;
					}
					if (ts->lex.src[ts->lex.pos] == '\n') {
						ts->lex.line++;
						ts->lex.col = 0;
					}
					ts->lex.pos++;
				}
				continue;
			}
		}

		// Identifiers and keywords
		if (isalpha(c) || c == '_') {
			uint32_t start = ts->lex.pos;
			uint32_t line = ts->lex.line;
			uint32_t col = ts->lex.col;

			ts->lex.pos++;
			while (ts->lex.pos < ts->lex.src_len && (isalnum(ts->lex.src[ts->lex.pos]) ||
													 ts->lex.src[ts->lex.pos] == '_')) {
				ts->lex.pos++;
			}

			uint32_t len = ts->lex.pos - start;
			TokenKind kind = keyword_kind(ts->lex.src + start, len);
			emit_token(ts, kind, start, len, line, col);
			continue;
		}

		// Numbers
		if (isdigit(c)) {
			uint32_t start = ts->lex.pos;
			uint32_t line = ts->lex.line;
			uint32_t col = ts->lex.col;

			// Handle hex/octal/binary prefixes
			if (c == '0' && ts->lex.pos + 1 < ts->lex.src_len) {
				char next = ts->lex.src[ts->lex.pos + 1];
				if (next == 'x' || next == 'X') {
					// Hex
					ts->lex.pos += 2;
					while (ts->lex.pos < ts->lex.src_len &&
						   isxdigit(ts->lex.src[ts->lex.pos])) {
						ts->lex.pos++;
					}
					emit_token(ts, TKN_INT_LITERAL, start, ts->lex.pos - start, line, col);
					continue;
				}
				if (next == 'o' || next == 'O') {
					// Octal
					ts->lex.pos += 2;
					while (ts->lex.pos < ts->lex.src_len &&
						   ts->lex.src[ts->lex.pos] >= '0' &&
						   ts->lex.src[ts->lex.pos] <= '7') {
						ts->lex.pos++;
					}
					emit_token(ts, TKN_INT_LITERAL, start, ts->lex.pos - start, line, col);
					continue;
				}
				if (next == 'b' || next == 'B') {
					// Binary
					ts->lex.pos += 2;
					while (ts->lex.pos < ts->lex.src_len &&
						   (ts->lex.src[ts->lex.pos] == '0' ||
							ts->lex.src[ts->lex.pos] == '1')) {
						ts->lex.pos++;
					}
					emit_token(ts, TKN_INT_LITERAL, start, ts->lex.pos - start, line, col);
					continue;
				}
			}

			// Decimal or float
			while (ts->lex.pos < ts->lex.src_len && isdigit(ts->lex.src[ts->lex.pos])) {
				ts->lex.pos++;
			}

			// Check for float
			if (ts->lex.pos < ts->lex.src_len && ts->lex.src[ts->lex.pos] == '.') {
				ts->lex.pos++;
				while (ts->lex.pos < ts->lex.src_len &&
					   isdigit(ts->lex.src[ts->lex.pos])) {
					ts->lex.pos++;
				}
				emit_token(ts, TKN_FLOAT_LITERAL, start, ts->lex.pos - start, line, col);
				continue;
			}

			emit_token(ts, TKN_INT_LITERAL, start, ts->lex.pos - start, line, col);
			continue;
		}

		// Strings
		if (c == '"') {
			uint32_t start = ts->lex.pos;
			uint32_t line = ts->lex.line;
			uint32_t col = ts->lex.col;

			ts->lex.pos++;
			while (ts->lex.pos < ts->lex.src_len && ts->lex.src[ts->lex.pos] != '"') {
				if (ts->lex.src[ts->lex.pos] == '\\' &&
					ts->lex.pos + 1 < ts->lex.src_len) {
					ts->lex.pos += 2; // Skip escape sequence
				} else {
					ts->lex.pos++;
				}
			}
			if (ts->lex.pos < ts->lex.src_len && ts->lex.src[ts->lex.pos] == '"') {
				ts->lex.pos++; // Include closing quote
			}
			emit_token(ts, TKN_STRING_LITERAL, start, ts->lex.pos - start, line, col);
			continue;
		}

		// Characters
		if (c == '\'') {
			uint32_t start = ts->lex.pos;
			uint32_t line = ts->lex.line;
			uint32_t col = ts->lex.col;

			ts->lex.pos++;
			if (ts->lex.pos < ts->lex.src_len && ts->lex.src[ts->lex.pos] == '\\') {
				ts->lex.pos += 2; // Skip escape sequence
			} else if (ts->lex.pos < ts->lex.src_len) {
				ts->lex.pos++;
			}
			if (ts->lex.pos < ts->lex.src_len && ts->lex.src[ts->lex.pos] == '\'') {
				ts->lex.pos++;
			}
			emit_token(ts, TKN_CHAR_LITERAL, start, ts->lex.pos - start, line, col);
			continue;
		}

		// Multi-character punctuation and operators
		if (ts->lex.pos + 1 < ts->lex.src_len) {
			char next = ts->lex.src[ts->lex.pos + 1];

			// Two-character operators
			if (c == '-' && next == '>') {
				emit_token(ts, TKN_ARROW, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
			if (c == '=' && next == '=') {
				emit_token(ts, TKN_EQEQ, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
			if (c == '!' && next == '=') {
				emit_token(ts, TKN_BANGEQ, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
			if (c == '&' && next == '&') {
				emit_token(ts, TKN_AMPAMP, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
			if (c == '|' && next == '|') {
				emit_token(ts, TKN_PIPEPIPE, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
			if (c == '+' && next == '+') {
				emit_token(ts, TKN_PLUSPLUS, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
			if (c == '<' && next == '=') {
				emit_token(ts, TKN_LTEQ, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
			if (c == '>' && next == '=') {
				emit_token(ts, TKN_GTEQ, ts->lex.pos, 2, ts->lex.line, ts->lex.col);
				ts->lex.pos += 2;
				continue;
			}
		}

		// Single-character tokens
		if (strchr("(){}[]:;,.-+*/%=", c)) {
			TokenKind kind;
			switch (c) {
			case '(':
				kind = TKN_LPAREN;
				break;
			case ')':
				kind = TKN_RPAREN;
				break;
			case '{':
				kind = TKN_LBRACE;
				break;
			case '}':
				kind = TKN_RBRACE;
				break;
			case '[':
				kind = TKN_LBRACKET;
				break;
			case ']':
				kind = TKN_RBRACKET;
				break;
			case ':':
				kind = TKN_COLON;
				break;
			case ';':
				kind = TKN_SEMI;
				break;
			case ',':
				kind = TKN_COMMA;
				break;
			case '.':
				kind = TKN_DOT;
				break;
			case '+':
				kind = TKN_PLUS;
				break;
			case '-':
				kind = TKN_MINUS;
				break;
			case '*':
				kind = TKN_STAR;
				break;
			case '/':
				kind = TKN_SLASH;
				break;
			case '%':
				kind = TKN_PERCENT;
				break;
			case '=':
				kind = TKN_EQ;
				break;
			default:
				kind = TKN_ERROR;
				break;
			}
			emit_token(ts, kind, ts->lex.pos, 1, ts->lex.line, ts->lex.col);
			ts->lex.pos++;
			continue;
		}

		// Unknown character - error
		emit_token(ts, TKN_ERROR, ts->lex.pos, 1, ts->lex.line, ts->lex.col);
		ts->lex.pos++;
	}

	// Emit EOF
	emit_token(ts, TKN_EOF, ts->lex.pos, 0, ts->lex.line, ts->lex.col);
}

Token *ts_peek(TokenStream *ts, size_t lookahead) {
	return &(ts->tokens[ts->cursor + lookahead]);
}

Token *ts_curr(TokenStream *ts) {
	return ts_peek(ts, 0);
}
Token *ts_advance(TokenStream *ts) {
	Token *prev = ts_curr(ts);
	ts->cursor++;
	return prev;
}
// just returns cursor
size_t ts_mark(TokenStream *ts) {
	return ts->cursor;
}
// just sets cursor — O(1), no re-lexing
void ts_reset(TokenStream *ts, size_t mark) {
	ts->cursor = mark;
}
void tkn_print(Token tkn, char *source) {
	switch (tkn.kind) {
	case TKN_EOF: {
		printf("EOF: ");
		break;
	}
	case TKN_IDENT: {
		printf("IDENT: ");
		break;
	}
	case TKN_INT_LITERAL: {
		printf("INT_LITERAL: ");
		break;
	}
	case TKN_FLOAT_LITERAL: {
		printf("FLOAT_LITERAL: ");
		break;
	}
	case TKN_STRING_LITERAL: {
		printf("STRING_LITERAL: ");
		break;
	}
	case TKN_CHAR_LITERAL: {
		printf("CHAR_LITERAL: ");
		break;
	}
	case TKN_KW_FN: {
		printf("KW_FN: ");
		break;
	}
	case TKN_KW_OBJ: {
		printf("KW_OBJ: ");
		break;
	}
	case TKN_KW_ENUM: {
		printf("KW_ENUM: ");
		break;
	}
	case TKN_KW_UNION: {
		printf("KW_UNION: ");
		break;
	}
	case TKN_KW_CONST: {
		printf("KW_CONST: ");
		break;
	}
	case TKN_KW_VAR: {
		printf("KW_VAR: ");
		break;
	}
	case TKN_KW_LET: {
		printf("KW_LET: ");
		break;
	}
	case TKN_KW_IF: {
		printf("KW_IF: ");
		break;
	}
	case TKN_KW_ELSE: {
		printf("KW_ELSE: ");
		break;
	}
	case TKN_KW_WHILE: {
		printf("KW_WHILE: ");
		break;
	}
	case TKN_KW_FOR: {
		printf("KW_FOR: ");
		break;
	}
	case TKN_KW_RETURN: {
		printf("KW_RETURN: ");
		break;
	}
	case TKN_KW_BREAK: {
		printf("KW_BREAK: ");
		break;
	}
	case TKN_KW_CONTINUE: {
		printf("KW_CONTINUE: ");
		break;
	}
	case TKN_KW_MODULE: {
		printf("KW_MODULE: ");
		break;
	}
	case TKN_KW_IMPORT: {
		printf("KW_IMPORT: ");
		break;
	}
	case TKN_KW_PUB: {
		printf("KW_PUB: ");
		break;
	}
	case TKN_KW_SELF: {
		printf("KW_SELF: ");
		break;
	}
	case TKN_KW_INIT: {
		printf("KW_INIT: ");
		break;
	}
	case TKN_KW_DEINIT: {
		printf("KW_DEINIT: ");
		break;
	}
	case TKN_KW_DEFER: {
		printf("KW_DEFER: ");
		break;
	}
	case TKN_KW_SWITCH: {
		printf("KW_SWITCH: ");
		break;
	}
	case TKN_KW_CASE: {
		printf("KW_CASE: ");
		break;
	}
	case TKN_KW_DEFAULT: {
		printf("KW_DEFAULT: ");
		break;
	}
	case TKN_KW_TRUE: {
		printf("KW_TRUE: ");
		break;
	}
	case TKN_KW_FALSE: {
		printf("KW_FALSE: ");
		break;
	}
	case TKN_KW_AS: {
		printf("KW_AS: ");
		break;
	}
	case TKN_LPAREN: {
		printf("LPAREN: ");
		break;
	}
	case TKN_RPAREN: {
		printf("RPAREN: ");
		break;
	}
	case TKN_LBRACE: {
		printf("LBRACE: ");
		break;
	}
	case TKN_RBRACE: {
		printf("RBRACE: ");
		break;
	}
	case TKN_LBRACKET: {
		printf("LBRACKET: ");
		break;
	}
	case TKN_RBRACKET: {
		printf("RBRACKET: ");
		break;
	}
	case TKN_ARROW: {
		printf("ARROW: ");
		break;
	}
	case TKN_COLON: {
		printf("COLON: ");
		break;
	}
	case TKN_SEMI: {
		printf("SEMI: ");
		break;
	}
	case TKN_COMMA: {
		printf("COMMA: ");
		break;
	}
	case TKN_DOT: {
		printf("DOT: ");
		break;
	}
	case TKN_PLUS: {
		printf("PLUS: ");
		break;
	}
	case TKN_MINUS: {
		printf("MINUS: ");
		break;
	}
	case TKN_STAR: {
		printf("STAR: ");
		break;
	}
	case TKN_SLASH: {
		printf("SLASH: ");
		break;
	}
	case TKN_PERCENT: {
		printf("PERCENT: ");
		break;
	}
	case TKN_EQ: {
		printf("EQ: ");
		break;
	}
	case TKN_EQEQ: {
		printf("EQEQ: ");
		break;
	}
	case TKN_BANGEQ: {
		printf("BANGEQ: ");
		break;
	}
	case TKN_LT: {
		printf("LT: ");
		break;
	}
	case TKN_LTEQ: {
		printf("LTEQ: ");
		break;
	}
	case TKN_GT: {
		printf("GT: ");
		break;
	}
	case TKN_GTEQ: {
		printf("GTEQ: ");
		break;
	}
	case TKN_AMPAMP: {
		printf("AMPAMP: ");
		break;
	}
	case TKN_PIPEPIPE: {
		printf("PIPEPIPE: ");
		break;
	}
	case TKN_BANG: {
		printf("BANG: ");
		break;
	}
	case TKN_PLUSPLUS: {
		printf("PLUSPLUS: ");
		break;
	}
	case TKN_ERROR: {
		printf("ERROR: ");
		break;
	}
	}
	printf("%.*s, %d:%d\n", tkn.length, tkn.offset + source, tkn.line, tkn.col);
}

void ts_dump(TokenStream *ts) {
	for (int i = 0; i < arrlen(ts->tokens); i++) {
		tkn_print(ts->tokens[i], (char *)ts->lex.src);
	}
}
