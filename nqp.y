%{
#include <stdio.h>
#include "node.h"

typedef struct {
    NQPC_NODE_TYPE type;
    union {
        int iv; // integer value
        double nv; // number value
    } body;
} NQPCNode;

static NQPCNode node_global;

#define YYSTYPE NQPCNode

static void nqpc_ast_number(NQPCNode & node, const char*txt, int len) {
    node.type = NQPC_NODE_NUMBER;
    node.body.nv = atof(txt);
}

static void nqpc_ast_integer(NQPCNode & node, const char*txt, int len, int base) {
    node.type = NQPC_NODE_INT;
    node.body.iv = strtol(txt, NULL, base);
}

static void indent(int n) {
    for (int i=0; i<n*4; i++) {
        printf(" ");
    }
}

static void nqpc_dump_node(NQPCNode &node, unsigned int depth) {
    printf("{\n");
    indent(depth+1);
    printf("\"type\":\"%s\",\n", nqpc_node_type2name(node.type));
    switch (node.type) {
    case NQPC_NODE_INT:
        indent(depth+1);
        printf("\"value\":%d\n", node.body.iv);
        break;
    case NQPC_NODE_NUMBER:
        indent(depth+1);
        printf("\"value\":%lf\n", node.body.nv);
        break;
    case NQPC_NODE_FUNCALL:
    case NQPC_NODE_STRING:
        // TODO
        break;
    }
    printf("}\n");
}

%}

comp_init = e:value end-of-line? end-of-file {
    node_global = e;
}

value = integer | dec_number

dec_number =
    <([.][0-9]+)> {
    nqpc_ast_number($$, yytext, yyleng);
}
    | <([0-9]+ '.' [0-9]+)> {
    nqpc_ast_number($$, yytext, yyleng);
}
    | <([0-9]+)> {
    nqpc_ast_integer($$, yytext, yyleng, 10);
}

integer =
    '0b' <[01]+> {
    nqpc_ast_integer($$, yytext, yyleng, 2);
}

# TODO
# space = ' ' | '\f' | '\v' | '\t' | '\205' | '\240' | end-of-line
end-of-line = '\r\n' | "\n" | '\r'
end-of-file = !'\0'

%%

int main()
{
    GREG g;
    yyinit(&g);
    while (yyparse(&g));
    yydeinit(&g);

    nqpc_dump_node(node_global, 0);

    return 0;
}
