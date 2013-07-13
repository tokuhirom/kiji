// vim:ts=2:sw=2:tw=0:


#include <stdio.h>
#include <assert.h>
#include <fstream>
#ifdef __unix__
#include <unistd.h>
#endif
extern "C" {
#include <moarvm.h>
}
#include "compiler.h"


void run_repl() {
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
  // This include apr_initialize().
  // You need to initialize before `apr_pool_create()`
  kiji::Interpreter interp;

  apr_status_t rv;
  apr_pool_t *mp;
  static const apr_getopt_option_t opt_option[] = {
      /* values must be greater than 255 so it doesn't have a single-char form
          Otherwise, use a character such as 'h' */
      { "dump-bytecode", 256, 0, "dump bytecode" },
      { "help", 257, 0, "show help" },
      { "dump-ast", 258, 0, "dump ast" },
      { NULL, 'e', 1, "eval" },
      { NULL, 0, 0, NULL }
  };
  apr_getopt_t *opt;
  int optch;
  const char *optarg;
  const char *eval = NULL;
  bool dump_ast = false;
  bool dump_bytecode = false;
  const char *helptext = "\
  MoarVM usage: moarvm [options] bytecode.moarvm [program args]           \n\
    --help, display this message                                          \n\
    --dump, dump the bytecode to stdout instead of executing              \n";
  int processed_args = 0;

  apr_pool_create(&mp, NULL);
  apr_getopt_init(&opt, mp, argc, argv);
  while ((rv = apr_getopt_long(opt, opt_option, &optch, &optarg)) == APR_SUCCESS) {
      switch (optch) {
      case 'e':
        eval = strdup(optarg);
        break;
      case 256:
        dump_bytecode = true;
        break;
      case 257:
        printf("%s", helptext);
        exit(1);
      case 258:
        dump_ast = true;
        break;
      }
  }

  processed_args = opt->ind;
  PVIPNode *root_node;
  if (eval) {
    root_node = PVIP_parse_string(eval, strlen(eval), 0);
  } else if (processed_args == argc) {
#ifdef __unix__
    if (isatty(fileno(stdin))) {
      run_repl();
      exit(0);
    }
#endif

    root_node = PVIP_parse_fp(stdin, 0);
  } else {
    FILE *fp = fopen(opt->argv[processed_args++], "rb");
    if (!fp) {
      std::cerr << "Cannot open file: " << opt->argv[processed_args-1] << ": " << strerror(errno) << std::endl;
      exit(1);
    }
    root_node = PVIP_parse_fp(fp, 0);
  }
  // stash the rest of the raw command line args in the instance
  interp.set_clargs(argc - processed_args, (char **)(opt->argv + processed_args));
  /*
  instance->num_clargs = argc - processed_args;
  instance->raw_clargs = (char **)(opt->argv + processed_args);
  */

  if (!root_node) {
    exit(1);
  }

  if (dump_ast) {
    PVIP_node_dump_sexp(root_node);
    return 0;
  }

  kiji::CompUnit cu(interp.main_thread());
  kiji::Compiler compiler(cu);
  compiler.compile(root_node);
  if (dump_bytecode) {
    cu.dump(interp.vm());
  } else {
    interp.run(cu);
  }

  return 0;
}
