#pragma once
#include <psp2/audioout.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <atomic>
#include <array>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <malloc.h>
#include <memory>

namespace th075 {
// Native, bounded DAT/WAV streaming; independent of guest DirectSound.
class MusicProbe {
    struct Track {std::string name;uint32_t offset,size;};
    std::vector<Track> tracks_;
    std::mt19937 random_;
    std::atomic<uint32_t> command_{0},playing_{0},state_{0};
    std::atomic<bool> quit_{false};
    SceUID thread_=-1;
    uint32_t serial_=0,desired_=0;
    static uint32_t u32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
    static uint16_t u16(const uint8_t* p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
    static constexpr const char* archive="ux0:data/TH075Vita/th075bgm.dat";
    bool inventory(FILE* log){
        FILE* f=fopen(archive,"rb");
        if(!f){fprintf(log,"bgm_inventory=archive_missing\n");return false;}
        struct Close{FILE* f;~Close(){fclose(f);}} close{f};
        if(fseek(f,0,SEEK_END))return false;
        const long length=ftell(f);uint8_t count_bytes[2]{};
        if(length<2 || fseek(f,0,SEEK_SET) || fread(count_bytes,1,2,f)!=2)return false;
        const unsigned count=u16(count_bytes);
        if(!count || count>4096 || uint64_t(2)+count*108>uint64_t(length))return false;
        std::vector<uint8_t> table(count*108);
        if(fread(table.data(),1,table.size(),f)!=table.size())return false;
        uint8_t key=0x64,delta=0x64;
        for(auto& byte:table){byte^=key;key=uint8_t(key+delta);delta=uint8_t(delta+0x4D);}
        for(unsigned i=0;i<count;++i){
            const auto* p=table.data()+108*i;
            if(!std::memchr(p,0,100))return false;
            std::string name(reinterpret_cast<const char*>(p));
            const uint32_t size=u32(p+100),offset=u32(p+104);
            if(offset<2+count*108 || uint64_t(offset)+size>uint64_t(length))return false;
            if(name.compare(0,9,"wave\\bgm\\") || name.size()<13 || name.substr(name.size()-4)!=".wav")continue;
            if(name.find("..")!=std::string::npos || name.find(':')!=std::string::npos || size<44)return false;
            if(tracks_.size()==64)return false;
            tracks_.push_back({name,offset,size});
        }
        const uint64_t seed=sceKernelGetProcessTimeWide();
        random_.seed(uint32_t(seed)^uint32_t(seed>>32)^uint32_t(length));
        fprintf(log,"bgm_inventory=passed tracks:%u seed:0x%08X\nbgm_scope=native_streaming_not_guest_directsound\nbgm_log=ux0:data/TH075Vita/iteration61-bgm.log\n",unsigned(tracks_.size()),uint32_t(seed)^uint32_t(seed>>32)^uint32_t(length));
        return !tracks_.empty();
    }
    struct WaveStream {
        FILE* f;uint32_t data=0,frames=0;
        uint32_t cache_first=0,cache_frames=0;
        std::array<uint8_t,4096*4> cache{};
        explicit WaveStream(FILE* source):f(source){}
        bool open(const Track& track,FILE* log){
            uint8_t h[12]{};cache_frames=0;data=frames=0;
            if(fseek(f,long(track.offset),SEEK_SET) || fread(h,1,12,f)!=12 || std::memcmp(h,"RIFF",4) || std::memcmp(h+8,"WAVE",4) || uint64_t(u32(h+4))+8!=track.size)return false;
            uint64_t cursor=12;bool format=false,pcm=false;
            while(cursor<track.size){
                uint8_t chunk[8]{};
                if(track.size-cursor<8 || fseek(f,long(uint64_t(track.offset)+cursor),SEEK_SET) || fread(chunk,1,8,f)!=8)return false;
                const uint32_t n=u32(chunk+4);cursor+=8;
                if(uint64_t(n)+(n&1)>track.size-cursor)return false;
                if(!std::memcmp(chunk,"fmt ",4)){
                    uint8_t fmt[16]{};
                    if(format || n<16 || fread(fmt,1,16,f)!=16 || u16(fmt)!=1 || u16(fmt+2)!=2 || u32(fmt+4)!=44100 || u32(fmt+8)!=176400 || u16(fmt+12)!=4 || u16(fmt+14)!=16)return false;
                    format=true;
                }else if(!std::memcmp(chunk,"data",4)){
                    if(pcm || !n || n%4 || n>44100u*4*600)return false;
                    data=uint32_t(uint64_t(track.offset)+cursor);frames=n/4;pcm=true;
                }
                cursor+=uint64_t(n)+(n&1);
            }
            fprintf(log,"bgm_wave=%s source_hz:44100 channels:2 bits:16 frames:%u data_offset:%u\n",track.name.c_str(),frames,data);
            return cursor==track.size && format && pcm;
        }
        bool sample(uint32_t index,std::array<int,2>& out){
            if(index>=frames)return false;
            if(!cache_frames || index<cache_first || index>=cache_first+cache_frames){
                cache_first=index;cache_frames=std::min(4096u,frames-index);
                if(fseek(f,long(uint64_t(data)+4ull*index),SEEK_SET) || fread(cache.data(),4,cache_frames,f)!=cache_frames){cache_frames=0;return false;}
            }
            const auto* p=cache.data()+4*(index-cache_first);
            for(unsigned c=0;c<2;++c){const unsigned s=u16(p+2*c);out[c]=s>=32768?int(s)-65536:int(s);}
            return true;
        }
    };
    static int entry(SceSize bytes,void* argument){
        MusicProbe* self=nullptr;
        if(bytes!=sizeof(self))return -1;
        std::memcpy(&self,argument,sizeof(self));return self->run();
    }
    int run(){
        FILE* log=fopen("ux0:data/TH075Vita/iteration61-bgm.log","wb");
        if(!log){state_.store(2);return -1;}
        setvbuf(log,nullptr,_IONBF,0);
        FILE* f=fopen(archive,"rb");
        if(!f){fprintf(log,"bgm_result=archive_open_failed\n");state_.store(2);fclose(log);return -1;}
        SceKernelThreadInfo info{};info.size=sizeof(info);
        if(sceKernelGetThreadInfo(sceKernelGetThreadId(),&info)>=0 && info.currentPriority>64){
            const int priority_rc=sceKernelChangeThreadPriority(sceKernelGetThreadId(),info.currentPriority-1);
            fprintf(log,"bgm_priority=before:%d requested:%d rc:0x%08X\n",info.currentPriority,info.currentPriority-1,unsigned(priority_rc));
        }
        constexpr unsigned frames_per_block=2048,buffer_samples=frames_per_block*2;
        // Alternating buffers follow SDL's Vita backend. No per-block drain.
        std::unique_ptr<int16_t,decltype(&std::free)> buffers(
            static_cast<int16_t*>(memalign(64,2*buffer_samples*sizeof(int16_t))),&std::free);
        if(!buffers){fprintf(log,"bgm_result=buffer_allocation_failed\n");state_.store(2);fclose(f);fclose(log);return -1;}
        const int port=sceAudioOutOpenPort(SCE_AUDIO_OUT_PORT_TYPE_BGM,frames_per_block,44100,SCE_AUDIO_OUT_MODE_STEREO);
        fprintf(log,"bgm_open_rc=0x%08X\n",unsigned(port));
        if(port<0){state_.store(2);fclose(f);fclose(log);return -1;}
        int volume[2]={8192,8192};
        const int volume_rc=sceAudioOutSetVolume(port,static_cast<SceAudioOutChannelFlag>(SCE_AUDIO_VOLUME_FLAG_L_CH|SCE_AUDIO_VOLUME_FLAG_R_CH),volume);
        const int len=sceAudioOutGetConfig(port,SCE_AUDIO_OUT_CONFIG_TYPE_LEN),freq=sceAudioOutGetConfig(port,SCE_AUDIO_OUT_CONFIG_TYPE_FREQ),mode=sceAudioOutGetConfig(port,SCE_AUDIO_OUT_CONFIG_TYPE_MODE);
        fprintf(log,"bgm_config=frames:%d hz:%d mode:%d volume_rc:0x%08X gain:25_percent\n",len,freq,mode,unsigned(volume_rc));
        bool good=volume_rc>=0 && len==int(frames_per_block) && freq==44100 && mode==SCE_AUDIO_OUT_MODE_STEREO;
        fprintf(log,"bgm_pipeline=bgm_44100_pcm_no_resampling double_buffer_64byte_aligned drain_only_at_shutdown\n");
        WaveStream stream(f);unsigned buffer_index=0;
        uint32_t active=0;uint64_t position=0,limit=0,total_blocks=0;int output_rc=0;
        const uint64_t begin=sceKernelGetProcessTimeWide();
        uint64_t fill_max=0,output_max=0,late_fill=0;
        while(good && !quit_.load(std::memory_order_acquire)){
            const uint32_t requested=command_.load(std::memory_order_acquire);
            if(requested!=active){
                const unsigned index=requested&63;
                if(index>=tracks_.size() || !stream.open(tracks_[index],log)){fprintf(log,"bgm_result=unsupported_or_invalid_wave\n");good=false;break;}
                active=requested;playing_.store(index);state_.store(1);position=0;
                limit=stream.frames;
                fprintf(log,"bgm_track_start=command:%u index:%u name:%s output_frames:%llu\n",active,index,tracks_[index].name.c_str(),(unsigned long long)limit);
            }
            int16_t* block=buffers.get()+buffer_index*buffer_samples;
            const uint64_t fill_begin=sceKernelGetProcessTimeWide();
            for(unsigned i=0;i<frames_per_block;++i,++position){
                if(position==limit){position=0;stream.cache_frames=0;fprintf(log,"bgm_track_loop=index:%u policy:whole_wav\n",active&63);}
                std::array<int,2> sample{};
                if(!stream.sample(uint32_t(position),sample)){good=false;break;}
                for(unsigned c=0;c<2;++c)block[2*i+c]=int16_t(sample[c]);
            }
            if(!good){fprintf(log,"bgm_result=pcm_read_failed\n");break;}
            const uint64_t output_begin=sceKernelGetProcessTimeWide(),fill_us=output_begin-fill_begin;
            fill_max=std::max(fill_max,fill_us);if(fill_us>uint64_t(frames_per_block)*1000000/44100)++late_fill;
            output_rc=sceAudioOutOutput(port,block);
            output_max=std::max(output_max,sceKernelGetProcessTimeWide()-output_begin);
            if(output_rc<0){good=false;break;}
            buffer_index^=1;
            ++total_blocks;
            if(total_blocks%240==0)fprintf(log,"bgm_progress=blocks:%llu track_index:%u position:%llu\n",(unsigned long long)total_blocks,active&63,(unsigned long long)position);
        }
        const int drain=sceAudioOutOutput(port,nullptr);
        fprintf(log,"bgm_final_drain_rc=0x%08X\nbgm_timing=elapsed_us:%llu fill_max_us:%llu output_max_us:%llu late_fill_blocks:%llu\n",unsigned(drain),(unsigned long long)(sceKernelGetProcessTimeWide()-begin),(unsigned long long)fill_max,(unsigned long long)output_max,(unsigned long long)late_fill);
        if(drain<0)good=false;
        fprintf(log,"bgm_output=%s blocks:%llu rc:0x%08X\n",good?"stopped_by_user":"failed",(unsigned long long)total_blocks,unsigned(output_rc));
        const int release=sceAudioOutReleasePort(port);
        fprintf(log,"bgm_release_rc=0x%08X\n",unsigned(release));
        state_.store(good?3:2);fclose(f);fclose(log);return good?0:-1;
    }
public:
    ~MusicProbe(){stop(nullptr);}
    bool start(FILE* log){
        if(!inventory(log)){fprintf(log,"bgm_start=inventory_failed\n");return false;}
        change(log,true);
        thread_=sceKernelCreateThread("TH075 native BGM",entry,0x10000100,0x10000,0,0,nullptr);
        fprintf(log,"bgm_thread_create_rc=0x%08X\n",unsigned(thread_));
        if(thread_<0)return false;
        MusicProbe* self=this;const int rc=sceKernelStartThread(thread_,sizeof(self),&self);
        fprintf(log,"bgm_thread_start_rc=0x%08X\n",unsigned(rc));
        if(rc<0){sceKernelDeleteThread(thread_);thread_=-1;return false;}
        return true;
    }
    void change(FILE* log,bool initial=false){
        if(tracks_.empty())return;
        const unsigned n=tracks_.size();
        if(initial)desired_=std::uniform_int_distribution<unsigned>(0,n-1)(random_);
        else if(n>1){unsigned next=std::uniform_int_distribution<unsigned>(0,n-2)(random_);if(next>=desired_)++next;desired_=next;}
        request(log,initial?"automatic_random":"triangle_random");
    }
    void replay(FILE* log){if(!tracks_.empty())request(log,"square_restart");}
    void request(FILE* log,const char* reason){
        const uint32_t value=(++serial_<<6)|desired_;
        command_.store(value,std::memory_order_release);
        fprintf(log,"bgm_command=reason:%s command:%u index:%u track:%s\n",reason,value,desired_,tracks_[desired_].name.c_str());fflush(log);
    }
    std::string label()const{
        if(tracks_.empty() || thread_<0)return "BGM UNAVAILABLE";
        if(state_.load()==2)return "BGM ERROR - SEE LOG";
        if(state_.load()==0)return "BGM STARTING";
        const unsigned index=playing_.load();
        return "BGM "+tracks_[index].name.substr(tracks_[index].name.find_last_of('\\')+1);
    }
    void stop(FILE* log){
        if(thread_<0)return;
        quit_.store(true,std::memory_order_release);
        const int rc=sceKernelWaitThreadEnd(thread_,nullptr,nullptr);
        const int deleted=rc>=0?sceKernelDeleteThread(thread_):rc;
        if(log)fprintf(log,"bgm_thread_join_rc=0x%08X delete_rc=0x%08X\n",unsigned(rc),unsigned(deleted));
        thread_=-1;
    }
};
}
