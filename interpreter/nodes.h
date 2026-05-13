#ifndef NODES_H
#define NODES_H
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	NODE_PROGRAM,
	NODE_MODULE_DECL,
	NODE_IMPORT_DECL,
	NODE_VISIBLITY,
	NODE_FUNC_DECL,
	NODE_PARAMETER_LIST,
	NODE_PARAMETER,
	NODE_RETURN_TYPE,
	NODE_BLOCK,
	NODE_PRIMITIVE_TYPE,
	NODE_POINTER_TYPE,
	NODE_SLICE_TYPE,
	NODE_OBJ_TYPE,
	NODE_ENUM_TYPE,
	NODE_UNION_TYPE,
	NODE_OBJ_DECL,
	NODE_FIELD_DECL,
	NODE_METHOD_DECL,
	NODE_SELF_PARAM,
	NODE_CONSTRUCTOR_DECL,
	NODE_DECONSTRUCTOR_DECL,
	NODE_ENUM_DECL,
	NODE_ENUM_VARIANT,
	NODE_UNION_DECL,
	NODE_UNION_VARIANT,
	NODE_CONST_DECL,
	NODE_VAR_DECL,
	NODE_LET_DECL,
	NODE_STATEMENT,
	NODE_DECLARATION_STMT,
	NODE_ASSIGNMENT_STMT,
	NODE_ASSIGNMENT_OPERATOR,
	NODE_EXPRESSION_STMT,
	NODE_RETURN_STMT,
	NODE_BREAK_STMT,
	NODE_CONTINUE_STMT,
	NODE_DEFER_STMT,
	NODE_BLOCK_STMT,
	NODE_CONDITIONAL_STMT,
	NODE_LOOP_STMT,
	NODE_WHILE_LOOP,
	NODE_FOR_LOOP,
	NODE_FOR_INIT,
	NODE_FOR_UPDATE,
	NODE_RANGE_FOR_LOOP,
	NODE_SWITCH_STMT,
	NODE_SWITCH_CASE,
	NODE_CASE_PATTERN,
	NODE_SWITCH_DEFAULT,
	NODE_EXPRESSION,
	NODE_LOGICAL_OR_EXPRESSION,
	NODE_LOGICAL_AND_EXPRESSION,
	NODE_EQUALITY_EXPRESSION,
	NODE_RELATIONAL_EXPRESSION,
	NODE_ADDITIVE_EXPRESSION,
	NODE_MULTIPLICTIVE_EXPRESSION,


} NodeType;

typedef union {
	bool visiblity;
	enum {
		u8,
		u32,
		u64,
		i32,
		i64,
		f32,
		f64,
		boolean,
		voidian,
		usize,
		isize,
	} primitive_type;
} NodeData;

typedef struct Node {
	NodeType type;
	char *name;
	NodeData data;
	struct Node *first_child;
	struct Node *last_sibling;
	struct Node *next_sibling;
	struct Node *parent;
	int line;
	int col;
} Node;

static void destroy_node(Node *node) {
	if (node == NULL)
		return;
	Node *curr = node->first_child;
	while (curr->next_sibling != NULL) {
		curr = curr->next_sibling;
		destroy_node(curr->last_sibling);
	}
	destroy_node(curr);
	if (node->next_sibling)
		node->next_sibling->last_sibling = node->last_sibling;
	if (node->last_sibling)
		node->last_sibling->next_sibling = node->next_sibling;
	free(node);
}
static void append_child(Node *parent, Node *child) {
	if (parent != NULL && child != NULL) {
		child->parent = parent;
		child->last_sibling = NULL;
		child->next_sibling = NULL;
		if (parent->first_child == NULL) {
			parent->first_child = child;
		} else {
			Node *curr = parent->first_child;
			while (curr->next_sibling != NULL) {
				curr = curr->next_sibling;
			}
			curr->next_sibling = child;
			child->last_sibling = curr;
		}
	}
}

static Node *create_node(NodeType node_type, char *node_name) {
	Node *node = (Node *)malloc(sizeof(Node));
	if (node != NULL) {
		node->type = (node_type);
		node->name = (node_name);
		node->first_child = NULL;
		node->next_sibling = NULL;
		node->parent = NULL;
		node->line = 0;
		node->col = 0;
		memset(&node->data, 0, sizeof(NodeData));
	}
	return node;
}
#endif
