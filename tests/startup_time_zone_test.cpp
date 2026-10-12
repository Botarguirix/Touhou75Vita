#include "../vita/src/startup_time_zone.h"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <limits>
#include <vector>
using namespace startup_time_zone;

struct HostCpu final:d2rt::Cpu {
    std::vector<uint8_t> bytes=std::vector<uint8_t>(0x02000000,0xA5);
    std::array<uint32_t,d2rt::R_COUNT> regs{};
    bool read_fault=false,write_fault=false,corrupt_readback=false;
    unsigned reads=0,writes=0;
    bool map(uint32_t,uint32_t,const void*,int)override{return false;}
    bool protect(uint32_t,uint32_t,int)override{return false;}
    bool read(uint32_t a,void* p,uint32_t n)override {
        ++reads;if(read_fault || uint64_t(a)+n>bytes.size())return false;
        std::memcpy(p,bytes.data()+a,n);
        if(corrupt_readback && reads==2)static_cast<uint8_t*>(p)[0]^=1;
        return true;
    }
    bool write(uint32_t a,const void* p,uint32_t n)override {
        ++writes;if(write_fault || uint64_t(a)+n>bytes.size())return false;
        std::memcpy(bytes.data()+a,p,n);return true;
    }
    uint32_t reg(int r)override{return regs[r];}
    void set_reg(int r,uint32_t value)override{regs[r]=value;}
    void set_trap(uint32_t,uint32_t,d2rt::TrapFn)override{assert(false);}
    bool run(uint32_t,const char**)override{assert(false);return false;}
    uint32_t fault_addr()const override{return 0;}
};
constexpr uint32_t output=0x0068E498,stack=0x009FEBA8,ret=0x00641000;
constexpr uint64_t minute=60000000;
static Snapshot snapshot(int local_minutes=0) {
    const uint64_t utc=1000000000000ull;
    const uint64_t delta=uint64_t(local_minutes<0?-local_minutes:local_minutes)*minute;
    return {utc,local_minutes<0?utc-delta:utc+delta,1000000,0};
}
static void registers(HostCpu& c) {
    for(unsigned i=0;i<c.regs.size();++i)c.regs[i]=0xAABB0000+i;
    c.set_reg(d2rt::R_ESP,stack);c.set_reg(d2rt::R_EIP,0x00B006E0);
    c.set_reg(d2rt::R_EFLAGS,0x202);
}
static void abi(const HostCpu& c,const std::array<uint32_t,d2rt::R_COUNT>& before,uint32_t value) {
    for(unsigned i=0;i<c.regs.size();++i)
        assert(c.regs[i]==(i==d2rt::R_EAX?value:i==d2rt::R_ESP?stack+8:i==d2rt::R_EIP?ret:before[i]));
}
int main() {
    std::array<uint8_t,size> bytes{};int32_t bias=0;
    assert(size==172 && standard_name==4 && standard_date==68 && standard_bias==84 && daylight_name==88 && daylight_date==152 && daylight_bias==168);
    assert(encode(snapshot(330),bytes,bias) && bias==-330);
    for(unsigned i=4;i<size;++i)assert(bytes[i]==0);
    assert(bytes[0]==0xB6 && bytes[1]==0xFE && bytes[2]==0xFF && bytes[3]==0xFF);
    puts("PASS explicit 172-byte PE32 layout empty UTF16 names and disabled DST dates");

    for(int minutes:{-1440,-720,-360,0,330,345,840,1440}) {
        assert(encode(snapshot(minutes),bytes,bias) && bias==-minutes);
        const uint32_t word=uint32_t(bytes[0])|(uint32_t(bytes[1])<<8)|(uint32_t(bytes[2])<<16)|(uint32_t(bytes[3])<<24);
        assert(word==uint32_t(-minutes));
    }
    puts("PASS UTC plus positive negative quarter-hour and bounded endpoint offsets");

    auto sample=snapshot();sample.resolution=0;assert(!encode(sample,bytes,bias));
    sample=snapshot();sample.local+=1;assert(!encode(sample,bytes,bias));
    assert(!encode(snapshot(1441),bytes,bias) && !encode(snapshot(-1441),bytes,bias));
    sample=snapshot();sample.rc=-1;assert(!encode(sample,bytes,bias));
    puts("PASS invalid RTC resolution fractional minutes and out-of-profile offsets rejected");

    {
        HostCpu c;registers(c);const auto before=c.regs;unsigned calls=0,error_calls=0;uint32_t error=123;
        const auto reply=invoke(c,output,ret,[&]{++calls;return snapshot(-360);},[&](uint32_t e){++error_calls;error=e;return true;});
        assert(reply.status==Status::Success && reply.value==unknown && reply.bias==360 && calls==1 && !error_calls && error==123);
        abi(c,before,unknown);assert(c.bytes[output-1]==0xA5 && c.bytes[output+size]==0xA5);
        assert(c.bytes[output]==0x68 && c.bytes[output+1]==1);
        for(unsigned i=4;i<size;++i)assert(c.bytes[output+i]==0);
        puts("PASS physical95 output address and actual stdcall epilogue preserve registers flags and last error");
    }
    for(bool rtc_failure:{false,true}) {
        HostCpu c;registers(c);const auto before=c.regs;uint32_t error=123;
        const auto reply=invoke(c,output,ret,[&]{auto s=snapshot();if(rtc_failure)s.rc=-123;else s.resolution=0;return s;},[&](uint32_t e){error=e;return true;});
        assert(reply.status==(rtc_failure?Status::RtcFailure:Status::InvalidOffset) && reply.value==invalid && !c.writes && error==clock_error);
        abi(c,before,invalid);for(unsigned i=0;i<size;++i)assert(c.bytes[output+i]==0xA5);
    }
    puts("PASS native clock failures return INVALID with real error and preserve all output bytes");

    {
        HostCpu c;
        for(uint32_t a:{0u,0xFFFFu,0x02000000-size+1,0xFFFFFFF0u}) {
            registers(c);const auto before=c.regs;unsigned calls=0;
            const auto reply=invoke(c,a,ret,[&]{++calls;return snapshot();},[](uint32_t){assert(false);return false;});
            assert(reply.status==Status::OutputFault && !calls && !c.writes && c.regs==before);
        }
        c.read_fault=true;registers(c);const auto before=c.regs;
        assert(invoke(c,output,ret,[]{assert(false);return Snapshot{};},[](uint32_t){return false;}).status==Status::OutputFault);
        assert(c.regs==before);
        puts("PASS null low wrapped end-crossing and unmapped outputs stop before RTC or epilogue");
    }
    for(bool corrupt:{false,true}) {
        HostCpu c;registers(c);const auto before=c.regs;c.write_fault=!corrupt;c.corrupt_readback=corrupt;
        const auto reply=invoke(c,output,ret,[]{return snapshot();},[](uint32_t){assert(false);return false;});
        assert(reply.status==Status::OutputFault && c.regs==before);
    }
    puts("PASS failed write and mismatched output readback refuse guest continuation");

    {
        HostCpu c;registers(c);const auto before=c.regs;
        const auto reply=invoke(c,output,ret,[]{auto s=snapshot();s.rc=-1;return s;},[](uint32_t){return false;});
        assert(reply.status==Status::OutputFault && c.regs==before && !c.writes);
        puts("PASS failed last-error publication remains a contract boundary");
    }
    {
        const auto top=std::numeric_limits<uint64_t>::max();
        assert(encode({top,top-minute,1000000,0},bytes,bias) && bias==1);
        assert(encode({top-minute,top,1000000,0},bytes,bias) && bias==-1);
        assert(!encode({0,top,1000000,0},bytes,bias));
        puts("PASS extreme unsigned RTC ticks avoid subtraction multiplication and narrowing overflow");
    }
    {
        HostCpu c;registers(c);const auto before=c.regs;const uint32_t end=0x02000000-size;
        assert(invoke(c,end,ret,[]{return snapshot();},[](uint32_t){return false;}).status==Status::Success);
        abi(c,before,unknown);assert(c.bytes[end-1]==0xA5 && c.bytes.back()==0);
        puts("PASS final complete guest span is admitted with exact buffer extent");
    }
}
