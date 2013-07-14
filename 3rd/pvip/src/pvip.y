%{

#include "pvip.h"
#include <assert.h>

#define YYSTYPE PVIPNode*
#define YY_NAME(n) PVIP_##n
#define YY_XTYPE PVIPParserContext

/*


    A  Level             Examples
    =  =====             ========
    N  Terms             42 3.14 "eek" qq["foo"] $x :!verbose @$array
    L  Method postfix    .meth .+ .? .* .() .[] .{} .<> .«» .:: .= .^ .:
    N  Autoincrement     ++ --
    R  Exponentiation    **
    L  Symbolic unary    ! + - ~ ? | || +^ ~^ ?^ ^
    L  Multiplicative    * / % %% +& +< +> ~& ~< ~> ?& div mod gcd lcm
    L  Additive          + - +| +^ ~| ~^ ?| ?^
    L  Replication       x xx
    X  Concatenation     ~
    X  Junctive and      & (&) ∩
    X  Junctive or       | ^ (|) (^) ∪ (-)
    L  Named unary       temp let
    N  Structural infix  but does <=> leg cmp .. ..^ ^.. ^..^
    C  Chaining infix    != == < <= > >= eq ne lt le gt ge ~~ === eqv !eqv (<) (elem)
    X  Tight and         &&
    X  Tight or          || ^^ // min max
    R  Conditional       ?? !! ff fff
    R  Item assignment   = => += -= **= xx= .=
    L  Loose unary       so not
    X  Comma operator    , :
    X  List infix        Z minmax X X~ X* Xeqv ...
    R  List prefix       print push say die map substr ... [+] [*] any Z=
    X  Loose and         and andthen
    X  Loose or          or xor orelse
    X  Sequencer         <== ==> <<== ==>>
    N  Terminator        ; {...} unless extra ) ] }

*/

typedef struct {
    const char  *buf;
    size_t len;
    size_t pos;
} PVIPParserStringState;

typedef struct {
    int line_number;
    PVIPNode *root;
    int is_string;
    PVIPParserStringState *str;
    FILE *fp;
} PVIPParserContext;

static char PVIP_input(char *buf, YY_XTYPE D) {
    if (D.is_string) {
        if (D.str->len == D.str->pos) {
            return 0;
        } else {
            *buf = D.str->buf[D.str->pos];
            D.str->pos = D.str->pos+1;
            return 1;
        }
    } else {
        char c = fgetc(D.fp);
        *buf = c;
        return (c==EOF) ? 0 : 1;
    }
}

#define YY_INPUT(buf, result, max_size, D)		\
    result = PVIP_input(buf, D);

%}

comp_init = BOM? e:statementlist end-of-file {
    $$ = (G->data.root = e);
}
    | BOM? end-of-file { $$ = (G->data.root = PVIP_node_new_children(PVIP_NODE_NOP)); }

BOM='\357' '\273' '\277'

statementlist =
    (
        s1:statement {
            $$ = PVIP_node_new_children(PVIP_NODE_STATEMENTS);
            PVIP_node_push_child($$, s1);
            s1 = $$;
        }
        (
            - s2:statement {
                PVIP_node_push_child(s1, s2);
                $$=s1;
            }
        )* eat_terminator?
    )

# TODO
statement =
        - (
              use_stmt
            | if_stmt
            | for_stmt
            | while_stmt
            | unless_stmt
            | module_stmt
            | method_stmt
            | die_stmt
            | funcdef - ';'*
            | bl:block ';'* { $$ = PVIP_node_new_children1(PVIP_NODE_BLOCK, bl); }
            | b:normal_or_postfix_stmt { $$ = b; }
            | ';'+ {
                $$ = PVIP_node_new_children(PVIP_NODE_NOP);
            }
          )

