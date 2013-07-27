/* vim:ts=2:sw=2:tw=0:
 */

#include "moarvm.h"
#include "nd.h"

/* $i-- */
ND(NODE_POSTDEC) {
  assert(node->children.size == 1);
  if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
    MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
  }
  MVMuint16 reg_no = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
  MVMuint16 i_tmp = Kiji_compiler_to_i(self, reg_no);
  ASM_DEC_I(i_tmp);
  Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), Kiji_compiler_to_o(self, i_tmp));
  return reg_no;
}

