#ifndef KIJI_LOOP_H_
#define KIJI_LOOP_H_

#include "../compiler.h"

#define LOOP_ENTER \
    do { \
        KijiLoop _loop; \
        _loop.start_offset = Kiji_compiler_bytecode_size(self)-1;

#define LOOP_NEXT \
     _loop.next_offset = Kiji_compiler_bytecode_size(self)-1+1

#define LOOP_LAST \
    _loop.last_offset = Kiji_compiler_bytecode_size(self)-1+1

#define LOOP_REDO \
    _loop.redo_offset = Kiji_compiler_bytecode_size(self)-1+1

#define LOOP_LEAVE \
        Kiji_loop_finalize(&_loop, self); \
    } while(0)

typedef struct {
  MVMuint32 start_offset;
  MVMuint32 last_offset;
  MVMuint32 next_offset;
  MVMuint32 redo_offset;
} KijiLoop;

#ifdef __cplusplus
extern "C" {
#endif
void Kiji_loop_finalize(KijiLoop *self, KijiCompiler *compiler);
#ifdef __cplusplus
}
#endif

#endif /* KIJI_LOOP_H_ */
