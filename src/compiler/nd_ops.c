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

ND(NODE_POSTINC) { // $i++
  assert(node->children.size == 1);
  if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
    MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
  }
  MVMuint16 reg_no = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
  MVMuint16 i_tmp = Kiji_compiler_to_i(self, reg_no);
  ASM_INC_I(i_tmp);
  MVMuint16 dst_reg = Kiji_compiler_to_o(self, i_tmp);
  Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), dst_reg);
  return reg_no;
}
ND(NODE_PREINC) { // ++$i
  assert(node->children.size == 1);
  if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
    MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
  }
  MVMuint16 reg_no = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
  MVMuint16 i_tmp = Kiji_compiler_to_i(self, reg_no);
  ASM_INC_I(i_tmp);
  MVMuint16 dst_reg = Kiji_compiler_to_o(self, i_tmp);
  Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), dst_reg);
  return dst_reg;
}
ND(NODE_PREDEC) { // --$i
  assert(node->children.size == 1);
  if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
    MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
  }
  MVMuint16 reg_no = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
  MVMuint16 i_tmp = Kiji_compiler_to_i(self, reg_no);
  ASM_DEC_I(i_tmp);
  MVMuint16 dst_reg = Kiji_compiler_to_o(self, i_tmp);
  Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), dst_reg);
  return dst_reg;
}

ND(NODE_UNARY_BITWISE_NEGATION) { // +^1
  MVMuint16 reg = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  ASM_BNOT_I(reg, reg);
  return reg;
}
ND(NODE_BRSHIFT) { // +>
  MVMuint16 l = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  MVMuint16 r = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
  ASM_BRSHIFT_I(r, l, r);
  return r;
}
ND(NODE_BLSHIFT) { // +<
  MVMuint16 l = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  MVMuint16 r = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
  ASM_BLSHIFT_I(r, l, r);
  return r;
}

ND(NODE_ABS) {
  // TODO support abs_n?
  auto r = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  ASM_ABS_I(r, r);
  return r;
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

ND(NODE_BIND) {
  PVIPNode* lhs = node->children.nodes[0];
  PVIPNode* rhs = node->children.nodes[1];
  if (lhs->type == PVIP_NODE_MY) {
    // my $var := foo;
    int lex_no = Kiji_compiler_do_compile(self, lhs);
    int val    = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, rhs));
    ASM_BINDLEX(
      lex_no, // lex number
      0,      // frame outer count
      val     // value
    );
    return val;
  } else if (lhs->type == PVIP_NODE_OUR) {
    // our $var := foo;
    assert(lhs->children.nodes[0]->type == PVIP_NODE_VARIABLE);
    MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, lhs->children.nodes[0]->pv->buf, lhs->children.nodes[0]->pv->len);
    Kiji_compiler_push_pkg_var(self, name);
    int varname = Kiji_compiler_push_string(self, newMVMStringFromPVIP(lhs->children.nodes[0]->pv));
    int val    = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, rhs));
    int outer = 0;
    int lex_no = 0;
    if (!Kiji_compiler_find_lexical_by_name(self, newMVMString_nolen("$?PACKAGE"), &lex_no, &outer)) {
      MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", "$?PACKAGE");
    }
    MVMuint16 package = REG_OBJ();
    ASM_GETLEX(
      package,
      lex_no,
      outer // outer frame
    );
    // TODO getwho
    MVMuint16 varname_s = REG_STR();
    ASM_CONST_S(varname_s, varname);
    // 0x0F    bindkey_o           r(obj) r(str) r(obj)
    ASM_BINDKEY_O(
      package,
      varname_s,
      val
    );
    return val;
  } else if (lhs->type == PVIP_NODE_VARIABLE) {
    // $var := foo;
    int val    = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, rhs));
    Kiji_compiler_set_variable(self, newMVMStringFromPVIP(lhs->pv), val);
    return val;
  } else {
    printf("You can't bind value to %s, currently.\n", PVIP_node_name(lhs->type));
    abort();
  }
}

ND(NODE_STRING_CONCAT) {
  MVMuint16 dst_reg = REG_STR();
  PVIPNode* lhs = node->children.nodes[0];
  PVIPNode* rhs = node->children.nodes[1];
  MVMuint16 l = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, lhs));
  MVMuint16 r = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, rhs));
  ASM_CONCAT_S(
    dst_reg,
    l,
    r
  );
  return dst_reg;
}