normal_or_postfix_stmt =
    n:normal_stmt (
          ( ' '+ 'if' - cond_if:expr - eat_terminator ) { $$ = PVIP_node_new_children2(PVIP_NODE_IF, cond_if, n); }
        | ( ' '+ 'unless' - cond_unless:expr - eat_terminator ) { $$ = PVIP_node_new_children2(PVIP_NODE_UNLESS, cond_unless, n); }
        | ( ' '+ 'for' - cond_for:expr - eat_terminator ) { $$ = PVIP_node_new_children2(PVIP_NODE_FOR, cond_for, n); }
        | ( - eat_terminator ) { $$=n; }
    )

last_stmt = 'last' { $$ = PVIP_node_new_children(PVIP_NODE_LAST); }

next_stmt = 'next' { $$ = PVIP_node_new_children(PVIP_NODE_NEXT); }

method_stmt = 'method' ws i:ident p:paren_args - b:block { $$ = PVIP_node_new_children3(PVIP_NODE_METHOD, i, p, b); }

normal_stmt = return_stmt | last_stmt | next_stmt | expr

return_stmt = 'return' ws e:expr { $$ = PVIP_node_new_children1(PVIP_NODE_RETURN, e); }

module_stmt = 'module' ws pkg:pkg_name eat_terminator { $$ = PVIP_node_new_children1(PVIP_NODE_MODULE, pkg); }

use_stmt = 'use ' pkg:pkg_name eat_terminator { $$ = PVIP_node_new_children1(PVIP_NODE_USE, pkg); }

pkg_name = < [a-zA-Z] [a-zA-Z0-9]* ( '::' [a-zA-Z0-9]+ )* > {
    $$ = PVIP_node_new_string(PVIP_NODE_IDENT, yytext, yyleng);
}

die_stmt = 'die' ws e:expr eat_terminator { $$ = PVIP_node_new_children1(PVIP_NODE_DIE, e); }

while_stmt = 'while' ws+ cond:expr - '{' - body:statementlist - '}' {
            $$ = PVIP_node_new_children2(PVIP_NODE_WHILE, cond, body);
        }

for_stmt =
    'for' - src:expr - '{' - body:statementlist - '}' { $$ = PVIP_node_new_children2(PVIP_NODE_FOR, src, body); }
    | 'for' - src:expr - body:lambda { $$ = PVIP_node_new_children2(PVIP_NODE_FOR, src, body); }

unless_stmt = 'unless' - cond:expr - '{' - body:statementlist - '}' {
            $$ = PVIP_node_new_children2(PVIP_NODE_UNLESS, cond, body);
        }

if_stmt = 'if' - if_cond:expr - '{' - if_body:statementlist - '}' {
            $$ = PVIP_node_new_children2(PVIP_NODE_IF, if_cond, if_body);
            if_cond=$$;
        }
        (
            ws+ 'elsif' - elsif_cond:expr - '{' - elsif_body:statementlist - '}' {
                // elsif_body.change_type(PVIP_NODE_ELSIF);
                $$ = PVIP_node_new_children2(PVIP_NODE_ELSIF, elsif_cond, elsif_body);
                // if_cond.push_child(elsif_cond);
                PVIP_node_push_child(if_cond, $$);
            }
        )*
        (
            ws+ 'else' ws+ - '{' - else_body:statementlist - '}' {
                PVIP_node_change_type(else_body, PVIP_NODE_ELSE);
                PVIP_node_push_child(if_cond, else_body);
            }
        )? { $$=if_cond; }

paren_args = '(' - a:expr - ')' {
        if (a->type == PVIP_NODE_LIST) {
            $$ = a;
            PVIP_node_change_type($$, PVIP_NODE_ARGS);
        } else {
            $$ = PVIP_node_new_children1(PVIP_NODE_ARGS, a);
        }
    }
    | '(' - ')' { $$ = PVIP_node_new_children(PVIP_NODE_ARGS); }

bare_args = a:expr {
        if (a->type == PVIP_NODE_LIST) {
            $$ = a;
            PVIP_node_change_type($$, PVIP_NODE_ARGS);
        } else {
            $$ = PVIP_node_new_children1(PVIP_NODE_ARGS, a);
        }
    }

expr = sequencer_expr

# TODO
sequencer_expr = loose_or_expr

