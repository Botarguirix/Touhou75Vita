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
        const uint32_t sp = 0x009FE000, canary = 0x12345678;
        const bool heap_create = std::strcmp(name, "HeapCreate") == 0;
        const uint32_t ret = heap_create ? 0x0064974C : 0x00650000;
        cpu.registers[d2rt::R_ESP] = sp;
        cpu.registers[d2rt::R_EBX] = 0x1357;
        assert(cpu.write(sp, &ret, 4));
        uint32_t p = sp+4;
        for (uint32_t value : args) { assert(cpu.write(p, &value, 4)); p += 4; }
        assert(cpu.write(p, &canary, 4));
        d2rt::ImportRef imp{"KERNEL32.dll", name};
        if (heap_create) imp.iat_va = 0x00657160;
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
    for (uint32_t count : {0u, 32u, 1024u}) {
        assert(call("SetHandleCount", {count}) == StartupServiceResult::Serviced);
        assert(cpu.reg(d2rt::R_EAX) == count);
    }
    assert(call("GetEnvironmentStringsW", {}) == StartupServiceResult::ContractFailure);
    assert(call("HeapCreate", {0, 4096, 0}) == StartupServiceResult::Serviced);
    assert(call("GetEnvironmentStringsW", {}) == StartupServiceResult::Serviced);
    const uint32_t wide = cpu.reg(d2rt::R_EAX);
    uint32_t terminators = 1; assert(cpu.read(wide, &terminators, 4) && terminators == 0);
    call("GetEnvironmentStringsA", {});
    const uint32_t ansi = cpu.reg(d2rt::R_EAX); assert(ansi != wide && ansi);
    call("FreeEnvironmentStringsA", {wide}); assert(cpu.reg(d2rt::R_EAX) == 0);
    call("FreeEnvironmentStringsW", {wide}); assert(cpu.reg(d2rt::R_EAX) == 1);
    call("FreeEnvironmentStringsW", {wide}); assert(cpu.reg(d2rt::R_EAX) == 0);
    call("FreeEnvironmentStringsA", {ansi}); assert(cpu.reg(d2rt::R_EAX) == 1);
    call("FreeEnvironmentStringsA", {0}); assert(cpu.reg(d2rt::R_EAX) == 0);
    call("GetEnvironmentStrings", {}); assert(cpu.reg(d2rt::R_EAX) == ansi); // released copy reused
    const uint16_t text[] = {'A', 0, 'B', 0};
    cpu.write(0x00682000, text, sizeof(text));
    assert(call("WideCharToMultiByte", {0, 0, 0x00682000, 0xFFFFFFFF, 0, 0, 0, 0}) == StartupServiceResult::Serviced);
    assert(cpu.reg(d2rt::R_EAX) == 2); // includes the first terminating NUL
    const uint8_t guard[5] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
    cpu.write(0x00683000, guard, 5);
    assert(call("WideCharToMultiByte", {932, 0, 0x00682000, 3, 0x00683001, 3, 0, 0x00684000}) == StartupServiceResult::Serviced);
    assert(cpu.reg(d2rt::R_EAX) == 3);
    uint8_t converted[5]; cpu.read(0x00683000, converted, 5);
    assert(converted[0] == 0xAA && converted[1] == 'A' && converted[2] == 0 && converted[3] == 'B' && converted[4] == 0xAA);
    uint32_t used = 1; cpu.read(0x00684000, &used, 4); assert(used == 0);
    cpu.write(0x00683000, guard, 5);
    call("WideCharToMultiByte", {0, 0, 0x00682000, 3, 0x00683001, 2, 0, 0});
    assert(cpu.reg(d2rt::R_EAX) == 0 && wx86_get_lasterr(cpu) == 122);
    cpu.read(0x00683000, converted, 5); assert(std::memcmp(converted, guard, 5) == 0);
    call("WideCharToMultiByte", {0, 0, 0x00682000, 0, 0, 0, 0, 0});
    assert(cpu.reg(d2rt::R_EAX) == 0 && wx86_get_lasterr(cpu) == 87);
    const uint16_t empty[2] = {0, 0}; cpu.write(0x00682000, empty, sizeof(empty));
    call("WideCharToMultiByte", {0, 0, 0x00682000, 1, 0, 0, 0, 0});
    assert(cpu.reg(d2rt::R_EAX) == 1); // actual CRT environment size query
    call("WideCharToMultiByte", {0, 0, 0x00682000, 1, 0x00683000, 1, 0, 0});
    assert(cpu.reg(d2rt::R_EAX) == 1); cpu.read(0x00683000, converted, 1); assert(converted[0] == 0);
    assert(call("WideCharToMultiByte", {0, 0, 0x01400000, 1, 0, 0, 0, 0}) == StartupServiceResult::ContractFailure);
    const uint16_t japanese[] = {0x3042, 0}; cpu.write(0x00682000, japanese, sizeof(japanese));
    assert(call("WideCharToMultiByte", {932, 0, 0x00682000, 1, 0, 0, 0, 0}) == StartupServiceResult::Unsupported);
    assert(call("WideCharToMultiByte", {99999, 0, 0x00682000, 1, 0, 0, 0, 0}) == StartupServiceResult::Unsupported);
    const uint32_t tid = 8, section = 0x00685000;
    cpu.write(wx86_cur_tib()+0x24, &tid, 4);
    assert(call("EnterCriticalSection", {section}) == StartupServiceResult::ContractFailure);
    assert(call("InitializeCriticalSectionAndSpinCount", {section, 4000}) == StartupServiceResult::Serviced);
    uint32_t cs[6] = {}; cpu.read(section, cs, sizeof(cs));
    assert(cs[1] == 0xFFFFFFFFu && cs[2] == 0 && cs[3] == 0 && cs[5] == 4000);
    assert(call("EnterCriticalSection", {section}) == StartupServiceResult::Serviced);
    assert(call("TryEnterCriticalSection", {section}) == StartupServiceResult::Serviced);
    assert(cpu.reg(d2rt::R_EAX) == 1);
    cpu.read(section, cs, sizeof(cs)); assert(cs[1] == 1 && cs[2] == 2 && cs[3] == tid);
    assert(call("DeleteCriticalSection", {section}) == StartupServiceResult::ContractFailure);
    call("LeaveCriticalSection", {section});
    cpu.read(section, cs, sizeof(cs)); assert(cs[2] == 1 && cs[3] == tid);
    call("LeaveCriticalSection", {section});
    cpu.read(section, cs, sizeof(cs)); assert(cs[1] == 0xFFFFFFFFu && cs[2] == 0 && cs[3] == 0);
    assert(call("LeaveCriticalSection", {section}) == StartupServiceResult::ContractFailure);
    cs[1] = 0; cs[2] = 1; cs[3] = 16; cpu.write(section, cs, sizeof(cs));
    assert(call("EnterCriticalSection", {section}) == StartupServiceResult::Unsupported);
    assert(call("TryEnterCriticalSection", {section}) == StartupServiceResult::Serviced && cpu.reg(d2rt::R_EAX) == 0);
    uint32_t untouched[6] = {}; cpu.read(section, untouched, sizeof(untouched));
    assert(std::memcmp(untouched, cs, sizeof(cs)) == 0);
    assert(call("LeaveCriticalSection", {section}) == StartupServiceResult::ContractFailure);
    cs[3] = tid; cpu.write(section, cs, sizeof(cs));
    call("LeaveCriticalSection", {section});
    assert(call("DeleteCriticalSection", {section}) == StartupServiceResult::Serviced);
    assert(call("EnterCriticalSection", {section}) == StartupServiceResult::ContractFailure);
    assert(call("InitializeCriticalSection", {section}) == StartupServiceResult::Serviced);
    assert(call("InitializeCriticalSection", {section}) == StartupServiceResult::ContractFailure);
    cpu.read(section, cs, sizeof(cs)); cs[1] = 1; cpu.write(section, cs, sizeof(cs));
    assert(call("EnterCriticalSection", {section}) == StartupServiceResult::ContractFailure);
    fclose(log);
}
