// vim:ts=2:sw=2:tw=0:

enum Kiji_variable_type_t {
  VARIABLE_TYPE_MY,
  VARIABLE_TYPE_OUR
};

typedef struct _KijiFrame {
  private:
  MVMStaticFrame frame_; // frame itself
  struct _KijiFrame* outer_;
  MVMThreadContext *tc_;

  std::vector<MVMuint16> local_types_;
  std::vector<MVMuint16> lexical_types_;
  std::vector<MVMString*> package_variables_;

  std::vector<MVMFrameHandler*> handlers_;

  kiji::Assembler assembler_;

  void set_cuuid() {
      static int cuuid_counter = 0;
      std::ostringstream oss;
      oss << "frame_cuuid_" << cuuid_counter++;
      std::string cuuid = oss.str();
      frame_.cuuid = MVM_string_utf8_decode(tc_, tc_->instance->VMString, cuuid.c_str(), cuuid.size());
  }

  public:
  MVMStaticFrame* frame() { return &frame_; }
  _KijiFrame(MVMThreadContext* tc, const std::string name) {
      memset(&frame_, 0, sizeof(MVMFrame));
      tc_ = tc;
      frame_.name = MVM_string_utf8_decode(tc, tc->instance->VMString, name.c_str(), name.size());
  }
  ~_KijiFrame(){ }

  kiji::Assembler & assembler() {
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
  void push_pkg_var(MVMString *name) {
      package_variables_.push_back(name);
  }

  void set_outer(struct _KijiFrame*frame) {
      frame_.outer = &(frame->frame_);
      outer_ = &(*frame);
  }

  Kiji_variable_type_t find_variable_by_name(MVMString * name, int &lex_no, int &outer) {
      struct _KijiFrame* f = this;
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
          if (MVM_string_equal(tc_, n, name)) {
            return VARIABLE_TYPE_OUR;
          }
      }

      f = &(*(f->outer_));
      ++outer;
      }
      // TODO I should use MVM_panic instead.
      printf("Unknown lexical variable in find_variable_by_name: ");
      MVM_string_say(tc_, name);
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
} KijiFrame;

