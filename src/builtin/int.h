#pragma once
// vim:ts=2:sw=2:tw=0:

namespace kiji {
  // http://doc.perl6.org/type/Int

  // chr

  static void Int_say(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMint64 i = REPR(self)->box_funcs->get_int(tc, STABLE(self), self, OBJECT_BODY(self));
    MVMString *s = MVM_coerce_i_s(tc, i);
    MVM_string_say(tc, s);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  void bootstrap_Int(MVMCompUnit* cu, MVMThreadContext*tc) {
    ClassBuilder b(cu->hll_config->int_box_type, tc);
    b.add_method("say", strlen("say"), Int_say);
  }
};
