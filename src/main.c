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

	Node *parse_tree = parse(source);
	//print_ast(parse_tree, source, 0, false);
	TypeTable types = build_type_table(parse_tree);
	//print_type_table(types);
	Sema sema = {0};
	sema.source = source;
	sema.root = parse_tree;
	sema.types = &types;
	global_symbol_registration(&sema);
	build_scopes(&sema, parse_tree);
	type_check(&sema);
	free_type_table(sema.types);
	free_scopes(parse_tree);
	destroy_node(parse_tree);
	free(source);

	//return false;
}
