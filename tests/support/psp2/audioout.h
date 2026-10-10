#pragma once
// Host-only declarations; tests simulate port setup and never submit audio.
constexpr int SCE_AUDIO_OUT_PORT_TYPE_MAIN=0, SCE_AUDIO_OUT_MODE_STEREO=1;
constexpr int SCE_AUDIO_OUT_CONFIG_TYPE_LEN=0, SCE_AUDIO_OUT_CONFIG_TYPE_FREQ=1,
    SCE_AUDIO_OUT_CONFIG_TYPE_MODE=2;
int sceAudioOutOpenPort(int,int,int,int);
int sceAudioOutGetConfig(int,int);
int sceAudioOutReleasePort(int);
