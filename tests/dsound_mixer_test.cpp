#include "../vita/src/dsound_pcm_mixer.h"
#include <cassert>
#include <cstdio>
using namespace dsound_pcm;
static Buffer voice(unsigned frames,unsigned channels=2,unsigned bits=16,unsigned hz=48000){
    Buffer b;b.refs=1;b.channels=channels;b.bits=bits;b.align=channels*bits/8;b.hz=b.original_hz=hz;
    b.bytes.assign(frames*b.align,bits==8?128:0);b.playing=true;return b;
}
static void put(Buffer& b,unsigned frame,unsigned c,int value){
    const unsigned off=frame*b.align+c*(b.bits/8);
    if(b.bits==8){b.bytes[off]=uint8_t(value);return;}
    const uint16_t w=uint16_t(value);b.bytes[off]=uint8_t(w);b.bytes[off+1]=uint8_t(w>>8);
}
int main(){
    {
        Buffers b{};b[0]=voice(2048);for(unsigned i=0;i<2048;++i){put(b[0],i,0,10000);put(b[0],i,1,-10000);}
        Block output{};Commit pending{};assert(prepare(b,output,pending)==1 && b[0].phase==0);
        for(unsigned i=0;i<1024;++i)assert(output[2*i]==10000 && output[2*i+1]==-10000);
        positions(b,0);assert(b[0].position==0);commit(b,pending);positions(b,1024);assert(b[0].position==0);
        positions(b,512);assert(b[0].position==2048 && write_cursor(b[0],512)==4);
        positions(b,0);assert(b[0].position==4096);puts("PASS stereo PCM, transactional submission and native queue cursor estimate");
    }
    {
        Buffers b{};b[0]=voice(1,1,8);b[0].looping=true;put(b[0],0,0,255);
        Block output{};Commit pending{};prepare(b,output,pending);for(auto v:output)assert(v==32512);
        b[0].bytes[0]=0;prepare(b,output,pending);for(auto v:output)assert(v==-32768);
        b[0].bytes[0]=128;prepare(b,output,pending);for(auto v:output)assert(v==0);
        puts("PASS unsigned 8-bit mono extremes, silence and looping interpolation");
    }
    {
        Buffers b{};b[0]=voice(44100,1,16,44100);b[0].looping=true;
        for(unsigned i=0;i<44100;++i)put(b[0],i,0,int(i%30000));
        Block output{};Commit pending{};prepare(b,output,pending);
        for(unsigned i=0;i<1024;++i)assert(output[2*i]==int(uint64_t(i)*44100/48000) && output[2*i]==output[2*i+1]);
        for(unsigned i=0;i<375;++i){prepare(b,output,pending);commit(b,pending);}
        assert(b[0].phase==uint64_t(375)*1024*44100);
        positions(b,0);assert(b[0].position==0);puts("PASS 44.1 to 48 kHz linear conversion and exact eight-second integer phase without drift");
    }
    {
        Buffers b{};b[0]=voice(2,1);put(b[0],0,0,12000);put(b[0],1,0,-12000);b[0].looping=true;
        Block output{};Commit pending{};prepare(b,output,pending);
        for(unsigned i=0;i<1024;++i)assert(output[2*i]==(i%2?-12000:12000));
        commit(b,pending);positions(b,0);assert(b[0].playing && b[0].position==0);
        b[0].looping=false;seek(b[0],0);prepare(b,output,pending);
        assert(output[0]==12000 && output[2]==-12000);for(unsigned i=4;i<output.size();++i)assert(output[i]==0);
        commit(b,pending);positions(b,1024);assert(b[0].playing);positions(b,0);assert(!b[0].playing && b[0].position==0);
        puts("PASS ring wrap, non-looping tail silence and natural completion after queue drains");
    }
    {
        Buffers b{};b[0]=voice(4096);Block output{};Commit pending{};prepare(b,output,pending);
        seek(b[0],64);commit(b,pending);assert(b[0].phase==uint64_t(16)*48000);
        prepare(b,output,pending);stop(b[0]);commit(b,pending);assert(!b[0].playing && b[0].position==64);
        b[0].playing=true;prepare(b,output,pending);b[0].refs=0;b[0].bytes.clear();commit(b,pending);
        assert(b[0].position==64);puts("PASS seek, Stop and release epochs invalidate pending cursor commits");
    }
    {
        Buffers b{};b[0]=voice(1);b[0].looping=true;put(b[0],0,0,30000);put(b[0],0,1,-30000);b[1]=b[0];
        Block output{};Commit pending{};prepare(b,output,pending);assert(output[0]==32767 && output[1]==-32768);
        b[1].playing=false;b[0].volume=-2000;prepare(b,output,pending);assert(output[0]>=2999 && output[0]<=3001);
        b[0].volume=0;b[0].pan=10000;prepare(b,output,pending);assert(output[0]==0 && output[1]==-30000);
        b[0].pan=-10000;prepare(b,output,pending);assert(output[0]==30000 && output[1]==0);
        b[0].pan=0;b[0].volume=-10000;prepare(b,output,pending);for(auto v:output)assert(v==0);
        puts("PASS multivoice saturation, logarithmic volume, pan and mute");
    }
    {
        Buffers b{};b[0]=voice(10000);b[0].looping=true;seek(b[0],4000);
        positions(b,2048);assert(b[0].position==4000);
        b[0].hz=200000;Block output{};Commit pending{};prepare(b,output,pending);commit(b,pending);
        assert(b[0].phase==uint64_t(1000)*48000+uint64_t(1024)*200000);
        positions(b,0);assert(b[0].position==21064);puts("PASS seek anchor and supported DirectSound frequency changes");
    }
}
