#include "../vita/src/native_thread_support.h"
#include <cassert>
#include <string>

int main() {
    FILE* log=std::tmpfile();assert(log);
    assert(check_native_thread_support(log));
    std::rewind(log);char bytes[1024]{};
    const auto count=std::fread(bytes,1,sizeof(bytes)-1,log);
    const std::string text(bytes,count);
    assert(text.find("gthread_active:yes")!=std::string::npos);
    assert(text.find("passed mutex:constructed_locked_unlocked")!=std::string::npos);
    assert(text.find("failed")==std::string::npos);
    std::fclose(log);
    puts("PASS native threading preflight checks GCC activation and actual mutex construction/trylock/unlock");
}
