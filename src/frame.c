// vim:ts=2:sw=2:tw=0:
#include <stdio.h>
#include "moarvm.h"
#include "frame.h"

// lexical variable number by name
Kiji_boolean Kiji_frame_find_lexical_by_name(KijiFrame* frame_, MVMThreadContext* tc, const MVMString* name, int *lex_no, int *outer) {
  MVMStaticFrame *f = &(frame_->frame);
  *outer = 0;
  while (f) {
    MVMLexicalHashEntry *lexical_names = f->lexical_names;
    MVMLexicalHashEntry *entry;
    MVM_HASH_GET(tc, lexical_names, name, entry);

    if (entry) {
      *lex_no= entry->value;
      return KIJI_TRUE;
    }
    f = f->outer;
    ++(*outer);
  }
  return KIJI_FALSE;
  // printf("Unknown lexical variable in find_lexical_by_name: %s\n", name_cc.c_str());
  // exit(0);
}

Kiji_variable_type_t Kiji_find_variable_by_name(KijiFrame *f, MVMThreadContext* tc, MVMString * name, int *lex_no, int *outer) {
    *outer = 0;
    while (f) {
      // check lexical variables
      MVMLexicalHashEntry *lexical_names = f->frame.lexical_names;
      MVMLexicalHashEntry *entry;
      MVM_HASH_GET(tc, lexical_names, name, entry);

      if (entry) {
          *lex_no = entry->value;
          return KIJI_VARIABLE_TYPE_MY;
      }

      // check package variables
      int i;
      for (i=0; i<f->num_package_variables; i++) {
        MVMString* n = f->package_variables[i];
        if (MVM_string_equal(tc, n, name)) {
          return KIJI_VARIABLE_TYPE_OUR;
        }
      }

      f = f->outer;
      ++(*outer);
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

/* Push lexical variable. */
int Kiji_frame_push_lexical(KijiFrame*self, MVMThreadContext *tc, MVMString *name, MVMuint16 type) {
    self->frame.num_lexicals++;
    Renew(self->frame.lexical_types, self->frame.num_lexicals, MVMuint16);
    self->frame.lexical_types[self->frame.num_lexicals-1] = type;

    int idx = self->frame.num_lexicals-1;

    MVMLexicalHashEntry *entry = (MVMLexicalHashEntry*)calloc(sizeof(MVMLexicalHashEntry), 1);
    entry->value = idx;

    MVM_string_flatten(tc, name);
    // lexical_names is Hash.
    MVM_HASH_BIND(tc, self->frame.lexical_names, name, entry);

    return idx;
}

uint16_t Kiji_frame_get_local_type(KijiFrame*self, int n) {
  assert(n>=0);
  assert(n<self->frame.num_locals);
  return self->frame.local_types[n];
}

// reserve register
int Kiji_frame_push_local_type(KijiFrame* self, MVMuint16 reg_type) {
  self->frame.num_locals++;
  Renew(self->frame.local_types, self->frame.num_locals, MVMuint16);
  self->frame.local_types[self->frame.num_locals-1] = reg_type;
  if (self->frame.num_locals >= 65535) {
    printf("[panic] Too much registers\n");
    abort();
  }
  return self->frame.num_locals-1;
}

void Kiji_frame_push_handler(KijiFrame*self, MVMFrameHandler* handler) {
  self->frame.num_handlers++;
  Renew(self->frame.handlers, self->frame.num_handlers, MVMFrameHandler);
  self->frame.handlers[self->frame.num_handlers-1] = *handler;
}

void Kiji_frame_add_exportable(KijiFrame *self, MVMThreadContext *tc, MVMString *name, int frame_no) {
  /* extend area */
  KijiFrame *target = self;
  while (target->outer) {
    if (target->frame_type == KIJI_FRAME_TYPE_USE) {
      break;
    }
    target = target->outer;
  }
  target->num_exportables++;
  Renew(target->exportables, target->num_exportables, KijiExportableEntry);

  /* And store data */
  Newxz(target->exportables[target->num_exportables-1], 1, KijiExportableEntry);
  KijiExportableEntry* e = target->exportables[target->num_exportables-1];
  e->frame_no = frame_no;
  e->name     = name;
}

