#pragma once
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cstdint>

namespace th075 {
inline bool compact_diagnostics=false;
inline uint64_t suppressed_diagnostics=0;
// Only repeated successful-call traces are omitted. Errors, boundaries,
// timing, resource allocation, PCM/input and final summaries remain visible.
inline bool repetitive_trace(const char* format) {
    static const char* const prefixes[]={
        "startup_d3d8_method=", "startup_d3d8_return=", "startup_d3d8_arg",
        "startup_d3d8_ref=", "startup_d3d8_vertex=", "startup_d3d8_vertex_bits=",
        "startup_d3d8_draw_vertices=", "startup_d3d8_draw_request=",
        "startup_d3d8_draw_geometry=", "startup_d3d8_draw_filter=",
        "startup_d3d8_draw_blend=", "startup_d3d8_draw_hash=",
        "startup_d3d8_draw_probe=", "startup_d3d8_linear_work=",
        "startup_d3d8_stage_set=", "startup_d3d8_stage_get=",
        "startup_d3d8_render_state=", "startup_d3d8_texture_binding=",
        "startup_d3d8_texture_get=", "startup_d3d8_present_request=",
        "startup_d3d8_present_rect=", "startup_import_call=", "startup_return_va=",
        "startup_serviced_import=", "startup_service_stack_bytes=",
        "startup_service_abi=passed", "startup_dynamic_export_called=yes"
    };
    for(const char* prefix:prefixes)
        if(!std::strncmp(format,prefix,std::strlen(prefix)))return true;
    return false;
}
#if defined(__GNUC__)
__attribute__((format(printf,2,3)))
#endif
inline int diagnostic_log(FILE* file,const char* format,...) {
    if(compact_diagnostics && repetitive_trace(format)){++suppressed_diagnostics;return 0;}
    va_list args;va_start(args,format);const int result=std::vfprintf(file,format,args);
    va_end(args);return result;
}
}
