#pragma once
// Host-test declarations for the user core subset of VitaSDK.
#define SCE_KERNEL_CPU_MASK_USER_0 0x00010000
#define SCE_KERNEL_CPU_MASK_USER_1 0x00020000
#define SCE_KERNEL_CPU_MASK_USER_2 0x00040000
int sceKernelGetCpuId();
