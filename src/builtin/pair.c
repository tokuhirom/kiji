// vim:ts=2:sw=2:tw=0:
#include <moarvm.h>
#include "../xsub.h"

/* Note. Pair() is not implemented in MoarVM 6model yet. */

MVMObject * Kiji_bootstrap_Pair(MVMCompUnit* cu, MVMThreadContext*tc) {
  MVMObject * type = MVM_6model_find_method(
    tc,
    STABLE(tc->instance->KnowHOW)->HOW,
    MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"new_type")
  );
  MVMObject * how = STABLE(type)->HOW;

  CLASS_INIT();
  /* CLASS_ADD_METHOD("elems",   Hash_elems); */
  CLASS_REGISTER(type);

  return type;
}

