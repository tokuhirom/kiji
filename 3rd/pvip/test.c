#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "pvip.h"

#define OK(n) printf("%s %d\n", (n) ? "ok" : "not ok", ++TAP_COUNT)

int TAP_COUNT;

void test_it(const char*src, const char *expected) {
    int debug = 0;
    PVIPString *buf = PVIP_string_new();
    PVIPNode *node = PVIP_parse_string(src, strlen(src), debug);
    assert(node);
    PVIP_node_as_sexp(node, buf);
    printf("# ");
    PVIP_string_say(buf);
    OK(PVIP_str_eq_c_str(buf, expected, strlen(expected)));
    PVIP_string_destroy(buf);
    PVIP_node_destroy(node);
}

int main() {
    TAP_COUNT=0;

    printf("1..1\n");

    {
        PVIPString *buf = PVIP_string_new();
        PVIP_string_concat(buf, "# ", 2);
        PVIP_string_concat(buf, "concat", 6);
        PVIP_string_concat(buf, " works", 6);
        PVIP_string_say(buf);
        PVIP_string_destroy(buf);
    }

    test_it("3", "(statements (int 3))");

    return 0;
}

