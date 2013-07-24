#include "moarvm.h"
#include "../compiler.h"
#include "loop.h"

void Kiji_loop_finalize(KijiLoop *self, KijiCompiler *compiler) {
    MVMuint32 end_offset = Kiji_compiler_bytecode_size(compiler)-1;

    MVMFrameHandler *last_handler;
    Newxz(last_handler, 1, MVMFrameHandler);
    last_handler->start_offset = self->start_offset;
    last_handler->end_offset = end_offset;
    last_handler->category_mask = MVM_EX_CAT_LAST;
    last_handler->action = MVM_EX_ACTION_GOTO;
    last_handler->block_reg = 0;
    last_handler->goto_offset = self->last_offset;
    Kiji_frame_push_handler(Kiji_compiler_top_frame(compiler), last_handler);

    MVMFrameHandler *next_handler;
    Newxz(next_handler, 1, MVMFrameHandler);
    next_handler->start_offset = self->start_offset;
    next_handler->end_offset = end_offset;
    next_handler->category_mask = MVM_EX_CAT_NEXT;
    next_handler->action = MVM_EX_ACTION_GOTO;
    next_handler->block_reg = 0;
    next_handler->goto_offset = self->next_offset;
    Kiji_frame_push_handler(Kiji_compiler_top_frame(compiler), next_handler);

    MVMFrameHandler *redo_handler;
    Newxz(redo_handler, 1, MVMFrameHandler);
    redo_handler->start_offset = self->start_offset;
    redo_handler->end_offset = end_offset;
    redo_handler->category_mask = MVM_EX_CAT_REDO;
    redo_handler->action = MVM_EX_ACTION_GOTO;
    redo_handler->block_reg = 0;
    redo_handler->goto_offset = self->redo_offset;
    Kiji_frame_push_handler(Kiji_compiler_top_frame(compiler), redo_handler);
}

