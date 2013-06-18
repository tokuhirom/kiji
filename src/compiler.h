#pragma once
// vim:ts=2:sw=2:tw=0:

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <stdint.h>
#include <sstream>
#include "gen.assembler.h"

namespace saru {
  class Frame {
  private:
    MVMStaticFrame frame_; // frame itself
    MVMThreadContext *tc_;
    std::string *name_; // TODO no pointer

    std::vector<MVMuint16> local_types_;
    std::vector<MVMuint16> lexical_types_;

    Assembler assembler_;

    void set_cuuid() {
      static int cuuid_counter = 0;
      std::ostringstream oss;
      oss << "frame_cuuid_" << cuuid_counter++;
      std::string cuuid = oss.str();
      frame_.cuuid = MVM_string_utf8_decode(tc_, tc_->instance->VMString, cuuid.c_str(), cuuid.size());
    }

  public:
    Frame(MVMThreadContext* tc, const std::string name) {
      memset(&frame_, 0, sizeof(MVMFrame));
      tc_ = tc;
      name_ = new std::string(name);
    }
    ~Frame(){
      delete name_;
    }

    Assembler & assembler() {
      return assembler_;
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

      // cuuid
      set_cuuid();

      // name
      // std::cout << *name_<< std::endl;
      frame_.name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_->c_str(), name_->size());

      // bytecode
      frame_.bytecode      = assembler_.bytecode();
      frame_.bytecode_size = assembler_.bytecode_size();

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
    int push_lexical(const std::string&name_cc, MVMuint16 type) {
      lexical_types_.push_back(type);

      int idx = lexical_types_.size()-1;

      MVMLexicalHashEntry *entry = (MVMLexicalHashEntry*)calloc(sizeof(MVMLexicalHashEntry), 1);
      entry->value = idx;

      MVMThreadContext *tc = tc_; // workaround for MVM's bad macro
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
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
  };

  class Interpreter {
  private:
    MVMInstance* vm_;
    MVMCompUnit* cu_;
    std::vector<MVMString*> strings_;
    std::vector<std::shared_ptr<Frame>> frames_;
    std::list<std::shared_ptr<Frame>> used_frames_;
    std::vector<MVMCallsite*> callsites_;

  protected:
    // Copy data to CompUnit.
    void finalize() {
      MVMThreadContext *tc = vm_->main_thread;

      // finalize strings
      cu_->strings     = strings_.data();
      cu_->num_strings = strings_.size();

      // finalize frames
      cu_->num_frames  = used_frames_.size();
      assert(frames_.size() >= 1);

      cu_->frames = (MVMStaticFrame**)malloc(sizeof(MVMStaticFrame*)*used_frames_.size());
      {
        int i=0;
        for (auto frame: used_frames_) {
          cu_->frames[i] = (MVMStaticFrame*)malloc(sizeof(MVMStaticFrame));
          frame->finalize(cu_->frames[i]);
          cu_->frames[i]->cu = cu_;
          cu_->frames[i]->work_size = 0;
          ++i;
        }
      }
      cu_->main_frame = cu_->frames[0];
      assert(cu_->main_frame->cuuid);

      // Creates code objects to go with each of the static frames.
      // ref create_code_objects in src/core/bytecode.c
      cu_->coderefs = (MVMObject**)malloc(sizeof(MVMObject *) * cu_->num_frames);
      memset(cu_->coderefs, 0, sizeof(MVMObject *) * cu_->num_frames); // is this needed?

      MVMObject* code_type = tc->instance->boot_types->BOOTCode;

      for (int i = 0; i < cu_->num_frames; i++) {
        cu_->coderefs[i] = REPR(code_type)->allocate(tc, STABLE(code_type));
        ((MVMCode *)cu_->coderefs[i])->body.sf = cu_->frames[i];
      }

      // setup hllconfig
      MVMString* str = MVM_string_utf8_decode(tc, tc->instance->VMString, "nqp", 4);
      cu_->hll_config = MVM_hll_get_config_for(tc, str);

      // setup callsite
      cu_->callsites = (MVMCallsite**)malloc(sizeof(MVMCallsite*)*callsites_.size());
      {
        int i=0;
        for (auto callsite: callsites_) {
          cu_->callsites[i] = callsite;
          ++i;
        }
      }
      cu_->num_callsites = callsites_.size();
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
      int apr_return_status;
      apr_pool_t  *pool        = NULL;
      /* Ensure the file exists, and get its size. */
      if ((apr_return_status = apr_pool_create(&pool, NULL)) != APR_SUCCESS) {
        MVM_panic(MVM_exitcode_compunit, "Could not allocate APR memory pool: errorcode %d", apr_return_status);
      }
      cu_->pool       = pool;
      this->push_frame("frame_name_0");
    }

