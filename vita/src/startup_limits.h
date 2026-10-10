#pragma once
#include <cstdint>

// One bounded EXE run; these are real frames/waits, not synthetic test cases.
namespace startup_limits {
// Physical84 exhausted 240 waits after 239 Present. The initial wait does
// not present a frame, so leave one extra resume without inventing a signal.
constexpr unsigned present_frames=360, frame_wait_resumes=present_frames+1;
// Physical84 took 104.133 s. Extend the bounded original opening run;
// unexpected APIs/geometry and memory limits still stop immediately.
constexpr uint64_t time_cap_us=180000000, watchdog_timeout_us=210000000;
// Opening has used 28 draws/frame. Allow that workload after frame 239;
// total covered pixels and the independent 64 MiB graphics budget stay bounded.
constexpr unsigned draw_calls=6144, draw_pixels=768u*1024u*1024u;
constexpr unsigned audio_trap_dispatches=1024;
}
