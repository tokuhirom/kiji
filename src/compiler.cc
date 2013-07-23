/* vim:ts=2:sw=2:tw=0:
 */

extern "C" {
#include "moarvm.h"
}
#include "compiler.h"

uint16_t Kiji_compiler_if_op(KijiCompiler* self, uint16_t cond_reg) {
    switch (Kiji_compiler_get_local_type(self, cond_reg)) {
    case MVM_reg_int64:
        return MVM_OP_if_i;
    case MVM_reg_num64:
    return MVM_OP_if_n;
    case MVM_reg_str:
    return MVM_OP_if_s;
    case MVM_reg_obj:
    return MVM_OP_if_o;
    default:
    abort();
    }
}


  void KijiLabel::put() {
    assert(address_ == -1);
    address_ = compiler_->frames_.back()->frame.bytecode_size;

    // rewrite reserved addresses
    for (auto r: reserved_addresses_) {
      Kiji_asm_write_uint32_t_for(&(*(compiler_->frames_.back())), address_, r);
    }
    reserved_addresses_.empty();
  }

      // fixme: `put` is not the best verb in English here.
      void KijiLoopGuard::put_last() {
        last_offset_ = compiler_->ASM_BYTECODE_SIZE()-1+1;
      }
      void KijiLoopGuard::put_redo() {
        redo_offset_ = compiler_->ASM_BYTECODE_SIZE()-1+1;
      }
      void KijiLoopGuard::put_next() {
        next_offset_ = compiler_->ASM_BYTECODE_SIZE()-1+1;
      }
