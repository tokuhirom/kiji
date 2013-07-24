#ifndef KIJI_ASM_H_
#define KIJI_ASM_H_

#ifdef __cplusplus
extern "C" {
#endif

void Kiji_asm_write_uint8_t(KijiFrame *frame, MVMuint8 u8);
void Kiji_asm_write_u16(KijiFrame *frame, MVMuint16 i);
void Kiji_asm_write_MVMnum64(KijiFrame *frame, MVMnum64 i);
void Kiji_asm_write_MVMnum32(KijiFrame *frame, MVMnum32 i);
void Kiji_asm_write_uint64_t(KijiFrame *frame, uint64_t i);
void Kiji_asm_write_int64_t(KijiFrame *frame, int64_t i);
void Kiji_asm_write_int8_t(KijiFrame *frame, int8_t i);
void Kiji_asm_write_int16_t(KijiFrame *frame, int16_t i);
void Kiji_asm_write_uint32_t_for(KijiFrame *frame, uint32_t val, size_t pos);
void Kiji_asm_write_uint16_t_for(KijiFrame *frame, uint16_t val, size_t pos);
void Kiji_asm_write_16(KijiFrame *frame, uint32_t i);
void Kiji_asm_write_uint16_t(KijiFrame *frame, uint16_t i);
void Kiji_asm_write_int32_t(KijiFrame *frame, int32_t i);
void Kiji_asm_write_uint32_t(KijiFrame *frame, uint32_t i);
void Kiji_asm_op(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num);
void Kiji_asm_op_u16(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1);
void Kiji_asm_op_u16_u16(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2);
void Kiji_asm_op_u16_u32(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2);
void Kiji_asm_op_u16_u16_u16(KijiFrame *frame, MVMuint8 bank_num, MVMuint8 op_num, uint16_t op1, uint16_t op2, uint16_t op3);
void Kiji_asm_dump_compunit(MVMCompUnit *cu);
void Kiji_asm_dump_frame(MVMStaticFrame *frame);

#ifdef __cplusplus
};
#endif

#endif /* KIJI_ASM_H_ */
