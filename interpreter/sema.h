#ifndef SEMA_H
#define SEMA_H

#include "util.h"
#include <stdint.h>

typedef struct Node Node;
typedef struct Type Type;
typedef struct Scope Scope;
typedef struct Symbol Symbol;

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
	prim,
	obj,
	_enum,
	_union,
	pointer,
	slice,
	func,
} TypeKind;

struct Type {
	TypeKind kind;
	char *name;
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
} TypeTable;

typedef enum {
	variable,
	function
} SymbolKind;

struct Scope {
	struct {
		char *key;
		Symbol *value;
	} symbols;
	Scope *parent;
};

struct Symbol {
	SymbolKind kind;
	char *name;
	Type *type;
	Node *decl;
	Scope *scope;
};

typedef struct {
	char *key;
	Symbol *value;
} SymbolTable;

typedef enum {
	unknown_identifier,
	unknown_type,
	duplicate_declaration,
	invalid_assignment,
	invalid_field_access,
	invalid_function_call,
	missing_return,
	const_reassignment,
	let_reassignment,
} ErrorList;

typedef struct {
	TypeTable types;
	ErrorList errors;
} Sema;

bool analyize(Node *root, char *source);

TypeTable *build_type_table(Node *root);
#endif