    int push_string(const std::string &str) {
      return this->push_string(str.c_str(), str.size());
    }

    Assembler & assembler() {
      return frames_.back()->assembler();
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
      return frames_.back()->push_local_type(reg_type);
    }
    // Get register type at 'n'
    uint16_t get_local_type(int n) {
      return frames_.back()->get_local_type(n);
    }
    // Push lexical variable.
    int push_lexical(const std::string name, MVMuint16 type) {
      return frames_.back()->push_lexical(name, type);
    }

    // lexical variable number by name
    // TODO: find from outer frame?
    int find_lexical_by_name(const std::string &name_cc, int &outer) {
      return frames_.back()->find_lexical_by_name(name_cc, outer);
    }

    int push_frame(const std::string & name) {
      frames_.push_back(std::make_shared<Frame>(vm_->main_thread, name));
      used_frames_.push_back(frames_.back());
      return frames_.size()-1;
    }
    void pop_frame() {
      frames_.pop_back();
    }

    size_t push_callsite(MVMCallsite *callsite) {
      // TODO: Make it unique?
      callsites_.push_back(callsite);
      return callsites_.size() - 1;
    }

    void run() {
      this->finalize();

      assert(cu_->main_frame);
      assert(cu_->main_frame->bytecode);
      assert(cu_->main_frame->bytecode_size > 0);

      MVMThreadContext *tc = vm_->main_thread;
      MVMStaticFrame *start_frame = cu_->main_frame ? cu_->main_frame : cu_->frames[0];
      MVM_interp_run(tc, &toplevel_initial_invoke, start_frame);
    }

