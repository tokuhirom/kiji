/* vim:ts=2:sw=2:tw=0:
 */

extern "C" {
#include "moarvm.h"
}
#include "../compiler.h"
#include "label.h"

void KijiLabel::put() {
    assert(address_ == -1);
    address_ = Kiji_compiler_top_frame(compiler_)->frame.bytecode_size;

    // rewrite reserved addresses
    for (auto r: reserved_addresses_) {
        Kiji_asm_write_uint32_t_for(&(*(Kiji_compiler_top_frame(compiler_))), address_, r);
    }
    reserved_addresses_.empty();
}

