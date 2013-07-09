%{

#include "pvip.h"
#include <assert.h>

#define YYSTYPE PVIPNode*
#define YY_NAME(n) PVIP_##n
#define YY_XTYPE PVIPParserContext

typedef struct {
    const char  *buf;
    size_t len;
    size_t pos;
} PVIPParserStringState;

typedef struct {
    int line_number;
    PVIPNode *root;
    int is_string;
    PVIPParserStringState *str;
    FILE *fp;
} PVIPParserContext;

static char PVIP_input(char *buf, YY_XTYPE D) {
    if (D.is_string) {
        if (D.str->len == D.str->pos) {
            return 0;
        } else {
            *buf = D.str->buf[D.str->pos];
            D.str->pos = D.str->pos+1;
            return 1;
        }
    } else {
        char c = fgetc(D.fp);
        *buf = c;
        return (c==EOF) ? 0 : 1;
    }
}

#define YY_INPUT(buf, result, max_size, D)		\
    result = PVIP_input(buf, D);

%}

comp_init = e:statementlist end-of-file {
    $$ = (G->data.root = e);
}

statementlist =
    (
        s1:statement {
            $$ = PVIP_node_new_children(PVIP_NODE_STATEMENTS);
            PVIP_node_push_child($$, s1);
            s1 = $$;
        }
        (
            - s2:statement {
                PVIP_node_push_child(s1, s2);
                $$=s1;
            }
        )* eat_terminator?
    )

# TODO

statement = "3" { $$ = PVIP_node_new_int(PVIP_NODE_INT, 3); }

#  <?MARKED('endstmt')>
#  <?terminator>
eat_terminator =
    (';' -) | end-of-file

comment= '#' [^\n]* end-of-line

# white space
ws = ' ' | '\f' | '\v' | '\t' | '\205' | '\240' | end-of-line
    | comment

- = ws*

end-of-line = ( '\r\n' | '\n' | '\r' ) {
    G->data.line_number++;
}
end-of-file = !'\0'

%%

PVIPNode * PVIP_parse_string(const char *string, int len, int debug) {
    GREG g;
    YY_NAME(init)(&g);

#ifdef YY_DEBUG
    g.debug=debug;
#endif
printf("PARSE\n");

    g.data.line_number = 1;
    g.data.is_string   = 1;
    g.data.str = malloc(sizeof(PVIPParserStringState));
    g.data.str->buf     = string;
    g.data.str->len     = len;
    g.data.str->pos     = 0;

    if (!YY_NAME(parse)(&g)) {
      fprintf(stderr, "** Syntax error at line %d\n", g.data.line_number);
      if (g.text[0]) {
        fprintf(stderr, "** near %s\n", g.text);
      }
      if (g.pos < g.limit || g.data.str->len==g.data.str->pos) {
        g.buf[g.limit]= '\0';
        fprintf(stderr, " before text \"");
        while (g.pos < g.limit) {
          if ('\n' == g.buf[g.pos] || '\r' == g.buf[g.pos]) break;
          fputc(g.buf[g.pos++], stderr);
        }
        if (g.pos == g.limit) {
            while (g.data.str->len!=g.data.str->pos) {
                char ch = g.data.str->buf[g.data.str->pos++];
                if (!ch || '\n' == ch || '\r' == ch) {
                    break;
                }
                fputc(ch, stderr);
            }
        }
        fputc('\"', stderr);
      }
      fprintf(stderr, "\n\n");
        free(g.data.str);
      return NULL;
    }
    /*
    if (g.data.str->len!=g.data.str->pos) {
      printf("Syntax error! Around:\n");
      for (int i=0; g.data.str->len!=g.data.str->pos && i<24; i++) {
        char ch = g.data.str->buf[g.data.str->pos++];
        if (ch) {
          printf("%c", ch);
        }
      }
      printf("\n");
      exit(1);
    }
    */
    free(g.data.str);
    PVIPNode *root = g.data.root;
    assert(g.data.root);
    YY_NAME(deinit)(&g);
    return root;
}

PVIPNode * PVIP_parse_fh(const char *string, int len) {
/* TODO */
    return NULL;
}

