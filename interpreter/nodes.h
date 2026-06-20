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
	char *name;
	NodeType type;
	NodeData data;
	struct Node *first_child;
	struct Node *last_child;
	struct Node *next_sibling;
	struct Node *prev_sibling;
	struct Node *parent;
	size_t number_of_children;
	int line;
	int col;
	void *resolved_type;
} Node;

typedef struct {
	Node *parent;
	Node *current;
	Node *first_child;
	Node *last_child;
	size_t current_child_index;
} NodeChildIter;
void destroy_node(Node *node);
void append_child(Node *parent, Node *child);
Node *create_node(NodeType node_type, char *node_name);
NodeChildIter *create_child_iter(Node *parent);
Node *start_iter(NodeChildIter *iter);
Node *end_iter(NodeChildIter *iter);
Node *next_iter(NodeChildIter *iter);
Node *prev_iter(NodeChildIter *iter);
Node *get_first_child_of_type(Node *parent, NodeType type);
Node *get_last_child_of_type(Node *parent, NodeType type);
Node *get_next_sibling_of_type(Node *node, NodeType type);
Node *get_prev_sibling_of_type(Node *node, NodeType type);
Node *get_first_child_by_name(Node *parent, const char *name);
Node *get_next_sibling_by_name(Node *node, const char *name);
Node *get_first_descendant_of_type(Node *root, NodeType type);
Node *get_first_child_where(Node *parent, bool (*predicate)(Node *));
Node *get_next_sibling_where(Node *node, bool (*predicate)(Node *));
Node *get_parent_of_type(Node *node, NodeType type);
bool has_child_of_type(Node *parent, NodeType type);
int count_children_of_type(Node *parent, NodeType type);
Node *get_child_at_index(Node *parent, size_t index);
Node *find_first_in_subtree(Node *root, bool (*predicate)(Node *));
Node *get_first_child(Node *parent);
Node *get_last_child(Node *parent);
Node *get_next_sibling(Node *node);
Node *get_prev_sibling(Node *node);
Node *get_parent(Node *node);
Node *get_child_at(Node *parent, size_t index);
Node *get_first_child_by_index(Node *parent, size_t index); // alias for get_child_at
size_t get_child_count(Node *parent);
bool has_children(Node *parent);
bool is_leaf(Node *node);
bool is_first_child(Node *node);
bool is_last_child(Node *node);
Node *get_root(Node *node);
Node *get_nth_sibling(Node *node, int offset); // offset can be positive (next) or negative (prev)
#endif
