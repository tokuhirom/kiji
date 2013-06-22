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

  // based on MVM_string_join in 3rd/MoarVM/src/strings/ops.c
  static MVMString * MVM_string_join_ex(MVMThreadContext *tc, MVMString *separator, MVMObject *input) {
      MVMint64 elems, length = 0, index = -1, position = 0;
      MVMString *portion, *result;
      // MVMuint32 codes = 0;
      MVMStrandIndex portion_index = 0, max_strand_depth;
      MVMStringIndex sgraphs, rgraphs;
      MVMStrand *strands;
      
      if (!IS_CONCRETE(input)) {
          MVM_exception_throw_adhoc(tc, "join needs a concrete array to join");
      }
      
      if (!IS_CONCRETE((MVMObject *)separator)) {
          MVM_exception_throw_adhoc(tc, "join needs a concrete separator");
      }
      
      MVMROOT(tc, separator, {
      MVMROOT(tc, input, {
          result = (MVMString *)MVM_repr_alloc_init(tc, (MVMObject*)separator);
          MVMROOT(tc, result, {
              elems = REPR(input)->elems(tc, STABLE(input),
                  input, OBJECT_BODY(input));
              
              sgraphs = NUM_GRAPHS(separator);
              max_strand_depth = STRAND_DEPTH(separator);
              
              while (++index < elems) {
                  MVMObject *item = MVM_repr_at_pos_o(tc, input, index);
                  MVMStringIndex pgraphs;
                  
                  /* allow null or type object items in the array, I guess.. */
                  if (!item || !IS_CONCRETE(item))
                      continue;
                  
                  MVMRegister reg;
                  MVM_coerce_smart_stringify(tc, item, &reg); // ←←← This is a modified point.
                  portion = reg.s;
                  pgraphs = NUM_GRAPHS(portion);
                  if (pgraphs) 
                      ++portion_index;
                  if (index && sgraphs)
                      ++portion_index;
                  length += pgraphs + (index ? sgraphs : 0);
                  /* XXX codes += portion->body.codes + (index ? separator->body.codes : 0); */
                  if (STRAND_DEPTH(portion) > max_strand_depth)
                      max_strand_depth = STRAND_DEPTH(portion);
              }
              
              rgraphs = length;
              /* XXX consider whether to coalesce combining characters
              if they cause new combining sequences to appear */
              /* XXX result->body.codes = codes; */
              
              if (portion_index > (1<<30)) {
                  MVM_exception_throw_adhoc(tc, "join array items > %lld arbitrarily unsupported...", (1<<30));
              }
              
              if (portion_index) {
                  index = -1;
                  position = 0;
                  strands = result->body.strands = (MVMStrand *)calloc(sizeof(MVMStrand), portion_index + 1);
                  
                  portion_index = 0;
                  while (++index < elems) {
                      MVMObject *item = MVM_repr_at_pos_o(tc, input, index);
                      
                      if (!item || !IS_CONCRETE(item))
                          continue;
                      
                      /* Note: this allows the separator to precede the empty string. */
                      if (index && sgraphs) {
                          strands[portion_index].compare_offset = position;
                          strands[portion_index].string = separator;
                          position += sgraphs;
                          ++portion_index;
                      }
                      
                      MVMRegister reg;
                      MVM_coerce_smart_stringify(tc, item, &reg); // ←←← This is a modified point.
                      portion = reg.s;
                      // portion = MVM_repr_get_str(tc, item);
                      length = NUM_GRAPHS(portion);
                      if (length) {
                          strands[portion_index].compare_offset = position;
                          strands[portion_index].string = portion;
                          position += length;
                          ++portion_index;
                      }
                  }
                  strands[portion_index].graphs = position;
                  strands[portion_index].strand_depth = max_strand_depth + 1;
                  result->body.flags = MVM_STRING_TYPE_ROPE;
                  result->body.num_strands = portion_index;
              }
              else {
                  /* leave type default of int32 and graphs 0 */
              }
          });
      });
      });
      
      /* assertion/check */
      if (NUM_GRAPHS(result) != position)
          MVM_exception_throw_adhoc(tc, "join had an internal error");
      
      return result;
  }

  static void Array_join(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVMArgInfo arg1 = MVM_args_get_pos_obj(tc, &arg_ctx, 1, MVM_ARG_OPTIONAL);
    MVMObject* joiner = arg1.exists ? arg1.arg.o : NULL;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    // dump_object(tc, joiner);

    MVM_gc_root_temp_push(tc, (MVMCollectable **)&self);
    if (joiner) {
      MVM_gc_root_temp_push(tc, (MVMCollectable **)&joiner);
    }

    // MVMString * joiner_s = REPR(joiner)->box_funcs->get_str(tc, STABLE(joiner), joiner, OBJECT_BODY(joiner));
    MVMRegister joiner_s;
    MVM_coerce_smart_stringify(tc, joiner, &joiner_s);

    MVMString * result = MVM_string_join_ex(tc, joiner_s.s, self);

    MVM_args_set_result_str(tc, result, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, joiner ? 2 : 1);
  }

  void bootstrap_Array(MVMCompUnit* cu, MVMThreadContext*tc) {
    ClassBuilder b(cu->hll_config->slurpy_array_type, tc);
    b.add_method("shift", strlen("shift"), Array_shift);
    b.add_method("pop", strlen("pop"), Array_pop);
    b.add_method("elems", strlen("elems"), Array_elems);
    b.add_method("join", strlen("join"), Array_join);
  }

};
