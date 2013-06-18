// vim:ts=2:sw=2:tw=0:
#include <stdio.h>
#include <assert.h>
#include "node.h"
#include "gen.saru.y.cc"
extern "C" {
#include "moarvm.h"
}
#include "compiler.h"

int main(int argc, char** argv) {
  GREG g;
  line_number=0;
  global_input_stream = &std::cin;
  yyinit(&g);
  if (!yyparse(&g)) {
    fprintf(stderr, "** Syntax error at line %d\n", line_number);
    if (g.text[0]) {
      fprintf(stderr, "** near %s\n", g.text);
    }
    if (g.pos < g.limit || !feof(stdin)) {
      g.buf[g.limit]= '\0';
      fprintf(stderr, " before text \"");
      while (g.pos < g.limit) {
        if ('\n' == g.buf[g.pos] || '\r' == g.buf[g.pos]) break;
        fputc(g.buf[g.pos++], stderr);
      }
      if (g.pos == g.limit) {
        int c;
        while (EOF != (c= fgetc(stdin)) && '\n' != c && '\r' != c)
        fputc(c, stderr);
      }
      fputc('\"', stderr);
    }
    fprintf(stderr, "\n");
    exit(1);
  }
  yydeinit(&g);

  saru::Interpreter interp;
  interp.initialize();
  saru::Compiler compiler(interp);
  compiler.compile(node_global);
  if (argc>1) {
    interp.dump();
  } else {
    interp.run();
  }

  return 0;
}
