%{

#include "node.h"
#include <iostream>

#define YYSTYPE saru::Node

std::istream *global_input_stream;

#define YY_INPUT(buf, result, max_size, D)		\
  {							\
    int yyc= global_input_stream->get();					\
    result= (EOF == yyc) ? 0 : (*(buf)= yyc, 1);	\
    yyprintf((stderr, "<%c>", yyc));			\
  }

%}

comp_init = e:statementlist end-of-file {
    $$ = (node_global = e);
}

statementlist = s1:statement {
        $$.set(saru::NODE_STATEMENTS, s1);
        s1 = $$;
            }
    (
        - s2:statement {
            s1.push_child(s2);
            $$ = s1;
        }
    )* eat_terminator?
#   s1:statement {
#       $$.set(saru::NODE_STATEMENTS, s1);
#       s1 = $$;
#   }
#   (
#       eat_terminator s2:statement {
#           s1.push_child(s2);
#           $$ = s1;
#       }
#       | if_stmt
#   )* eat_terminator?

statement = b:bind_stmt eat_terminator { $$ = b; }
          | b:return_stmt eat_terminator { $$ = b; }
          | if_stmt
          | for_stmt
          | while_stmt
          | unless_stmt
          | die_stmt
          | funcdef - ';'*

return_stmt = 'return' ws e:expr { $$.set(saru::NODE_RETURN, e); }

die_stmt = 'die' ws e:expr eat_terminator { $$.set(saru::NODE_DIE, e); }

while_stmt = 'while' ws+ cond:expr - '{' - body:statementlist - '}' {
            $$.set(saru::NODE_WHILE, cond, body);
        }

for_stmt = 'for' - ( src:array_var | src:list_expr) - '{' - body:statementlist - '}' { $$.set(saru::NODE_FOR, src, body); }

unless_stmt = 'unless' - cond:expr - '{' - body:statementlist - '}' {
            $$.set(saru::NODE_UNLESS, cond, body);
        }

if_stmt = 'if' - if_cond:expr - '{' - if_body:statementlist - '}' {
            $$.set(saru::NODE_IF, if_cond, if_body);
            if_cond=$$;
        }
        (
            ws+ 'elsif' - elsif_cond:expr - '{' - elsif_body:statementlist - '}' {
                // elsif_body.change_type(saru::NODE_ELSIF);
                $$.set(saru::NODE_ELSIF, elsif_cond, elsif_body);
                // if_cond.push_child(elsif_cond);
                if_cond.push_child($$);
            }
        )*
        (
            ws+ 'else' ws+ - '{' - else_body:statementlist - '}' {
                else_body.change_type(saru::NODE_ELSE);
                if_cond.push_child(else_body);
            }
        )? { $$=if_cond; }

# FIXME: simplify the code
bind_stmt =
          e1:my - ':=' - e2:list_expr { $$.set(saru::NODE_BIND, e1, e2); }
        | e3:my - ':=' - e4:expr { $$.set(saru::NODE_BIND, e3, e4); }
        | e5:expr { $$ = e5; }

list_expr =
    (a:methodcall_expr { $$.set(saru::NODE_LIST, a); a = $$; }
        (- ','  - b:methodcall_expr { a.push_child(b); $$ = a; } )+
    ) { $$=a }

args =
    (
        s1:expr {
            $$.set(saru::NODE_ARGS, s1);
            s1 = $$;
        }
        ( ',' s2:expr {
            s1.push_child(s2);
            $$ = s1;
        } )*
    )
    | '' { $$.set_children(saru::NODE_ARGS); }

expr = bind_expr

bind_expr =
    (v:variable ':=' e:cmp_expr) { $$.set(saru::NODE_BIND, v, e); }
    | cmp_expr

cmp_expr = f1:methodcall_expr - (
          '==' - f2:methodcall_expr { $$.set(saru::NODE_EQ, f1, f2); f1=$$; }
        | '!=' - f2:methodcall_expr { $$.set(saru::NODE_NE, f1, f2); f1=$$; }
        | '<'  - f2:methodcall_expr { $$.set(saru::NODE_LT, f1, f2); f1=$$; }
        | '<=' - f2:methodcall_expr { $$.set(saru::NODE_LE, f1, f2); f1=$$; }
        | '>'  - f2:methodcall_expr { $$.set(saru::NODE_GT, f1, f2); f1=$$; }
        | '>=' - f2:methodcall_expr { $$.set(saru::NODE_GE, f1, f2); f1=$$; }
    )* {
        $$ = f1;
    }

