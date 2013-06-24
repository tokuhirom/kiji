#pragma once
// vim:ts=2:sw=2:tw=0:

namespace kiji {

  static void File_eof(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMint64 result = MVM_file_eof(tc, self);

    MVM_args_set_result_int(tc, result, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  void bootstrap_File(MVMCompUnit* cu, MVMThreadContext*tc) {
    ClassBuilder b(tc->instance->boot_types->BOOTIO, tc);
    b.add_method("eof", strlen("eof"), File_eof);
  }
};
