#pragma once
/* vim:ts=2:sw=2:tw=0:
 */

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
#include "frame.h"
#include "handy.h"
#include "asm.h"

#define ASM_OP_U16_U16_U16(a,b,c,d,e) \
  Kiji_asm_op_u16_u16_u16(&(*(frames_.back())), a,b,c,d,e)

#define ASM_OP_U16_U16(a,b,c,d) \
  Kiji_asm_op_u16_u16(&(*(frames_.back())), a,b,c,d)

#define ASM_WRITE_UINT16_T(a,b) \
  Kiji_asm_write_uint16_t_for(&(*(frames_.back())), a,b)

#define ASM_OP_U16_U32(a,b, c, d) \
  Kiji_asm_op_u16_u32(&(*(frames_.back())), a,b, c,d)

#define MVM_ASSIGN_REF2(tc, update_root, update_addr, referenced) \
    { \
        void *_r = referenced; \
        MVM_WB(tc, update_root, _r); \
        update_addr = (MVMObject*)_r; \
    }

#define PVIPSTRING2STDSTRING(pv) std::string((pv)->buf, (pv)->len)
#define CU cu_

#define MEMORY_ERROR() \
          MVM_panic(MVM_exitcode_compunit, "Compilation error. return with non-value.");

#define REG_OBJ() Kiji_compiler_push_local_type(this, MVM_reg_obj)
#define REG_STR() Kiji_compiler_push_local_type(this, MVM_reg_str)
#define REG_INT64() Kiji_compiler_push_local_type(this, MVM_reg_int64)
#define REG_NUM64() Kiji_compiler_push_local_type(this, MVM_reg_num64)


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


KIJI_STATIC_INLINE void dump_object(MVMThreadContext*tc, MVMObject* obj) {
  if (obj==NULL) {
    printf("(null)\n");
    return;
  }
  MVM_string_say(tc, REPR(obj)->name);
}

struct KijiCompiler;

    class KijiLabel {
    private:
      KijiCompiler *compiler_;
      ssize_t address_;
      std::vector<ssize_t> reserved_addresses_;
    public:
      KijiLabel(KijiCompiler *compiler) :compiler_(compiler), address_(-1) { }
      KijiLabel(KijiCompiler *compiler, ssize_t address) :compiler_(compiler), address_(address) { }
      ~KijiLabel() {
        assert(address_ != -1 && "Unsolved label");
      }
      ssize_t address() const {
        return address_;
      }
      void put();
      void reserve(ssize_t address) {
        reserved_addresses_.push_back(address);
      }
      bool is_solved() const { return address_!=-1; }
    };

    class KijiLoopGuard {
    private:
      KijiCompiler *compiler_;
      MVMuint32 start_offset_;
      MVMuint32 last_offset_;
      MVMuint32 next_offset_;
      MVMuint32 redo_offset_;
    public:
      KijiLoopGuard(KijiCompiler *compiler);
      ~KijiLoopGuard();
      // fixme: `put` is not the best verb in English here.
      void put_last();
      void put_redo();
      void put_next();
    };

  KIJI_STATIC_INLINE uint16_t Kiji_compiler_get_local_type(KijiCompiler* self, int n);
