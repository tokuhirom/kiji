/* vim:ts=2:sw=2:tw=0:
 */
#include "moarvm.h"
#include "../compiler.h"
#include "../gen.assembler.h"

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
