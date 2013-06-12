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
    printf("'%s', %f\n", txt, node.body.nv);
}

static void nqpc_ast_integer(NQPCNode & node, const char*txt, int len) {
    node.type = NQPC_NODE_INT;
    node.body.iv = atoi(txt);
    printf("'%s', %d\n", txt, node.body.iv);
}

static void indent(int n) {
    for (int i=0; i<n; i++) {
        printf(" ");
    }
}

static void nqpc_dump_node(NQPCNode &node, unsigned int depth) {
    printf("- %s\n", nqpc_node_type2name(node.type));
    switch (node.type) {
    case NQPC_NODE_INT:
        indent(depth*4 + 4);
        printf("%d\n", node.body.iv);
        break;
    case NQPC_NODE_NUMBER:
        indent(depth*4 + 4);
        printf("%lf\n", node.body.nv);
        break;
    case NQPC_NODE_FUNCALL:
    case NQPC_NODE_STRING:
        // TODO
        break;
    }
}

%}

comp_init = e:dec_number end-of-line? end-of-file {
    printf("GAH : %d\n", e.body.iv);
    node_global = e;
}


dec_number =
    <([.][0-9]+)> {
    printf("NAY");
    nqpc_ast_number($$, yytext, yyleng);
}
    | <([0-9]+ '.' [0-9]+)> {
    printf("YAY");
    nqpc_ast_number($$, yytext, yyleng);
}
    | <([0-9]+)> {
    nqpc_ast_integer($$, yytext, yyleng);
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
    printf("values: %d\n", node_global.body.iv);

    return 0;
}
