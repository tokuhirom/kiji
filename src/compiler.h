#pragma once
// vim:ts=2:sw=2:tw=0:

#include <string>
#include <vector>

namespace saru {
  class Assembler {
  public:
    void write(MVMuint8 bank_num, MVMuint8 op_num) {
      write_8(bank_num);
      write_8(op_num);
    }
    void write(MVMuint8 bank_num, MVMuint8 op_num, MVMuint16 op1, MVMuint16 op2) {
      write(bank_num, op_num);
      write_16(op1);
      write_16(op2);
    }
    void write_8(MVMuint8 i) {
      bytecode_.push_back(i);
    }
    void write_16(MVMuint16 i) {
      bytecode_.push_back(i&0xffff);
      bytecode_.push_back(i>>4);
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

  protected:
    // Copy data to CompUnit.
    void prepare() {
      cu_->strings = strings_.data();
      cu_->num_strings = strings_.size();

      cu_->main_frame->local_types = local_types_.data();
      cu_->main_frame->num_locals  = local_types_.size();
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

      // cu_->main_frame->num_locals= 1; // register size
      // cu_->main_frame->local_types = (MVMuint16*)malloc(sizeof(MVMuint16)*cu_->main_frame->num_locals);
      // cu_->main_frame->local_types[0] = MVM_reg_str;

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
      return local_types_.size()-1;
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

  class Compiler {
  private:
    Interpreter &interp_;
    Assembler assembler_;
    int do_compile(SARUNode &node) {
      // printf("node: %s\n", node.type_name());
      switch (node.type()) {
      case SARU_NODE_STRING: {
        int str_num = interp_.push_string(node.pv());
        int reg_num = interp_.push_local_type(MVM_reg_str);
        assembler_.write(MVM_OP_BANK_primitives, MVM_OP_const_s, reg_num, str_num);
        return reg_num;
      }
      case SARU_NODE_IDENT:
        break;
      case SARU_NODE_STATEMENTS:
        for (auto n: node.children()) {
          do_compile(n);
        }
        break;
      case SARU_NODE_FUNCALL: {
        assert(node.children().size() == 2);
        const SARUNode &ident = node.children()[0];
        const SARUNode &args  = node.children()[1];
        if (ident.pv() == "say") {
          for (auto a:args.children()) {
            int reg_num = do_compile(a);
            assembler_.write(MVM_OP_BANK_io, MVM_OP_say, reg_num, 0);
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
      return 0;
    }
  public:
    Compiler(Interpreter &interp): interp_(interp) { }
    void compile(SARUNode &node) {
      do_compile(node);

      // final op must be return.
      assembler_.write(MVM_OP_BANK_primitives, MVM_OP_return);
      /*
      assembler_.write(MVM_OP_BANK_primitives, MVM_OP_const_s, 0, 0);
      assembler_.write(MVM_OP_BANK_io,         MVM_OP_say,     0, 0);
      */
      interp_.set_bytecode(assembler_.bytecode(), assembler_.bytecode_size());
    }
  };
}

