#include "../parse/nodes.h"

void destroy_node(Node *node) {
	return;
	if (node == NULL)
		return;
	Node *curr = node->first_child;
	while (curr != NULL) {
		Node *next = curr->next_sibling;
		destroy_node(curr);
		curr = next;
	}
	if (node->name) {
		free(node->name);
	}
	node->first_child = NULL;
	node->next_sibling = NULL;
	node->last_child = NULL;
	node->parent = NULL;

	free(node);
}

void append_child(Node *parent, Node *child) {
	if (parent == NULL || child == NULL)
		return;
	child->parent = parent;
	child->next_sibling = NULL;
	if (parent->first_child == NULL) {
		parent->first_child = child;
		parent->last_child = child;
		child->prev_sibling = NULL;
	} else {
		child->prev_sibling = parent->last_child;
		parent->last_child->next_sibling = child;
		parent->last_child = child;
	}
	parent->number_of_children++;
}

Node *create_node(Arena *arena, NodeType node_type, char *node_name) {
	Node *node = (Node *) alloc(arena, sizeof(Node));
	if (node == NULL) {
		perror("Failed to allocate AST Node");
		return NULL;
	}
	node->type = node_type;
	node->name = NULL;
	node->first_child = NULL;
	node->last_child = NULL;
	node->next_sibling = NULL;
	node->prev_sibling = NULL;
	node->parent = NULL;
	node->line = 0;
	node->col = 0;
	node->number_of_children = 0;
	node->resolved_type = NULL;
	memset(&node->data, 0, sizeof(NodeData));
	if (node_name[0] != '\0') {
		char *name = (char *) alloc(arena, sizeof(char) * (strlen(node_name) + 1));
		if (name == NULL) {
			perror("Failed to allocate AST Node Name");
			return NULL;
		}
		strcpy(name, node_name);
		node->name = name;
	}

	return node;
}

Node *get_first_child_of_type(Node *parent, NodeType type) {
	if (parent == NULL)
		return NULL;
	Node *child = parent->first_child;
	while (child != NULL) {
		if (child->type == type) {
			return child;
		}
		child = child->next_sibling;
	}
	return NULL;
}

Node *get_last_child_of_type(Node *parent, NodeType type) {
	if (parent == NULL)
		return NULL;
	Node *child = parent->last_child;
	while (child != NULL) {
		if (child->type == type) {
			return child;
		}
		child = child->prev_sibling;
	}
	return NULL;
}

Node *get_next_sibling_of_type(Node *node, NodeType type) {
	if (node == NULL)
		return NULL;
	Node *sibling = node->next_sibling;
	while (sibling != NULL) {
		if (sibling->type == type) {
			return sibling;
		}
		sibling = sibling->next_sibling;
	}
	return NULL;
}

Node *get_prev_sibling_of_type(Node *node, NodeType type) {
	if (node == NULL)
		return NULL;
	Node *sibling = node->prev_sibling;
	while (sibling != NULL) {
		if (sibling->type == type) {
			return sibling;
		}
		sibling = sibling->prev_sibling;
	}
	return NULL;
}

Node *get_first_child_by_name(Node *parent, const char *name) {
	if (parent == NULL || name == NULL)
		return NULL;
	Node *child = parent->first_child;
	while (child != NULL) {
		if (child->name != NULL && strcmp(child->name, name) == 0) {
			return child;
		}
		child = child->next_sibling;
	}
	return NULL;
}

Node *get_next_sibling_by_name(Node *node, const char *name) {
	if (node == NULL || name == NULL)
		return NULL;
	Node *sibling = node->next_sibling;
	while (sibling != NULL) {
		if (sibling->name != NULL && strcmp(sibling->name, name) == 0) {
			return sibling;
		}
		sibling = sibling->next_sibling;
	}
	return NULL;
}

Node *get_first_descendant_of_type(Node *root, NodeType type) {
	if (root == NULL)
		return NULL;
	if (root->type == type)
		return root;

	Node *child = root->first_child;
	while (child != NULL) {
		Node *result = get_first_descendant_of_type(child, type);
		if (result != NULL)
			return result;
		child = child->next_sibling;
	}
	return NULL;
}

Node *get_first_child_where(Node *parent, bool (*predicate)(Node *)) {
	if (parent == NULL || predicate == NULL)
		return NULL;
	Node *child = parent->first_child;
	while (child != NULL) {
		if (predicate(child)) {
			return child;
		}
		child = child->next_sibling;
	}
	return NULL;
}

