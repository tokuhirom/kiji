#pragma once
// vim:ts=2:sw=2:tw=0:

#include <string>
#include <vector>
#include "node_dump.h"

namespace saru {
  class Assembler {
  public:
    void op(MVMuint8 bank_num, MVMuint8 op_num) {
      write_u8(bank_num);
      write_u8(op_num);
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

  class Interpreter {
  private:
    MVMInstance* vm_;
    MVMCompUnit* cu_;
    std::vector<MVMString*> strings_;
    std::vector<MVMuint16> local_types_;
    std::vector<MVMuint16> lexical_types_; // per frame

  protected:
    // Copy data to CompUnit.
    void prepare() {
      cu_->strings = strings_.data();
      cu_->num_strings = strings_.size();

      cu_->main_frame->local_types = local_types_.data();
      cu_->main_frame->num_locals  = local_types_.size();

      cu_->main_frame->num_lexicals  = lexical_types_.size();
      cu_->main_frame->lexical_types = lexical_types_.data();

      // see src/core/bytecode.c
      cu_->main_frame->env_size = cu_->main_frame->num_lexicals * sizeof(MVMRegister);
      cu_->main_frame->static_env = (MVMRegister*)malloc(cu_->main_frame->env_size);
      memset(cu_->main_frame->static_env, 0, cu_->main_frame->env_size);
    }
  public:
    Interpreter() {
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
      MVMThreadContext *tc = vm_->main_thread;
      int          apr_return_status;
      apr_pool_t  *pool        = NULL;
      /* Ensure the file exists, and get its size. */
      if ((apr_return_status = apr_pool_create(&pool, NULL)) != APR_SUCCESS) {
        MVM_panic(MVM_exitcode_compunit, "Could not allocate APR memory pool: errorcode %d", apr_return_status);
      }
      cu_->pool       = pool;
      cu_->num_frames  = 1;
      cu_->main_frame = (MVMStaticFrame*)malloc(sizeof(MVMStaticFrame));
      cu_->frames = (MVMStaticFrame**)malloc(sizeof(MVMStaticFrame) * 1);
      cu_->frames[0] = cu_->main_frame;
      memset(cu_->main_frame, 0, sizeof(MVMStaticFrame));
      cu_->main_frame->cuuid = MVM_string_utf8_decode(tc, tc->instance->VMString, "cuuid", strlen("cuuid"));
      cu_->main_frame->name = MVM_string_utf8_decode(tc, tc->instance->VMString, "main frame", strlen("main frame"));
      assert(cu_->main_frame->cuuid);
      cu_->main_frame->cu = cu_;

      cu_->main_frame->work_size = 0;

      cu_->num_strings = 1;
    }

    void set_bytecode(MVMuint8*b, size_t size) {
      assert(cu_->main_frame && "Call initialize() before call this method");
      cu_->main_frame->bytecode      = b;
      cu_->main_frame->bytecode_size = size;
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

      MVMThreadContext *tc = vm_->main_thread;
      MVMLexicalHashEntry *entry = (MVMLexicalHashEntry*)calloc(sizeof(MVMLexicalHashEntry), 1);
      entry->value = idx;

      MVMString* name = MVM_string_utf8_decode(tc, tc->instance->VMString, name_c, name_len);
      MVM_string_flatten(tc, name);
      // lexical_names is Hash.
      MVM_HASH_BIND(tc, cu_->main_frame->lexical_names, name, entry);

      return idx;
    }

    // lexical variable number by name
    // TODO: find from outer frame?
    int find_lexical_by_name(const std::string &name_cc, int &outer) {
      MVMThreadContext *tc = vm_->main_thread;
      MVMString* name = MVM_string_utf8_decode(tc, tc->instance->VMString, name_cc.c_str(), name_cc.size());
      MVMLexicalHashEntry *lexical_names = cu_->main_frame->lexical_names;
      MVMLexicalHashEntry *entry;
      MVM_HASH_GET(tc, lexical_names, name, entry);
      if (entry) {
        return entry->value;
      } else {
        printf("Unknown lexical variable: %s\n", name_cc.c_str());
        abort();
      }
    }

    void run() {
      assert(cu_->main_frame);
      assert(cu_->main_frame->bytecode);
      assert(cu_->main_frame->bytecode_size > 0);

      this->prepare();

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
    int do_compile(const SARUNode &node) {
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
        int val    = do_compile(rhs);
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
        auto reg_no = interp_.push_local_type(MVM_reg_int64);
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
        int idx = interp_.push_lexical(n.pv().c_str(), n.pv().size(), MVM_reg_int64);
        return idx;
      }
      case SARU_NODE_IDENT:
        break;
      case SARU_NODE_STATEMENTS:
        for (auto n: node.children()) {
          do_compile(n);
        }
        return -1;
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
        const SARUNode &ident = node.children()[0];
        const SARUNode &args  = node.children()[1];
        if (ident.pv() == "say") {
          for (auto a:args.children()) {
            uint16_t reg_num = do_compile(a);
            assert(reg_num >= 0);
            switch (interp_.get_local_type(reg_num)) {
            case MVM_reg_str:
              // nop
              break;
            case MVM_reg_int64: {
              // need stringify
              uint16_t orig_reg_num = reg_num;
              reg_num = interp_.push_local_type(MVM_reg_str);
              assembler_.op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_coerce_is, reg_num, orig_reg_num);
              break;
            }
            default:
              // TODO
              MVM_panic(MVM_exitcode_compunit, "Not implemented, say %d", interp_.get_local_type(reg_num));
              break;
            }
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
    int numeric_binop(const SARUNode& node, uint16_t op) {
        assert(node.children().size() == 2);
        int reg_num_dst = interp_.push_local_type(MVM_reg_int64);
        int reg_num1 = do_compile(node.children()[0]);
        int reg_num2 = do_compile(node.children()[1]);
        if (interp_.get_local_type(reg_num1) == MVM_reg_int64) {
          if (interp_.get_local_type(reg_num2) == MVM_reg_int64) {
            assert(interp_.get_local_type(reg_num2) == MVM_reg_int64);
            assembler_.op_u16_u16_u16(MVM_OP_BANK_primitives, op, reg_num_dst, reg_num1, reg_num2);
            return reg_num_dst;
          } else {
            abort(); // TODO
          }
        } else {
          abort(); // TODO
        }
    }
  public:
    Compiler(Interpreter &interp): interp_(interp) { }
    void compile(SARUNode &node) {
      do_compile(node);

      // final op must be return.
      assembler_.op(MVM_OP_BANK_primitives, MVM_OP_return);

      interp_.set_bytecode(assembler_.bytecode(), assembler_.bytecode_size());
    }
  };
}

