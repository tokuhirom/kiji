#include <stdio.h>
extern "C" {
#include "moarvm.h"
};
#include "frame.h"

// lexical variable number by name
bool Kiji_frame_find_lexical_by_name(KijiFrame* frame_, MVMThreadContext* tc, const MVMString* name, int *lex_no, int *outer) {
  MVMStaticFrame *f = &(frame_->frame);
  *outer = 0;
  while (f) {
    MVMLexicalHashEntry *lexical_names = f->lexical_names;
    MVMLexicalHashEntry *entry;
    MVM_HASH_GET(tc, lexical_names, name, entry);

    if (entry) {
      *lex_no= entry->value;
      return true;
    }
    f = f->outer;
    ++(*outer);
  }
  return false;
  // printf("Unknown lexical variable in find_lexical_by_name: %s\n", name_cc.c_str());
  // exit(0);
}