loose_or_expr =
    f1:loose_and_expr (
        - 'or' ![a-zA-Z0-9_] - f2:loose_and_expr { $$ = PVIP_node_new_children2(PVIP_NODE_LOGICAL_OR, f1, f2); f1=$$; }
        | - 'xor' ![a-zA-Z0-9_] - f2:loose_and_expr { $$ = PVIP_node_new_children2(PVIP_NODE_LOGICAL_XOR, f1, f2); f1=$$; }
    )* { $$=f1; }

loose_and_expr =
    f1:list_prefix_expr (
        - 'and' ![a-zA-Z0-9_]  - f2:list_prefix_expr { $$ = PVIP_node_new_children2(PVIP_NODE_LOGICAL_AND, f1, f2); f1=$$; }
    )* { $$=f1; }

list_prefix_expr =
    (v:lvalue - ':'? '=' - e:comma_operator_expr) { $$ = PVIP_node_new_children2(PVIP_NODE_BIND, v, e); }
    | comma_operator_expr

lvalue =
    my
    | v:variable { $$=v; } (
        '[' - e:expr - ']' { $$=PVIP_node_new_children2(PVIP_NODE_ATPOS, v, e); }
    )?

comma_operator_expr = a:loose_unary_expr { $$=a; } ( - ',' - b:loose_unary_expr {
        if (a->type==PVIP_NODE_LIST) {
            PVIP_node_push_child(a, b);
            $$=a;
        } else {
            $$ = PVIP_node_new_children2(PVIP_NODE_LIST, a, b);
            a=$$;
        }
    } )*

loose_unary_expr =
    'not' - f1:item_assignment_expr { $$ = PVIP_node_new_children1(PVIP_NODE_NOT, f1); }
    | f1:item_assignment_expr { $$=f1 }

item_assignment_expr =
    a:conditional_expr (
        - (
              '=>'  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_PAIR,             a, b); a=$$; }
            | '+='  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_ADD,      a, b); a=$$; }
            | '-='  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_SUB,      a, b); a=$$; }
            | '*='  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_MUL,      a, b); a=$$; }
            | '/='  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_DIV,      a, b); a=$$; }
            | '%='  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_MOD,      a, b); a=$$; }
            | '**=' - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_POW,      a, b); a=$$; }
            | '+|=' - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_BIN_OR,   a, b); a=$$; }
            | '+&=' - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_BIN_AND,  a, b); a=$$; }
            | '+^=' - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_BIN_XOR,  a, b); a=$$; }
            | '+<=' - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_BLSHIFT,  a, b); a=$$; }
            | '+>=' - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_BRSHIFT,  a, b); a=$$; }
            | '~='  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_CONCAT_S, a, b); a=$$; }
            | 'x='  - b:conditional_expr { $$ = PVIP_node_new_children2(PVIP_NODE_INPLACE_REPEAT_S, a, b); a=$$; }
        )
    )* { $$=a; }

conditional_expr = e1:tight_or - '??' - e2:tight_or - '!!' - e3:tight_or { $$ = PVIP_node_new_children3(PVIP_NODE_CONDITIONAL, e1, e2, e3); }
                | tight_or

tight_or = f1:tight_and (
        - '||' - f2:tight_and { $$ = PVIP_node_new_children2(PVIP_NODE_LOGICAL_OR, f1, f2); f1 = $$; }
        | - '^^' - f2:tight_and { $$ = PVIP_node_new_children2(PVIP_NODE_LOGICAL_XOR, f1, f2); f1 = $$; }
    )* { $$ = f1; }

tight_and = f1:chaining_infix_expr (
        - '&&' - f2:chaining_infix_expr { $$ = PVIP_node_new_children2(PVIP_NODE_LOGICAL_AND, f1, f2); f1 = $$; }
    )* { $$ = f1; }

