#pragma once
#include <mutex>
#include <cstdio>
#include <system_error>
#include <atomic>
#include <cstdint>

namespace native_thread_detail {
inline pthread_once_t once=PTHREAD_ONCE_INIT;
inline std::atomic<unsigned> once_calls{0};
inline void initialize_once(){once_calls.fetch_add(1);}
// Volatile ensures the compiler performs the TLS access in the preflight.
// Vita GCC implements this through the same libgcc emutls path as the JIT.
inline thread_local volatile uint32_t tls_cookie=0x75;
}

inline bool check_native_thread_tls(uint32_t token) {
    auto& cookie=native_thread_detail::tls_cookie;
    const bool initial=cookie==0x75;
    cookie=token;const bool retained=cookie==token;cookie=0x75;
    return initial && retained;
}

// Check the same GCC activation predicate that std::mutex uses, before
// starting the VM or any native worker. No thread is cancelled by this check.
inline bool check_native_thread_support(FILE* log) {
    const bool active=__gthread_active_p()!=0;
    fprintf(log,"startup_native_thread_support=gthread_active:%s\n",active?"yes":"no");
    if(!active) {
        fprintf(log,"startup_native_thread_support=failed reason:pthread_activation_missing execution:refused\n");
        fflush(log);return false;
    }
    try {
        std::mutex probe;
        if(!probe.try_lock()) {
            fprintf(log,"startup_native_thread_support=failed reason:mutex_trylock\n");
            fflush(log);return false;
        }
        probe.unlock();
    } catch(const std::system_error& error) {
        fprintf(log,"startup_native_thread_support=failed reason:mutex_exception code:%d\n",error.code().value());
        fflush(log);return false;
    }
    fprintf(log,"startup_native_thread_support=passed mutex:constructed_locked_unlocked\n");
    const int once_first=pthread_once(&native_thread_detail::once,native_thread_detail::initialize_once);
    const int once_second=pthread_once(&native_thread_detail::once,native_thread_detail::initialize_once);
    const bool once_ok=once_first==0 && once_second==0 && native_thread_detail::once_calls.load()==1;
    fprintf(log,"startup_native_thread_once=%s first_rc:%d second_rc:%d callbacks:%u\n",
        once_ok?"passed":"failed",once_first,once_second,native_thread_detail::once_calls.load());
    fflush(log);if(!once_ok)return false;
    fprintf(log,"startup_native_thread_tls=begin scope:main_before_vm\n");fflush(log);
    const bool tls_ok=check_native_thread_tls(0x8875);
    fprintf(log,"startup_native_thread_tls=%s scope:main_before_vm\n",tls_ok?"passed":"failed");
    fprintf(log,"startup_native_thread_preflight=%s\n",tls_ok?"passed":"failed");
    fflush(log);return tls_ok;
}
