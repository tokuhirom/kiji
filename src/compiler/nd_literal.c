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

