/* vim:ts=2:sw=2:tw=0:
 */
#include "moarvm.h"
#include "../compiler.h"
#include "../gen.assembler.h"
#include "../asm.h"

/* This reg returns register number contains true value. */
int Kiji_compiler_const_true(KijiCompiler *self) {
  auto reg = REG_INT64();
  ASM_CONST_I64(reg, 1);
  return reg;
}

void Kiji_compiler_goto(KijiCompiler*self, KijiLabel *label) {
  if (!Kiji_label_is_solved(label)) {
    Kiji_label_reserve(label, Kiji_compiler_bytecode_size(self) + 2);
  }
  ASM_GOTO(label->address);
}

void Kiji_compiler_unless_any(KijiCompiler *self, uint16_t reg, KijiLabel *label) {
  if (!Kiji_label_is_solved(label)) {
    Kiji_label_reserve(label, Kiji_compiler_bytecode_size(self) + 2 + 2);
  }
  ASM_OP_U16_U32(MVM_OP_BANK_primitives, Kiji_compiler_unless_op(self, reg), reg, label->address);
}

void Kiji_compiler_if_any(KijiCompiler *self, uint16_t reg, KijiLabel *label) {
  if (!Kiji_label_is_solved(label)) {
    Kiji_label_reserve(label, Kiji_compiler_bytecode_size(self) + 2 + 2);
  }
  ASM_OP_U16_U32(MVM_OP_BANK_primitives, Kiji_compiler_if_op(self, reg), reg, label->address);
}

