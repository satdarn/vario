#include "util.h"

const char *node_type_name(NodeType type) {
	switch (type) {
	case NODE_PROGRAM:
		return "PROGRAM";
	case NODE_FUNC_DECL:
		return "FUNC_DECL";
	case NODE_OBJ_DECL:
		return "OBJ_DECL";
	case NODE_ENUM_DECL:
		return "ENUM_DECL";
	case NODE_UNION_DECL:
		return "UNION_DECL";
	case NODE_CONST_DECL:
		return "CONST_DECL";
	case NODE_VAR_DECL:
		return "VAR_DECL";
	case NODE_LET_DECL:
		return "LET_DECL";
	case NODE_MODULE_DECL:
		return "MODULE_DECL";
	case NODE_IMPORT_DECL:
		return "IMPORT_DECL";
	case NODE_PARAMETER_LIST:
		return "PARAMETER_LIST";
	case NODE_PARAMETER:
		return "PARAMETER";
	case NODE_RETURN_TYPE:
		return "RETURN_TYPE";
	case NODE_BLOCK:
		return "BLOCK";
	case NODE_FIELD_DECL:
		return "FIELD_DECL";
	case NODE_METHOD_DECL:
		return "METHOD_DECL";
	case NODE_CONSTRUCTOR_DECL:
		return "CONSTRUCTOR_DECL";
	case NODE_DESTRUCTOR_DECL:
		return "DESTRUCTOR_DECL";
	case NODE_SELF_PARAM:
		return "SELF_PARAM";
	case NODE_ENUM_VARIANT:
		return "ENUM_VARIANT";
	case NODE_UNION_VARIANT:
		return "UNION_VARIANT";
	case NODE_VISIBLITY:
		return "VISIBILITY";
	case NODE_PRIMITIVE_TYPE:
		return "PRIMITIVE_TYPE";
	case NODE_POINTER_TYPE:
		return "POINTER_TYPE";
	case NODE_SLICE_TYPE:
		return "SLICE_TYPE";
	case NODE_OBJ_TYPE:
		return "OBJ_TYPE";
	case NODE_ENUM_TYPE:
		return "ENUM_TYPE";
	case NODE_UNION_TYPE:
		return "UNION_TYPE";
	case NODE_ASSIGNMENT_STMT:
		return "ASSIGNMENT_STMT";
	case NODE_ASSIGNMENT_OPERATOR:
		return "ASSIGNMENT_OP";
	case NODE_CONDITIONAL_STMT:
		return "IF";
	case NODE_WHILE_LOOP:
		return "WHILE";
	case NODE_FOR_LOOP:
		return "FOR";
	case NODE_FOR_INIT:
		return "FOR_INIT";
	case NODE_FOR_UPDATE:
		return "FOR_UPDATE";
	case NODE_RANGE_FOR_LOOP:
		return "RANGE_FOR";
	case NODE_SWITCH_STMT:
		return "SWITCH";
	case NODE_SWITCH_CASE:
		return "CASE";
	case NODE_SWITCH_DEFAULT:
		return "DEFAULT";
	case NODE_RETURN_STMT:
		return "RETURN";
	case NODE_BREAK_STMT:
		return "BREAK";
	case NODE_CONTINUE_STMT:
		return "CONTINUE";
	case NODE_DEFER_STMT:
		return "DEFER";
	case NODE_LOGICAL_OR_EXPRESSION:
		return "LOGICAL_OR";
	case NODE_LOGICAL_AND_EXPRESSION:
		return "LOGICAL_AND";
	case NODE_EQUALITY_EXPRESSION:
		return "EQUALITY";
	case NODE_RELATIONAL_EXPRESSION:
		return "RELATIONAL";
	case NODE_ADDITIVE_EXPRESSION:
		return "ADDITIVE";
	case NODE_MULTIPLICTIVE_EXPRESSION:
		return "MULTIPLICATIVE";
	case NODE_UNARY_EXPRESSION:
		return "UNARY";
	case NODE_FUNC_CALL:
		return "FUNC_CALL";
	case NODE_ACCESS:
		return "ACCESS";
	case NODE_INDEX:
		return "INDEX";
	case NODE_INC_DEC:
		return "INC_DEC";
	case NODE_CAST_EXPRESSION:
		return "CAST";
	case NODE_GROUPED_EXPRESSION:
		return "GROUP";
	case NODE_SIZE_OF_EXPRESSION:
		return "SIZEOF";
	case NODE_ARGUMENT_LIST:
		return "ARGUMENT_LIST";
	case NODE_IDENTIFER:
		return "IDENTIFIER";
	case NODE_DECIMAL_LITERAL:
		return "DECIMAL";
	case NODE_HEX_LITERAL:
		return "HEX";
	case NODE_OCTAL_LITERAL:
		return "OCTAL";
	case NODE_BINARY_LITERAL:
		return "BINARY";
	case NODE_FLOAT_LITERAL:
		return "FLOAT";
	case NODE_STRING_LITERAL:
		return "STRING";
	case NODE_BOOLEAN_LITERAL:
		return "BOOL";
	case NODE_ARRAY_LITERAL:
		return "ARRAY";
	case NODE_DECIMAL_DIGIT:
		return "DECIMAL_DIGIT";
	default:
		return "UNKNOWN";
	}
}

