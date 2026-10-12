#include "../vita/src/d3d8_present_cache.h"
#include "../vita/src/d3d8_present_frame.h"
#include "../vita/src/exe_draw_preview.h"
#include <cassert>
#include <cstdio>
#include <limits>
#include <vector>

using namespace d3d8_present;
int main() {
    std::vector<uint8_t> bytes(FrameCache::source_bytes,0);
    for(size_t i=0;i<bytes.size();i+=4)
        d3d8_quad::detail::store32(bytes.data()+i,0x0F000000u|uint32_t(i/4));
    d3d8_quad::Image image{bytes.data(),bytes.size(),640,480,2560,22};
    FrameCache cache;unsigned slot=9;uint32_t hash=0x12345678;
    assert(!cache.lookup(image,slot,hash) && slot==9 && hash==0x12345678);
    std::vector<uint32_t> first(width*height),second(width*height);
    uint32_t expected=0,again=0;
    assert(prepare(image,first.data(),first.size(),expected));
    assert(cache.remember_confirmed(image,0,expected));
    assert(cache.lookup(image,slot,hash) && slot==0 && hash==expected);
    auto copy=bytes;image.bytes=copy.data();
    assert(cache.lookup(image,slot,hash)); // Own snapshot; equality ignores pointer identity.
    assert(prepare(image,second.data(),second.size(),again) && first==second && hash==again);
    puts("PASS cold_miss_confirmed_source_exact_copy_and_scanout_equivalence");

    uint32_t absent=0x12345678;
    assert(prepare(image,second.data(),second.size(),absent,false) && first==second && absent==0);
    FrameCache performance_cache;
    assert(performance_cache.remember_confirmed(image,1,absent));
    assert(performance_cache.lookup(image,slot,hash) && slot==1 && hash==0);
    bytes.back()^=1;image.bytes=bytes.data();assert(!performance_cache.lookup(image,slot,hash));bytes.back()^=1;
    image.bytes=copy.data();
    puts("PASS omitted scanout hashes preserve every output pixel and full-byte cache equality");

    image.bytes=bytes.data();
    for(size_t index:{size_t(0),size_t(3),bytes.size()/2,bytes.size()-1}) {
        bytes[index]^=1;
        assert(!cache.lookup(image,slot,hash));
        bytes[index]^=1;assert(cache.lookup(image,slot,hash));
    }
    puts("PASS every_source_region_including_alpha_and_last_pixel_invalidates_hit");

    for(unsigned field=0;field<7;++field) {
        auto bad=image;
        switch(field) {
        case 0:bad.width=639;break;
        case 1:bad.height=479;break;
        case 2:bad.pitch=2564;break;
        case 3:bad.format=21;break;
        case 4:--bad.size;break;
        case 5:++bad.size;break;
        case 6:bad.bytes=nullptr;break;
        }
        assert(!cache.lookup(bad,slot,hash));
    }
    auto wrapped=image;
    wrapped.bytes=reinterpret_cast<const uint8_t*>(std::numeric_limits<uintptr_t>::max()-100);
    assert(!cache.lookup(wrapped,slot,hash));
    assert(!cache.remember_confirmed(wrapped,0,expected));
    assert(!cache.lookup(image,slot,hash));
    assert(cache.remember_confirmed(image,0,expected));
    puts("PASS invalid_metadata_null_short_span_and_pointer_wrap_rejected_before_reads");

    bytes[8]^=1; // Guest source changes after the preceding confirmed Present.
    assert(!cache.lookup(image,slot,hash));
    assert(prepare(image,second.data(),second.size(),again));
    // The presenter selects the inactive slot (confirmed slot xor 1) on a miss.
    const unsigned inactive=slot^1u;
    assert(inactive==1 && cache.remember_confirmed(image,inactive,again));
    assert(cache.lookup(image,slot,hash) && slot==1 && hash==again);
    assert(hash!=expected && first!=second);
    cache.invalidate(); // E.g. native set/wait/query did not confirm scanout.
    assert(!cache.lookup(image,slot,hash));
    assert(!cache.remember_confirmed(image,2,again));
    assert(!cache.lookup(image,slot,hash));
    assert(cache.remember_confirmed(image,1,again));
    assert(cache.lookup(image,slot,hash) && slot==1);
    puts("PASS changed_source_uses_other_slot_and_failed_confirmation_discards_cache");

    FILE* log=std::tmpfile();assert(log);
    th075::capture_first_exe_quad(bytes.data(),bytes.size(),640,480,2560,0,0,640,480,log);
    assert(th075::exe_draw_ready && th075::exe_present_frames==0);
    th075::exe_draw_preview.fill(0); // A first-quad diagnostic cannot qualify for reuse.
    th075::capture_presented_exe_frame(bytes.data(),bytes.size(),640,480,2560,1,log,true);
    assert(th075::exe_present_frames==1 && th075::exe_draw_preview[13*260+1]!=0);
    const auto preview=th075::exe_draw_preview;
    th075::capture_presented_exe_frame(bytes.data(),bytes.size(),640,480,2560,2,log,true);
    assert(th075::exe_present_frames==2 && th075::exe_draw_preview==preview);
    // A miss regenerates the thumbnail rather than retaining an earlier scene.
    for(size_t i=0;i<bytes.size();i+=4)d3d8_quad::detail::store32(bytes.data()+i,0xFF112233);
    th075::capture_presented_exe_frame(bytes.data(),bytes.size(),640,480,2560,3,log,false);
    assert(th075::exe_present_frames==3 && th075::exe_draw_preview!=preview);
    std::fclose(log);
    puts("PASS only_confirmed_identical_preview_reused_and_real_frame_count_advances");
}
