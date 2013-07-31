/* vim:ts=2:sw=2:tw=0
 */

#include "moarvm.h"
#include "nd.h"
#include "../handy.h"
#include "loop.h"

ND(NODE_LAST) {
  // break from for, while, loop.
  MVMuint16 ret = REG_OBJ();
  ASM_THROWCATLEX(
    ret,
    MVM_EX_CAT_LAST
  );
  return ret;
}
ND(NODE_REDO) {
  // redo from for, while, loop.
  MVMuint16 ret = REG_OBJ();
  ASM_THROWCATLEX(
    ret,
    MVM_EX_CAT_REDO
  );
  return ret;
}
ND(NODE_NEXT) {
  // continue from for, while, loop.
  MVMuint16 ret = REG_OBJ();
  ASM_THROWCATLEX(
    ret,
    MVM_EX_CAT_NEXT
  );
  return ret;
}

ND(NODE_RETURN) {
  assert(node->children.size ==1);
  MVMuint16 reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
  if (reg < 0) {
    MVM_panic(MVM_exitcode_compunit, "Compilation error. return with non-value.");
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
ND(NODE_MODULE) {
  // nop, for now.
  return UNKNOWN_REG;
}

ND(NODE_DIE) {
  int msg_reg = Kiji_compiler_to_s(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
  int dst_reg = REG_OBJ();
  assert(msg_reg != UNKNOWN_REG);
  ASM_DIE(dst_reg, msg_reg);
  return UNKNOWN_REG;
}

ND(NODE_WHILE) {
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

ND(NODE_LAMBDA) {
  int frame_no = Kiji_compiler_push_frame(self, "lambda", strlen("lambda"));
  ASM_CHECKARITY(
    node->children.nodes[0]->children.size,
    node->children.nodes[0]->children.size
  );
  /* Compile parameters */
  int i;
  for (i=0; i<node->children.nodes[0]->children.size; i++) {
    PVIPNode *n = node->children.nodes[0]->children.nodes[i]->children.nodes[1];
    int reg = REG_OBJ();
    MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, n->pv->buf, n->pv->len);
    int lex = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
    ASM_PARAM_RP_O(reg, i);
    ASM_BINDLEX(lex, 0, reg);
  }
  int retval = Kiji_compiler_do_compile(self, node->children.nodes[1]);
  if (retval == UNKNOWN_REG) {
    retval = REG_OBJ();
    ASM_NULL(retval);
  }
  Kiji_compiler_return_any(self, retval);
  Kiji_compiler_pop_frame(self);

  // warn if void context.
  MVMuint16 dst_reg = REG_OBJ();
  ASM_GETCODE(dst_reg, frame_no);

  return dst_reg;
}

ND(NODE_BLOCK) {
  int frame_no = Kiji_compiler_push_frame(self, "block", strlen("block"));
  ASM_CHECKARITY(0,0);
  int i;
  for (i=0; i<node->children.size; ++i) {
    PVIPNode *n = node->children.nodes[i];
    (void)Kiji_compiler_do_compile(self, n);
  }
  ASM_RETURN();
  Kiji_compiler_pop_frame(self);

  MVMuint16 frame_reg = REG_OBJ();
  ASM_GETCODE(frame_reg, frame_no);

  MVMCallsite* callsite;
  Newxz(callsite, 1, MVMCallsite);
  callsite->arg_count = 0;
  callsite->num_pos = 0;
  callsite->arg_flags = NULL;
  int callsite_no = Kiji_compiler_push_callsite(self, callsite);
  ASM_PREPARGS(callsite_no);

  ASM_INVOKE_V(frame_reg); // trash result

  return UNKNOWN_REG;
}

ND(NODE_USE) {
  assert(node->children.size==1);
  PVIPString *name_pv = node->children.nodes[0]->pv;
  if (name_pv->len == 2 && !memcmp(name_pv->buf, "v6", 2)) {
    return UNKNOWN_REG; // nop.
  }
  char *path;
  size_t path_len = strlen("lib/") + name_pv->len + strlen(".p6");
  Newxz(path, path_len + 1, char);
  memcpy(path, "lib/", strlen("lib/"));
  memcpy(path+strlen("lib/"), name_pv->buf, name_pv->len);
  memcpy(path+strlen("lib/")+name_pv->len, ".p6", strlen(".p6"));
  memcpy(path+strlen("lib/")+name_pv->len+strlen(".p6"), "\0", 1);
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    MVM_panic(MVM_exitcode_compunit, "Cannot open file: %s", path);
  }
  PVIPString *error;
  PVIPNode* root_node = PVIP_parse_fp(fp, 0, &error);
  if (!root_node) {
    MVM_panic(MVM_exitcode_compunit, "Cannot parse: %s", error->buf);
  }

  /* Compile the library code. */
  int frame_no = Kiji_compiler_push_frame(self, path, path_len);
  self->frames[frame_no]->frame_type = KIJI_FRAME_TYPE_USE;
  Safefree(path);
  ASM_CHECKARITY(0,0);
  Kiji_compiler_do_compile(self, root_node);
  ASM_RETURN();
  KijiFrame * lib_frame = Kiji_compiler_top_frame(self);
  Kiji_compiler_pop_frame(self);

  int i;
  for (i=0; i<lib_frame->num_exportables; ++i) {
    KijiExportableEntry*e = lib_frame->exportables[i];
    MVMuint16 reg = REG_OBJ();
    int lex = Kiji_compiler_push_lexical(self, e->name, MVM_reg_obj);
    ASM_GETCODE(reg, e->frame_no);
    ASM_BINDLEX(
      lex,
      0,
      reg
    );
  }

  /* And call it */
  uint16_t code_reg = REG_OBJ();
  ASM_GETCODE(code_reg, frame_no);
  MVMCallsite* callsite;
  Newxz(callsite, 1, MVMCallsite);
  callsite->arg_count = 0;
  callsite->num_pos = 0;
  callsite->arg_flags = NULL;
  int callsite_no = Kiji_compiler_push_callsite(self, callsite);
  ASM_PREPARGS(callsite_no);
  ASM_INVOKE_V(code_reg);

  return UNKNOWN_REG;
}

ND(NODE_FOR) {
  //   init_iter
  // label_for:
  //   body
  //   shift iter
  //   if_o label_for
  // label_end:

  LOOP_ENTER;
    MVMuint16 src_reg = Kiji_compiler_to_o(self, Kiji_compiler_do_compile(self, node->children.nodes[0]));
    MVMuint16 iter_reg = REG_OBJ();
    LABEL(label_end);
    ASM_ITER(iter_reg, src_reg);
    Kiji_compiler_unless_any(self, iter_reg, &label_end);

  LABEL(label_for); LABEL_PUT(label_for);
  LOOP_NEXT;

    MVMuint16 val = REG_OBJ();
    ASM_SHIFT_O(val, iter_reg);

    if (node->children.nodes[1]->type == PVIP_NODE_LAMBDA) {
      int body = Kiji_compiler_do_compile(self, node->children.nodes[1]);
      MVMCallsite* callsite;
      Newxz(callsite, 1, MVMCallsite);
      callsite->arg_count = 1;
      callsite->num_pos = 1;
      Newxz(callsite->arg_flags, 1, MVMCallsiteEntry);
      callsite->arg_flags[0] = MVM_CALLSITE_ARG_OBJ;
      int callsite_no = Kiji_compiler_push_callsite(self, callsite);
      ASM_PREPARGS(callsite_no);
      ASM_ARG_O(0, val);
      ASM_INVOKE_V(body);
    } else {
      MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, "$_", strlen("$_"));
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

ND(NODE_OUR) {
  if (node->children.size != 1) {
    printf("NOT IMPLEMENTED\n");
    abort();
  }
  PVIPNode* n = node->children.nodes[0];
  if (n->type != PVIP_NODE_VARIABLE) {
    printf("self is variable: %s\n", PVIP_node_name(n->type));
    exit(1);
  }
  MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, n->pv->buf, n->pv->len);
  Kiji_compiler_push_pkg_var(self, name);
  return UNKNOWN_REG;
}

ND(NODE_MY) {
  if (node->children.size != 1) {
    printf("NOT IMPLEMENTED\n");
    abort();
  }
  PVIPNode* n = node->children.nodes[0];
  if (n->type != PVIP_NODE_VARIABLE) {
    printf("self is variable: %s\n", PVIP_node_name(n->type));
    exit(1);
  }
  MVMString * name = MVM_string_utf8_decode(self->tc, self->tc->instance->VMString, n->pv->buf, n->pv->len);
  int idx = Kiji_compiler_push_lexical(self, name, MVM_reg_obj);
  return idx;
}

ND(NODE_UNLESS) {
  //   cond
  //   if_o label_end
  //   body
  // label_end:
  int cond_reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);

  LABEL(label_end);
  Kiji_compiler_if_any(self, cond_reg, &label_end);
  int dst_reg = Kiji_compiler_do_compile(self, node->children.nodes[1]);
  LABEL_PUT(label_end);
  return dst_reg;
}

ND(NODE_IF) {
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

  int if_cond_reg = Kiji_compiler_do_compile(self, node->children.nodes[0]);
  PVIPNode* if_body = node->children.nodes[1];
  MVMuint16 dst_reg = REG_OBJ();

  LABEL(label_if);
  Kiji_compiler_if_any(self, if_cond_reg, &label_if);

  /* put else if conditions */
  KijiLabel* elsif_poses = NULL;
  size_t num_elsif_poses = 0;
  int i;
  for (i=2; i<node->children.size; ++i) {
    PVIPNode *n = node->children.nodes[i];
    if (n->type == PVIP_NODE_ELSE) {
      break;
    }
    int reg = Kiji_compiler_do_compile(self, n->children.nodes[0]);

    ++num_elsif_poses;
    Renew(elsif_poses, num_elsif_poses, KijiLabel);
    Kiji_label_init(&(elsif_poses[num_elsif_poses-1]));
    Kiji_compiler_if_any(self, reg, &(elsif_poses[num_elsif_poses-1]));
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
  int elsif_poses_i =0;
  for (i=2; i<node->children.size; ++i) {
    PVIPNode*n = node->children.nodes[i];
    if (n->type == PVIP_NODE_ELSE) {
      break;
    }

    LABEL_PUT(elsif_poses[elsif_poses_i++]);
    Kiji_compiler_compile_statements(self, n->children.nodes[1], dst_reg);
    Kiji_compiler_goto(self, &label_end);
  }
  assert(elsif_poses_i == num_elsif_poses);

  LABEL_PUT(label_end);

  return dst_reg;
}

ND(NODE_ELSIF) { abort(); }
ND(NODE_ELSE) { abort(); }
