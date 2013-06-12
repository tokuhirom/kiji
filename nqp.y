%{
#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <assert.h>
#include "node.h"

class NQPCNode {
public:
    NQPCNode() : type(NQPC_NODE_UNDEF) { }
    NQPCNode(const NQPCNode &node) {
        this->type = node.type;
        switch (type) {
        case NQPC_NODE_INT:
            this->body.iv = node.body.iv;
            break;
        case NQPC_NODE_NUMBER:
            this->body.nv = node.body.nv;
            break;
        case NQPC_NODE_STATEMENTS:
            this->children = node.children;
            break;
        case NQPC_NODE_UNDEF:
            abort();
        }
    }

    NQPC_NODE_TYPE type;
    union {
        long int iv; // integer value
        double nv; // number value
    } body;
    std::vector<NQPCNode> children;
};

static NQPCNode node_global;

#define YYSTYPE NQPCNode

static void nqpc_ast_number(NQPCNode & node, const char*txt, int len) {
    assert(len);
    node.type = NQPC_NODE_NUMBER;
    node.body.nv = atof(txt);
}

static void nqpc_ast_integer(NQPCNode & node, const char*txt, int len, int base) {
    assert(len);
    node.type = NQPC_NODE_INT;
    node.body.iv = strtol(txt, NULL, base);
}

static void nqpc_ast_statement_list(NQPCNode &node, NQPCNode &child) {
    node.type = NQPC_NODE_STATEMENTS;
    node.children.push_back(child);
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
        printf("\"value\":%ld\n", node.body.iv);
        break;
    case NQPC_NODE_NUMBER:
        indent(depth+1);
        printf("\"value\":%lf\n", node.body.nv);
        break;
    case NQPC_NODE_STATEMENTS: {
        indent(depth+1);
        printf("\"value\":[\n");
        int i=0;
        for (auto &x:node.children) {
            indent(depth+2);
            nqpc_dump_node(
                x, depth+2
            );
            if (i==node.children.size()-2) {
                printf(",\n");
            } else {
                printf("\n");
            }
            i++;
        }
        indent(depth+1);
        printf("]\n");
        break;
    }
    case NQPC_NODE_UNDEF:
        break;
    }
    indent(depth);
    printf("}");
    if (depth == 0) {
        printf("\n");
    }
}

%}

comp_init = e:statementlist end-of-file {
    node_global = e;
}

statementlist =
    s:statement {
        nqpc_ast_statement_list($$, s);
    }
    ( eat_terminator? q:statement {
        $$.children.push_back(q);
    } )*

# TODO
statement = term

term = value

value = 
    ( ('-' integer) | ('-' dec_number) ) {
        if ($$.type == NQPC_NODE_INT) {
            $$.body.iv = - $$.body.iv;
        } else {
            $$.body.nv = - $$.body.nv;
        }
    }
    | integer
    | dec_number

#  <?MARKED('endstmt')>
#  <?terminator>
eat_terminator =
    ';'
    | end-of-file

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
    | '0x' <[0-9a-f]+> {
    nqpc_ast_integer($$, yytext, yyleng, 16);
}
    | '0o' <[0-7]+> {
    nqpc_ast_integer($$, yytext, yyleng, 8);
}

# TODO
# space = ' ' | '\f' | '\v' | '\t' | '\205' | '\240' | end-of-line
# end-of-line = '\r\n' | "\n" | '\r'
end-of-file = !'\0'

%%

int main()
{
    GREG g;
    yyinit(&g);
    if (!yyparse(&g)) {
        fprintf(stderr, "** Syntax error!\n");
    }
    yydeinit(&g);

    nqpc_dump_node(node_global, 0);

    return 0;
}
