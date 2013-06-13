%{

%}

comp_init = e:statementlist end-of-file {
    $$ = (node_global = e);
}

statementlist =
    s1:statement {
        $$.set(NQPC_NODE_STATEMENTS, s1);
        s1 = $$;
    }
    ( eat_terminator s2:statement {
        s1.push_child(s2);
        $$ = s1;
    } )* eat_terminator?

# TODO
statement = e:expr ws* { $$ = e; }

args =
    (
        s1:expr {
            $$.set(NQPC_NODE_ARGS, s1);
            s1 = $$;
        }
        ( ',' s2:expr {
            s1.push_child(s2);
            $$ = s1;
        } )*
    )
    | '' { $$.set_children(NQPC_NODE_ARGS); }

expr = funcall_expr

funcall_expr =
    i:ident '(' a:args ')' {
        $$.set(NQPC_NODE_FUNCALL, i, a);
    }
    | add_expr

add_expr =
    l:mul_expr (
          '+' r1:mul_expr {
            $$.set(NQPC_NODE_ADD, l, r1);
            l = $$;
          }
        | '-' r2:mul_expr {
            $$.set(NQPC_NODE_SUB, l, r2);
            l = $$;
          }
    )* {
        $$ = l;
    }

mul_expr =
    l:term (
        '*' r:term {
            $$.set(NQPC_NODE_MUL, l, r);
            l = $$;
        }
        | '/' r:term {
            $$.set(NQPC_NODE_DIV, l, r);
            l = $$;
        }
    )* {
        $$ = l;
    }

term = value

ident = < [a-zA-Z] [a-zA-Z0-9]+ ( [_-] [a-zA-Z0-9]+ )* > {
    $$.set_ident(yytext, yyleng);
}

value = 
    ( '-' ( integer | dec_number) ) {
        $$.negate();
    }
    | integer
    | dec_number

#  <?MARKED('endstmt')>
#  <?terminator>
eat_terminator =
    ';' | end-of-file

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

# TODO

# white space
ws = ' ' | '\f' | '\v' | '\t' | '\205' | '\240' | end-of-line
    | '#' [^\n]*
end-of-line = ( '\r\n' | '\n' | '\r' ) {
    line_number++;
}
end-of-file = !'\0'

%%
