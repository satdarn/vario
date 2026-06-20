#ifndef NODES_H
#define NODES_H

#include "common.h"
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
	NODE_DESTRUCTOR_DECL,
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
	NODE_UNARY_EXPRESSION,
	NODE_FUNC_CALL,
	NODE_ACCESS,
	NODE_INDEX,
	NODE_INC_DEC,
	NODE_SIZE_OF_EXPRESSION,
	NODE_CAST_EXPRESSION,
	NODE_GROUPED_EXPRESSION,
	NODE_ARGUMENT_LIST,
	NODE_FLOAT_LITERAL,
	NODE_STRING_LITERAL,
	NODE_BOOLEAN_LITERAL,
	NODE_ARRAY_LITERAL,
	NODE_IDENTIFER,
	NODE_DECIMAL_LITERAL,
	NODE_HEX_LITERAL,
	NODE_OCTAL_LITERAL,
	NODE_BINARY_LITERAL,
	NODE_DECIMAL_DIGIT,
} NodeType;
typedef enum {
	equals,
	plus_equals,
	minus_equals,
	star_equals,
	slash_equals,
	percent_equals,
	and_equals,
	or_equals,
	xor_equals,
	lshift_equals,
	rshift_equals,
	eq_equals,
	not_equals,
	less_than,
	less_than_eq,
	greater_than,
	greater_than_eq,
	plus,
	minus,
	star,
	slash,
	percent,
	log_not,
	log_and,
	log_or,
	bit_not,
	and_perc,
	plus_plus,
	minus_minus,
} Op;

typedef enum {
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
} PrimitiveType;
typedef union {
	bool visiblity;
	struct {
		size_t start;
		size_t end;
	} literal;
	Op op;
	PrimitiveType primitive_type;
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
    while (curr != NULL) {
        Node *next = curr->next_sibling;
        destroy_node(curr);
        curr = next;
    }

    // 2. Clear our own string memory if allocated (Optional, depends on how node->name is assigned)
    // if (node->name != NULL) { free(node->name); }

    // 3. Nullify pointers for safety before freeing
    node->first_child = NULL;
    node->next_sibling = NULL;
    node->last_sibling = NULL;
    node->parent = NULL;

    free(node);
}

static void append_child(Node *parent, Node *child) {
    if (parent == NULL || child == NULL) 
        return;
    child->parent = parent;
    child->next_sibling = NULL;
    if (parent->first_child == NULL) {
        parent->first_child = child;
        child->last_sibling = child; 
    } else {
        Node *tail = parent->first_child->last_sibling;
        tail->next_sibling = child;
        child->last_sibling = tail;
        parent->first_child->last_sibling = child;
    }
}


static Node *create_node(NodeType node_type, char *node_name) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) {
        perror("Failed to allocate AST Node");
        return NULL;
    }
    node->type = node_type;
    node->name = node_name;
    node->first_child = NULL;
    node->next_sibling = NULL;
    node->last_sibling = NULL;
    node->parent = NULL;
    node->line = 0;
    node->col = 0;
    memset(&node->data, 0, sizeof(NodeData));
    return node;
}
#endif
