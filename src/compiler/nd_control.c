/* vim:ts=2:sw=2:tw=0:
 */

#include "moarvm.h"
#include "nd.h"
#include "../handy.h"

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

  int frame_no = Kiji_compiler_push_frame(self, path, path_len);
  Safefree(path);
  ASM_CHECKARITY(0,0);
  Kiji_compiler_do_compile(self, root_node);
  ASM_RETURN();
  Kiji_compiler_pop_frame(self);

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
