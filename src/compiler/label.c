/* vim:ts=2:sw=2:tw=0:
 */

#include "moarvm.h"
#include <stdint.h>
#include <stdio.h>
#include "label.h"
#include "../compiler.h"
#include "../asm.h"

void Kiji_label_init(KijiLabel *self) {
  self->address = -1;
  self->reserved_addresses = NULL;
  self->num_reserved_addresses = 0;
}

void Kiji_label_put(KijiLabel *self, KijiCompiler *compiler) {
  assert(self->address == -1);
  self->address = Kiji_compiler_bytecode_size(compiler);

  /* rewrite reserved addresses */
  int i;
  for (i=0; i<self->num_reserved_addresses; ++i) {
    Kiji_asm_write_uint32_t_for(Kiji_compiler_top_frame(compiler), self->address, self->reserved_addresses[i]);
  }
  self->num_reserved_addresses = 0;
  Safefree(self->reserved_addresses);
}

bool Kiji_label_is_solved(const KijiLabel *self) {
  return self->address!=-1;
}

void Kiji_label_reserve(KijiLabel *self, ssize_t address) {
  if (self->address == -1) {
    self->num_reserved_addresses++;
    Renew(self->reserved_addresses, self->num_reserved_addresses, ssize_t);
    self->reserved_addresses[self->num_reserved_addresses-1] = address;
  } else {
    abort(); /* NYI */
  }
}
