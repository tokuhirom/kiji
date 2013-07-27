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

void Kiji_compiler_push_sc_object(KijiCompiler *self, MVMObject * object, int *wval1, int *wval2) {
  self->num_sc_classes++;

  *wval1 = 1;
  *wval2 = self->num_sc_classes-1;

  MVM_sc_set_object(self->tc, self->sc_classes, self->num_sc_classes-1, object);
}

/* Push lexical variable. */
int Kiji_compiler_push_lexical(KijiCompiler *self, MVMString *name, MVMuint16 type) {
  return Kiji_frame_push_lexical(Kiji_compiler_top_frame(self), self->tc, name, type);
}

void Kiji_compiler_push_pkg_var(KijiCompiler *self, MVMString *name) {
  Kiji_frame_push_pkg_var(Kiji_compiler_top_frame(self), name);
}

void Kiji_compiler_pop_frame(KijiCompiler* self) {
  self->num_frames--;
  Renew(self->frames, self->num_frames, KijiFrame*);
  /* You don't  need to free the frame itself */
}

int Kiji_compiler_push_frame(KijiCompiler* self, const char* name, size_t name_len) {
  MVMThreadContext *tc = self->tc;
  assert(tc);
  char *buf = (char*)malloc((name_len+32)*sizeof(char));
  int len = snprintf(buf, name_len+31, "%s%d", name, self->frame_no++);
  // TODO Newxz
  KijiFrame* frame = (KijiFrame*)malloc(sizeof(KijiFrame));
  memset(frame, 0, sizeof(KijiFrame));
  frame->frame.name = MVM_string_utf8_decode(tc, tc->instance->VMString, buf, len);
  free(buf);
  if (self->num_frames != 0) {
    Kiji_frame_set_outer(frame, Kiji_compiler_top_frame(self));
  }
  
  /* push frame */
  self->num_frames++;
  Renew(self->frames, self->num_frames, KijiFrame*);
  self->frames[self->num_frames-1] = frame;

  /* store to compunit, too. */
  MVMCompUnit *cu = self->cu;
  cu->num_frames++;
  Renew(cu->frames, cu->num_frames, MVMStaticFrame*);
  cu->frames[cu->num_frames-1] = &(Kiji_compiler_top_frame(self)->frame);
  cu->frames[cu->num_frames-1]->cu = cu;
  cu->frames[cu->num_frames-1]->work_size = 0;
  return cu->num_frames-1;
}

