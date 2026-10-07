#pragma once
#include <cstdio>
#include <cstdint>
namespace d2rt { class Cpu; class PeImage; }
bool run_worker_probe(d2rt::Cpu&, const d2rt::PeImage&, uint32_t, FILE*);
