// vim:ts=2:sw=2:tw=0:


#include <stdio.h>
#include <assert.h>
#include <fstream>
#ifdef __unix__
#include <unistd.h>
#endif
extern "C" {
#include <moarvm.h>
#include "commander.h"
}
#include "compiler.h"

typedef struct _CmdLineState {
    int dump_ast;
    int dump_bytecode;
    const char* eval;
} CmdLineState;

static void dump_bytecode(command_t *self) {
    ((CmdLineState*)self->data)->dump_bytecode = 1;
}

static void dump_ast(command_t *self) {
    ((CmdLineState*)self->data)->dump_ast = 1;
}

static void eval(command_t *self) {
    ((CmdLineState*)self->data)->eval = self->arg;
}

static void run_repl() {
  // TODO disabled for now(pvip)
  /*
  std::string src;
  while (!std::cin.eof()) {
    std::cout << "> ";
    std::cin >> src;

    {
      std::unique_ptr<std::istringstream> iss(new std::istringstream(src));

      kiji::Node root;

      if (!kiji::parse(&(*iss), root)) {
        continue;
      }

      kiji::Interpreter interp;
      kiji::CompUnit cu(interp.main_thread());
      kiji::Compiler compiler(cu);
      compiler.compile(root);
      interp.run(cu);
    }
    std::cout << std::endl;
  }
  */
}

int main(int argc, char** argv) {
  CmdLineState*state = (CmdLineState*)malloc(sizeof(CmdLineState));
  memset(state, 0, sizeof(CmdLineState));

  command_t cmd;
  cmd.data = state;
  command_init(&cmd, argv[0], "0.0.1");
  command_option(&cmd, "-q", "--dump-bytecode", "dump bytecode", dump_bytecode);
  command_option(&cmd, "-p", "--dump-ast", "dump Abstract Syntax Tree", dump_ast);
  command_option(&cmd, "-e", "--eval [code]", "eval code", eval);
  command_parse(&cmd, argc, argv);

  int processed_args = 0;

  PVIPNode *root_node;
  if (state->eval) {
    root_node = PVIP_parse_string(state->eval, strlen(state->eval), 0);
  } else if (cmd.argc==0) {
#ifdef __unix__
    if (isatty(fileno(stdin))) {
      run_repl();
      exit(0);
    }
#endif

    root_node = PVIP_parse_fp(stdin, 0);
  } else {
    FILE *fp = fopen(cmd.argv[0], "rb");
    if (!fp) {
      printf("Cannot open file %s for reading: %s", cmd.argv[0], strerror(errno));
      exit(1);
    }
    root_node = PVIP_parse_fp(fp, 0);
  }

  // This include apr_initialize().
  // You need to initialize before `apr_pool_create()`
  kiji::Interpreter interp;

  // stash the rest of the raw command line args in the instance
  interp.set_clargs(cmd.argc-1, (char **)cmd.argv+1);
  /*
  instance->num_clargs = argc - processed_args;
  instance->raw_clargs = (char **)(opt->argv + processed_args);
  */

  if (!root_node) {
    exit(1);
  }

  if (state->dump_ast) {
    PVIP_node_dump_sexp(root_node);
    return 0;
  }

  kiji::CompUnit cu(interp.main_thread());
  kiji::Compiler compiler(cu);
  compiler.compile(root_node);
  if (state->dump_bytecode) {
    cu.dump(interp.vm());
  } else {
    interp.run(cu);
  }

  return 0;
}