void Kiji_compiler_return_any(KijiCompiler *self, uint16_t reg) {
  switch (Kiji_compiler_get_local_type(self, reg)) {
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

uint16_t Kiji_compiler_unless_op(KijiCompiler * self, uint16_t cond_reg) {
  switch (Kiji_compiler_get_local_type(self, cond_reg)) {
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

uint16_t Kiji_compiler_if_op(KijiCompiler* self, uint16_t cond_reg) {
  switch (Kiji_compiler_get_local_type(self, cond_reg)) {
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



int Kiji_compiler_compile_class(KijiCompiler *self, const PVIPNode* node) {
  MVMThreadContext *tc = self->tc;
  int wval1, wval2;
  {
    // Create new class.
    MVMObject * type = MVM_6model_find_method(
      tc,
      STABLE(tc->instance->KnowHOW)->HOW,
      MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"new_type")
    );
    MVMObject * how = STABLE(type)->HOW;

    // Add "new" method
    {
      MVMString * name = MVM_string_ascii_decode_nt(tc, tc->instance->VMString, (char*)"new");
      // self, type_obj, name, method
      MVMObject * method_cache = REPR(tc->instance->boot_types->BOOTHash)->allocate(tc, STABLE(tc->instance->boot_types->BOOTHash));
      // MVMObject * method_cache = REPR(type)->allocate(tc, STABLE(type));
      // MVMObject * method_table = ((MVMKnowHOWREPR *)type)->body.methods;
      MVMObject * BOOTCCode = tc->instance->boot_types->BOOTCCode;
      MVMObject * method = REPR(BOOTCCode)->allocate(tc, STABLE(BOOTCCode));
      ((MVMCFunction *)method)->body.func = Mu_new;
      REPR(method_cache)->ass_funcs->bind_key_boxed(
          tc,
          STABLE(method_cache),
          method_cache,
          OBJECT_BODY(method_cache),
          (MVMObject*)name,
          method);
      // REPR(method_table)->ass_funcs->bind_key_boxed(tc, STABLE(method_table),
          // method_table, OBJECT_BODY(method_table), (MVMObject *)name, method);
      STABLE(type)->method_cache = method_cache;
    }

    Kiji_compiler_push_sc_object(self, type, &wval1, &wval2);
    self->current_class_how = how;
  }

  /* compile body */
  int i;
  for (i=0; i<node->children.nodes[2]->children.size; i++) {
    PVIPNode *n = node->children.nodes[2]->children.nodes[i];
    (void)Kiji_compiler_do_compile(self, n);
  }

  self->current_class_how = NULL;

  auto retval = REG_OBJ();
  ASM_WVAL(retval, wval1, wval2);

  // Bind class object to lexical variable
  PVIPNode * name_node = node->children.nodes[0];
  if (PVIP_node_category(name_node->type) == PVIP_CATEGORY_STRING) {
    MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, name_node->pv->buf, name_node->pv->len);
    auto lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
    ASM_BINDLEX(
      lex,
      0,
      retval
    );
  }

  return retval;
}

int Kiji_compiler_str_cmp_binop(KijiCompiler * self, uint16_t lhs, uint16_t rhs, uint16_t op) {
    int reg_num_dst = REG_INT64();
    ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, reg_num_dst, Kiji_compiler_to_s(self, lhs), Kiji_compiler_to_s(self, rhs));
    return reg_num_dst;
}


uint16_t Kiji_compiler_do_compare(KijiCompiler* self, PVIP_node_type_t type, uint16_t lhs, uint16_t rhs) {
  switch (type) {
  case PVIP_NODE_STREQ:
    return Kiji_compiler_str_cmp_binop(self, lhs, rhs, MVM_OP_eq_s);
  case PVIP_NODE_STRNE:
    return Kiji_compiler_str_cmp_binop(self, lhs, rhs, MVM_OP_ne_s);
  case PVIP_NODE_STRGT:
    return Kiji_compiler_str_cmp_binop(self, lhs, rhs, MVM_OP_gt_s);
  case PVIP_NODE_STRGE:
    return Kiji_compiler_str_cmp_binop(self, lhs, rhs, MVM_OP_ge_s);
  case PVIP_NODE_STRLT:
    return Kiji_compiler_str_cmp_binop(self, lhs, rhs, MVM_OP_lt_s);
  case PVIP_NODE_STRLE:
    return Kiji_compiler_str_cmp_binop(self, lhs, rhs, MVM_OP_le_s);
  case PVIP_NODE_EQ:
    return Kiji_compiler_num_cmp_binop(self, lhs, rhs, MVM_OP_eq_i, MVM_OP_eq_n);
  case PVIP_NODE_NE:
    return Kiji_compiler_num_cmp_binop(self, lhs, rhs, MVM_OP_ne_i, MVM_OP_ne_n);
  case PVIP_NODE_LT:
    return Kiji_compiler_num_cmp_binop(self, lhs, rhs, MVM_OP_lt_i, MVM_OP_lt_n);
  case PVIP_NODE_LE:
    return Kiji_compiler_num_cmp_binop(self, lhs, rhs, MVM_OP_le_i, MVM_OP_le_n);
  case PVIP_NODE_GT:
    return Kiji_compiler_num_cmp_binop(self, lhs, rhs, MVM_OP_gt_i, MVM_OP_gt_n);
  case PVIP_NODE_GE:
    return Kiji_compiler_num_cmp_binop(self, lhs, rhs, MVM_OP_ge_i, MVM_OP_ge_n);
  case PVIP_NODE_EQV:
    return Kiji_compiler_str_cmp_binop(self, lhs, rhs, MVM_OP_eq_s);
  default:
    abort();
  }
}

/* Compile chained comparisions like `1 < $n < 3`.
 TODO: optimize simple case like `1 < $n`
 */
int Kiji_compiler_num_cmp_binop(KijiCompiler *self, uint16_t lhs, uint16_t rhs, uint16_t op_i, uint16_t op_n) {
    int reg_num_dst = REG_INT64();
    if (Kiji_compiler_get_local_type(self, lhs) == MVM_reg_int64) {
      assert(Kiji_compiler_get_local_type(self, lhs) == MVM_reg_int64);
      // assert(Kiji_compiler_get_local_type(self, rhs) == MVM_reg_int64);
      ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_i, reg_num_dst, lhs, Kiji_compiler_to_i(self, rhs));
      return reg_num_dst;
    } else if (Kiji_compiler_get_local_type(self, lhs) == MVM_reg_num64) {
      ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, lhs, Kiji_compiler_to_n(self, rhs));
      return reg_num_dst;
    } else if (Kiji_compiler_get_local_type(self, lhs) == MVM_reg_obj) {
      // TODO should I use intify instead if the object is int?
      ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, Kiji_compiler_to_n(self, lhs), Kiji_compiler_to_n(self, rhs));
      return reg_num_dst;
    } else {
      // NOT IMPLEMENTED
      abort();
    }
}

