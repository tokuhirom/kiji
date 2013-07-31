/* vim:ts=2:sw=2:tw=0:
 */

#include "moarvm.h"
#include "nd.h"

ND(NODE_METHOD) {
  PVIPNode * name_node = node->children.nodes[0];
  const PVIPString *name = name_node->pv;

  MVMuint16 funcreg = REG_OBJ();
  PVIPString *amp_name = PVIP_string_new();
  PVIP_string_concat(amp_name, "&", strlen("&"));
  PVIP_string_concat(amp_name, name->buf, name->len);
  MVMString * mvm_name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, amp_name->buf, amp_name->len);
  PVIP_string_destroy(amp_name);
  int funclex = Kiji_compiler_push_lexical(self, mvm_name, MVM_reg_obj);
  int func_pos = Kiji_compiler_bytecode_size(self) + 2 + 2;
  ASM_GETCODE(funcreg, 0);
  ASM_BINDLEX(
    funclex,
    0, // frame outer count
    funcreg
  );

  // Compile function body
  int frame_no = Kiji_compiler_push_frame(self, name->buf, name->len);

  // TODO process named params
  // TODO process types
  {
    ASM_CHECKARITY(
      node->children.nodes[1]->children.size+1,
      node->children.nodes[1]->children.size+1
    );

    {
      // push self
      MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, "__self", strlen("__self"));
      int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
      int reg = REG_OBJ();
      ASM_PARAM_RP_O(reg, 0);
      ASM_BINDLEX(lex, 0, reg);
    }

    int i;
    for (i=1; i<node->children.nodes[1]->children.size; ++i) {
      PVIPNode* n = node->children.nodes[1]->children.nodes[i];
      int reg = REG_OBJ();
      MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, n->pv->buf, n->pv->len);
      int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
      ASM_PARAM_RP_O(reg, i);
      ASM_BINDLEX(lex, 0, reg);
      // ++i; // <-- really?
    }
  }

  bool returned = false;
  {
    const PVIPNode*stmts = node->children.nodes[2];
    assert(stmts->type == PVIP_NODE_STATEMENTS);
    int i;
    for (i=0; i<stmts->children.size ; ++i) {
      PVIPNode * n=stmts->children.nodes[i];
      int reg = Kiji_compiler_do_compile(self, n);
      if (i==stmts->children.size-1 && reg >= 0) {
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
  Kiji_compiler_pop_frame(self);

  ASM_WRITE_UINT16_T(frame_no, func_pos);

  // bind method object to class how
  if (!self->current_class_how) {
      MVM_panic(MVM_exitcode_compunit, "Compilation error. You can't write methods outside of class definition");
  }
  {
    MVMString * methname = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, name->buf, name->len);
    // self, type_obj, name, method
    MVMObject * method_table = ((MVMKnowHOWREPR *)self->current_class_how)->body.methods;
    MVMObject* code_type = self->tc->instance->boot_types->BOOTCode;
    MVMCode *coderef = (MVMCode*)REPR(code_type)->allocate(self->tc, STABLE(code_type));
    coderef->body.sf = self->cu->frames[frame_no];
    REPR(method_table)->ass_funcs->bind_key_boxed(self->tc, STABLE(method_table),
        method_table, OBJECT_BODY(method_table), (MVMObject *)methname, (MVMObject*)coderef);
  }

  return funcreg;
}

ND(NODE_FUNC) {
  PVIPNode * name_node = node->children.nodes[0];
  const PVIPString * const name = name_node->pv;

  MVMuint16 funcreg = REG_OBJ();
  PVIPString * amp_name = PVIP_string_new();
  PVIP_string_concat(amp_name, "&", strlen("&"));
  PVIP_string_concat(amp_name, name->buf, name->len);
  MVMString * mvm_name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, amp_name->buf, amp_name->len);
  int funclex = Kiji_compiler_push_lexical(self, mvm_name, MVM_reg_obj);
  int func_pos = Kiji_compiler_bytecode_size(self) + 2 + 2;
  ASM_GETCODE(funcreg, 0);
  ASM_BINDLEX(
      funclex,
      0, // frame outer count
      funcreg
  );

  // Compile function body
  int frame_no = Kiji_compiler_push_frame(self, name->buf, name->len);

  if (node->children.nodes[2]->type == PVIP_NODE_EXPORTABLE) {
    Kiji_frame_add_exportable(
      Kiji_compiler_top_frame(self),
      self->tc,
      mvm_name,
      frame_no
    );
  }

  // TODO process named params
  // TODO process types
  {
    /* guess minimum argument count */
    ASM_CHECKARITY(
        Kiji_compiler_count_min_arity(self, node->children.nodes[1]),
        node->children.nodes[1]->children.size
    );

    /* expand params */
    int i;
    for (i=0; i<node->children.nodes[1]->children.size; ++i) {
      PVIPNode* t = node->children.nodes[1]->children.nodes[i];
      PVIPNode* n = t->children.nodes[1];
      PVIPNode* defaults = t->children.nodes[2];
      int reg = REG_OBJ();
      MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, n->pv->buf, n->pv->len);
      int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
      if (defaults->type == PVIP_NODE_NOP) { /* no default value */
        ASM_PARAM_RP_O(reg, i);
        ASM_BINDLEX(lex, 0, reg);
      } else { /* optional argument */
        /*
          *   param_rp_o reg, i, label_jmp
          *   set reg, to_o(do_compile())
          * label_jmp:
          *   bindlex lex, 0, reg
          */
        LABEL(jmp);

        Kiji_label_reserve(&jmp, Kiji_compiler_bytecode_size(self)+1+1+2+2);
        ASM_PARAM_OP_O(reg, i, 0);

        ASM_SET(reg, Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, defaults)));

        Kiji_label_put(&jmp, self);

        ASM_BINDLEX(lex, 0, reg);
      }
    }
  }
  /* Kiji_asm_dump_frame(&(Kiji_compiler_top_frame(self)->frame)); */

  bool returned = false;
  {
    const PVIPNode*stmts = node->children.nodes[3];
    assert(stmts->type == PVIP_NODE_STATEMENTS);
    int i;
    for (i=0; i<stmts->children.size ; ++i) {
      PVIPNode * n=stmts->children.nodes[i];
      int reg = Kiji_compiler_do_compile(self, n);
      if (i==stmts->children.size-1 && reg >= 0) {
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
  Kiji_compiler_pop_frame(self);

  ASM_WRITE_UINT16_T(frame_no, func_pos);

  return funcreg;
}

ND(NODE_IT_METHODCALL) {
  PVIPNode * node_it = PVIP_node_new_string(PVIP_NODE_VARIABLE, "$_", 2);
  PVIPNode * call = PVIP_node_new_children2(PVIP_NODE_METHODCALL, node_it, node->children.nodes[0]);
  if (node->children.size == 2) {
    PVIP_node_push_child(call, node->children.nodes[1]);
  }
  /* TODO possibly memory leaks */
  return Kiji_compiler_do_compile(self, call);
}

ND(NODE_METHODCALL) {
  assert(node->children.size == 3 || node->children.size==2);
  MVMuint16 obj = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  int str = Kiji_compiler_push_string(self, newMVMStringFromPVIP(node->children.nodes[1]->pv));
  MVMuint16 meth = REG_OBJ();
  MVMuint16 ret = REG_OBJ();

  ASM_FINDMETH(meth, obj, str);
  ASM_ARG_O(0, obj);

  if (node->children.size == 3) {
    PVIPNode* args = node->children.nodes[2];

    MVMCallsite* callsite;
    Newxz(callsite, 1, MVMCallsite);
    callsite->arg_count = 1+args->children.size;
    callsite->num_pos = 1+args->children.size;
    Newxz(callsite->arg_flags, 1+args->children.size, MVMCallsiteEntry);
    callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
    int i=1;
    int j;
    for (j=0; j<args->children.size; j++) {
      PVIPNode* a= args->children.nodes[j];
      callsite->arg_flags[i] = MVM_CALLSITE_ARG_OBJ;
      ASM_ARG_O(i, Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, a)));
      ++i;
    }
    auto callsite_no = Kiji_compiler_push_callsite(self, callsite);
    ASM_PREPARGS(callsite_no);
  } else {
    MVMCallsite* callsite;
    Newxz(callsite, 1, MVMCallsite);
    callsite->arg_count = 1;
    callsite->num_pos = 1;
    Newxz(callsite->arg_flags, 1, MVMCallsiteEntry);
    callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
    int callsite_no = Kiji_compiler_push_callsite(self, callsite);
    ASM_PREPARGS(callsite_no);
  }

  ASM_INVOKE_O(ret, meth);
  return ret;
}
