/* vim:ts=2:sw=2:tw=0
 */
#include "moarvm.h"
#include "../compiler.h"
#include "../builtin.h"
#include "../gen.assembler.h"

#define dTC MVMThreadContext *tc=self->tc
#define dVM MVMInstance *vm=self->vm
#define dCU MVMCompUnit *cu=self->cu

int Kiji_compiler_do_compile(KijiCompiler *self, const PVIPNode*node) {
  return Kiji_compiler_compile_nodes(self, node);
}

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

void Kiji_compiler_push_sc(KijiCompiler *self, MVMSerializationContext*sc) {
  MVMCompUnit*cu = self->cu;
  cu->num_scs++;
  Renew(cu->scs, cu->num_scs, MVMSerializationContext*);
  cu->scs[cu->num_scs-1] = sc;
}

void Kiji_compiler_push_sc_object(KijiCompiler *self, MVMString *handle, MVMObject * object, int *wval1, int *wval2) {
  MVMCompUnit*cu = self->cu;

  MVMSerializationContext *sc = (MVMSerializationContext *)MVM_sc_create(self->tc, handle);
  MVM_sc_set_object(self->tc, sc, 0, object);

  Kiji_compiler_push_sc(self, sc);

  *wval1 = self->cu->num_scs-1;
  *wval2 = 0;
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
  PVIPString * name_with_id = PVIP_string_new();
  PVIP_string_concat(name_with_id, name, name_len);
  PVIP_string_printf(name_with_id, "%d", self->frame_no++);
  KijiFrame* frame;
  Newxz(frame, 1, KijiFrame);
  frame->frame.name = MVM_string_utf8_decode(tc, tc->instance->VMString, name_with_id->buf, name_with_id->len);
  PVIP_string_destroy(name_with_id);

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

Kiji_variable_type_t Kiji_compiler_find_variable_by_name(KijiCompiler* self, MVMString *name, int *lex_no, int *outer) {
  return Kiji_find_variable_by_name(Kiji_compiler_top_frame(self), self->tc, name, lex_no, outer);
}

void Kiji_compiler_finalize(KijiCompiler *self, MVMInstance* vm) {
  MVMThreadContext *tc = self->tc; // remove me
  MVMCompUnit *cu = self->cu;

  /* finalize frame */
  int i;
  for (i=0; i<cu->num_frames; i++) {
    MVMStaticFrame *frame = cu->frames[i];
    // XXX Should I need to time sizeof(MVMRegister)??
    frame->env_size = frame->num_lexicals * sizeof(MVMRegister);
    Newxz(frame->static_env, frame->env_size, MVMRegister);

    char buf[1023+1];
    int len = snprintf(buf, 1023, "frame_cuuid_%d", i);
    frame->cuuid = MVM_string_utf8_decode(tc, tc->instance->VMString, buf, len);
  }

  cu->main_frame = cu->frames[0];
  assert(cu->main_frame->cuuid);

  // Creates code objects to go with each of the static frames.
  // ref create_code_objects in src/core/bytecode.c
  Newxz(cu->coderefs, cu->num_frames, MVMObject*);

  MVMObject* code_type = tc->instance->boot_types->BOOTCode;

  for (i = 0; i < cu->num_frames; i++) {
    cu->coderefs[i] = REPR(code_type)->allocate(tc, STABLE(code_type));
    ((MVMCode *)cu->coderefs[i])->body.sf = cu->frames[i];
  }
}

/* Initialize @*ARGS */
void Kiji_compiler_setup_twargs(KijiCompiler *self) {
  dTC; dVM; dCU;

  MVMObject *clargs = MVM_repr_alloc_init(tc, tc->instance->boot_types->BOOTArray);
  MVM_gc_root_add_permanent(tc, (MVMCollectable **)&clargs);
  MVMString* handle = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"@*ARGS");
  MVMint64 count;
  for (count = 0; count < vm->num_clargs; count++) {
    MVMString *string = MVM_string_utf8_decode(tc,
      tc->instance->VMString,
      vm->raw_clargs[count], strlen(vm->raw_clargs[count])
    );
    MVMObject*type = cu->hll_config->str_box_type;
    MVMObject *box = REPR(type)->allocate(tc, STABLE(type));
    if (REPR(box)->initialize)
        REPR(box)->initialize(tc, STABLE(box), box, OBJECT_BODY(box));
    REPR(box)->box_funcs->set_str(tc, STABLE(box), box,
        OBJECT_BODY(box), string);
    MVM_repr_push_o(tc, clargs, box);
  }
  int wval1, wval2;
  Kiji_compiler_push_sc_object(self, handle, clargs, &wval1, &wval2);
}