#  C  Chaining infix    != == < <= > >= eq ne lt le gt ge ~~ === eqv !eqv (<) (elem)
chaining_infix_expr = f1:methodcall_expr { $$ = PVIP_node_new_children1(PVIP_NODE_CHAIN, f1); f1=$$; } (
          - '=='  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_EQ, f2);    PVIP_node_push_child(f1, tmp); }
        | - '!='  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_NE,    f2); PVIP_node_push_child(f1, tmp); }
        | - '<'   - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_LT,    f2); PVIP_node_push_child(f1, tmp); }
        | - '<='  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_LE,    f2); PVIP_node_push_child(f1, tmp); }
        | - '>'   - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_GT,    f2); PVIP_node_push_child(f1, tmp); }
        | - '>='  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_GE,    f2); PVIP_node_push_child(f1, tmp); }
        | - 'eqv' - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_EQV,   f2); PVIP_node_push_child(f1, tmp); }
        | - 'eq'  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_STREQ, f2); PVIP_node_push_child(f1, tmp); }
        | - 'ne'  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_STRNE, f2); PVIP_node_push_child(f1, tmp); }
        | - 'gt'  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_STRGT, f2); PVIP_node_push_child(f1, tmp); }
        | - 'ge'  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_STRGE, f2); PVIP_node_push_child(f1, tmp); }
        | - 'lt'  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_STRLT, f2); PVIP_node_push_child(f1, tmp); }
        | - 'le'  - f2:methodcall_expr { PVIPNode* tmp = PVIP_node_new_children1(PVIP_NODE_STRLE, f2); PVIP_node_push_child(f1, tmp); }
    )* { if (f1->children.size==1) { $$=f1->children.nodes[0]; } else { $$=f1; } }

methodcall_expr =
    a1:named_unary_expr (
        '.' a2:ident { $$ = PVIP_node_new_children2(PVIP_NODE_METHODCALL, a1, a2); a1=$$; }
        (
            a3:paren_args { PVIP_node_push_child(a1, a3) }
        )?
    )? { $$=a1; }

funcall =
    i:ident - a:paren_args {
        $$ = PVIP_node_new_children2(PVIP_NODE_FUNCALL, i, a);
    }
    | !reserved i:ident ws+ a:bare_args {
        $$ = PVIP_node_new_children2(PVIP_NODE_FUNCALL, i, a);
    }

#  L  Named unary       temp let
named_unary_expr =
    'abs' ws+ a:junctive_or_expr { $$ = PVIP_node_new_children1(PVIP_NODE_ABS, a); }
    | 'my' ws+ a:junctive_or_expr { $$ = PVIP_node_new_children1(PVIP_NODE_MY, a); }
    | 'our' ws+ a:junctive_or_expr { $$ = PVIP_node_new_children1(PVIP_NODE_OUR, a); }
    | !reserved i:ident ws+ a:bare_args { $$ = PVIP_node_new_children2(PVIP_NODE_FUNCALL, i, a); }
    | junctive_or_expr

junctive_or_expr =
    junctive_and_expr

junctive_and_expr =
    concatenation_expr

#  X  Concatenation     ~
concatenation_expr =
    replication_expr

#  L  Replication       x xx
# TODO: xx
replication_expr =
    l:additive_expr (
        - (
            'x'  ![a-zA-Z0-9_] - r:additive_expr {
                $$ = PVIP_node_new_children2(PVIP_NODE_REPEAT_S, l, r);
                l=$$;
            }
        )
    )* { $$=l; }

