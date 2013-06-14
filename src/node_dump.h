#pragma once

static void indent(int n) {
    for (int i=0; i<n*4; i++) {
        printf(" ");
    }
}

static void nqpc_dump_node(const SARUNode &node, unsigned int depth) {
    printf("{\n");
    indent(depth+1);
    printf("\"type\":\"%s\",\n", nqpc_node_type2name(node.type()));
    switch (node.type()) {
    case SARU_NODE_INT:
        indent(depth+1);
        printf("\"value\":%ld\n", node.iv());
        break;
    case SARU_NODE_NUMBER:
        indent(depth+1);
        printf("\"value\":%lf\n", node.nv());
        break;
        // Node has a PV
    case SARU_NODE_STRING:
    case SARU_NODE_IDENT:
        indent(depth+1);
        printf("\"value\":\"%s\"\n", node.pv().c_str()); // TODO need escape
        break;
        // Node has children
    case SARU_NODE_ARGS:
    case SARU_NODE_FUNCALL:
    case SARU_NODE_MUL:
    case SARU_NODE_ADD:
    case SARU_NODE_SUB:
    case SARU_NODE_DIV:
    case SARU_NODE_STATEMENTS: {
        indent(depth+1);
        printf("\"value\":[\n");
        int i=0;
        for (auto &x:node.children()) {
            indent(depth+2);
            nqpc_dump_node(
                x, depth+2
            );
            if (i==node.children().size()-1) {
                printf("\n");
            } else {
                printf(",\n");
            }
            i++;
        }
        indent(depth+1);
        printf("]\n");
        break;
    }
    case SARU_NODE_UNDEF:
        break;
    }
    indent(depth);
    printf("}");
    if (depth == 0) {
        printf("\n");
    }
}

static void nqpc_dump_node(const SARUNode &node) {
    nqpc_dump_node(node, 0);
}
