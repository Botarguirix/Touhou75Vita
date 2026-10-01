#pragma once
#include <cstdint>

// Validate a bounded x86 SEH registration chain; this does not dispatch exceptions.
template<class Read>
bool validate_seh_chain(uint32_t head, uint32_t stack_begin, uint32_t stack_end,
                        uint32_t code_begin, uint32_t code_end, Read read,
                        unsigned& depth) {
    depth = 0;
    uint32_t visited[32] = {};
    while (head != 0xFFFFFFFFu) {
        if (depth == 32 || (head & 3u) || head < stack_begin ||
            uint64_t(head) + 8 > stack_end) return false;
        for (unsigned i = 0; i < depth; ++i)
            if (visited[i] == head) return false;
        visited[depth++] = head;
        uint32_t previous = 0, handler = 0;
        if (!read(head, previous) || !read(head + 4, handler) ||
            handler < code_begin || handler >= code_end) return false;
        head = previous;
    }
    return depth != 0;
}
