#pragma once
#include <cstdint>
using SceUID=int;
constexpr int SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW=0;
SceUID sceKernelAllocMemBlock(const char*,int,uint32_t,void*);
int sceKernelGetMemBlockBase(SceUID,void**);
int sceKernelFreeMemBlock(SceUID);
