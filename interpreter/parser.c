#include "parser.h"
#include "nodes.h"

int main() {
	return 0;
}
Node *parse(char *data) {
	Tokens tokens = { 0 };
	tokens.data = data;
	tokens.size = strlen(data);
	return parse_program(&tokens);
}

Node *parse_program(Tokens *tokens) {
	Node *root = CREATE_NODE(NODE_PROGRAM, "Root");
	Node *curr = parse_top_level_decl(tokens);
	while (curr) {
		APPEND_CHILD(root, curr);
		curr = parse_top_level_decl(tokens);
	}
	return curr;
}

Node *parse_top_level_decl(Tokens *tokens) {
	size_t start_pos = tokens->position;
	Node *curr = parse_func_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_obj_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_enum_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_union_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_const_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_module_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	curr = parse_import_decl(tokens);
	if (curr)
		return curr;
	tokens->position = start_pos;
	return NULL;
}

Node *parse_module_decl(Tokens *tokens) {
	int start_pos = tokens->position;
	if (!peek_string(tokens, "module")) {
		tokens->position = start_pos;
		return NULL;
	}
	consume_string(tokens, "module");
	Node identifier = parse_identifier(tokens);
	if (!identifier) {
		tokens->position = start_pos;
		return NULL;
	}
	if (!consume_if(tokens, ';')) {
		tokens->position = start_pos;
		return NULL;
	}
	
}
