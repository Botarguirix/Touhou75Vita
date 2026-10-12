#pragma once
#include <cstdint>
#include <cstdio>
#ifdef __vita__
#include <malloc.h>
#include <psp2/kernel/sysmem.h>
extern "C" unsigned int _get_vita_heap_size();
#endif

// Read-only native diagnostics. Kernel free USER_RW is distinct from free
// space in the already reserved newlib heap; neither measures fragmentation.
inline void report_native_memory(FILE* log,const char* phase,uint32_t graphics_used,uint32_t graphics_budget) {
#ifdef __vita__
    const auto heap=mallinfo();
    const unsigned total=_get_vita_heap_size();
    fprintf(log,"startup_native_heap=phase:%s reserved:%u managed:%u in_use:%u free_chunks:%u top_free:%u graphics_used:%u graphics_budget:%u\n",
        phase,total,unsigned(heap.arena),unsigned(heap.uordblks),unsigned(heap.fordblks),unsigned(heap.keepcost),graphics_used,graphics_budget);
    SceKernelFreeMemorySizeInfo free={};free.size=sizeof(free);
    const int rc=sceKernelGetFreeMemorySize(&free);
    if(rc>=0)fprintf(log,"startup_kernel_memory=phase:%s rc:0x%08X free_user:%u free_cdram:%u free_phycont:%u\n",
        phase,unsigned(rc),unsigned(free.size_user),unsigned(free.size_cdram),unsigned(free.size_phycont));
    else fprintf(log,"startup_kernel_memory=phase:%s rc:0x%08X values:unavailable\n",phase,unsigned(rc));
#else
    fprintf(log,"startup_native_memory=phase:%s values:unavailable_host_check graphics_used:%u graphics_budget:%u\n",phase,graphics_used,graphics_budget);
#endif
}
