%{

#include "node.h"

#define YYSTYPE saru::Node

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

# TODO
statement = b:bind_stmt eat_terminator { $$ = b; }
          | b:return_stmt eat_terminator { $$ = b; }
          | if_stmt
          | funcdef - ';'*

return_stmt = 'return' ws v:value { $$.set(saru::NODE_RETURN, v); }

if_stmt = 'if' - cond:expr - '{' - sl:statementlist - '}' {
            $$.set(saru::NODE_IF, cond, sl);
        }

bind_stmt = e1:my - ':=' - e2:expr { $$.set(saru::NODE_BIND, e1, e2); }
        | e3:expr { $$ = e3; }

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

expr = cmp_expr

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
    f1:funcall_expr - '[' - f2:funcall_expr - ']' {
        $$.set(saru::NODE_ATPOS, f1, f2);
    }
    | funcall_expr

funcall_expr =
    i:ident '(' - a:args - ')' - {
        $$.set(saru::NODE_FUNCALL, i, a);
    }
    | add_expr

add_expr =
    l:mul_expr - (
          '+' r1:mul_expr {
            $$.set(saru::NODE_ADD, l, r1);
            l = $$;
          }
        | '-' r2:mul_expr {
            $$.set(saru::NODE_SUB, l, r2);
            l = $$;
          }
        | '~' - r2:mul_expr {
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

funcdef =
    'sub' - i:ident - '(' - p:params - ')' - b:block {
        $$.set(saru::NODE_FUNC, i, p, b);
    }

params =
    (
        v:value { $$.set(saru::NODE_PARAMS, v); }
        ( - ',' - v1:value { v.push_child(v1); $$=v; } )*
        { $$=v; }
    )
    | '' { $$.set_children(saru::NODE_PARAMS); }

block = 
    '{' - statementlist - '}'

array =
    '[' e:expr { $$.set(saru::NODE_ARRAY, e); e=$$; } ( ',' e2:expr { e.push_child(e2); $$=e; } )* ']' { $$=e; }
    | '[' - ']' { $$.set_children(saru::NODE_ARRAY); }

my = 'my' ws v:variable { $$.set(saru::NODE_MY, v); }

variable = < '$' [a-zA-Z] [a-zA-Z0-9]* > { assert(yyleng > 0); $$.set_variable(yytext, yyleng); }

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
- = ws?
end-of-line = ( '\r\n' | '\n' | '\r' ) {
    line_number++;
}
end-of-file = !'\0'

%%
