#ifndef SEMA_H
#define SEMA_H
#include "../shared/util.h"
#include "../type/types.h"

typedef struct Node Node;
typedef struct Scope Scope;
typedef struct Symbol Symbol;

typedef enum {
    SYM_VARIABLE,
    SYM_CONSTANT,
    SYM_SELF_PARAMETER,
    SYM_PARAMETER,
    SYM_FIELD,
    SYM_METHOD,
    SYM_VARIANT,
    SYM_TYPE,
    SYM_FUNCTION,
} SymbolKind;

typedef enum {
    SCOPE_GLOBAL,
    SCOPE_FUNCTION,
    SCOPE_BLOCK,
    SCOPE_OBJECT,
    SCOPE_ENUM,
    SCOPE_UNION,
} ScopeKind;

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

struct Scope {
    SymbolTable symbols;
    struct Scope *parent;
    Node *node; // owner
    size_t depth;
    ScopeKind kind;
};

typedef struct {
    TypeTable *types;
    Scope *global;
    Scope *current_scope;
    Node *root;
    char *source;
} Sema;

void global_symbol_registration(Sema *sema);
void build_scopes(Sema *sema, Node *node);
Symbol *lookup_symbol(Scope *scope, const char *name);
Symbol *lookup_symbol_local(Scope *scope, const char *name);
void free_scopes(Node *root);
void type_check(Sema *sema);
#endif