KijiLoopGuard::~KijiLoopGuard() {
        MVMuint32 end_offset = compiler_->ASM_BYTECODE_SIZE()-1;

        MVMFrameHandler *last_handler = new MVMFrameHandler;
        last_handler->start_offset = start_offset_;
        last_handler->end_offset = end_offset;
        last_handler->category_mask = MVM_EX_CAT_LAST;
        last_handler->action = MVM_EX_ACTION_GOTO;
        last_handler->block_reg = 0;
        last_handler->goto_offset = last_offset_;
        compiler_->push_handler(last_handler);

        MVMFrameHandler *next_handler = new MVMFrameHandler;
        next_handler->start_offset = start_offset_;
        next_handler->end_offset = end_offset;
        next_handler->category_mask = MVM_EX_CAT_NEXT;
        next_handler->action = MVM_EX_ACTION_GOTO;
        next_handler->block_reg = 0;
        next_handler->goto_offset = next_offset_;
        compiler_->push_handler(next_handler);

        MVMFrameHandler *redo_handler = new MVMFrameHandler;
        redo_handler->start_offset = start_offset_;
        redo_handler->end_offset = end_offset;
        redo_handler->category_mask = MVM_EX_CAT_REDO;
        redo_handler->action = MVM_EX_ACTION_GOTO;
        redo_handler->block_reg = 0;
        redo_handler->goto_offset = redo_offset_;
        compiler_->push_handler(redo_handler);
      }
      KijiLoopGuard::KijiLoopGuard(KijiCompiler *compiler) :compiler_(compiler) {
        start_offset_ = compiler_->ASM_BYTECODE_SIZE()-1;
      }
    int KijiCompiler::numeric_inplace(const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = REG_NUM64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, tmp, to_n(lhs), to_n(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int KijiCompiler::binary_inplace(const PVIPNode* node, uint16_t op) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = REG_INT64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op, tmp, to_i(lhs), to_i(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int KijiCompiler::str_inplace(const PVIPNode* node, uint16_t op, uint16_t rhs_type) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = REG_STR();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, tmp, to_s(lhs), rhs_type == MVM_reg_int64 ? to_i(rhs) : to_s(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int KijiCompiler::numeric_binop(const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
        assert(node->children.size == 2);

        int reg_num1 = do_compile(node->children.nodes[0]);
        if (Kiji_compiler_get_local_type(this, reg_num1) == MVM_reg_int64) {
          int reg_num_dst = REG_INT64();
          int reg_num2 = this->to_i(do_compile(node->children.nodes[1]));
          assert(Kiji_compiler_get_local_type(this, reg_num1) == MVM_reg_int64);
          assert(Kiji_compiler_get_local_type(this, reg_num2) == MVM_reg_int64);
          ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_i, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (Kiji_compiler_get_local_type(this, reg_num1) == MVM_reg_num64) {
          int reg_num_dst = REG_NUM64();
          int reg_num2 = this->to_n(do_compile(node->children.nodes[1]));
          assert(Kiji_compiler_get_local_type(this, reg_num2) == MVM_reg_num64);
          ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (Kiji_compiler_get_local_type(this, reg_num1) == MVM_reg_obj) {
          // TODO should I use intify instead if the object is int?
          int reg_num_dst = REG_NUM64();

          int dst_num = REG_NUM64();
          ASM_OP_U16_U16(MVM_OP_BANK_primitives, MVM_OP_smrt_numify, dst_num, reg_num1);

          int reg_num2 = this->to_n(do_compile(node->children.nodes[1]));
          assert(Kiji_compiler_get_local_type(this, reg_num2) == MVM_reg_num64);
          ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);
          return reg_num_dst;
        } else if (Kiji_compiler_get_local_type(this, reg_num1) == MVM_reg_str) {
          int dst_num = REG_NUM64();
          ASM_COERCE_SN(dst_num, reg_num1);

          int reg_num_dst = REG_NUM64();
          int reg_num2 = this->to_n(do_compile(node->children.nodes[1]));
          ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);

          return reg_num_dst;
        } else {
          abort();
        }
    }

    uint16_t KijiCompiler::unless_op(uint16_t cond_reg) {
      switch (Kiji_compiler_get_local_type(this, cond_reg)) {
      case MVM_reg_int64:
        return MVM_OP_unless_i;
      case MVM_reg_num64:
        return MVM_OP_unless_n;
      case MVM_reg_str:
        return MVM_OP_unless_s;
      case MVM_reg_obj:
        return MVM_OP_unless_o;
      default:
        abort();
      }
    }
    void KijiCompiler::compile_statements(const PVIPNode*node, int dst_reg) {
      int reg = UNKNOWN_REG;
      if (node->type == PVIP_NODE_STATEMENTS || node->type == PVIP_NODE_ELSE) {
        for (int i=0, l=node->children.size; i<l; i++) {
          reg = do_compile(node->children.nodes[i]);
        }
      } else {
        reg = do_compile(node);
      }
      if (reg == UNKNOWN_REG) {
        ASM_NULL(dst_reg);
      } else {
        ASM_SET(dst_reg, to_o(reg));
      }
    }

    // Compile chained comparisions like `1 < $n < 3`.
    // TODO: optimize simple case like `1 < $n`
    uint16_t KijiCompiler::compile_chained_comparisions(const PVIPNode* node) {
      auto lhs = do_compile(node->children.nodes[0]);
      auto dst_reg = REG_INT64();
      auto label_end = label_unsolved();
      auto label_false = label_unsolved();
      for (int i=1; i<node->children.size; i++) {
        PVIPNode *iter = node->children.nodes[i];
        auto rhs = do_compile(iter->children.nodes[0]);
        // result will store to lhs.
        uint16_t ret = do_compare(iter->type, lhs, rhs);
        unless_any(ret, label_false);
        lhs = rhs;
      }
      ASM_CONST_I64(dst_reg, 1);
      goto_(label_end);
    label_false.put();
      ASM_CONST_I64(dst_reg, 0);
      // goto_(label_end());
    label_end.put();
      return dst_reg;
    }
    int KijiCompiler::num_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op_i, uint16_t op_n) {
        int reg_num_dst = REG_INT64();
        if (Kiji_compiler_get_local_type(this, lhs) == MVM_reg_int64) {
          assert(Kiji_compiler_get_local_type(this, lhs) == MVM_reg_int64);
          // assert(Kiji_compiler_get_local_type(this, rhs) == MVM_reg_int64);
          ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_i, reg_num_dst, lhs, to_i(rhs));
          return reg_num_dst;
        } else if (Kiji_compiler_get_local_type(this, lhs) == MVM_reg_num64) {
          ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, lhs, to_n(rhs));
          return reg_num_dst;
        } else if (Kiji_compiler_get_local_type(this, lhs) == MVM_reg_obj) {
          // TODO should I use intify instead if the object is int?
          ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, to_n(lhs), to_n(rhs));
          return reg_num_dst;
        } else {
          // NOT IMPLEMENTED
          abort();
        }
    }

    int KijiCompiler::str_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op) {
        int reg_num_dst = REG_INT64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, reg_num_dst, to_s(lhs), to_s(rhs));
        return reg_num_dst;
    }
    uint16_t KijiCompiler::do_compare(PVIP_node_type_t type, uint16_t lhs, uint16_t rhs) {
      switch (type) {
      case PVIP_NODE_STREQ:
        return this->str_cmp_binop(lhs, rhs, MVM_OP_eq_s);
      case PVIP_NODE_STRNE:
        return this->str_cmp_binop(lhs, rhs, MVM_OP_ne_s);
      case PVIP_NODE_STRGT:
        return this->str_cmp_binop(lhs, rhs, MVM_OP_gt_s);
      case PVIP_NODE_STRGE:
        return this->str_cmp_binop(lhs, rhs, MVM_OP_ge_s);
      case PVIP_NODE_STRLT:
        return this->str_cmp_binop(lhs, rhs, MVM_OP_lt_s);
      case PVIP_NODE_STRLE:
        return this->str_cmp_binop(lhs, rhs, MVM_OP_le_s);
      case PVIP_NODE_EQ:
        return this->num_cmp_binop(lhs, rhs, MVM_OP_eq_i, MVM_OP_eq_n);
      case PVIP_NODE_NE:
        return this->num_cmp_binop(lhs, rhs, MVM_OP_ne_i, MVM_OP_ne_n);
      case PVIP_NODE_LT:
        return this->num_cmp_binop(lhs, rhs, MVM_OP_lt_i, MVM_OP_lt_n);
      case PVIP_NODE_LE:
        return this->num_cmp_binop(lhs, rhs, MVM_OP_le_i, MVM_OP_le_n);
      case PVIP_NODE_GT:
        return this->num_cmp_binop(lhs, rhs, MVM_OP_gt_i, MVM_OP_gt_n);
      case PVIP_NODE_GE:
        return this->num_cmp_binop(lhs, rhs, MVM_OP_ge_i, MVM_OP_ge_n);
      case PVIP_NODE_EQV:
        return this->str_cmp_binop(lhs, rhs, MVM_OP_eq_s);
      default:
        abort();
      }
    }

    KijiCompiler::KijiCompiler(MVMCompUnit * cu, MVMThreadContext * tc): cu_(cu), frame_no_(0), tc_(tc) {
      initialize();

      current_class_how_ = NULL;

      auto handle = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"__SARU_CLASSES__");
      assert(tc);
      sc_classes_ = (MVMSerializationContext*)MVM_sc_create(tc_, handle);

      num_sc_classes_ = 0;
    }

    void KijiCompiler::initialize() {
      // init compunit.
      int apr_return_status;
      apr_pool_t  *pool        = NULL;
      /* Ensure the file exists, and get its size. */
      if ((apr_return_status = apr_pool_create(&pool, NULL)) != APR_SUCCESS) {
        MVM_panic(MVM_exitcode_compunit, "Could not allocate APR memory pool: errorcode %d", apr_return_status);
      }
      cu_->pool       = pool;
      assert(tc_);
      this->push_frame(std::string("frame_name_0"));
    }
    void KijiCompiler::finalize(MVMInstance* vm) {
      MVMThreadContext *tc = tc_; // remove me
      MVMInstance *vm_ = vm; // remove me

      // finalize frame
      for (int i=0; i<cu_->num_frames; i++) {
        MVMStaticFrame *frame = cu_->frames[i];
        // XXX Should I need to time sizeof(MVMRegister)??
        frame->env_size = frame->num_lexicals * sizeof(MVMRegister);
        Newxz(frame->static_env, frame->env_size, MVMRegister);

        char buf[1023+1];
        int len = snprintf(buf, 1023, "frame_cuuid_%d", i);
        frame->cuuid = MVM_string_utf8_decode(tc_, tc_->instance->VMString, buf, len);
      }

      cu_->main_frame = cu_->frames[0];
      assert(cu_->main_frame->cuuid);

      // Creates code objects to go with each of the static frames.
      // ref create_code_objects in src/core/bytecode.c
      Newxz(cu_->coderefs, cu_->num_frames, MVMObject*);

      MVMObject* code_type = tc->instance->boot_types->BOOTCode;

      for (int i = 0; i < cu_->num_frames; i++) {
        cu_->coderefs[i] = REPR(code_type)->allocate(tc, STABLE(code_type));
        ((MVMCode *)cu_->coderefs[i])->body.sf = cu_->frames[i];
      }
    }

    void KijiCompiler::compile(PVIPNode*node, MVMInstance* vm) {
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
        auto lex = push_lexical("$?PACKAGE", MVM_reg_obj);
        auto package = REG_OBJ();
        auto hash_type = REG_OBJ();
        ASM_HLLHASH(hash_type);
        ASM_CREATE(package, hash_type);
        ASM_BINDLEX(lex, 0, package);
      }

      do_compile(node);

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

      // setup hllconfig
      MVMThreadContext * tc = tc_;
      {
        MVMString *hll_name = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"kiji");
        CU->hll_config = MVM_hll_get_config_for(tc, hll_name);

        MVMObject *config = REPR(tc->instance->boot_types->BOOTHash)->allocate(tc, STABLE(tc->instance->boot_types->BOOTHash));
        MVM_hll_set_config(tc, hll_name, config);
      }

      // hacking hll
      Kiji_bootstrap_Array(CU, tc);
      Kiji_bootstrap_Str(CU,   tc);
      Kiji_bootstrap_Hash(CU,  tc);
      Kiji_bootstrap_File(CU,  tc);
      Kiji_bootstrap_Int(CU,   tc);

      // finalize callsite
      CU->max_callsite_size = 0;
      for (int i=0; i<CU->num_callsites; i++) {
        auto callsite = CU->callsites[i];
        CU->max_callsite_size = std::max(CU->max_callsite_size, callsite->arg_count);
      }

      // Initialize @*ARGS
      MVMObject *clargs = MVM_repr_alloc_init(tc, tc->instance->boot_types->BOOTArray);
      MVM_gc_root_add_permanent(tc, (MVMCollectable **)&clargs);
      auto handle = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"__SARU_CORE__");
      auto sc = (MVMSerializationContext *)MVM_sc_create(tc, handle);
      MVMROOT(tc, sc, {
        MVMROOT(tc, clargs, {
          MVMint64 count;
          for (count = 0; count < vm->num_clargs; count++) {
            MVMString *string = MVM_string_utf8_decode(tc,
              tc->instance->VMString,
              vm->raw_clargs[count], strlen(vm->raw_clargs[count])
            );
            MVMObject*type = CU->hll_config->str_box_type;
            MVMObject *box = REPR(type)->allocate(tc, STABLE(type));
            MVMROOT(tc, box, {
                if (REPR(box)->initialize)
                    REPR(box)->initialize(tc, STABLE(box), box, OBJECT_BODY(box));
                REPR(box)->box_funcs->set_str(tc, STABLE(box), box,
                    OBJECT_BODY(box), string);
            });
            MVM_repr_push_o(tc, clargs, box);
          }
          MVM_sc_set_object(tc, sc, 0, clargs);
        });
      });

      CU->num_scs = 2;
      CU->scs = (MVMSerializationContext**)malloc(sizeof(MVMSerializationContext*)*2);
      CU->scs[0] = sc;
      CU->scs[1] = sc_classes_;
      CU->scs_to_resolve = (MVMString**)malloc(sizeof(MVMString*)*2);
      CU->scs_to_resolve[0] = NULL;
      CU->scs_to_resolve[1] = NULL;
    }
