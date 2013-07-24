/* vim:ts=2:sw=2:tw=0:
 */
#include "moarvm.h"
#include "../compiler.h"

/* lexical variable number by name */
bool Kiji_compiler_find_lexical_by_name(KijiCompiler *self, MVMString *name, int *lex_no, int *outer) {
  return Kiji_frame_find_lexical_by_name(Kiji_compiler_top_frame(self), self->tc, name, lex_no, outer) == KIJI_TRUE;
}

size_t Kiji_compiler_bytecode_size(KijiCompiler*self) {
    return Kiji_compiler_top_frame(self)->frame.bytecode_size;
}


int Kiji_compiler_push_string(KijiCompiler *self, MVMString *str) {
  self->cu->num_strings++;
  self->cu->strings = (MVMString**)realloc(self->cu->strings, sizeof(MVMString*)*self->cu->num_strings);
  if (!self->cu->strings) {
    MVM_panic(MVM_exitcode_compunit, "Cannot allocate memory");
  }
  self->cu->strings[self->cu->num_strings-1] = str;
  return self->cu->num_strings-1;
}

