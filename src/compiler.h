#pragma once
// vim:ts=2:sw=2:tw=0:

#include <string>
#include <vector>

namespace saru {
  class Assembler {
  public:
    void op(MVMuint8 bank_num, MVMuint8 op_num) {
      if (0) {
        MVMOpInfo *op = MVM_op_get_op(bank_num, op_num);
        printf("%d %d %s\n", bank_num, op_num, op->name);
      }
      write_u8(bank_num);
      write_u8(op_num);
    }
    void op_u16(MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1) {
      op(bank_num, op_num);
      write_u16(op1);
    }
    void op_u16_u16(MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2) {
      op(bank_num, op_num);
      write_u16(op1);
      write_u16(op2);
    }
    void op_u16_u16_u16(MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2, uint16_t op3) {
      op(bank_num, op_num);
      write_u16(op1);
      write_u16(op2);
      write_u16(op3);
    }
    void op_u16_i64(MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, int64_t op2) {
      op(bank_num, op_num);
      write_u16(op1);
      write_i64(op2);
    }
    void write_u8(MVMuint8 i) {
      bytecode_.push_back(i);
    }
    void write_u16(MVMuint16 i) {
      bytecode_.push_back((i>>0)  &0xffff);
      bytecode_.push_back((i>>8)  &0xffff);
    }
    void write_16(uint32_t i) {
      // optmize?
      bytecode_.push_back((i>>0)  &0xffff);
      bytecode_.push_back((i>>8)  &0xffff);
      bytecode_.push_back((i>>16) &0xffff);
      bytecode_.push_back((i>>24) &0xffff);
    }
    void write_i64(int64_t i) {
      // optmize?
      static char buf[8];
      memcpy(buf, &i, 8); // TODO endian
      for (int i=0; i<8; i++) {
        bytecode_.push_back(buf[i]);
      }
    }
    MVMuint8* bytecode() {
      return bytecode_.data(); // C++11
    }
    size_t bytecode_size() {
      return bytecode_.size();
    }
  private:
    std::vector<MVMuint8> bytecode_;
  };

  class Frame {
  private:
    MVMStaticFrame frame_; // frame itself
    MVMThreadContext *tc_;

    std::vector<MVMuint16> local_types_;
    std::vector<MVMuint16> lexical_types_;

  public:
    Frame() {
      memset(&frame_, 0, sizeof(MVMFrame));
      tc_ = NULL;
    }
    void set_tc(MVMThreadContext *tc) {
      tc_ = tc;
    }

    void finalize(MVMStaticFrame * frame) {
      assert(frame);

      frame_.local_types = local_types_.data();
      frame_.num_locals  = local_types_.size();

      frame_.num_lexicals  = lexical_types_.size();
      frame_.lexical_types = lexical_types_.data();

      // see src/core/bytecode.c
      frame_.env_size = frame_.num_lexicals * sizeof(MVMRegister);
      frame_.static_env = (MVMRegister*)malloc(frame_.env_size);
      memset(frame_.static_env, 0, frame_.env_size);

      frame_.cuuid = MVM_string_utf8_decode(tc_, tc_->instance->VMString, "cuuid", strlen("cuuid"));
      frame_.name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, "main frame", strlen("main frame"));

      memcpy(frame, &frame_, sizeof(MVMStaticFrame));
    }

    // reserve register
    int push_local_type(MVMuint16 reg_type) {
      local_types_.push_back(reg_type);
      if (local_types_.size() >= 65535) {
        printf("[panic] Too much registers\n");
        abort();
      }
      return local_types_.size()-1;
    }
    // Get register type at 'n'
    uint16_t get_local_type(int n) {
      assert(n>=0);
      assert(n<local_types_.size());
      return local_types_[n];
    }
    // Push lexical variable.
    int push_lexical(const char*name_c, int name_len, MVMuint16 type) {
      lexical_types_.push_back(type);

      int idx = lexical_types_.size()-1;

      MVMLexicalHashEntry *entry = (MVMLexicalHashEntry*)calloc(sizeof(MVMLexicalHashEntry), 1);
      entry->value = idx;

      MVMThreadContext *tc = tc_; // workaround for MVM's bad macro
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_c, name_len);
      MVM_string_flatten(tc_, name);
      // lexical_names is Hash.
      MVM_HASH_BIND(tc_, frame_.lexical_names, name, entry);

