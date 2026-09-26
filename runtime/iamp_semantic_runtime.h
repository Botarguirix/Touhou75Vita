#ifndef IAMP_SEMANTIC_RUNTIME_H
#define IAMP_SEMANTIC_RUNTIME_H
#include <stdint.h>
#include <stddef.h>
typedef struct IAMP_CPU {
 uint32_t r[8]; uint32_t eip; uint32_t eflags;
 uint32_t *mem; size_t mem_words; uint8_t df;
 double x87[8]; uint8_t x87_top; uint16_t x87_status; uint16_t x87_control;
 uint8_t xmm[8][16]; uint8_t mmx[8][8]; uint32_t mxcsr;
 uint8_t halted;
} IAMP_CPU;
enum {R_EAX,R_ECX,R_EDX,R_EBX,R_ESP,R_EBP,R_ESI,R_EDI};
uint32_t iamp_readop(IAMP_CPU*,const char*,int); void iamp_writeop(IAMP_CPU*,const char*,int,uint32_t); void iamp_push(IAMP_CPU*,uint32_t); uint32_t iamp_pop(IAMP_CPU*);
void iamp_mov(IAMP_CPU*,const char*,const char*); void iamp_movzx(IAMP_CPU*,const char*,const char*); void iamp_movsx(IAMP_CPU*,const char*,const char*); void iamp_lea(IAMP_CPU*,const char*,const char*); void iamp_xchg(IAMP_CPU*,const char*,const char*);
#define DECL2(n) void iamp_##n(IAMP_CPU*,const char*,const char*)
DECL2(add);DECL2(adc);DECL2(sub);DECL2(sbb);DECL2(and);DECL2(or);DECL2(xor);DECL2(cmp);DECL2(test);DECL2(inc);DECL2(dec);DECL2(neg);DECL2(not);DECL2(imul);DECL2(mul);DECL2(idiv);DECL2(div);DECL2(shl);DECL2(shr);DECL2(sar);DECL2(rol);DECL2(ror);DECL2(rcl);DECL2(rcr);
void iamp_sete(IAMP_CPU*,const char*);void iamp_setne(IAMP_CPU*,const char*);void iamp_setz(IAMP_CPU*,const char*);void iamp_setnz(IAMP_CPU*,const char*);void iamp_seto(IAMP_CPU*,const char*);void iamp_setno(IAMP_CPU*,const char*);void iamp_setb(IAMP_CPU*,const char*);void iamp_setae(IAMP_CPU*,const char*);void iamp_setbe(IAMP_CPU*,const char*);void iamp_seta(IAMP_CPU*,const char*);void iamp_setl(IAMP_CPU*,const char*);void iamp_setge(IAMP_CPU*,const char*);void iamp_setle(IAMP_CPU*,const char*);void iamp_setg(IAMP_CPU*,const char*);void iamp_sets(IAMP_CPU*,const char*);void iamp_setns(IAMP_CPU*,const char*);void iamp_setp(IAMP_CPU*,const char*);void iamp_setnp(IAMP_CPU*,const char*);void iamp_setc(IAMP_CPU*,const char*);void iamp_setnc(IAMP_CPU*,const char*);int iamp_cond(IAMP_CPU*,const char*);
void iamp_call(IAMP_CPU*,uint32_t,uint32_t);void iamp_indirect_call(IAMP_CPU*,const char*,uint32_t);void iamp_ret(IAMP_CPU*);void iamp_jump(IAMP_CPU*,uint32_t);void iamp_indirect_jump(IAMP_CPU*,const char*);void iamp_leave(IAMP_CPU*);void iamp_clc(IAMP_CPU*);void iamp_stc(IAMP_CPU*);void iamp_cld(IAMP_CPU*);void iamp_std(IAMP_CPU*);void iamp_cdq(IAMP_CPU*);void iamp_cwd(IAMP_CPU*);void iamp_cwde(IAMP_CPU*);void iamp_cbw(IAMP_CPU*);void iamp_emms(IAMP_CPU*);
void iamp_pusha(IAMP_CPU*); void iamp_popa(IAMP_CPU*); void iamp_pushf(IAMP_CPU*); void iamp_popf(IAMP_CPU*); void iamp_lahf(IAMP_CPU*); void iamp_cpuid(IAMP_CPU*); void iamp_xlat(IAMP_CPU*); void iamp_bit(IAMP_CPU*,const char*,const char*); void iamp_double_shift(IAMP_CPU*,const char*,const char*); void iamp_cmov(IAMP_CPU*,const char*,const char*);
void iamp_x87(IAMP_CPU*,const char*,const char*,const char*); void iamp_sahf(IAMP_CPU*);
void iamp_simd(IAMP_CPU*,const char*,const char*); void iamp_string(IAMP_CPU*,const char*,const char*);
void iamp_trap(IAMP_CPU*,uint32_t,const char*);void iamp_unimplemented(IAMP_CPU*,uint32_t,const char*);
#endif
