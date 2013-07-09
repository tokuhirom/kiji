#ifndef PVIP_H_
#define PVIP_H_

#include "gen.node.h"

typedef enum {
    PVIP_CATEGORY_STR,
    PVIP_CATEGORY_INT,
    PVIP_CATEGORY_DOUBLE,
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
        int    iv;
        double nv;
        PVIPString pv;
        struct {
            int size;
            struct _PVIPNode **nodes;
        } children;
    };
} PVIPNode;

/* node */
PVIPNode* PVIP_node_new_children(PVIP_node_type_t type);
PVIPNode * PVIP_node_new_int(PVIP_node_type_t type, int n);
void PVIP_node_destroy(PVIPNode *node);
void PVIP_node_push_child(PVIPNode* node, PVIPNode* child);
void PVIP_node_as_sexp(PVIPNode * node, PVIPString *buf);

/* string */
PVIPString *PVIP_string_new();
void PVIP_string_destroy(PVIPString *str);
void PVIP_string_concat(PVIPString *str, const char *src, size_t len);
void PVIP_string_concat_int(PVIPString *str, int n);
void PVIP_string_say(PVIPString *str);
int PVIP_str_eq_c_str(PVIPString *str, const char *buf, int len);

/* parser */
PVIPNode * PVIP_parse_string(const char *string, int len, int debug);

#endif /* PVIP_H_ */
