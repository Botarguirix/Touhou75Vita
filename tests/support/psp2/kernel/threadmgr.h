#pragma once
#include <cstdint>
using SceUID=int;
using SceSize=unsigned;
using SceKernelThreadEntry=int(*)(SceSize,void*);
SceUID sceKernelCreateThread(const char*,SceKernelThreadEntry,int,SceSize,unsigned,int,void*);
int sceKernelStartThread(SceUID,SceSize,void*);
int sceKernelWaitThreadEnd(SceUID,int*,unsigned*);
int sceKernelDeleteThread(SceUID);
int sceKernelDelayThread(unsigned);
