#pragma once
#include <cstdio>
#include <cstdint>
#include "runtime/guest_thread.h"
namespace d2rt { class Cpu; class PeImage; }
class StartupServices;
struct StartupWorker {
    d2rt::X86Context context{};
    uint32_t handle = 0x00AB4000, id = 12, event = 0, timeout_ms = 0;
    uint64_t wait_started_us = 0;
    bool blocked = false;
};
bool run_worker_probe(d2rt::Cpu&, const d2rt::PeImage&, uint32_t, StartupServices&, StartupWorker&, FILE*);
