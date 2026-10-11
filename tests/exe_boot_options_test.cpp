#include "../vita/src/exe_boot_options.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

struct Memory {
    static constexpr uint32_t base=0x00400000;
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x40000,0xA5);
    unsigned reads=0,writes=0;unsigned failed_read=0,failed_write=0,corrupt_read=0;
    bool fail_all_writes=false;
    Memory(){for(const auto& p:exe_boot::menu_patches)std::memcpy(bytes.data()+p.address-base,p.original.data(),p.size);}
    bool read(uint32_t address,void* output,unsigned count){
        ++reads;if(reads==failed_read)return false;
        assert(address>=base && uint64_t(address-base)+count<=bytes.size());
        std::memcpy(output,bytes.data()+address-base,count);
        if(reads==corrupt_read)static_cast<uint8_t*>(output)[0]^=1;
        return true;
    }
    bool write(uint32_t address,const void* input,unsigned count){
        ++writes;if(writes==failed_write || (fail_all_writes && writes>=2))return false;
        assert(reads>=4);assert(address>=base && uint64_t(address-base)+count<=bytes.size());
        std::memcpy(bytes.data()+address-base,input,count);return true;
    }
    exe_boot::Result apply(){return exe_boot::apply_menu_preset(
        [&](uint32_t a,void* p,unsigned n){return read(a,p,n);},
        [&](uint32_t a,const void* p,unsigned n){return write(a,p,n);});}
};
int main(){
    using exe_boot::Result;
    {
        Memory m;const auto before=m.bytes;assert(m.apply()==Result::Applied);
        auto expected=before;unsigned changed=0;
        for(const auto& p:exe_boot::menu_patches)std::memcpy(expected.data()+p.address-Memory::base,p.replacement.data(),p.size);
        assert(m.bytes==expected && m.writes==4);
        for(unsigned i=0;i<m.bytes.size();++i)changed+=m.bytes[i]!=before[i];
        assert(changed==10);
        puts("PASS menu preset changes only ten expected bytes after all fingerprints validate");
    }
    for(const auto& p:exe_boot::menu_patches){
        Memory m;m.bytes[p.address-Memory::base]^=1;const auto before=m.bytes;
        assert(m.apply()==Result::FingerprintMismatch && m.bytes==before && !m.writes);
    }
    puts("PASS each foreign fingerprint rejects the preset before any code writes");
    for(unsigned i=1;i<=4;++i){
        Memory m;m.failed_read=i;const auto before=m.bytes;
        assert(m.apply()==Result::ReadFailure && m.bytes==before && !m.writes);
    }
    puts("PASS preflight read faults preserve the mapped image");
    for(unsigned i=1;i<=4;++i){
        Memory m;m.failed_write=i;const auto before=m.bytes;
        assert(m.apply()==Result::WriteFailure && m.bytes==before);
    }
    puts("PASS failed writes restore all original sites and refuse execution");
    for(unsigned i=5;i<=8;++i){
        Memory m;m.corrupt_read=i;const auto before=m.bytes;
        assert(m.apply()==Result::WriteFailure && m.bytes==before);
    }
    puts("PASS failed replacement readback rolls back without accepting a preset");
    {
        Memory m;m.fail_all_writes=true;
        assert(m.apply()==Result::RollbackFailure);
        puts("PASS unrestorable code is an explicit fatal preset failure");
    }
    const auto& logo=exe_boot::menu_patches[0];const auto& destination=exe_boot::menu_patches[1];
    const auto read32=[](const uint8_t* p){return uint32_t(p[0])|(uint32_t(p[1])<<8)|(uint32_t(p[2])<<16)|(uint32_t(p[3])<<24);};
    assert(read32(logo.original.data()+2)==180 && read32(logo.replacement.data()+2)==0);
    assert(destination.replacement[2]==2 && destination.replacement[3]==0x22);
    for(unsigned i:{2u,3u})assert(read32(exe_boot::menu_patches[i].replacement.data()+3)==0xFFFFFFFFu);
    puts("PASS timed logo threshold, original title transition and unsigned menu age gates modeled");
}
