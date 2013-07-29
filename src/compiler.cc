/* vim:ts=2:sw=2:tw=0:
 */

extern "C" {
#include "moarvm.h"
}
#include "compiler.h"
#include "builtin.h"
#include "asm.h"
#include "compiler/loop.h"
#include "gen.assembler.h"
#include "compiler/gen.nd.h"

#include <list>
#include <string>
#include <vector>

#define MEMORY_ERROR() \
          MVM_panic(MVM_exitcode_compunit, "Compilation error. return with non-value.");

#define ASM_BYTECODE_SIZE() \
  Kiji_compiler_bytecode_size(self)

// This reg returns register number contains true value.
int Kiji_compiler_do_compile(KijiCompiler *self, const PVIPNode*node) {
  MVMThreadContext *tc = self->tc;
  int ret = Kiji_compiler_compile_nodes(self, node);
  if (ret != -18185963) {
    return ret;
  }

  // printf("node: %s\n", node.type_name());
  switch (node->type) {
  case PVIP_NODE_POSTINC: { // $i++
    assert(node->children.size == 1);
    if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
      MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
    }
    auto reg_no = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
    auto i_tmp = Kiji_compiler_to_i(self, reg_no);
    ASM_INC_I(i_tmp);
    auto dst_reg = Kiji_compiler_to_o(self, i_tmp);
    Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), dst_reg);
    return reg_no;
  }
  case PVIP_NODE_PREINC: { // ++$i
    assert(node->children.size == 1);
    if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
      MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
    }
    auto reg_no = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
    auto i_tmp = Kiji_compiler_to_i(self, reg_no);
    ASM_INC_I(i_tmp);
    auto dst_reg = Kiji_compiler_to_o(self, i_tmp);
    Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), dst_reg);
    return dst_reg;
  }
  case PVIP_NODE_PREDEC: { // --$i
    assert(node->children.size == 1);
    if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {
      MVM_panic(MVM_exitcode_compunit, "The argument for postinc operator is not a variable");
    }
    auto reg_no = Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv));
    auto i_tmp = Kiji_compiler_to_i(self, reg_no);
    ASM_DEC_I(i_tmp);
    auto dst_reg = Kiji_compiler_to_o(self, i_tmp);
    Kiji_compiler_set_variable(self, newMVMStringFromPVIP(node->children.nodes[0]->pv), dst_reg);
    return dst_reg;
  }
  case PVIP_NODE_UNARY_BITWISE_NEGATION: { // +^1
    auto reg = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    ASM_BNOT_I(reg, reg);
    return reg;
  }
  case PVIP_NODE_BRSHIFT: { // +>
    auto l = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    auto r = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
    ASM_BRSHIFT_I(r, l, r);
    return r;
  }
  case PVIP_NODE_BLSHIFT: { // +<
    auto l = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    auto r = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
    ASM_BLSHIFT_I(r, l, r);
    return r;
  }
  case PVIP_NODE_ABS: {
    // TODO support abs_n?
    auto r = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
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
    auto reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
    if (reg < 0) {
      MEMORY_ERROR();
    }
    // ASM_RETURN_O(Kiji_compiler_to_o(self, reg));
    switch (Kiji_compiler_get_local_type(self, reg)) {
    case MVM_reg_int64:
      ASM_RETURN_I(reg);
      break;
    case MVM_reg_str:
      ASM_RETURN_S(reg);
      break;
    case MVM_reg_obj:
      ASM_RETURN_O(Kiji_compiler_to_o(self, reg));
      break;
    case MVM_reg_num64:
      ASM_RETURN_N(reg);
      break;
    default:
      MVM_panic(MVM_exitcode_compunit, "Compilation error. Unknown register for returning: %d", Kiji_compiler_get_local_type(self, reg));
    }
    return UNKNOWN_REG;
  }
  case PVIP_NODE_MODULE: {
    // nop, for now.
    return UNKNOWN_REG;
  }
  case PVIP_NODE_DIE: {
    int msg_reg = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
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

    LOOP_ENTER;

    LABEL(label_while); LABEL_PUT(label_while);

    LOOP_NEXT;
      int reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
      assert(reg != UNKNOWN_REG);
      LABEL(label_end);
      Kiji_compiler_unless_any(self, reg, &label_end);
      Kiji_compiler_do_compile(self, node->children.nodes[1]);
      Kiji_compiler_goto(self, &label_while);
    LABEL_PUT(label_end);

    LOOP_LAST;

    LOOP_LEAVE;

    return UNKNOWN_REG;
  }
  case PVIP_NODE_LAMBDA: {
    auto frame_no = Kiji_compiler_push_frame(self, "lambda", strlen("lambda"));
    ASM_CHECKARITY(
      node->children.nodes[0]->children.size,
      node->children.nodes[0]->children.size
    );
    /* Compile parameters */
    for (int i=0; i<node->children.nodes[0]->children.size; i++) {
      PVIPNode *n = node->children.nodes[0]->children.nodes[i]->children.nodes[1];
      int reg = REG_OBJ();
      MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, n->pv->buf, n->pv->len);
      int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
      ASM_PARAM_RP_O(reg, i);
      ASM_BINDLEX(lex, 0, reg);
    }
    auto retval = Kiji_compiler_do_compile(self, node->children.nodes[1]);
    if (retval == UNKNOWN_REG) {
      retval = REG_OBJ();
      ASM_NULL(retval);
    }
    Kiji_compiler_return_any(self, retval);
    Kiji_compiler_pop_frame(self);

    // warn if void context.
    auto dst_reg = REG_OBJ();
    ASM_GETCODE(dst_reg, frame_no);

    return dst_reg;
  }
  case PVIP_NODE_BLOCK: {
    auto frame_no = Kiji_compiler_push_frame(self, "block", strlen("block"));
    ASM_CHECKARITY(0,0);
    for (int i=0; i<node->children.size; i++) {
      PVIPNode *n = node->children.nodes[i];
      (void)Kiji_compiler_do_compile(self, n);
    }
    ASM_RETURN();
    Kiji_compiler_pop_frame(self);

    auto frame_reg = REG_OBJ();
    ASM_GETCODE(frame_reg, frame_no);

    MVMCallsite* callsite = new MVMCallsite;
    memset(callsite, 0, sizeof(MVMCallsite));
    callsite->arg_count = 0;
    callsite->num_pos = 0;
    callsite->arg_flags = NULL;
    auto callsite_no = Kiji_compiler_push_callsite(self, callsite);
    ASM_PREPARGS(callsite_no);

    ASM_INVOKE_V(frame_reg); // trash result

    return UNKNOWN_REG;
  }
  case PVIP_NODE_STRING: {
    int str_num = Kiji_compiler_push_string(self, newMVMStringFromPVIP(node->pv));
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
      int lex_no = Kiji_compiler_do_compile(self, lhs);
      int val    = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, rhs));
      ASM_BINDLEX(
        lex_no, // lex number
        0,      // frame outer count
        val     // value
      );
      return val;
    } else if (lhs->type == PVIP_NODE_OUR) {
      // our $var := foo;
      assert(lhs->children.nodes[0]->type == PVIP_NODE_VARIABLE);
      MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, lhs->children.nodes[0]->pv->buf, lhs->children.nodes[0]->pv->len);
      Kiji_compiler_push_pkg_var(self, name);
      auto varname = Kiji_compiler_push_string(self, newMVMStringFromPVIP(lhs->children.nodes[0]->pv));
      int val    = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, rhs));
      int outer = 0;
      int lex_no = 0;
      if (!Kiji_compiler_find_lexical_by_name(self, newMVMString_nolen("$?PACKAGE"), &lex_no, &outer)) {
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
      int val    = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, rhs));
      Kiji_compiler_set_variable(self, newMVMStringFromPVIP(lhs->pv), val);
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
    std::string amp_name(std::string("&") + name);
    MVMString * mvm_name = MVM_string_utf8_decode(tc, tc->instance->VMString, amp_name.c_str(), amp_name.size());
    auto funclex = Kiji_compiler_push_lexical(self, mvm_name, MVM_reg_obj);
    auto func_pos = ASM_BYTECODE_SIZE() + 2 + 2;
    ASM_GETCODE(funcreg, 0);
    ASM_BINDLEX(
        funclex,
        0, // frame outer count
        funcreg
    );

    // Compile function body
    auto frame_no = Kiji_compiler_push_frame(self, name.c_str(), name.size());

    // TODO process named params
    // TODO process types
    {
      ASM_CHECKARITY(
          node->children.nodes[1]->children.size+1,
          node->children.nodes[1]->children.size+1
      );

      {
        // push self
        MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, "__self", strlen("__self"));
        int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
        int reg = REG_OBJ();
        ASM_PARAM_RP_O(reg, 0);
        ASM_BINDLEX(lex, 0, reg);
      }

      for (int i=1; i<node->children.nodes[1]->children.size; ++i) {
        auto n = node->children.nodes[1]->children.nodes[i];
        int reg = REG_OBJ();
        MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, n->pv->buf, n->pv->len);
        int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
        ASM_PARAM_RP_O(reg, i);
        ASM_BINDLEX(lex, 0, reg);
        ++i; // <- really??
      }
    }

    bool returned = false;
    {
      const PVIPNode*stmts = node->children.nodes[2];
      assert(stmts->type == PVIP_NODE_STATEMENTS);
      for (int i=0; i<stmts->children.size ; ++i) {
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
      MVMString * methname = MVM_string_utf8_decode(tc, tc->instance->VMString, name.c_str(), name.size());
      // self, type_obj, name, method
      MVMObject * method_table = ((MVMKnowHOWREPR *)self->current_class_how)->body.methods;
      MVMObject* code_type = tc->instance->boot_types->BOOTCode;
      MVMCode *coderef = (MVMCode*)REPR(code_type)->allocate(tc, STABLE(code_type));
      coderef->body.sf = self->cu->frames[frame_no];
      REPR(method_table)->ass_funcs->bind_key_boxed(tc, STABLE(method_table),
          method_table, OBJECT_BODY(method_table), (MVMObject *)methname, (MVMObject*)coderef);
    }

    return funcreg;
  }
  case PVIP_NODE_FUNC: {
    PVIPNode * name_node = node->children.nodes[0];
    std::string name(name_node->pv->buf, name_node->pv->len);

    auto funcreg = REG_OBJ();
    std::string amp_name(std::string("&") + name);
    MVMString * mvm_name = MVM_string_utf8_decode(tc, tc->instance->VMString, amp_name.c_str(), amp_name.size());
    auto funclex = Kiji_compiler_push_lexical(self, mvm_name, MVM_reg_obj);
    auto func_pos = ASM_BYTECODE_SIZE() + 2 + 2;
    ASM_GETCODE(funcreg, 0);
    ASM_BINDLEX(
        funclex,
        0, // frame outer count
        funcreg
    );

    // Compile function body
    auto frame_no = Kiji_compiler_push_frame(self, name.c_str(), name.size());

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
      ASM_CHECKARITY(
          node->children.nodes[1]->children.size,
          node->children.nodes[1]->children.size
      );

      for (int i=0; i<node->children.nodes[1]->children.size; ++i) {
        PVIPNode* n = node->children.nodes[1]->children.nodes[i]->children.nodes[1];
        int reg = REG_OBJ();
        MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, n->pv->buf, n->pv->len);
        int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
        ASM_PARAM_RP_O(reg, i);
        ASM_BINDLEX(lex, 0, reg);
        ++i;
      }
    }

    bool returned = false;
    {
      const PVIPNode*stmts = node->children.nodes[3];
      assert(stmts->type == PVIP_NODE_STATEMENTS);
      for (int i=0; i<stmts->children.size ; ++i) {
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
  case PVIP_NODE_VARIABLE: {
    // copy lexical variable to register
    return Kiji_compiler_get_variable(self, newMVMStringFromPVIP(node->pv));
  }
  case PVIP_NODE_CLARGS: { // @*ARGS
    auto retval = REG_OBJ();
    ASM_WVAL(retval, 0,0);
    return retval;
  }
  case PVIP_NODE_CLASS: {
    return Kiji_compiler_compile_class(self, node);
  }
  case PVIP_NODE_FOR: {
    //   init_iter
    // label_for:
    //   body
    //   shift iter
    //   if_o label_for
    // label_end:

    LOOP_ENTER;
      auto src_reg = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
      auto iter_reg = REG_OBJ();
      LABEL(label_end);
      ASM_ITER(iter_reg, src_reg);
      Kiji_compiler_unless_any(self, iter_reg, &label_end);

    LABEL(label_for); LABEL_PUT(label_for);
    LOOP_NEXT;

      auto val = REG_OBJ();
      ASM_SHIFT_O(val, iter_reg);

      if (node->children.nodes[1]->type == PVIP_NODE_LAMBDA) {
        auto body = Kiji_compiler_do_compile(self, node->children.nodes[1]);
        MVMCallsite* callsite = new MVMCallsite;
        memset(callsite, 0, sizeof(MVMCallsite));
        callsite->arg_count = 1;
        callsite->num_pos = 1;
        callsite->arg_flags = new MVMCallsiteEntry[1];
        callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
        auto callsite_no = Kiji_compiler_push_callsite(self, callsite);
        ASM_PREPARGS(callsite_no);
        ASM_ARG_O(0, val);
        ASM_INVOKE_V(body);
      } else {
        MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, "$_", strlen("$_"));
        int it = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
        ASM_BINDLEX(it, 0, val);
        Kiji_compiler_do_compile(self, node->children.nodes[1]);
      }

      Kiji_compiler_if_any(self, iter_reg, &label_for);

    LABEL_PUT(label_end);
    LOOP_LAST;

    LOOP_LEAVE;

    return UNKNOWN_REG;
  }
  case PVIP_NODE_OUR: {
    if (node->children.size != 1) {
      printf("NOT IMPLEMENTED\n");
      abort();
    }
    auto n = node->children.nodes[0];
    if (n->type != PVIP_NODE_VARIABLE) {
      printf("self is variable: %s\n", PVIP_node_name(n->type));
      exit(1);
    }
    MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, n->pv->buf, n->pv->len);
    Kiji_compiler_push_pkg_var(self, name);
    return UNKNOWN_REG;
  }
  case PVIP_NODE_MY: {
    if (node->children.size != 1) {
      printf("NOT IMPLEMENTED\n");
      abort();
    }
    auto n = node->children.nodes[0];
    if (n->type != PVIP_NODE_VARIABLE) {
      printf("self is variable: %s\n", PVIP_node_name(n->type));
      exit(1);
    }
    MVMString * name = MVM_string_utf8_decode(tc, tc->instance->VMString, n->pv->buf, n->pv->len);
    int idx = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
    return idx;
  }
  case PVIP_NODE_UNLESS: {
    //   cond
    //   if_o label_end
    //   body
    // label_end:
    auto cond_reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
    LABEL(label_end);
    Kiji_compiler_if_any(self, cond_reg, &label_end);
    auto dst_reg = Kiji_compiler_do_compile(self, node->children.nodes[1]);
    LABEL_PUT(label_end);
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

    auto if_cond_reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
    auto if_body = node->children.nodes[1];
    auto dst_reg = REG_OBJ();

    LABEL(label_if);
    Kiji_compiler_if_any(self, if_cond_reg, &label_if);

    // put else if conditions
    std::list<KijiLabel> elsif_poses;
    for (int i=2; i<node->children.size; ++i) {
      PVIPNode *n = node->children.nodes[i];
      if (n->type == PVIP_NODE_ELSE) {
        break;
      }
      auto reg = Kiji_compiler_do_compile(self, n->children.nodes[0]);
      elsif_poses.emplace_back();
      Kiji_label_init(&(elsif_poses.back()));
      Kiji_compiler_if_any(self, reg, &(elsif_poses.back()));
    }

    // compile else clause
    if (node->children.nodes[node->children.size-1]->type == PVIP_NODE_ELSE) {
      Kiji_compiler_compile_statements(self, node->children.nodes[node->children.size-1], dst_reg);
    }

    LABEL(label_end);
    Kiji_compiler_goto(self, &label_end);

    // update if_label and compile if body
    LABEL_PUT(label_if);
      Kiji_compiler_compile_statements(self, if_body, dst_reg);
      Kiji_compiler_goto(self, &label_end);

    // compile elsif body
    for (int i=2; i<node->children.size; i++) {
      PVIPNode*n = node->children.nodes[i];
      if (n->type == PVIP_NODE_ELSE) {
        break;
      }

      LABEL_PUT(elsif_poses.front());
      elsif_poses.pop_front();
      Kiji_compiler_compile_statements(self, n->children.nodes[1], dst_reg);
      Kiji_compiler_goto(self, &label_end);
    }
    assert(elsif_poses.size() == 0);

    LABEL_PUT(label_end);

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
    if (Kiji_compiler_find_lexical_by_name(self, newMVMStringFromPVIP(node->pv), &lex_no, &outer)) {
      auto reg_no = REG_OBJ();
      ASM_GETLEX(
        reg_no,
        lex_no,
        outer // outer frame
      );
      return reg_no;
    // sub fooo { }; foooo;
    } else if (Kiji_compiler_find_lexical_by_name(self, newMVMStringFromSTDSTRING(std::string("&") + std::string(node->pv->buf, node->pv->len)), &lex_no, &outer)) {
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

      auto callsite_no = Kiji_compiler_push_callsite(self, callsite);
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
      Kiji_compiler_do_compile(self, n);
    }
    return UNKNOWN_REG;
  case PVIP_NODE_STRING_CONCAT: {
    auto dst_reg = REG_STR();
    auto lhs = node->children.nodes[0];
    auto rhs = node->children.nodes[1];
    auto l = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, lhs));
    auto r = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, rhs));
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
      Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, lhs)),
      Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, rhs))
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
      Kiji_compiler_compile_array(self, array_reg, n);
    }
    return array_reg;
  }
  case PVIP_NODE_ATPOS: {
    assert(node->children.size == 2);
    auto container = Kiji_compiler_do_compile(self, node->children.nodes[0]);
    auto idx       = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
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
    return Kiji_compiler_do_compile(self, call);
  }
  case PVIP_NODE_METHODCALL: {
    assert(node->children.size == 3 || node->children.size==2);
    auto obj = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    auto str = Kiji_compiler_push_string(self, newMVMStringFromPVIP(node->children.nodes[1]->pv));
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
        ASM_ARG_O(i, Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, a)));
        ++i;
      }
      auto callsite_no = Kiji_compiler_push_callsite(self, callsite);
      ASM_PREPARGS(callsite_no);
    } else {
      MVMCallsite* callsite = new MVMCallsite;
      memset(callsite, 0, sizeof(MVMCallsite));
      callsite->arg_count = 1;
      callsite->num_pos = 1;
      callsite->arg_flags = new MVMCallsiteEntry[1];
      callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
      auto callsite_no = Kiji_compiler_push_callsite(self, callsite);
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
    LABEL(label_end);
    LABEL(label_else);
    auto dst_reg = REG_OBJ();

      auto cond_reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
      Kiji_compiler_unless_any(self, cond_reg, &label_else);

      auto if_reg = Kiji_compiler_do_compile(self, node->children.nodes[1]);
      ASM_SET(dst_reg, Kiji_compiler_to_o(self, if_reg));
      Kiji_compiler_goto(self, &label_end);

    LABEL_PUT(label_else);
      auto else_reg = Kiji_compiler_do_compile(self, node->children.nodes[2]);
      ASM_SET(dst_reg, Kiji_compiler_to_o(self, else_reg));

    LABEL_PUT(label_end);

    return dst_reg;
  }
  case PVIP_NODE_NOT: {
    auto src_reg = Kiji_compiler_to_i(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    auto dst_reg = REG_INT64();
    ASM_NOT_I(dst_reg, src_reg);
    return dst_reg;
  }
  case PVIP_NODE_NOP:
    return -1;
  case PVIP_NODE_ATKEY: {
    assert(node->children.size == 2);
    auto dst       = REG_OBJ();
    auto container = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    auto key       = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
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
      auto k = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, pair->children.nodes[0]));
      auto v = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, pair->children.nodes[1]));
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
    LABEL(label_both_false);
    LABEL(label_a1_true);
    LABEL(label_both_true);
    LABEL(label_end);

    auto dst_reg = REG_OBJ();

      auto arg1 = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
      auto arg2 = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
      Kiji_compiler_if_any(self, arg1, &label_a1_true);
      Kiji_compiler_unless_any(self, arg2, &label_both_false);
      ASM_SET(dst_reg, arg2);
      Kiji_compiler_goto(self, &label_end);
    LABEL_PUT(label_both_false);
      ASM_SET(dst_reg, arg1);
      Kiji_compiler_goto(self, &label_end);
    LABEL_PUT(label_a1_true); // a1:true, a2:unknown
      Kiji_compiler_if_any(self, arg2, &label_both_true);
      ASM_SET(dst_reg, arg1);
      Kiji_compiler_goto(self, &label_end);
    LABEL_PUT(label_both_true);
      ASM_NULL(dst_reg);
      Kiji_compiler_goto(self, &label_end);
    LABEL_PUT(label_end);
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
    LABEL(label_end);
    LABEL(label_a1);
    auto dst_reg = REG_OBJ();
      auto arg1 = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
      Kiji_compiler_if_any(self, arg1, &label_a1);
      auto arg2 = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
      ASM_SET(dst_reg, arg2);
      Kiji_compiler_goto(self, &label_end);
    LABEL_PUT(label_a1);
      ASM_SET(dst_reg, arg1);
    LABEL_PUT(label_end);
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
    LABEL(label_end);
    LABEL(label_a1);
    auto dst_reg = REG_OBJ();
      auto arg1 = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
      Kiji_compiler_unless_any(self, arg1, &label_a1);
      auto arg2 = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[1]));
      ASM_SET(dst_reg, arg2);
      Kiji_compiler_goto(self, &label_end);
    LABEL_PUT(label_a1);
      ASM_SET(dst_reg, arg1);
    LABEL_PUT(label_end);
    return dst_reg;
  }
  case PVIP_NODE_UNARY_PLUS: {
    return Kiji_compiler_to_n(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  }
  case PVIP_NODE_UNARY_MINUS: {
    auto reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
    if (Kiji_compiler_get_local_type(self, reg) == MVM_reg_int64) {
      ASM_NEG_I(reg, reg);
      return reg;
    } else {
      reg = Kiji_compiler_to_n(self, reg);
      ASM_NEG_N(reg, reg);
      return reg;
    }
  }
  case PVIP_NODE_CHAIN:
    if (node->children.size==1) {
      return Kiji_compiler_do_compile(self, node->children.nodes[0]);
    } else {
      return Kiji_compiler_compile_chained_comparisions(self, node);
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
          uint16_t reg_num = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, a));
          if (i==args->children.size-1) {
            ASM_SAY(reg_num);
          } else {
            ASM_PRINT(reg_num);
          }
        }
        return Kiji_compiler_const_true(self);
      } else if (std::string(ident->pv->buf, ident->pv->len) == "print") {
        for (int i=0; i<args->children.size; i++) {
          PVIPNode* a = args->children.nodes[i];
          uint16_t reg_num = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, a));
          ASM_PRINT(reg_num);
        }
        return Kiji_compiler_const_true(self);
      } else if (std::string(ident->pv->buf, ident->pv->len) == "open") {
        // TODO support arguments
        assert(args->children.size == 1);
        auto fname_s = Kiji_compiler_do_compile(self, args->children.nodes[0]);
        auto dst_reg_o = REG_OBJ();
        // TODO support latin1, etc.
        auto mode = Kiji_compiler_push_string(self, newMVMString_nolen("r"));
        auto mode_s = REG_STR();
        ASM_CONST_S(mode_s, mode);
        ASM_OPEN_FH(dst_reg_o, fname_s, mode_s);
        return dst_reg_o;
      } else if (std::string(ident->pv->buf, ident->pv->len) == "slurp") {
        assert(args->children.size <= 2);
        assert(args->children.size != 2 && "Encoding option is not supported yet");
        auto fname_s = Kiji_compiler_do_compile(self, args->children.nodes[0]);
        auto dst_reg_s = REG_STR();
        auto encoding_s = REG_STR();
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
        if (!Kiji_compiler_find_lexical_by_name(self, newMVMStringFromSTDSTRING(std::string("&") + std::string(ident->pv->buf, ident->pv->len)), &lex_no, &outer)) {
          MVM_panic(MVM_exitcode_compunit, "Unknown lexical variable in find_lexical_by_name: %s\n", (std::string("&") + std::string(ident->pv->buf, ident->pv->len)).c_str());
        }
        ASM_GETLEX(
          func_reg_no,
          lex_no,
          outer // outer frame
        );
      } else {
        func_reg_no = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
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
          auto reg = Kiji_compiler_do_compile(self, a);
          if (reg<0) {
            MVM_panic(MVM_exitcode_compunit, "Compilation error. You should not pass void function as an argument: %s", PVIP_node_name(a->type));
          }

          switch (Kiji_compiler_get_local_type(self, reg)) {
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

        auto callsite_no = Kiji_compiler_push_callsite(self, callsite);
        ASM_PREPARGS(callsite_no);

        int i=0;
        for (auto reg:arg_regs) {
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