    void dump() {
      this->finalize();

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

    Assembler& assembler() {
      return interp_.assembler(); // FIXME ugly
    }

    int do_compile(const saru::Node &node) {
      // printf("node: %s\n", node.type_name());
      switch (node.type()) {
      case NODE_RETURN: {
        assert(node.children().size() ==1);
        auto reg = do_compile(node.children()[0]);
        if (reg < 0) {
          MVM_panic(MVM_exitcode_compunit, "Compilation error. return with non-value.");
        }
        switch (interp_.get_local_type(reg)) {
        case MVM_reg_int64:
          assembler().return_i(reg);
          break;
        case MVM_reg_str:
          assembler().return_s(reg);
          break;
        case MVM_reg_obj:
          assembler().return_o(reg);
          break;
        default:
          abort();
        }
        return -1;
      }
      case NODE_STRING: {
        int str_num = interp_.push_string(node.pv());
        int reg_num = interp_.push_local_type(MVM_reg_str);
        assembler().const_s(reg_num, str_num);
        return reg_num;
      }
      case NODE_INT: {
        uint16_t reg_num = interp_.push_local_type(MVM_reg_int64);
        int64_t n = node.iv();
        assembler().const_i64(reg_num, n);
        return reg_num;
      }
      case NODE_NUMBER: {
        uint16_t reg_num = interp_.push_local_type(MVM_reg_num64);
        MVMnum64 n = node.nv();
        assembler().const_n64(reg_num, n);
        return reg_num;
      }
      case NODE_BIND: {
        auto lhs = node.children()[0];
        auto rhs = node.children()[1];
        if (lhs.type() != NODE_MY) {
          printf("You can't bind value to %s, currently.\n", lhs.type_name());
          abort();
        }
        int lex_no = do_compile(lhs);
        int val    = this->box(do_compile(rhs));
        assembler().bindlex(
          lex_no, // lex number
          0,      // frame outer count
          val     // value
        );
        return -1;
      }
      case NODE_FUNC: {
        const std::string& name = node.children()[0].pv();


        // Compile function body
        auto frame = interp_.push_frame(name);

        // TODO process named params
        // TODO process types
        {
          assembler().checkarity(
              node.children()[1].children().size(),
              node.children()[1].children().size()
          );

          int i=0;
          for (auto n: node.children()[1].children()) {
            int reg = interp_.push_local_type(MVM_reg_obj);
            int lex = interp_.push_lexical(n.pv(), MVM_reg_obj);
            assembler().param_rp_o(reg, i);
            assembler().bindlex(lex, 0, reg);
            ++i;
          }
        }

        bool returned = false;
        {
          const Node &stmts = node.children()[2];
          assert(stmts.type() == NODE_STATEMENTS);
          for (auto iter=stmts.children().begin()
              ; iter!=stmts.children().end() ; ++iter) {
            int reg = do_compile(*iter);
            if (iter==stmts.children().end()-1 && reg >= 0) {
              switch (interp_.get_local_type(reg)) {
              case MVM_reg_int64:
                assembler().return_i(reg);
                break;
              case MVM_reg_str:
                assembler().return_s(reg);
                break;
              case MVM_reg_obj:
                assembler().return_o(reg);
                break;
              case MVM_reg_num64:
                assembler().return_n(reg);
                break;
              default: abort();
              }
              returned = true;
            }
          }

          // return null
          if (!returned) {
            int reg = interp_.push_local_type(MVM_reg_obj);
            assembler().null(reg);
            assembler().return_o(reg);
          }
        }
        interp_.pop_frame();

        auto funcreg = interp_.push_local_type(MVM_reg_obj);
        auto funclex = interp_.push_lexical(std::string("&") + name, MVM_reg_obj);
        assembler().getcode(funcreg, frame);
        assembler().bindlex(
            funclex,
            0, // frame outer count
            funcreg
        );

        return funcreg;
      }
      case NODE_VARIABLE: {
        // copy lexical variable to register
        auto reg_no = interp_.push_local_type(MVM_reg_obj);
        int outer = 0;
        auto lex_no = interp_.find_lexical_by_name(node.pv(), outer);
        assembler().getlex(
          reg_no,
          lex_no,
          outer // outer frame
        );
        return reg_no;
      }
      case NODE_MY: {
        if (node.children().size() != 1) {
          printf("NOT IMPLEMENTED\n");
          abort();
        }
        auto n = node.children()[0];
        if (n.type() != NODE_VARIABLE) {
          printf("This is variable: %s\n", n.type_name());
          exit(0);
        }
        int idx = interp_.push_lexical(n.pv(), MVM_reg_obj);
        return idx;
      }
      case NODE_IF: {
        auto cond  = node.children()[0];
        auto stmts = node.children()[1];
        auto cond_reg = do_compile(cond);
        switch (interp_.get_local_type(cond_reg)) {
        case MVM_reg_int64: {
          uint16_t pos = assembler().bytecode_size() + 2 + 2;
          assembler().unless_i(cond_reg, 0);
          (void)do_compile(stmts);
          assembler().write_uint32_t(assembler().bytecode_size(), pos);
          return -1;
        }
        case MVM_reg_num64: {
          uint16_t pos = assembler().bytecode_size() + 2 + 2;
          assembler().unless_n(cond_reg, 0);
          (void)do_compile(stmts);
          assembler().write_uint32_t(assembler().bytecode_size(), pos);
          return -1;
        }
        case MVM_reg_str: {
          uint16_t pos = assembler().bytecode_size() + 2 + 2;
          assembler().unless_s(cond_reg, 0);
          (void)do_compile(stmts);
          assembler().write_uint32_t(assembler().bytecode_size(), pos);
          return -1;
        }
        case MVM_reg_obj: {
          uint16_t pos = assembler().bytecode_size() + 2 + 2;
          assembler().unless_o(cond_reg, 0);
          (void)do_compile(stmts);
          assembler().write_uint32_t(assembler().bytecode_size(), pos);
          return -1;
        }
        default:
          abort(); // should not reach here?
        }
      }
      case NODE_IDENT:
        break;
      case NODE_STATEMENTS:
        for (auto n: node.children()) {
          do_compile(n);
        }
        return -1;
      case NODE_STRING_CONCAT: {
        auto dst_reg = interp_.push_local_type(MVM_reg_str);
        auto lhs = node.children()[0];
        auto rhs = node.children()[1];
        auto l = stringify(do_compile(lhs));
        auto r = stringify(do_compile(rhs));
        assembler().concat_s(
          dst_reg,
          l,
          r
        );
        return dst_reg;
      }
      case NODE_ARRAY: {
        // create array
        auto array_reg = interp_.push_local_type(MVM_reg_obj);
        assembler().hlllist(array_reg);
        assembler().create(array_reg, array_reg);

        // push elements
        for (auto n:node.children()) {
          auto reg = this->box(do_compile(n));
          assembler().push_o(array_reg, reg);
        }
        return array_reg;
      }
      case NODE_ATPOS: {
        assert(node.children().size() == 2);
        auto container = do_compile(node.children()[0]);
        auto idx       = this->to_i(do_compile(node.children()[1]));
        auto dst = interp_.push_local_type(MVM_reg_obj);
        assembler().atpos_o(dst, container, idx);
        return dst;
      }
      case NODE_METHODCALL: {
        assert(node.children().size() == 3);
        auto obj = do_compile(node.children()[0]);
        auto str = interp_.push_string(node.children()[1].pv());
        auto meth = interp_.push_local_type(MVM_reg_obj);
        auto ret = interp_.push_local_type(MVM_reg_obj);

        // TODO process args
        assembler().findmeth(meth, obj, str);

        MVMCallsite* callsite = new MVMCallsite;
        memset(callsite, 0, sizeof(MVMCallsite));
        callsite->arg_count = 1;
        callsite->num_pos = 1;
        callsite->arg_flags = new MVMCallsiteEntry[1];
        callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;;

        auto callsite_no = interp_.push_callsite(callsite);
        assembler().prepargs(callsite_no);

        assembler().arg_o(0, obj);
        assembler().invoke_o(ret, meth);
        return ret;
      }
      case NODE_EQ: {
        return this->numeric_binop(node, MVM_OP_eq_i, MVM_OP_eq_n);
      }
      case NODE_NE: {
        return this->numeric_binop(node, MVM_OP_ne_i, MVM_OP_ne_n);
      }
      case NODE_LT: {
        return this->numeric_binop(node, MVM_OP_lt_i, MVM_OP_lt_n);
      }
      case NODE_LE: {
        return this->numeric_binop(node, MVM_OP_le_i, MVM_OP_le_n);
      }
      case NODE_GT: {
        return this->numeric_binop(node, MVM_OP_gt_i, MVM_OP_gt_n);
      }
      case NODE_GE: {
        return this->numeric_binop(node, MVM_OP_ge_i, MVM_OP_ge_n);
      }
      case NODE_MUL: {
        return this->numeric_binop(node, MVM_OP_mul_i, MVM_OP_mul_n);
      }
      case NODE_SUB: {
        return this->numeric_binop(node, MVM_OP_sub_i, MVM_OP_sub_n);
      }
      case NODE_DIV: {
        return this->numeric_binop(node, MVM_OP_div_i, MVM_OP_div_n);
      }
      case NODE_ADD: {
        return this->numeric_binop(node, MVM_OP_add_i, MVM_OP_add_n);
      }
      case NODE_MOD: {
        return this->numeric_binop(node, MVM_OP_mod_i, MVM_OP_mod_n);
      }
      case NODE_FUNCALL: {
        assert(node.children().size() == 2);
        const saru::Node &ident = node.children()[0];
        const saru::Node &args  = node.children()[1];
        if (ident.pv() == "say") {
          for (auto a:args.children()) {
            uint16_t reg_num = stringify(do_compile(a));
            assembler().say(reg_num);
            return -1; // TODO: Is there a result?
          }
          return -1; // TODO: Is there a result?
        } else {
          auto reg_no = interp_.push_local_type(MVM_reg_obj);
          int outer = 0;
          auto lex_no = interp_.find_lexical_by_name(std::string("&") + ident.pv(), outer);
          assembler().getlex(
            reg_no,
            lex_no,
            outer // outer frame
          );

          {
            MVMCallsite* callsite = new MVMCallsite;
            memset(callsite, 0, sizeof(MVMCallsite));
            // TODO support named params
            callsite->arg_count = args.children().size();
            callsite->num_pos   = args.children().size();

            auto callsite_no = interp_.push_callsite(callsite);
            assembler().prepargs(callsite_no);

            callsite->arg_flags = new MVMCallsiteEntry[args.children().size()];

            int i=0;
            for (auto a:args.children()) {
              auto reg = do_compile(a);
              if (reg<0) {
                MVM_panic(MVM_exitcode_compunit, "Compilation error. You should not pass void function as an argument: %s", a.type_name());
              }

              switch (interp_.get_local_type(reg)) {
              case MVM_reg_int64:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_INT;
                assembler().arg_i(i, reg);
                break;
              case MVM_reg_num64:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_NUM;
                assembler().arg_n(i, reg);
                break;
              case MVM_reg_str:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_STR;
                assembler().arg_s(i, reg);
                break;
              case MVM_reg_obj:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_OBJ;
                assembler().arg_o(i, reg);
                break;
              default:
                abort();
              }
              ++i;
            }
          }
          auto dest_reg = interp_.push_local_type(MVM_reg_obj);
          assembler().invoke_o(
              dest_reg,
              reg_no
          );
          return dest_reg;
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
        assembler().hllboxtype_s(boxtype_reg);
        assembler().box_s(dst_num, reg_num, boxtype_reg);
        return dst_num;
      case MVM_reg_int64:
        assembler().hllboxtype_i(boxtype_reg);
        assembler().box_i(dst_num, reg_num, boxtype_reg);
        return dst_num;
      case MVM_reg_num64:
        assembler().hllboxtype_n(boxtype_reg);
        assembler().box_n(dst_num, reg_num, boxtype_reg);
        return dst_num;
      default:
        MVM_panic(MVM_exitcode_compunit, "Not implemented, boxify %d", interp_.get_local_type(reg_num));
        abort();
      }
    }
    int to_n(int reg_num) {
      assert(reg_num >= 0);
      switch (interp_.get_local_type(reg_num)) {
      case MVM_reg_str: {
        int dst_num = interp_.push_local_type(MVM_reg_num64);
        assembler().coerce_sn(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_num64: {
        return reg_num;
      }
      case MVM_reg_int64: {
        int dst_num = interp_.push_local_type(MVM_reg_num64);
        assembler().coerce_in(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_obj: {
        int dst_num = interp_.push_local_type(MVM_reg_num64);
        assembler().unbox_n(dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", interp_.get_local_type(reg_num));
        break;
      }
    }
    int to_i(int reg_num) {
      assert(reg_num >= 0);
      switch (interp_.get_local_type(reg_num)) {
      case MVM_reg_str: {
        int dst_num = interp_.push_local_type(MVM_reg_int64);
        assembler().coerce_si(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_num64: {
        int dst_num = interp_.push_local_type(MVM_reg_num64);
        assembler().coerce_ni(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_int64: {
        return reg_num;
      }
      case MVM_reg_obj: {
        int dst_num = interp_.push_local_type(MVM_reg_int64);
        assembler().unbox_i(dst_num, reg_num);
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
      case MVM_reg_num64: {
        int dst_num = interp_.push_local_type(MVM_reg_str);
        assembler().coerce_ns(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_int64: {
        int dst_num = interp_.push_local_type(MVM_reg_str);
        assembler().coerce_is(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_obj: {
        int dst_num = interp_.push_local_type(MVM_reg_str);
        assembler().smrt_strify(dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, stringify %d", interp_.get_local_type(reg_num));
        break;
      }
    }
    int numeric_binop(const saru::Node& node, uint16_t op_i, uint16_t op_n) {
        assert(node.children().size() == 2);

        int reg_num1 = do_compile(node.children()[0]);
        if (interp_.get_local_type(reg_num1) == MVM_reg_int64) {
          int reg_num_dst = interp_.push_local_type(MVM_reg_int64);
          int reg_num2 = this->to_i(do_compile(node.children()[1]));
          assert(interp_.get_local_type(reg_num1) == MVM_reg_int64);
          assert(interp_.get_local_type(reg_num2) == MVM_reg_int64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_i, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (interp_.get_local_type(reg_num1) == MVM_reg_num64) {
          int reg_num_dst = interp_.push_local_type(MVM_reg_num64);
          int reg_num2 = this->to_n(do_compile(node.children()[1]));
          assert(interp_.get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (interp_.get_local_type(reg_num1) == MVM_reg_obj) {
          // TODO should I use intify instead if the object is int?
          int reg_num_dst = interp_.push_local_type(MVM_reg_num64);

          int dst_num = interp_.push_local_type(MVM_reg_num64);
          assembler().op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_smrt_numify, dst_num, reg_num1);

          int reg_num2 = this->to_n(do_compile(node.children()[1]));
          assert(interp_.get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);
          return reg_num_dst;
        } else {
          // NOT IMPLEMENTED
          abort();
        }
    }
  public:
    Compiler(Interpreter &interp): interp_(interp) { }
    void compile(saru::Node &node) {
      assembler().checkarity(0, -1);

      do_compile(node);

      // bootarray
      /*
      int ary = interp_.push_local_type(MVM_reg_obj);
      assembler().bootarray(ary);
      assembler().create(ary, ary);
      assembler().prepargs(1);
      assembler().invoke_o(ary, ary);

      assembler().bootstrarray(ary);
      assembler().create(ary, ary);
      assembler().prepargs(1);
      assembler().invoke_o(ary, ary);
      */

      // final op must be return.
      int reg = interp_.push_local_type(MVM_reg_obj);
      assembler().null(reg);
      assembler().return_o(reg);
    }
  };
}

