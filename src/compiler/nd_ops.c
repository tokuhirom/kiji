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

ND(NODE_BIN_AND) {
  return Kiji_compiler_binary_binop(self, node, MVM_OP_band_i);
}
ND(NODE_BIN_OR) {
  return Kiji_compiler_binary_binop(self, node, MVM_OP_bor_i);
}
ND(NODE_BIN_XOR) {
  return Kiji_compiler_binary_binop(self, node, MVM_OP_bxor_i);
}
ND(NODE_MUL) {
  return Kiji_compiler_numeric_binop(self, node, MVM_OP_mul_i, MVM_OP_mul_n);
}
ND(NODE_SUB) {
  return Kiji_compiler_numeric_binop(self, node, MVM_OP_sub_i, MVM_OP_sub_n);
}
ND(NODE_DIV) {
  return Kiji_compiler_numeric_binop(self, node, MVM_OP_div_i, MVM_OP_div_n);
}
ND(NODE_ADD) {
  return Kiji_compiler_numeric_binop(self, node, MVM_OP_add_i, MVM_OP_add_n);
}
ND(NODE_MOD) {
  return Kiji_compiler_numeric_binop(self, node, MVM_OP_mod_i, MVM_OP_mod_n);
}
ND(NODE_POW) {
  return Kiji_compiler_numeric_binop(self, node, MVM_OP_pow_i, MVM_OP_pow_n);
}
ND(NODE_INPLACE_ADD) {
  return Kiji_compiler_numeric_inplace(self, node, MVM_OP_add_i, MVM_OP_add_n);
}
ND(NODE_INPLACE_SUB) {
  return Kiji_compiler_numeric_inplace(self, node, MVM_OP_sub_i, MVM_OP_sub_n);
}
ND(NODE_INPLACE_MUL) {
  return Kiji_compiler_numeric_inplace(self, node, MVM_OP_mul_i, MVM_OP_mul_n);
}
ND(NODE_INPLACE_DIV) {
  return Kiji_compiler_numeric_inplace(self, node, MVM_OP_div_i, MVM_OP_div_n);
}
ND(NODE_INPLACE_POW) { // **=
  return Kiji_compiler_numeric_inplace(self, node, MVM_OP_pow_i, MVM_OP_pow_n);
}
ND(NODE_INPLACE_MOD) { // %=
  return Kiji_compiler_numeric_inplace(self, node, MVM_OP_mod_i, MVM_OP_mod_n);
}
ND(NODE_INPLACE_BIN_OR) { // +|=
  return Kiji_compiler_binary_inplace(self, node, MVM_OP_bor_i);
}
ND(NODE_INPLACE_BIN_AND) { // +&=
  return Kiji_compiler_binary_inplace(self, node, MVM_OP_band_i);
}
ND(NODE_INPLACE_BIN_XOR) { // +^=
  return Kiji_compiler_binary_inplace(self, node, MVM_OP_bxor_i);
}
ND(NODE_INPLACE_BLSHIFT) { // +<=
  return Kiji_compiler_binary_inplace(self, node, MVM_OP_blshift_i);
}
ND(NODE_INPLACE_BRSHIFT) { // +>=
  return Kiji_compiler_binary_inplace(self, node, MVM_OP_brshift_i);
}
ND(NODE_INPLACE_CONCAT_S) { // ~=
  return Kiji_compiler_str_inplace(self, node, MVM_OP_concat_s, MVM_reg_str);
}
ND(NODE_INPLACE_REPEAT_S) { // x=
  return Kiji_compiler_str_inplace(self, node, MVM_OP_repeat_s, MVM_reg_int64);
}
ND(NODE_UNARY_TILDE) { // ~
  return Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
}
