#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>

// Bounds checked PE32 resources and the observed 32x32 indexed icon DIB.
namespace th075 {
inline std::vector<uint32_t> icon_preview;
struct ResourceBlob { uint32_t rva=0,size=0; uint16_t language=0; };
inline bool resource(const std::vector<uint8_t>& image,uint16_t type,
                     const std::string& name,uint16_t ordinal,ResourceBlob& out) {
    auto read=[&](uint64_t offset,void* data,size_t size) {
        if(offset+size>image.size())return false;
        std::memcpy(data,image.data()+offset,size);return true;
    };
    uint32_t nt=0,root=0,length=0;uint16_t magic=0;
    if(!read(60,&nt,4)||!read(uint64_t(nt)+24,&magic,2)||magic!=0x10B||
       !read(uint64_t(nt)+136,&root,4)||!read(uint64_t(nt)+140,&length,4)||
       !root || length<16 || uint64_t(root)+length>image.size())return false;
    auto directory_read=[&](uint32_t offset,void* data,size_t size) {
        return uint64_t(offset)+size<=length && read(uint64_t(root)+offset,data,size);
    };
    uint32_t directory=0;
    for(unsigned depth=0;depth<3;++depth) {
        uint16_t named=0,ids=0;
        if(!directory_read(directory+12,&named,2)||!directory_read(directory+14,&ids,2))return false;
        const unsigned count=unsigned(named)+ids;
        if(!count || count>4096 || uint64_t(directory)+16+8ull*count>length)return false;
        bool found=false;uint32_t next=0;
        for(unsigned index=0;index<count;++index) {
            uint32_t key=0,value=0;
            if(!directory_read(directory+16+8*index,&key,4)||!directory_read(directory+20+8*index,&value,4))return false;
            bool match=false;
            if(depth==0)match=key==type;
            else if(depth==2)match=!(key&0x80000000u); // Prefer neutral/Japanese, deterministic fallback.
            else if(name.empty())match=key==ordinal;
            else if(key&0x80000000u) {
                const uint32_t string_offset=key&0x7FFFFFFFu;uint16_t size=0;
                if(!directory_read(string_offset,&size,2)||size>128)return false;
                match=size==name.size();
                for(unsigned i=0;i<size;++i) {uint16_t ch=0;
                    if(!directory_read(string_offset+2+2*i,&ch,2))return false;
                    match=match && ch==uint8_t(name[i<name.size()?i:0]);
                }
            }
            if(!match)continue;
            if(!found || depth!=2 || key==0 || key==0x411) {next=value;found=true;if(depth==2)out.language=uint16_t(key);}
            if(depth!=2 || key==0 || key==0x411)break;
        }
        if(!found)return false;
        if(depth<2) {if(!(next&0x80000000u))return false;directory=next&0x7FFFFFFFu;}
        else {
            if(next&0x80000000u)return false;
            if(!directory_read(next,&out.rva,4)||!directory_read(next+4,&out.size,4)||!out.size||
               uint64_t(out.rva)+out.size>image.size())return false;
        }
    }
    return true;
}
inline bool decode_icon(const std::vector<uint8_t>& image,const ResourceBlob& blob,
                        std::vector<uint32_t>& pixels) {
    if(blob.size!=2216 || uint64_t(blob.rva)+blob.size>image.size())return false;
    const uint8_t* data=image.data()+blob.rva;
    uint32_t header=0,width=0,height=0,compression=0,colors=0;
    uint16_t planes=0,bpp=0;
    std::memcpy(&header,data,4);std::memcpy(&width,data+4,4);std::memcpy(&height,data+8,4);
    std::memcpy(&planes,data+12,2);std::memcpy(&bpp,data+14,2);
    std::memcpy(&compression,data+16,4);std::memcpy(&colors,data+32,4);
    if(header!=40 || width!=32 || height!=64 || planes!=1 || bpp!=8 || compression || colors!=256)return false;
    pixels.resize(1024);
    for(unsigned y=0;y<32;++y)for(unsigned x=0;x<32;++x) {
        const unsigned row=31-y;
        const unsigned index=data[1064+32*row+x];const uint8_t* color=data+40+4*index;
        const bool transparent=data[2088+4*row+x/8] & (0x80u>>(x%8));
        pixels[32*y+x]=(transparent?0u:0xFF000000u)|uint32_t(color[0])<<16|uint32_t(color[1])<<8|color[2];
    }
    return true;
}
}
