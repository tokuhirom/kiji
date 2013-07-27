#ifndef KIJI_COMPILER_ND_H_
#define KIJI_COMPILER_ND_H_

#include "pvip.h"
#include "../compiler.h"
#include "../gen.assembler.h"

#define ND(name) \
  int Kiji_compiler_nd_ ## name (KijiCompiler *self, const PVIPNode *node)

#endif /* KIJI_COMPILER_ND_H_ */
