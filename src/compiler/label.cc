/* vim:ts=2:sw=2:tw=0:
 */

extern "C" {
#include "moarvm.h"
}
#include "../compiler.h"
#include "../asm.h"
#include "label.h"

void Kiji_label_put(KijiLabel *self, KijiCompiler *compiler) {
    assert(self->address == -1);
    self->address = Kiji_compiler_top_frame(compiler)->frame.bytecode_size;

    // rewrite reserved addresses
    for (auto r: self->reserved_addresses) {
        Kiji_asm_write_uint32_t_for(&(*(Kiji_compiler_top_frame(compiler))), self->address, r);
    }
    self->reserved_addresses.empty();
}

bool Kiji_label_is_solved(const KijiLabel *self) {
  return self->address!=-1;
}

void Kiji_label_reserve(KijiLabel *self, ssize_t address) {
  self->reserved_addresses.push_back(address);
}