void Kiji_compiler_compile(KijiCompiler *self, PVIPNode*node, MVMInstance* vm) {
  MVMCompUnit *cu = self->cu;
  MVMThreadContext *tc = self->tc;

  // setup hllconfig
  {
    MVMString *hll_name = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"kiji");
    cu->hll_config = MVM_hll_get_config_for(tc, hll_name);

    MVMObject *config = REPR(tc->instance->boot_types->BOOTHash)->allocate(tc, STABLE(tc->instance->boot_types->BOOTHash));
    MVM_hll_set_config(tc, hll_name, config);
  }

  /* setup builtin scs */
  Kiji_compiler_setup_twargs(self);

  /* Pair class */
  /*
  {
    MVMObject *pair_type = Kiji_bootstrap_Pair(self->cu, self->tc);
    MVM_sc_set_object(tc, sc, KIJI_SC_BUILTIN_PAIR_CLASS, pair_type);
  }
  */

  ASM_CHECKARITY(0, -1);

  /*
  int code = REG_OBJ();
  int dest_reg = REG_OBJ();
  ASM_WVAL(code, 0, 1);
  MVMCallsite* callsite = new MVMCallsite;
  memset(callsite, 0, sizeof(MVMCallsite));
  callsite->arg_count = 0;
  callsite->num_pos = 0;
  callsite->arg_flags = new MVMCallsiteEntry[0];
  // callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;;

  auto callsite_no = interp_.push_callsite(callsite);
  ASM_PREPARGS(callsite_no);
  ASM_INVOKE_O( dest_reg, code);
  */

  // bootstrap $?PACKAGE
  // TODO I should wrap it to any object. And set WHO.
  // $?PACKAGE.WHO<bar> should work.
  {
    MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, "$?PACKAGE", strlen("$?PACKAGE"));
    int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
    MVMuint16 package = REG_OBJ();
    MVMuint16 hash_type = REG_OBJ();
    ASM_HLLHASH(hash_type);
    ASM_CREATE(package, hash_type);
    ASM_BINDLEX(lex, 0, package);
  }

  Kiji_compiler_do_compile(self, node);

  // bootarray
  /*
  int ary = interp_.push_local_type(MVM_reg_obj);
  ASM_BOOTARRAY(ary);
  ASM_CREATE(ary, ary);
  ASM_PREPARGS(1);
  ASM_INVOKE_O(ary, ary);

  ASM_BOOTSTRARRAY(ary);
  ASM_CREATE(ary, ary);
  ASM_PREPARGS(1);
  ASM_INVOKE_O(ary, ary);
  */

  // final op must be return.
  int reg = REG_OBJ();
  ASM_NULL(reg);
  ASM_RETURN_O(reg);

  // hacking hll
  Kiji_bootstrap_Array(cu, tc);
  Kiji_bootstrap_Str(cu,   tc);
  Kiji_bootstrap_Hash(cu,  tc);
  Kiji_bootstrap_File(cu,  tc);
  Kiji_bootstrap_Int(cu,   tc);

  // finalize callsite
  cu->max_callsite_size = 0;
  int i;
  for (i=0; i<cu->num_callsites; i++) {
    MVMCallsite *callsite = cu->callsites[i];
    cu->max_callsite_size = MAX(cu->max_callsite_size, callsite->arg_count);
  }

  /* fix scs_to_resolve ... (is this valid thing?) */
  {
    cu->scs_to_resolve = (MVMString**)malloc(sizeof(MVMString*)*cu->num_scs);
    for (i=0; i<cu->num_scs; ++i) {
      cu->scs_to_resolve[i] = NULL;
    }
  }
  // Kiji_compiler_dump_scs(self);
}

