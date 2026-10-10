#pragma once
#include <cstdint>

// One bounded EXE run; these are real frames/waits, not synthetic test cases.
namespace startup_limits {
constexpr unsigned present_frames=240, frame_wait_resumes=240;
// Physical83 met the next draw at 99.672 s. Leave a bounded 30-second margin
// to exercise that geometry through the same 240-frame scope.
constexpr uint64_t time_cap_us=130000000, watchdog_timeout_us=150000000;
// Opening uses 28 draws per frame in physical82. Permit that measured
// workload through the existing 240-frame/time cap; memory stays separate.
constexpr unsigned draw_calls=2048, draw_pixels=384u*1024u*1024u;
constexpr unsigned audio_trap_dispatches=1024;
}
