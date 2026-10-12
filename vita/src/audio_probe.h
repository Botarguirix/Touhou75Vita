#pragma once
#include <psp2/audioout.h>
#include <array>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <algorithm>

namespace th075 {
// Independent decoder for one original PAK1 PCM effect. Not guest DirectSound.
inline std::vector<int16_t> audio_probe_pcm;
inline uint32_t audio_probe_runs=0;
inline uint32_t audio_u32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
inline uint16_t audio_u16(const uint8_t* p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
inline bool load_audio_probe(FILE* log){
    audio_probe_pcm.clear();
    fprintf(log,"audio_probe_scope=original_dat_effect_native_output_not_guest_directsound\n");
    FILE* file=fopen("ux0:data/TH075Vita/th075.dat","rb");
    if(!file){fprintf(log,"audio_probe_asset=archive_missing\n");return false;}
    struct Close{FILE* file;~Close(){fclose(file);}} close{file};
    auto fail=[&](){fprintf(log,"audio_probe_asset=decode_failed\n");return false;};
    if(fseek(file,0,SEEK_END))return fail();
    const long length=ftell(file);uint8_t count_bytes[2]{};
    if(length<2 || fseek(file,0,SEEK_SET) || fread(count_bytes,1,2,file)!=2)return fail();
    const unsigned count=audio_u16(count_bytes);
    if(!count || count>4096 || 2u+count*108u>uint64_t(length))return fail();
    std::vector<uint8_t> directory(count*108u);
    if(fread(directory.data(),1,directory.size(),file)!=directory.size())return fail();
    uint8_t key=0x64,delta=0x64;
    for(auto& byte:directory){byte^=key;key=uint8_t(key+delta);delta=uint8_t(delta+0x4D);}
    uint32_t offset=0,size=0;unsigned matches=0;
    for(unsigned i=0;i<count;++i){
        const auto* item=directory.data()+108*i;
        if(!std::memchr(item,0,100))return fail();
        const uint32_t n=audio_u32(item+100),o=audio_u32(item+104);
        if(o<2u+count*108u || uint64_t(o)+n>uint64_t(length))return fail();
        if(!std::strcmp(reinterpret_cast<const char*>(item),"wave\\se.dat")){offset=o;size=n;++matches;}
    }
    if(matches!=1 || size<4 || size>4u*1024*1024 || fseek(file,long(offset),SEEK_SET))return fail();
    std::vector<uint8_t> container(size);
    if(fread(container.data(),1,size,file)!=size)return fail();
    const unsigned entries=audio_u32(container.data());
    if(!entries || entries>4096)return fail();
    size_t cursor=4,selected=0;uint32_t pcm_bytes=0,rate=0;unsigned channels=0;
    for(unsigned i=0;i<entries;++i){
        if(cursor>=container.size())return fail();
        if(!container[cursor++])continue;
        if(container.size()-cursor<22)return fail();
        const auto* h=container.data()+cursor;const uint32_t bytes=audio_u32(h);cursor+=22;
        if(bytes>container.size()-cursor)return fail();
        if(i==2){
            const unsigned format=audio_u16(h+4),align=audio_u16(h+16),bits=audio_u16(h+18),extra=audio_u16(h+20);
            channels=audio_u16(h+6);rate=audio_u32(h+8);
            if(format!=1 || (channels!=1 && channels!=2) || bits!=16 || extra ||
               (rate!=22050 && rate!=44100 && rate!=48000) || align!=2*channels ||
               audio_u32(h+12)!=rate*align || !bytes || bytes%align || bytes>2u*rate*align)return fail();
            selected=cursor;pcm_bytes=bytes;
        }
        cursor+=bytes;
    }
    if(cursor!=container.size() || !selected)return fail();
    const auto* pcm=container.data()+selected;
    uint32_t hash=2166136261u,nonzero=0;int peak=0;
    for(uint32_t i=0;i<pcm_bytes;++i){hash^=pcm[i];hash*=16777619u;}
    const auto sample=[&](uint32_t frame,unsigned channel){
        const uint16_t word=audio_u16(pcm+2u*(frame*channels+channel));
        return word>=32768?int(word)-65536:int(word);
    };
    const uint32_t frames=pcm_bytes/(2*channels);
    for(uint32_t i=0;i<frames;++i)for(unsigned c=0;c<channels;++c){const int s=sample(i,c);if(s)++nonzero;peak=std::max(peak,s<0?-s:s);}
    if(!peak)return fail();
    fprintf(log,"audio_probe_source=wave/se.dat effect_index:2\naudio_probe_original=channels:%u bits:16 hz:%u frames:%u bytes:%u peak:%d nonzero:%u fnv1a32:0x%08X\n",channels,rate,frames,pcm_bytes,peak,nonzero,hash);
    // Integer linear interpolation to MAIN's 48 kHz; mono duplicated to stereo.
    const uint32_t output_frames=(uint64_t(frames)*48000+rate-1)/rate;
    audio_probe_pcm.resize(size_t(output_frames)*2);
    for(uint32_t i=0;i<output_frames;++i){
        const uint64_t position=uint64_t(i)*rate;
        const uint32_t first=std::min(uint32_t(position/48000),frames-1),next=std::min(first+1,frames-1),fraction=position%48000;
        for(unsigned c=0;c<2;++c){const unsigned source=channels==1?0:c;
            audio_probe_pcm[2*i+c]=int16_t((int64_t(sample(first,source))*(48000-fraction)+int64_t(sample(next,source))*fraction)/48000);
        }
    }
    fprintf(log,"audio_probe_asset=decoded\naudio_probe_conversion=linear_48000hz_stereo frames:%u\n",output_frames);
    return true;
}

inline bool play_audio_probe(FILE* log,const char* reason){
    if(audio_probe_pcm.empty()){fprintf(log,"audio_probe_output=skipped_missing_asset\n");fflush(log);return false;}
    fprintf(log,"audio_probe_run=%u reason:%s\n",++audio_probe_runs,reason);
    fflush(log);
    const int port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_MAIN,1024,48000,SCE_AUDIO_OUT_MODE_STEREO);
    fprintf(log,"audio_probe_open_rc=0x%08X\n",unsigned(port));
    if(port<0){fprintf(log,"audio_probe_output=open_failed\n");fflush(log);return false;}
    struct Close{int port;FILE* log;~Close(){const int rc=sceAudioOutReleasePort(port);fprintf(log,"audio_probe_release_rc=0x%08X\n",unsigned(rc));fflush(log);}} close{port,log};
    int volume[2]={8192,8192}; // 25% port gain; console's physical volume still applies.
    const int volume_rc=sceAudioOutSetVolume(port,static_cast<SceAudioOutChannelFlag>(SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH),volume);
    const int len=sceAudioOutGetConfig(port,SCE_AUDIO_OUT_CONFIG_TYPE_LEN);
    const int freq=sceAudioOutGetConfig(port,SCE_AUDIO_OUT_CONFIG_TYPE_FREQ);
    const int mode=sceAudioOutGetConfig(port,SCE_AUDIO_OUT_CONFIG_TYPE_MODE);
    fprintf(log,"audio_probe_native_config=frames:%d hz:%d mode:%d volume_rc:0x%08X gain:25_percent\n",len,freq,mode,unsigned(volume_rc));
    if(volume_rc<0 || len!=1024 || freq!=48000 || mode!=SCE_AUDIO_OUT_MODE_STEREO){fprintf(log,"audio_probe_output=config_failed\n");return false;}
    std::array<int16_t,2048> block{};
    uint32_t blocks=0;int rc=0;
    // Keep each submitted buffer alive until the documented drain completes.
    for(size_t cursor=0;cursor<audio_probe_pcm.size();cursor+=block.size()){
        block.fill(0);const size_t n=std::min(block.size(),audio_probe_pcm.size()-cursor);
        std::copy_n(audio_probe_pcm.data()+cursor,n,block.data());
        rc=sceAudioOutOutput(port,block.data());if(rc<0)break;
        ++blocks;rc=sceAudioOutOutput(port,nullptr);if(rc<0)break;
    }
    fprintf(log,"audio_probe_output=%s blocks:%u rc:0x%08X\naudio_probe_audible_confirmation=pending_user\n",rc<0?"failed":"submitted_and_drained",blocks,unsigned(rc));
    return rc>=0;
}
}
