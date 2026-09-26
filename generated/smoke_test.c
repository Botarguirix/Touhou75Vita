#include "iamp_semantic_runtime.h"
#include <stdio.h>
#include <stdlib.h>
void guest_00401000(IAMP_CPU*);
int main(){size_t words=0x800000/4; uint32_t *mem=calloc(words,sizeof(uint32_t)); IAMP_CPU s={0}; s.mem=mem; s.mem_words=words; s.r[R_ESP]=0x700000; s.r[R_EBP]=0; s.r[R_ECX]=0x12345678; iamp_push(&s,0xdeadbeef); guest_00401000(&s); printf("eax=%08x esp=%08x eip=%08x global=%08x\n",s.r[R_EAX],s.r[R_ESP],s.eip,mem[0x671210/4]); free(mem); return 0;}
