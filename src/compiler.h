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
#define CU cu

#define MEMORY_ERROR() \
          MVM_panic(MVM_exitcode_compunit, "Compilation error. return with non-value.");

#define REG_OBJ() Kiji_compiler_push_local_type(this, MVM_reg_obj)
#define REG_STR() Kiji_compiler_push_local_type(this, MVM_reg_str)
#define REG_INT64() Kiji_compiler_push_local_type(this, MVM_reg_int64)
#define REG_NUM64() Kiji_compiler_push_local_type(this, MVM_reg_num64)


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
    MVMCompUnit* cu;
    MVMThreadContext *tc_;
    int frame_no_;
    MVMObject* current_class_how_;

    MVMSerializationContext * sc_classes_;
    int num_sc_classes_;

    KijiLabel label() { return KijiLabel(this, frames_.back()->frame.bytecode_size); }
    KijiLabel label_unsolved() { return KijiLabel(this); }

    void goto_(KijiLabel &label);
    void return_any(uint16_t reg);
    void if_any(uint16_t reg, KijiLabel &label);
    void unless_any(uint16_t reg, KijiLabel &label);

    int compile_class(const PVIPNode* node);

    uint16_t get_variable(MVMString *name);
    void set_variable(MVMString *name, uint16_t val_reg);

    // This reg returns register number contains true value.
    int const_true();

    int push_string(MVMString *name);
    // lexical variable number by name
    bool find_lexical_by_name(MVMString *name_cc, int *lex_no, int *outer);
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

