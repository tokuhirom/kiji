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
  void bootstrap_Array(MVMCompUnit* cu, MVMThreadContext*tc);
  void bootstrap_Str(MVMCompUnit* cu, MVMThreadContext*tc);
  void bootstrap_Hash(MVMCompUnit* cu, MVMThreadContext*tc);
  void bootstrap_File(MVMCompUnit* cu, MVMThreadContext*tc);

  class ClassBuilder {
  private:
    MVMObject*obj_;
    MVMThreadContext *tc_;
    MVMObject*cache_;
  public:
    ClassBuilder(MVMObject*obj, MVMThreadContext*tc) : obj_(obj), tc_(tc) {
      cache_ = REPR(tc_->instance->boot_types->BOOTHash)->allocate(tc_, STABLE(tc_->instance->boot_types->BOOTHash));
    }
    ~ClassBuilder() {
      STABLE(obj_)->method_cache = cache_;
    }
    void add_method(const char*name_c, size_t name_len, void (*func) (MVMThreadContext *, MVMCallsite *, MVMRegister *)) {
      MVMString *string = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_c, name_len);
      MVMObject * BOOTCCode = tc_->instance->boot_types->BOOTCCode;
      MVMObject* code_obj = REPR(BOOTCCode)->allocate(tc_, STABLE(BOOTCCode));
      ((MVMCFunction *)code_obj)->body.func = func;
      REPR(cache_)->ass_funcs->bind_key_boxed(
          tc_,
          STABLE(cache_),
          cache_,
          OBJECT_BODY(cache_),
          (MVMObject*)string,
          code_obj);
    }
  };

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

    MVMStaticFrame* finalize() {
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

      return &frame_;
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

    void set_outer(const std::shared_ptr<Frame>&frame) {
      frame_.outer = &(frame->frame_);
    }

    // lexical variable number by name
    int find_lexical_by_name(const std::string &name_cc, int &outer) {
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      MVMStaticFrame *f = &frame_;
      outer = 0;
      while (f) {
        MVMLexicalHashEntry *lexical_names = f->lexical_names;
        MVMLexicalHashEntry *entry;
        MVM_HASH_GET(tc_, lexical_names, name, entry);

        if (entry) {
          return entry->value;
        }
        f = f->outer;
        ++outer;
      }
      printf("Unknown lexical variable: %s\n", name_cc.c_str());
      exit(0);
    }
  };

  class CompUnit {
  private:
    MVMThreadContext *tc_;
    MVMCompUnit*cu_;
    std::vector<MVMString*> strings_;
    std::vector<std::shared_ptr<Frame>> frames_;
    std::list<std::shared_ptr<Frame>> used_frames_;
    std::vector<MVMCallsite*> callsites_;
  public:
    CompUnit(MVMThreadContext* tc) :tc_(tc) {
      cu_ = (MVMCompUnit*)malloc(sizeof(MVMCompUnit));
      memset(cu_, 0, sizeof(MVMCompUnit));
    }
    ~CompUnit() {
      free(cu_);
    }
    void finalize(MVMInstance* vm) {
      MVMThreadContext *tc = tc_; // remove me
      MVMInstance *vm_ = vm; // remove me

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
          cu_->frames[i] = frame->finalize();
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
      {
        MVMString *hll_name = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"saru");
        cu_->hll_config = MVM_hll_get_config_for(tc, hll_name);

        MVMObject *config = REPR(tc->instance->boot_types->BOOTHash)->allocate(tc, STABLE(tc->instance->boot_types->BOOTHash));
        // MVMObject *key = (MVMObject *)MVM_string_utf8_decode((tc), (tc)->instance->VMString, "list", strlen("list");
        // MVM_HASH_BIND(tc, frames[i]->lexical_names, key, entry);
        MVM_hll_set_config(tc, hll_name, config);
      }

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
      cu_->max_callsite_size = 0;
      for (auto callsite: callsites_) {
        cu_->max_callsite_size = std::max(cu_->max_callsite_size, callsite->arg_count);
      }

      // Initialize @*ARGS
      MVMObject *clargs = MVM_repr_alloc_init(tc, tc->instance->boot_types->BOOTArray);
      MVM_gc_root_add_permanent(tc, (MVMCollectable **)&clargs);
      auto handle = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"__SARU_CORE__");
      auto sc = (MVMSerializationContext *)MVM_sc_create(tc, handle);
      MVMROOT(tc, sc, {
        MVMROOT(tc, clargs, {
          MVMint64 count;
          for (count = 0; count < vm_->num_clargs; count++) {
            MVMString *string = MVM_string_utf8_decode(tc,
              tc->instance->VMString,
              vm_->raw_clargs[count], strlen(vm_->raw_clargs[count]));
            MVMObject*type = cu_->hll_config->str_box_type;
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

      // hacking hll
      bootstrap_Array(cu_, tc);
      bootstrap_Str(cu_, tc);
      bootstrap_Hash(cu_, tc);
      bootstrap_File(cu_, tc);

      cu_->num_scs = 1;
      cu_->scs = (MVMSerializationContext**)malloc(sizeof(MVMSerializationContext*)*1);
      cu_->scs[0] = sc;
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
      MVMString* str = MVM_string_utf8_decode(tc_, tc_->instance->VMString, string, length);
      strings_.push_back(str);
      return strings_.size() - 1;
    }

    // reserve register
    int push_local_type(MVMuint16 reg_type) {
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
    int find_lexical_by_name(const std::string &name_cc, int &outer) {
      return frames_.back()->find_lexical_by_name(name_cc, outer);
    }

    int push_frame(const std::string & name) {
      std::shared_ptr<Frame> frame = std::make_shared<Frame>(tc_, name);
      if (frames_.size() != 0) {
        frame->set_outer(frames_.back());
      }
      frames_.push_back(frame);
      used_frames_.push_back(frames_.back());
      return frames_.size()-1;
    }
    void pop_frame() {
      frames_.pop_back();
    }
    size_t frame_size() const {
      return frames_.size();
    }

    size_t push_callsite(MVMCallsite *callsite) {
      // TODO: Make it unique?
      callsites_.push_back(callsite);
      return callsites_.size() - 1;
    }
    MVMStaticFrame * get_start_frame() {
      return cu_->main_frame ? cu_->main_frame : cu_->frames[0];
    }

    void dump(MVMInstance * vm) {
      this->finalize(vm);

      // dump it
      char *dump = MVM_bytecode_dump(tc_, cu_);

      printf("%s", dump);
      free(dump);
    }
  };

  class Interpreter {
  private:
    MVMInstance* vm_;

  protected:
    /*
    MVMObject * mk_boxed_string(const char *name, size_t len) {
      MVMThreadContext *tc = vm_->main_thread;
      MVMString *string = MVM_string_utf8_decode(tc, tc->instance->VMString, name, len);
      MVMObject*type = cu_->hll_config->str_box_type;
      MVMObject *box = REPR(type)->allocate(tc, STABLE(type));
      MVMROOT(tc, box, {
          if (REPR(box)->initialize)
              REPR(box)->initialize(tc, STABLE(box), box, OBJECT_BODY(box));
          REPR(box)->box_funcs->set_str(tc, STABLE(box), box,
              OBJECT_BODY(box), string);
      });
      return box;
    }
    */
  public:
    Interpreter() {
      vm_ = MVM_vm_create_instance();
    }
    ~Interpreter() {
      MVM_vm_destroy_instance(vm_);
    }
    void set_clargs(int n, char**args) {
      vm_->num_clargs = n;
      vm_->raw_clargs = args;
    }
    MVMThreadContext * main_thread() {
      return vm_->main_thread;
    }
    MVMInstance * vm() {
      return vm_;
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

    void run(CompUnit & cu) {
      cu.finalize(vm_);

      MVMThreadContext *tc = vm_->main_thread;
      MVMStaticFrame *start_frame = cu.get_start_frame();
      MVM_interp_run(tc, &toplevel_initial_invoke, start_frame);
    }
  };

  /**
   * OP map is 3rd/MoarVM/src/core/oplist
   * interp code is 3rd/MoarVM/src/core/interp.c
   */
  enum { UNKNOWN_REG = -1 };
  class Compiler {
  private:
    // Interpreter &interp_;
    CompUnit & cu_;

    class Label {
    private:
      Compiler *compiler_;
      ssize_t address_;
      std::vector<ssize_t> reserved_addresses_;
    public:
      Label(Compiler *compiler) :compiler_(compiler), address_(-1) { }
      Label(Compiler *compiler, ssize_t address) :compiler_(compiler), address_(address) { }
      ~Label() {
        assert(address_ != -1 && "Unsolved label");
      }
      ssize_t address() const {
        return address_;
      }
      void put() {
        assert(address_ == -1);
        address_ = compiler_->assembler().bytecode_size();

        // rewrite reserved addresses
        for (auto r: reserved_addresses_) {
          compiler_->assembler().write_uint32_t(address_, r);
        }
        reserved_addresses_.empty();
      }
      void reserve(ssize_t address) {
        reserved_addresses_.push_back(address);
      }
      bool is_solved() const { return address_!=-1; }
    };

    Label label() { return Label(this, assembler().bytecode_size()); }
    Label label_unsolved() { return Label(this); }

    void goto_(Label &label) {
      if (!label.is_solved()) {
        label.reserve(assembler().bytecode_size() + 2);
      }
      assembler().goto_(label.address());
    }
    void if_any(uint16_t reg, Label &label) {
      if (!label.is_solved()) {
        label.reserve(assembler().bytecode_size() + 2 + 2);
      }
      assembler().op_u16_u32(MVM_OP_BANK_primitives, if_op(reg), reg, label.address());
    }
    void unless_any(uint16_t reg, Label &label) {
      if (!label.is_solved()) {
        label.reserve(assembler().bytecode_size() + 2 + 2);
      }
      assembler().op_u16_u32(MVM_OP_BANK_primitives, unless_op(reg), reg, label.address());
    }

    Assembler& assembler() {
      return cu_.assembler(); // FIXME ugly
    }

    int reg_obj() { return cu_.push_local_type(MVM_reg_obj); }
    int reg_str() { return cu_.push_local_type(MVM_reg_str); }
    int reg_int64() { return cu_.push_local_type(MVM_reg_int64); }
    int reg_num64() { return cu_.push_local_type(MVM_reg_num64); }

    // This reg returns register number contains true value.
    int const_true() {
      auto reg = reg_int64();
      assembler().const_i64(reg, 1);
      return reg;
    }

    uint16_t get_local_type(int n) {
      return cu_.get_local_type(n);
    }
    int push_string(const std::string & str) {
      return cu_.push_string(str.c_str(), str.size());
    }
    int push_string(const char*string, int length) {
      return cu_.push_string(string, length);
    }
    // lexical variable number by name
    int find_lexical_by_name(const std::string &name_cc, int &outer) {
      return cu_.find_lexical_by_name(name_cc, outer);
    }
    // Push lexical variable.
    int push_lexical(const std::string &name, MVMuint16 type) {
      return cu_.push_lexical(name, type);
    }
    int push_frame(const std::string & name) {
      return cu_.push_frame(name);
    }
    void pop_frame() {
      cu_.pop_frame();
    }
    size_t push_callsite(MVMCallsite *callsite) {
      return cu_.push_callsite(callsite);
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
        // assembler().return_o(this->box(reg));
        switch (get_local_type(reg)) {
        case MVM_reg_int64:
          assembler().return_i(reg);
          break;
        case MVM_reg_str:
          assembler().return_s(reg);
          break;
        case MVM_reg_obj:
          assembler().return_o(this->box(reg));
          break;
        case MVM_reg_num64:
          assembler().return_n(reg);
          break;
        default:
          MVM_panic(MVM_exitcode_compunit, "Compilation error. Unknown register for returning: %d", get_local_type(reg));
        }
        return UNKNOWN_REG;
      }
      case NODE_DIE: {
        int msg_reg = to_s(do_compile(node.children()[0]));
        int dst_reg = reg_obj();
        assert(msg_reg != UNKNOWN_REG);
        assembler().die(dst_reg, msg_reg);
        return UNKNOWN_REG;
      }
      case NODE_WHILE: {
        /*
         *  label_while:
         *    cond
         *    unless_o label_end
         *    body
         *  label_end:
         */
        auto label_while = label();
          int reg = do_compile(node.children()[0]);
          assert(reg != UNKNOWN_REG);
          auto label_end = label_unsolved();
          unless_any(reg, label_end);
          do_compile(node.children()[1]);
          goto_(label_while);
        label_end.put();
        return UNKNOWN_REG;
      }
      case NODE_STRING: {
        int str_num = push_string(node.pv());
        int reg_num = reg_str();
        assembler().const_s(reg_num, str_num);
        return reg_num;
      }
      case NODE_INT: {
        uint16_t reg_num = reg_int64();
        int64_t n = node.iv();
        assembler().const_i64(reg_num, n);
        return reg_num;
      }
      case NODE_NUMBER: {
        uint16_t reg_num = reg_num64();
        MVMnum64 n = node.nv();
        assembler().const_n64(reg_num, n);
        return reg_num;
      }
      case NODE_BIND: {
        auto lhs = node.children()[0];
        auto rhs = node.children()[1];
        if (lhs.type() == NODE_MY) {
          // my $var := foo;
          int lex_no = do_compile(lhs);
          int val    = this->box(do_compile(rhs));
          assembler().bindlex(
            lex_no, // lex number
            0,      // frame outer count
            val     // value
          );
          return val;
        } else if (lhs.type() == NODE_VARIABLE) {
          int outer;
          auto lex_no = find_lexical_by_name(lhs.pv(), outer);
          int val    = this->box(do_compile(rhs));
          assembler().bindlex(
            lex_no, // lex number
            outer,      // frame outer count
            val     // value
          );
          return val;
          // $var := foo;
        } else {
          printf("You can't bind value to %s, currently.\n", lhs.type_name());
          abort();
        }
      }
      case NODE_FUNC: {
        const std::string& name = node.children()[0].pv();

        auto funcreg = reg_obj();
        auto funclex = push_lexical(std::string("&") + name, MVM_reg_obj);
        auto func_pos = assembler().bytecode_size() + 2 + 2;
        assembler().getcode(funcreg, 0);
        assembler().bindlex(
            funclex,
            0, // frame outer count
            funcreg
        );

        // Compile function body
        auto frame_no = push_frame(name);

        // TODO process named params
        // TODO process types
        {
          assembler().checkarity(
              node.children()[1].children().size(),
              node.children()[1].children().size()
          );

          int i=0;
          for (auto n: node.children()[1].children()) {
            int reg = reg_obj();
            int lex = push_lexical(n.pv(), MVM_reg_obj);
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
              switch (get_local_type(reg)) {
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
            int reg = reg_obj();
            assembler().null(reg);
            assembler().return_o(reg);
          }
        }
        pop_frame();

        assembler().write_uint16_t(frame_no, func_pos);

        return funcreg;
      }
      case NODE_VARIABLE: {
        // copy lexical variable to register
        auto reg_no = reg_obj();
        int outer = 0;
        auto lex_no = find_lexical_by_name(node.pv(), outer);
        assembler().getlex(
          reg_no,
          lex_no,
          outer // outer frame
        );
        return reg_no;
      }
      case NODE_CLARGS: { // @*ARGS
        auto retval = reg_obj();
        assembler().wval(retval, 0,0);
        return retval;
      }
      case NODE_FOR: {
        //   init_iter
        // label_for:
        //   body
        //   shift iter
        //   if_o label_for
        // label_end:
          auto src_reg = box(do_compile(node.children()[0]));
          auto iter_reg = reg_obj();
          auto label_end = label_unsolved();
          assembler().iter(iter_reg, src_reg);
          unless_any(iter_reg, label_end);

        auto label_for = label();

          auto val = reg_obj();
          assembler().shift_o(val, iter_reg);

          int it = push_lexical("$_", MVM_reg_obj);
          assembler().bindlex(it, 0, val);

          do_compile(node.children()[1]);

          if_any(iter_reg, label_for);

        label_end.put();

        return UNKNOWN_REG;
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
        int idx = push_lexical(n.pv(), MVM_reg_obj);
        return idx;
      }
      case NODE_UNLESS: {
        //   cond
        //   if_o label_end
        //   body
        // label_end:
        auto cond_reg = do_compile(node.children()[0]);
        auto label_end = label_unsolved();
        if_any(cond_reg, label_end);
        auto dst_reg = do_compile(node.children()[1]);
        label_end.put();
        return dst_reg;
      }
      case NODE_IF: {
        //   if_cond
        //   if_o if_cond, label_if
        //   elsif1_cond
        //   if_o elsif1_cond, label_elsif1
        //   elsif2_cond
        //   if_o elsif2_cond, label_elsif2
        // label_else:
        //   else_body
        //   goto label_end
        // label_if:
        //   if_body
        //   goto lable_end
        // label_elsif1:
        //   elsif2_body
        //   goto lable_end
        // label_elsif2:
        //   elsif1_body
        //   goto lable_end
        // label_end:

        auto if_cond_reg = do_compile(node.children()[0]);
        auto if_body = node.children()[1];

        auto label_if = label_unsolved();
        if_any(if_cond_reg, label_if);

        // put else if conditions
        std::list<Label> elsif_poses;
        for (auto iter=node.children().begin()+2; iter!=node.children().end(); ++iter) {
          if (iter->type() == NODE_ELSE) {
            break;
          }
          auto reg = do_compile(iter->children()[0]);
          elsif_poses.emplace_back(this);
          if_any(reg, elsif_poses.back());
        }

        // compile else clause
        if (node.children().back().type() == NODE_ELSE) {
            for (auto n: node.children().back().children()) {
              // TODO return data
              do_compile(n);
            }
        }

        auto label_end = label_unsolved();
        goto_(label_end);

        // update if_label and compile if body
        label_if.put();
          (void)do_compile(if_body);
          goto_(label_end);

        // compile else body
        for (auto iter=node.children().begin()+2; iter!=node.children().end(); ++iter) {
          if (iter->type() == NODE_ELSE) {
            break;
          }

          elsif_poses.front().put();
          elsif_poses.pop_back();
          for (auto n: iter->children()[1].children()) {
            // TODO return data
            do_compile(n);
          }
          goto_(label_end);
        }
        assert(elsif_poses.size() == 0);

        label_end.put();

        return -1;
      }
      case NODE_ELSIF:
      case NODE_ELSE: {
        abort();
      }
      case NODE_IDENT:
        break;
      case NODE_STATEMENTS:
        for (auto n: node.children()) {
          // should i return values?
          do_compile(n);
        }
        return UNKNOWN_REG;
      case NODE_STRING_CONCAT: {
        auto dst_reg = reg_str();
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
      case NODE_LIST:
      case NODE_ARRAY: {
        // create array
        auto array_reg = reg_obj();
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
        auto dst = reg_obj();
        assembler().atpos_o(dst, container, idx);
        return dst;
      }
      case NODE_METHODCALL: {
        assert(node.children().size() == 3 || node.children().size()==2);
        auto obj = to_o(do_compile(node.children()[0]));
        auto str = push_string(node.children()[1].pv());
        auto meth = reg_obj();
        auto ret = reg_obj();

        // TODO process args
        assembler().findmeth(meth, obj, str);

        MVMCallsite* callsite = new MVMCallsite;
        memset(callsite, 0, sizeof(MVMCallsite));
        callsite->arg_count = 1;
        callsite->num_pos = 1;
        callsite->arg_flags = new MVMCallsiteEntry[1];
        callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;;

        auto callsite_no = push_callsite(callsite);
        assembler().prepargs(callsite_no);

        assembler().arg_o(0, obj);
        assembler().invoke_o(ret, meth);
        return ret;
      }
      case NODE_CONDITIONAL: {
        /*
         *   cond
         *   unless_o cond, :label_else
         *   if_body
         *   copy dst_reg, result
         *   goto :label_end
         * label_else:
         *   else_bdoy
         *   copy dst_reg, result
         * label_end:
         */
        auto label_end  = label_unsolved();
        auto label_else = label_unsolved();
        auto dst_reg = reg_obj();

          auto cond_reg = do_compile(node.children()[0]);
          unless_any(cond_reg, label_else);

          auto if_reg = do_compile(node.children()[1]);
          assembler().set(dst_reg, to_o(if_reg));
          goto_(label_end);

        label_else.put();
          auto else_reg = do_compile(node.children()[2]);
          assembler().set(dst_reg, to_o(else_reg));

        label_end.put();

        return dst_reg;
      }
      case NODE_NOT: {
        auto src_reg = this->to_i(do_compile(node.children()[0]));
        auto dst_reg = reg_int64();
        assembler().not_i(dst_reg, src_reg);
        return dst_reg;
      }
      case NODE_BIN_AND: {
        return this->binary_binop(node, MVM_OP_band_i);
      }
      case NODE_BIN_OR: {
        return this->binary_binop(node, MVM_OP_bor_i);
      }
      case NODE_BIN_XOR: {
        return this->binary_binop(node, MVM_OP_bxor_i);
      }
      case NODE_EQ: {
        return this->numeric_cmp_binop(node, MVM_OP_eq_i, MVM_OP_eq_n);
      }
      case NODE_NE: {
        return this->numeric_cmp_binop(node, MVM_OP_ne_i, MVM_OP_ne_n);
      }
      case NODE_LT: {
        return this->numeric_cmp_binop(node, MVM_OP_lt_i, MVM_OP_lt_n);
      }
      case NODE_LE: {
        return this->numeric_cmp_binop(node, MVM_OP_le_i, MVM_OP_le_n);
      }
      case NODE_GT: {
        return this->numeric_cmp_binop(node, MVM_OP_gt_i, MVM_OP_gt_n);
      }
      case NODE_GE: {
        return this->numeric_cmp_binop(node, MVM_OP_ge_i, MVM_OP_ge_n);
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
      case NODE_POW: {
        return this->numeric_binop(node, MVM_OP_pow_i, MVM_OP_pow_n);
      }
      case NODE_STREQ:
        return this->str_binop(node, MVM_OP_eq_s);
      case NODE_STRNE:
        return this->str_binop(node, MVM_OP_ne_s);
      case NODE_STRGT:
        return this->str_binop(node, MVM_OP_gt_s);
      case NODE_STRGE:
        return this->str_binop(node, MVM_OP_ge_s);
      case NODE_STRLT:
        return this->str_binop(node, MVM_OP_lt_s);
      case NODE_STRLE:
        return this->str_binop(node, MVM_OP_le_s);
      case NODE_NOP:
        return -1;
      case NODE_ATKEY: {
        assert(node.children().size() == 2);
        auto dst       = reg_obj();
        auto container = to_o(do_compile(node.children()[0]));
        auto key       = to_s(do_compile(node.children()[1]));
        assembler().atkey_o(dst, container, key);
        return dst;
      }
      case NODE_HASH: {
        auto hashtype = reg_obj();
        auto hash     = reg_obj();
        assembler().hllhash(hashtype);
        assembler().create(hash, hashtype);
        for (auto pair: node.children()) {
          assert(pair.type() == NODE_PAIR);
          auto k = to_s(do_compile(pair.children()[0]));
          auto v = to_o(do_compile(pair.children()[1]));
          assembler().bindkey_o(hash, k, v);
        }
        return hash;
      }
      case NODE_LOGICAL_OR: { // '||'
        //   calc_arg1
        //   if_o arg1, label_a1
        //   calc_arg2
        //   set dst_reg, arg2
        //   goto label_end
        // label_a1:
        //   set dst_reg, arg1
        //   goto label_end // omit-able
        // label_end:
        auto label_end = label_unsolved();
        auto label_a1  = label_unsolved();
        auto dst_reg = reg_obj();
          auto arg1 = to_o(do_compile(node.children()[0]));
          if_any(arg1, label_a1);
          auto arg2 = to_o(do_compile(node.children()[1]));
          assembler().set(dst_reg, arg2);
          goto_(label_end);
        label_a1.put();
          assembler().set(dst_reg, arg1);
        label_end.put();
        return dst_reg;
      }
      case NODE_LOGICAL_AND: { // '&&'
        //   calc_arg1
        //   unless_o arg1, label_a1
        //   calc_arg2
        //   set dst_reg, arg2
        //   goto label_end
        // label_a1:
        //   set dst_reg, arg1
        //   goto label_end // omit-able
        // label_end:
        auto label_end = label_unsolved();
        auto label_a1  = label_unsolved();
        auto dst_reg = reg_obj();
          auto arg1 = to_o(do_compile(node.children()[0]));
          unless_any(arg1, label_a1);
          auto arg2 = to_o(do_compile(node.children()[1]));
          assembler().set(dst_reg, arg2);
          goto_(label_end);
        label_a1.put();
          assembler().set(dst_reg, arg1);
        label_end.put();
        return dst_reg;
      }
      case NODE_FUNCALL: {
        assert(node.children().size() == 2);
        const saru::Node &ident = node.children()[0];
        const saru::Node &args  = node.children()[1];
        assert(args.type() == NODE_ARGS);
        if (ident.pv() == "say") {
          int i=0;
          for (auto a:args.children()) {
            uint16_t reg_num = stringify(do_compile(a));
            if (i==args.children().size()-1) {
              assembler().say(reg_num);
            } else {
              assembler().print(reg_num);
            }
            ++i;
          }
          return const_true();
        } else if (ident.pv() == "print") {
          for (auto a:args.children()) {
            uint16_t reg_num = stringify(do_compile(a));
            assembler().print(reg_num);
          }
          return const_true();
        } else if (ident.pv() == "open") {
          // TODO support arguments
          assert(args.children().size() == 1);
          auto fname_s = do_compile(args.children()[0]);
          auto dst_reg_o = reg_obj();
          auto flag_i = reg_int64();
          assembler().const_i64(flag_i, APR_FOPEN_READ); // TODO support other flags, etc.
          auto encoding_flag_i = reg_int64();
          assembler().const_i64(encoding_flag_i, MVM_encoding_type_utf8); // TODO support latin1, etc.
          assembler().open_fh(dst_reg_o, fname_s, flag_i, encoding_flag_i);
          return dst_reg_o;
        } else if (ident.pv() == "slurp") {
          assert(args.children().size() <= 2);
          assert(args.children().size() != 2 && "Encoding option is not supported yet");
          auto fname_s = do_compile(args.children()[0]);
          auto dst_reg_s = reg_str();
          auto encoding_flag_i = reg_int64();
          assembler().const_i64(encoding_flag_i, MVM_encoding_type_utf8); // TODO support latin1, etc.
          assembler().slurp(dst_reg_s, fname_s, encoding_flag_i);
          return dst_reg_s;
        } else {
          auto reg_no = reg_obj();
          int outer = 0;
          auto lex_no = find_lexical_by_name(std::string("&") + ident.pv(), outer);
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

            auto callsite_no = push_callsite(callsite);
            callsite->arg_flags = new MVMCallsiteEntry[args.children().size()];

            std::vector<uint16_t> arg_regs;

            int i=0;
            for (auto a:args.children()) {
              auto reg = do_compile(a);
              if (reg<0) {
                MVM_panic(MVM_exitcode_compunit, "Compilation error. You should not pass void function as an argument: %s", a.type_name());
              }

              switch (get_local_type(reg)) {
              case MVM_reg_int64:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_INT;
                arg_regs.push_back(reg);
                break;
              case MVM_reg_num64:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_NUM;
                arg_regs.push_back(reg);
                break;
              case MVM_reg_str:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_STR;
                arg_regs.push_back(reg);
                break;
              case MVM_reg_obj:
                callsite->arg_flags[i] = MVM_CALLSITE_ARG_OBJ;
                arg_regs.push_back(reg);
                break;
              default:
                abort();
              }
              ++i;
            }

            assembler().prepargs(callsite_no);

            i=0;
            for (auto reg:arg_regs) {
              switch (get_local_type(reg)) {
              case MVM_reg_int64:
                assembler().arg_i(i, reg);
                break;
              case MVM_reg_num64:
                assembler().arg_n(i, reg);
                break;
              case MVM_reg_str:
                assembler().arg_s(i, reg);
                break;
              case MVM_reg_obj:
                assembler().arg_o(i, reg);
                break;
              default:
                abort();
              }
              ++i;
            }
          }
          auto dest_reg = reg_obj();
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
    int to_o(int reg_num) { return box(reg_num); }
    int box(int reg_num) {
      assert(reg_num != UNKNOWN_REG);
      auto reg_type = get_local_type(reg_num);
      if (reg_type == MVM_reg_obj) {
        return reg_num;
      }

      int dst_num = reg_obj();
      int boxtype_reg = reg_obj();
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
        MVM_panic(MVM_exitcode_compunit, "Not implemented, boxify %d", get_local_type(reg_num));
        abort();
      }
    }
    int to_n(int reg_num) {
      assert(reg_num != UNKNOWN_REG);
      switch (get_local_type(reg_num)) {
      case MVM_reg_str: {
        int dst_num = reg_num64();
        assembler().coerce_sn(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_num64: {
        return reg_num;
      }
      case MVM_reg_int64: {
        int dst_num = reg_num64();
        assembler().coerce_in(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_obj: {
        int dst_num = reg_num64();
        assembler().smrt_numify(dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", get_local_type(reg_num));
        break;
      }
    }
    int to_i(int reg_num) {
      assert(reg_num != UNKNOWN_REG);
      switch (get_local_type(reg_num)) {
      case MVM_reg_str: {
        int dst_num = reg_int64();
        assembler().coerce_si(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_num64: {
        int dst_num = reg_num64();
        assembler().coerce_ni(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_int64: {
        return reg_num;
      }
      case MVM_reg_obj: {
        int dst_num = reg_int64();
        assembler().unbox_i(dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", get_local_type(reg_num));
        break;
      }
    }
    int to_s(int reg_num) { return stringify(reg_num); }
    int stringify(int reg_num) {
      assert(reg_num != UNKNOWN_REG);
      switch (get_local_type(reg_num)) {
      case MVM_reg_str:
        // nop
        return reg_num;
      case MVM_reg_num64: {
        int dst_num = reg_str();
        assembler().coerce_ns(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_int64: {
        int dst_num = reg_str();
        assembler().coerce_is(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_obj: {
        int dst_num = reg_str();
        assembler().smrt_strify(dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, stringify %d", get_local_type(reg_num));
        break;
      }
    }
    int numeric_cmp_binop(const saru::Node& node, uint16_t op_i, uint16_t op_n) {
        assert(node.children().size() == 2);

        int reg_num1 = do_compile(node.children()[0]);
        int reg_num_dst = reg_int64();
        if (get_local_type(reg_num1) == MVM_reg_int64) {
          int reg_num2 = this->to_i(do_compile(node.children()[1]));
          assert(get_local_type(reg_num1) == MVM_reg_int64);
          assert(get_local_type(reg_num2) == MVM_reg_int64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_i, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_num64) {
          int reg_num2 = this->to_n(do_compile(node.children()[1]));
          assert(get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_obj) {
          // TODO should I use intify instead if the object is int?
          int dst_num = reg_num64();
          assembler().op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_smrt_numify, dst_num, reg_num1);

          int reg_num2 = this->to_n(do_compile(node.children()[1]));
          assert(get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);
          return reg_num_dst;
        } else {
          // NOT IMPLEMENTED
          abort();
        }
    }
    int str_binop(const saru::Node& node, uint16_t op) {
        assert(node.children().size() == 2);

        int reg_num1 = to_s(do_compile(node.children()[0]));
        int reg_num2 = to_s(do_compile(node.children()[1]));
        int reg_num_dst = reg_int64();
        assembler().op_u16_u16_u16(MVM_OP_BANK_string, op, reg_num_dst, reg_num1, reg_num2);
        return reg_num_dst;
    }
    int binary_binop(const saru::Node& node, uint16_t op_i) {
        assert(node.children().size() == 2);

        int reg_num1 = to_i(do_compile(node.children()[0]));
        int reg_num2 = to_i(do_compile(node.children()[1]));
        auto dst_reg = reg_int64();
        assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_i, dst_reg, reg_num1, reg_num2);
        return dst_reg;
    }
    int numeric_binop(const saru::Node& node, uint16_t op_i, uint16_t op_n) {
        assert(node.children().size() == 2);

        int reg_num1 = do_compile(node.children()[0]);
        if (get_local_type(reg_num1) == MVM_reg_int64) {
          int reg_num_dst = reg_int64();
          int reg_num2 = this->to_i(do_compile(node.children()[1]));
          assert(get_local_type(reg_num1) == MVM_reg_int64);
          assert(get_local_type(reg_num2) == MVM_reg_int64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_i, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_num64) {
          int reg_num_dst = reg_num64();
          int reg_num2 = this->to_n(do_compile(node.children()[1]));
          assert(get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_obj) {
          // TODO should I use intify instead if the object is int?
          int reg_num_dst = reg_num64();

          int dst_num = reg_num64();
          assembler().op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_smrt_numify, dst_num, reg_num1);

          int reg_num2 = this->to_n(do_compile(node.children()[1]));
          assert(get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_str) {
          int dst_num = reg_num64();
          assembler().coerce_sn(dst_num, reg_num1);

          int reg_num_dst = reg_num64();
          int reg_num2 = this->to_n(do_compile(node.children()[1]));
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);

          return reg_num_dst;
        } else {
          abort();
        }
    }
    uint16_t unless_op(uint16_t cond_reg) {
      switch (get_local_type(cond_reg)) {
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
    uint16_t if_op(uint16_t cond_reg) {
      switch (get_local_type(cond_reg)) {
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
  public:
    Compiler(CompUnit & cu): cu_(cu) {
      cu_.initialize();
    }
    void compile(saru::Node &node) {
      assembler().checkarity(0, -1);


      /*
      int code = reg_obj();
      int dest_reg = reg_obj();
      assembler().wval(code, 0, 1);
      MVMCallsite* callsite = new MVMCallsite;
      memset(callsite, 0, sizeof(MVMCallsite));
      callsite->arg_count = 0;
      callsite->num_pos = 0;
      callsite->arg_flags = new MVMCallsiteEntry[0];
      // callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;;

      auto callsite_no = interp_.push_callsite(callsite);
      assembler().prepargs(callsite_no);
      assembler().invoke_o( dest_reg, code);
      */

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
      int reg = reg_obj();
      assembler().null(reg);
      assembler().return_o(reg);
    }
  };
}

#include "builtin/array.h"
#include "builtin/str.h"
#include "builtin/hash.h"
#include "builtin/io.h"