uint16_t Kiji_compiler_if_op(KijiCompiler* self, uint16_t cond_reg);
    KIJI_STATIC_INLINE int Kiji_compiler_push_local_type(KijiCompiler* self, MVMuint16 reg_type);


  /**
   * OP map is 3rd/MoarVM/src/core/oplist
   * interp code is 3rd/MoarVM/src/core/interp.c
   */
  enum { UNKNOWN_REG = -1 };
  class KijiCompiler {
  public:
    std::vector<KijiFrame*> frames_;
  public:
    MVMCompUnit* cu_;
    MVMThreadContext *tc_;
    int frame_no_;
    MVMObject* current_class_how_;

    MVMSerializationContext * sc_classes_;
    int num_sc_classes_;

    void push_sc_object(MVMObject * object, int *wval1, int *wval2) {
      num_sc_classes_++;

      *wval1 = 1;
      *wval2 = num_sc_classes_-1;

      MVM_sc_set_object(tc_, sc_classes_, num_sc_classes_-1, object);
    }

    KijiLabel label() { return KijiLabel(this, frames_.back()->frame.bytecode_size); }
    KijiLabel label_unsolved() { return KijiLabel(this); }

#define ASM_BYTECODE_SIZE() \
    frames_.back()->frame.bytecode_size

    void goto_(KijiLabel &label) {
      if (!label.is_solved()) {
        label.reserve(ASM_BYTECODE_SIZE() + 2);
      }
      ASM_GOTO(label.address());
    }
    void return_any(uint16_t reg) {
      switch (Kiji_compiler_get_local_type(this, reg)) {
      case MVM_reg_int64:
        ASM_RETURN_I(reg);
        break;
      case MVM_reg_str:
        ASM_RETURN_S(reg);
        break;
      case MVM_reg_obj:
        ASM_RETURN_O(reg);
        break;
      case MVM_reg_num64:
        ASM_RETURN_N(reg);
        break;
      default: abort();
      }
    }
    void if_any(uint16_t reg, KijiLabel &label) {
      if (!label.is_solved()) {
        label.reserve(ASM_BYTECODE_SIZE() + 2 + 2);
      }
      ASM_OP_U16_U32(MVM_OP_BANK_primitives, Kiji_compiler_if_op(this, reg), reg, label.address());
    }
    void unless_any(uint16_t reg, KijiLabel &label) {
      if (!label.is_solved()) {
        label.reserve(ASM_BYTECODE_SIZE() + 2 + 2);
      }
      ASM_OP_U16_U32(MVM_OP_BANK_primitives, unless_op(reg), reg, label.address());
    }

    int compile_class(const PVIPNode* node) {
      int wval1, wval2;
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

        this->push_sc_object(type, &wval1, &wval2);
        current_class_how_ = how;
      }

      // compile body
      for (int i=0; i<node->children.nodes[2]->children.size; i++) {
        PVIPNode *n = node->children.nodes[2]->children.nodes[i];
        (void)do_compile(n);
      }

      current_class_how_ = NULL;

      auto retval = REG_OBJ();
      ASM_WVAL(retval, wval1, wval2);

      // Bind class object to lexical variable
      auto name_node = node->children.nodes[0];
      if (PVIP_node_category(name_node->type) == PVIP_CATEGORY_STRING) {
        auto lex = push_lexical(PVIPSTRING2STDSTRING(name_node->pv), MVM_reg_obj);
        ASM_BINDLEX(
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
      Kiji_variable_type_t vartype = find_variable_by_name(name, lex_no, outer);
      if (vartype==KIJI_VARIABLE_TYPE_MY) {
        auto reg_no = REG_OBJ();
        ASM_GETLEX(
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
        auto reg = REG_OBJ();
        auto varname = push_string(name);
        auto varname_s = REG_STR();
        ASM_GETLEX(
          reg,
          lex_no,
          outer // outer frame
        );
        ASM_CONST_S(
          varname_s,
          varname
        );
        // TODO getwho
        ASM_ATKEY_O(
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
      Kiji_variable_type_t vartype = find_variable_by_name(name, lex_no, outer);
      if (vartype==KIJI_VARIABLE_TYPE_MY) {
        ASM_BINDLEX(
          lex_no,
          outer,
          val_reg
        );
      } else {
        int lex_no;
        if (!find_lexical_by_name("$?PACKAGE", &lex_no, &outer)) {
          MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", "$?PACKAGE");
        }
        auto reg = REG_OBJ();
        auto varname = push_string(name);
        auto varname_s = REG_STR();
        ASM_GETLEX(
          reg,
          lex_no,
          outer // outer frame
        );
        ASM_CONST_S(
          varname_s,
          varname
        );
        // TODO getwho
        ASM_BINDKEY_O(
          reg,
          varname_s,
          val_reg
        );
      }
    }

    // This reg returns register number contains true value.
    int const_true() {
      auto reg = REG_INT64();
      ASM_CONST_I64(reg, 1);
      return reg;
    }

    int push_string(PVIPString* pv) {
      return push_string(pv->buf, pv->len);
    }
    int push_string(const std::string & str) {
      return push_string(str.c_str(), str.size());
    }
    int push_string(const char*string, int length) {
      MVMString* str = MVM_string_utf8_decode(tc_, tc_->instance->VMString, string, length);
      CU->num_strings++;
      CU->strings = (MVMString**)realloc(CU->strings, sizeof(MVMString*)*CU->num_strings);
      if (!CU->strings) {
        MVM_panic(MVM_exitcode_compunit, "Cannot allocate memory");
      }
      CU->strings[CU->num_strings-1] = str;
      return CU->num_strings-1;
    }
    Kiji_variable_type_t find_variable_by_name(const std::string &name_cc, int &lex_no, int &outer) {
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      return Kiji_find_variable_by_name(frames_.back(), tc_, name, &lex_no, &outer);
    }
    // lexical variable number by name
    bool find_lexical_by_name(const std::string &name_cc, int *lex_no, int *outer) {
      MVMString* name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      return Kiji_frame_find_lexical_by_name(&(*(frames_.back())), tc_, name, lex_no, outer) == KIJI_TRUE;
    }
    // Push lexical variable.
    int push_lexical(PVIPString *pv, MVMuint16 type) {
      MVMString * name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, pv->buf, pv->len);
      return Kiji_frame_push_lexical(frames_.back(), tc_, name, type);
    }
    int push_lexical(const std::string name_cc, MVMuint16 type) {
      MVMString * name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      return Kiji_frame_push_lexical(frames_.back(), tc_, name, type);
    }
    void push_pkg_var(const std::string name_cc) {
      MVMString * name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name_cc.c_str(), name_cc.size());
      Kiji_frame_push_pkg_var(frames_.back(), name);
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
      for (i=0; i<CU->num_callsites; i++) {
        if (callsite_eq(CU->callsites[i], callsite)) {
          delete callsite; // free memory
          return i;
        }
      }
      CU->num_callsites++;
      CU->callsites = (MVMCallsite**)realloc(CU->callsites, sizeof(MVMCallsite*)*CU->num_callsites);
      if (!CU->callsites) {
        MEMORY_ERROR();
      }
      CU->callsites[CU->num_callsites-1] = callsite;
      return CU->num_callsites-1;
    }
    void compile_array(uint16_t array_reg, const PVIPNode* node);
    int do_compile(const PVIPNode*node);
  private:
    // objectify the register.
    int to_o(int reg_num);
    int to_n(int reg_num);
    int to_i(int reg_num);
    int to_s(int reg_num);
    int str_binop(const PVIPNode* node, uint16_t op);
    int binary_binop(const PVIPNode* node, uint16_t op_i);
    int numeric_inplace(const PVIPNode* node, uint16_t op_i, uint16_t op_n);
    int binary_inplace(const PVIPNode* node, uint16_t op);
    int str_inplace(const PVIPNode* node, uint16_t op, uint16_t rhs_type);
    int numeric_binop(const PVIPNode* node, uint16_t op_i, uint16_t op_n);
    uint16_t unless_op(uint16_t cond_reg);
    void compile_statements(const PVIPNode*node, int dst_reg);
    uint16_t compile_chained_comparisions(const PVIPNode* node);
    int num_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op_i, uint16_t op_n);
    int str_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op);
    uint16_t do_compare(PVIP_node_type_t type, uint16_t lhs, uint16_t rhs);
  public:
    KijiCompiler(MVMCompUnit * cu, MVMThreadContext * tc);
    ~KijiCompiler() { }
    void initialize();
    void finalize(MVMInstance* vm);
    void compile(PVIPNode*node, MVMInstance* vm);
  };

  KIJI_STATIC_INLINE KijiFrame* Kiji_compiler_top_frame(KijiCompiler *self) {
    return self->frames_.back();
  }

    // reserve register
    KIJI_STATIC_INLINE int Kiji_compiler_push_local_type(KijiCompiler* self, MVMuint16 reg_type) {
      return Kiji_frame_push_local_type(Kiji_compiler_top_frame(self), reg_type);
    }

  // Get register type at 'n'
  KIJI_STATIC_INLINE uint16_t Kiji_compiler_get_local_type(KijiCompiler* self, int n) {
    return Kiji_frame_get_local_type(Kiji_compiler_top_frame(self), n);
  }

