#include "parser.h"
#include "sema.h"
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
	analyize(parse_tree, source);
	free_scopes(parse_tree);
	destroy_node(parse_tree);
	free(source);
}
