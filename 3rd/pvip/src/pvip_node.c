#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <inttypes.h>
#include "pvip.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

PVIPNode * PVIP_node_new_int(PVIP_node_type_t type, int64_t n) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    assert(type == PVIP_NODE_INT);
    node->type = type;
    node->iv = n;
    return node;
}

PVIPNode * PVIP_node_new_intf(PVIP_node_type_t type, const char *str, size_t len, int base) {
    char * buf = malloc(len+1);
    char *bufp = buf;
    for (int i=0; i<len; i++) {
        if (str[i] != '_') {
            *bufp++ = str[i];
        }
    }
    *bufp++ = '\0';
    int64_t n = strtoll(buf, NULL, base);
    free(buf);
    return PVIP_node_new_int(type, n);
}

PVIPNode * PVIP_node_new_string(PVIP_node_type_t type, const char* str, size_t len) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    assert(
         type != PVIP_NODE_IDENT
      || type != PVIP_NODE_VARIABLE
      || type != PVIP_NODE_STRING
    );
    node->type = type;
    node->pv = PVIP_string_new();
    PVIP_string_concat(node->pv, str, len);
    return node;
}

PVIPNode* PVIP_node_append_string(PVIPNode *node, const char* txt, size_t length) {
    if (node->type == PVIP_NODE_STRING) {
        PVIP_string_concat(node->pv, txt, length);
        return node;
    } else if (node->type == PVIP_NODE_STRING_CONCAT) {
        if (node->children.nodes[node->children.size-1]->type == PVIP_NODE_STRING) {
            PVIP_string_concat(node->children.nodes[node->children.size-1]->pv, txt, length);
            return node;
        } else {
            PVIPNode *s = PVIP_node_new_string(PVIP_NODE_STRING, txt, length);
            return PVIP_node_new_children2(PVIP_NODE_STRING_CONCAT, node, s);
        }
    } else {
        abort();
    }
}

PVIPNode* PVIP_node_append_string_from_hex(PVIPNode *node, const char* str, size_t len) {
    assert(PVIP_node_category(node->type) == PVIP_CATEGORY_STR);
    assert(len==2);

    char buf[3];
    buf[0] = str[0];
    buf[1] = str[1];
    buf[2] = '\0';
    char c = strtol(buf, NULL, 16);
    return PVIP_node_append_string(node, &c, 1);
}

PVIPNode* PVIP_node_append_string_from_oct(PVIPNode *node, const char* str, size_t len) {
    assert(PVIP_node_category(node->type) == PVIP_CATEGORY_STR);
    assert(len==2);

    char buf[3];
    buf[0] = str[0];
    buf[1] = str[1];
    buf[2] = '\0';
    char c = strtol(buf, NULL, 8);
    return PVIP_node_append_string(node, &c, 1);
}

PVIPNode* PVIP_node_append_string_variable(PVIPNode*node, PVIPNode*var) {
    if (node->type == PVIP_NODE_STRING) {
        return PVIP_node_new_children2(PVIP_NODE_STRING_CONCAT, node, var);
    } else if (node->type == PVIP_NODE_STRING_CONCAT) {
        return PVIP_node_new_children2(PVIP_NODE_STRING_CONCAT, node, var);
    } else {
        abort();
    }
}


void PVIP_node_change_type(PVIPNode *node, PVIP_node_type_t type) {
    assert(PVIP_node_category(node->type) == PVIP_node_category(type));
    node->type = type;
}

PVIPNode* PVIP_node_new_number(PVIP_node_type_t type, const char *str, size_t len) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    assert(type == PVIP_NODE_NUMBER);
    node->type = type;
    node->nv = atof(str);
    return node;
}

PVIPNode* PVIP_node_new_children(PVIP_node_type_t type) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    memset(node, 0, sizeof(PVIPNode));
    assert(type != PVIP_NODE_NUMBER);
    assert(type != PVIP_NODE_INT);
    node->type = type;
    node->children.size  = 0;
    node->children.nodes = NULL;
    return node;
}
PVIPNode* PVIP_node_new_children1(PVIP_node_type_t type, PVIPNode* n1) {
    PVIPNode* node = PVIP_node_new_children(type);
    PVIP_node_push_child(node, n1);
    return node;
}

