#include "runtime/cpu.h"
#include "runtime/pe_image.h"
#include "../vita/src/startup_services.h"
#include <cassert>
#include <cstring>
#include <initializer_list>

uint32_t wx86_cur_tib() { return 0x00730000; }
void wx86_set_lasterr(d2rt::Cpu& c, uint32_t v) { assert(c.write(wx86_cur_tib()+0x34, &v, 4)); }
uint32_t wx86_get_lasterr(d2rt::Cpu& c) {
    uint32_t value = 0; assert(c.read(wx86_cur_tib()+0x34, &value, 4)); return value;
}
int main() {
    d2rt::Cpu cpu;
    d2rt::PeImage image;
    FILE* log = tmpfile(); assert(log);
    StartupServices services(cpu, image, log);
    auto call = [&](const char* name, std::initializer_list<uint32_t> args) {
        const uint32_t sp = 0x009FE000, ret = 0x00650000, canary = 0x12345678;
        cpu.registers[d2rt::R_ESP] = sp;
        cpu.registers[d2rt::R_EBX] = 0x1357;
        assert(cpu.write(sp, &ret, 4));
        uint32_t p = sp+4;
        for (uint32_t value : args) { assert(cpu.write(p, &value, 4)); p += 4; }
        assert(cpu.write(p, &canary, 4));
        d2rt::ImportRef imp{"KERNEL32.dll", name};
        const auto result = services.call(imp);
        if (result == StartupServiceResult::Serviced) {
            assert(cpu.reg(d2rt::R_ESP) == p && cpu.reg(d2rt::R_EIP) == ret);
            assert(cpu.reg(d2rt::R_EBX) == 0x1357);
            uint32_t intact = 0; assert(cpu.read(p, &intact, 4) && intact == canary);
        } else assert(cpu.reg(d2rt::R_ESP) == sp);
        return result;
    };
    for (auto name : {"FlsAlloc", "FlsGetValue", "FlsSetValue", "FlsFree"}) {
        assert(cpu.write(0x00661500, name, std::strlen(name)+1));
        assert(call("GetProcAddress", {0x00AB1000, 0x00661500}) == StartupServiceResult::Serviced);
        assert(cpu.reg(d2rt::R_EAX) == 0 && wx86_get_lasterr(cpu) == 127);
    }
    const char unknown[] = "UnknownExport";
    cpu.write(0x00661500, unknown, sizeof(unknown));
    assert(call("GetProcAddress", {0x00AB1000, 0x00661500}) == StartupServiceResult::Unsupported);
    assert(call("TlsAlloc", {}) == StartupServiceResult::Serviced && cpu.reg(d2rt::R_EAX) == 0);
    assert(call("TlsGetValue", {0}) == StartupServiceResult::Serviced && cpu.reg(d2rt::R_EAX) == 0);
    assert(call("TlsSetValue", {0, 0x00680000}) == StartupServiceResult::Serviced && cpu.reg(d2rt::R_EAX) == 1);
    uint32_t slot = 0; assert(cpu.read(wx86_cur_tib()+0xE10, &slot, 4) && slot == 0x00680000);
    call("TlsGetValue", {0}); assert(cpu.reg(d2rt::R_EAX) == slot && wx86_get_lasterr(cpu) == 0);
    call("TlsFree", {0}); assert(cpu.reg(d2rt::R_EAX) == 1);
    call("TlsSetValue", {0, 123}); assert(cpu.reg(d2rt::R_EAX) == 0 && wx86_get_lasterr(cpu) == 87);
    for (uint32_t i = 0; i < 64; ++i) {
        call("TlsAlloc", {}); assert(cpu.reg(d2rt::R_EAX) == i);
    }
    call("TlsGetValue", {0}); assert(cpu.reg(d2rt::R_EAX) == 0); // reused slot cleared
    call("TlsAlloc", {}); assert(cpu.reg(d2rt::R_EAX) == 0xFFFFFFFF && wx86_get_lasterr(cpu) == 259);
    uint32_t dirty[19]; for (auto& word : dirty) word = 0xA5A5A5A5;
    cpu.write(0x00680000, dirty, sizeof(dirty));
    assert(call("GetStartupInfoA", {0x00680004}) == StartupServiceResult::Serviced);
    uint32_t result[19] = {}; cpu.read(0x00680000, result, sizeof(result));
    assert(result[0] == 0xA5A5A5A5 && result[18] == 0xA5A5A5A5);
    assert(result[1] == 68);
    for (unsigned i = 2; i < 18; ++i) assert(result[i] == 0);
    assert(call("GetStartupInfoA", {0x013FFFF0}) == StartupServiceResult::ContractFailure);
    assert(call("GetStartupInfoA", {0}) == StartupServiceResult::ContractFailure);
    for (uint32_t selector : {0xFFFFFFF6u, 0xFFFFFFF5u, 0xFFFFFFF4u}) {
        assert(call("GetStdHandle", {selector}) == StartupServiceResult::Serviced);
        assert(cpu.reg(d2rt::R_EAX) == 0);
    }
    call("GetStdHandle", {123}); assert(cpu.reg(d2rt::R_EAX) == 0xFFFFFFFFu);
    assert(call("GetFileType", {0}) == StartupServiceResult::Serviced);
    assert(cpu.reg(d2rt::R_EAX) == 0 && wx86_get_lasterr(cpu) == 6);
    assert(call("GetFileType", {0x1234}) == StartupServiceResult::Unsupported);
    const char command[] = "\"C:\\TH075\\TH075.exe\"";
    cpu.write(0x00732000, command, sizeof(command));
    assert(call("GetCommandLineA", {}) == StartupServiceResult::Serviced);
    assert(cpu.reg(d2rt::R_EAX) == 0x00732000);
    char stored[sizeof(command)]; cpu.read(cpu.reg(d2rt::R_EAX), stored, sizeof(stored));
    assert(std::strcmp(stored, command) == 0);
    fclose(log);
}
