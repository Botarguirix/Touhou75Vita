#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

// A single owned thread/window's posted-message queue. Sent callbacks and paint
// must be completed by StartupServices before consulting this queue. No host
// keyboard, timers or external Windows messages are synthesized here.
namespace startup_messages {
struct Message {
    uint32_t hwnd=0, message=0, wparam=0, lparam=0, time=0;
    int32_t x=0, y=0;
};
static_assert(sizeof(Message)==28, "32-bit Windows MSG public payload");
enum class Result { Empty, Available, Unsupported, OutputFault };
struct Query {
    uint32_t output, hwnd, minimum, maximum, flags;
};
class Queue {
public:
    bool bind(uint32_t tib,uint32_t hwnd) {
        if(!tib || !hwnd || hwnd==0xFFFFFFFFu)return false;
        if(tib_)return tib_==tib && hwnd_==hwnd; // Never reset queued messages.
        tib_=tib;hwnd_=hwnd;return true;
    }
    size_t size() const { return count_; }
    uint32_t owner_tib() const { return tib_; }
    // Reserved for an explicit producer. The current Vita integration does not
    // call this: unsupported posting/input APIs remain boundaries in the EXE.
    bool post(const Message& message) {
        if(!tib_ || count_==messages_.size() || message.message>0xFFFFu ||
           (message.hwnd && message.hwnd!=hwnd_) ||
           (message.message==0x12 && message.hwnd))return false;
        messages_[count_++]=message;return true;
    }
    template<class Write>
    Result peek(uint32_t tib,const Query& query,Write write) {
        if(!tib_ || tib!=tib_ ||
           (query.hwnd && query.hwnd!=hwnd_ && query.hwnd!=0xFFFFFFFFu) ||
           query.minimum>0xFFFFu || query.maximum>0xFFFFu ||
           query.minimum>query.maximum || (query.flags&~3u))return Result::Unsupported;
        if(!query.output || uint64_t(query.output)+sizeof(Message)>0x100000000ull)
            return Result::OutputFault;
        for(size_t i=0;i<count_;++i) {
            const auto& message=messages_[i];
            const bool quit=message.message==0x12;
            const bool window=!query.hwnd || query.hwnd==message.hwnd ||
                (query.hwnd==0xFFFFFFFFu && !message.hwnd);
            const bool range=(!query.minimum && !query.maximum) ||
                (message.message>=query.minimum && message.message<=query.maximum);
            if(!quit && (!window || !range))continue;
            // PM_NOREMOVE retains the message. PM_REMOVE commits only after
            // the output copy succeeds, so an invalid guest pointer cannot
            // consume an event. PM_NOYIELD is supported: no idle waiter exists.
            if(!write(query.output,message))return Result::OutputFault;
            if(query.flags&1u) {
                for(size_t j=i+1;j<count_;++j)messages_[j-1]=messages_[j];
                --count_;
            }
            return Result::Available;
        }
        return Result::Empty; // Leave the guest MSG and LastError untouched.
    }
private:
    uint32_t tib_=0, hwnd_=0;
    std::array<Message,32> messages_{};
    size_t count_=0;
};
}
