#pragma once
// vim:ts=2:sw=2:tw=0:

namespace saru {
  static void Hash_elems(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMuint64 elems = REPR(self)->elems(tc, STABLE(self), self,
                                    OBJECT_BODY(self));

    MVM_args_set_result_int(tc, elems, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  void bootstrap_Hash(MVMCompUnit* cu, MVMThreadContext*tc) {
    ClassBuilder b(cu->hll_config->slurpy_hash_type, tc);
    b.add_method("elems", strlen("elems"), Hash_elems);
  }
};
