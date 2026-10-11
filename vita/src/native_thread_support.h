#pragma once
#include <mutex>
#include <cstdio>
#include <system_error>

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
    fflush(log);return true;
}
