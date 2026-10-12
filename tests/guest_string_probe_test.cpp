#include "../vita/src/guest_string_probe.h"
#include <cassert>
#include <cstdio>
#include <vector>
using guest_string_probe::Status;
int main() {
    unsigned calls=0;
    const auto never=[&](uint32_t,uint8_t&){++calls;return false;};
    assert(guest_string_probe::observe(0,never).status==Status::Null);
    for(uint32_t a:{1u,0xFFFFu,0x02000000u,0xFFFFFFFFu})
        assert(guest_string_probe::observe(a,never).status==Status::OutOfRange);
    assert(calls==0);puts("PASS null and invalid guest pointers rejected without a read");
    const uint8_t cp932[]={0x82,0xA0,0x82,0xA2,0};
    const auto snapshot=guest_string_probe::observe(0x10000,[&](uint32_t a,uint8_t& b){
        assert(a>=0x10000 && a-0x10000<sizeof(cp932));b=cp932[a-0x10000];return true;});
    assert(snapshot.status==Status::Terminated && snapshot.length==4);
    for(size_t i=0;i<snapshot.length;++i)assert(snapshot.bytes[i]==cp932[i]);
    puts("PASS terminated raw CP932 bytes preserved without text execution or decoding");
    calls=0;
    auto s=guest_string_probe::observe(0x10000,[&](uint32_t,uint8_t& b){b='a';return ++calls<4;});
    assert(s.status==Status::Unreadable && s.length==3 && calls==4);
    puts("PASS failed read keeps only the valid prefix");
    calls=0;s=guest_string_probe::observe(0x01FFFFFF,[&](uint32_t a,uint8_t& b){
        assert(a==0x01FFFFFF);++calls;b='a';return true;});
    assert(s.status==Status::OutOfRange && s.length==1 && calls==1);
    puts("PASS arena end rejected before crossing the guest span");
    calls=0;s=guest_string_probe::observe(0x10000,[&](uint32_t,uint8_t& b){++calls;b='a';return true;});
    assert(s.status==Status::Truncated && s.length==256 && calls==256);
    puts("PASS unterminated string bounded to 256 bytes");
}
