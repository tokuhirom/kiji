#pragma once
// vim:ts=2:sw=2:tw=0:

namespace saru {

  // http://doc.perl6.org/type/Array

  // TODO:
  // end
  // keys
  // values
  // kv
  // pairs
  // join
  // map
  // grep
  // first
  // classify
  // Bool
  // Str
  // Int
  // pick
  // roll
  // eager
  // reverse
  // rotate
  // sort
  // reduce
  // splice
  // push
  // unshift

  static void Array_elems(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
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

  // see bootstrap.c about argument operation.
  static void Array_shift(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMRegister result;

    REPR(self)->pos_funcs->shift(tc, STABLE(self), self,
                                    OBJECT_BODY(self), &result, MVM_reg_obj);

    MVM_args_set_result_obj(tc, result.o, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  static void Array_pop(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);

    MVMRegister result;

    REPR(self)->pos_funcs->pop(tc, STABLE(self), self,
                                    OBJECT_BODY(self), &result, MVM_reg_obj);

    MVM_args_set_result_obj(tc, result.o, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }

  void bootstrap_Array(MVMCompUnit* cu, MVMThreadContext*tc) {
    ClassBuilder b(cu->hll_config->slurpy_array_type, tc);
    b.add_method("shift", strlen("shift"), Array_shift);
    b.add_method("pop", strlen("pop"), Array_pop);
    b.add_method("elems", strlen("elems"), Array_elems);
  }

};
