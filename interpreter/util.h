#ifndef UTIL_H
#define UTIL_H
#include "nodes.h"


void print_ast(Node *node, char *source, int depth, bool last_child);
const char *primitive_name(PrimitiveType pt);
char *slice_string(Slice slice);
#endif