      return idx;
    }

    // lexical variable number by name
    // TODO: find from outer frame?
    int find_lexical_by_name(const std::string &name_cc, int &outer) {
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      MVMLexicalHashEntry *lexical_names = frame_.lexical_names;
      MVMLexicalHashEntry *entry;
      MVM_HASH_GET(tc_, lexical_names, name, entry);
      if (entry) {
        return entry->value;
      } else {
        printf("Unknown lexical variable: %s\n", name_cc.c_str());
        abort();
      }
    }

    void set_bytecode(MVMuint8*b, size_t size) {
      frame_.bytecode      = b;
      frame_.bytecode_size = size;
    }
  };

  class Interpreter {
  private:
    MVMInstance* vm_;
    MVMCompUnit* cu_;
    std::vector<MVMString*> strings_;
    std::vector<Frame> frames_;

  protected:
    // Copy data to CompUnit.
    void prepare() {
      // finalize strings
      cu_->strings     = strings_.data();
      cu_->num_strings = strings_.size();

      // finalize frames
      cu_->num_frames  = frames_.size();
      assert(frames_.size() >= 1);

      cu_->frames = (MVMStaticFrame**)malloc(sizeof(MVMStaticFrame*)*frames_.size());
      cu_->frames[0] = (MVMStaticFrame*)malloc(sizeof(MVMStaticFrame));
      cu_->main_frame = cu_->frames[0];

      for (int i=0; i<frames_.size(); i++) {
        frames_[i].finalize(cu_->frames[i]);
        cu_->frames[i]->cu = cu_;
        cu_->frames[i]->work_size = 0;
      }
      assert(cu_->main_frame->cuuid);

      // setup hllconfig
      MVMThreadContext *tc = vm_->main_thread;
      MVMString* str = MVM_string_utf8_decode(tc, tc->instance->VMString, "saru", 4);
      cu_->hll_config = MVM_hll_get_config_for(tc, str);
    }
  public:
    Interpreter() :frames_(1) {
      vm_ = MVM_vm_create_instance();
      cu_ = (MVMCompUnit*)malloc(sizeof(MVMCompUnit));
      memset(cu_, 0, sizeof(MVMCompUnit));
    }
    ~Interpreter() {
      MVM_vm_destroy_instance(vm_);
      free(cu_);
    }
    static void toplevel_initial_invoke(MVMThreadContext *tc, void *data) {
      /* Dummy, 0-arg callsite. */
      static MVMCallsite no_arg_callsite;
      no_arg_callsite.arg_flags = NULL;
      no_arg_callsite.arg_count = 0;
      no_arg_callsite.num_pos   = 0;

      /* Create initial frame, which sets up all of the interpreter state also. */
      MVM_frame_invoke(tc, (MVMStaticFrame *)data, &no_arg_callsite, NULL, NULL, NULL);
    }
    void initialize() {
      // init compunit.
      int apr_return_status;
      apr_pool_t  *pool        = NULL;
      /* Ensure the file exists, and get its size. */
      if ((apr_return_status = apr_pool_create(&pool, NULL)) != APR_SUCCESS) {
        MVM_panic(MVM_exitcode_compunit, "Could not allocate APR memory pool: errorcode %d", apr_return_status);
      }
      cu_->pool       = pool;
      frames_[0].set_tc(vm_->main_thread);
    }

    void set_bytecode(int frame_no, MVMuint8*b, size_t size) {
      frames_[frame_no].set_bytecode(b, size);
    }

    int push_string(const std::string &str) {
      return this->push_string(str.c_str(), str.size());
    }

    int push_string(const char*string, int length) {
      MVMThreadContext *tc = vm_->main_thread;
      MVMString* str = MVM_string_utf8_decode(tc, tc->instance->VMString, string, length);
      strings_.push_back(str);
      return strings_.size() - 1;
    }

    // reserve register
    int push_local_type(MVMuint16 reg_type) {
      // TODO deprecate
      return frames_[0].push_local_type(reg_type);
    }
    // Get register type at 'n'
    uint16_t get_local_type(int n) {
      return frames_[0].get_local_type(n);
    }
    // Push lexical variable.
    int push_lexical(const char*name_c, int name_len, MVMuint16 type) {
      return frames_[0].push_lexical(name_c, name_len, type);
    }

    // lexical variable number by name
    // TODO: find from outer frame?
    int find_lexical_by_name(const std::string &name_cc, int &outer) {
      return frames_[0].find_lexical_by_name(name_cc, outer);
    }

    void run() {
      this->prepare();

      assert(cu_->main_frame);
      assert(cu_->main_frame->bytecode);
      assert(cu_->main_frame->bytecode_size > 0);

      MVMThreadContext *tc = vm_->main_thread;
      MVMStaticFrame *start_frame = cu_->main_frame ? cu_->main_frame : cu_->frames[0];
      MVM_interp_run(tc, &toplevel_initial_invoke, start_frame);
    }

    void dump() {
      this->prepare();

      MVMThreadContext *tc = vm_->main_thread;
      // dump it
      char *dump = MVM_bytecode_dump(tc, cu_);

      printf("%s", dump);
      free(dump);
    }
  };

  /**
   * OP map is 3rd/MoarVM/src/core/oplist
   * interp code is 3rd/MoarVM/src/core/interp.c
   */
  class Compiler {
  private:
    Interpreter &interp_;
    Assembler assembler_;
    int do_compile(const saru::Node &node) {
      // printf("node: %s\n", node.type_name());
      switch (node.type()) {
      case SARU_NODE_STRING: {
        int str_num = interp_.push_string(node.pv());
        int reg_num = interp_.push_local_type(MVM_reg_str);
        assembler_.op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_const_s, reg_num, str_num);
        return reg_num;
      }
      case SARU_NODE_INT: {
        uint16_t reg_num = interp_.push_local_type(MVM_reg_int64);
        int64_t n = node.iv();
        assembler_.op_u16_i64(MVM_OP_BANK_primitives, MVM_OP_const_i64, reg_num, n);
        return reg_num;
      }
      case SARU_NODE_BIND: {
        auto lhs = node.children()[0];
        auto rhs = node.children()[1];
        if (lhs.type() != SARU_NODE_MY) {
          printf("You can't bind value to %s, currently.\n", lhs.type_name());
          abort();
        }
        int lex_no = do_compile(lhs);
        int val    = this->box(do_compile(rhs));
        assembler_.op_u16_u16_u16(
            MVM_OP_BANK_primitives,
            MVM_OP_bindlex,
            lex_no, // lex number
            0,      // frame outer count
            val     // value
        );
        return -1;
      }
      case SARU_NODE_VARIABLE: {
        // copy lexical variable to register
        auto reg_no = interp_.push_local_type(MVM_reg_obj);
        int outer = 0;
        auto lex_no = interp_.find_lexical_by_name(node.pv(), outer);
        assembler_.op_u16_u16_u16(
          MVM_OP_BANK_primitives,
          MVM_OP_getlex,
          reg_no,
          lex_no,
          outer // outer frame
        );
        return reg_no;
      }
      case SARU_NODE_MY: {
        if (node.children().size() != 1) {
          printf("NOT IMPLEMENTED\n");
          abort();
        }
        auto n = node.children()[0];
        if (n.type() != SARU_NODE_VARIABLE) {
          printf("This is variable: %s\n", n.type_name());
          exit(0);
        }
        int idx = interp_.push_lexical(n.pv().c_str(), n.pv().size(), MVM_reg_obj);
        return idx;
      }
      case SARU_NODE_IDENT:
        break;
      case SARU_NODE_STATEMENTS:
        for (auto n: node.children()) {
          do_compile(n);
        }
        return -1;
      case SARU_NODE_STRING_CONCAT: {
        auto dst_reg = interp_.push_local_type(MVM_reg_str);
        auto lhs = node.children()[0];
        auto rhs = node.children()[1];
        auto l = stringify(do_compile(lhs));
        auto r = stringify(do_compile(rhs));
        assembler_.op_u16_u16_u16(
          MVM_OP_BANK_string,
          MVM_OP_concat_s,
          dst_reg,
          l,
          r
        );
        return dst_reg;
      }
      case SARU_NODE_MUL: {
        return this->numeric_binop(node, MVM_OP_mul_i);
      }
      case SARU_NODE_SUB: {
        return this->numeric_binop(node, MVM_OP_sub_i);
      }
      case SARU_NODE_DIV: {
        return this->numeric_binop(node, MVM_OP_div_i);
      }
      case SARU_NODE_ADD: {
        return this->numeric_binop(node, MVM_OP_add_i);
      }
      case SARU_NODE_MOD: {
        return this->numeric_binop(node, MVM_OP_mod_i);
      }
      case SARU_NODE_FUNCALL: {
        assert(node.children().size() == 2);
        const saru::Node &ident = node.children()[0];
        const saru::Node &args  = node.children()[1];
        if (ident.pv() == "say") {
          for (auto a:args.children()) {
            uint16_t reg_num = stringify(do_compile(a));
            assembler_.op_u16_u16(MVM_OP_BANK_io, MVM_OP_say, reg_num, 0);
            return -1; // TODO: Is there a result?
          }
        } else {
          MVM_panic(MVM_exitcode_compunit, "Not implemented, normal function call: '%s'", ident.pv().c_str());
        }
        break;
      }
      default:
        MVM_panic(MVM_exitcode_compunit, "Not implemented op: %s", node.type_name());
        break;
      }
      printf("Should not reach here: %s\n", node.type_name());
      abort();
    }
  private:
    // objectify the register.
    int box(int reg_num) {
      assert(reg_num >= 0);
      auto reg_type = interp_.get_local_type(reg_num);
      if (reg_type == MVM_reg_obj) {
        return reg_num;
      }

      int dst_num = interp_.push_local_type(MVM_reg_obj);
      int boxtype_reg = interp_.push_local_type(MVM_reg_obj);
      switch (reg_type) {
      case MVM_reg_str:
        assembler_.op_u16(MVM_OP_BANK_object, MVM_OP_hllboxtype_s, boxtype_reg);
        assembler_.op_u16_u16_u16(MVM_OP_BANK_object, MVM_OP_box_s, dst_num, reg_num, boxtype_reg);
        return dst_num;
      case MVM_reg_int64:
        assembler_.op_u16(MVM_OP_BANK_object, MVM_OP_hllboxtype_i, boxtype_reg);
        assembler_.op_u16_u16_u16(MVM_OP_BANK_object, MVM_OP_box_i, dst_num, reg_num, boxtype_reg);
        return dst_num;
      default:
        MVM_panic(MVM_exitcode_compunit, "Not implemented, boxify %d", interp_.get_local_type(reg_num));
        abort();
      }
    }
    int to_i(int reg_num) {
      assert(reg_num >= 0);
      switch (interp_.get_local_type(reg_num)) {
      case MVM_reg_str: {
        int dst_num = interp_.push_local_type(MVM_reg_int64);
        assembler_.op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_coerce_si, dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_int64: {
        return reg_num;
      }
      case MVM_reg_obj: {
        int dst_num = interp_.push_local_type(MVM_reg_int64);
        assembler_.op_u16_u16(MVM_OP_BANK_object, MVM_OP_unbox_i, dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", interp_.get_local_type(reg_num));
        break;
      }
    }
    int stringify(int reg_num) {
      assert(reg_num >= 0);
      switch (interp_.get_local_type(reg_num)) {
      case MVM_reg_str:
        // nop
        return reg_num;
      case MVM_reg_int64: {
        int dst_num = interp_.push_local_type(MVM_reg_str);
        assembler_.op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_coerce_is, dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_obj: {
        int dst_num = interp_.push_local_type(MVM_reg_str);
        assembler_.op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_smrt_strify, dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, stringify %d", interp_.get_local_type(reg_num));
        break;
      }
    }
    int numeric_binop(const saru::Node& node, uint16_t op) {
        assert(node.children().size() == 2);

        // TODO support dobule
        int reg_num_dst = interp_.push_local_type(MVM_reg_int64);
        int reg_num1 = this->to_i(do_compile(node.children()[0]));
        int reg_num2 = this->to_i(do_compile(node.children()[1]));
        assert(interp_.get_local_type(reg_num1) == MVM_reg_int64);
        assert(interp_.get_local_type(reg_num2) == MVM_reg_int64);
        assembler_.op_u16_u16_u16(MVM_OP_BANK_primitives, op, reg_num_dst, reg_num1, reg_num2);
        return reg_num_dst;
    }
  public:
    Compiler(Interpreter &interp): interp_(interp) { }
    void compile(saru::Node &node) {
      do_compile(node);

      // final op must be return.
      assembler_.op(MVM_OP_BANK_primitives, MVM_OP_return);

      interp_.set_bytecode(0, assembler_.bytecode(), assembler_.bytecode_size());
    }
  };
}

