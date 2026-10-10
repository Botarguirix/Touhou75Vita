#pragma once
#include <cstdint>

// One bounded EXE run; these are real frames/waits, not synthetic test cases.
namespace startup_limits {
constexpr unsigned present_frames=240, frame_wait_resumes=240;
constexpr uint64_t time_cap_us=75000000, watchdog_timeout_us=90000000;
constexpr unsigned draw_calls=512, draw_pixels=128u*1024u*1024u;
}
