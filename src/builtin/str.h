#pragma once
// vim:ts=2:sw=2:tw=0:

namespace saru {

  // http://doc.perl6.org/type/Str

  // TODO
  // chop
  // chomp
  // fc
  // tc
  // tclc
  // tcuc
  // wordcase
  // lcfirst
  // ucfirst
  // chars
  // encode
  // index
  // rindex
  // split
  // comb
  // lines
  // words
  // flip
  // sprintf
  // subst
  // substr
  // succ
  // pred
  // ord
  // ords
  // trim
  // trim-trailing
  // trim-leading

  static void Str_uc(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMString * self_s = REPR(self)->box_funcs->get_str(tc, STABLE(self), self, OBJECT_BODY(self));

    MVMString * result = MVM_string_uc(tc, (MVMString*)self_s);
    assert(result);

    MVM_args_set_result_str(tc, result, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  static void Str_lc(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMString * self_s = REPR(self)->box_funcs->get_str(tc, STABLE(self), self, OBJECT_BODY(self));

    MVMString * result = MVM_string_lc(tc, (MVMString*)self_s);
    assert(result);

    MVM_args_set_result_str(tc, result, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  static void Str_length(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMString * self_s = REPR(self)->box_funcs->get_str(tc, STABLE(self), self, OBJECT_BODY(self));

    MVMint64 length = NUM_GRAPHS((MVMString*)self_s);

    MVM_args_set_result_int(tc, length, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  void bootstrap_Str(MVMCompUnit* cu, MVMThreadContext*tc) {
    ClassBuilder b(cu->hll_config->str_box_type, tc);
    b.add_method("lc", strlen("lc"), Str_lc);
    b.add_method("uc", strlen("uc"), Str_uc);
    b.add_method("length", strlen("length"), Str_length);
  }
};
