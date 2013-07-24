// vim:ts=2:sw=2:tw=0:

#include <stdint.h>
#include "moarvm.h"
#include "handy.h"
#include "frame.h"
#include "asm.h"

#ifdef DEBUG_ASM
#define ASM_DEBUG(x) printf("ASM: %02X %02X\n", frame->frame.bytecode_size, x);
#else
#define ASM_DEBUG(x)
#endif

void Kiji_asm_write_uint8_t(KijiFrame *frame, MVMuint8 u8) {
  ASM_DEBUG(u8);
  frame->frame.bytecode_size++;
  Renew(frame->frame.bytecode, frame->frame.bytecode_size, MVMuint8);
  frame->frame.bytecode[frame->frame.bytecode_size-1] = u8;
}

static void Kiji_asm_write_uint8(KijiFrame *frame, MVMuint8 u8) {
  return Kiji_asm_write_uint8_t(frame, u8);
}

void Kiji_asm_write_u16(KijiFrame *frame, MVMuint16 i) {
  Kiji_asm_write_uint8(frame, (i>>0)  &0xffff);
  Kiji_asm_write_uint8(frame, (i>>8)  &0xffff);
}

void Kiji_asm_write_MVMnum64(KijiFrame *frame, MVMnum64 val) {
  // optmize?
  int i;
  char buf[8];
  memcpy(buf, &val, 8); // TODO endian
  for (i=0; i<8; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}

void Kiji_asm_write_MVMnum32(KijiFrame *frame, MVMnum32 val) {
  // optmize?
  char buf[4];
  memcpy(buf, &val, 4); // TODO endian
  int i;
  for (i=0; i<4; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}

void Kiji_asm_write_uint64_t(KijiFrame *frame, uint64_t val) {
  // optmize?
  char buf[8];
  memcpy(buf, &val, 8); // TODO endian
  int i;
  for (i=0; i<8; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}

void Kiji_asm_write_int64_t(KijiFrame *frame, int64_t val) {
  // optmize?
  char buf[8];
  memcpy(buf, &val, 8); // TODO endian
  int i;
  for (i=0; i<8; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}

void Kiji_asm_write_int8_t(KijiFrame *frame, int8_t val) {
  // optmize?
  char buf[1];
  memcpy(buf, &val, 1); // TODO endian
  int i;
  for (i=0; i<1; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}
void Kiji_asm_write_int16_t(KijiFrame *frame, int16_t val) {
  // optmize?
  char buf[2];
  memcpy(buf, &val, 2); // TODO endian
  int i;
  for (i=0; i<2; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}
void Kiji_asm_write_uint32_t_for(KijiFrame *frame, uint32_t val, size_t pos) {
  // optmize?
  char buf[4];
  memcpy(buf, &val, 4); // TODO endian
#ifdef DEBUG_ASM
  printf("ASM: write_uint32_t, val:%X pos:%X\n", val, pos);
#endif
  int i;
  for (i=0; i<4; i++) {
    frame->frame.bytecode[pos+i] = buf[i];
  }
}
void Kiji_asm_write_uint16_t_for(KijiFrame *frame, uint16_t val, size_t pos) {
  // optmize?
  char buf[2];
  memcpy(buf, &val, 2); // TODO endian
  int i;
  for (i=0; i<2; i++) {
    frame->frame.bytecode[pos+i] = buf[i];
  }
}

/*
void Kiji_asm_write_16(KijiFrame *frame, uint32_t i) {
    // optmize?
    bytecode_.push_back((i>>0)  &0xffff);
    bytecode_.push_back((i>>8)  &0xffff);
    bytecode_.push_back((i>>16) &0xffff);
    bytecode_.push_back((i>>24) &0xffff);
}
*/

void Kiji_asm_write_uint16_t(KijiFrame *frame, uint16_t val) {
  // optmize?
  char buf[2];
  memcpy(buf, &val, 2); // TODO endian
  int i;
  for (i=0; i<2; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}

void Kiji_asm_write_int32_t(KijiFrame *frame, int32_t val) {
  // optmize?
  char buf[4];
  memcpy(buf, &val, 4); // TODO endian
  int i;
  for (i=0; i<4; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}
void Kiji_asm_write_uint32_t(KijiFrame *frame, uint32_t val) {
  // optmize?
  char buf[4];
  memcpy(buf, &val, 4); // TODO endian
  int i;
  for (i=0; i<4; i++) {
    Kiji_asm_write_uint8_t(frame, buf[i]);
  }
}

void Kiji_asm_op(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num) {
#ifdef DEBUG_ASM
    MVMOpInfo *op = MVM_op_get_op(bank_num, op_num);
    printf("%d %X %s %X\n", bank_num, op_num, op->name, frame->frame.bytecode_size);
#endif
    Kiji_asm_write_uint8_t(frame, bank_num);
    Kiji_asm_write_uint8_t(frame, op_num);
}

void Kiji_asm_op_u16(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1) {
    Kiji_asm_op(frame, bank_num, op_num);
    Kiji_asm_write_u16(frame, op1);
}

void Kiji_asm_op_u16_u16(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2) {
    Kiji_asm_op(frame, bank_num, op_num);
    Kiji_asm_write_u16(frame, op1);
    Kiji_asm_write_u16(frame, op2);
}

void Kiji_asm_op_u16_u32(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2) {
    Kiji_asm_op(frame, bank_num, op_num);
    Kiji_asm_write_u16(frame, op1);
    Kiji_asm_write_uint32_t(frame, op2);
}

void Kiji_asm_op_u16_u16_u16(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2, uint16_t op3) {
    Kiji_asm_op(frame, bank_num, op_num);
    Kiji_asm_write_u16(frame, op1);
    Kiji_asm_write_u16(frame, op2);
    Kiji_asm_write_u16(frame, op3);
}

void Kiji_asm_dump_compunit(MVMCompUnit *cu) {
  int i;
  for (i=0; i<cu->num_frames; i++) {
    printf("-- frame %d --\n", i);
    printf("      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
    Kiji_asm_dump_frame(cu->frames[i]);
  }
}

void Kiji_asm_dump_frame(MVMStaticFrame *frame) {
  int x=0;
  int z=0;
  for (z=0; z<frame->bytecode_size; z++) {
    if (z%16==0) {
      if (z!=0) {
        printf("\n");
      }
      printf("%04X", x++);
    }
    printf(" %02X", frame->bytecode[z]);
  }
  printf("\n");
}

