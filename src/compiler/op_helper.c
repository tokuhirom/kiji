/* vim:ts=2:sw=2:tw=0:
 */
#include "moarvm.h"
#include "../compiler.h"
#include "../gen.assembler.h"
#include "../asm.h"

/* This reg returns register number contains true value. */
int Kiji_compiler_const_true(KijiCompiler *self) {
  auto reg = REG_INT64();
  ASM_CONST_I64(reg, 1);
  return reg;
}

void Kiji_compiler_goto(KijiCompiler*self, KijiLabel *label) {
  if (!Kiji_label_is_solved(label)) {
    Kiji_label_reserve(label, Kiji_compiler_bytecode_size(self) + 2);
  }
  ASM_GOTO(label->address);
}

void Kiji_compiler_unless_any(KijiCompiler *self, uint16_t reg, KijiLabel *label) {
  if (!Kiji_label_is_solved(label)) {
    Kiji_label_reserve(label, Kiji_compiler_bytecode_size(self) + 2 + 2);
  }
  ASM_OP_U16_U32(MVM_OP_BANK_primitives, Kiji_compiler_unless_op(self, reg), reg, label->address);
}

void Kiji_compiler_if_any(KijiCompiler *self, uint16_t reg, KijiLabel *label) {
  if (!Kiji_label_is_solved(label)) {
    Kiji_label_reserve(label, Kiji_compiler_bytecode_size(self) + 2 + 2);
  }
  ASM_OP_U16_U32(MVM_OP_BANK_primitives, Kiji_compiler_if_op(self, reg), reg, label->address);
}

void Kiji_compiler_return_any(KijiCompiler *self, uint16_t reg) {
  switch (Kiji_compiler_get_local_type(self, reg)) {
  case MVM_reg_int64:
    ASM_RETURN_I(reg);
    break;
  case MVM_reg_str:
    ASM_RETURN_S(reg);
    break;
  case MVM_reg_obj:
    ASM_RETURN_O(reg);
    break;
  case MVM_reg_num64:
    ASM_RETURN_N(reg);
    break;
  default: abort();
  }
}

uint16_t Kiji_compiler_unless_op(KijiCompiler * self, uint16_t cond_reg) {
  switch (Kiji_compiler_get_local_type(self, cond_reg)) {
  case MVM_reg_int64:
    return MVM_OP_unless_i;
  case MVM_reg_num64:
    return MVM_OP_unless_n;
  case MVM_reg_str:
    return MVM_OP_unless_s;
  case MVM_reg_obj:
    return MVM_OP_unless_o;
  default:
    abort();
  }
}

uint16_t Kiji_compiler_if_op(KijiCompiler* self, uint16_t cond_reg) {
  switch (Kiji_compiler_get_local_type(self, cond_reg)) {
  case MVM_reg_int64:
    return MVM_OP_if_i;
  case MVM_reg_num64:
    return MVM_OP_if_n;
  case MVM_reg_str:
    return MVM_OP_if_s;
  case MVM_reg_obj:
    return MVM_OP_if_o;
  default:
    abort();
  }
}
