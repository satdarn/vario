#ifndef UTIL_H
#define UTIL_H
#include "../parse/nodes.h"

void print_ast(Node *node, char *source, int depth, bool last_child);

void print_node_inline(Node *node, char *source);

const char *primitive_name(PrimitiveType pt);
const char *node_type_name(NodeType type);
char *slice_string(Slice slice);
#endif
