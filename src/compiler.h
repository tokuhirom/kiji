#pragma once
// vim:ts=2:sw=2:tw=0:

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <stdint.h>
#include <sstream>
#include <memory>
#include "gen.assembler.h"
#include "builtin.h"
#include "pvip.h"

#define MVM_ASSIGN_REF2(tc, update_root, update_addr, referenced) \
    { \
        void *_r = referenced; \
        MVM_WB(tc, update_root, _r); \
        update_addr = (MVMObject*)_r; \
    }

#define PVIPSTRING2STDSTRING(pv) std::string((pv)->buf, (pv)->len)

// taken from 'compose' function in 6model/bootstrap.c.
static MVMObject* object_compose(MVMThreadContext *tc, MVMObject *self, MVMObject *type_obj) {
    MVMObject *method_table, *attributes, *BOOTArray, *BOOTHash,
              *repr_info_hash, *repr_info, *type_info, *attr_info_list, *parent_info;
    MVMint64   num_attrs, i;

    MVMString *str_name = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"name");
    MVMString * str_type     = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"type");
    MVMString * str_box_target = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"box_target");
    MVMString* str_attribute = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"attribute");
    
    /* Get arguments. */
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    if (!self || !IS_CONCRETE(self) || REPR(self)->ID != MVM_REPR_ID_KnowHOWREPR)
        MVM_exception_throw_adhoc(tc, "KnowHOW methods must be called on object instance with REPR KnowHOWREPR");
    
    /* Fill out STable. */
    method_table = ((MVMKnowHOWREPR *)self)->body.methods;
    MVM_ASSIGN_REF2(tc, STABLE(type_obj), STABLE(type_obj)->method_cache, (void*)method_table);
    STABLE(type_obj)->mode_flags              = MVM_METHOD_CACHE_AUTHORITATIVE;
    STABLE(type_obj)->type_check_cache_length = 1;
    STABLE(type_obj)->type_check_cache        = (MVMObject **)malloc(sizeof(MVMObject *));
    MVM_ASSIGN_REF2(tc, STABLE(type_obj), STABLE(type_obj)->type_check_cache[0], type_obj);
    
    /* Use any attribute information to produce attribute protocol
     * data. The protocol consists of an array... */
    BOOTArray = tc->instance->boot_types->BOOTArray;
    BOOTHash = tc->instance->boot_types->BOOTHash;
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&BOOTArray);
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&BOOTHash);
    repr_info = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&repr_info);
    REPR(repr_info)->initialize(tc, STABLE(repr_info), repr_info, OBJECT_BODY(repr_info));
    
    /* ...which contains an array per MRO entry (just us)... */
    type_info = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&type_info);
    REPR(type_info)->initialize(tc, STABLE(type_info), type_info, OBJECT_BODY(type_info));
    MVM_repr_push_o(tc, repr_info, type_info);
        
    /* ...which in turn contains this type... */
    MVM_repr_push_o(tc, type_info, type_obj);
    
    /* ...then an array of hashes per attribute... */
    attr_info_list = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&attr_info_list);
    REPR(attr_info_list)->initialize(tc, STABLE(attr_info_list), attr_info_list,
        OBJECT_BODY(attr_info_list));
    MVM_repr_push_o(tc, type_info, attr_info_list);
    attributes = ((MVMKnowHOWREPR *)self)->body.attributes;
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&attributes);
    num_attrs = REPR(attributes)->elems(tc, STABLE(attributes),
        attributes, OBJECT_BODY(attributes));
    for (i = 0; i < num_attrs; i++) {
        MVMObject *attr_info = REPR(BOOTHash)->allocate(tc, STABLE(BOOTHash));
        MVMKnowHOWAttributeREPR *attribute = (MVMKnowHOWAttributeREPR *)
            MVM_repr_at_pos_o(tc, attributes, i);
        MVM_gc_root_temp_push(tc, (MVMCollectable **)&attr_info);
        MVM_gc_root_temp_push(tc, (MVMCollectable **)&attribute);
        if (REPR((MVMObject *)attribute)->ID != MVM_REPR_ID_KnowHOWAttributeREPR)
            MVM_exception_throw_adhoc(tc, "KnowHOW attributes must use KnowHOWAttributeREPR");
        
        REPR(attr_info)->initialize(tc, STABLE(attr_info), attr_info,
            OBJECT_BODY(attr_info));
        REPR(attr_info)->ass_funcs->bind_key_boxed(tc, STABLE(attr_info),
            attr_info, OBJECT_BODY(attr_info), (MVMObject *)str_name, (MVMObject *)attribute->body.name);
        REPR(attr_info)->ass_funcs->bind_key_boxed(tc, STABLE(attr_info),
            attr_info, OBJECT_BODY(attr_info), (MVMObject *)str_type, attribute->body.type);
        if (attribute->body.box_target) {
            /* Merely having the key serves as a "yes". */
            REPR(attr_info)->ass_funcs->bind_key_boxed(tc, STABLE(attr_info),
                attr_info, OBJECT_BODY(attr_info), (MVMObject *)str_box_target, attr_info);
        }
        
        MVM_repr_push_o(tc, attr_info_list, attr_info);
        MVM_gc_root_temp_pop_n(tc, 2);
    }
    
    /* ...followed by a list of parents (none). */
    parent_info = REPR(BOOTArray)->allocate(tc, STABLE(BOOTArray));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&parent_info);
    REPR(parent_info)->initialize(tc, STABLE(parent_info), parent_info,
        OBJECT_BODY(parent_info));
    MVM_repr_push_o(tc, type_info, parent_info);
    
    /* Finally, this all goes in a hash under the key 'attribute'. */
    repr_info_hash = REPR(BOOTHash)->allocate(tc, STABLE(BOOTHash));
    MVM_gc_root_temp_push(tc, (MVMCollectable **)&repr_info_hash);
    REPR(repr_info_hash)->initialize(tc, STABLE(repr_info_hash), repr_info_hash, OBJECT_BODY(repr_info_hash));
    REPR(repr_info_hash)->ass_funcs->bind_key_boxed(tc, STABLE(repr_info_hash),
            repr_info_hash, OBJECT_BODY(repr_info_hash), (MVMObject *)str_attribute, repr_info);

    /* Compose the representation using it. */
    REPR(type_obj)->compose(tc, STABLE(type_obj), repr_info_hash);
    
    /* Clear temporary roots. */
    MVM_gc_root_temp_pop_n(tc, 7);
    
    /* Return type object. */
    return type_obj;
}

  static void Mu_new(MVMThreadContext *tc, MVMCallsite *callsite, MVMRegister *args) {
    MVMArgProcContext arg_ctx; arg_ctx.named_used = NULL;
    MVM_args_proc_init(tc, &arg_ctx, callsite, args);
    MVMObject* self     = MVM_args_get_pos_obj(tc, &arg_ctx, 0, MVM_ARG_REQUIRED).arg.o;
    MVM_args_proc_cleanup(tc, &arg_ctx);

    MVMObject *obj = object_compose(tc, STABLE(self)->HOW, self);

    MVM_args_set_result_obj(tc, obj, MVM_RETURN_CURRENT_FRAME);

    MVM_gc_root_temp_pop_n(tc, 1);
  }


namespace kiji {

  enum variable_type_t {
    VARIABLE_TYPE_MY,
    VARIABLE_TYPE_OUR
  };

  void dump_object(MVMThreadContext*tc, MVMObject* obj) {
    if (obj==NULL) {
      printf("(null)\n");
      return;
    }
    MVM_string_say(tc, REPR(obj)->name);
  }

  class Frame {
  private:
    MVMStaticFrame frame_; // frame itself
    std::shared_ptr<Frame> outer_;
    MVMThreadContext *tc_;

    std::vector<MVMuint16> local_types_;
    std::vector<MVMuint16> lexical_types_;
    std::vector<std::string> package_variables_;

    std::vector<MVMFrameHandler*> handlers_;

    Assembler assembler_;

    void set_cuuid() {
      static int cuuid_counter = 0;
      std::ostringstream oss;
      oss << "frame_cuuid_" << cuuid_counter++;
      std::string cuuid = oss.str();
      frame_.cuuid = MVM_string_utf8_decode(tc_, tc_->instance->VMString, cuuid.c_str(), cuuid.size());
    }