additive_expr =
    l:multiplicative_expr (
          - '+' - r1:multiplicative_expr {
            $$ = PVIP_node_new_children2(PVIP_NODE_ADD, l, r1);
            l = $$;
          }
        | - '-' - r2:multiplicative_expr {
            $$ = PVIP_node_new_children2(PVIP_NODE_SUB, l, r2);
            l = $$;
          }
        | - '~' - r2:multiplicative_expr {
            $$ = PVIP_node_new_children2(PVIP_NODE_STRING_CONCAT, l, r2);
            l = $$;
          }
        | - '+|' - r:exponentiation_expr {
            $$ = PVIP_node_new_children2(PVIP_NODE_BIN_OR, l, r);
            l = $$;
        }
        | - '+^' - r:exponentiation_expr {
            $$ = PVIP_node_new_children2(PVIP_NODE_BIN_XOR, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

multiplicative_expr =
    l:symbolic_unary (
        - '*' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(PVIP_NODE_MUL, l, r);
            l = $$;
        }
        | - '/' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(PVIP_NODE_DIV, l, r);
            l = $$;
        }
        | - '%' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(PVIP_NODE_MOD, l, r);
            l = $$;
        }
        | - '+&' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(PVIP_NODE_BIN_AND, l, r);
            l = $$;
        }
        | - '+>' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(PVIP_NODE_BRSHIFT, l, r);
            l = $$;
        }
        | - '+<' - r:symbolic_unary {
            $$ = PVIP_node_new_children2(PVIP_NODE_BLSHIFT, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

#  L  Symbolic unary    ! + - ~ ? | || +^ ~^ ?^ ^
symbolic_unary =
    '+' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(PVIP_NODE_UNARY_PLUS, f1); }
    | '-' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(PVIP_NODE_UNARY_MINUS, f1); }
    | '!' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(PVIP_NODE_NOT, f1); }
    | '+^' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(PVIP_NODE_UNARY_BITWISE_NEGATION, f1); }
    | '~' - f1:exponentiation_expr { $$ = PVIP_node_new_children1(PVIP_NODE_UNARY_TILDE, f1); }
    | exponentiation_expr

exponentiation_expr = 
    f1:autoincrement_expr (
        - '**' - f2:autoincrement_expr {
            $$ = PVIP_node_new_children2(PVIP_NODE_POW, f1, f2);
            f1=$$;
        }
    )* {
        $$=f1;
    }

# ++, -- is not supported yet
autoincrement_expr =
      '++' v:variable { $$ = PVIP_node_new_children1(PVIP_NODE_PREINC, v); }
    | '--' v:variable { $$ = PVIP_node_new_children1(PVIP_NODE_PREDEC, v); }
    | n:method_postfix_expr (
        '++' { $$ = PVIP_node_new_children1(PVIP_NODE_POSTINC, n); }
        | '--' { $$ = PVIP_node_new_children1(PVIP_NODE_POSTDEC, n); }
        | '' { $$=n; }
    )

method_postfix_expr =
          f1:term { $$=f1; } (
              '{' - k:term - '}' { $$ = PVIP_node_new_children2(PVIP_NODE_ATKEY, f1, k); }
            | '<' - k:ident - '>' { PVIP_node_change_type(k, PVIP_NODE_STRING); $$ = PVIP_node_new_children2(PVIP_NODE_ATKEY, f1, k); }
            | '[' - f2:term - ']' {
                $$ = PVIP_node_new_children2(PVIP_NODE_ATPOS, f1, f2);
            }
            | '.' f2:ident f3:paren_args {
                $$ = PVIP_node_new_children3(PVIP_NODE_METHODCALL, f1, f2, f3);
            }
            | a:paren_args { $$ = PVIP_node_new_children2(PVIP_NODE_FUNCALL, f1, a); }
          )?

term = 
    integer
    | dec_number
    | string
    | '(' - e:expr  - ')' { $$ = e; }
    | variable
    | '$?LINE' { $$ = PVIP_node_new_int(PVIP_NODE_INT, G->data.line_number); }
    | array
    | class
    | funcall
    | qw
    | twargs
    | hash
    | lambda
    | it_method
    | 'try' ws - b:block { $$ = PVIP_node_new_children1(PVIP_NODE_TRY, b); }
    | !reserved ident
    | '\\' t:term { $$ = PVIP_node_new_children1(PVIP_NODE_REF, t); }

reserved = 'class' | 'try'

# TODO optimizable
class =
    'class' ws+ b:block { $$ = PVIP_node_new_children2(PVIP_NODE_CLASS, PVIP_node_new_children(PVIP_NODE_NOP), b); }
    | 'class' ws+ i:ident - b:block { $$ = PVIP_node_new_children2(PVIP_NODE_CLASS, i, b); }


it_method = (
        '.' i:ident { $$ = PVIP_node_new_children1(PVIP_NODE_IT_METHODCALL, i); i=$$; }
        (
            a:paren_args { PVIP_node_push_child(i, a); }
        )?
    ) { $$=i; }

