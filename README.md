# General Idea 
ITS TO MAKE A COMPILER 

for what language?

I am scared about AI dependence... 

Languages:
- functions strongly typed parameters and return values
- math operations
- pointers, address of, dereference op, strongly typed
- strings are slices 
- obj (struct with methods, optional self pointer, constructors and destructors are explicitly called)
- primitives: u8 u32 u64, i32 i64, f32 f64
- arrays are slices 
- enum
- tagged union by default
- constants

program = top_level_decl | top_level_decl program
top_level_decl = func_decl | union_decl | obj_decl | const_decl 
func_decl = "fn" identifier "(" paramater_list ")" return_type "{" statement_list "}"
union_decl = "union" identifier "{" union_members "}" ";"
obj_decl = "obj" identifier "{" struct_members "}" ";"
const_decl = "const" type identifier "=" expression ";"
statement_list = e| statement | statement statement_list
statement = expression ";" 

