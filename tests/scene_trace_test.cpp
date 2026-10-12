#include "../vita/src/scene_trace.h"
#include <cassert>
#include <string>

int main() {
    auto* file=std::tmpfile();assert(file);th075::SceneTrace trace;
    trace.observe(0,0,0,0,0,file);
    trace.observe(0xC06B70,0x657BE0,0x4255D0,0x425620,1,file);
    trace.observe(0xC06B70,0x657BE0,0x4255D0,0x425620,2,file);
    trace.observe(0xC0F220,0x658308,0x43A270,0x43A7A0,3,file);
    trace.observe(0xC20000,0x658400,0x43B000,0x43C000,4,file);
    trace.observe(0xC21000,0x658400,0x43B000,0x43C000,5,file);
    trace.log(file);std::rewind(file);std::string text;char line[512];
    while(std::fgets(line,sizeof(line),file))text+=line;
    std::fclose(file);
    assert(text.find("frame:2 ")==std::string::npos);
    puts("PASS unchanged scene owner produces no duplicate handoff record");
    assert(text.find("from_vtable:0x00658308 object:0x00C20000")!=std::string::npos);
    assert(text.find("observations:5 changes:3 title_handoffs:1")!=std::string::npos);
    puts("PASS title departure and regenerated owner counted independently from playable combat");
    assert(text.find("combat:unverified")!=std::string::npos && text.find("frame:0 ")==std::string::npos);
    puts("PASS invalid observation excluded and summary retains physical-playability limitation");
}
