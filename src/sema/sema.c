#include "../sema/sema.h"

Scope *create_scope(Scope *parent, Node *node, ScopeKind kind) {
	Scope *scope = calloc(1, sizeof(Scope));
	if (!scope) {
		perror("create_scope: calloc");
		return NULL;
	}
	sh_new_strdup(scope->symbols);
	scope->parent = parent;
	scope->node = node;
	scope->kind = kind;
	scope->depth = parent ? parent->depth + 1 : 0;
	return scope;
}

void push_scope(Sema *sema, Scope *scope) { sema->current_scope = scope; }

void pop_scope(Sema *sema) {
	if (sema->current_scope)
		sema->current_scope = sema->current_scope->parent;
}

Symbol *make_symbol(SymbolKind kind, char *name, Type *type, Node *decl,
					Scope *scope) {
	Symbol *sym = calloc(1, sizeof(Symbol));
	if (!sym) {
		perror("make_symbol: calloc");
		return NULL;
	}
	sym->kind = kind;
	sym->name = name;
	sym->type = type;
	sym->decl = decl;
	sym->scope = scope;
	return sym;
}

bool add_symbol(Sema *sema, Symbol *symbol) {
	Scope *scope = sema->current_scope;
	if (shget(scope->symbols, symbol->name)) {
		printf("sema: '%s' already declared in this scope\n", symbol->name);
		return false;
	}
	shput(scope->symbols, symbol->name, symbol);
	return true;
}

Symbol *lookup_symbol(Scope *scope, const char *name) {
	for (Scope *s = scope; s; s = s->parent) {
		Symbol *sym = shget(s->symbols, name);
		if (sym)
			return sym;
	}
	return NULL;
}

Symbol *lookup_symbol_local(Scope *scope, const char *name) {
	if (!scope)
		return NULL;
	return shget(scope->symbols, name);
}

void global_symbol_registration(Sema *sema) {
	Scope *global = create_scope(NULL, sema->root, SCOPE_GLOBAL);
	if (!global)
		return;

	sema->root->scope = global;
	sema->global = global;
	sema->current_scope = global;

	for (Node *curr = get_first_child(sema->root); curr;
		 curr = get_next_sibling(curr)) {
		SymbolKind kind;
		Type *type = NULL;
		if (curr->type == NODE_FUNC_DECL) {
			kind = SYM_FUNCTION;
			type = resolve_decl_type(sema->types, curr);
		} else if (curr->type == NODE_CONST_DECL) {
			kind = SYM_CONSTANT;
			type = resolve_decl_type(sema->types, curr);
		} else {
			continue;
		}
		Node *ident = get_first_child_of_type(curr, NODE_IDENTIFER);
		char *name = slice_string(ident->data.literal);
		if (!type) {
			printf("warning: could not resolve type for '%s'\n", name);
			free(name);
			continue;
		}
		Symbol *sym = make_symbol(kind, name, type, curr, global);
		if (sym) {
			curr->resolved_symbol = sym;
			add_symbol(sema, sym);
		}
	}
}

static void free_symbol_table(SymbolTable *table) {
	if (!table || !*table)
		return;
	int len = shlen(*table);
	for (int i = 0; i < len; i++) {
		Symbol *sym = (*table)[i].value;
		if (sym) {
			if (sym->kind != SYM_SELF_PARAMETER && sym->name) {
				free(sym->name);
			}
			free(sym);
		}
	}
	shfree(*table);
}

Scope *find_nearest_scope_of_kind(Scope *scope, ScopeKind kind) {
	for (Scope *s = scope; s; s = s->parent) {
		if (s->kind == kind)
			return s;
	}
	return NULL;
}

