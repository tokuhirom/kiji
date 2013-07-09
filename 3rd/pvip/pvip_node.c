#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "pvip.h"

#ifndef MIN
#define MIN(a,b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a,b) (((a)>(b))?(a):(b))
#endif

PVIPNode * PVIP_node_new_int(PVIP_node_type_t type, int n) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    assert(type == PVIP_NODE_INT);
    node->type = type;
    node->iv = n;
    return node;
}

PVIPNode * PVIP_node_new_pv(PVIP_node_type_t type, const char* str, size_t len) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    assert(
         type != PVIP_NODE_IDENT
      || type != PVIP_NODE_VARIABLE
      || type != PVIP_NODE_STRING
    );
    node->type = type;
    node->pv.buf = (char*)malloc(len);
    memcpy(node->pv.buf, str, len);
    return node;
}

PVIPNode* PVIP_node_new_nv(PVIP_node_type_t type, double n) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    assert(type == PVIP_NODE_DOUBLE);
    node->type = type;
    node->nv = n;
    return node;
}

PVIPNode* PVIP_node_new_children(PVIP_node_type_t type) {
    PVIPNode *node = malloc(sizeof(PVIPNode));
    assert(type != PVIP_NODE_DOUBLE);
    assert(type != PVIP_NODE_INT);
    node->type = type;
    node->children.size = 0;
    node->children.nodes = malloc(0);
    return node;
}

void PVIP_node_push_child(PVIPNode* node, PVIPNode* child) {
    node->children.nodes = (PVIPNode**)realloc(node->children.nodes, node->children.size+1);
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
    case PVIP_NODE_DOUBLE:
        return PVIP_CATEGORY_DOUBLE;
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
        free(node->pv.buf);
    }
    free(node);
}

PVIPString *PVIP_string_new() {
    PVIPString *str = malloc(sizeof(PVIPString));
    memset(str, 0, sizeof(PVIPString));
    assert(str);
    str->len    = 0;
    str->buflen = 1024;
    str->buf    = malloc(str->buflen);
    return str;
}

void PVIP_string_destroy(PVIPString *str) {
    free(str->buf);
    str->buf = NULL;
    free(str);
}

void PVIP_string_concat(PVIPString *str, const char *src, size_t len) {
    if (str->len + len > str->buflen) {
        str->buflen = ( str->len + len ) * 2;
        str->buf    = realloc(str->buf, str->buflen);
    }
    memcpy(str->buf + str->len, src, len);
    str->len += len;
}

void PVIP_string_concat_int(PVIPString *str, int n) {
    char buf[1024];
    int res = snprintf(buf, 1023, "%d", n);
    PVIP_string_concat(str, buf, res);
}

void PVIP_string_say(PVIPString *str) {
    fwrite(str->buf, 1, str->len, stdout);
    fwrite("\n", 1, 1, stdout);
}

/* Compare PVIPString with C sring.
 * It returns true if both strings are same value. */
int PVIP_str_eq_c_str(PVIPString *str, const char *buf, int len) {
    if (str->len != len) {
        return 0;
    }
    return memcmp(str->buf, buf, len)==0;
}

static void _PVIP_node_as_sexp(PVIPNode * node, PVIPString *buf, int indent) {
    PVIP_string_concat(buf, "(", 1);
    const char *name = PVIP_node_name(node->type);
    PVIP_string_concat(buf, name, strlen(name));
    PVIP_string_concat(buf, " ", 1);
    switch (PVIP_node_category(node->type)) {
    case PVIP_CATEGORY_STR:
        abort(); /* NIY */
    case PVIP_CATEGORY_INT:
        PVIP_string_concat_int(buf, node->iv);
        break;
    case PVIP_CATEGORY_DOUBLE:
        abort(); /* NIY */
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

