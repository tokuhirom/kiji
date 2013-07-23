/* vim:ts=2:sw=2:tw=0:
 */

extern "C" {
#include "moarvm.h"
}
#include "compiler.h"

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


  void KijiLabel::put() {
    assert(address_ == -1);
    address_ = compiler_->frames_.back()->frame.bytecode_size;

    // rewrite reserved addresses
    for (auto r: reserved_addresses_) {
      Kiji_asm_write_uint32_t_for(&(*(compiler_->frames_.back())), address_, r);
    }
    reserved_addresses_.empty();
  }

      // fixme: `put` is not the best verb in English here.
      void KijiLoopGuard::put_last() {
        last_offset_ = compiler_->ASM_BYTECODE_SIZE()-1+1;
      }
      void KijiLoopGuard::put_redo() {
        redo_offset_ = compiler_->ASM_BYTECODE_SIZE()-1+1;
      }
      void KijiLoopGuard::put_next() {
        next_offset_ = compiler_->ASM_BYTECODE_SIZE()-1+1;
      }
KijiLoopGuard::~KijiLoopGuard() {
        MVMuint32 end_offset = compiler_->ASM_BYTECODE_SIZE()-1;

        MVMFrameHandler *last_handler = new MVMFrameHandler;
        last_handler->start_offset = start_offset_;
        last_handler->end_offset = end_offset;
        last_handler->category_mask = MVM_EX_CAT_LAST;
        last_handler->action = MVM_EX_ACTION_GOTO;
        last_handler->block_reg = 0;
        last_handler->goto_offset = last_offset_;
        compiler_->push_handler(last_handler);

        MVMFrameHandler *next_handler = new MVMFrameHandler;
        next_handler->start_offset = start_offset_;
        next_handler->end_offset = end_offset;
        next_handler->category_mask = MVM_EX_CAT_NEXT;
        next_handler->action = MVM_EX_ACTION_GOTO;
        next_handler->block_reg = 0;
        next_handler->goto_offset = next_offset_;
        compiler_->push_handler(next_handler);

        MVMFrameHandler *redo_handler = new MVMFrameHandler;
        redo_handler->start_offset = start_offset_;
        redo_handler->end_offset = end_offset;
        redo_handler->category_mask = MVM_EX_CAT_REDO;
        redo_handler->action = MVM_EX_ACTION_GOTO;
        redo_handler->block_reg = 0;
        redo_handler->goto_offset = redo_offset_;
        compiler_->push_handler(redo_handler);
      }
      KijiLoopGuard::KijiLoopGuard(KijiCompiler *compiler) :compiler_(compiler) {
        start_offset_ = compiler_->ASM_BYTECODE_SIZE()-1;
      }