methodcall_expr =
    a1:atpos_expr '.' a2:ident '(' - a3:args - ')' {
        $$.set(saru::NODE_METHODCALL, a1, a2, a3);
    }
    | atpos_expr

atpos_expr =
    f1:add_expr - '[' - f2:add_expr - ']' {
        $$.set(saru::NODE_ATPOS, f1, f2);
    }
    | add_expr

funcall =
    i:ident '(' - a:args - ')' {
        $$.set(saru::NODE_FUNCALL, i, a);
    }

add_expr =
    l:mul_expr (
          - '+' - r1:mul_expr {
            $$.set(saru::NODE_ADD, l, r1);
            l = $$;
          }
        | - '-' - r2:mul_expr {
            $$.set(saru::NODE_SUB, l, r2);
            l = $$;
          }
        | - '~' - r2:mul_expr {
            $$.set(saru::NODE_STRING_CONCAT, l, r2);
            l = $$;
          }
    )* {
        $$ = l;
    }

mul_expr =
    l:term - (
        '*' - r:term {
            $$.set(saru::NODE_MUL, l, r);
            l = $$;
        }
        | '/' - r:term {
            $$.set(saru::NODE_DIV, l, r);
            l = $$;
        }
        | '%' - r:term {
            $$.set(saru::NODE_MOD, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

term = value

ident = < [a-zA-Z] [a-zA-Z0-9]+ ( ( '_' | '-') [a-zA-Z0-9]+ )* > {
    $$.set_ident(yytext, yyleng);
} -

value = 
    ( '-' ( integer | dec_number) ) {
        $$.negate();
    }
    | integer
    | dec_number
    | string
    | '(' e:expr ')' { $$ = e; }
    | variable
    | array
    | funcall

funcdef =
    'sub' - i:ident - '(' - p:params - ')' - b:block {
        $$.set(saru::NODE_FUNC, i, p, b);
    }

params =
    (
        v:value { $$.set(saru::NODE_PARAMS, v); v=$$; }
        ( - ',' - v1:value { v.push_child(v1); $$=v; } )*
        { $$=v; }
    )
    | '' { $$.set_children(saru::NODE_PARAMS); }

block = 
    ('{' - s:statementlist - '}') { $$=s; }
    | ('{' - '}' ) { $$.set_children(saru::NODE_STATEMENTS); }

array =
    '[' e:expr { $$.set(saru::NODE_ARRAY, e); e=$$; } ( ',' e2:expr { e.push_child(e2); $$=e; } )* ']' { $$=e; }
    | '[' - ']' { $$.set_children(saru::NODE_ARRAY); }

my = 'my' ws v:variable { $$.set(saru::NODE_MY, v); }

variable = scalar | array_var

array_var = < '@' [a-zA-Z_] [a-zA-Z0-9]* > { $$.set_variable(yytext, yyleng); }

scalar = < '$' [a-zA-Z_] [a-zA-Z0-9]* > { assert(yyleng > 0); $$.set_variable(yytext, yyleng); }

#  <?MARKED('endstmt')>
#  <?terminator>
eat_terminator =
    ';' - | end-of-file

dec_number =
    <([.][0-9]+)> {
    $$.set_number(yytext);
}
    | <([0-9]+ '.' [0-9]+)> {
    $$.set_number(yytext);
}
    | <([0-9]+)> {
    $$.set_integer(yytext, 10);
}

integer =
    '0b' <[01]+> {
    $$.set_integer(yytext, 2);
}
    | '0x' <[0-9a-f]+> {
    $$.set_integer(yytext, 16);
}
    | '0o' <[0-7]+> {
    $$.set_integer(yytext, 8);
}

# Missing foo
string = '"' { $$.init_string(); } (
        < [^"]* > { $$.append_string(yytext, yyleng); }
        | '\a' { $$.append_string("\a", 1); }
        | '\b' { $$.append_string("\b", 1); }
        | '\t' { $$.append_string("\t", 1); }
        | '\r' { $$.append_string("\r", 1); }
        | '\n' { $$.append_string("\n", 1); }
        | '\"' { $$.append_string("\"", 1); }
    ) '"'

# TODO

# white space
ws = ' ' | '\f' | '\v' | '\t' | '\205' | '\240' | end-of-line
    | '#' [^\n]*
- = ws*
end-of-line = ( '\r\n' | '\n' | '\r' ) {
    line_number++;
}
end-of-file = !'\0'

%%
