#ifndef PVIP_H_
#define PVIP_H_

#include <stdint.h>
#include "gen.node.h"

typedef enum {
    PVIP_CATEGORY_STR,
    PVIP_CATEGORY_INT,
    PVIP_CATEGORY_NUMBER,
    PVIP_CATEGORY_CHILDREN
} PVIP_category_t;

typedef struct {
    char *buf;
    size_t len;
    size_t buflen;
} PVIPString;

typedef struct _PVIPNode {
    PVIP_node_type_t type;
    union {
        int64_t iv;
        double nv;
        PVIPString *pv;
        struct {
            int size;
            struct _PVIPNode **nodes;
        } children;
    };
} PVIPNode;

/* node */
PVIPNode* PVIP_node_new_children(PVIP_node_type_t type);
PVIPNode* PVIP_node_new_children1(PVIP_node_type_t type, PVIPNode* n1);
PVIPNode* PVIP_node_new_children2(PVIP_node_type_t type, PVIPNode* n1, PVIPNode *n2);
PVIPNode* PVIP_node_new_children3(PVIP_node_type_t type, PVIPNode* n1, PVIPNode *n2, PVIPNode *n3);
PVIPNode* PVIP_node_new_int(PVIP_node_type_t type, int64_t n);
PVIPNode* PVIP_node_new_intf(PVIP_node_type_t type, const char *str, size_t len, int base);
PVIPNode* PVIP_node_new_string(PVIP_node_type_t type, const char* str, size_t len);
PVIPNode* PVIP_node_new_number(PVIP_node_type_t type, const char *str, size_t len);

void PVIP_node_push_child(PVIPNode* node, PVIPNode* child);

void PVIP_node_destroy(PVIPNode *node);

PVIPNode* PVIP_node_append_string(PVIPNode *node, const char* str, size_t len);
PVIPNode* PVIP_node_append_string_from_hex(PVIPNode * node, const char *str, size_t len);
PVIPNode* PVIP_node_append_string_from_oct(PVIPNode * node, const char *str, size_t len);
PVIPNode* PVIP_node_append_string_variable(PVIPNode*node, PVIPNode*var);

void PVIP_node_change_type(PVIPNode *node, PVIP_node_type_t type);

PVIP_category_t PVIP_node_category(PVIP_node_type_t type);

void PVIP_node_as_sexp(PVIPNode * node, PVIPString *buf);
void PVIP_node_dump_sexp(PVIPNode * node);

/* string */
PVIPString *PVIP_string_new();
void PVIP_string_destroy(PVIPString *str);
void PVIP_string_concat(PVIPString *str, const char *src, size_t len);
void PVIP_string_concat_int(PVIPString *str, int64_t n);
void PVIP_string_say(PVIPString *str);

/* parser */
PVIPNode * PVIP_parse_string(const char *string, int len, int debug);
PVIPNode * PVIP_parse_fp(FILE *fp, int debug);

#endif /* PVIP_H_ */
