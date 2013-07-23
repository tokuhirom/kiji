#pragma once
/* vim:ts=2:sw=2:tw=0:
 */

#include <stdint.h>
#include <memory>
#include "gen.assembler.h"
#include "pvip.h"
#include "frame.h"
#include "handy.h"
#include "asm.h"
#include <vector>

#define ASM_OP_U16_U16_U16(a,b,c,d,e) \
  Kiji_asm_op_u16_u16_u16(Kiji_compiler_top_frame(self), a,b,c,d,e)

#define ASM_OP_U16_U16(a,b,c,d) \
  Kiji_asm_op_u16_u16(Kiji_compiler_top_frame(self), a,b,c,d)

#define ASM_WRITE_UINT16_T(a,b) \
  Kiji_asm_write_uint16_t_for(Kiji_compiler_top_frame(self), a,b)

#define ASM_OP_U16_U32(a,b, c, d) \
  Kiji_asm_op_u16_u32(Kiji_compiler_top_frame(self), a,b, c,d)

#define MVM_ASSIGN_REF2(tc, update_root, update_addr, referenced) \
    { \
        void *_r = referenced; \
        MVM_WB(tc, update_root, _r); \
        update_addr = (MVMObject*)_r; \
    }

#define REG_OBJ() Kiji_compiler_push_local_type(self, MVM_reg_obj)
#define REG_STR() Kiji_compiler_push_local_type(self, MVM_reg_str)
#define REG_INT64() Kiji_compiler_push_local_type(self, MVM_reg_int64)
#define REG_NUM64() Kiji_compiler_push_local_type(self, MVM_reg_num64)

struct KijiCompiler;

class KijiLabel {
private:
  KijiCompiler *compiler_;
  ssize_t address_;
  std::vector<ssize_t> reserved_addresses_;
public:
  KijiLabel(KijiCompiler *compiler) :compiler_(compiler), address_(-1) { }
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

/**
* OP map is 3rd/MoarVM/src/core/oplist
* interp code is 3rd/MoarVM/src/core/interp.c
*/
enum { UNKNOWN_REG = -1 };
struct KijiCompiler {
  KijiFrame** frames;
  size_t num_frames;

  MVMCompUnit* cu;
  MVMThreadContext *tc;
  int frame_no;
  MVMObject* current_class_how;

  MVMSerializationContext * sc_classes;
  int num_sc_classes;
};

KIJI_STATIC_INLINE KijiFrame* Kiji_compiler_top_frame(KijiCompiler *self) {
  return self->frames[self->num_frames-1];
}

// reserve register
KIJI_STATIC_INLINE int Kiji_compiler_push_local_type(KijiCompiler* self, MVMuint16 reg_type) {
  return Kiji_frame_push_local_type(Kiji_compiler_top_frame(self), reg_type);
}

// Get register type at 'n'
KIJI_STATIC_INLINE uint16_t Kiji_compiler_get_local_type(KijiCompiler* self, int n) {
  return Kiji_frame_get_local_type(Kiji_compiler_top_frame(self), n);
}

uint16_t Kiji_compiler_get_variable(KijiCompiler *self, MVMString *name);
int Kiji_compiler_push_string(KijiCompiler *self, MVMString *str);
void Kiji_compiler_goto(KijiCompiler*self, KijiLabel &label);
void Kiji_compiler_return_any(KijiCompiler *self, uint16_t reg);
void Kiji_compiler_if_any(KijiCompiler *self, uint16_t reg, KijiLabel &label);
void Kiji_compiler_unless_any(KijiCompiler *self, uint16_t reg, KijiLabel &label);
uint16_t Kiji_compiler_unless_op(KijiCompiler * self, uint16_t cond_reg);
int Kiji_compiler_const_true(KijiCompiler *self);
void Kiji_compiler_compile_array(KijiCompiler* self, uint16_t array_reg, const PVIPNode* node);
int Kiji_compiler_compile_class(KijiCompiler *self, const PVIPNode* node);
void Kiji_compiler_set_variable(KijiCompiler *self, MVMString * name, uint16_t val_reg);
int Kiji_compiler_to_o(KijiCompiler *self, int reg_num);
int Kiji_compiler_to_n(KijiCompiler *self, int reg_num);
int Kiji_compiler_to_i(KijiCompiler *self, int reg_num);
int Kiji_compiler_to_s(KijiCompiler *self, int reg_num);
int Kiji_compiler_str_binop(KijiCompiler *self, const PVIPNode* node, uint16_t op);
int Kiji_compiler_binary_binop(KijiCompiler *self, const PVIPNode* node, uint16_t op_i);
int Kiji_compiler_numeric_inplace(KijiCompiler *self, const PVIPNode* node, uint16_t op_i, uint16_t op_n);
int Kiji_compiler_binary_inplace(KijiCompiler *self, const PVIPNode* node, uint16_t op);
int Kiji_compiler_str_inplace(KijiCompiler *self, const PVIPNode* node, uint16_t op, uint16_t rhs_type);
int Kiji_compiler_numeric_binop(KijiCompiler *self, const PVIPNode* node, uint16_t op_i, uint16_t op_n);
void Kiji_compiler_compile_statements(KijiCompiler *self, const PVIPNode*node, int dst_reg);
uint16_t Kiji_compiler_compile_chained_comparisions(KijiCompiler *self, const PVIPNode* node);
int Kiji_compiler_num_cmp_binop(KijiCompiler *self, uint16_t lhs, uint16_t rhs, uint16_t op_i, uint16_t op_n);
uint16_t Kiji_compiler_do_compare(KijiCompiler* self, PVIP_node_type_t type, uint16_t lhs, uint16_t rhs);
void Kiji_compiler_compile(KijiCompiler *self, PVIPNode*node, MVMInstance* vm);
int Kiji_compiler_do_compile(KijiCompiler *self, const PVIPNode*node);
void Kiji_compiler_init(KijiCompiler *self, MVMCompUnit * cu, MVMThreadContext * tc);
uint16_t Kiji_compiler_if_op(KijiCompiler* self, uint16_t cond_reg);
void Kiji_compiler_finalize(KijiCompiler *self, MVMInstance* vm);
size_t Kiji_compiler_bytecode_size(KijiCompiler*self);