Node *get_next_sibling_where(Node *node, bool (*predicate)(Node *)) {
	if (node == NULL || predicate == NULL)
		return NULL;
	Node *sibling = node->next_sibling;
	while (sibling != NULL) {
		if (predicate(sibling)) {
			return sibling;
		}
		sibling = sibling->next_sibling;
	}
	return NULL;
}

Node *get_parent_of_type(Node *node, NodeType type) {
	if (node == NULL)
		return NULL;
	Node *parent = node->parent;
	while (parent != NULL) {
		if (parent->type == type) {
			return parent;
		}
		parent = parent->parent;
	}
	return NULL;
}

bool has_child_of_type(Node *parent, NodeType type) {
	return get_first_child_of_type(parent, type) != NULL;
}

int count_children_of_type(Node *parent, NodeType type) {
	if (parent == NULL)
		return 0;
	int count = 0;
	Node *child = parent->first_child;
	while (child != NULL) {
		if (child->type == type) {
			count++;
		}
		child = child->next_sibling;
	}
	return count;
}

Node *get_child_at_index(Node *parent, size_t index) {
	if (parent == NULL || index >= parent->number_of_children)
		return NULL;
	Node *child = parent->first_child;
	for (size_t i = 0; i < index; i++) {
		child = child->next_sibling;
	}
	return child;
}

Node *find_first_in_subtree(Node *root, bool (*predicate)(Node *)) {
	if (root == NULL || predicate == NULL)
		return NULL;

// Use a stack to do iterative DFS to avoid recursion issues
#define MAX_STACK_SIZE 1024
	Node *stack[MAX_STACK_SIZE];
	int top = 0;
	stack[top++] = root;

	while (top > 0) {
		Node *current = stack[--top];
		if (predicate(current)) {
			return current;
		}

		// Push children in reverse order so they're processed left-to-right
		Node *child = current->last_child;
		while (child != NULL) {
			if (top < MAX_STACK_SIZE) {
				stack[top++] = child;
			}
			child = child->prev_sibling;
		}
	}
	return NULL;
}

Node *get_first_child(Node *parent) {
	if (parent == NULL)
		return NULL;
	return parent->first_child;
}

Node *get_last_child(Node *parent) {
	if (parent == NULL)
		return NULL;
	return parent->last_child;
}

Node *get_next_sibling(Node *node) {
	if (node == NULL)
		return NULL;
	return node->next_sibling;
}

Node *get_prev_sibling(Node *node) {
	if (node == NULL)
		return NULL;
	return node->prev_sibling;
}

Node *get_parent(Node *node) {
	if (node == NULL)
		return NULL;
	return node->parent;
}

Node *get_child_at(Node *parent, size_t index) {
	if (parent == NULL || index >= parent->number_of_children)
		return NULL;
	Node *child = parent->first_child;
	for (size_t i = 0; i < index; i++) {
		child = child->next_sibling;
	}
	return child;
}

size_t get_child_count(Node *parent) {
	if (parent == NULL)
		return 0;
	return parent->number_of_children;
}

bool has_children(Node *parent) {
	if (parent == NULL)
		return false;
	return parent->first_child != NULL;
}

bool is_leaf(Node *node) {
	if (node == NULL)
		return false;
	return node->first_child == NULL;
}

bool is_first_child(Node *node) {
	if (node == NULL || node->parent == NULL)
		return false;
	return node->parent->first_child == node;
}

bool is_last_child(Node *node) {
	if (node == NULL || node->parent == NULL)
		return false;
	return node->parent->last_child == node;
}

Node *get_root(Node *node) {
	if (node == NULL)
		return NULL;
	while (node->parent != NULL) {
		node = node->parent;
	}
	return node;
}

Node *get_nth_sibling(Node *node, int offset) {
	if (node == NULL)
		return NULL;

	if (offset > 0) {
		// Move forward
		Node *current = node;
		for (int i = 0; i < offset; i++) {
			if (current->next_sibling == NULL)
				return NULL;
			current = current->next_sibling;
		}
		return current;
	} else if (offset < 0) {
		// Move backward
		Node *current = node;
		for (int i = 0; i < -offset; i++) {
			if (current->prev_sibling == NULL)
				return NULL;
			current = current->prev_sibling;
		}
		return current;
	}
	return node; // offset == 0
}
