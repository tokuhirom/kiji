#ifndef KIJI_XSUB_H_
#define KIJI_XSUB_H_

#define CLASS_INIT() \
  MVMObject* _cache = REPR(tc->instance->boot_types->BOOTHash)->allocate(tc, STABLE(tc->instance->boot_types->BOOTHash)); \
  MVMObject *_BOOTCCode = tc->instance->boot_types->BOOTCCode;

#define CLASS_REGISTER(type) \
  STABLE(type)->method_cache = _cache;

#define CLASS_ADD_METHOD(name, funcref) \
  do { \
    MVMString *string = MVM_string_utf8_decode(tc, tc->instance->VMString, name, strlen(name)); \
    MVMObject *code_obj = REPR(_BOOTCCode)->allocate(tc, STABLE(_BOOTCCode)); \
    ((MVMCFunction *)code_obj)->body.func = funcref; \
    REPR(_cache)->ass_funcs->bind_key_boxed( \
        tc, \
        STABLE(_cache), \
        _cache, \
        OBJECT_BODY(_cache), \
        (MVMObject*)string, \
        code_obj); \
  } while (0)

#endif /* KIJI_XSUB_H_ */
