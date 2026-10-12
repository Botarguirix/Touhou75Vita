# Iteration 31: tracked heap blocks and resizing

Hardware iteration 30 R2 installed successfully, registered the filter and
reached HeapReAlloc at return VA 0x006468E4. Before that, the old HeapSize stub
returned zero and HeapFree merely reported success. The log recorded 72
counted calls, elapsed 15,815,890 us, and a disarmed watchdog without a limit hit.
Those old stub returns did not establish correct allocator behavior.

Iteration 31 tracks each HeapAlloc pointer, requested size and reserved aligned
capacity. Zero-size requests reserve a nonzero 16-byte block. HeapSize returns
the recorded requested size, and HeapFree invalidates that exact pointer.
Foreign, interior and double-freed pointers cause an explicit contract stop.
Environment-copy ownership remains separate from caller HeapAlloc blocks.

HeapReAlloc takes four arguments (20-byte stdcall frame). Shrinking or growth
within reserved capacity retains the address. Larger requests allocate a new
block and preserve min(old size, new size) bytes, checked by readback, before
invalidating the old pointer. HEAP_ZERO_MEMORY clears only the newly exposed
region and verifies it; preserved bytes are not overwritten. In-place-only
growth fails without changing the original block when capacity is insufficient.
Out-of-space failure similarly leaves the original valid. HeapReAlloc does
not change LastError on allocation failure. Unsupported flags, including
exception-generating allocation, remain diagnostic boundaries.

The heap is still an 8 MiB, single-guest-thread bounded allocator. Freed and
moved blocks are invalidated but their address ranges are not recycled yet;
long-running gameplay needs free-space reuse/coalescing. This implementation
targets correct startup data/size handling, not a finished gameplay allocator.
Counters now include HeapFree/HeapSize/HeapReAlloc under startup_heap_other_calls;
previous iteration totals excluded those calls, so totals are not directly
comparable. Log addresses can change because HeapSize no longer returns zero.

Install 01.35 and confirm ITERATION 31 / iteration31-heap-resize-startup-r1.
Logs: iteration31.log, iteration31-runtime.log, iteration31-watchdog.log.
Look for recorded block sizes and preserved_readback=passed on resize; fixing
HeapSize may change the path so the previous ReAlloc boundary need not recur.
Full game boot remains unverified. VitaSDK compilation is performed; no
automated tests were added or run.

References:
- https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heaprealloc
- https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapsize
- https://learn.microsoft.com/en-us/windows/win32/api/heapapi/nf-heapapi-heapfree
