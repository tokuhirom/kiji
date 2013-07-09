#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "pvip.h"

int main(int argc, char **argv) {
    if (argc==3 && strcmp(argv[1], "-e")==0) {
        int debug = 0;
        PVIPString *buf = PVIP_string_new();
        char *src =argv[2];
        PVIPNode *node = PVIP_parse_string(src, strlen(src), debug);
        assert(node);
        PVIP_node_as_sexp(node, buf);
        PVIP_string_say(buf);
        PVIP_string_destroy(buf);
        PVIP_node_destroy(node);
    } else if (argc==2) {
        int debug = 0;
        FILE *fp = fopen(argv[1], "rb");
        PVIPString *buf = PVIP_string_new();
        PVIPNode *node = PVIP_parse_fp(fp, debug);
        assert(node);
        PVIP_node_as_sexp(node, buf);
        PVIP_string_say(buf);
        PVIP_string_destroy(buf);
        PVIP_node_destroy(node);
        fclose(fp);
    } else if (argc==1) {
        /* TODO: read from file */
        int debug = 0;
        PVIPString *buf = PVIP_string_new();
        PVIPNode *node = PVIP_parse_fp(stdin, debug);
        assert(node);
        PVIP_node_as_sexp(node, buf);
        PVIP_string_say(buf);
        PVIP_string_destroy(buf);
        PVIP_node_destroy(node);
    } else {
        abort();
    }

    return 0;
}

