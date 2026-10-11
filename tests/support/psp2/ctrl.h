#pragma once
// Host test subset of the VitaSDK control API; no controller/device is used.
#include <cstdint>
enum SceCtrlPadInputMode { SCE_CTRL_MODE_ANALOG_WIDE=2 };
enum SceCtrlButtons {
    SCE_CTRL_SELECT=1,SCE_CTRL_START=8,SCE_CTRL_UP=0x10,SCE_CTRL_RIGHT=0x20,
    SCE_CTRL_DOWN=0x40,SCE_CTRL_LEFT=0x80,SCE_CTRL_LTRIGGER=0x100,
    SCE_CTRL_RTRIGGER=0x200,SCE_CTRL_TRIANGLE=0x1000,SCE_CTRL_CIRCLE=0x2000,
    SCE_CTRL_CROSS=0x4000,SCE_CTRL_SQUARE=0x8000
};
struct SceCtrlData {
    uint64_t timeStamp=0;uint32_t buttons=0;
    uint8_t lx=128,ly=128,rx=128,ry=128;
    uint8_t unused_pressure[12]{},reserved[4]{};
};
static_assert(sizeof(SceCtrlData)==32);
int sceCtrlSetSamplingMode(SceCtrlPadInputMode);
int sceCtrlPeekBufferPositive(int,SceCtrlData*,int);
