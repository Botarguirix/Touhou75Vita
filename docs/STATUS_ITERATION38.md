# Iteration 38: four checks per installed VPK

Hardware iteration 37 passed main-context restoration and thread-ID output,
returned the observed CreateThread and reached SetThreadPriority at return
0x00423A6C. The worker is saved at its nonsignaled event wait with 16 ms
timeout. The run counted 114 services in 24,625,577 us; the watchdog disarmed.
The submitted screenshot is from iteration 36; the logs identify iteration 37.

At the user's request, iteration 38 runs four independent synthetic service
checks after the original startup diagnostic. A failure does not skip subsequent
cases; each reports batch_test_NN_name/result/error and tags its calls with
batch_test_begin and batch_api. Final summary: batch_total, batch_passed,
batch_failed, batch_result. The screen reports BATCH: N/4 PASSED.

1. Auto-reset event: initially nonsignaled poll times out; SetEvent allows one
   successful wait, and a second poll times out because the signal was consumed.
2. Manual-reset event: initially signaled permits two waits; ResetEvent causes
   a zero-time poll to time out. Both event checks close their objects.
3. Heap: allocate 17 bytes with a recognizable pattern; grow to 65 bytes by
   moving with HEAP_ZERO_MEMORY; verify preserved bytes and a zero-filled tail;
   verify HeapSize, shrink in place to 9 bytes and verify size, then free.
4. Clocks: nonzero UTC FILETIME; QPF=1,000,000; two nondecreasing QPC reads;
   LastError preserved across those successful calls.

These checks call the same StartupServices implementation via synthetic guest
stdcall frames. They do not execute original guest instructions for those calls.
They use separate scratch storage, save/restore the CPU context and frame bytes,
and restore the main thread's LastError. Temporary event/heap resources are
closed/freed. The bounded heap does not recycle freed address ranges, so this
small batch advances its allocator cursor. Startup counters are printed before
the batch; subsequent service records belong to batch_scope. They must not be
interpreted as additional calls made by the original executable.

The original 30-second watchdog ends before the batch; the batch has a fixed
number of direct service calls and only zero-time event polls. Overall success
requires both the startup checkpoint and all four checks. Startup failures
still allow the batch to report its available checks; unavailable setup yields
failures rather than invented successes. The main startup path continues to
stop at SetThreadPriority. Priorities, timed wakeups and continuous cooperative
scheduling remain required. This batch improves coverage per physical install;
it is not four completed game-port milestones or visual boot.

Install 01.42 and confirm ITERATION 38 / BATCH CHECKS,
build_id=iteration38-batch-checks-r1. Logs: iteration38.log,
iteration38-runtime.log, iteration38-watchdog.log. VitaSDK compilation is
performed; the four new checks run on the user's Vita when launched. Their
hardware outcome is pending, not assumed passed from compilation.
