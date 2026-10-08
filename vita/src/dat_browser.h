#pragma once
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <vector>
#include <string>
#include <array>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <psp2/kernel/processmgr.h>

namespace th075 {
class DatBrowser {
    struct Entry{std::string name;uint32_t offset,size;};
    struct Frame{uint32_t offset,width,height,size;uint8_t depth;};
    struct Index{std::vector<Frame> frames;std::array<uint16_t,256> palette{};bool indexed=false,palettes=false;};
    std::vector<Entry> entries_;
    std::vector<Index> indexes_;
    std::vector<Frame> frames_;
    std::vector<uint32_t> pixels_;
    std::vector<uint32_t> preview_;
    bool preview_ready_=false;
    std::array<uint16_t,256> palette_{};
    FILE* file_=nullptr;unsigned selected_=0,frame_=0;
    uint32_t width_=0,height_=0;bool palette_present_=false;
    static uint32_t u32(const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;}
    static uint16_t u16(const uint8_t* p){return uint16_t(p[0])|uint16_t(p[1])<<8;}
    bool read(uint64_t offset,void* p,size_t bytes){return offset<=0x7FFFFFFF && !fseek(file_,long(offset),SEEK_SET) && fread(p,1,bytes,file_)==bytes;}
    bool container(FILE* log){
        frames_.clear();pixels_.clear();const auto& e=entries_[selected_];
        auto& cached=indexes_[selected_];
        if(cached.indexed){frames_=cached.frames;palette_=cached.palette;palette_present_=cached.palettes;frame_=0;
            fprintf(log,"dat_view_index=cache_hit container:%s\n",e.name.c_str());return decode(log);}
        const uint64_t begin=sceKernelGetProcessTimeWide();
        uint8_t palettes=0;if(!read(e.offset,&palettes,1))return false;
        uint64_t cursor=1+uint32_t(palettes)*512;palette_present_=palettes>0;
        if(cursor>e.size)return false;
        if(palettes){std::array<uint8_t,512> bytes{};if(!read(e.offset+1,bytes.data(),bytes.size()))return false;for(unsigned i=0;i<256;++i)palette_[i]=u16(bytes.data()+i*2);}
        while(cursor<e.size){
            uint8_t h[17]{};
            if(e.size-cursor<17 || !read(uint64_t(e.offset)+cursor,h,17))return false;
            Frame f{uint32_t(cursor+17),u32(h),u32(h+4),u32(h+13),h[12]};
            if(!f.width || !f.height || f.width>4096 || f.height>4096 || uint64_t(f.width)*f.height>4u*1024*1024 ||
               (f.depth!=8 && f.depth!=16 && f.depth!=24 && f.depth!=32) || !f.size || uint64_t(f.offset)+f.size>e.size || frames_.size()>=16384)return false;
            frames_.push_back(f);cursor=uint64_t(f.offset)+f.size;
        }
        fprintf(log,"dat_view_container=%s frames:%u\n",e.name.c_str(),unsigned(frames_.size()));
        if(!frames_.empty()){cached.frames=frames_;cached.palette=palette_;cached.palettes=palette_present_;cached.indexed=true;}
        fprintf(log,"dat_view_index=scan_us:%llu container:%s\n",(unsigned long long)(sceKernelGetProcessTimeWide()-begin),e.name.c_str());
        frame_=0;return !frames_.empty() && decode(log);
    }
    bool decode(FILE* log){
        const uint64_t begin=sceKernelGetProcessTimeWide();
        preview_ready_=false;pixels_.clear();if(frames_.empty())return false;
        const auto& e=entries_[selected_];const auto& f=frames_[frame_];
        const unsigned unit=f.depth>=24?4:f.depth/8;
        if(f.size%(unit*2) || (f.depth==8 && !palette_present_) || f.size>32u*1024*1024)return false;
        if(fseek(file_,long(uint64_t(e.offset)+f.offset),SEEK_SET))return false;
        std::vector<uint32_t> decoded(size_t(f.width)*f.height);size_t pixel=0;
        std::array<uint8_t,8192> input{};uint32_t consumed=0;
        while(consumed<f.size){
            const unsigned bytes=std::min(uint32_t(input.size()),f.size-consumed);
            if(fread(input.data(),1,bytes,file_)!=bytes)return false;
            for(unsigned i=0;i<bytes;i+=unit*2){
                const auto* p=input.data()+i;
                const uint32_t run=unit==4?u32(p):(unit==2?u16(p):p[0]);
                if(!run || run>decoded.size()-pixel)return false;
                uint32_t color=0;
                if(f.depth>=24){
                    const unsigned b=p[4],g=p[5],r=p[6];
                    const unsigned a=f.depth==32?p[7]:((r|g|b)?255:0);
                    color=(a<<24)|(b<<16)|(g<<8)|r;
                }else{
                    const uint16_t value=f.depth==8?palette_[p[1]]:u16(p+2);
                    color=((value&0x8000)?0xFF000000u:0u)|((value&31)*255/31<<16)|(((value>>5)&31)*255/31<<8)|((value>>10)&31)*255/31;
                }
                std::fill_n(decoded.data()+pixel,run,color);pixel+=run;
            }
            consumed+=bytes;
        }
        if(pixel!=decoded.size())return false;
        pixels_.swap(decoded);width_=f.width;height_=f.height;
        const uint64_t decoded_at=sceKernelGetProcessTimeWide();
        prepare_preview();
        fprintf(log,"dat_view_timing=decode_us:%llu preview_us:%llu compressed_bytes:%u rgba_bytes:%u\n",
            (unsigned long long)(decoded_at-begin),(unsigned long long)(sceKernelGetProcessTimeWide()-decoded_at),f.size,unsigned(pixels_.size()*4));
        fprintf(log,"dat_view_frame=%s index:%u dimensions:%ux%u depth:%u pixels:%u\n",e.name.c_str(),frame_,width_,height_,f.depth,unsigned(pixel));fflush(log);return true;
    }
    void prepare_preview(){
        constexpr unsigned w=260,h=180;
        preview_.resize(w*h);
        for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)preview_[y*w+x]=((x/8+y/8)%2)?0xFF484848:0xFF282828;
        unsigned dw=w,dh=uint64_t(height_)*w/width_;if(dh>h){dh=h;dw=uint64_t(width_)*h/height_;}
        dw=std::max(dw,1u);dh=std::max(dh,1u);
        for(unsigned y=0;y<dh;++y)for(unsigned x=0;x<dw;++x){
            const uint32_t src=pixels_[(uint64_t(y)*height_/dh)*width_+uint64_t(x)*width_/dw];
            auto& dst=preview_[((h-dh)/2+y)*w+(w-dw)/2+x];const unsigned a=src>>24;uint32_t rgb=0;
            for(unsigned c=0;c<3;++c)rgb|=((((src>>(8*c))&255)*a+((dst>>(8*c))&255)*(255-a))/255)<<(8*c);
            dst=0xFF000000|rgb;
        }
        preview_ready_=true;
    }
public:
    ~DatBrowser(){if(file_)fclose(file_);}
    bool open(FILE* log){
        file_=fopen("ux0:data/TH075Vita/th075.dat","rb");if(!file_)return false;
        if(fseek(file_,0,SEEK_END))return false;
        const long size=ftell(file_);
        uint8_t b[2]{};if(size<2 || !read(0,b,2))return false;
        const unsigned count=u16(b);if(!count || count>4096 || 2+count*108>uint64_t(size))return false;
        std::vector<uint8_t> table(count*108);if(!read(2,table.data(),table.size()))return false;
        uint8_t key=0x64,delta=0x64;for(auto& v:table){v^=key;key=uint8_t(key+delta);delta=uint8_t(delta+0x4D);}
        for(unsigned i=0;i<count;++i){const auto* p=table.data()+i*108;if(!std::memchr(p,0,100))return false;
            std::string name(reinterpret_cast<const char*>(p));const uint32_t bytes=u32(p+100),offset=u32(p+104);
            if(offset<2+count*108 || uint64_t(offset)+bytes>uint64_t(size))return false;
            bool image=name.compare(0,12,"data\\system\\")==0 && name.find('\\',12)==std::string::npos;
            image=image || name.compare(0,16,"data\\background\\")==0;
            if(name.compare(0,15,"data\\character\\")==0){const auto stem=name.substr(15);image=stem.find('\\')==std::string::npos;}
            if(image && name.size()>4 && name.substr(name.size()-4)==".dat")entries_.push_back({name,offset,bytes});
        }
        fprintf(log,"dat_view_inventory=archive_entries:%u image_containers:%u\n",count,unsigned(entries_.size()));
        indexes_.resize(entries_.size());
        return !entries_.empty() && container(log);
    }
    void next(FILE* log){if(entries_.empty())return;selected_=(selected_+1)%entries_.size();if(!container(log)){pixels_.clear();fprintf(log,"dat_view_decode=unsupported_container\n");}}
    void frame(int step,FILE* log){if(frames_.empty())return;frame_=unsigned((int(frame_)+step+int(frames_.size()))%int(frames_.size()));if(!decode(log)){pixels_.clear();fprintf(log,"dat_view_decode=failed_frame\n");}}
    std::string label()const{return entries_.empty()?"DAT UNAVAILABLE":entries_[selected_].name+" F"+std::to_string(frame_)+"/"+std::to_string(frames_.size());}
    void draw(uint32_t* target)const{
        constexpr unsigned x0=690,y0=228,w=260,h=180;
        if(preview_ready_ && !pixels_.empty())for(unsigned y=0;y<h;++y)std::memcpy(target+(y0+y)*960+x0,preview_.data()+y*w,w*4);
        else for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x)target[(y0+y)*960+x0+x]=((x/8+y/8)%2)?0xFF484848:0xFF282828;
    }
    void export_current(FILE* log){
        if(entries_.empty() || pixels_.empty())return;
        const auto& e=entries_[selected_];std::string flat=e.name;
        for(auto& c:flat)if(!((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='.'))c='_';
        const std::string root="ux0:data/TH075Vita/extracted/";sceIoMkdir("ux0:data/TH075Vita/extracted",0777);
        const std::string raw=root+flat;FILE* out=fopen(raw.c_str(),"wb");bool good=out && !fseek(file_,long(e.offset),SEEK_SET);
        std::array<uint8_t,8192> copy{};uint32_t cursor=0;
        while(good && cursor<e.size){const unsigned n=std::min(uint32_t(copy.size()),e.size-cursor);good=fread(copy.data(),1,n,file_)==n && fwrite(copy.data(),1,n,out)==n;cursor+=n;}
        if(out && fclose(out))good=false;
        const std::string bmp=root+flat+"-"+std::to_string(frame_)+".bmp";out=fopen(bmp.c_str(),"wb");
        std::array<uint8_t,122> header{};header[0]='B';header[1]='M';
        auto put=[&](unsigned p,uint32_t v){for(unsigned i=0;i<4;++i)header[p+i]=v>>(i*8);};
        put(2,122+width_*height_*4);put(10,122);put(14,108);put(18,width_);put(22,uint32_t(-int32_t(height_)));header[26]=1;header[28]=32;
        put(30,3);put(34,width_*height_*4);put(54,0x00FF0000);put(58,0x0000FF00);put(62,0x000000FF);put(66,0xFF000000);put(70,0x73524742);
        bool bitmap=out && fwrite(header.data(),1,header.size(),out)==header.size();std::vector<uint8_t> row(width_*4);
        for(unsigned y=0;bitmap && y<height_;++y){for(unsigned x=0;x<width_;++x){const auto p=pixels_[y*width_+x];row[4*x]=p>>16;row[4*x+1]=p>>8;row[4*x+2]=p;row[4*x+3]=p>>24;}bitmap=fwrite(row.data(),1,row.size(),out)==row.size();}
        if(out && fclose(out))bitmap=false;
        fprintf(log,"dat_view_export=raw:%s raw_result:%s bitmap:%s bitmap_result:%s\n",raw.c_str(),good?"passed":"failed",bmp.c_str(),bitmap?"passed":"failed");fflush(log);
    }
};
}
