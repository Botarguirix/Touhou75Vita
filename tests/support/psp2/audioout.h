#pragma once
// Host-only declarations; tests simulate native APIs, never Vita hardware.
constexpr int SCE_AUDIO_OUT_PORT_TYPE_MAIN=0, SCE_AUDIO_OUT_MODE_STEREO=1;
constexpr int SCE_AUDIO_OUT_CONFIG_TYPE_LEN=0, SCE_AUDIO_OUT_CONFIG_TYPE_FREQ=1,
    SCE_AUDIO_OUT_CONFIG_TYPE_MODE=2;
int sceAudioOutOpenPort(int,int,int,int);
int sceAudioOutGetConfig(int,int);
int sceAudioOutReleasePort(int);
constexpr int SCE_AUDIO_OUT_MAX_VOL=32768;
enum SceAudioOutChannelFlag {SCE_AUDIO_VOLUME_FLAG_L_CH=1,SCE_AUDIO_VOLUME_FLAG_R_CH=2};
int sceAudioOutSetVolume(int,SceAudioOutChannelFlag,int*);
int sceAudioOutOutput(int,const void*);
int sceAudioOutGetRestSample(int);
