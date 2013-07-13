namespace kiji {

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

};