uint16_t Kiji_compiler_compile_chained_comparisions(KijiCompiler *self, const PVIPNode* node) {
  MVMuint16 lhs = Kiji_compiler_do_compile(self, node->children.nodes[0]);
  MVMuint16 dst_reg = REG_INT64();
  LABEL(label_end);
  LABEL(label_false);
  int i;
  for (i=1; i<node->children.size; i++) {
    PVIPNode *iter = node->children.nodes[i];
    MVMuint16 rhs = Kiji_compiler_do_compile(self, iter->children.nodes[0]);
    // result will store to lhs.
    uint16_t ret = Kiji_compiler_do_compare(self, iter->type, lhs, rhs);
    Kiji_compiler_unless_any(self, ret, &label_false);
    lhs = rhs;
  }
  ASM_CONST_I64(dst_reg, 1);
  Kiji_compiler_goto(self, &label_end);
LABEL_PUT(label_false);
  ASM_CONST_I64(dst_reg, 0);
  // Kiji_compiler_goto(self, &label_end());
LABEL_PUT(label_end);
  return dst_reg;
}

void Kiji_compiler_compile_statements(KijiCompiler *self, const PVIPNode*node, int dst_reg) {
  int reg = UNKNOWN_REG;
  if (node->type == PVIP_NODE_STATEMENTS || node->type == PVIP_NODE_ELSE) {
    int i, l;
    for (i=0, l=node->children.size; i<l; i++) {
      reg = Kiji_compiler_do_compile(self, node->children.nodes[i]);
    }
  } else {
    reg = Kiji_compiler_do_compile(self, node);
  }
  if (reg == UNKNOWN_REG) {
    ASM_NULL(dst_reg);
  } else {
    ASM_SET(dst_reg, Kiji_compiler_to_o(self, reg));
  }
}

int Kiji_compiler_numeric_binop(KijiCompiler *self, const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
  assert(node->children.size == 2);

  int reg_num1 = Kiji_compiler_do_compile(self, node->children.nodes[0]);
  if (Kiji_compiler_get_local_type(self, reg_num1) == MVM_reg_int64) {
    int reg_num_dst = REG_INT64();
    int reg_num2 = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
    assert(Kiji_compiler_get_local_type(self, reg_num1) == MVM_reg_int64);
    assert(Kiji_compiler_get_local_type(self, reg_num2) == MVM_reg_int64);
    ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_i, reg_num_dst, reg_num1, reg_num2);
    return reg_num_dst;
  } else if (Kiji_compiler_get_local_type(self, reg_num1) == MVM_reg_num64) {
    int reg_num_dst = REG_NUM64();
    int reg_num2 = Kiji_compiler_to_n(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
    assert(Kiji_compiler_get_local_type(self, reg_num2) == MVM_reg_num64);
    ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, reg_num1, reg_num2);
    return reg_num_dst;
  } else if (Kiji_compiler_get_local_type(self, reg_num1) == MVM_reg_obj) {
    // TODO should I use intify instead if the object is int?
    int reg_num_dst = REG_NUM64();

    int dst_num = REG_NUM64();
    ASM_OP_U16_U16(MVM_OP_BANK_primitives, MVM_OP_smrt_numify, dst_num, reg_num1);

    int reg_num2 = Kiji_compiler_to_n(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
    assert(Kiji_compiler_get_local_type(self, reg_num2) == MVM_reg_num64);
    ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);
    return reg_num_dst;
  } else if (Kiji_compiler_get_local_type(self, reg_num1) == MVM_reg_str) {
    int dst_num = REG_NUM64();
    ASM_COERCE_SN(dst_num, reg_num1);

    int reg_num_dst = REG_NUM64();
    int reg_num2 = Kiji_compiler_to_n(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
    ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, reg_num_dst, dst_num, reg_num2);

    return reg_num_dst;
  } else {
    abort();
  }
}

int Kiji_compiler_str_inplace(KijiCompiler *self, const PVIPNode* node, uint16_t op, uint16_t rhs_type) {
    assert(node->children.size == 2);
    assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

    MVMuint16 lhs = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
    MVMuint16 rhs = Kiji_compiler_do_compile(self, node->children.nodes[1]);
    MVMuint16 tmp = REG_STR();
    ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, tmp, Kiji_compiler_to_s(self, lhs), rhs_type == MVM_reg_int64 ? Kiji_compiler_to_i(self, rhs) : Kiji_compiler_to_s(self, rhs));
    Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), Kiji_compiler_to_o(self, tmp));
    return tmp;
}

