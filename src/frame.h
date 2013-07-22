// vim:ts=2:sw=2:tw=0:

/*
=for apidoc Am|void|Renewc|void* ptr|int nitems|type|cast
The XSUB-writer's interface to the C C<realloc> function, with
cast.
*/

#pragma once

#include <sstream>
#include "handy.h"

enum Kiji_variable_type_t {
  KIJI_VARIABLE_TYPE_MY,
  KIJI_VARIABLE_TYPE_OUR
};

typedef struct _KijiFrame {
public:
  MVMString **package_variables;
  MVMuint32 num_package_variables;
  MVMStaticFrame frame; // frame itself
  struct _KijiFrame* outer;

  public:
  void push_handler(MVMFrameHandler* handler) {
    frame.num_handlers++;
    Renew(frame.handlers, frame.num_handlers, MVMFrameHandler);
    frame.handlers[frame.num_handlers-1] = *handler;
  }

  // reserve register
  int push_local_type(MVMuint16 reg_type) {
      frame.num_locals++;
      Renew(frame.local_types, frame.num_locals, MVMuint16);
      frame.local_types[frame.num_locals-1] = reg_type;
      if (frame.num_locals >= 65535) {
        printf("[panic] Too much registers\n");
        abort();
      }
      return frame.num_locals-1;
  }
  // Get register type at 'n'
  uint16_t get_local_type(int n) {
      assert(n>=0);
      assert(n<frame.num_locals);
      return frame.local_types[n];
  }

  // Push lexical variable.
  int push_lexical(MVMThreadContext *tc, const std::string&name_cc, MVMuint16 type) {
      frame.num_lexicals++;
      Renew(frame.lexical_types, frame.num_lexicals, MVMuint16);
      frame.lexical_types[frame.num_lexicals-1] = type;

      int idx = frame.num_lexicals-1;

      MVMLexicalHashEntry *entry = (MVMLexicalHashEntry*)calloc(sizeof(MVMLexicalHashEntry), 1);
      entry->value = idx;

      MVMString* name = MVM_string_utf8_decode(tc, tc->instance->VMString, name_cc.c_str(), name_cc.size());
      MVM_string_flatten(tc, name);
      // lexical_names is Hash.
      MVM_HASH_BIND(tc, frame.lexical_names, name, entry);

      return idx;
  }

  // TODO Throw exception at this code: `our $n; my $n`
  void push_pkg_var(MVMString *name) {
    num_package_variables++;
    Renew(package_variables, num_package_variables, MVMString*);
    package_variables[num_package_variables-1] = name;
  }

  void set_outer(struct _KijiFrame*framef) {
      frame.outer = &(framef->frame);
      this->outer = &(*framef);
  }

  Kiji_variable_type_t find_variable_by_name(MVMThreadContext* tc, MVMString * name, int &lex_no, int &outer) {
      struct _KijiFrame* f = this;
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

      f = &(*(f->outer));
      ++outer;
      }
      // TODO I should use MVM_panic instead.
      printf("Unknown lexical variable in find_variable_by_name: ");
      MVM_string_say(tc, name);
      exit(0);
  }

} KijiFrame;

bool Kiji_frame_find_lexical_by_name(KijiFrame* frame_, MVMThreadContext* tc, const MVMString* name, int *lex_no, int *outer);