  public:
    MVMStaticFrame* frame() { return &frame_; }
    Frame(MVMThreadContext* tc, const std::string name) {
      memset(&frame_, 0, sizeof(MVMFrame));
      tc_ = tc;
      frame_.name = MVM_string_utf8_decode(tc, tc->instance->VMString, name.c_str(), name.size());
    }
    ~Frame(){ }

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

      // bytecode
      frame_.bytecode      = assembler_.bytecode();
      frame_.bytecode_size = assembler_.bytecode_size();

      // frame handlers
      frame_.num_handlers = handlers_.size();
      frame_.handlers = new MVMFrameHandler[handlers_.size()];
      int i=0;
      for (auto f: handlers_) {
        frame_.handlers[i] = *f;
        ++i;
      }

      return &frame_;
    }

    void push_handler(MVMFrameHandler* handler) {
      handlers_.push_back(handler);
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

    // TODO Throw exception at this code: `our $n; my $n`
    void push_pkg_var(const std::string &name_cc) {
      package_variables_.push_back(name_cc);
    }

    void set_outer(const std::shared_ptr<Frame>&frame) {
      frame_.outer = &(frame->frame_);
      outer_ = frame;
    }

    variable_type_t find_variable_by_name(const std::string &name_cc, int &lex_no, int &outer) {
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      Frame* f = this;
      outer = 0;
      while (f) {
        // check lexical variables
        MVMLexicalHashEntry *lexical_names = f->frame_.lexical_names;
        MVMLexicalHashEntry *entry;
        MVM_HASH_GET(tc_, lexical_names, name, entry);

        if (entry) {
          lex_no = entry->value;
          return VARIABLE_TYPE_MY;
        }

        // check package variables
        for (auto n: f->package_variables_) {
          if (n==name_cc) {
            return VARIABLE_TYPE_OUR;
          }
        }

        f = &(*(f->outer_));
        ++outer;
      }
      // TODO I should use MVM_panic instead.
      printf("Unknown lexical variable in find_variable_by_name: %s\n", name_cc.c_str());
      exit(0);
    }

    // lexical variable number by name
    bool find_lexical_by_name(const std::string &name_cc, int *lex_no, int *outer) {
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      MVMStaticFrame *f = &frame_;
      *outer = 0;
      while (f) {
        MVMLexicalHashEntry *lexical_names = f->lexical_names;
        MVMLexicalHashEntry *entry;
        MVM_HASH_GET(tc_, lexical_names, name, entry);

        if (entry) {
          *lex_no= entry->value;
          return true;
        }
        f = f->outer;
        ++(*outer);
      }
      return false;
      // printf("Unknown lexical variable in find_lexical_by_name: %s\n", name_cc.c_str());
      // exit(0);
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
    MVMSerializationContext * sc_classes_;
    int num_sc_classes_;
  public:
    CompUnit(MVMThreadContext* tc) :tc_(tc) {
      cu_ = (MVMCompUnit*)malloc(sizeof(MVMCompUnit));
      memset(cu_, 0, sizeof(MVMCompUnit));

      auto handle = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"__SARU_CLASSES__");
      sc_classes_ = (MVMSerializationContext*)MVM_sc_create(tc_, handle);

      num_sc_classes_ = 0;
    }
    ~CompUnit() {
      free(cu_);
    }
    MVMThreadContext *tc() {  return tc_; }
    MVMCompUnit *cu() {  return cu_; }
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
        MVMString *hll_name = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"kiji");
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
      Kiji_bootstrap_Array(cu_, tc);
      Kiji_bootstrap_Str(cu_, tc);
      Kiji_bootstrap_Hash(cu_, tc);
      Kiji_bootstrap_File(cu_, tc);
      Kiji_bootstrap_Int(cu_, tc);