ident = < [a-zA-Z] [a-zA-Z0-9]* ( ( '_' | '-') [a-zA-Z0-9]+ )* > {
    $$ = PVIP_node_new_string(PVIP_NODE_IDENT, yytext, yyleng);
}


hash = '{' -
    p1:pair { $$ = PVIP_node_new_children1(PVIP_NODE_HASH, p1); p1=$$; } ( -  ',' - p2:pair { PVIP_node_push_child(p1, p2); $$=p1; } )*
    - '}' { $$=p1; }

pair = k:hash_key - '=>' - v:loose_unary_expr { $$ = PVIP_node_new_children2(PVIP_NODE_PAIR, k, v); }

hash_key =
    < [a-zA-Z0-9_]+ > { $$ = PVIP_node_new_string(PVIP_NODE_STRING, yytext, yyleng); }
    | string

twargs='@*ARGS' { $$ = PVIP_node_new_children(PVIP_NODE_CLARGS); }

qw =
    '<<' - qw_list - '>>'
    | '<' - qw_list - '>'

qw_list =
        a:qw_item { $$ = PVIP_node_new_children1(PVIP_NODE_LIST, a); a = $$; }
        ( ws+ b:qw_item { PVIP_node_push_child(a, b); $$ = a; } )*
        { $$=a; }

# I want to use [^ ] but greg does not support it...
# https://github.com/nddrylliog/greg/issues/12
qw_item = < [a-zA-Z0-9_]+ > { $$ = PVIP_node_new_string(PVIP_NODE_STRING, yytext, yyleng); }

# TODO optimize
funcdef =
    'sub' - i:ident - '(' - p:params? - ')' - b:block {
        if (!p) {
            p = PVIP_node_new_children(PVIP_NODE_PARAMS);
        }
        $$ = PVIP_node_new_children3(PVIP_NODE_FUNC, i, p, b);
    }
    | 'sub' - i:ident - b:block {
        PVIPNode* pp = PVIP_node_new_children(PVIP_NODE_PARAMS);
        $$ = PVIP_node_new_children3(PVIP_NODE_FUNC, i, pp, b);
    }

lambda =
    '->' - p:params? - b:block {
        if (!p) {
            p = PVIP_node_new_children(PVIP_NODE_PARAMS);
        }
        $$ = PVIP_node_new_children2(PVIP_NODE_LAMBDA, p, b);
    }

params =
    v:term { $$ = PVIP_node_new_children1(PVIP_NODE_PARAMS, v); v=$$; }
    ( - ',' - v1:term { PVIP_node_push_child(v, v1); $$=v; } )*
    { $$=v; }

block = 
    ('{' - s:statementlist - '}') { $$=s; }
    | ('{' - '}' ) { $$ = PVIP_node_new_children(PVIP_NODE_STATEMENTS); }

# XXX optimizable
array =
    '[' e:expr ']' { $$=e; PVIP_node_change_type($$, PVIP_NODE_ARRAY); }
    | '[' - ']' { $$ = PVIP_node_new_children(PVIP_NODE_ARRAY); }

my = 
    'my' ws+ v:variable { $$ = PVIP_node_new_children1(PVIP_NODE_MY, v); }
    | 'our' ws+ v:variable { $$ = PVIP_node_new_children1(PVIP_NODE_OUR, v); }

variable = scalar | array_var | hash_var

array_var = < '@' [a-zA-Z_] [a-zA-Z0-9]* > { $$ = PVIP_node_new_string(PVIP_NODE_VARIABLE, yytext, yyleng); }

hash_var = < '%' [a-zA-Z_] [a-zA-Z0-9]* > { $$ = PVIP_node_new_string(PVIP_NODE_VARIABLE, yytext, yyleng); }

scalar = < '$' [a-zA-Z_] [a-zA-Z0-9_]* > { assert(yyleng > 0); $$ = PVIP_node_new_string(PVIP_NODE_VARIABLE, yytext, yyleng); }

