// vim:ts=2:sw=2:tw=0:
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

Kiji_variable_type_t Kiji_find_variable_by_name(KijiFrame *f, MVMThreadContext* tc, MVMString * name, int &lex_no, int &outer) {
    outer = 0;
    while (f) {
      // check lexical variables
      MVMLexicalHashEntry *lexical_names = f->frame.lexical_names;
      MVMLexicalHashEntry *entry;
      MVM_HASH_GET(tc, lexical_names, name, entry);

      if (entry) {
          lex_no = entry->value;
          return KIJI_VARIABLE_TYPE_MY;
      }

      // check package variables
      for (int i=0; i<f->num_package_variables; i++) {
        auto n = f->package_variables[i];
        if (MVM_string_equal(tc, n, name)) {
          return KIJI_VARIABLE_TYPE_OUR;
        }
      }

      f = f->outer;
      ++outer;
    }
    // TODO I should use MVM_panic instead.
    printf("Unknown lexical variable in find_variable_by_name: ");
    MVM_string_say(tc, name);
    exit(0);
}

void Kiji_frame_set_outer(KijiFrame *self, KijiFrame*framef) {
    self->frame.outer = &(framef->frame);
    self->outer = framef;
}

// TODO Throw exception at this code: `our $n; my $n`
void Kiji_frame_push_pkg_var(KijiFrame* self, MVMString *name) {
  self->num_package_variables++;
  Renew(self->package_variables, self->num_package_variables, MVMString*);
  self->package_variables[self->num_package_variables-1] = name;
}
