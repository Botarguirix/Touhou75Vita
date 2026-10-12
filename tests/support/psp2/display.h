#pragma once
#include <cstdint>
// Host-only declaration shim. These tests do not exercise Vita scanout.
struct SceDisplayFrameBuf {
    uint32_t size; void* base; uint32_t pitch, pixelformat, width, height;
};
constexpr int SCE_DISPLAY_PIXELFORMAT_A8B8G8R8=0;
constexpr int SCE_DISPLAY_SETBUF_NEXTFRAME=1, SCE_DISPLAY_SETBUF_IMMEDIATE=0;
int sceDisplaySetFrameBuf(const SceDisplayFrameBuf*,int);
int sceDisplayGetFrameBuf(SceDisplayFrameBuf*,int);
int sceDisplayWaitVblankStart();