int Kiji_compiler_binary_inplace(KijiCompiler *self, const PVIPNode* node, uint16_t op) {
    assert(node->children.size == 2);
    assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

    MVMuint16 lhs = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
    MVMuint16 rhs = Kiji_compiler_do_compile(self, node->children.nodes[1]);
    MVMuint16 tmp = REG_INT64();
    ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op, tmp, Kiji_compiler_to_i(self, lhs), Kiji_compiler_to_i(self, rhs));
    Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), Kiji_compiler_to_o(self, tmp));
    return tmp;
}

int Kiji_compiler_numeric_inplace(KijiCompiler *self, const PVIPNode* node, uint16_t op_i, uint16_t op_n) {
    assert(node->children.size == 2);
    assert(node->children.nodes[0]->type == PVIP_NODE_VARIABLE);

    MVMuint16 lhs = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
    MVMuint16 rhs = Kiji_compiler_do_compile(self, node->children.nodes[1]);
    MVMuint16 tmp = REG_NUM64();
    ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_n, tmp, Kiji_compiler_to_n(self, lhs), Kiji_compiler_to_n(self, rhs));
    Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), Kiji_compiler_to_o(self, tmp));
    return tmp;
}


int Kiji_compiler_binary_binop(KijiCompiler *self, const PVIPNode* node, uint16_t op_i) {
    assert(node->children.size == 2);

    MVMuint16 reg_num1 = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    MVMuint16 reg_num2 = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
    MVMuint16 dst_reg = REG_INT64();
    ASM_OP_U16_U16_U16(MVM_OP_BANK_primitives, op_i, dst_reg, reg_num1, reg_num2);
    return dst_reg;
}


int Kiji_compiler_str_binop(KijiCompiler *self, const PVIPNode* node, uint16_t op) {
  assert(node->children.size == 2);

  int reg_num1 = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  int reg_num2 = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
  int reg_num_dst = REG_INT64();
  ASM_OP_U16_U16_U16(MVM_OP_BANK_string, op, reg_num_dst, reg_num1, reg_num2);
  return reg_num_dst;
}

int Kiji_compiler_to_s(KijiCompiler *self, int reg_num) {
  assert(reg_num != UNKNOWN_REG);
  switch (Kiji_compiler_get_local_type(self, reg_num)) {
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
    MVM_panic(MVM_exitcode_compunit, "Not implemented, to_s %d", Kiji_compiler_get_local_type(self, reg_num));
    break;
  }
}

int Kiji_compiler_to_i(KijiCompiler * self, int reg_num) {
  assert(reg_num != UNKNOWN_REG);
  switch (Kiji_compiler_get_local_type(self, reg_num)) {
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
    MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", Kiji_compiler_get_local_type(self, reg_num));
    break;
  }
}

int Kiji_compiler_to_n(KijiCompiler *self, int reg_num) {
  assert(reg_num != UNKNOWN_REG);
  switch (Kiji_compiler_get_local_type(self, reg_num)) {
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
    MVM_panic(MVM_exitcode_compunit, "Not implemented, numify %d", Kiji_compiler_get_local_type(self, reg_num));
    break;
  }
}

int Kiji_compiler_to_o(KijiCompiler *self, int reg_num) {
  assert(reg_num != UNKNOWN_REG);
  MVMuint16 reg_type = Kiji_compiler_get_local_type(self, reg_num);
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
    MVM_panic(MVM_exitcode_compunit, "Not implemented, boxify %d", Kiji_compiler_get_local_type(self, reg_num));
    abort();
  }
}

void Kiji_compiler_compile_array(KijiCompiler* self, uint16_t array_reg, const PVIPNode* node) {
  if (node->type==PVIP_NODE_LIST) {
    int i;
    for (i=0; i<node->children.size; i++) {
      PVIPNode* m = node->children.nodes[i];
      Kiji_compiler_compile_array(self, array_reg, m);
    }
  } else {
    auto reg = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node));
    ASM_PUSH_O(array_reg, reg);
  }
}

MVMuint16 Kiji_compiler_count_min_arity(KijiCompiler* self, PVIPNode*node) {
  int i;
  MVMuint16 ret = 0;
  for (i=0; i<node->children.size; ++i) {
    PVIPNode* n = node->children.nodes[i];
    if (n->children.nodes[2]->type == PVIP_NODE_NOP) {
      ++ret;
    }
  }
  return ret;
}

