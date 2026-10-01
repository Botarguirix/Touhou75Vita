#pragma once
#include <cstdint>
#include <cstdio>
#include <vector>
namespace d2rt { class Cpu; class PeImage; }
// Runs pristine EXE instructions, stopping BEFORE servicing the first import.
bool run_startup_probe(d2rt::Cpu& cpu, const d2rt::PeImage& image,
                       const std::vector<uint8_t>& exe, FILE* log);