static const char *op_name(Op op) {
	switch (op) {
	case plus:
		return "+";
	case minus:
		return "-";
	case star:
		return "*";
	case slash:
		return "/";
	case percent:
		return "%";
	case eq_equals:
		return "==";
	case not_equals:
		return "!=";
	case less_than:
		return "<";
	case less_than_eq:
		return "<=";
	case greater_than:
		return ">";
	case greater_than_eq:
		return ">=";
	case log_not:
		return "!";
	case log_and:
		return "&&";
	case log_or:
		return "||";
	case bit_not:
		return "~";
	case and_perc:
		return "&";
	case plus_plus:
		return "++";
	case minus_minus:
		return "--";
	case equals:
		return "=";
	case plus_equals:
		return "+=";
	case minus_equals:
		return "-=";
	case star_equals:
		return "*=";
	case slash_equals:
		return "/=";
	case percent_equals:
		return "%=";
	case and_equals:
		return "&=";
	case or_equals:
		return "|=";
	case xor_equals:
		return "^=";
	case lshift_equals:
		return "<<=";
	case rshift_equals:
		return ">>=";
	default:
		return "?";
	}
}

const char *primitive_name(PrimitiveType pt) {
	switch (pt) {
	case u8:
		return "u8";
	case u32:
		return "u32";
	case u64:
		return "u64";
	case i32:
		return "i32";
	case i64:
		return "i64";
	case f32:
		return "f32";
	case f64:
		return "f64";
	case boolean:
		return "bool";
	case voidian:
		return "void";
	case usize:
		return "usize";
	case isize:
		return "isize";
	default:
		return "?";
	}
}

// Print the indent prefix, using '│' and '├'/'└' box-drawing chars
static void print_indent(int depth, bool last_child) {
	// We need to track which levels are "still open" to draw │ lines.
	// Simple approach: just use spaces + branch chars, no tracking needed
	// for a basic pretty printer.
	for (int i = 0; i < depth - 1; i++)
		printf("│   ");
	if (depth > 0)
		printf(last_child ? "└── " : "├── ");
}

void print_node_inline(Node *node, char *source) {
	switch (node->type) {
	case NODE_IDENTIFER:
	case NODE_DECIMAL_LITERAL:
	case NODE_HEX_LITERAL:
	case NODE_OCTAL_LITERAL:
	case NODE_BINARY_LITERAL:
	case NODE_FLOAT_LITERAL:
	case NODE_STRING_LITERAL:
	case NODE_BOOLEAN_LITERAL:
	case NODE_OBJ_TYPE:
	case NODE_ENUM_TYPE:
	case NODE_UNION_TYPE:
	case NODE_RETURN_TYPE: {
		size_t len = node->data.literal.end - node->data.literal.start;
		printf(" \"%.*s\"", (int) len, source + node->data.literal.start);
		break;
	}
	case NODE_PRIMITIVE_TYPE:
		printf(" %s", primitive_name(node->data.primitive_type));
		break;
	case NODE_VISIBLITY:
		printf(" %s", node->data.visiblity ? "pub" : "priv");
		break;
	case NODE_ASSIGNMENT_OPERATOR:
	case NODE_UNARY_EXPRESSION:
	case NODE_ADDITIVE_EXPRESSION:
	case NODE_MULTIPLICTIVE_EXPRESSION:
	case NODE_RELATIONAL_EXPRESSION:
	case NODE_EQUALITY_EXPRESSION:
	case NODE_INC_DEC:
		printf(" %s", op_name(node->data.op));
		break;
	default:
		//		printf("%s\n", node_type_name(node->type));
		break;
	}
}

void print_ast(Node *node, char *source, int depth, bool last_child) {
	if (!node)
		return;
	print_indent(depth, last_child);
	printf("%s", node_type_name(node->type));
	print_node_inline(node, source);
	printf("\n");
	int child_count = 0;
	Node *c = node->first_child;
	while (c) {
		child_count++;
		c = c->next_sibling;
	}
	c = node->first_child;
	int i = 0;
	while (c) {
		print_ast(c, source, depth + 1, i == child_count - 1);
		c = c->next_sibling;
		i++;
	}
}

//drop in replacement for strcmp
int slice_equals(Slice s1, Slice s2) {
	int len1 = s1.end - s1.start;
	int len2 = s2.end - s2.start;
	int min_len = (len1 < len2) ? len1 : len2;

	for (int i = 0; i < min_len; i++) {
		char c1 = s1.source[s1.start + i];
		char c2 = s2.source[s2.start + i];
		if (c1 != c2) {
			return c1 - c2;
		}
	}
	return len1 - len2;
}

char *slice_string(Arena *arena, Slice slice) {
	if (slice.source == NULL || slice.start > slice.end) {
		return NULL;
	}
	size_t length = slice.end - slice.start;
	char *result = (char *) alloc(arena, length + 1);
	if (result == NULL) {
		perror("Failed allocation from source to string");
		return NULL;
	}
	memcpy(result, slice.source + slice.start, length);
	result[length] = '\0';
	return result;
}
