#include "moarvm.h"
#include "../compiler.h"

size_t Kiji_compiler_bytecode_size(KijiCompiler*self) {
    return Kiji_compiler_top_frame(self)->frame.bytecode_size;
}

