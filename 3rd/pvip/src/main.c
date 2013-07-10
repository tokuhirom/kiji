#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "pvip.h"
#include "commander.h"

typedef struct _CmdLineState {
    int debug;
    const char* eval;
} CmdLineState;

static void debug(command_t *self) {
    ((CmdLineState*)self->data)->debug = strtoll(self->arg, NULL, 10);
}

static void eval(command_t *self) {
    ((CmdLineState*)self->data)->eval = self->arg;
}

int main(int argc, char **argv) {
    CmdLineState*state = malloc(sizeof(CmdLineState));
    memset(state, 0, sizeof(CmdLineState));

    command_t cmd;
    cmd.data = state;
    command_init(&cmd, argv[0], "0.0.1");
    command_option(&cmd, "-d", "--debug <arg>", "enable debugging stuff", debug);
    command_option(&cmd, "-e", "--eval [code]", "eval code", eval);
    command_parse(&cmd, argc, argv);

    if (state->eval) {
            const char *src = state->eval;
            PVIPNode *node = PVIP_parse_string(src, strlen(src), state->debug);
            assert(node);

            PVIPString *buf = PVIP_string_new();
            PVIP_node_as_sexp(node, buf);
            PVIP_string_say(buf);
            PVIP_string_destroy(buf);
            PVIP_node_destroy(node);
    } else if (cmd.argc==1) {
            FILE *fp = fopen(cmd.argv[0], "rb");
            if (!fp) {
                perror(cmd.argv[0]);
                exit(0);
            }
            PVIPString *buf = PVIP_string_new();
            PVIPNode *node = PVIP_parse_fp(fp, state->debug);
            assert(node);
            PVIP_node_as_sexp(node, buf);
            PVIP_string_say(buf);
            PVIP_string_destroy(buf);
            PVIP_node_destroy(node);
            fclose(fp);
    } else if (cmd.argc==0) {
            PVIPString *buf = PVIP_string_new();
            PVIPNode *node = PVIP_parse_fp(stdin, state->debug);
            assert(node);
            PVIP_node_as_sexp(node, buf);
            PVIP_string_say(buf);
            PVIP_string_destroy(buf);
            PVIP_node_destroy(node);
    } else {
        command_help(&cmd);
    }
    free(state);
    command_free(&cmd);
    return 0;
}