#  <?MARKED('endstmt')>
#  <?terminator>
eat_terminator =
    (';' -) | end-of-file

dec_number =
    <([.][0-9]+)> {
    $$ = PVIP_node_new_number(PVIP_NODE_NUMBER, yytext, yyleng);
}
    | <([0-9]+ '.' [0-9]+)> {
    $$ = PVIP_node_new_number(PVIP_NODE_NUMBER, yytext, yyleng);
}
    | <([0-9_]+)> {
    $$ = PVIP_node_new_intf(PVIP_NODE_INT, yytext, yyleng, 10);
}

integer =
    '0b' <[01_]+> {
    $$ = PVIP_node_new_intf(PVIP_NODE_INT, yytext, yyleng, 2);
}
    | '0d' <[0-9]+> {
    $$ = PVIP_node_new_intf(PVIP_NODE_INT, yytext, yyleng, 10);
}
    | '0x' <[0-9a-f_]+> {
    $$ = PVIP_node_new_intf(PVIP_NODE_INT, yytext, yyleng, 16);
}
    | '0o' <[0-7]+> {
    $$ = PVIP_node_new_intf(PVIP_NODE_INT, yytext, yyleng, 8);
}

string = dq_string | sq_string

dq_string_start='"' { $$ = PVIP_node_new_string(PVIP_NODE_STRING, "", 0); }

dq_string = s:dq_string_start { s = PVIP_node_new_string(PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; s=PVIP_node_append_string(s, "\n", 1); }
        | < [^"\\\n$]+ > { s=PVIP_node_append_string(s, yytext, yyleng); }
        | v:variable { s=PVIP_node_append_string_variable(s, v); }
        | esc 'a' { s=PVIP_node_append_string(s, "\a", 1); }
        | esc 'b' { s=PVIP_node_append_string(s, "\b", 1); }
        | esc 't' { s=PVIP_node_append_string(s, "\t", 1); }
        | esc 'r' { s=PVIP_node_append_string(s, "\r", 1); }
        | esc 'n' { s=PVIP_node_append_string(s, "\n", 1); }
        | esc '"' { s=PVIP_node_append_string(s, "\"", 1); }
        | esc '$' { s=PVIP_node_append_string(s, "\"", 1); }
        | ( esc 'x' (
                  '0'? < ( [a-fA-F0-9] [a-fA-F0-9] ) >
            | '[' '0'? < ( [a-fA-F0-9] [a-fA-F0-9] ) > ']' )
        ) {
            s=PVIP_node_append_string_from_hex(s, yytext, yyleng);
        }
        | esc 'o' < '0'? [0-7] [0-7] > {
            s=PVIP_node_append_string_from_oct(s, yytext, yyleng);
        }
        | esc 'o['
             '0'? < [0-7] [0-7] > { s=PVIP_node_append_string_from_oct(s, yytext, yyleng); } (
            ',' '0'? < [0-7] [0-7] > { s=PVIP_node_append_string_from_oct(s, yytext, yyleng); }
        )* ']'
        | esc esc { s=PVIP_node_append_string(s, "\\", 1); }
    )* '"' { $$=s; }

esc = '\\'

sq_string = "'" { $$ = PVIP_node_new_string(PVIP_NODE_STRING, "", 0); } (
        "\n" { G->data.line_number++; $$=PVIP_node_append_string($$, "\n", 1); }
        | < [^'\\\n]+ > { $$=PVIP_node_append_string($$, yytext, yyleng); }
        | esc "'" { $$=PVIP_node_append_string($$, "'", 1); }
        | esc esc { $$=PVIP_node_append_string($$, "\\", 1); }
        | < esc . > { $$=PVIP_node_append_string($$, yytext, yyleng); }
    )* "'"

comment =
    '#`[' [^\]]* ']'
    | '#`(' [^)]* ')'
    | '#`（' [^）]* '）'
    | '#`『' [^』]* '』'
    | '#`《' [^》]* '》'
    | '#' [^\n]* end-of-line

