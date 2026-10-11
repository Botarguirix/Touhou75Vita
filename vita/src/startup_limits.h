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
// Physical86 used 5811 draws for 272 frames. Leave enough draw capacity for
// all 360 frames even at the observed earlier 28 draws/frame. Other caps stay.
constexpr unsigned draw_calls=12288, draw_pixels=768u*1024u*1024u;
constexpr unsigned audio_trap_dispatches=1024;
}
