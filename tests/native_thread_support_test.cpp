#include "../vita/src/native_thread_support.h"
#include <cassert>
#include <string>
#include <thread>

int main() {
    FILE* log=std::tmpfile();assert(log);
    assert(check_native_thread_support(log));
    std::rewind(log);char bytes[1024]{};
    const auto count=std::fread(bytes,1,sizeof(bytes)-1,log);
    const std::string text(bytes,count);
    assert(text.find("gthread_active:yes")!=std::string::npos);
    assert(text.find("passed mutex:constructed_locked_unlocked")!=std::string::npos);
    assert(text.find("startup_native_thread_once=passed")!=std::string::npos);
    assert(text.find("callbacks:1")!=std::string::npos);
    assert(text.find("startup_native_thread_tls=passed scope:main_before_vm")!=std::string::npos);
    assert(text.find("startup_native_thread_preflight=passed")!=std::string::npos);
    assert(text.find("failed")==std::string::npos);
    std::fclose(log);
    puts("PASS native threading preflight checks GCC activation, mutex, once callback and TLS roundtrip");
    log=std::tmpfile();assert(log);
    assert(check_native_thread_support(log));
    assert(native_thread_detail::once_calls.load()==1);
    std::fclose(log);
    puts("PASS repeated preflight does not rerun pthread_once initialization");
    native_thread_detail::tls_cookie=0x11223344;
    std::atomic<unsigned> ready{0};
    auto isolated=[&](uint32_t token) {
        assert(native_thread_detail::tls_cookie==0x75);
        native_thread_detail::tls_cookie=token;
        ready.fetch_add(1);
        while(ready.load()!=2)std::this_thread::yield();
        assert(native_thread_detail::tls_cookie==token);
        native_thread_detail::tls_cookie=0x75;
        assert(check_native_thread_tls(token));
    };
    std::thread first(isolated,0x757501),second(isolated,0x757502);
    first.join();second.join();
    assert(native_thread_detail::tls_cookie==0x11223344);
    native_thread_detail::tls_cookie=0x75;
    assert(check_native_thread_tls(0x7500));
    puts("PASS main and two worker TLS values remain isolated and restore their original cookie");
}
