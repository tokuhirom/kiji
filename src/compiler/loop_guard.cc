extern "C" {
#include "moarvm.h"
}
#include "../compiler.h"
#include <list>
#include <string>
#include "loop_guard.h"
#include <vector>

// fixme: `put` is not the best verb in English here.
void KijiLoopGuard::put_last() {
    last_offset_ = Kiji_compiler_bytecode_size(compiler_)-1+1;
}
void KijiLoopGuard::put_redo() {
    redo_offset_ = Kiji_compiler_bytecode_size(compiler_)-1+1;
}
void KijiLoopGuard::put_next() {
    next_offset_ = Kiji_compiler_bytecode_size(compiler_)-1+1;
}
KijiLoopGuard::~KijiLoopGuard() {
    MVMuint32 end_offset = Kiji_compiler_bytecode_size(compiler_)-1;

    MVMFrameHandler *last_handler = new MVMFrameHandler;
    last_handler->start_offset = start_offset_;
    last_handler->end_offset = end_offset;
    last_handler->category_mask = MVM_EX_CAT_LAST;
    last_handler->action = MVM_EX_ACTION_GOTO;
    last_handler->block_reg = 0;
    last_handler->goto_offset = last_offset_;
    Kiji_frame_push_handler(Kiji_compiler_top_frame(compiler_), last_handler);

    MVMFrameHandler *next_handler = new MVMFrameHandler;
    next_handler->start_offset = start_offset_;
    next_handler->end_offset = end_offset;
    next_handler->category_mask = MVM_EX_CAT_NEXT;
    next_handler->action = MVM_EX_ACTION_GOTO;
    next_handler->block_reg = 0;
    next_handler->goto_offset = next_offset_;
    Kiji_frame_push_handler(Kiji_compiler_top_frame(compiler_), next_handler);

    MVMFrameHandler *redo_handler = new MVMFrameHandler;
    redo_handler->start_offset = start_offset_;
    redo_handler->end_offset = end_offset;
    redo_handler->category_mask = MVM_EX_CAT_REDO;
    redo_handler->action = MVM_EX_ACTION_GOTO;
    redo_handler->block_reg = 0;
    redo_handler->goto_offset = redo_offset_;
    Kiji_frame_push_handler(Kiji_compiler_top_frame(compiler_), redo_handler);
}
KijiLoopGuard::KijiLoopGuard(KijiCompiler *compiler) :compiler_(compiler) {
    start_offset_ = Kiji_compiler_bytecode_size(compiler_)-1;
}

