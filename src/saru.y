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

statementlist =
    (
        s1:statement {
            $$.set(saru::NODE_STATEMENTS, s1);
            s1 = $$;
        }
        (
            - s2:statement {
                s1.push_child(s2);
                $$ = s1;
            }
        )* eat_terminator?
    )

statement =
            e:postfix_if_stmt eat_terminator { $$ = e; }
          | e:postfix_unless_stmt eat_terminator { $$ = e; }
          | e:postfix_for_stmt eat_terminator { $$ = e; }
          | if_stmt
          | for_stmt
          | while_stmt
          | unless_stmt
          | die_stmt
          | funcdef - ';'*
          | block
          | b:normal_stmt - eat_terminator { $$ = b; }

normal_stmt = return_stmt | bind_stmt

return_stmt = 'return' ws e:expr { $$.set(saru::NODE_RETURN, e); }

die_stmt = 'die' ws e:expr eat_terminator { $$.set(saru::NODE_DIE, e); }

while_stmt = 'while' ws+ cond:expr - '{' - body:statementlist - '}' {
            $$.set(saru::NODE_WHILE, cond, body);
        }

for_stmt = 'for' - ( src:array_var | src:list_expr | src:qw | src:twargs ) - '{' - body:statementlist - '}' { $$.set(saru::NODE_FOR, src, body); }

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

postfix_if_stmt = body:normal_stmt - 'if' - cond:expr { $$.set(saru::NODE_IF, cond, body); }

postfix_unless_stmt = body:normal_stmt - 'unless' - cond:expr { $$.set(saru::NODE_UNLESS, cond, body); }

postfix_for_stmt = body:normal_stmt - 'for' - ( src:array_var | src:list_expr | src:qw | src:twargs ) { $$.set(saru::NODE_FOR, src, body); }

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
        ( - ',' - s2:expr {
            s1.push_child(s2);
            $$ = s1;
        } )*
    )
    | '' { $$.set_children(saru::NODE_ARGS); }

expr = sequencer_expr

# TODO
sequencer_expr = loose_or_expr

loose_or_expr =
    f1:loose_and_expr (
        - 'or' - f2:loose_and_expr { $$.set(saru::NODE_LOGICAL_AND, f1, f2); f1=$$; }
    )* { $$=f1; }

loose_and_expr =
    f1:list_prefix_expr (
        - 'and' - f2:list_prefix_expr { $$.set(saru::NODE_LOGICAL_AND, f1, f2); f1=$$; }
    )* { $$=f1; }

list_prefix_expr =
    (v:variable - ':=' - e:conditional_expr) { $$.set(saru::NODE_BIND, v, e); }
    | conditional_expr

conditional_expr = e1:tight_or - '??' - e2:tight_or - '!!' - e3:tight_or { $$.set(saru::NODE_CONDITIONAL, e1, e2, e3); }
                | tight_or

tight_or = f1:tight_and (
        - '||' - f2:tight_and { $$.set(saru::NODE_LOGICAL_OR, f1, f2); f1 = $$; }
    )* { $$ = f1; }

tight_and = f1:cmp_expr (
        - '&&' - f2:cmp_expr { $$.set(saru::NODE_LOGICAL_AND, f1, f2); f1 = $$; }
    )* { $$ = f1; }

cmp_expr = f1:methodcall_expr (
          - '==' - f2:methodcall_expr { $$.set(saru::NODE_EQ, f1, f2); f1=$$; }
        | - '!=' - f2:methodcall_expr { $$.set(saru::NODE_NE, f1, f2); f1=$$; }
        | - '<'  - f2:methodcall_expr { $$.set(saru::NODE_LT, f1, f2); f1=$$; }
        | - '<=' - f2:methodcall_expr { $$.set(saru::NODE_LE, f1, f2); f1=$$; }
        | - '>'  - f2:methodcall_expr { $$.set(saru::NODE_GT, f1, f2); f1=$$; }
        | - '>=' - f2:methodcall_expr { $$.set(saru::NODE_GE, f1, f2); f1=$$; }
        | - 'eq' - f2:methodcall_expr { $$.set(saru::NODE_STREQ, f1, f2); f1=$$; }
        | - 'ne' - f2:methodcall_expr { $$.set(saru::NODE_STRNE, f1, f2); f1=$$; }
        | - 'gt' - f2:methodcall_expr { $$.set(saru::NODE_STRGT, f1, f2); f1=$$; }
        | - 'ge' - f2:methodcall_expr { $$.set(saru::NODE_STRGE, f1, f2); f1=$$; }
        | - 'lt' - f2:methodcall_expr { $$.set(saru::NODE_STRLT, f1, f2); f1=$$; }
        | - 'le' - f2:methodcall_expr { $$.set(saru::NODE_STRLE, f1, f2); f1=$$; }
    )* {
        $$ = f1;
    }

