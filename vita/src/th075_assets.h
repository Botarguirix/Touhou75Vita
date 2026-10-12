#pragma once
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

// Independent reader for the observed PAK1 archive and 24/32-bit RLE frame.
// Format references are listed in docs/STATUS_ITERATION45.md.
namespace th075 {
inline std::vector<uint32_t> title_preview;
inline bool load_title_asset(FILE* log) {
    title_preview.clear();
    FILE* file=fopen("ux0:data/TH075Vita/th075.dat","rb");
    if(!file) {fprintf(log,"dat_title_result=archive_missing_optional\n");return false;}
    struct Close {FILE* file;~Close(){fclose(file);}} close{file};
    auto read=[&](void* p,size_t n){return fread(p,1,n,file)==n;};
    auto u32=[](const uint8_t* p){return uint32_t(p[0])|uint32_t(p[1])<<8|uint32_t(p[2])<<16|uint32_t(p[3])<<24;};
    uint8_t count_bytes[2];
    if(fseek(file,0,SEEK_END))return false;
    const long length=ftell(file);
    if(length<2 || fseek(file,0,SEEK_SET) || !read(count_bytes,2))return false;
    const unsigned count=count_bytes[0]|unsigned(count_bytes[1])<<8;
    if(!count || count>4096 || 2u+108u*count>uint64_t(length))return false;
    std::vector<uint8_t> table(count*108u);
    if(!read(table.data(),table.size()))return false;
    uint8_t key=0x64,delta=0x64;
    for(auto& byte:table) {byte^=key;key=uint8_t(key+delta);delta=uint8_t(delta+0x4D);}
    uint32_t offset=0,size=0;
    for(unsigned i=0;i<count;++i) {
        const uint8_t* record=table.data()+108*i;
        if(!std::memchr(record,0,100))return false;
        const uint32_t item_size=u32(record+100),item_offset=u32(record+104);
        if(item_offset<2+108u*count || uint64_t(item_offset)+item_size>uint64_t(length))return false;
        if(!std::strcmp(reinterpret_cast<const char*>(record),"data\\system\\title.dat")) {
            offset=item_offset;size=item_size;
        }
    }
    fprintf(log,"dat_archive_entries=%u\n",count);
    if(!offset || size<18 || fseek(file,long(offset),SEEK_SET))return false;
    uint8_t header[18];
    if(!read(header,18) || header[0]!=0 || u32(header+1)!=640 || u32(header+5)!=480 ||
       u32(header+9)!=640 || (header[13]!=24 && header[13]!=32))return false;
    const uint32_t compressed=u32(header+14);
    if(!compressed || compressed%8 || uint64_t(compressed)+18>size || compressed>8u*1024u*1024u)return false;
    title_preview.resize(640u*480u);
    unsigned pixel=0;
    for(uint32_t consumed=0;consumed<compressed;consumed+=8) {
        uint8_t pair[8];
        if(!read(pair,8)) {title_preview.clear();return false;}
        const uint32_t run=u32(pair);
        if(!run || run>title_preview.size()-pixel) {title_preview.clear();return false;}
        // Fresh original upload confirms RGB black is transparent in 24-bit frames.
        const uint32_t alpha=header[13]==32 ? uint32_t(pair[7])<<24:
            ((pair[4]|pair[5]|pair[6])?0xFF000000u:0u);
        const uint32_t color=alpha|
            uint32_t(pair[4])<<16|uint32_t(pair[5])<<8|pair[6];
        for(uint32_t n=0;n<run;++n)title_preview[pixel++]=color;
    }
    if(pixel!=title_preview.size()) {title_preview.clear();return false;}
    fprintf(log,"dat_title_source=data/system/title.dat\n");
    fprintf(log,"dat_title_dimensions=640x480\ndat_title_frame=0\n");
    fprintf(log,"dat_title_compressed_bytes=%u\ndat_title_pixels=%u\n",compressed,pixel);
    fprintf(log,"dat_title_result=decoded_original_archive_frame\n");
    fprintf(log,"dat_title_scope=asset_preview_not_exe_renderer\n");
    fprintf(log,"dat_title_alpha_policy=%s\n",header[13]==24?"verified_black_color_key":"stored_alpha");
    return true;
}
}
