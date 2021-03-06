#pragma once
/* vim:ts=2:sw=2:tw=0:
 */

#include <stdint.h>
#include "pvip.h"
#include "frame.h"
#include "handy.h"
#include "compiler/label.h"

#define KIJI_SC_BUILTIN_TWARGS     0
#define KIJI_SC_BUILTIN_PAIR_CLASS 1

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

#define newMVMString(str, len) \
  MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, str, len)

#define newMVMString_nolen(str) \
  MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, (str), strlen((str)))

#define newMVMStringFromPVIP(p) \
  MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, (p)->buf, (p)->len)

#define newMVMStringFromSTDSTRING(p) \
  MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, (p).c_str(), (p).size())



#define REG_OBJ() Kiji_compiler_push_local_type(self, MVM_reg_obj)
#define REG_STR() Kiji_compiler_push_local_type(self, MVM_reg_str)
#define REG_INT64() Kiji_compiler_push_local_type(self, MVM_reg_int64)
#define REG_NUM64() Kiji_compiler_push_local_type(self, MVM_reg_num64)

/**
* OP map is 3rd/MoarVM/src/core/oplist
* interp code is 3rd/MoarVM/src/core/interp.c
*/
enum { UNKNOWN_REG = -1 };
typedef struct _KijiCompiler {
  KijiFrame** frames;
  size_t num_frames;

  MVMCompUnit* cu;
  MVMThreadContext *tc;
  int frame_no;
  MVMObject* current_class_how;
  MVMInstance *vm;
} KijiCompiler;

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

#ifdef __cplusplus
extern "C" {
#endif
int Kiji_compiler_do_compile(KijiCompiler *self, const PVIPNode*node);
size_t Kiji_compiler_bytecode_size(KijiCompiler*self);
bool Kiji_compiler_find_lexical_by_name(KijiCompiler *self, MVMString *name, int *lex_no, int *outer);
int Kiji_compiler_push_string(KijiCompiler *self, MVMString *str);
int Kiji_compiler_const_true(KijiCompiler *self);
void Kiji_compiler_goto(KijiCompiler*self, KijiLabel *label);
void Kiji_compiler_return_any(KijiCompiler *self, uint16_t reg);
void Kiji_compiler_if_any(KijiCompiler *self, uint16_t reg, KijiLabel *label);
void Kiji_compiler_unless_any(KijiCompiler *self, uint16_t reg, KijiLabel *label);
uint16_t Kiji_compiler_if_op(KijiCompiler* self, uint16_t cond_reg);
uint16_t Kiji_compiler_unless_op(KijiCompiler * self, uint16_t cond_reg);
int Kiji_compiler_push_lexical(KijiCompiler *self, MVMString *name, MVMuint16 type);
void Kiji_compiler_push_pkg_var(KijiCompiler *self, MVMString *name);
void Kiji_compiler_pop_frame(KijiCompiler* self);
int Kiji_compiler_push_frame(KijiCompiler* self, const char* name, size_t name_len);
Kiji_variable_type_t Kiji_compiler_find_variable_by_name(KijiCompiler* self, MVMString *name, int *lex_no, int *outer);
void Kiji_compiler_finalize(KijiCompiler *self, MVMInstance* vm);
void Kiji_compiler_compile(KijiCompiler *self, PVIPNode*node, MVMInstance* vm);
uint16_t Kiji_compiler_get_variable(KijiCompiler *self, MVMString *name);
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
void Kiji_compiler_init(KijiCompiler *self, MVMCompUnit * cu, MVMThreadContext * tc, MVMInstance *vm);
int Kiji_compiler_str_cmp_binop(KijiCompiler * self, uint16_t lhs, uint16_t rhs, uint16_t op);
int Kiji_compiler_num_cmp_binop(KijiCompiler *self, uint16_t lhs, uint16_t rhs, uint16_t op_i, uint16_t op_n);
size_t Kiji_compiler_push_callsite(KijiCompiler *self, MVMCallsite *callsite);
MVMuint16 Kiji_compiler_count_min_arity(KijiCompiler* self, PVIPNode*node);
void Kiji_compiler_push_sc_object(KijiCompiler *self, MVMString *handle, MVMObject * object, int *wval1, int *wval2);
void Kiji_compiler_push_sc(KijiCompiler *self, MVMSerializationContext*sc);
#ifdef __cplusplus
};
#endif