methodcall_expr =
    a1:atpos_expr '.' a2:ident '(' - a3:args - ')' {
        $$.set(saru::NODE_METHODCALL, a1, a2, a3);
    }
    | a1:atpos_expr '.' a2:ident {
        $$.set(saru::NODE_METHODCALL, a1, a2);
    }
    | atpos_expr

atpos_expr =
    f1:not_expr - '[' - f2:not_expr - ']' {
        $$.set(saru::NODE_ATPOS, f1, f2);
    }
    | not_expr

funcall =
    (i:ident - '(' - a:args - ')') {
        $$.set(saru::NODE_FUNCALL, i, a);
    }
    | (i:ident ws+ a:args) {
        // funcall without parens.
        if (i.pv()=="return") {
            $$.set(saru::NODE_RETURN, a);
        } else {
            $$.set(saru::NODE_FUNCALL, i, a);
        }
    }

not_expr =
    ( '!' a:add_expr ) { $$.set(saru::NODE_NOT, a); }
    | add_expr

add_expr =
    l:multiplicative_expr (
          - '+' - r1:multiplicative_expr {
            $$.set(saru::NODE_ADD, l, r1);
            l = $$;
          }
        | - '-' - r2:multiplicative_expr {
            $$.set(saru::NODE_SUB, l, r2);
            l = $$;
          }
        | - '~' - r2:multiplicative_expr {
            $$.set(saru::NODE_STRING_CONCAT, l, r2);
            l = $$;
          }
        | - '+|' - r:exponentiation_expr {
            $$.set(saru::NODE_BIN_OR, l, r);
            l = $$;
        }
        | - '+^' - r:exponentiation_expr {
            $$.set(saru::NODE_BIN_XOR, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

multiplicative_expr =
    l:exponentiation_expr (
        - '*' - r:exponentiation_expr {
            $$.set(saru::NODE_MUL, l, r);
            l = $$;
        }
        | - '/' - r:exponentiation_expr {
            $$.set(saru::NODE_DIV, l, r);
            l = $$;
        }
        | - '%' - r:exponentiation_expr {
            $$.set(saru::NODE_MOD, l, r);
            l = $$;
        }
        | - '+&' - r:exponentiation_expr {
            $$.set(saru::NODE_BIN_AND, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

exponentiation_expr = 
    f1:autoincrement_expr (
        - '**' - f2:autoincrement_expr {
            $$.set(saru::NODE_POW, f1, f2);
            f1=$$;
        }
    )* {
        $$=f1;
    }

# ++, -- is not supported yet
autoincrement_expr = method_postfix_expr

method_postfix_expr = ( container:term '{' - k:term - '}' ) { $$.set(saru::NODE_ATKEY, container, k); }
           | ( container:term '<' - k:ident - '>' ) { k.change_type(saru::NODE_STRING); $$.set(saru::NODE_ATKEY, container, k); }
           | term

term = 
    ( '-' ( integer | dec_number) ) {
        $$.negate();
    }
    | integer
    | dec_number
    | string
    | '(' e:expr ')' { $$ = e; }
    | '(' l:list_expr ')' { $$ = l; }
    | variable
    | array
    | funcall
    | qw
    | twargs
    | hash

ident = < [a-zA-Z] [a-zA-Z0-9]* ( ( '_' | '-') [a-zA-Z0-9]+ )* > {
    $$.set_ident(yytext, yyleng);
}


hash = '{' -
    p1:pair { $$.set(saru::NODE_HASH, p1); p1=$$; } ( -  ',' - p2:pair { p1.push_child(p2); $$=p1; } )*
    - '}' { $$=p1; }

pair = k:hash_key - '=>' - v:expr { $$.set(saru::NODE_PAIR, k, v); }

hash_key =
    < [a-zA-Z0-9_]+ > { $$.set_string(yytext, yyleng); }
    | string

twargs='@*ARGS' { $$.set_clargs(); }

qw =
    '<<' -
        a:qw_item { $$.set(saru::NODE_LIST, a); a = $$; }
        ( ws+ b:qw_item { a.push_child(b); $$ = a; } )*
    - '>>' { $$=a }

# I want to use [^ ] but greg does not support it...
# https://github.com/nddrylliog/greg/issues/12
qw_item = < [a-zA-Z0-9_]+ > { $$.set_string(yytext, yyleng); }

funcdef =
    'sub' - i:ident - '(' - p:params - ')' - b:block {
        $$.set(saru::NODE_FUNC, i, p, b);
    }

params =
    (
        v:term { $$.set(saru::NODE_PARAMS, v); v=$$; }
        ( - ',' - v1:term { v.push_child(v1); $$=v; } )*
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
    (';' -) | end-of-file

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
