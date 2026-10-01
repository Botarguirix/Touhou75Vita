#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>
namespace d2rt { class Cpu; class PeImage; }
// Runs original EXE instructions through two known API contracts to HeapCreate.
bool run_startup_probe(d2rt::Cpu& cpu, const d2rt::PeImage& image,
                       const std::vector<uint8_t>& exe, FILE* log);
