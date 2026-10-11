#include "../vita/src/startup_message_queue.h"
#include <cassert>
#include <cstdio>
#include <cstring>

using namespace startup_messages;
int main() {
    constexpr uint32_t tib=0x00730000, hwnd=0x00AB7000;
    Query query{0x009FEE70,0,0,0,0}; // Physical 73's main-loop query.
    Message output{0xAA,0xBB,0xCC,0xDD,0xEE,-1,-2};
    const Message guard=output;
    unsigned writes=0;
    auto write=[&](uint32_t pointer,const Message& message) {
        assert(pointer==query.output);++writes;output=message;return true;
    };
    Queue queue;
    assert(queue.peek(tib,query,write)==Result::Unsupported);
    assert(!queue.bind(0,hwnd) && !queue.bind(tib,0));
    assert(queue.bind(tib,hwnd));
    for(uint32_t flags=0;flags<=3;++flags) {
        query.flags=flags;
        assert(queue.peek(tib,query,write)==Result::Empty);
        assert(writes==0 && !std::memcmp(&output,&guard,sizeof(guard)));
    }
    puts("PASS empty Peek retains MSG, including PM_REMOVE/PM_NOYIELD");

    query.flags=0;
    Message window{hwnd,0x8001,0x1234,0x5678,123,-17,42};
    Message thread{0,0x8002,9,8,124,1,2};
    assert(queue.post(window) && queue.post(thread));
    assert(queue.bind(tib,hwnd) && queue.size()==2);
    assert(!queue.bind(tib+0x1000,hwnd));
    assert(queue.peek(tib,query,write)==Result::Available);
    assert(!std::memcmp(&output,&window,sizeof(window)) && queue.size()==2);
    query.flags=1;
    assert(queue.peek(tib,query,write)==Result::Available && queue.size()==1);
    assert(queue.peek(tib,query,write)==Result::Available && queue.size()==0);
    assert(!std::memcmp(&output,&thread,sizeof(thread)));
    puts("PASS FIFO order, rebind retention, PM_NOREMOVE and PM_REMOVE");

    assert(queue.post(window) && queue.post(thread));
    query.hwnd=0xFFFFFFFFu;query.flags=0;
    assert(queue.peek(tib,query,write)==Result::Available && output.hwnd==0);
    query.hwnd=hwnd;
    assert(queue.peek(tib,query,write)==Result::Available && output.hwnd==hwnd);
    query.hwnd=0;query.minimum=0x8002;query.maximum=0x8002;
    assert(queue.peek(tib,query,write)==Result::Available && output.message==0x8002);
    query.minimum=query.maximum=0x9000;
    const auto before=writes;
    assert(queue.peek(tib,query,write)==Result::Empty && writes==before && queue.size()==2);
    puts("PASS HWND/thread and inclusive message-range filtering");

    assert(queue.post(Message{0,0x12,7,0,125,0,0}));
    query.flags=1;
    assert(queue.peek(tib,query,write)==Result::Available && output.message==0x12);
    assert(output.wparam==7 && queue.size()==2);
    puts("PASS WM_QUIT bypasses range filter");

    query={0x009FEE70,0,0,0,1};
    assert(queue.peek(tib,query,[](uint32_t,const Message&){return false;})==Result::OutputFault);
    assert(queue.size()==2);
    query.output=0;
    assert(queue.peek(tib,query,write)==Result::OutputFault && queue.size()==2);
    query.output=0xFFFFFFF0u;
    assert(queue.peek(tib,query,write)==Result::OutputFault && queue.size()==2);
    query.output=0x009FEE70;
    assert(queue.peek(tib+0x1000,query,write)==Result::Unsupported);
    query.hwnd=hwnd+4;
    assert(queue.peek(tib,query,write)==Result::Unsupported);
    query.hwnd=0;query.flags=0x10000;
    assert(queue.peek(tib,query,write)==Result::Unsupported);
    query.flags=4;
    assert(queue.peek(tib,query,write)==Result::Unsupported);
    query.flags=0;query.minimum=10;query.maximum=9;
    assert(queue.peek(tib,query,write)==Result::Unsupported);
    query.minimum=0x10000;query.maximum=0x10000;
    assert(queue.peek(tib,query,write)==Result::Unsupported);
    assert(queue.size()==2);
    puts("PASS copy-failure, invalid pointers, foreign thread/window and unsupported flags preserve queue");

    Queue full;assert(full.bind(tib,hwnd));
    assert(!full.post(Message{hwnd+4,0x8001,0,0,0,0,0}));
    assert(!full.post(Message{hwnd,0x12,0,0,0,0,0}));
    assert(!full.post(Message{0,0x10000,0,0,0,0,0}));
    for(unsigned i=0;i<32;++i)assert(full.post(Message{hwnd,0x8001,i,0,0,0,0}));
    assert(!full.post(window) && full.size()==32);
    query={0x009FEE70,0,0,0,1};
    for(unsigned i=0;i<32;++i) {
        assert(full.peek(tib,query,write)==Result::Available && output.wparam==i);
    }
    assert(full.peek(tib,query,write)==Result::Empty);
    puts("PASS bounded capacity and rejected producers leave existing events intact");
}
