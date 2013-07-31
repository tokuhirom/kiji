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

ND(NODE_FUNCALL) {
  assert(node->children.size == 2);
  const PVIPNode*ident = node->children.nodes[0];
  const PVIPNode*args  = node->children.nodes[1];
  assert(args->type == PVIP_NODE_ARGS);
  
  if (node->children.nodes[0]->type == PVIP_NODE_IDENT) {
#define PVIP_string_cmp(n, c) ((n)->len==strlen((c)) && !memcmp(n->buf,c,strlen(c)))
    if (PVIP_string_cmp(ident->pv, "say")) {
      int i;
      for (i=0; i<args->children.size; i++) {
        PVIPNode* a = args->children.nodes[i];
        uint16_t reg_num = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, a));
        if (i==args->children.size-1) {
          ASM_SAY(reg_num);
        } else {
          ASM_PRINT(reg_num);
        }
      }
      return Kiji_compiler_const_true(self);
    } else if (PVIP_string_cmp(ident->pv, "print")) {
      int i;
      for (i=0; i<args->children.size; i++) {
        PVIPNode* a = args->children.nodes[i];
        uint16_t reg_num = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, a));
        ASM_PRINT(reg_num);
      }
      return Kiji_compiler_const_true(self);
    } else if (PVIP_string_cmp(ident->pv, "open")) {
      // TODO support arguments
      assert(args->children.size == 1);
      int fname_s = Kiji_compiler_do_compile(self, args->children.nodes[0]);
      MVMuint16 dst_reg_o = REG_OBJ();
      // TODO support latin1, etc.
      int mode = Kiji_compiler_push_string(self, newMVMString_nolen("r"));
      MVMuint16 mode_s = REG_STR();
      ASM_CONST_S(mode_s, mode);
      ASM_OPEN_FH(dst_reg_o, fname_s, mode_s);
      return dst_reg_o;
    } else if (PVIP_string_cmp(ident->pv, "slurp")) {
      assert(args->children.size <= 2);
      assert(args->children.size != 2 && "Encoding option is not supported yet");
      MVMuint16 fname_s = Kiji_compiler_do_compile(self, args->children.nodes[0]);
      MVMuint16 dst_reg_s = REG_STR();
      MVMuint16 encoding_s = REG_STR();
      ASM_CONST_S(encoding_s, Kiji_compiler_push_string(self, newMVMString_nolen("utf8"))); // TODO support latin1, etc.
      ASM_SLURP(dst_reg_s, fname_s, encoding_s);
      return dst_reg_s;
    }
  }

  {
    uint16_t func_reg_no;
    if (node->children.nodes[0]->type == PVIP_NODE_IDENT) {
      func_reg_no = REG_OBJ();
      int lex_no, outer;
      PVIPString *amp_name = PVIP_string_new();
      PVIP_string_concat(amp_name, "&", strlen("&"));
      PVIP_string_concat(amp_name, ident->pv->buf, ident->pv->len);
      if (!Kiji_compiler_find_lexical_by_name(self, newMVMStringFromPVIP(amp_name), &lex_no, &outer)) {
        PVIP_string_concat(amp_name, "\0", 1);
        MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", amp_name->buf);
      }
      PVIP_string_destroy(amp_name);
      ASM_GETLEX(
        func_reg_no,
        lex_no,
        outer // outer frame
      );
    } else {
      func_reg_no = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    }

    {
      MVMCallsite* callsite;
      Newxz(callsite, 1, MVMCallsite);
      // TODO support named params
      callsite->arg_count = args->children.size;
      callsite->num_pos   = args->children.size;
      Newxz(callsite->arg_flags, args->children.size, MVMCallsiteEntry);

      uint16_t *arg_regs  = NULL;
      size_t num_arg_regs = 0;

      int i;
      for (i=0; i<args->children.size; i++) {
        PVIPNode *a = args->children.nodes[i];
        int reg = Kiji_compiler_do_compile(self, a);
        if (reg<0) {
          MVM_panic(MVM_exitcode_compunit, "Compilation error. You should not pass void function as an argument: %s", PVIP_node_name(a->type));
        }

        switch (Kiji_compiler_get_local_type(self, reg)) {
        case MVM_reg_int64:
          callsite->arg_flags[i] = MVM_CALLSITE_ARG_INT;
          break;
        case MVM_reg_num64:
          callsite->arg_flags[i] = MVM_CALLSITE_ARG_NUM;
          break;
        case MVM_reg_str:
          callsite->arg_flags[i] = MVM_CALLSITE_ARG_STR;
          break;
        case MVM_reg_obj:
          callsite->arg_flags[i] = MVM_CALLSITE_ARG_OBJ;
          break;
        default:
          abort();
        }

        num_arg_regs++;
        Renew(arg_regs, num_arg_regs, uint16_t);
        arg_regs[num_arg_regs-1] = reg;
      }

      int callsite_no = Kiji_compiler_push_callsite(self, callsite);
      ASM_PREPARGS(callsite_no);

      for (i=0; i<num_arg_regs; ++i) {
        uint16_t reg = arg_regs[i];
        switch (Kiji_compiler_get_local_type(self, reg)) {
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
      }
    }
    MVMuint16 dest_reg = REG_OBJ(); // ctx
    ASM_INVOKE_O(
        dest_reg,
        func_reg_no
    );
    return dest_reg;
  }
}
