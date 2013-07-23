#pragma once

class KijiLoopGuard {
private:
  KijiCompiler *compiler_;
  MVMuint32 start_offset_;
  MVMuint32 last_offset_;
  MVMuint32 next_offset_;
  MVMuint32 redo_offset_;
public:
  KijiLoopGuard(KijiCompiler *compiler);
  ~KijiLoopGuard();
  // fixme: `put` is not the best verb in English here.
  void put_last();
  void put_redo();
  void put_next();
};

