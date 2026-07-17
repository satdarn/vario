#include "token/tokens.h"
#include "parse/parser.h"
#include "type/types.h"
#include "sema/sema.h"
#include <stdio.h>

int main(int argc, char *argv[]) {
	if (argc < 3) {
		printf("Incorrect usage: %s <source> <destination>", argv[0]);
		return 1;
	}
	FILE *f = fopen(argv[1], "r");
	if (!f) {
		perror("fopen");
		return 1;
	}
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	rewind(f);
	char *source = malloc(size + 1);
	fread(source, 1, size, f);
	source[size] = '\0';
	fclose(f);

	TokenStream ts = { 0 };
	ts.lex.src = source;
	ts.lex.src_len = strlen(source);
	Arena str_arena = { 0 };
	ts.str_arena = &str_arena;
	lexize(&ts);
	// ts_dump(&ts, 0);
	Arena main_arena = {0};

	bool has_parse_error = false;
	Node *parse_tree = parse(&main_arena, &ts, &has_parse_error);
	// ts_dump(&ts, 0);
	print_ast(parse_tree, source, 0, false);
	if (has_parse_error) {
		destroy_arena(&main_arena);
		destroy_arena(&str_arena);
		arrfree(ts.tokens);
		free(source);
		return 1;
	}
	TypeTable types = build_type_table(&main_arena, parse_tree);
	print_type_table(types);
	Sema sema = { 0 };
	sema.source = source;
	sema.root = parse_tree;
	sema.types = &types;
	sema.arena = &main_arena;
	global_symbol_registration(&sema);
	build_scopes(&sema, parse_tree);
	type_check(&sema);
	free_type_table(sema.types);
	free_scopes(parse_tree);
	destroy_arena(&main_arena);
	destroy_arena(&str_arena);
	arrfree(ts.tokens);
	free(source);
}
