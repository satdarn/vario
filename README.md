(* 
   Vario Language Grammar
   A systems programming language with manual memory management,
   slices, tagged unions, and module-based organization
*)

(* ============================================================ *)
(* TOP LEVEL                                                      *)
(* ============================================================ *)

program = { top_level_decl } ;

top_level_decl = func_decl
               | obj_decl
               | enum_decl
               | union_decl
               | const_decl
               | module_decl
               | import_decl
               ;

(* ============================================================ *)
(* MODULES & VISIBILITY                                          *)
(* ============================================================ *)

module_decl = "module" identifier ";" ;
import_decl = "import" identifier ( "as" identifier )? ";" ;

visibility = [ "pub" ] ;

(* ============================================================ *)
(* FUNCTIONS                                                     *)
(* ============================================================ *)

func_decl = visibility "fn" identifier "(" parameter_list ")" return_type block ;
parameter_list = [ parameter { "," parameter } ] ;
parameter = identifier ":" type ;
return_type = "->" type ;  (* explicit - void is a type *)
block = "{" { statement } "}" ;

(* ============================================================ *)
(* TYPES                                                         *)
(* ============================================================ *)

type = primitive_type
     | pointer_type
     | slice_type
     | obj_type
     | enum_type
     | union_type
     ;

primitive_type = "u8" | "u32" | "u64" 
               | "i32" | "i64" 
               | "f32" | "f64" 
               | "bool"
               | "void"
               | "usize"
               | "isize"
               ;

pointer_type = "*" type ;
slice_type = "[" [ integer_literal ] "]" type ;  (* []T or [N]T *)
obj_type = identifier ;      (* reference to defined obj *)
enum_type = identifier ;      (* reference to defined enum *)
union_type = identifier ;      (* reference to defined union *)

(* ============================================================ *)
(* OBJECTS (STRUCTS WITH METHODS)                                *)
(* ============================================================ *)

obj_decl = visibility "obj" identifier "{" { obj_member } "}" ";" ;
obj_member = field_decl
           | method_decl
           | constructor_decl
           | destructor_decl
           ;

field_decl = visibility var_decl ";" ;
method_decl = visibility "fn" identifier "(" self_param "," parameter_list ")" return_type block ;
self_param = "self" ":" "*" obj_type ;  (* self is always a pointer *)
constructor_decl = "fn" "init" "(" parameter_list ")" "->" obj_type block ;
destructor_decl = "fn" "deinit" "(" "self" ":" "*" obj_type ")" block ;

(* ============================================================ *)
(* ENUMS                                                         *)
(* ============================================================ *)

enum_decl = visibility "enum" identifier "{" { enum_variant } "}" ";" ;
enum_variant = identifier ( "=" integer_literal )? [ "," ] ;

(* ============================================================ *)
(* TAGGED UNIONS                                                 *)
(* ============================================================ *)

union_decl = visibility "union" identifier "{" { union_variant } "}" ";" ;
union_variant = identifier ":" type [ "," ] ;

(* ============================================================ *)
(* CONSTANTS & VARIABLES                                         *)
(* ============================================================ *)

const_decl = visibility "const" identifier ":" type "=" expression ";" ;
var_decl = "var" identifier ":" type ( "=" expression )? ;
let_decl = "let" identifier ":" type ( "=" expression )? ;  (* immutable *)

(* ============================================================ *)
(* STATEMENTS                                                    *)
(* ============================================================ *)

statement = declaration_stmt
          | assignment_stmt
          | expression_stmt
          | conditional_stmt
          | loop_stmt
          | switch_stmt
          | return_stmt
          | break_stmt
          | continue_stmt
          | defer_stmt
          | block_stmt
          ;

declaration_stmt = ( var_decl | let_decl ) ";" ;
assignment_stmt = expression assignment_operator expression ";" ;
assignment_operator = "=" 
                    | "+=" | "-=" | "*=" | "/=" | "%=" 
                    | "&=" | "|=" | "^=" | "<<=" | ">>=" 
                    ;
expression_stmt = expression ";" ;
return_stmt = "return" expression? ";" ;
break_stmt = "break" ";" ;
continue_stmt = "continue" ";" ;
defer_stmt = "defer" statement ;
block_stmt = block ;

