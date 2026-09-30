#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h>
#include <stdlib.h>
#include <string.h>
#include "iamp_semantic_runtime.h"

extern void iamp_dispatch(IAMP_CPU *, uint32_t);

int main(void) {
    const size_t guest_size = 16 * 1024 * 1024;
    IAMP_CPU *cpu = calloc(1, sizeof(*cpu));
    uint32_t *mem = calloc(1, guest_size);
    if (!cpu || !mem) sceKernelExitProcess(1);
    cpu->mem = mem;
    cpu->mem_words = guest_size / 4;
    cpu->r[R_ESP] = 0x700000;
    cpu->r[R_EBP] = 0;
    cpu->eip = 0x64232c;
    iamp_dispatch(cpu, cpu->eip);
    sceKernelExitProcess((int)(cpu->r[R_EAX] & 0x7fffffff));
    return 0;
}