void build_scopes(Sema *sema, Node *node) {
	if (!node)
		return;
	switch (node->type) {
	case NODE_OBJ_DECL: {
		Scope *scope = create_scope(sema->current_scope, node, SCOPE_OBJECT);
		push_scope(sema, scope);
		node->scope = scope;
		break;
	}
	case NODE_METHOD_DECL: {
		Scope *obj_scope =
			find_nearest_scope_of_kind(sema->current_scope, SCOPE_OBJECT);
		Scope *func_scope =
			create_scope(sema->current_scope, node, SCOPE_FUNCTION);
		push_scope(sema, func_scope);
		node->scope = func_scope;
		if (obj_scope) {
			Node *ident = get_first_child_of_type(node, NODE_IDENTIFER);
			char *name = slice_string(ident->data.literal);
			Type *method_type = resolve_decl_type(sema->types, node);
			Symbol *sym =
				make_symbol(SYM_METHOD, name, method_type, node, obj_scope);
			node->resolved_symbol = sym;
			shput(obj_scope->symbols, name, sym);
		}
		Node *self_param = get_first_child_of_type(node, NODE_SELF_PARAM);
		if (self_param) {
			Node *obj_type = get_first_child(self_param);
			char *type_name = slice_string(obj_type->data.literal);
			Type *self_type = shget(*sema->types, type_name);
			free(type_name);
			char *ptr_name = malloc(strlen(self_type->name) + 2);
			sprintf(ptr_name, "*%s", self_type->name);
			Type *ptr_type = shget(*sema->types, ptr_name);
			free(ptr_name);

			Symbol *sym = make_symbol(SYM_SELF_PARAMETER, "self", ptr_type,
									  self_param, obj_scope);
			self_param->resolved_symbol = sym;
			add_symbol(sema, sym);
		}
		break;
	}
	case NODE_FUNC_DECL: {
		Scope *func_scope =
			create_scope(sema->current_scope, node, SCOPE_FUNCTION);
		push_scope(sema, func_scope);
		node->scope = func_scope;
		break;
	}
	case NODE_BLOCK: {
		Scope *block_scope =
			create_scope(sema->current_scope, node, SCOPE_BLOCK);
		push_scope(sema, block_scope);
		node->scope = block_scope;
		break;
	}
	case NODE_PARAMETER: {
		Node *identfier = get_first_child_of_type(node, NODE_IDENTIFER);
		Node *type_node = get_next_sibling(identfier);
		Type *type = resolve_type(sema->types, type_node);
		char *name = slice_string(identfier->data.literal);
		printf("\nadding an parameter: %s to scope %p\n", name,
			   sema->current_scope);
		Symbol *sym =
			make_symbol(SYM_PARAMETER, name, type, node, sema->current_scope);
		if (sym)
			add_symbol(sema, sym);
		break;
	}

	case NODE_RANGE_FOR_LOOP: {
		Node *element = get_first_child_of_type(node, NODE_IDENTIFER);
		char *name = slice_string(element->data.literal);
		Symbol *sym =
			make_symbol(SYM_VARIABLE, name, NULL, node, sema->current_scope);
		element->resolved_symbol = sym;
		if (sym)
			add_symbol(sema, sym);
		break;
	}
	case NODE_VAR_DECL:
	case NODE_LET_DECL: {
		Node *identfier = get_first_child_of_type(node, NODE_IDENTIFER);
		Node *type_node = get_next_sibling(identfier);
		Type *type = resolve_type(sema->types, type_node);
		char *name = slice_string(identfier->data.literal);
		printf("\nadding an indentifier: %s to scope %p\n", name,
			   sema->current_scope);
		Symbol *sym =
			make_symbol(SYM_VARIABLE, name, type, node, sema->current_scope);
		node->resolved_symbol = sym;
		if (sym)
			add_symbol(sema, sym);
		break;
	}
	case NODE_FIELD_DECL: {
		Scope *obj_scope =
			find_nearest_scope_of_kind(sema->current_scope, SCOPE_OBJECT);
		if (obj_scope) {
			Node *var_decl = get_first_child_of_type(node, NODE_VAR_DECL);
			Node *ident = get_first_child_of_type(var_decl, NODE_IDENTIFER);
			Node *type_ann = get_next_sibling(ident);
			Type *field_type = resolve_type(sema->types, type_ann);
			char *name = slice_string(ident->data.literal);
			Symbol *sym =
				make_symbol(SYM_FIELD, name, field_type, node, obj_scope);
			// Add to object's symbol table, not current scope
			shput(obj_scope->symbols, name, sym);
		}
		return;
	}
	default: {
		break;
	}
	}
	for (Node *child = node->first_child; child; child = child->next_sibling) {
		build_scopes(sema, child);
	}
	if (node->type == NODE_FUNC_DECL || node->type == NODE_BLOCK ||
		node->type == NODE_METHOD_DECL || node->type == NODE_OBJ_DECL) {
		pop_scope(sema);
	}
}

// Add a helper for error reporting

void free_scopes(Node *root) {
	if (!root)
		return;
	for (Node *curr = get_first_child(root); curr;
		 curr = get_next_sibling(curr)) {
		free_scopes(curr);
	}
	if (root->scope) {
		if (root->scope->symbols) {
			free_symbol_table(&(root->scope->symbols));
		}
		free(root->scope);
		root->scope = NULL;
	}
}

void print_symbol_table(SymbolTable table) {

	int len = shlen(table);
	printf("                              SYMBOL TABLE                         "
		   "             \n");
	printf("║ %-25s │ %-12s │ %-30s │ %-20s ║\n", "Name", "Kind", "Type",
		   "Scope");

	for (int i = 0; i < len; i++) {
		Symbol *sym = table[i].value;
		if (!sym)
			continue;

		const char *kind_str;
		switch (sym->kind) {
		case SYM_VARIABLE:
			kind_str = "Variable";
			break;
		case SYM_CONSTANT:
			kind_str = "Constant";
			break;
		case SYM_SELF_PARAMETER:
		case SYM_PARAMETER:
			kind_str = "Parameter";
			break;
		case SYM_FIELD:
			kind_str = "Field";
			break;
		case SYM_METHOD:
			kind_str = "Method";
			break;
		case SYM_VARIANT:
			kind_str = "Variant";
			break;
		case SYM_TYPE:
			kind_str = "Type";
			break;
		case SYM_FUNCTION:
			kind_str = "Function";
			break;
		default:
			kind_str = "Unknown";
			break;
		}

		char type_name[20] = "?";
		if (sym->type && sym->type->name) {
			snprintf(type_name, sizeof(type_name), "%s", sym->type->name);
		}

		char scope_name[20] = "?";
		if (sym->scope) {
			switch (sym->scope->kind) {
			case SCOPE_GLOBAL:
				snprintf(scope_name, sizeof(scope_name), "global");
				break;
			case SCOPE_FUNCTION:
				snprintf(scope_name, sizeof(scope_name), "function");
				break;
			case SCOPE_BLOCK:
				snprintf(scope_name, sizeof(scope_name), "block");
				break;
			case SCOPE_OBJECT:
				snprintf(scope_name, sizeof(scope_name), "object");
				break;
			case SCOPE_ENUM:
				snprintf(scope_name, sizeof(scope_name), "enum");
				break;
			case SCOPE_UNION:
				snprintf(scope_name, sizeof(scope_name), "union");
				break;
			default:
				snprintf(scope_name, sizeof(scope_name), "?");
				break;
			}
		}

		printf("║ %-25s │ %-12s │ %-30s │ %-20s ║\n",
			   sym->name ? sym->name : "<unnamed>", kind_str, type_name,
			   scope_name);
	}
	printf("Total symbols: %d\n\n", len);
}
