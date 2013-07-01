%{

#include "node.h"
#include <iostream>

#define YYSTYPE kiji::Node

#define YY_XTYPE kiji::ParserContext

namespace kiji {
  struct ParserContext {
    int line_number;
    kiji::Node *root;
    std::istream *input_stream;
  };
};

#define YY_INPUT(buf, result, max_size, D)		\
  {							\
    int yyc= D.input_stream->get();					\
    result= (EOF == yyc) ? 0 : (*(buf)= yyc, 1);	\
    yyprintf((stderr, "<%c>", yyc));			\
  }

// See http://perlcabal.org/syn/S03.html
//
//  A  Level             Examples
//  =  =====             ========
//  N  Terms             42 3.14 "eek" qq["foo"] $x :!verbose @$array
//  L  Method postfix    .meth .+ .? .* .() .[] .{} .<> .«» .:: .= .^ .:
//  N  Autoincrement     ++ --
//  R  Exponentiation    **
//  L  Symbolic unary    ! + - ~ ? | || +^ ~^ ?^ ^
//  L  Multiplicative    * / % %% +& +< +> ~& ~< ~> ?& div mod gcd lcm
//  L  Additive          + - +| +^ ~| ~^ ?| ?^
//  L  Replication       x xx
//  X  Concatenation     ~
//  X  Junctive and      & (&) ∩
//  X  Junctive or       | ^ (|) (^) ∪ (-)
//  L  Named unary       temp let
//  N  Structural infix  but does <=> leg cmp .. ..^ ^.. ^..^
//  C  Chaining infix    != == < <= > >= eq ne lt le gt ge ~~ === eqv !eqv (<) (elem)
//  X  Tight and         &&
//  X  Tight or          || ^^ // min max
//  R  Conditional       ?? !! ff fff
//  R  Item assignment   = => += -= **= xx= .=
//  L  Loose unary       so not
//  X  Comma operator    , :
//  X  List infix        Z minmax X X~ X* Xeqv ...
//  R  List prefix       print push say die map substr ... [+] [*] any Z=
//  X  Loose and         and andthen
//  X  Loose or          or xor orelse
//  X  Sequencer         <== ==> <<== ==>>
//  N  Terminator        ; {...} unless extra ) ] }

%}

comp_init = e:statementlist end-of-file {
    $$ = (*(G->data.root) = e);
}

