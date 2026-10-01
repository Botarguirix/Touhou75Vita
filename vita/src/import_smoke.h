#pragma once
#include <cstdio>
#include <vector>
#include <cstdint>
namespace d2rt { class Cpu; }
bool run_import_smoke(d2rt::Cpu& cpu, const std::vector<uint8_t>& exe, FILE* log);
