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
} TypeKind;

struct Type {
	TypeKind kind;
	char *name;
	Scope *scope;
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

typedef enum {
	SYM_VARIABLE,
	SYM_CONSTANT,
	SYM_PARAMETER,
	SYM_FIELD,
	SYM_METHOD,
	SYM_VARIANT,
	SYM_TYPE,
	SYM_FUNCTION,
} SymbolKind;

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
} SymbolTableEntry;

typedef SymbolTableEntry *SymbolTable;

typedef enum {
	SCOPE_GLOBAL,
	SCOPE_FUNCTION,
	SCOPE_BLOCK,
	SCOPE_OBJECT,
	SCOPE_ENUM,
	SCOPE_UNION,
} ScopeKind;

struct Scope {
	SymbolTable symbols;
	struct Scope *parent;
	Node *node; // owner
	size_t depth;
	ScopeKind kind;
};
typedef enum {
	ERR_UNKNOWN_IDENTIFIER,
	ERR_UNKNOWN_TYPE,
	ERR_DUPLICATE_DECLARATION,
	ERR_INVALID_ASSIGNMENT,
	ERR_INVALID_FIELD_ACCESS,
	ERR_INVALID_FUNCTION_CALL,
	ERR_MISSING_RETURN,
	ERR_CONST_REASSIGNMENT,
	ERR_LET_REASSIGNMENT,
} ErrorList;

typedef struct {
	TypeTable types;
	Scope *global;
	Scope *current_scope;
	Node *root;
	char *source;
} Sema;

bool analyize(Node *root, char *source);

#endif