      cu_->num_scs = 2;
      cu_->scs = (MVMSerializationContext**)malloc(sizeof(MVMSerializationContext*)*2);
      cu_->scs[0] = sc;
      cu_->scs[1] = sc_classes_;
      cu_->scs_to_resolve = (MVMString**)malloc(sizeof(MVMString*)*2);
      cu_->scs_to_resolve[0] = NULL;
      cu_->scs_to_resolve[1] = NULL;
    }

    void push_sc_object(MVMObject * object, int *wval1, int *wval2) {
      num_sc_classes_++;

      *wval1 = 1;
      *wval2 = num_sc_classes_-1;

      MVM_sc_set_object(tc_, sc_classes_, num_sc_classes_-1, object);
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
    // TODO maybe not useful.
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
    void push_pkg_var(const std::string name) {
      frames_.back()->push_pkg_var(name);
    }

    // lexical variable number by name
    bool find_lexical_by_name(const std::string &name_cc, int *lex_no, int *outer) {
      return frames_.back()->find_lexical_by_name(name_cc, lex_no, outer);
    }
    variable_type_t find_variable_by_name(const std::string &name_cc, int &lex_no, int &outer) {
      return frames_.back()->find_variable_by_name(name_cc, lex_no, outer);
    }

    void push_handler(MVMFrameHandler *handler) {
      return frames_.back()->push_handler(handler);
    }
    MVMStaticFrame* get_frame(int frame_no) {
      auto iter = used_frames_.begin();
      for (int i=0; i<frame_no; i++) {
        iter++;
      }
      return (*iter)->frame();
    }
    int push_frame(const std::string & name) {
      std::shared_ptr<Frame> frame = std::make_shared<Frame>(tc_, name);
      if (frames_.size() != 0) {
        frame->set_outer(frames_.back());
      }
      frames_.push_back(frame);
      used_frames_.push_back(frames_.back());
      return used_frames_.size()-1;
    }
    void pop_frame() {
      frames_.pop_back();
    }
    size_t frame_size() const {
      return frames_.size();
    }

    // Is a and b equivalent?
    bool callsite_eq(MVMCallsite *a, MVMCallsite *b) {
      if (a->arg_count != b->arg_count) {
        return false;
      }
      if (a->num_pos != b->num_pos) {
        return false;
      }
      // Should I use memcmp?
      if (a->arg_count !=0) {
        for (int i=0; i<a->arg_count; ++i) {
          if (a->arg_flags[i] != b->arg_flags[i]) {
            return false;
          }
        }
      }
      return true;
    }

    size_t push_callsite(MVMCallsite *callsite) {
      int i=0;
      for (auto c:callsites_) {
        if (callsite_eq(c, callsite)) {
          delete callsite; // free memory
          return i;
        }
        ++i;
      }
      callsites_.push_back(callsite);
      return callsites_.size() - 1;
    }
    MVMStaticFrame * get_start_frame() {
      return cu_->main_frame ? cu_->main_frame : cu_->frames[0];
    }
  };

  /**
   * OP map is 3rd/MoarVM/src/core/oplist
   * interp code is 3rd/MoarVM/src/core/interp.c
   */
  enum { UNKNOWN_REG = -1 };
  class Compiler {
  private:
    CompUnit & cu_;
    int frame_no_;
    MVMObject* current_class_how_;

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
    void return_any(uint16_t reg) {
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

    int compile_class(const PVIPNode* node) {
      int wval1, wval2;
      MVMThreadContext*tc_ = cu_.tc();
      {
        // Create new class.
        MVMObject * type = MVM_6model_find_method(
          tc_,
          STABLE(tc_->instance->KnowHOW)->HOW,
          MVM_string_ascii_decode_nt(tc_, tc_->instance->VMString, (char*)"new_type")
        );
        MVMObject * how = STABLE(type)->HOW;

        // Add "new" method
        {
          MVMString * name = MVM_string_ascii_decode_nt(tc_, tc_->instance->VMString, (char*)"new");
          // self, type_obj, name, method
          MVMObject * method_cache = REPR(tc_->instance->boot_types->BOOTHash)->allocate(tc_, STABLE(tc_->instance->boot_types->BOOTHash));
          // MVMObject * method_cache = REPR(type)->allocate(tc_, STABLE(type));
          // MVMObject * method_table = ((MVMKnowHOWREPR *)type)->body.methods;
          MVMObject * BOOTCCode = tc_->instance->boot_types->BOOTCCode;
          MVMObject * method = REPR(BOOTCCode)->allocate(tc_, STABLE(BOOTCCode));
          ((MVMCFunction *)method)->body.func = Mu_new;
          REPR(method_cache)->ass_funcs->bind_key_boxed(
              tc_,
              STABLE(method_cache),
              method_cache,
              OBJECT_BODY(method_cache),
              (MVMObject*)name,
              method);
          // REPR(method_table)->ass_funcs->bind_key_boxed(tc_, STABLE(method_table),
              // method_table, OBJECT_BODY(method_table), (MVMObject *)name, method);
          STABLE(type)->method_cache = method_cache;
        }

        cu_.push_sc_object(type, &wval1, &wval2);
        current_class_how_ = how;
      }

      // compile body
      for (int i=0; i<node->children.nodes[1]->children.size; i++) {
        PVIPNode *n = node->children.nodes[1]->children.nodes[i];
        (void)do_compile(n);
      }

      current_class_how_ = NULL;

      auto retval = reg_obj();
      assembler().wval(retval, wval1, wval2);

      // Bind class object to lexical variable
      auto name_node = node->children.nodes[0];
      if (PVIP_node_category(name_node->type) == PVIP_CATEGORY_STR) {
        auto lex = push_lexical(PVIPSTRING2STDSTRING(name_node->pv), MVM_reg_obj);
        assembler().bindlex(
          lex,
          0,
          retval
        );
      }

      return retval;
    }

    uint16_t get_variable(PVIPString * name) {
      return get_variable(std::string(name->buf, name->len));
    }

    uint16_t get_variable(const std::string &name) {
      int outer = 0;
      int lex_no = 0;
      variable_type_t vartype = cu_.find_variable_by_name(name, lex_no, outer);
      if (vartype==VARIABLE_TYPE_MY) {
        auto reg_no = reg_obj();
        assembler().getlex(
          reg_no,
          lex_no,
          outer // outer frame
        );
        return reg_no;
      } else {
        int lex_no;
        if (!find_lexical_by_name("$?PACKAGE", &lex_no, &outer)) {
          MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", "$?PACKAGE");
        }
        auto reg = reg_obj();
        auto varname = push_string(name);
        auto varname_s = reg_str();
        assembler().getlex(
          reg,
          lex_no,
          outer // outer frame
        );
        assembler().const_s(
          varname_s,
          varname
        );
        // TODO getwho
        assembler().atkey_o(
          reg,
          reg,
          varname_s
        );
        return reg;
      }
    }
    void set_variable(const PVIPString *name, uint16_t val_reg) {
      set_variable(std::string(name->buf, name->len), val_reg);
    }
    void set_variable(const std::string &name, uint16_t val_reg) {
      int lex_no = -1;
      int outer = -1;
      variable_type_t vartype = cu_.find_variable_by_name(name, lex_no, outer);
      if (vartype==VARIABLE_TYPE_MY) {
        assembler().bindlex(
          lex_no,
          outer,
          val_reg
        );
      } else {
        int lex_no;
        if (!find_lexical_by_name("$?PACKAGE", &lex_no, &outer)) {
          MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", "$?PACKAGE");
        }
        auto reg = reg_obj();
        auto varname = push_string(name);
        auto varname_s = reg_str();
        assembler().getlex(
          reg,
          lex_no,
          outer // outer frame
        );
        assembler().const_s(
          varname_s,
          varname
        );
        // TODO getwho
        assembler().bindkey_o(
          reg,
          varname_s,
          val_reg
        );
      }
    }

    // This reg returns register number contains true value.
    int const_true() {
      auto reg = reg_int64();
      assembler().const_i64(reg, 1);
      return reg;
    }

    uint16_t get_local_type(int n) {
      return cu_.get_local_type(n);
    }
    int push_string(PVIPString* pv) {
      return push_string(pv->buf, pv->len);
    }
    int push_string(const std::string & str) {
      return cu_.push_string(str.c_str(), str.size());
    }
    int push_string(const char*string, int length) {
      return cu_.push_string(string, length);
    }
    // lexical variable number by name
    int find_lexical_by_name(const std::string &name_cc, int *lex_no, int *outer) {
      return cu_.find_lexical_by_name(name_cc, lex_no, outer);
    }
    // Push lexical variable.
    int push_lexical(PVIPString *pv, MVMuint16 type) {
      return push_lexical(std::string(pv->buf, pv->len), type);
    }
    int push_lexical(const std::string &name, MVMuint16 type) {
      return cu_.push_lexical(name, type);
    }
    void push_pkg_var(const std::string &name) {
      cu_.push_pkg_var(name);
    }
    int push_frame(const std::string & name) {
      std::ostringstream oss;
      oss << name << frame_no_++;
      return cu_.push_frame(oss.str());
    }
    void pop_frame() {
      cu_.pop_frame();
    }
    size_t push_callsite(MVMCallsite *callsite) {
      return cu_.push_callsite(callsite);
    }
    void push_handler(MVMFrameHandler *handler) {
      return cu_.push_handler(handler);
    }
    void compile_array(uint16_t array_reg, const PVIPNode* node) {
      if (node->type==PVIP_NODE_LIST) {
        for (int i=0; i<node->children.size; i++) {
          PVIPNode* m = node->children.nodes[i];
          compile_array(array_reg, m);
        }
      } else {
        auto reg = this->box(do_compile(node));
        assembler().push_o(array_reg, reg);
      }
    }

    class LoopGuard {
    private:
      kiji::Compiler *compiler_;
      MVMuint32 start_offset_;
      MVMuint32 last_offset_;
      MVMuint32 next_offset_;
      MVMuint32 redo_offset_;
    public:
      LoopGuard(kiji::Compiler *compiler) :compiler_(compiler) {
        start_offset_ = compiler_->assembler().bytecode_size()-1;
      }
      ~LoopGuard() {
        MVMuint32 end_offset = compiler_->assembler().bytecode_size()-1;

        MVMFrameHandler *last_handler = new MVMFrameHandler;
        last_handler->start_offset = start_offset_;
        last_handler->end_offset = end_offset;
        last_handler->category_mask = MVM_EX_CAT_LAST;
        last_handler->action = MVM_EX_ACTION_GOTO;
        last_handler->block_reg = 0;
        last_handler->goto_offset = last_offset_;
        compiler_->cu_.push_handler(last_handler);

        MVMFrameHandler *next_handler = new MVMFrameHandler;
        next_handler->start_offset = start_offset_;
        next_handler->end_offset = end_offset;
        next_handler->category_mask = MVM_EX_CAT_NEXT;
        next_handler->action = MVM_EX_ACTION_GOTO;
        next_handler->block_reg = 0;
        next_handler->goto_offset = next_offset_;
        compiler_->cu_.push_handler(next_handler);

        MVMFrameHandler *redo_handler = new MVMFrameHandler;
        redo_handler->start_offset = start_offset_;
        redo_handler->end_offset = end_offset;
        redo_handler->category_mask = MVM_EX_CAT_REDO;
        redo_handler->action = MVM_EX_ACTION_GOTO;
        redo_handler->block_reg = 0;
        redo_handler->goto_offset = redo_offset_;
        compiler_->cu_.push_handler(redo_handler);
      }
      // fixme: `put` is not the best verb in English here.
      void put_last() {
        last_offset_ = compiler_->assembler().bytecode_size()-1+1;
      }
      void put_redo() {
        redo_offset_ = compiler_->assembler().bytecode_size()-1+1;
      }
      void put_next() {
        next_offset_ = compiler_->assembler().bytecode_size()-1+1;
      }
    };

    int do_compile(const PVIPNode*node) {
      // printf("node: %s\n", node.type_name());
      switch (node->type) {
      case PVIP_NODE_POSTDEC: { // $i--
        assert(node->children.size == 1);
        if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
          MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
        }
        auto reg_no = get_variable(node->children.nodes[0]->pv);
        auto i_tmp = to_i(reg_no);
        assembler().dec_i(i_tmp);
        set_variable(node->children.nodes[0]->pv, to_o(i_tmp));
        return reg_no;
      }
      case PVIP_NODE_POSTINC: { // $i++
        assert(node->children.size == 1);
        if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
          MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
        }
        auto reg_no = get_variable(node->children.nodes[0]->pv);
        auto i_tmp = to_i(reg_no);
        assembler().inc_i(i_tmp);
        auto dst_reg = to_o(i_tmp);
        set_variable(node->children.nodes[0]->pv, dst_reg);
        return reg_no;
      }
      case PVIP_NODE_PREINC: { // ++$i
        assert(node->children.size == 1);
        if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
          MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
        }
        auto reg_no = get_variable(node->children.nodes[0]->pv);
        auto i_tmp = to_i(reg_no);
        assembler().inc_i(i_tmp);
        auto dst_reg = to_o(i_tmp);
        set_variable(node->children.nodes[0]->pv, dst_reg);
        return dst_reg;
      }
      case PVIP_NODE_PREDEC: { // --$i
        assert(node->children.size == 1);
        if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
          MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
        }
        auto reg_no = get_variable(PVIPSTRING2STDSTRING(node->children.nodes[0]->pv));
        auto i_tmp = to_i(reg_no);
        assembler().dec_i(i_tmp);
        auto dst_reg = to_o(i_tmp);
        set_variable(PVIPSTRING2STDSTRING(node->children.nodes[0]->pv), dst_reg);
        return dst_reg;
      }
      case PVIP_NODE_UNARY_BITWISE_NEGATION: { // +^1
        auto reg = to_i(do_compile(node->children.nodes[0]));
        assembler().bnot_i(reg, reg);
        return reg;
      }
      case PVIP_NODE_BRSHIFT: { // +>
        auto l = to_i(do_compile(node->children.nodes[0]));
        auto r = to_i(do_compile(node->children.nodes[1]));
        assembler().brshift_i(r, l, r);
        return r;
      }
      case PVIP_NODE_BLSHIFT: { // +<
        auto l = to_i(do_compile(node->children.nodes[0]));
        auto r = to_i(do_compile(node->children.nodes[1]));
        assembler().blshift_i(r, l, r);
        return r;
      }
      case PVIP_NODE_ABS: {
        // TODO support abs_n?
        auto r = to_i(do_compile(node->children.nodes[0]));
        assembler().abs_i(r, r);
        return r;
      }
      case PVIP_NODE_LAST: {
        // break from for, while, loop.
        auto ret = reg_obj();
        assembler().throwcatlex(
          ret,
          MVM_EX_CAT_LAST
        );
        return ret;
      }
      case PVIP_NODE_REDO: {
        // redo from for, while, loop.
        auto ret = reg_obj();
        assembler().throwcatlex(
          ret,
          MVM_EX_CAT_REDO
        );
        return ret;
      }
      case PVIP_NODE_NEXT: {
        // continue from for, while, loop.
        auto ret = reg_obj();
        assembler().throwcatlex(
          ret,
          MVM_EX_CAT_NEXT
        );
        return ret;
      }
      case PVIP_NODE_RETURN: {
        assert(node->children.size ==1);
        auto reg = do_compile(node->children.nodes[0]);
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
      case PVIP_NODE_MODULE: {
        // nop, for now.
        return UNKNOWN_REG;
      }
      case PVIP_NODE_USE: {
        assert(node->children.size==1);
        auto name = PVIPSTRING2STDSTRING(node->children.nodes[0]->pv);
        if (name == "v6") {
          return UNKNOWN_REG; // nop.
        }
        std::string path = std::string("lib/") + name + ".p6";
        FILE *fp = fopen(path.c_str(), "rb");
        if (!fp) {
          MVM_panic(MVM_exitcode_compunit, "Cannot open file: %s", path.c_str());
        }
        PVIPNode* root_node = PVIP_parse_fp(fp, 0);
        if (!root_node) {
          MVM_panic(MVM_exitcode_compunit, "Cannot parse: %s", path.c_str());
        }

        auto frame_no = push_frame(path);
        assembler().checkarity(0,0);
        this->do_compile(root_node);
        assembler().return_();
        pop_frame();

        auto code_reg = reg_obj();
        assembler().getcode(code_reg, frame_no);
        MVMCallsite* callsite = new MVMCallsite;
        memset(callsite, 0, sizeof(MVMCallsite));
        callsite->arg_count = 0;
        callsite->num_pos = 0;
        callsite->arg_flags = NULL;
        auto callsite_no = push_callsite(callsite);
        assembler().prepargs(callsite_no);
        assembler().invoke_v(code_reg);

        return UNKNOWN_REG;
      }
      case PVIP_NODE_DIE: {
        int msg_reg = to_s(do_compile(node->children.nodes[0]));
        int dst_reg = reg_obj();
        assert(msg_reg != UNKNOWN_REG);
        assembler().die(dst_reg, msg_reg);
        return UNKNOWN_REG;
      }
      case PVIP_NODE_WHILE: {
        /*
         *  label_while:
         *    cond
         *    unless_o label_end
         *    body
         *  label_end:
         */

        LoopGuard loop(this);

        auto label_while = label();
        loop.put_next();
          int reg = do_compile(node->children.nodes[0]);
          assert(reg != UNKNOWN_REG);
          auto label_end = label_unsolved();
          unless_any(reg, label_end);
          do_compile(node->children.nodes[1]);
          goto_(label_while);
        label_end.put();

        loop.put_last();

        return UNKNOWN_REG;
      }
      case PVIP_NODE_LAMBDA: {
        auto frame_no = push_frame("lambda");
        assembler().checkarity(
          node->children.nodes[0]->children.size,
          node->children.nodes[0]->children.size
        );
        for (int i=0; i<node->children.nodes[0]->children.size; i++) {
          PVIPNode *n = node->children.nodes[0]->children.nodes[i];
          int reg = reg_obj();
          int lex = push_lexical(PVIPSTRING2STDSTRING(n->pv), MVM_reg_obj);
          assembler().param_rp_o(reg, i);
          assembler().bindlex(lex, 0, reg);
        }
        auto retval = do_compile(node->children.nodes[1]);
        if (retval == UNKNOWN_REG) {
          retval = reg_obj();
          assembler().null(retval);
        }
        return_any(retval);
        pop_frame();

        // warn if void context.
        auto dst_reg = reg_obj();
        assembler().getcode(dst_reg, frame_no);

        return dst_reg;
      }
      case PVIP_NODE_BLOCK: {
        auto frame_no = push_frame("block");
        assembler().checkarity(0,0);
        for (int i=0; i<node->children.size; i++) {
          PVIPNode *n = node->children.nodes[i];
          (void)do_compile(n);
        }
        assembler().return_();
        pop_frame();

        auto frame_reg = reg_obj();
        assembler().getcode(frame_reg, frame_no);

        MVMCallsite* callsite = new MVMCallsite;
        memset(callsite, 0, sizeof(MVMCallsite));
        callsite->arg_count = 0;
        callsite->num_pos = 0;
        callsite->arg_flags = NULL;
        auto callsite_no = push_callsite(callsite);
        assembler().prepargs(callsite_no);

        assembler().invoke_v(frame_reg); // trash result

        return UNKNOWN_REG;
      }
      case PVIP_NODE_STRING: {
        int str_num = push_string(node->pv);
        int reg_num = reg_str();
        assembler().const_s(reg_num, str_num);
        return reg_num;
      }
      case PVIP_NODE_INT: {
        uint16_t reg_num = reg_int64();
        int64_t n = node->iv;
        assembler().const_i64(reg_num, n);
        return reg_num;
      }
      case PVIP_NODE_NUMBER: {
        uint16_t reg_num = reg_num64();
        MVMnum64 n = node->nv;
        assembler().const_n64(reg_num, n);
        return reg_num;
      }
      case PVIP_NODE_BIND: {
        auto lhs = node->children.nodes[0];
        auto rhs = node->children.nodes[1];
        if (lhs->type == PVIP_NODE_MY) {
          // my $var := foo;
          int lex_no = do_compile(lhs);
          int val    = this->box(do_compile(rhs));
          assembler().bindlex(
            lex_no, // lex number
            0,      // frame outer count
            val     // value
          );
          return val;
        } else if (lhs->type == PVIP_NODE_OUR) {
          // our $var := foo;
          assert(lhs->children.nodes[0]->type == PVIP_NODE_VARIABLE);
          push_pkg_var(PVIPSTRING2STDSTRING(lhs->children.nodes[0]->pv));
          auto varname = push_string(lhs->children.nodes[0]->pv);
          int val    = to_o(do_compile(rhs));
          int outer = 0;
          int lex_no = 0;
          if (!find_lexical_by_name("$?PACKAGE", &lex_no, &outer)) {
            MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", "$?PACKAGE");
          }
          auto package = reg_obj();
          assembler().getlex(
            package,
            lex_no,
            outer // outer frame
          );
          // TODO getwho
          auto varname_s = reg_str();
          assembler().const_s(varname_s, varname);
          // 0x0F    bindkey_o           r(obj) r(str) r(obj)
          assembler().bindkey_o(
            package,
            varname_s,
            val
          );
          return val;
        } else if (lhs->type == PVIP_NODE_VARIABLE) {
          // $var := foo;
          int val    = this->box(do_compile(rhs));
          set_variable(std::string(lhs->pv->buf, lhs->pv->len), val);
          return val;
        } else {
          printf("You can't bind value to %s, currently.\n", PVIP_node_name(lhs->type));
          abort();
        }
      }
      case PVIP_NODE_METHOD: {
        PVIPNode * name_node = node->children.nodes[0];
        std::string name(name_node->pv->buf, name_node->pv->len);

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
              node->children.nodes[1]->children.size+1,
              node->children.nodes[1]->children.size+1
          );

          {
            // push self
            int lex = push_lexical("__self", MVM_reg_obj);
            int reg = reg_obj();
            assembler().param_rp_o(reg, 0);
            assembler().bindlex(lex, 0, reg);
          }

          for (int i=1; i<node->children.nodes[1]->children.size; ++i) {
            auto n = node->children.nodes[1]->children.nodes[i];
            int reg = reg_obj();
            int lex = push_lexical(n->pv, MVM_reg_obj);
            assembler().param_rp_o(reg, i);
            assembler().bindlex(lex, 0, reg);
            ++i;
          }
        }

        bool returned = false;
        {
          const PVIPNode*stmts = node->children.nodes[2];
          assert(stmts->type == PVIP_NODE_STATEMENTS);
          for (int i=0; i<stmts->children.size ; ++i) {
            PVIPNode * n=stmts->children.nodes[i];
            int reg = do_compile(n);
            if (i==stmts->children.size-1 && reg >= 0) {
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

        // bind method object to class how
        MVMThreadContext*tc_ = cu_.tc();
        if (!current_class_how_) {
          MVM_panic(MVM_exitcode_compunit, "Compilation error. You can't write methods outside of class definition");
        }
        {
          MVMString * methname = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name.c_str(), name.size());
          // self, type_obj, name, method
          MVMObject * method_table = ((MVMKnowHOWREPR *)current_class_how_)->body.methods;
          MVMObject* code_type = cu_.tc()->instance->boot_types->BOOTCode;
          MVMCode *coderef = (MVMCode*)REPR(code_type)->allocate(tc_, STABLE(code_type));
          coderef->body.sf = cu_.get_frame(frame_no);
          REPR(method_table)->ass_funcs->bind_key_boxed(tc_, STABLE(method_table),
              method_table, OBJECT_BODY(method_table), (MVMObject *)methname, (MVMObject*)coderef);
        }

        return funcreg;
      }
      case PVIP_NODE_FUNC: {
        PVIPNode * name_node = node->children.nodes[0];
        std::string name(name_node->pv->buf, name_node->pv->len);

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
              node->children.nodes[1]->children.size,
              node->children.nodes[1]->children.size
          );

          for (int i=0; i<node->children.nodes[1]->children.size; ++i) {
            auto n = node->children.nodes[1]->children.nodes[i];
            int reg = reg_obj();
            int lex = push_lexical(n->pv, MVM_reg_obj);
            assembler().param_rp_o(reg, i);
            assembler().bindlex(lex, 0, reg);
            ++i;
          }
        }

        bool returned = false;
        {
          const PVIPNode*stmts = node->children.nodes[2];
          assert(stmts->type == PVIP_NODE_STATEMENTS);
          for (int i=0; i<stmts->children.size ; ++i) {
            PVIPNode * n=stmts->children.nodes[i];
            int reg = do_compile(n);
            if (i==stmts->children.size-1 && reg >= 0) {
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
      case PVIP_NODE_VARIABLE: {
        // copy lexical variable to register
        return get_variable(std::string(node->pv->buf, node->pv->len));
      }
      case PVIP_NODE_CLARGS: { // @*ARGS
        auto retval = reg_obj();
        assembler().wval(retval, 0,0);
        return retval;
      }
      case PVIP_NODE_CLASS: {
        return compile_class(node);
      }
      case PVIP_NODE_FOR: {
        //   init_iter
        // label_for:
        //   body
        //   shift iter
        //   if_o label_for
        // label_end:

        LoopGuard loop(this);
          auto src_reg = box(do_compile(node->children.nodes[0]));
          auto iter_reg = reg_obj();
          auto label_end = label_unsolved();
          assembler().iter(iter_reg, src_reg);
          unless_any(iter_reg, label_end);

        auto label_for = label();
        loop.put_next();

          auto val = reg_obj();
          assembler().shift_o(val, iter_reg);

          if (node->children.nodes[1]->type == PVIP_NODE_LAMBDA) {
            auto body = do_compile(node->children.nodes[1]);
            MVMCallsite* callsite = new MVMCallsite;
            memset(callsite, 0, sizeof(MVMCallsite));
            callsite->arg_count = 1;
            callsite->num_pos = 1;
            callsite->arg_flags = new MVMCallsiteEntry[1];
            callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
            auto callsite_no = push_callsite(callsite);
            assembler().prepargs(callsite_no);
            assembler().arg_o(0, val);
            assembler().invoke_v(body);
          } else {
            int it = push_lexical("$_", MVM_reg_obj);
            assembler().bindlex(it, 0, val);
            do_compile(node->children.nodes[1]);
          }

          if_any(iter_reg, label_for);

        label_end.put();
        loop.put_last();

        return UNKNOWN_REG;
      }
      case PVIP_NODE_OUR: {
        if (node->children.size != 1) {
          printf("NOT IMPLEMENTED\n");
          abort();
        }
        auto n = node->children.nodes[0];
        if (n->type != PVIP_NODE_VARIABLE) {
          printf("This is variable: %s\n", PVIP_node_name(n->type));
          exit(0);
        }
        push_pkg_var(std::string(n->pv->buf, n->pv->len));
        return UNKNOWN_REG;
      }
      case PVIP_NODE_MY: {
        if (node->children.size != 1) {
          printf("NOT IMPLEMENTED\n");
          abort();
        }
        auto n = node->children.nodes[0];
        if (n->type != PVIP_NODE_VARIABLE) {
          printf("This is variable: %s\n", PVIP_node_name(n->type));
          exit(0);
        }
        int idx = push_lexical(n->pv, MVM_reg_obj);
        return idx;
      }
      case PVIP_NODE_UNLESS: {
        //   cond
        //   if_o label_end
        //   body
        // label_end:
        auto cond_reg = do_compile(node->children.nodes[0]);
        auto label_end = label_unsolved();
        if_any(cond_reg, label_end);
        auto dst_reg = do_compile(node->children.nodes[1]);
        label_end.put();
        return dst_reg;
      }
      case PVIP_NODE_IF: {
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

        auto if_cond_reg = do_compile(node->children.nodes[0]);
        auto if_body = node->children.nodes[1];
        auto dst_reg = reg_obj();

        auto label_if = label_unsolved();
        if_any(if_cond_reg, label_if);

        // put else if conditions
        std::list<Label> elsif_poses;
        for (int i=2; i<node->children.size; ++i) {
          PVIPNode *n = node->children.nodes[i];
          if (n->type == PVIP_NODE_ELSE) {
            break;
          }
          auto reg = do_compile(n->children.nodes[0]);
          elsif_poses.emplace_back(this);
          if_any(reg, elsif_poses.back());
        }

        // compile else clause
        if (node->children.nodes[node->children.size-1]->type == PVIP_NODE_ELSE) {
          compile_statements(node->children.nodes[node->children.size-1], dst_reg);
        }

        auto label_end = label_unsolved();
        goto_(label_end);

        // update if_label and compile if body
        label_if.put();
          compile_statements(if_body, dst_reg);
          goto_(label_end);

        // compile elsif body
        for (int i=2; i<node->children.size; i++) {
          PVIPNode*n = node->children.nodes[i];
          if (n->type == PVIP_NODE_ELSE) {
            break;
          }

          elsif_poses.front().put();
          elsif_poses.pop_front();
          compile_statements(n->children.nodes[1], dst_reg);
          goto_(label_end);
        }
        assert(elsif_poses.size() == 0);

        label_end.put();

        return dst_reg;
      }
      case PVIP_NODE_ELSIF:
      case PVIP_NODE_ELSE: {
        abort();
      }
      case PVIP_NODE_IDENT: {
        int lex_no, outer;
        // auto lex_no = find_lexical_by_name(std::string(node->pv->buf, node->pv->len), outer);
        // class Foo { }; Foo;
        if (find_lexical_by_name(std::string(node->pv->buf, node->pv->len), &lex_no, &outer)) {
          auto reg_no = reg_obj();
          assembler().getlex(
            reg_no,
            lex_no,
            outer // outer frame
          );
          return reg_no;
        // sub fooo { }; foooo;
        } else if (find_lexical_by_name(std::string("&") + std::string(node->pv->buf, node->pv->len), &lex_no, &outer)) {
          auto func_reg_no = reg_obj();
          assembler().getlex(
            func_reg_no,
            lex_no,
            outer // outer frame
          );

          MVMCallsite* callsite = new MVMCallsite;
          memset(callsite, 0, sizeof(MVMCallsite));
          callsite->arg_count = 0;
          callsite->num_pos = 0;
          callsite->arg_flags = new MVMCallsiteEntry[0];

          auto callsite_no = push_callsite(callsite);
          assembler().prepargs(callsite_no);

          auto dest_reg = reg_obj(); // ctx
          assembler().invoke_o(
              dest_reg,
              func_reg_no
          );
          return dest_reg;
        } else {
          MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", std::string(node->pv->buf, node->pv->len).c_str());
        }
      }
      case PVIP_NODE_STATEMENTS:
        for (int i=0; i<node->children.size; i++) {
          PVIPNode*n = node->children.nodes[i];
          // should i return values?
          do_compile(n);
        }
        return UNKNOWN_REG;
      case PVIP_NODE_STRING_CONCAT: {
        auto dst_reg = reg_str();
        auto lhs = node->children.nodes[0];
        auto rhs = node->children.nodes[1];
        auto l = stringify(do_compile(lhs));
        auto r = stringify(do_compile(rhs));
        assembler().concat_s(
          dst_reg,
          l,
          r
        );
        return dst_reg;
      }
      case PVIP_NODE_REPEAT_S: { // x operator
        auto dst_reg = reg_str();
        auto lhs = node->children.nodes[0];
        auto rhs = node->children.nodes[1];
        assembler().repeat_s(
          dst_reg,
          to_s(do_compile(lhs)),
          to_i(do_compile(rhs))
        );
        return dst_reg;
      }
      case PVIP_NODE_LIST:
      case PVIP_NODE_ARRAY: { // TODO: use 6model's container feature after released it.
        // create array
        auto array_reg = reg_obj();
        assembler().hlllist(array_reg);
        assembler().create(array_reg, array_reg);

        // push elements
        for (int i=0; i<node->children.size; i++) {
          PVIPNode*n = node->children.nodes[i];
          compile_array(array_reg, n);
        }
        return array_reg;
      }
      case PVIP_NODE_ATPOS: {
        assert(node->children.size == 2);
        auto container = do_compile(node->children.nodes[0]);
        auto idx       = this->to_i(do_compile(node->children.nodes[1]));
        auto dst = reg_obj();
        assembler().atpos_o(dst, container, idx);
        return dst;
      }
      case PVIP_NODE_IT_METHODCALL: {
        PVIPNode * node_it = PVIP_node_new_string(PVIP_NODE_VARIABLE, "$_", 2);
        PVIPNode * call = PVIP_node_new_children2(PVIP_NODE_METHODCALL, node_it, node->children.nodes[0]);
        if (node->children.size == 2) {
          PVIP_node_push_child(call, node->children.nodes[1]);
        }
        // TODO possibly memory leaks
        return do_compile(call);
      }
      case PVIP_NODE_METHODCALL: {
        assert(node->children.size == 3 || node->children.size==2);
        auto obj = to_o(do_compile(node->children.nodes[0]));
        auto str = push_string(node->children.nodes[1]->pv);
        auto meth = reg_obj();
        auto ret = reg_obj();

        assembler().findmeth(meth, obj, str);
        assembler().arg_o(0, obj);

        if (node->children.size == 3) {
          auto args = node->children.nodes[2];

          MVMCallsite* callsite = new MVMCallsite;
          memset(callsite, 0, sizeof(MVMCallsite));
          callsite->arg_count = 1+args->children.size;
          callsite->num_pos = 1+args->children.size;
          callsite->arg_flags = new MVMCallsiteEntry[1+args->children.size];
          callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
          int i=1;
          for (int j=0; j<args->children.size; j++) {
            PVIPNode* a= args->children.nodes[j];
            callsite->arg_flags[i] = MVM_CALLSITE_ARG_OBJ;
            assembler().arg_o(i, to_o(do_compile(a)));
            ++i;
          }
          auto callsite_no = push_callsite(callsite);
          assembler().prepargs(callsite_no);
        } else {
          MVMCallsite* callsite = new MVMCallsite;
          memset(callsite, 0, sizeof(MVMCallsite));
          callsite->arg_count = 1;
          callsite->num_pos = 1;
          callsite->arg_flags = new MVMCallsiteEntry[1];
          callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
          auto callsite_no = push_callsite(callsite);
          assembler().prepargs(callsite_no);
        }

        assembler().invoke_o(ret, meth);
        return ret;
      }
      case PVIP_NODE_CONDITIONAL: {
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

          auto cond_reg = do_compile(node->children.nodes[0]);
          unless_any(cond_reg, label_else);

          auto if_reg = do_compile(node->children.nodes[1]);
          assembler().set(dst_reg, to_o(if_reg));
          goto_(label_end);

        label_else.put();
          auto else_reg = do_compile(node->children.nodes[2]);
          assembler().set(dst_reg, to_o(else_reg));

        label_end.put();

        return dst_reg;
      }
      case PVIP_NODE_NOT: {
        auto src_reg = this->to_i(do_compile(node->children.nodes[0]));
        auto dst_reg = reg_int64();
        assembler().not_i(dst_reg, src_reg);
        return dst_reg;
      }
      case PVIP_NODE_BIN_AND: {
        return this->binary_binop(node, MVM_OP_band_i);
      }
      case PVIP_NODE_BIN_OR: {
        return this->binary_binop(node, MVM_OP_bor_i);
      }
      case PVIP_NODE_BIN_XOR: {
        return this->binary_binop(node, MVM_OP_bxor_i);
      }
      case PVIP_NODE_MUL: {
        return this->numeric_binop(node, MVM_OP_mul_i, MVM_OP_mul_n);
      }
      case PVIP_NODE_SUB: {
        return this->numeric_binop(node, MVM_OP_sub_i, MVM_OP_sub_n);
      }
      case PVIP_NODE_DIV: {
        return this->numeric_binop(node, MVM_OP_div_i, MVM_OP_div_n);
      }
      case PVIP_NODE_ADD: {
        return this->numeric_binop(node, MVM_OP_add_i, MVM_OP_add_n);
      }
      case PVIP_NODE_MOD: {
        return this->numeric_binop(node, MVM_OP_mod_i, MVM_OP_mod_n);
      }
      case PVIP_NODE_POW: {
        return this->numeric_binop(node, MVM_OP_pow_i, MVM_OP_pow_n);
      }
      case PVIP_NODE_INPLACE_ADD: {
        return this->numeric_inplace(node, MVM_OP_add_i, MVM_OP_add_n);
      }
      case PVIP_NODE_INPLACE_SUB: {
        return this->numeric_inplace(node, MVM_OP_sub_i, MVM_OP_sub_n);
      }
      case PVIP_NODE_INPLACE_MUL: {
        return this->numeric_inplace(node, MVM_OP_mul_i, MVM_OP_mul_n);
      }
      case PVIP_NODE_INPLACE_DIV: {
        return this->numeric_inplace(node, MVM_OP_div_i, MVM_OP_div_n);
      }
      case PVIP_NODE_INPLACE_POW: { // **=
        return this->numeric_inplace(node, MVM_OP_pow_i, MVM_OP_pow_n);
      }
      case PVIP_NODE_INPLACE_MOD: { // %=
        return this->numeric_inplace(node, MVM_OP_mod_i, MVM_OP_mod_n);
      }
      case PVIP_NODE_INPLACE_BIN_OR: { // +|=
        return this->binary_inplace(node, MVM_OP_bor_i);
      }
      case PVIP_NODE_INPLACE_BIN_AND: { // +&=
        return this->binary_inplace(node, MVM_OP_band_i);
      }
      case PVIP_NODE_INPLACE_BIN_XOR: { // +^=
        return this->binary_inplace(node, MVM_OP_bxor_i);
      }
      case PVIP_NODE_INPLACE_BLSHIFT: { // +<=
        return this->binary_inplace(node, MVM_OP_blshift_i);
      }
      case PVIP_NODE_INPLACE_BRSHIFT: { // +>=
        return this->binary_inplace(node, MVM_OP_brshift_i);
      }
      case PVIP_NODE_INPLACE_CONCAT_S: { // ~=
        return this->str_inplace(node, MVM_OP_concat_s, MVM_reg_str);
      }
      case PVIP_NODE_INPLACE_REPEAT_S: { // x=
        return this->str_inplace(node, MVM_OP_repeat_s, MVM_reg_int64);
      }
      case PVIP_NODE_UNARY_TILDE: { // ~
        return to_s(do_compile(node->children.nodes[0]));
      }
      case PVIP_NODE_NOP:
        return -1;
      case PVIP_NODE_ATKEY: {
        assert(node->children.size == 2);
        auto dst       = reg_obj();
        auto container = to_o(do_compile(node->children.nodes[0]));
        auto key       = to_s(do_compile(node->children.nodes[1]));
        assembler().atkey_o(dst, container, key);
        return dst;
      }
      case PVIP_NODE_HASH: {
        auto hashtype = reg_obj();
        auto hash     = reg_obj();
        assembler().hllhash(hashtype);
        assembler().create(hash, hashtype);
        for (int i=0; i<node->children.size; i++) {
          PVIPNode* pair = node->children.nodes[i];
          assert(pair->type == PVIP_NODE_PAIR);
          auto k = to_s(do_compile(pair->children.nodes[0]));
          auto v = to_o(do_compile(pair->children.nodes[1]));
          assembler().bindkey_o(hash, k, v);
        }
        return hash;
      }
      case PVIP_NODE_LOGICAL_XOR: { // '^^'
        //   calc_arg1
        //   calc_arg2
        //   if_o arg1, label_a1_true
        //   # arg1 is false.
        //   unless_o arg2, label_both_false
        //   # arg1=false, arg2=true
        //   set dst_reg, arg2
        //   goto label_end
        // label_both_false:
        //   null dst_reg
        //   goto label_end
        // label_a1_true:
        //   if_o arg2, label_both_true
        //   set dst_reg, arg1
        //   goto label_end
        // label_both_true:
        //   set dst_reg, arg1
        //   goto label_end
        // label_end:
        auto label_both_false = label_unsolved();
        auto label_a1_true    = label_unsolved();
        auto label_both_true  = label_unsolved();
        auto label_end        = label_unsolved();

        auto dst_reg = reg_obj();

          auto arg1 = to_o(do_compile(node->children.nodes[0]));
          auto arg2 = to_o(do_compile(node->children.nodes[1]));
          if_any(arg1, label_a1_true);
          unless_any(arg2, label_both_false);
          assembler().set(dst_reg, arg2);
          goto_(label_end);
        label_both_false.put();
          assembler().set(dst_reg, arg1);
          goto_(label_end);
        label_a1_true.put(); // a1:true, a2:unknown
          if_any(arg2, label_both_true);
          assembler().set(dst_reg, arg1);
          goto_(label_end);
        label_both_true.put();
          assembler().null(dst_reg);
          goto_(label_end);
        label_end.put();
        return dst_reg;
      }
      case PVIP_NODE_LOGICAL_OR: { // '||'
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
          auto arg1 = to_o(do_compile(node->children.nodes[0]));
          if_any(arg1, label_a1);
          auto arg2 = to_o(do_compile(node->children.nodes[1]));
          assembler().set(dst_reg, arg2);
          goto_(label_end);
        label_a1.put();
          assembler().set(dst_reg, arg1);
        label_end.put();
        return dst_reg;
      }
      case PVIP_NODE_LOGICAL_AND: { // '&&'
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
          auto arg1 = to_o(do_compile(node->children.nodes[0]));
          unless_any(arg1, label_a1);
          auto arg2 = to_o(do_compile(node->children.nodes[1]));
          assembler().set(dst_reg, arg2);
          goto_(label_end);
        label_a1.put();
          assembler().set(dst_reg, arg1);
        label_end.put();
        return dst_reg;
      }
      case PVIP_NODE_UNARY_PLUS: {
        return to_n(do_compile(node->children.nodes[0]));
      }
      case PVIP_NODE_UNARY_MINUS: {
        auto reg = do_compile(node->children.nodes[0]);
        if (get_local_type(reg) == MVM_reg_int64) {
          assembler().neg_i(reg, reg);
          return reg;
        } else {
          reg = to_n(reg);
          assembler().neg_n(reg, reg);
          return reg;
        }
      }
      case PVIP_NODE_CHAIN:
        if (node->children.size==1) {
          return do_compile(node->children.nodes[0]);
        } else {
          return this->compile_chained_comparisions(node);
        }
      case PVIP_NODE_FUNCALL: {
        assert(node->children.size == 2);
        const PVIPNode*ident = node->children.nodes[0];
        const PVIPNode*args  = node->children.nodes[1];
        assert(args->type == PVIP_NODE_ARGS);
        
        if (node->children.nodes[0]->type == PVIP_NODE_IDENT) {
          if (std::string(ident->pv->buf, ident->pv->len) == "say") {
            for (int i=0; i<args->children.size; i++) {
              PVIPNode* a = args->children.nodes[i];
              uint16_t reg_num = to_s(do_compile(a));
              if (i==args->children.size-1) {
                assembler().say(reg_num);
              } else {
                assembler().print(reg_num);
              }
            }
            return const_true();
          } else if (std::string(ident->pv->buf, ident->pv->len) == "print") {
            for (int i=0; i<args->children.size; i++) {
              PVIPNode* a = args->children.nodes[i];
              uint16_t reg_num = stringify(do_compile(a));
              assembler().print(reg_num);
            }
            return const_true();
          } else if (std::string(ident->pv->buf, ident->pv->len) == "open") {
            // TODO support arguments
            assert(args->children.size == 1);
            auto fname_s = do_compile(args->children.nodes[0]);
            auto dst_reg_o = reg_obj();
            auto flag_i = reg_int64();
            assembler().const_i64(flag_i, APR_FOPEN_READ); // TODO support other flags, etc.
            auto encoding_flag_i = reg_int64();
            assembler().const_i64(encoding_flag_i, MVM_encoding_type_utf8); // TODO support latin1, etc.
            assembler().open_fh(dst_reg_o, fname_s, flag_i, encoding_flag_i);
            return dst_reg_o;
          } else if (std::string(ident->pv->buf, ident->pv->len) == "slurp") {
            assert(args->children.size <= 2);
            assert(args->children.size != 2 && "Encoding option is not supported yet");
            auto fname_s = do_compile(args->children.nodes[0]);
            auto dst_reg_s = reg_str();
            auto encoding_flag_i = reg_int64();
            assembler().const_i64(encoding_flag_i, MVM_encoding_type_utf8); // TODO support latin1, etc.
            assembler().slurp(dst_reg_s, fname_s, encoding_flag_i);
            return dst_reg_s;
          }
        }

        {
          uint16_t func_reg_no;
          if (node->children.nodes[0]->type == PVIP_NODE_IDENT) {
            func_reg_no = reg_obj();
            int lex_no, outer;
            if (!find_lexical_by_name(std::string("&") + std::string(ident->pv->buf, ident->pv->len), &lex_no, &outer)) {
              MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", (std::string("&") + std::string(ident->pv->buf, ident->pv->len)).c_str());
            }
            assembler().getlex(
              func_reg_no,
              lex_no,
              outer // outer frame
            );
          } else {
            func_reg_no = to_o(do_compile(node->children.nodes[0]));
          }

          {
            MVMCallsite* callsite = new MVMCallsite;
            memset(callsite, 0, sizeof(MVMCallsite));
            // TODO support named params
            callsite->arg_count = args->children.size;
            callsite->num_pos   = args->children.size;
            callsite->arg_flags = new MVMCallsiteEntry[args->children.size];

            std::vector<uint16_t> arg_regs;

            for (int i=0; i<args->children.size; i++) {
              PVIPNode *a = args->children.nodes[i];
              auto reg = do_compile(a);
              if (reg<0) {
                MVM_panic(MVM_exitcode_compunit, "Compilation error. You should not pass void function as an argument: %s", PVIP_node_name(a->type));
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
            }

            auto callsite_no = push_callsite(callsite);
            assembler().prepargs(callsite_no);

            int i=0;
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
          auto dest_reg = reg_obj(); // ctx
          assembler().invoke_o(
              dest_reg,
              func_reg_no
          );
          return dest_reg;
        }
        break;
      }
      default:
        MVM_panic(MVM_exitcode_compunit, "Not implemented op: %s", PVIP_node_name(node->type));
        break;
      }
      printf("Should not reach here: %s\n", PVIP_node_name(node->type));
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
        int dst_num = reg_num64();
        int dst_int = reg_int64();
        // TODO: I need smrt_intify?
        assembler().smrt_numify(dst_num, reg_num);
        assembler().coerce_ni(dst_int, dst_num);
        return dst_int;
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
    int str_binop(const PVIPNode* node, uint16_t op) {
        assert(node->children.size == 2);

        int reg_num1 = to_s(do_compile(node->children.nodes[0]));
        int reg_num2 = to_s(do_compile(node->children.nodes[1]));
        int reg_num_dst = reg_int64();
        assembler().op_u16_u16_u16(MVM_OP_BANK_string, op, reg_num_dst, reg_num1, reg_num2);
        return reg_num_dst;
    }
    int binary_binop(const PVIPNode* node, uint16_t op_i) {
        assert(node->children.size == 2);

        int reg_num1 = to_i(do_compile(node->children.nodes[0]));
        int reg_num2 = to_i(do_compile(node->children.nodes[1]));
        auto dst_reg = reg_int64();
        assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_i, dst_reg, reg_num1, reg_num2);
        return dst_reg;
    }
    int numeric_inplace(const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = reg_num64();
        assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, tmp, to_n(lhs), to_n(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int binary_inplace(const PVIPNode* node, uint16_t op) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = reg_int64();
        assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op, tmp, to_i(lhs), to_i(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int str_inplace(const PVIPNode* node, uint16_t op, uint16_t rhs_type) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = reg_str();
        assembler().op_u16_u16_u16(MVM_OP_BANK_string, op, tmp, to_s(lhs), rhs_type == MVM_reg_int64 ? to_i(rhs) : to_s(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int numeric_binop(const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
        assert(node->children.size == 2);

        int reg_num1 = do_compile(node->children.nodes[0]);
        if (get_local_type(reg_num1) == MVM_reg_int64) {
          int reg_num_dst = reg_int64();
          int reg_num2 = this->to_i(do_compile(node->children.nodes[1]));
          assert(get_local_type(reg_num1) == MVM_reg_int64);
          assert(get_local_type(reg_num2) == MVM_reg_int64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_i, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_num64) {
          int reg_num_dst = reg_num64();
          int reg_num2 = this->to_n(do_compile(node->children.nodes[1]));
          assert(get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, reg_num1, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_obj) {
          // TODO should I use intify instead if the object is int?
          int reg_num_dst = reg_num64();

          int dst_num = reg_num64();
          assembler().op_u16_u16(MVM_OP_BANK_primitives, MVM_OP_smrt_numify, dst_num, reg_num1);

          int reg_num2 = this->to_n(do_compile(node->children.nodes[1]));
          assert(get_local_type(reg_num2) == MVM_reg_num64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);
          return reg_num_dst;
        } else if (get_local_type(reg_num1) == MVM_reg_str) {
          int dst_num = reg_num64();
          assembler().coerce_sn(dst_num, reg_num1);

          int reg_num_dst = reg_num64();
          int reg_num2 = this->to_n(do_compile(node->children.nodes[1]));
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
    void compile_statements(const PVIPNode*node, int dst_reg) {
      int reg = UNKNOWN_REG;
      if (node->type == PVIP_NODE_STATEMENTS || node->type == PVIP_NODE_ELSE) {
        for (int i=0, l=node->children.size; i<l; i++) {
          reg = do_compile(node->children.nodes[i]);
        }
      } else {
        reg = do_compile(node);
      }
      if (reg == UNKNOWN_REG) {
        assembler().null(dst_reg);
      } else {
        assembler().set(dst_reg, to_o(reg));
      }
    }
    // Compile chained comparisions like `1 < $n < 3`.
    // TODO: optimize simple case like `1 < $n`
    uint16_t compile_chained_comparisions(const PVIPNode* node) {
      auto lhs = do_compile(node->children.nodes[0]);
      auto dst_reg = reg_int64();
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
      assembler().const_i64(dst_reg, 1);
      goto_(label_end);
    label_false.put();
      assembler().const_i64(dst_reg, 0);
      // goto_(label_end());
    label_end.put();
      return dst_reg;
    }
    int num_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op_i, uint16_t op_n) {
        int reg_num_dst = reg_int64();
        if (get_local_type(lhs) == MVM_reg_int64) {
          assert(get_local_type(lhs) == MVM_reg_int64);
          // assert(get_local_type(rhs) == MVM_reg_int64);
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_i, reg_num_dst, lhs, to_i(rhs));
          return reg_num_dst;
        } else if (get_local_type(lhs) == MVM_reg_num64) {
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, lhs, to_n(rhs));
          return reg_num_dst;
        } else if (get_local_type(lhs) == MVM_reg_obj) {
          // TODO should I use intify instead if the object is int?
          assembler().op_u16_u16_u16(MVM_OP_BANK_primitives, op_n, reg_num_dst, to_n(lhs), to_n(rhs));
          return reg_num_dst;
        } else {
          // NOT IMPLEMENTED
          abort();
        }
    }
    int str_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op) {
        int reg_num_dst = reg_int64();
        assembler().op_u16_u16_u16(MVM_OP_BANK_string, op, reg_num_dst, to_s(lhs), to_s(rhs));
        return reg_num_dst;
    }
    uint16_t do_compare(PVIP_node_type_t type, uint16_t lhs, uint16_t rhs) {
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
  public:
    Compiler(CompUnit & cu): cu_(cu), frame_no_(0) {
      cu_.initialize();
      current_class_how_ = NULL;
    }
    void compile(PVIPNode*node) {
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

      // bootstrap $?PACKAGE
      // TODO I should wrap it to any object. And set WHO.
      // $?PACKAGE.WHO<bar> should work.
      {
        auto lex = push_lexical("$?PACKAGE", MVM_reg_obj);
        auto package = reg_obj();
        auto hash_type = reg_obj();
        assembler().hllhash(hash_type);
        assembler().create(package, hash_type);
        assembler().bindlex(lex, 0, package);
      }

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