void Kiji_compiler_dump_scs(KijiCompiler *self) {
  dCU; dTC;

  int i;
  printf("  num_scs: %d\n", cu->num_scs);
  for (i=0; i<cu->num_scs; ++i) {
    MVMSerializationContext*sc = cu->scs[i];
    MVMString *handle = MVM_sc_get_handle(tc, sc);
    printf("    %d %s\n", i, MVM_string_utf8_encode_C_string(tc, handle));
  }
}

uint16_t Kiji_compiler_get_variable(KijiCompiler *self, MVMString *name) {
  int outer = 0;
  int lex_no = 0;
  Kiji_variable_type_t vartype = Kiji_compiler_find_variable_by_name(self, name, &lex_no, &outer);
  if (vartype==KIJI_VARIABLE_TYPE_MY) {
    MVMuint16 reg_no = REG_OBJ();
    ASM_GETLEX(
      reg_no,
      lex_no,
      outer // outer frame
    );
    return reg_no;
  } else {
    int lex_no;
    if (!Kiji_compiler_find_lexical_by_name(self, newMVMString_nolen("$?PACKAGE"), &lex_no, &outer)) {
      MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", "$?PACKAGE");
    }
    MVMuint16 reg = REG_OBJ();
    int varname = Kiji_compiler_push_string(self, name);
    MVMuint16 varname_s = REG_STR();
    ASM_GETLEX(
      reg,
      lex_no,
      outer // outer frame
    );
    ASM_CONST_S(
      varname_s,
      varname
    );
    // TODO getwho
    ASM_ATKEY_O(
      reg,
      reg,
      varname_s
    );
    return reg;
  }
}

void Kiji_compiler_set_variable(KijiCompiler *self, MVMString * name, uint16_t val_reg) {
  int lex_no = -1;
  int outer = -1;
  Kiji_variable_type_t vartype = Kiji_compiler_find_variable_by_name(self, name, &lex_no, &outer);
  if (vartype==KIJI_VARIABLE_TYPE_MY) {
    ASM_BINDLEX(
      lex_no,
      outer,
      val_reg
    );
  } else {
    int lex_no;
    if (!Kiji_compiler_find_lexical_by_name(self, newMVMString_nolen("$?PACKAGE"), &lex_no, &outer)) {
      MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", "$?PACKAGE");
    }
    MVMuint16 reg = REG_OBJ();
    int varname = Kiji_compiler_push_string(self, name);
    MVMuint16 varname_s = REG_STR();
    ASM_GETLEX(
      reg,
      lex_no,
      outer // outer frame
    );
    ASM_CONST_S(
      varname_s,
      varname
    );
    // TODO getwho
    ASM_BINDKEY_O(
      reg,
      varname_s,
      val_reg
    );
  }
}


void Kiji_compiler_init(KijiCompiler *self, MVMCompUnit * cu, MVMThreadContext * tc, MVMInstance *vm) {
  memset(self, 0, sizeof(KijiCompiler));
  self->cu = cu;
  self->tc = tc;
  self->vm = vm;
  self->frame_no = 0;

  // init compunit.
  Kiji_compiler_push_frame(self, "frame_name_0", strlen("frame_name_0"));

  self->current_class_how = NULL;
}

/* Is a and b equivalent? */
KIJI_STATIC_INLINE bool callsite_eq(MVMCallsite *a, MVMCallsite *b) {
  if (a->arg_count != b->arg_count) {
    return false;
  }
  if (a->num_pos != b->num_pos) {
    return false;
  }
  // Should I use memcmp?
  if (a->arg_count !=0) {
    int i;
    for (i=0; i<a->arg_count; ++i) {
      if (a->arg_flags[i] != b->arg_flags[i]) {
        return false;
      }
    }
  }
  return true;
}

size_t Kiji_compiler_push_callsite(KijiCompiler *self, MVMCallsite *callsite) {
  int i=0;
  for (i=0; i<self->cu->num_callsites; i++) {
    if (callsite_eq(self->cu->callsites[i], callsite)) {
      free(callsite); // free memory
      return i;
    }
  }
  self->cu->num_callsites++;
  Renew(self->cu->callsites, self->cu->num_callsites, MVMCallsite*);
  self->cu->callsites[self->cu->num_callsites-1] = callsite;
  return self->cu->num_callsites-1;
}

