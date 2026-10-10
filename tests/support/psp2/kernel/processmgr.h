#pragma once
#include <cstdint>
uint64_t sceKernelGetProcessTimeWide();
[[noreturn]] void sceKernelExitProcess(int);
