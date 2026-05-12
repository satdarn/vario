#ifndef NODES_H
#define NODES_H

typedef enum {
	NODE_PROGRAM,
} NodeType;

typedef union {
} NodeData;

typedef struct Node {
	NodeType type;
	char *name;
	NodeData data;
	struct Node *first_child;
	struct Node *next_sibling;
	struct Node *parent;
	int line;
	int col;
} Node;

#define APPEND_CHILD(parent, child) do {                          \
		if ((parent) != NULL && (child) != NULL) {                \
			(child)->parent = (parent);                           \
			(child)->next_sibling = NULL;                         \
                                                                  \
			if ((parent)->first_child == NULL) {                  \
				(parent)->first_child = (child);                  \
			} else {                                              \
				Node *append_child_curr =                         \
					(parent)->first_child;                        \
                                                                  \
				while (append_child_curr->next_sibling != NULL) { \
					append_child_curr =                           \
						append_child_curr->next_sibling;          \
				}                                                 \
                                                                  \
				append_child_curr->next_sibling = (child);        \
			}                                                     \
		}                                                         \
	}                                                             \
	while (0)
#define CREATE_NODE(node_type, node_name)                          \
	({                                                             \
		Node *node = (Node *)malloc(sizeof(Node));                 \
		if (node != NULL) {                                        \
			node->type = (node_type);                              \
			node->name = (node_name);                              \
			node->first_child = NULL;                              \
			node->next_sibling = NULL;                             \
			node->parent = NULL;                                   \
			node->line = 0;                                        \
			node->col = 0;                                         \
			memset(&node->data, 0, sizeof(NodeData));              \
		}                                                          \
		node;                                                      \
	})
#endif