PVIPNode* PVIP_node_new_children2(PVIP_node_type_t type, PVIPNode* n1, PVIPNode *n2) {
    PVIPNode* node = PVIP_node_new_children(type);
    PVIP_node_push_child(node, n1);
    PVIP_node_push_child(node, n2);
    return node;
}

PVIPNode* PVIP_node_new_children3(PVIP_node_type_t type, PVIPNode* n1, PVIPNode *n2, PVIPNode *n3) {
    PVIPNode* node = PVIP_node_new_children(type);
    PVIP_node_push_child(node, n1);
    PVIP_node_push_child(node, n2);
    PVIP_node_push_child(node, n3);
    return node;
}

void PVIP_node_push_child(PVIPNode* node, PVIPNode* child) {
    node->children.nodes = (PVIPNode**)realloc(node->children.nodes, sizeof(PVIPNode*)*(node->children.size+1));
    assert(node->children.nodes);
    node->children.nodes[node->children.size] = child;
    node->children.size++;
}

PVIP_category_t PVIP_node_category(PVIP_node_type_t type) {
    switch (type) {
    case PVIP_NODE_STRING:
    case PVIP_NODE_VARIABLE:
    case PVIP_NODE_IDENT:
        return PVIP_CATEGORY_STR;
    case PVIP_NODE_INT:
        return PVIP_CATEGORY_INT;
    case PVIP_NODE_NUMBER:
        return PVIP_CATEGORY_NUMBER;
    default:
        return PVIP_CATEGORY_CHILDREN;
    }
}

void PVIP_node_destroy(PVIPNode *node) {
    PVIP_category_t category = PVIP_node_category(node->type);
    if (category == PVIP_CATEGORY_CHILDREN) {
        int i;
        for (i=0; i<node->children.size; i++) {
            PVIP_node_destroy(node->children.nodes[i]);
        }
    } else if (category == PVIP_CATEGORY_STR) {
        PVIP_string_destroy(node->pv);
    }
    free(node);
}
static void _PVIP_node_as_sexp(PVIPNode * node, PVIPString *buf, int indent) {
    PVIP_string_concat(buf, "(", 1);
    const char *name = PVIP_node_name(node->type);
    PVIP_string_concat(buf, name, strlen(name));
    PVIP_string_concat(buf, " ", 1);
    switch (PVIP_node_category(node->type)) {
    case PVIP_CATEGORY_STR: {
        int i;
        PVIP_string_concat(buf, "\"", 1);
        for (i=0; i<node->pv->len; i++) {
            char c = node->pv->buf[i];
            switch (c) {
            case '\\': PVIP_string_concat(buf, "\\\\",    2); break;
            case '"':  PVIP_string_concat(buf, "\\\"",    2); break;
            case '/':  PVIP_string_concat(buf, "\\/",     2); break;
            case '\b': PVIP_string_concat(buf, "\\b",     2); break;
            case '\f': PVIP_string_concat(buf, "\\f",     2); break;
            case '\n': PVIP_string_concat(buf, "\\n",     2); break;
            case '\r': PVIP_string_concat(buf, "\\r",     2); break;
            case '\t': PVIP_string_concat(buf, "\\t",     2); break;
            case '\a': PVIP_string_concat(buf, "\\u0007", 6); break;
            default:   PVIP_string_concat(buf, &c,        1); break;
            }
        }
        PVIP_string_concat(buf, "\"", 1);
        break;
    }
    case PVIP_CATEGORY_INT:
        PVIP_string_concat_int(buf, node->iv);
        break;
    case PVIP_CATEGORY_NUMBER:
        PVIP_string_concat_number(buf, node->nv);
        break;
    case PVIP_CATEGORY_CHILDREN: {
        int i=0;
        for (i=0; i<node->children.size; i++) {
            _PVIP_node_as_sexp(node->children.nodes[i], buf, indent+1);
            if (i!=node->children.size-1) {
                PVIP_string_concat(buf, " ", 1);
            }
        }
        break;
    }
    }
    PVIP_string_concat(buf, ")", 1);
}

void PVIP_node_as_sexp(PVIPNode * node, PVIPString *buf) {
    assert(node);
    _PVIP_node_as_sexp(node, buf, 0);
}

void PVIP_node_dump_sexp(PVIPNode * node) {
    PVIPString*buf = PVIP_string_new();
    PVIP_node_as_sexp(node, buf);
    PVIP_string_say(buf);
    PVIP_string_destroy(buf);
}