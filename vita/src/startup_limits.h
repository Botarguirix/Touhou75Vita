#pragma once
#include <cstdint>

// One bounded EXE run; these are real frames/waits, not synthetic test cases.
namespace startup_limits {
constexpr unsigned present_frames=16, frame_wait_resumes=16;
constexpr uint64_t time_cap_us=45000000, watchdog_timeout_us=60000000;
constexpr unsigned draw_calls=64, draw_pixels=16u*1024u*1024u;
}
