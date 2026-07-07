#include "../sema/sema.h"

Type *validate_type(Sema *sema, Node *node) {
	if (!node)
		return NULL;

	switch (node->type) {
	// ============ DECLARATIONS ============
	case NODE_VAR_DECL: {
		Node *ident = get_first_child(node);
		Node *type_node = get_next_sibling(ident);
		Node *expr = get_next_sibling(type_node);

		Type *decl_type = resolve_type(sema->arena , sema->types, type_node);

		if (!decl_type) {
			type_error(node, "unknown type in variable declaration");
			return NULL;
		}
		node->resolved_type = decl_type;

		if (expr) {
			Type *expr_type = validate_type(sema, expr);
			if (!expr_type) {
				return NULL; // error already reported
			}
			if (expr_type != decl_type) {
				type_error(node,
						   "type mismatch in var declaration: expected "
						   "'%s', got '%s'",
						   decl_type->name, expr_type->name);
				return NULL;
			}
		}
		return decl_type;
	}

	case NODE_LET_DECL: {
		Node *ident = get_first_child(node);
		Node *type_node = get_next_sibling(ident);
		Node *expr = get_next_sibling(type_node);

		Type *decl_type = resolve_type(sema->arena , sema->types, type_node);
		if (!decl_type) {
			type_error(node, "unknown type in let declaration");
			return NULL;
		}
		node->resolved_type = decl_type;

		if (!expr) {
			type_error(node, "let declaration requires an initializer");
			return NULL;
		}

		Type *expr_type = validate_type(sema, expr);
		if (!expr_type) {
			return NULL;
		}
		if (expr_type != decl_type) {
			type_error(
				node,
				"type mismatch in let declaration: expected '%s', got '%s'",
				decl_type->name, expr_type->name);
			return NULL;
		}
		return decl_type;
	}

	case NODE_CONST_DECL: {
		Node *vis = get_first_child(node);
		Node *ident = get_next_sibling(vis);
		Node *type_node = get_next_sibling(ident);
		Node *expr = get_next_sibling(type_node);

		Type *decl_type = resolve_type(sema->arena , sema->types, type_node);
		if (!decl_type) {
			type_error(node, "unknown type in const declaration");
			return NULL;
		}
		node->resolved_type = decl_type;

		Type *expr_type = validate_type(sema, expr);
		if (!expr_type) {
			return NULL;
		}
		if (expr_type != decl_type) {
			type_error(node,
					   "type mismatch in const declaration: expected '%s', "
					   "got '%s'",
					   decl_type->name, expr_type->name);
			return NULL;
		}
		return decl_type;
	}

	// ============ ASSIGNMENTS ============
	case NODE_ASSIGNMENT_STMT: {
		Node *left = get_first_child(node);
		Node *op = get_next_sibling(left);
		Node *right = get_next_sibling(op);

		Type *left_type = validate_type(sema, left);
		if (!left_type) {
			return NULL;
		}

		// Check if left is assignable (not a constant)
		if (left->type == NODE_IDENTIFER) {
			char *name = slice_string(sema->arena, left->data.literal);
			Symbol *sym = lookup_symbol(sema->current_scope, name);
			
			if (sym && sym->kind == SYM_CONSTANT) {
				type_error(node, "cannot assign to constant '%s'", sym->name);
				return NULL;
			}
		}

		Type *right_type = validate_type(sema, right);
		if (!right_type) {
			return NULL;
		}

		if (left_type != right_type) {
			type_error(
				node,
				"type mismatch in assignment: left is '%s', right is '%s'",
				left_type->name, right_type->name);
			return NULL;
		}

		node->resolved_type = left_type;
		return left_type;
	}

	// ============ STATEMENTS ============
	case NODE_EXPRESSION_STMT: {
		Node *expr = get_first_child(node);
		if (expr) {
			return validate_type(sema, expr);
		}
		return shget(*sema->types, "void");
	}

	case NODE_RETURN_STMT: {
		Node *expr = get_first_child(node);
		Type *expr_type = NULL;

		if (expr) {
			expr_type = validate_type(sema, expr);
			if (!expr_type) {
				return NULL;
			}
		}

		// Find enclosing function/method
		Node *func = get_parent_of_type(node, NODE_FUNC_DECL);
		if (!func) {
			func = get_parent_of_type(node, NODE_METHOD_DECL);
		}

		if (func) {
			Node *return_type_node =
				get_first_child_of_type(func, NODE_RETURN_TYPE);
			if (return_type_node) {
				Node *type_node = get_first_child(return_type_node);
				Type *func_return_type = resolve_type(sema->arena , sema->types, type_node);
				if (!func_return_type) {
					type_error(node, "unknown return type in function");
					return NULL;
				}

				// void return type means no expression allowed
				if (func_return_type == shget(*sema->types, "void")) {
					if (expr) {
						type_error(node, "void function cannot return a value");
						return NULL;
					}
				} else {
					if (!expr) {
						type_error(node, "expected return value of type '%s'",
								   func_return_type->name);
						return NULL;
					}
					if (expr_type != func_return_type) {
						type_error(
							node,
							"return type mismatch: expected '%s', got '%s'",
							func_return_type->name, expr_type->name);
						return NULL;
					}
				}
			}
		}

		node->resolved_type =
			expr_type ? expr_type : shget(*sema->types, "void");
		return node->resolved_type;
	}

	case NODE_BLOCK: {
		Scope *saved_scope = sema->current_scope;
		sema->current_scope = node->scope ? node->scope : sema->current_scope;

		Type *last_type = shget(*sema->types, "void");
		for (Node *child = get_first_child(node); child;
			 child = get_next_sibling(child)) {
			Type *child_type = validate_type(sema, child);
			if (child_type && child_type != shget(*sema->types, "void")) {
				last_type = child_type;
			}
		}
		node->resolved_type = last_type;

		sema->current_scope = saved_scope; // restore on the way back out
		return last_type;
	}

	case NODE_CONDITIONAL_STMT: {
		Node *condition = get_first_child(node);
		Node *then_block = get_next_sibling(condition);
		Node *else_block = get_next_sibling(then_block);

		Type *cond_type = validate_type(sema, condition);
		if (!cond_type) {
			return NULL;
		}

		Type *bool_type = shget(*sema->types, "bool");
		if (cond_type != bool_type) {
			type_error(node, "if condition must be boolean, got '%s'",
					   cond_type->name);
			return NULL;
		}

		Type *then_type = validate_type(sema, then_block);
		Type *else_type = NULL;
		if (else_block) {
			else_type = validate_type(sema, else_block);
		}

		// If both branches exist, they should have the same type
		if (then_type && else_type && then_type != else_type) {
			type_error(node,
					   "if/else branches have different types: then='%s', "
					   "else='%s'",
					   then_type->name, else_type->name);
			return NULL;
		}

		node->resolved_type =
			then_type ? then_type : shget(*sema->types, "void");
		return node->resolved_type;
	}

		// ============ EXPRESSIONS ============
	case NODE_IDENTIFER: {
		char *name = slice_string(sema->arena, node->data.literal);
		Symbol *sym = lookup_symbol(sema->current_scope, name);
		if (!sym) {
			// Check if it's a type name
			Type *type = shget(*sema->types, name);
			if (type) {
				
				node->resolved_type = type;
				return type;
			}
			type_error(node, "undeclared identifier: %s", name);
			
			return NULL;
		}
		

		node->resolved_symbol = sym;
		node->resolved_type = sym->type;
		return sym->type;
	}

	// ============ LITERALS ============
	case NODE_DECIMAL_LITERAL:
	case NODE_HEX_LITERAL:
	case NODE_OCTAL_LITERAL:
	case NODE_BINARY_LITERAL: {
		Type *int_type = shget(*sema->types, "i32"); // default integer type
		node->resolved_type = int_type;
		return int_type;
	}

	case NODE_FLOAT_LITERAL: {
		Type *float_type = shget(*sema->types, "f32");
		node->resolved_type = float_type;
		return float_type;
	}

	case NODE_BOOLEAN_LITERAL: {
		Type *bool_type = shget(*sema->types, "bool");
		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_STRING_LITERAL: {
		Type *u8_type = shget(*sema->types, "u8");
		char *slice_name = malloc(strlen(u8_type->name) + 3);
		sprintf(slice_name, "[]%s", u8_type->name);
		Type *slice_type = shget(*sema->types, slice_name);
		free(slice_name);
		node->resolved_type = slice_type;
		return slice_type;
	}

	case NODE_CHAR_LITERAL: {
		Type *u8_type = shget(*sema->types, "u8");
		node->resolved_type = u8_type;
		return u8_type;
	}

	case NODE_ARRAY_LITERAL: {
		// Type will be determined by first element, all elements must match
		Node *first_expr = get_first_child(node);
		if (!first_expr) {
			type_error(node, "empty array literal");
			return NULL;
		}

		Type *elem_type = validate_type(sema, first_expr);
		if (!elem_type) {
			return NULL;
		}

		// Check all elements have the same type
		for (Node *expr = get_next_sibling(first_expr); expr;
			 expr = get_next_sibling(expr)) {
			Type *expr_type = validate_type(sema, expr);
			if (!expr_type) {
				return NULL;
			}
			if (expr_type != elem_type) {
				type_error(node,
						   "array literal elements must have same type: "
						   "expected '%s', got '%s'",
						   elem_type->name, expr_type->name);
				return NULL;
			}
		}

		// Build slice type: []T
		char *slice_name = malloc(strlen(elem_type->name) + 3);
		sprintf(slice_name, "[]%s", elem_type->name);
		Type *slice_type = shget(*sema->types, slice_name);
		free(slice_name);
		node->resolved_type = slice_type;
		return slice_type;
	}

	// ============ BINARY OPERATIONS ============
	case NODE_ADDITIVE_EXPRESSION:
	case NODE_MULTIPLICTIVE_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		// Both must be the same primitive numeric type
		if (left_type->kind != TYPE_PRIM || right_type->kind != TYPE_PRIM) {
			type_error(node,
					   "arithmetic requires numeric types, got '%s' and '%s'",
					   left_type->name, right_type->name);
			return NULL;
		}

		if (left_type != right_type) {
			type_error(node,
					   "arithmetic operands must have same type: got '%s' "
					   "and '%s'",
					   left_type->name, right_type->name);
			return NULL;
		}

		node->resolved_type = left_type;
		return left_type;
	}

	case NODE_EQUALITY_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		// Both must be the same type (any type is fine for equality)
		if (left_type != right_type) {
			type_error(
				node,
				"equality operands must have same type: got '%s' and '%s'",
				left_type->name, right_type->name);
			return NULL;
		}

		Type *bool_type = shget(*sema->types, "bool");
		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_RELATIONAL_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		// Both must be primitive numeric types
		if (left_type->kind != TYPE_PRIM || right_type->kind != TYPE_PRIM) {
			type_error(node,
					   "relational requires numeric types, got '%s' and '%s'",
					   left_type->name, right_type->name);
			return NULL;
		}

		if (left_type != right_type) {
			type_error(node,
					   "relational operands must have same type: got '%s' "
					   "and '%s'",
					   left_type->name, right_type->name);
			return NULL;
		}

		Type *bool_type = shget(*sema->types, "bool");
		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_LOGICAL_AND_EXPRESSION:
	case NODE_LOGICAL_OR_EXPRESSION: {
		Node *left = get_first_child(node);
		Node *right = get_next_sibling(left);

		Type *left_type = validate_type(sema, left);
		if (!left_type)
			return NULL;

		Type *right_type = validate_type(sema, right);
		if (!right_type)
			return NULL;

		Type *bool_type = shget(*sema->types, "bool");
		if (left_type != bool_type || right_type != bool_type) {
			type_error(node,
					   "logical operation requires boolean operands, got "
					   "'%s' and '%s'",
					   left_type->name, right_type->name);
			return NULL;
		}

		node->resolved_type = bool_type;
		return bool_type;
	}

	case NODE_UNARY_EXPRESSION: {
		Node *operand = get_first_child(node);
		Type *op_type = validate_type(sema, operand);
		if (!op_type)
			return NULL;

		Op op = node->data.op;

		// Check operation validity based on operator
		switch (op) {
		case minus:
			// Unary minus requires numeric type
			if (op_type->kind != TYPE_PRIM) {
				type_error(node, "unary minus requires numeric type, got '%s'",
						   op_type->name);
				return NULL;
			}
			break;
		case log_not:
			// Logical NOT requires boolean
			if (op_type != shget(*sema->types, "bool")) {
				type_error(node, "logical not requires boolean, got '%s'",
						   op_type->name);
				return NULL;
			}
			break;
		case bit_not:
			// Bitwise NOT requires integer type
			if (op_type->kind != TYPE_PRIM) {
				type_error(node, "bitwise not requires integer type, got '%s'",
						   op_type->name);
				return NULL;
			}
			// Check it's not a float
			char *type_name = op_type->name;
			if (type_name[0] == 'f') {
				type_error(node,
						   "bitwise not cannot be used on float, got '%s'",
						   op_type->name);
				return NULL;
			}
			break;
		case star: // dereference
			if (op_type->kind != TYPE_POINTER) {
				type_error(node, "dereference requires pointer type, got '%s'",
						   op_type->name);
				return NULL;
			}
			// Result type is the base type of the pointer
			node->resolved_type = op_type->data.pointer.base;
			return op_type->data.pointer.base;
		case and_perc: // address-of
			// Can take address of anything
			// Result type is pointer to operand type
			// We need to create or find the pointer type
			{
				char *ptr_name = malloc(strlen(op_type->name) + 2);
				sprintf(ptr_name, "*%s", op_type->name);
				Type *ptr_type = shget(*sema->types, ptr_name);
				free(ptr_name);
				if (!ptr_type) {
					// Pointer type should have been created during type
					// resolution
					type_error(node, "pointer type not found for '%s'",
							   op_type->name);
					return NULL;
				}
				node->resolved_type = ptr_type;
				return ptr_type;
			}
		default:
			type_error(node, "unknown unary operator");
			return NULL;
		}

		node->resolved_type = op_type;
		return op_type;
	}

	case NODE_INC_DEC: {
		Node *operand = get_first_child(node);
		Type *op_type = validate_type(sema, operand);
		if (!op_type)
			return NULL;

		// Increment/decrement requires numeric type
		if (op_type->kind != TYPE_PRIM) {
			type_error(node,
					   "increment/decrement requires numeric type, got '%s'",
					   op_type->name);
			return NULL;
		}

		// Also check it's assignable (variable, not constant)
		if (operand->type == NODE_IDENTIFER) {
			char *name = slice_string(sema->arena, operand->data.literal);
			Symbol *sym = lookup_symbol(sema->current_scope, name);
			
			if (sym && sym->kind == SYM_CONSTANT) {
				type_error(node, "cannot increment/decrement constant '%s'",
						   sym->name);
				return NULL;
			}
		}

		node->resolved_type = op_type;
		return op_type;
	}

	case NODE_ACCESS: {
		Node *obj = get_first_child(node);
		Node *field = get_next_sibling(obj);
		// We need to check if this obj is an instance identifier or the static
		// reference
		Type *obj_type = validate_type(sema, obj);
		if (!obj_type)
			return NULL;

		Type *actual_obj_type = obj_type;
		if (obj_type->kind == TYPE_POINTER)
			actual_obj_type = obj_type->data.pointer.base;

		if (actual_obj_type->kind != TYPE_OBJ) {
			type_error(node, "field access on non-object type '%s'",
					   actual_obj_type->name);
			return NULL;
		}

		char *field_name = slice_string(sema->arena, field->data.literal);

		Scope *obj_scope = actual_obj_type->scope;
		if (!obj_scope) {
			type_error(node, "object '%s' has no scope", actual_obj_type->name);
			
			return NULL;
		}

		Symbol *field_sym = lookup_symbol_local(obj_scope, field_name);

		if (!field_sym) {
			type_error(node, "field '%s' not found in object '%s'", field_name,
					   actual_obj_type->name);
			return NULL;
		}
		

		node->resolved_symbol = field_sym;
		node->resolved_type = field_sym->type;
		return field_sym->type;
	}

	case NODE_FUNC_CALL: {
		Node *callee = get_first_child(node);
		Node *args = get_next_sibling(callee);

		Type *func_type = validate_type(sema, callee);
		if (!func_type)
			return NULL;

		if (func_type->kind != TYPE_FUNC) {
			type_error(node, "calling non-function type '%s'", func_type->name);
			return NULL;
		}

		// Validate arguments match parameters
		size_t arg_count = get_child_count(args);
		if (arg_count != func_type->data.func.param_count) {
			type_error(node, "wrong number of arguments: expected %zu, got %zu",
					   func_type->data.func.param_count, arg_count);
			return NULL;
		}

		Node *arg = get_first_child(args);
		for (size_t i = 0; i < arg_count && arg; i++) {
			Type *arg_type = validate_type(sema, arg);
			if (!arg_type)
				return NULL;

			Type *param_type = func_type->data.func.params[i];
			if (arg_type != param_type) {
				type_error(
					node, "argument %zu type mismatch: expected '%s', got '%s'",
					i + 1, param_type->name, arg_type->name);
				return NULL;
			}
			arg = get_next_sibling(arg);
		}

		node->resolved_type = func_type->data.func.return_type;
		return func_type->data.func.return_type;
	}

	case NODE_INDEX: {
		// Array/slice indexing: arr[index] or slice[from:to]
		Node *obj = get_first_child(node);
		Node *index = get_next_sibling(obj);
		Node *slice_end = get_next_sibling(index); // optional for slicing

		Type *obj_type = validate_type(sema, obj);
		if (!obj_type)
			return NULL;

		// Object must be a slice or pointer to slice
		Type *actual_type = obj_type;
		if (obj_type->kind == TYPE_POINTER) {
			actual_type = obj_type->data.pointer.base;
		}

		if (actual_type->kind != TYPE_SLICE) {
			type_error(node, "indexing non-slice type '%s'", actual_type->name);
			return NULL;
		}

		if (slice_end) {
			// Slicing: arr[start:end]
			Type *start_type = validate_type(sema, index);
			if (!start_type)
				return NULL;

			Type *end_type = validate_type(sema, slice_end);
			if (!end_type)
				return NULL;

			// Both must be integers
			Type *int_type = shget(*sema->types, "i32");
			if (start_type != int_type || end_type != int_type) {
				type_error(node, "slice indices must be integers");
				return NULL;
			}

			// Result is same type as the slice
			node->resolved_type = obj_type;
			return obj_type;
		} else {
			// Single index: arr[i]
			Type *index_type = validate_type(sema, index);
			if (!index_type)
				return NULL;

			Type *int_type = shget(*sema->types, "i64");
			if (index_type != int_type) {
				type_error(node, "array index must be integer, got '%s'",
						   index_type->name);
				return NULL;
			}

			// Result is the element type of the slice
			node->resolved_type = actual_type->data.slice.element;
			return actual_type->data.slice.element;
		}
	}

	case NODE_CAST_EXPRESSION: {
		Node *type_node = get_first_child(node);
		Node *expr = get_next_sibling(type_node);

		Type *target_type = resolve_type(sema->arena , sema->types, type_node);
		if (!target_type) {
			type_error(node, "unknown target type in cast");
			return NULL;
		}

		Type *expr_type = validate_type(sema, expr);
		if (!expr_type)
			return NULL;

		// Casts require both types to be primitive numeric types for now
		if (expr_type->kind != TYPE_PRIM || target_type->kind != TYPE_PRIM) {
			type_error(node,
					   "cast only supports primitive types, got '%s' -> '%s'",
					   expr_type->name, target_type->name);
			return NULL;
		}

		node->resolved_type = target_type;
		return target_type;
	}

	case NODE_SIZE_OF_EXPRESSION: {
		Node *type_node = get_first_child(node);
		Type *type = resolve_type(sema->arena , sema->types, type_node);
		if (!type) {
			type_error(node, "unknown type in sizeof");
			return NULL;
		}

		// sizeof returns usize
		Type *usize_type = shget(*sema->types, "usize");
		node->resolved_type = usize_type;
		return usize_type;
	}

	case NODE_GROUPED_EXPRESSION: {
		Node *expr = get_first_child(node);
		Type *expr_type = validate_type(sema, expr);
		if (!expr_type)
			return NULL;

		node->resolved_type = expr_type;
		return expr_type;
	}

	// ============ LOOPS ============
	case NODE_WHILE_LOOP: {
		Node *condition = get_first_child(node);
		Node *block = get_next_sibling(condition);

		Type *cond_type = validate_type(sema, condition);
		if (!cond_type)
			return NULL;

		Type *bool_type = shget(*sema->types, "bool");
		if (cond_type != bool_type) {
			type_error(node, "while condition must be boolean, got '%s'",
					   cond_type->name);
			return NULL;
		}

		validate_type(sema, block);
		node->resolved_type = shget(*sema->types, "void");
		return node->resolved_type;
	}

	case NODE_FOR_LOOP: {
		Node *init = get_first_child(node);
		Node *condition = get_next_sibling(init);
		Node *update = get_next_sibling(condition);
		Node *block = get_next_sibling(update);

		validate_type(sema, init);

		Type *cond_type = validate_type(sema, condition);
		if (!cond_type)
			return NULL;

		Type *bool_type = shget(*sema->types, "bool");
		if (cond_type != bool_type) {
			type_error(node, "for condition must be boolean, got '%s'",
					   cond_type->name);
			return NULL;
		}

		validate_type(sema, update);
		validate_type(sema, block);

		node->resolved_type = shget(*sema->types, "void");
		return node->resolved_type;
	}

	case NODE_RANGE_FOR_LOOP: {
		Node *ident = get_first_child(node);
		Node *expr = get_next_sibling(ident);
		Node *block = get_next_sibling(expr);
		Type *range_type = validate_type(sema, expr);
		if (!range_type)
			return NULL;
		if (range_type->kind != TYPE_SLICE) {
			type_error(node, "range for requires slice type, got '%s'",
					   range_type->name);
			return NULL;
		}
		Type *elem_type = range_type->data.slice.element;
		char *name = slice_string(sema->arena, ident->data.literal);
		Symbol *sym = lookup_symbol(sema->current_scope, name);
		if (sym) {
			if (sym->type == NULL) {
				sym->type = elem_type; // first time: resolve the placeholder
			} else if (sym->type != elem_type) {
				type_error(
					node,
					"range variable type mismatch: expected '%s', got '%s'",
					elem_type->name, sym->type->name);
				return NULL;
			}
		}
		Scope *saved_scope = sema->current_scope;
		sema->current_scope = block->scope;
		validate_type(sema, block);
		sema->current_scope = saved_scope;
		node->resolved_type = shget(*sema->types, "void");
		return node->resolved_type;
	}
	case NODE_OBJ_DECL: {
		Node *visablity = get_first_child(node);
		Node *identfier = get_next_sibling(visablity);
		for (Node *child = get_first_child(identfier); child;
			 child = get_next_sibling(child)) {
			validate_type(sema, child);
		}
		node->resolved_type = shget(*sema->types, "void");
		return node->resolved_type;
	}
	case NODE_FUNC_DECL: {
		sema->current_scope = node->scope;
		validate_type(sema, get_first_child_of_type(node, NODE_BLOCK));
		return shget(*sema->types, "void");
		break;
	}
	case NODE_UNION_DECL:
	case NODE_ENUM_DECL:
	case NODE_MODULE_DECL: {
		return shget(*sema->types, "void");
		break;
	}

	default: {
		for (Node *child = get_first_child(node); child;
			 child = get_next_sibling(child)) {
			validate_type(sema, child);
		}
		node->resolved_type = shget(*sema->types, "void");
		return node->resolved_type;
	}
	}
}

void type_check(Sema *sema) {
	bool has_errors = false;
	for (Node *curr = get_first_child(sema->root); curr;
		 curr = get_next_sibling(curr)) {
		Type *type = validate_type(sema, curr);
		if (!type) {
			has_errors = true;
			print_node_inline(curr, sema->source);
			printf(" -> ERROR\n");
		}
	}
	if (has_errors) {
		printf("\nType checking completed with errors.\n");
	} else {
		printf("Type checking completed successfully.\n");
	}
}
