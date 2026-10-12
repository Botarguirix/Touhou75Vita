#pragma once
#include <cstdint>

// One bounded EXE run; these are real frames/waits, not synthetic test cases.
namespace startup_limits {
// Physical92 reached the 360-frame test limit without a scene handoff in its
// supplied log. Allow selection/scene loading in a longer interactive run.
// The first wait has no Present, so retain one extra genuine event resume.
constexpr unsigned present_frames=1800, frame_wait_resumes=present_frames+1;
constexpr uint64_t time_cap_us=300000000, watchdog_timeout_us=330000000;
// Capacity scales with the longer run; allocation and state/geometry guards
// still stop immediately. Pixel accounting remains bounded in uint32_t.
constexpr unsigned draw_calls=65536, draw_pixels=768u*1024u*1024u;
constexpr unsigned audio_trap_dispatches=8192;
}
