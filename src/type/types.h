#ifndef TYPES_H
#define TYPES_H
#include "../shared/util.h"

typedef struct Type Type;
typedef struct Scope Scope;

typedef struct {
	char *name;
	Type *type;
	uint32_t index;
} Field;

typedef struct {
	char *name;
	int64_t value;
} EnumVariant;

typedef struct {
	char *name;
	Type *type;
	uint32_t tag;
} UnionVariant;

typedef struct {
	Field *fields;
	size_t field_count;
} ObjectType;

typedef struct {
	EnumVariant *variants;
	size_t variant_count;
} EnumType;

typedef struct {
	UnionVariant *variants;
	size_t variant_count;
} UnionType;

typedef struct {
	Type *base;
} PointerType;

typedef struct {
	Type *element;
} SliceType;

typedef struct {
	Type **params;
	size_t param_count;
	Type *return_type;
} FunctionType;

typedef enum {
	TYPE_PRIM,
	TYPE_OBJ,
	TYPE_ENUM,
	TYPE_UNION,
	TYPE_POINTER,
	TYPE_SLICE,
	TYPE_FUNC,
	TYPE_NON = 67,
} TypeKind;

struct Type {
	TypeKind kind;
	char *name;
	Scope *scope;
	size_t size;
	union {
		ObjectType obj;
		EnumType enum_;
		UnionType union_;
		PointerType pointer;
		SliceType slice;
		PrimitiveType prim;
		FunctionType func;
	} data;
};

typedef struct {
	char *key;
	Type *value;
} TypeTableEntry;
typedef TypeTableEntry *TypeTable;

void print_type_table(TypeTable table);
Type *resolve_type(Arena *arena, TypeTable *table, Node *type_node);
Type *resolve_decl_type(Arena *arean, TypeTable *table, Node *decl_node);
TypeTable build_type_table(Arena *arena, Node *root);
void type_error(Node *node, const char *format, ...);
void free_type_table(TypeTable *table);
#endif
