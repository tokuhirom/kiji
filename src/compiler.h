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
    void push_handler(MVMFrameHandler *handler) {
      return Kiji_frame_push_handler(frames_.back(), handler);
    }
    MVMStaticFrame* get_frame(int frame_no) {
      return cu_->frames[frame_no];
    }
    int push_frame(const std::string & name) {
      assert(tc_);
      char *buf = (char*)malloc((name.size()+32)*sizeof(char));
      int len = snprintf(buf, name.size()+31, "%s%d", name.c_str(), frame_no_++);
      // TODO Newxz
      KijiFrame* frame = (KijiFrame*)malloc(sizeof(KijiFrame));
      memset(frame, 0, sizeof(KijiFrame));
      frame->frame.name = MVM_string_utf8_decode(tc_, tc_->instance->VMString, buf, len);
      free(buf);
      if (frames_.size() != 0) {
        Kiji_frame_set_outer(frame, frames_.back());
      }
      frames_.push_back(frame);
      cu_->num_frames++;
      Renew(cu_->frames, cu_->num_frames, MVMStaticFrame*);
      cu_->frames[cu_->num_frames-1] = &(frames_.back()->frame);
      cu_->frames[cu_->num_frames-1]->cu = cu_;
      cu_->frames[cu_->num_frames-1]->work_size = 0;
      return cu_->num_frames-1;
    }
    void pop_frame() {
      frames_.pop_back();
    }
    size_t frame_size() const {
      return frames_.size();
    }


    void compile_array(uint16_t array_reg, const PVIPNode* node) {
      if (node->type==PVIP_NODE_LIST) {
        for (int i=0; i<node->children.size; i++) {
          PVIPNode* m = node->children.nodes[i];
          compile_array(array_reg, m);
        }
      } else {
        auto reg = this->box(do_compile(node));
        ASM_PUSH_O(array_reg, reg);
      }
    }

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
        ASM_DEC_I(i_tmp);
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
        ASM_INC_I(i_tmp);
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
        ASM_INC_I(i_tmp);
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
        ASM_DEC_I(i_tmp);
        auto dst_reg = to_o(i_tmp);
        set_variable(PVIPSTRING2STDSTRING(node->children.nodes[0]->pv), dst_reg);
        return dst_reg;
      }
      case PVIP_NODE_UNARY_BITWISE_NEGATION: { // +^1
        auto reg = to_i(do_compile(node->children.nodes[0]));
        ASM_BNOT_I(reg, reg);
        return reg;
      }
      case PVIP_NODE_BRSHIFT: { // +>
        auto l = to_i(do_compile(node->children.nodes[0]));
        auto r = to_i(do_compile(node->children.nodes[1]));
        ASM_BRSHIFT_I(r, l, r);
        return r;
      }
      case PVIP_NODE_BLSHIFT: { // +<
        auto l = to_i(do_compile(node->children.nodes[0]));
        auto r = to_i(do_compile(node->children.nodes[1]));
        ASM_BLSHIFT_I(r, l, r);
        return r;
      }
      case PVIP_NODE_ABS: {
        // TODO support abs_n?
        auto r = to_i(do_compile(node->children.nodes[0]));
        ASM_ABS_I(r, r);
        return r;
      }
      case PVIP_NODE_LAST: {
        // break from for, while, loop.
        auto ret = REG_OBJ();
        ASM_THROWCATLEX(
          ret,
          MVM_EX_CAT_LAST
        );
        return ret;
      }
      case PVIP_NODE_REDO: {
        // redo from for, while, loop.
        auto ret = REG_OBJ();
        ASM_THROWCATLEX(
          ret,
          MVM_EX_CAT_REDO
        );
        return ret;
      }
      case PVIP_NODE_NEXT: {
        // continue from for, while, loop.
        auto ret = REG_OBJ();
        ASM_THROWCATLEX(
          ret,
          MVM_EX_CAT_NEXT
        );
        return ret;
      }
      case PVIP_NODE_RETURN: {
        assert(node->children.size ==1);
        auto reg = do_compile(node->children.nodes[0]);
        if (reg < 0) {
          MEMORY_ERROR();
        }
        // ASM_RETURN_O(this->box(reg));
        switch (Kiji_compiler_get_local_type(this, reg)) {
        case MVM_reg_int64:
          ASM_RETURN_I(reg);
          break;
        case MVM_reg_str:
          ASM_RETURN_S(reg);
          break;
        case MVM_reg_obj:
          ASM_RETURN_O(this->box(reg));
          break;
        case MVM_reg_num64:
          ASM_RETURN_N(reg);
          break;
        default:
          MVM_panic(MVM_exitcode_compunit, "Compilation error. Unknown register for returning: %d", Kiji_compiler_get_local_type(this, reg));
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
        PVIPString *error;
        PVIPNode* root_node = PVIP_parse_fp(fp, 0, &error);
        if (!root_node) {
          MVM_panic(MVM_exitcode_compunit, "Cannot parse: %s", error->buf);
        }

        auto frame_no = push_frame(path);
        ASM_CHECKARITY(0,0);
        this->do_compile(root_node);
        ASM_RETURN();
        pop_frame();

        auto code_reg = REG_OBJ();
        ASM_GETCODE(code_reg, frame_no);
        MVMCallsite* callsite = new MVMCallsite;
        memset(callsite, 0, sizeof(MVMCallsite));
        callsite->arg_count = 0;
        callsite->num_pos = 0;
        callsite->arg_flags = NULL;
        auto callsite_no = push_callsite(callsite);
        ASM_PREPARGS(callsite_no);
        ASM_INVOKE_V(code_reg);

        return UNKNOWN_REG;
      }
      case PVIP_NODE_DIE: {
        int msg_reg = to_s(do_compile(node->children.nodes[0]));
        int dst_reg = REG_OBJ();
        assert(msg_reg != UNKNOWN_REG);
        ASM_DIE(dst_reg, msg_reg);
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

        KijiLoopGuard loop(this);

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
        ASM_CHECKARITY(
          node->children.nodes[0]->children.size,
          node->children.nodes[0]->children.size
        );
        for (int i=0; i<node->children.nodes[0]->children.size; i++) {
          PVIPNode *n = node->children.nodes[0]->children.nodes[i];
          int reg = REG_OBJ();
          int lex = push_lexical(PVIPSTRING2STDSTRING(n->pv), MVM_reg_obj);
          ASM_PARAM_RP_O(reg, i);
          ASM_BINDLEX(lex, 0, reg);
        }
        auto retval = do_compile(node->children.nodes[1]);
        if (retval == UNKNOWN_REG) {
          retval = REG_OBJ();
          ASM_NULL(retval);
        }
        return_any(retval);
        pop_frame();

        // warn if void context.
        auto dst_reg = REG_OBJ();
        ASM_GETCODE(dst_reg, frame_no);

        return dst_reg;
      }
      case PVIP_NODE_BLOCK: {
        auto frame_no = push_frame("block");
        ASM_CHECKARITY(0,0);
        for (int i=0; i<node->children.size; i++) {
          PVIPNode *n = node->children.nodes[i];
          (void)do_compile(n);
        }
        ASM_RETURN();
        pop_frame();

        auto frame_reg = REG_OBJ();
        ASM_GETCODE(frame_reg, frame_no);

        MVMCallsite* callsite = new MVMCallsite;
        memset(callsite, 0, sizeof(MVMCallsite));
        callsite->arg_count = 0;
        callsite->num_pos = 0;
        callsite->arg_flags = NULL;
        auto callsite_no = push_callsite(callsite);
        ASM_PREPARGS(callsite_no);

        ASM_INVOKE_V(frame_reg); // trash result

        return UNKNOWN_REG;
      }
      case PVIP_NODE_STRING: {
        int str_num = push_string(node->pv);
        int reg_num = REG_STR();
        ASM_CONST_S(reg_num, str_num);
        return reg_num;
      }
      case PVIP_NODE_INT: {
        uint16_t reg_num = REG_INT64();
        int64_t n = node->iv;
        ASM_CONST_I64(reg_num, n);
        return reg_num;
      }
      case PVIP_NODE_NUMBER: {
        uint16_t reg_num = REG_NUM64();
        MVMnum64 n = node->nv;
        ASM_CONST_N64(reg_num, n);
        return reg_num;
      }
      case PVIP_NODE_BIND: {
        auto lhs = node->children.nodes[0];
        auto rhs = node->children.nodes[1];
        if (lhs->type == PVIP_NODE_MY) {
          // my $var := foo;
          int lex_no = do_compile(lhs);
          int val    = this->box(do_compile(rhs));
          ASM_BINDLEX(
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
          auto package = REG_OBJ();
          ASM_GETLEX(
            package,
            lex_no,
            outer // outer frame
          );
          // TODO getwho
          auto varname_s = REG_STR();
          ASM_CONST_S(varname_s, varname);
          // 0x0F    bindkey_o           r(obj) r(str) r(obj)
          ASM_BINDKEY_O(
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

        auto funcreg = REG_OBJ();
        auto funclex = push_lexical(std::string("&") + name, MVM_reg_obj);
        auto func_pos = ASM_BYTECODE_SIZE() + 2 + 2;
        ASM_GETCODE(funcreg, 0);
        ASM_BINDLEX(
            funclex,
            0, // frame outer count
            funcreg
        );

        // Compile function body
        auto frame_no = push_frame(name);

        // TODO process named params
        // TODO process types
        {
          ASM_CHECKARITY(
              node->children.nodes[1]->children.size+1,
              node->children.nodes[1]->children.size+1
          );

          {
            // push self
            int lex = push_lexical("__self", MVM_reg_obj);
            int reg = REG_OBJ();
            ASM_PARAM_RP_O(reg, 0);
            ASM_BINDLEX(lex, 0, reg);
          }

          for (int i=1; i<node->children.nodes[1]->children.size; ++i) {
            auto n = node->children.nodes[1]->children.nodes[i];
            int reg = REG_OBJ();
            int lex = push_lexical(n->pv, MVM_reg_obj);
            ASM_PARAM_RP_O(reg, i);
            ASM_BINDLEX(lex, 0, reg);
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
              returned = true;
            }
          }

          // return null
          if (!returned) {
            int reg = REG_OBJ();
            ASM_NULL(reg);
            ASM_RETURN_O(reg);
          }
        }
        pop_frame();

        ASM_WRITE_UINT16_T(frame_no, func_pos);

        // bind method object to class how
        if (!current_class_how_) {
          MVM_panic(MVM_exitcode_compunit, "Compilation error. You can't write methods outside of class definition");
        }
        {
          MVMString * methname = MVM_string_utf8_decode(tc_, tc_->instance->VMString, name.c_str(), name.size());
          // self, type_obj, name, method
          MVMObject * method_table = ((MVMKnowHOWREPR *)current_class_how_)->body.methods;
          MVMObject* code_type = tc_->instance->boot_types->BOOTCode;
          MVMCode *coderef = (MVMCode*)REPR(code_type)->allocate(tc_, STABLE(code_type));
          coderef->body.sf = get_frame(frame_no);
          REPR(method_table)->ass_funcs->bind_key_boxed(tc_, STABLE(method_table),
              method_table, OBJECT_BODY(method_table), (MVMObject *)methname, (MVMObject*)coderef);
        }

        return funcreg;
      }
      case PVIP_NODE_FUNC: {
        PVIPNode * name_node = node->children.nodes[0];
        std::string name(name_node->pv->buf, name_node->pv->len);

        auto funcreg = REG_OBJ();
        auto funclex = push_lexical(std::string("&") + name, MVM_reg_obj);
        auto func_pos = ASM_BYTECODE_SIZE() + 2 + 2;
        ASM_GETCODE(funcreg, 0);
        ASM_BINDLEX(
            funclex,
            0, // frame outer count
            funcreg
        );

        // Compile function body
        auto frame_no = push_frame(name);

        // TODO process named params
        // TODO process types
        {
          ASM_CHECKARITY(
              node->children.nodes[1]->children.size,
              node->children.nodes[1]->children.size
          );

          for (int i=0; i<node->children.nodes[1]->children.size; ++i) {
            auto n = node->children.nodes[1]->children.nodes[i];
            int reg = REG_OBJ();
            int lex = push_lexical(n->pv, MVM_reg_obj);
            ASM_PARAM_RP_O(reg, i);
            ASM_BINDLEX(lex, 0, reg);
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
              returned = true;
            }
          }

          // return null
          if (!returned) {
            int reg = REG_OBJ();
            ASM_NULL(reg);
            ASM_RETURN_O(reg);
          }
        }
        pop_frame();

        ASM_WRITE_UINT16_T(frame_no, func_pos);

        return funcreg;
      }
      case PVIP_NODE_VARIABLE: {
        // copy lexical variable to register
        return get_variable(std::string(node->pv->buf, node->pv->len));
      }
      case PVIP_NODE_CLARGS: { // @*ARGS
        auto retval = REG_OBJ();
        ASM_WVAL(retval, 0,0);
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

        KijiLoopGuard loop(this);
          auto src_reg = box(do_compile(node->children.nodes[0]));
          auto iter_reg = REG_OBJ();
          auto label_end = label_unsolved();
          ASM_ITER(iter_reg, src_reg);
          unless_any(iter_reg, label_end);

        auto label_for = label();
        loop.put_next();

          auto val = REG_OBJ();
          ASM_SHIFT_O(val, iter_reg);

          if (node->children.nodes[1]->type == PVIP_NODE_LAMBDA) {
            auto body = do_compile(node->children.nodes[1]);
            MVMCallsite* callsite = new MVMCallsite;
            memset(callsite, 0, sizeof(MVMCallsite));
            callsite->arg_count = 1;
            callsite->num_pos = 1;
            callsite->arg_flags = new MVMCallsiteEntry[1];
            callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
            auto callsite_no = push_callsite(callsite);
            ASM_PREPARGS(callsite_no);
            ASM_ARG_O(0, val);
            ASM_INVOKE_V(body);
          } else {
            int it = push_lexical("$_", MVM_reg_obj);
            ASM_BINDLEX(it, 0, val);
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
        auto dst_reg = REG_OBJ();

        auto label_if = label_unsolved();
        if_any(if_cond_reg, label_if);

        // put else if conditions
        std::list<KijiLabel> elsif_poses;
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
          auto reg_no = REG_OBJ();
          ASM_GETLEX(
            reg_no,
            lex_no,
            outer // outer frame
          );
          return reg_no;
        // sub fooo { }; foooo;
        } else if (find_lexical_by_name(std::string("&") + std::string(node->pv->buf, node->pv->len), &lex_no, &outer)) {
          auto func_reg_no = REG_OBJ();
          ASM_GETLEX(
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
          ASM_PREPARGS(callsite_no);

          auto dest_reg = REG_OBJ(); // ctx
          ASM_INVOKE_O(
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
        auto dst_reg = REG_STR();
        auto lhs = node->children.nodes[0];
        auto rhs = node->children.nodes[1];
        auto l = stringify(do_compile(lhs));
        auto r = stringify(do_compile(rhs));
        ASM_CONCAT_S(
          dst_reg,
          l,
          r
        );
        return dst_reg;
      }
      case PVIP_NODE_REPEAT_S: { // x operator
        auto dst_reg = REG_STR();
        auto lhs = node->children.nodes[0];
        auto rhs = node->children.nodes[1];
        ASM_REPEAT_S(
          dst_reg,
          to_s(do_compile(lhs)),
          to_i(do_compile(rhs))
        );
        return dst_reg;
      }
      case PVIP_NODE_LIST:
      case PVIP_NODE_ARRAY: { // TODO: use 6model's container feature after released it.
        // create array
        auto array_reg = REG_OBJ();
        ASM_HLLLIST(array_reg);
        ASM_CREATE(array_reg, array_reg);

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
        auto dst = REG_OBJ();
        ASM_ATPOS_O(dst, container, idx);
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
        auto meth = REG_OBJ();
        auto ret = REG_OBJ();

        ASM_FINDMETH(meth, obj, str);
        ASM_ARG_O(0, obj);

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
            ASM_ARG_O(i, to_o(do_compile(a)));
            ++i;
          }
          auto callsite_no = push_callsite(callsite);
          ASM_PREPARGS(callsite_no);
        } else {
          MVMCallsite* callsite = new MVMCallsite;
          memset(callsite, 0, sizeof(MVMCallsite));
          callsite->arg_count = 1;
          callsite->num_pos = 1;
          callsite->arg_flags = new MVMCallsiteEntry[1];
          callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
          auto callsite_no = push_callsite(callsite);
          ASM_PREPARGS(callsite_no);
        }

        ASM_INVOKE_O(ret, meth);
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
        auto dst_reg = REG_OBJ();

          auto cond_reg = do_compile(node->children.nodes[0]);
          unless_any(cond_reg, label_else);

          auto if_reg = do_compile(node->children.nodes[1]);
          ASM_SET(dst_reg, to_o(if_reg));
          goto_(label_end);

        label_else.put();
          auto else_reg = do_compile(node->children.nodes[2]);
          ASM_SET(dst_reg, to_o(else_reg));

        label_end.put();

        return dst_reg;
      }
      case PVIP_NODE_NOT: {
        auto src_reg = this->to_i(do_compile(node->children.nodes[0]));
        auto dst_reg = REG_INT64();
        ASM_NOT_I(dst_reg, src_reg);
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
        auto dst       = REG_OBJ();
        auto container = to_o(do_compile(node->children.nodes[0]));
        auto key       = to_s(do_compile(node->children.nodes[1]));
        ASM_ATKEY_O(dst, container, key);
        return dst;
      }
      case PVIP_NODE_HASH: {
        auto hashtype = REG_OBJ();
        auto hash     = REG_OBJ();
        ASM_HLLHASH(hashtype);
        ASM_CREATE(hash, hashtype);
        for (int i=0; i<node->children.size; i++) {
          PVIPNode* pair = node->children.nodes[i];
          assert(pair->type == PVIP_NODE_PAIR);
          auto k = to_s(do_compile(pair->children.nodes[0]));
          auto v = to_o(do_compile(pair->children.nodes[1]));
          ASM_BINDKEY_O(hash, k, v);
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

        auto dst_reg = REG_OBJ();

          auto arg1 = to_o(do_compile(node->children.nodes[0]));
          auto arg2 = to_o(do_compile(node->children.nodes[1]));
          if_any(arg1, label_a1_true);
          unless_any(arg2, label_both_false);
          ASM_SET(dst_reg, arg2);
          goto_(label_end);
        label_both_false.put();
          ASM_SET(dst_reg, arg1);
          goto_(label_end);
        label_a1_true.put(); // a1:true, a2:unknown
          if_any(arg2, label_both_true);
          ASM_SET(dst_reg, arg1);
          goto_(label_end);
        label_both_true.put();
          ASM_NULL(dst_reg);
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
        auto dst_reg = REG_OBJ();
          auto arg1 = to_o(do_compile(node->children.nodes[0]));
          if_any(arg1, label_a1);
          auto arg2 = to_o(do_compile(node->children.nodes[1]));
          ASM_SET(dst_reg, arg2);
          goto_(label_end);
        label_a1.put();
          ASM_SET(dst_reg, arg1);
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
        auto dst_reg = REG_OBJ();
          auto arg1 = to_o(do_compile(node->children.nodes[0]));
          unless_any(arg1, label_a1);
          auto arg2 = to_o(do_compile(node->children.nodes[1]));
          ASM_SET(dst_reg, arg2);
          goto_(label_end);
        label_a1.put();
          ASM_SET(dst_reg, arg1);
        label_end.put();
        return dst_reg;
      }
      case PVIP_NODE_UNARY_PLUS: {
        return to_n(do_compile(node->children.nodes[0]));
      }
      case PVIP_NODE_UNARY_MINUS: {
        auto reg = do_compile(node->children.nodes[0]);
        if (Kiji_compiler_get_local_type(this, reg) == MVM_reg_int64) {
          ASM_NEG_I(reg, reg);
          return reg;
        } else {
          reg = to_n(reg);
          ASM_NEG_N(reg, reg);
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
                ASM_SAY(reg_num);
              } else {
                ASM_PRINT(reg_num);
              }
            }
            return const_true();
          } else if (std::string(ident->pv->buf, ident->pv->len) == "print") {
            for (int i=0; i<args->children.size; i++) {
              PVIPNode* a = args->children.nodes[i];
              uint16_t reg_num = stringify(do_compile(a));
              ASM_PRINT(reg_num);
            }
            return const_true();
          } else if (std::string(ident->pv->buf, ident->pv->len) == "open") {
            // TODO support arguments
            assert(args->children.size == 1);
            auto fname_s = do_compile(args->children.nodes[0]);
            auto dst_reg_o = REG_OBJ();
            // TODO support latin1, etc.
            auto mode = push_string("r");
            auto mode_s = REG_STR();
            ASM_CONST_S(mode_s, mode);
            ASM_OPEN_FH(dst_reg_o, fname_s, mode_s);
            return dst_reg_o;
          } else if (std::string(ident->pv->buf, ident->pv->len) == "slurp") {
            assert(args->children.size <= 2);
            assert(args->children.size != 2 && "Encoding option is not supported yet");
            auto fname_s = do_compile(args->children.nodes[0]);
            auto dst_reg_s = REG_STR();
            auto encoding_s = REG_STR();
            ASM_CONST_S(encoding_s, push_string("utf8")); // TODO support latin1, etc.
            ASM_SLURP(dst_reg_s, fname_s, encoding_s);
            return dst_reg_s;
          }
        }

        {
          uint16_t func_reg_no;
          if (node->children.nodes[0]->type == PVIP_NODE_IDENT) {
            func_reg_no = REG_OBJ();
            int lex_no, outer;
            if (!find_lexical_by_name(std::string("&") + std::string(ident->pv->buf, ident->pv->len), &lex_no, &outer)) {
              MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", (std::string("&") + std::string(ident->pv->buf, ident->pv->len)).c_str());
            }
            ASM_GETLEX(
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

              switch (Kiji_compiler_get_local_type(this, reg)) {
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
            ASM_PREPARGS(callsite_no);

            int i=0;
            for (auto reg:arg_regs) {
              switch (Kiji_compiler_get_local_type(this, reg)) {
              case MVM_reg_int64:
                ASM_ARG_I(i, reg);
                break;
              case MVM_reg_num64:
                ASM_ARG_N(i, reg);
                break;
              case MVM_reg_str:
                ASM_ARG_S(i, reg);
                break;
              case MVM_reg_obj:
                ASM_ARG_O(i, reg);
                break;
              default:
                abort();
              }
              ++i;
            }
          }
          auto dest_reg = REG_OBJ(); // ctx
          ASM_INVOKE_O(
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
      auto reg_type = Kiji_compiler_get_local_type(this, reg_num);
      if (reg_type == MVM_reg_obj) {
        return reg_num;
      }

      int dst_num = REG_OBJ();
      int boxtype_reg = REG_OBJ();
      switch (reg_type) {
      case MVM_reg_str:
        ASM_HLLBOXTYPE_S(boxtype_reg);
        ASM_BOX_S(dst_num, reg_num, boxtype_reg);
        return dst_num;
      case MVM_reg_int64:
        ASM_HLLBOXTYPE_I(boxtype_reg);
        ASM_BOX_I(dst_num, reg_num, boxtype_reg);
        return dst_num;
      case MVM_reg_num64:
        ASM_HLLBOXTYPE_N(boxtype_reg);
        ASM_BOX_N(dst_num, reg_num, boxtype_reg);
        return dst_num;
      default:
        MVM_panic(MVM_exitcode_compunit, "Not implemented, boxify %d", Kiji_compiler_get_local_type(this, reg_num));
        abort();
      }
    }
    int to_n(int reg_num) {
      assert(reg_num != UNKNOWN_REG);
      switch (Kiji_compiler_get_local_type(this, reg_num)) {
      case MVM_reg_str: {
        int dst_num = REG_NUM64();
        ASM_COERCE_SN(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_num64: {
        return reg_num;
      }
      case MVM_reg_int64: {
        int dst_num = REG_NUM64();
        ASM_COERCE_IN(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_obj: {
        int dst_num = REG_NUM64();
        ASM_SMRT_NUMIFY(dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", Kiji_compiler_get_local_type(this, reg_num));
        break;
      }
    }
    int to_i(int reg_num) {
      assert(reg_num != UNKNOWN_REG);
      switch (Kiji_compiler_get_local_type(this, reg_num)) {
      case MVM_reg_str: {
        int dst_num = REG_INT64();
        ASM_COERCE_SI(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_num64: {
        int dst_num = REG_NUM64();
        ASM_COERCE_NI(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_int64: {
        return reg_num;
      }
      case MVM_reg_obj: {
        int dst_num = REG_NUM64();
        int dst_int = REG_INT64();
        // TODO: I need smrt_intify?
        ASM_SMRT_NUMIFY(dst_num, reg_num);
        ASM_COERCE_NI(dst_int, dst_num);
        return dst_int;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", Kiji_compiler_get_local_type(this, reg_num));
        break;
      }
    }
    int to_s(int reg_num) { return stringify(reg_num); }
    int stringify(int reg_num) {
      assert(reg_num != UNKNOWN_REG);
      switch (Kiji_compiler_get_local_type(this, reg_num)) {
      case MVM_reg_str:
        // nop
        return reg_num;
      case MVM_reg_num64: {
        int dst_num = REG_STR();
        ASM_COERCE_NS(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_int64: {
        int dst_num = REG_STR();
        ASM_COERCE_IS(dst_num, reg_num);
        return dst_num;
      }
      case MVM_reg_obj: {
        int dst_num = REG_STR();
        ASM_SMRT_STRIFY(dst_num, reg_num);
        return dst_num;
      }
      default:
        // TODO
        MVM_panic(MVM_exitcode_compunit, "Not implemented, stringify %d", Kiji_compiler_get_local_type(this, reg_num));
        break;
      }
    }
    int str_binop(const PVIPNode* node, uint16_t op) {
        assert(node->children.size == 2);

        int reg_num1 = to_s(do_compile(node->children.nodes[0]));
        int reg_num2 = to_s(do_compile(node->children.nodes[1]));
        int reg_num_dst = REG_INT64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, reg_num_dst, reg_num1, reg_num2);
        return reg_num_dst;
    }
    int binary_binop(const PVIPNode* node, uint16_t op_i) {
        assert(node->children.size == 2);

        int reg_num1 = to_i(do_compile(node->children.nodes[0]));
        int reg_num2 = to_i(do_compile(node->children.nodes[1]));
        auto dst_reg = REG_INT64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_i, dst_reg, reg_num1, reg_num2);
        return dst_reg;
    }
    int numeric_inplace(const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = REG_NUM64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, tmp, to_n(lhs), to_n(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int binary_inplace(const PVIPNode* node, uint16_t op) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = REG_INT64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op, tmp, to_i(lhs), to_i(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int str_inplace(const PVIPNode* node, uint16_t op, uint16_t rhs_type) {
        assert(node->children.size == 2);
        assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

        auto lhs = get_variable(node->children.nodes[0]->pv);
        auto rhs = do_compile(node->children.nodes[1]);
        auto tmp = REG_STR();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, tmp, to_s(lhs), rhs_type == MVM_reg_int64 ? to_i(rhs) : to_s(rhs));
        set_variable(node->children.nodes[0]->pv, to_o(tmp));
        return tmp;
    }
    int numeric_binop(const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
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
    uint16_t unless_op(uint16_t cond_reg) {
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
        ASM_NULL(dst_reg);
      } else {
        ASM_SET(dst_reg, to_o(reg));
      }
    }
    // Compile chained comparisions like `1 < $n < 3`.
    // TODO: optimize simple case like `1 < $n`
    uint16_t compile_chained_comparisions(const PVIPNode* node) {
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
    int num_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op_i, uint16_t op_n) {
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
    int str_cmp_binop(uint16_t lhs, uint16_t rhs, uint16_t op) {
        int reg_num_dst = REG_INT64();
        ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, reg_num_dst, to_s(lhs), to_s(rhs));
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
    KijiCompiler(MVMCompUnit * cu, MVMThreadContext * tc): cu_(cu), frame_no_(0), tc_(tc) {
      initialize();

      current_class_how_ = NULL;

      auto handle = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"__SARU_CLASSES__");
      assert(tc);
      sc_classes_ = (MVMSerializationContext*)MVM_sc_create(tc_, handle);

      num_sc_classes_ = 0;
    }
    ~KijiCompiler() { }
    void initialize() {
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
    void finalize(MVMInstance* vm) {
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
    void compile(PVIPNode*node, MVMInstance* vm) {
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

