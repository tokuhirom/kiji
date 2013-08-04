/* vim:ts=2:sw=2:tw=0
 */

#include "moarvm.h"
#include "nd.h"
#include "../handy.h"


ND(NODE_STRING) {
  int str_num = Kiji_compiler_push_string(self, newMVMStringFromPVIP(node->pv));
  MVMuint16 reg_num = REG_STR();
  ASM_CONST_S(reg_num, str_num);
  return reg_num;
}

ND(NODE_INT) {
  MVMuint16 reg_num = REG_INT64();
  int64_t n = node->iv;
  ASM_CONST_I64(reg_num, n);
  return reg_num;
}

ND(NODE_NUMBER) {
  MVMuint16 reg_num = REG_NUM64();
  MVMnum64 n = node->nv;
  ASM_CONST_N64(reg_num, n);
  return reg_num;
}

ND(NODE_VARIABLE) {
  // copy lexical variable to register
  return Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->pv));
}

ND(NODE_CLARGS) { // @*ARGS
  MVMuint16 retval = REG_OBJ();
  ASM_WVAL(retval, KIJI_SC_BUILTIN_TWARGS,0);
  return retval;
}

ND(NODE_CLASS) {
  return Kiji_compiler_compile_class(self, node);
}

ND(NODE_IDENT) {
  int lex_no, outer;

  // auto lex_no = find_lexical_by_name(std::string(node->pv->buf, node->pv->len), outer);
  // class Foo { }; Foo;
  if (Kiji_compiler_find_lexical_by_name(self, newMVMStringFromPVIP(node->pv), &lex_no, &outer)) {
    MVMuint16 reg_no = REG_OBJ();
    ASM_GETLEX(
      reg_no,
      lex_no,
      outer // outer frame
    );
    return reg_no;
  // sub fooo { }; foooo;
  } else {
    PVIPString *amp_name = PVIP_string_new();
    PVIP_string_concat(amp_name, "&", strlen("&"));
    PVIP_string_concat(amp_name, node->pv->buf, node->pv->len);
    if (Kiji_compiler_find_lexical_by_name(self, newMVMStringFromPVIP(amp_name), &lex_no, &outer)) {
      PVIP_string_destroy(amp_name);
      MVMuint16 func_reg_no = REG_OBJ();
      ASM_GETLEX(
        func_reg_no,
        lex_no,
        outer // outer frame
      );

      MVMCallsite* callsite;
      Newxz(callsite, 1, MVMCallsite);
      callsite->arg_count = 0;
      callsite->num_pos = 0;
      Newxz(callsite->arg_flags, 0, MVMCallsiteEntry);

      int callsite_no = Kiji_compiler_push_callsite(self, callsite);
      ASM_PREPARGS(callsite_no);

      MVMuint16 dest_reg = REG_OBJ(); // ctx
      ASM_INVOKE_O(
          dest_reg,
          func_reg_no
      );
      return dest_reg;
    }
    PVIP_string_destroy(amp_name);
  }
  PVIP_string_concat(node->pv, "\0", 1);
  MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", node->pv->buf);
}

ND(NODE_LIST) {
  // create array
  MVMuint16 array_reg = REG_OBJ();
  ASM_HLLLIST(array_reg);
  ASM_CREATE(array_reg, array_reg);

  // push elements
  int i;
  for (i=0; i<node->children.size; ++i) {
    PVIPNode*n = node->children.nodes[i];
    Kiji_compiler_compile_array(self, array_reg, n);
  }
  return array_reg;
}
ND(NODE_ARRAY) {
  /* TODO: use 6model's container feature after released it. */
  // create array
  MVMuint16 array_reg = REG_OBJ();
  ASM_HLLLIST(array_reg);
  ASM_CREATE(array_reg, array_reg);

  // push elements
  int i;
  for (i=0; i<node->children.size; ++i) {
    PVIPNode*n = node->children.nodes[i];
    Kiji_compiler_compile_array(self, array_reg, n);
  }
  return array_reg;
}

/*
 * TODO I guess 6model commiters will be implement Pair() in MoarVM/src/6model/
 * I use array instead, for now.
 */
ND(NODE_PAIR) {
  MVMuint16 array_reg = REG_OBJ();
  ASM_HLLLIST(array_reg);
  ASM_CREATE(array_reg, array_reg);

  Kiji_compiler_compile_array(self, array_reg, node->children.nodes[0]);
  return array_reg;
}

ND(NODE_HASH) {
  MVMuint16 hashtype = REG_OBJ();
  MVMuint16 hash     = REG_OBJ();
  ASM_HLLHASH(hashtype);
  ASM_CREATE(hash, hashtype);
  int i;
  for (i=0; i<node->children.size; i++) {
    PVIPNode* pair = node->children.nodes[i];
    assert(pair->type == PVIP_NODE_PAIR);
    MVMuint16 k = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, pair->children.nodes[0]));
    MVMuint16 v = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, pair->children.nodes[1]));
    ASM_BINDKEY_O(hash, k, v);
  }
  return hash;
}