statementlist =
    (
        s1:statement {
            $$.set(kiji::NODE_STATEMENTS, s1);
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
        - (
              use_stmt
            | if_stmt
            | for_stmt
            | while_stmt
            | unless_stmt
            | module_stmt
            | class_stmt
            | method_stmt
            | die_stmt
            | funcdef - ';'*
            | bl:block { $$.set(kiji::NODE_BLOCK, bl); }
            | b:normal_or_postfix_stmt { $$ = b; }
          )

normal_or_postfix_stmt =
    n:normal_stmt (
          ( ' '+ 'if' - cond_if:expr - eat_terminator ) { $$.set(kiji::NODE_IF, cond_if, n); }
        | ( ' '+ 'unless' - cond_unless:expr - eat_terminator ) { $$.set(kiji::NODE_UNLESS, cond_unless, n); }
        | ( ' '+ 'for' - cond_for:expr - eat_terminator ) { $$.set(kiji::NODE_FOR, cond_for, n); }
        | ( - eat_terminator ) { $$=n; }
    )

last_stmt = 'last' { $$.set_children(kiji::NODE_LAST); }

next_stmt = 'next' { $$.set_children(kiji::NODE_NEXT); }

class_stmt = 'class' ws i:ident - b:block { $$.set(kiji::NODE_CLASS, i, b); }

method_stmt = 'method' ws i:ident p:paren_args - b:block { $$.set(kiji::NODE_METHOD, i, p, b); }

normal_stmt = return_stmt | last_stmt | next_stmt | expr

return_stmt = 'return' ws e:expr { $$.set(kiji::NODE_RETURN, e); }

module_stmt = 'module' ws pkg:pkg_name eat_terminator { $$.set(kiji::NODE_MODULE, pkg); }

use_stmt = 'use ' pkg:pkg_name eat_terminator { $$.set(kiji::NODE_USE, pkg); }

pkg_name = < [a-zA-Z] [a-zA-Z0-9]* ( '::' [a-zA-Z0-9]+ )* > {
    $$.set_ident(yytext, yyleng);
}

die_stmt = 'die' ws e:expr eat_terminator { $$.set(kiji::NODE_DIE, e); }

while_stmt = 'while' ws+ cond:expr - '{' - body:statementlist - '}' {
            $$.set(kiji::NODE_WHILE, cond, body);
        }

for_stmt =
    'for' - src:expr - '{' - body:statementlist - '}' { $$.set(kiji::NODE_FOR, src, body); }
    | 'for' - src:expr - body:lambda { $$.set(kiji::NODE_FOR, src, body); }

unless_stmt = 'unless' - cond:expr - '{' - body:statementlist - '}' {
            $$.set(kiji::NODE_UNLESS, cond, body);
        }

if_stmt = 'if' - if_cond:expr - '{' - if_body:statementlist - '}' {
            $$.set(kiji::NODE_IF, if_cond, if_body);
            if_cond=$$;
        }
        (
            ws+ 'elsif' - elsif_cond:expr - '{' - elsif_body:statementlist - '}' {
                // elsif_body.change_type(kiji::NODE_ELSIF);
                $$.set(kiji::NODE_ELSIF, elsif_cond, elsif_body);
                // if_cond.push_child(elsif_cond);
                if_cond.push_child($$);
            }
        )*
        (
            ws+ 'else' ws+ - '{' - else_body:statementlist - '}' {
                else_body.change_type(kiji::NODE_ELSE);
                if_cond.push_child(else_body);
            }
        )? { $$=if_cond; }

paren_args = '(' - a:expr - ')' {
        if (a.type() == kiji::NODE_LIST) {
            $$ = a;
            $$.change_type(kiji::NODE_ARGS);
        } else {
            $$.set(kiji::NODE_ARGS, a);
        }
    }
    | '(' - ')' { $$.set_children(kiji::NODE_ARGS); }

bare_args = a:expr {
        if (a.type() == kiji::NODE_LIST) {
            $$ = a;
            $$.change_type(kiji::NODE_ARGS);
        } else {
            $$.set(kiji::NODE_ARGS, a);
        }
    }

expr = sequencer_expr

# TODO
sequencer_expr = loose_or_expr

loose_or_expr =
    f1:loose_and_expr (
        - 'or' - f2:loose_and_expr { $$.set(kiji::NODE_LOGICAL_AND, f1, f2); f1=$$; }
    )* { $$=f1; }

loose_and_expr =
    f1:list_prefix_expr (
        - 'and' - f2:list_prefix_expr { $$.set(kiji::NODE_LOGICAL_AND, f1, f2); f1=$$; }
    )* { $$=f1; }

list_prefix_expr =
    (v:variable - ':'? '=' - e:comma_operator_expr) { $$.set(kiji::NODE_BIND, v, e); }
    | (v:my - ':'? '=' - e:comma_operator_expr) { $$.set(kiji::NODE_BIND, v, e); }
    | comma_operator_expr


comma_operator_expr = a:loose_unary_expr { $$=a; } ( - ',' - b:loose_unary_expr {
        if (a.type()==kiji::NODE_LIST) {
            a.push_child(b);
            $$=a;
        } else {
            $$.set(kiji::NODE_LIST, a, b);
            a=$$;
        }
    } )*

loose_unary_expr =
    'not' - f1:item_assignment_expr { $$.set(kiji::NODE_NOT, f1); }
    | f1:item_assignment_expr { $$=f1 }

item_assignment_expr =
    a:conditional_expr (
        '=>' b:conditional_expr { $$.set(kiji::NODE_PAIR, a, b); a=$$; }
    )* { $$=a; }

conditional_expr = e1:tight_or - '??' - e2:tight_or - '!!' - e3:tight_or { $$.set(kiji::NODE_CONDITIONAL, e1, e2, e3); }
                | tight_or

tight_or = f1:tight_and (
        - '||' - f2:tight_and { $$.set(kiji::NODE_LOGICAL_OR, f1, f2); f1 = $$; }
    )* { $$ = f1; }

tight_and = f1:cmp_expr (
        - '&&' - f2:cmp_expr { $$.set(kiji::NODE_LOGICAL_AND, f1, f2); f1 = $$; }
    )* { $$ = f1; }

cmp_expr = f1:methodcall_expr (
          - '==' - f2:methodcall_expr { $$.set(kiji::NODE_EQ, f1, f2); f1=$$; }
        | - '!=' - f2:methodcall_expr { $$.set(kiji::NODE_NE, f1, f2); f1=$$; }
        | - '<'  - f2:methodcall_expr { $$.set(kiji::NODE_LT, f1, f2); f1=$$; }
        | - '<=' - f2:methodcall_expr { $$.set(kiji::NODE_LE, f1, f2); f1=$$; }
        | - '>'  - f2:methodcall_expr { $$.set(kiji::NODE_GT, f1, f2); f1=$$; }
        | - '>=' - f2:methodcall_expr { $$.set(kiji::NODE_GE, f1, f2); f1=$$; }
        | - 'eq' - f2:methodcall_expr { $$.set(kiji::NODE_STREQ, f1, f2); f1=$$; }
        | - 'ne' - f2:methodcall_expr { $$.set(kiji::NODE_STRNE, f1, f2); f1=$$; }
        | - 'gt' - f2:methodcall_expr { $$.set(kiji::NODE_STRGT, f1, f2); f1=$$; }
        | - 'ge' - f2:methodcall_expr { $$.set(kiji::NODE_STRGE, f1, f2); f1=$$; }
        | - 'lt' - f2:methodcall_expr { $$.set(kiji::NODE_STRLT, f1, f2); f1=$$; }
        | - 'le' - f2:methodcall_expr { $$.set(kiji::NODE_STRLE, f1, f2); f1=$$; }
    )* {
        $$ = f1;
    }

methodcall_expr =
    a1:named_unary_expr (
        '.' a2:ident { $$.set(kiji::NODE_METHODCALL, a1, a2); a1=$$; }
        (
            a3:paren_args { a1.push_child(a3) }
        )?
    )? { $$=a1; }

funcall =
    i:ident - a:paren_args {
        $$.set(kiji::NODE_FUNCALL, i, a);
    }

#  L  Named unary       temp let
named_unary_expr =
    'abs' ws+ a:junctive_or_expr { $$.set(kiji::NODE_ABS, a); }
    | i:ident ws+ a:bare_args { $$.set(kiji::NODE_FUNCALL, i, a); }
    | junctive_or_expr

junctive_or_expr =
    junctive_and_expr

junctive_and_expr =
    concatenation_expr

#  X  Concatenation     ~
concatenation_expr =
    replication_expr

#  L  Replication       x xx
replication_expr =
    additive_expr

additive_expr =
    l:multiplicative_expr (
          - '+' - r1:multiplicative_expr {
            $$.set(kiji::NODE_ADD, l, r1);
            l = $$;
          }
        | - '-' - r2:multiplicative_expr {
            $$.set(kiji::NODE_SUB, l, r2);
            l = $$;
          }
        | - '~' - r2:multiplicative_expr {
            $$.set(kiji::NODE_STRING_CONCAT, l, r2);
            l = $$;
          }
        | - '+|' - r:exponentiation_expr {
            $$.set(kiji::NODE_BIN_OR, l, r);
            l = $$;
        }
        | - '+^' - r:exponentiation_expr {
            $$.set(kiji::NODE_BIN_XOR, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

multiplicative_expr =
    l:symbolic_unary (
        - '*' - r:symbolic_unary {
            $$.set(kiji::NODE_MUL, l, r);
            l = $$;
        }
        | - '/' - r:symbolic_unary {
            $$.set(kiji::NODE_DIV, l, r);
            l = $$;
        }
        | - '%' - r:symbolic_unary {
            $$.set(kiji::NODE_MOD, l, r);
            l = $$;
        }
        | - '+&' - r:symbolic_unary {
            $$.set(kiji::NODE_BIN_AND, l, r);
            l = $$;
        }
        | - '+>' - r:symbolic_unary {
            $$.set(kiji::NODE_BRSHIFT, l, r);
            l = $$;
        }
        | - '+<' - r:symbolic_unary {
            $$.set(kiji::NODE_BLSHIFT, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

#  L  Symbolic unary    ! + - ~ ? | || +^ ~^ ?^ ^
symbolic_unary =
    '+' - f1:exponentiation_expr { $$.set(kiji::NODE_UNARY_PLUS, f1); }
    | '-' - f1:exponentiation_expr { $$.set(kiji::NODE_UNARY_MINUS, f1); }
    | '!' - f1:exponentiation_expr { $$.set(kiji::NODE_NOT, f1); }
    | '+^' - f1:exponentiation_expr { $$.set(kiji::NODE_UNARY_BITWISE_NEGATION, f1); }
    | exponentiation_expr

exponentiation_expr = 
    f1:autoincrement_expr (
        - '**' - f2:autoincrement_expr {
            $$.set(kiji::NODE_POW, f1, f2);
            f1=$$;
        }
    )* {
        $$=f1;
    }

# ++, -- is not supported yet
autoincrement_expr =
      '++' v:variable { $$.set(kiji::NODE_PREINC, v); }
    | '--' v:variable { $$.set(kiji::NODE_PREDEC, v); }
    | n:method_postfix_expr (
        '++' { $$.set(kiji::NODE_POSTINC, n); }
        | '--' { $$.set(kiji::NODE_POSTDEC, n); }
        | '' { $$=n; }
    )

# FIXME: optimizable
method_postfix_expr = ( container:term '{' - k:term - '}' ) { $$.set(kiji::NODE_ATKEY, container, k); }
           | ( container:term '<' - k:ident - '>' ) { k.change_type(kiji::NODE_STRING); $$.set(kiji::NODE_ATKEY, container, k); }
           | f1:term - '[' - f2:term - ']' {
                $$.set(kiji::NODE_ATPOS, f1, f2);
            }
           | ( container:term a:paren_args ) { $$.set(kiji::NODE_FUNCALL, container, a); }
           | term

term = 
    integer
    | dec_number
    | string
    | '(' - e:expr  - ')' { $$ = e; }
    | variable
    | '$?LINE' { $$.set_integer(G->data.line_number); }
    | array
    | funcall
    | qw
    | twargs
    | hash
    | lambda
    | it_method

it_method = (
        '.' i:ident { $$.set(kiji::NODE_IT_METHODCALL, i); i=$$; }
        (
            a:paren_args { i.push_child(a); }
        )?
    ) { $$=i; }

ident = < [a-zA-Z] [a-zA-Z0-9]* ( ( '_' | '-') [a-zA-Z0-9]+ )* > {
    $$.set_ident(yytext, yyleng);
}


hash = '{' -
    p1:pair { $$.set(kiji::NODE_HASH, p1); p1=$$; } ( -  ',' - p2:pair { p1.push_child(p2); $$=p1; } )*
    - '}' { $$=p1; }

pair = k:hash_key - '=>' - v:loose_unary_expr { $$.set(kiji::NODE_PAIR, k, v); }

hash_key =
    < [a-zA-Z0-9_]+ > { $$.set_string(yytext, yyleng); }
    | string

twargs='@*ARGS' { $$.set_clargs(); }

qw =
    '<<' - qw_list - '>>'
    | '<' - qw_list - '>'

qw_list =
        a:qw_item { $$.set(kiji::NODE_LIST, a); a = $$; }
        ( ws+ b:qw_item { a.push_child(b); $$ = a; } )*
        { $$=a; }

# I want to use [^ ] but greg does not support it...
# https://github.com/nddrylliog/greg/issues/12
qw_item = < [a-zA-Z0-9_]+ > { $$.set_string(yytext, yyleng); }

funcdef =
    'sub' - i:ident - '(' - p:params? - ')' - b:block {
        if (p.is_undefined()) {
            p.set_children(kiji::NODE_PARAMS);
        }
        $$.set(kiji::NODE_FUNC, i, p, b);
    }

lambda =
    '->' - p:params? - b:block {
        if (p.is_undefined()) {
            p.set_children(kiji::NODE_PARAMS);
        }
        $$.set(kiji::NODE_LAMBDA, p, b);
    }

params =
    v:term { $$.set(kiji::NODE_PARAMS, v); v=$$; }
    ( - ',' - v1:term { v.push_child(v1); $$=v; } )*
    { $$=v; }

block = 
    ('{' - s:statementlist - '}') { $$=s; }
    | ('{' - '}' ) { $$.set_children(kiji::NODE_STATEMENTS); }

# XXX optimizable
array =
    '[' e:expr ']' { $$=e; $$.change_type(kiji::NODE_ARRAY); }
    | '[' - ']' { $$.set_children(kiji::NODE_ARRAY); }

my = 'my' ws v:variable { $$.set(kiji::NODE_MY, v); }

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
    | <([0-9_]+)> {
    $$.set_integer(yytext, yyleng, 10);
}

integer =
    '0b' <[01_]+> {
    $$.set_integer(yytext, yyleng, 2);
}
    | '0d' <[0-9]+> {
    $$.set_integer(yytext, yyleng, 10);
}
    | '0x' <[0-9a-f_]+> {
    $$.set_integer(yytext, yyleng, 16);
}
    | '0o' <[0-7]+> {
    $$.set_integer(yytext, yyleng, 8);
}

string = dq_string | sq_string

dq_string_start='"' { $$.init_string(); }

dq_string = s:dq_string_start { s.init_string(); } (
        "\n" { G->data.line_number++; s.append_string("\n", 1); }
        | < [^"\\\n$]+ > { s.append_string(yytext, yyleng); }
        | v:variable { s.append_string_variable(v); }
        | esc 'a' { s.append_string("\a", 1); }
        | esc 'b' { s.append_string("\b", 1); }
        | esc 't' { s.append_string("\t", 1); }
        | esc 'r' { s.append_string("\r", 1); }
        | esc 'n' { s.append_string("\n", 1); }
        | esc '"' { s.append_string("\"", 1); }
        | esc '$' { s.append_string("\"", 1); }
        | ( esc 'x' (
                  '0'? < ( [a-fA-F0-9] [a-fA-F0-9] ) >
            | '[' '0'? < ( [a-fA-F0-9] [a-fA-F0-9] ) > ']' )
        ) {
            s.append_string_from_hex(yytext, yyleng);
        }
        | esc 'o' < '0'? [0-7] [0-7] > {
            s.append_string_from_oct(yytext, yyleng);
        }
        | esc 'o['
             '0'? < [0-7] [0-7] > { s.append_string_from_oct(yytext, yyleng); } (
            ',' '0'? < [0-7] [0-7] > { s.append_string_from_oct(yytext, yyleng); }
        )* ']'
        | esc esc { s.append_string("\\", 1); }
    )* '"' { $$=s; }

esc = '\\'

sq_string = "'" { $$.init_string(); } (
        "\n" { G->data.line_number++; $$.append_string("\n", 1); }
        | < [^'\\\n]+ > { $$.append_string(yytext, yyleng); }
        | esc "'" { $$.append_string("'", 1); }
        | esc esc { $$.append_string("\\", 1); }
        | < esc . > { $$.append_string(yytext, yyleng); }
    )* "'"

# TODO

comment= '#' [^\n]* end-of-line

# white space
ws = ' ' | '\f' | '\v' | '\t' | '\205' | '\240' | end-of-line
    | comment
- = ws*
end-of-line = ( '\r\n' | '\n' | '\r' ) {
    G->data.line_number++;
}
end-of-file = !'\0'

%%