# white space
ws = 
    '\n=begin ' [a-z]+ '\n' ( !'=end ' [^\n]* '\n')* '=end ' [a-z]+ '\n'
    | '\n=begin END\n' .* | ' ' | '\f' | '\v' | '\t' | '\205' | '\240'
    | '\n=END\n' .*
    | end-of-line
    | comment

- = ws*

end-of-line = ( '\r\n' | '\n' | '\r' ) {
    G->data.line_number++;
}
end-of-file = !'\0'

%%

PVIPNode * PVIP_parse_string(const char *string, int len, int debug) {
    GREG g;
    YY_NAME(init)(&g);

#ifdef YY_DEBUG
    g.debug=debug;
#endif

    g.data.line_number = 1;
    g.data.is_string   = 1;
    g.data.str = malloc(sizeof(PVIPParserStringState));
    g.data.str->buf     = string;
    g.data.str->len     = len;
    g.data.str->pos     = 0;

    if (!YY_NAME(parse)(&g)) {
      fprintf(stderr, "** Syntax error at line %d\n", g.data.line_number);
      if (g.text[0]) {
        fprintf(stderr, "** near %s\n", g.text);
      }
      if (g.pos < g.limit || g.data.str->len==g.data.str->pos) {
        g.buf[g.limit]= '\0';
        fprintf(stderr, " before text \"");
        while (g.pos < g.limit) {
          if ('\n' == g.buf[g.pos] || '\r' == g.buf[g.pos]) break;
          fputc(g.buf[g.pos++], stderr);
        }
        if (g.pos == g.limit) {
            while (g.data.str->len!=g.data.str->pos) {
                char ch = g.data.str->buf[g.data.str->pos++];
                if (!ch || '\n' == ch || '\r' == ch) {
                    break;
                }
                fputc(ch, stderr);
            }
        }
        fputc('\"', stderr);
      }
      fprintf(stderr, "\n\n");
        free(g.data.str);
      return NULL;
    }
    if (g.limit!=g.pos) {
      printf("Syntax error! Around:\n");
      int i;
      for (i=0; g.limit!=g.pos && i<24; i++) {
        char ch = g.data.str->buf[g.pos++];
        if (ch) {
          printf("%c", ch);
        }
      }
      printf("\n");
      exit(1);
    }
    free(g.data.str);
    PVIPNode *root = g.data.root;
    assert(g.data.root);
    YY_NAME(deinit)(&g);
    return root;
}

/*
XXX Output error message to stderr is ugly.
XXX We need to add APIs for getting error message.
 */
PVIPNode * PVIP_parse_fp(FILE *fp, int debug) {
    GREG g;
    YY_NAME(init)(&g);

#ifdef YY_DEBUG
    g.debug=debug;
#endif

    g.data.line_number = 1;
    g.data.is_string   = 0;
    g.data.fp = fp;

    if (!YY_NAME(parse)(&g)) {
      fprintf(stderr, "** Syntax error at line %d\n", g.data.line_number);
      if (g.text[0]) {
        fprintf(stderr, "** near %s\n", g.text);
      }
      if (g.pos < g.limit || !feof(fp)) {
        g.buf[g.limit]= '\0';
        fprintf(stderr, " before text \"");
        while (g.pos < g.limit) {
          if ('\n' == g.buf[g.pos] || '\r' == g.buf[g.pos]) break;
          fputc(g.buf[g.pos++], stderr);
        }
        if (g.pos == g.limit) {
          int c;
          while (EOF != (c= fgetc(fp)) && '\n' != c && '\r' != c)
          fputc(c, stderr);
        }
        fputc('\"', stderr);
      }
      fprintf(stderr, "\n\n");
      return NULL;
    }
    if (!feof(fp)) {
      printf("Syntax error! Around:\n");
      int i;
      for (i=0; !feof(fp) && i<24; i++) {
        char ch = fgetc(fp);
        if (ch != EOF) {
          printf("%c", ch);
        }
      }
      printf("\n");
      exit(1);
    }
    free(g.data.str);
    PVIPNode *root = g.data.root;
    assert(g.data.root);
    YY_NAME(deinit)(&g);
    return root;
}