(* ============================================================ *)
(* CONDITIONALS                                                  *)
(* ============================================================ *)

conditional_stmt = "if" expression block ( "else" ( conditional_stmt | block ) )? ;

(* ============================================================ *)
(* LOOPS                                                         *)
(* ============================================================ *)

loop_stmt = while_loop | for_loop | range_for_loop ;

while_loop = "while" expression block ;

for_loop = "for" for_init? ";" expression? ";" for_update? block ;
for_init = var_decl | assignment_stmt | expression_stmt ;
for_update = assignment_stmt | expression_stmt | expression ;  (* no trailing semicolon *)

range_for_loop = "for" identifier ":" expression block ;  (* expression must be slice/array *)

(* ============================================================ *)
(* SWITCH STATEMENT                                             *)
(* ============================================================ *)

switch_stmt = "switch" expression "{" { switch_case } [ switch_default ] "}" ;
switch_case = "case" case_pattern { "," case_pattern } ":" { statement } ;
case_pattern = expression | literal | identifier ;  (* identifier as catch-all *)
switch_default = "default" ":" { statement } ;

(* ============================================================ *)
(* EXPRESSIONS (with precedence levels)                         *)
(* ============================================================ *)

expression = logical_or_expression ;

logical_or_expression = logical_and_expression { "||" logical_and_expression } ;
logical_and_expression = equality_expression { "&&" equality_expression } ;
equality_expression = relational_expression { ("==" | "!=") relational_expression } ;
relational_expression = additive_expression { ("<" | "<=" | ">" | ">=") additive_expression } ;
additive_expression = multiplicative_expression { ("+" | "-") multiplicative_expression } ;
multiplicative_expression = unary_expression { ("*" | "/" | "%") unary_expression } ;

unary_expression = ("-" | "!" | "~" | "*" | "&") unary_expression
                 | postfix_expression
                 ;

postfix_expression = primary_expression
                   { "(" argument_list ")"      (* function call *)
                   | "." identifier             (* field/method access *)
                   | "[" expression "]"         (* indexing *)
                   | "[" expression? ":" expression? "]"  (* slicing *)
                   | "++" | "--"                (* postfix inc/dec *)
                   } ;

primary_expression = literal
                   | identifier
                   | "(" expression ")"
                   | "sizeof" "(" type ")"
                   | "(" type ")" expression    (* cast *)
                   ;

argument_list = [ expression { "," expression } ] ;

(* ============================================================ *)
(* LITERALS                                                      *)
(* ============================================================ *)

literal = integer_literal
        | float_literal
        | string_literal
        | boolean_literal
        | array_literal
        ;

integer_literal = decimal_literal | hex_literal | octal_literal | binary_literal ;
float_literal = decimal_digits "." decimal_digits [ "e" [ "+" "-" ] decimal_digits ] ;
string_literal = '"' { string_char } '"' ;  (* yields []u8 *)
boolean_literal = "true" | "false" ;
array_literal = "[" [ expression { "," expression } ] "]" ;  (* slice literal *)

(* ============================================================ *)
(* LEXICAL ELEMENTS                                              *)
(* ============================================================ *)

identifier = letter { letter | digit | "_" } ;

(* Numbers *)
decimal_literal = decimal_digits ;
hex_literal = "0x" hex_digits ;
octal_literal = "0o" octal_digits ;
binary_literal = "0b" binary_digits ;

decimal_digits = digit { digit } ;
hex_digits = hex_digit { hex_digit } ;
octal_digits = octal_digit { octal_digit } ;
binary_digits = binary_digit { binary_digit } ;

(* Character classes *)
letter = "A" | "B" | ... | "Z" | "a" | "b" | ... | "z" ;
digit = "0" | "1" | ... | "9" ;
hex_digit = digit | "A" | "B" | "C" | "D" | "E" | "F" | "a" | "b" | "c" | "d" | "e" | "f" ;
octal_digit = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" ;
binary_digit = "0" | "1" ;

(* Strings - simplified *)
string_char = ? any character except " or newline, with escape sequences ? ;

(* Comments *)
comment = "//" { ? any character except newline ? } newline
        | "/*" { ? any character ? } "*/"
        ;

(* Whitespace *)
whitespace = { space | tab | newline } ;
