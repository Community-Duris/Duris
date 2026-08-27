# Code Review

## Findings Repaired

1. **High - blocked multi-key work did not reserve acceptance order.** A later command
   could acquire one of its keys while the earlier command waited on another key. Worker
   eligibility now requires the operation to be first in every per-key fence queue.
2. **High - completion output capacity could lose terminal publication.** The pulse path
   formerly consumed all raw results even after filling the caller buffer. Terminal
   results now remain queued until output capacity is available; a focused two-result,
   one-slot regression proves both are delivered.
3. **High - journal record count was checked only during replay.** An active process
   could append beyond the replay bound and create a journal that refused its next
   startup. Admission now enforces both byte and record limits.
4. **Medium - journal corruption and I/O state disappeared from stopped health.** The
   last typed result and cumulative corruption/I/O counters now survive failed init and
   render as redacted operator metadata.
5. **Medium - replay and codec allocation failures could terminate the process.**
   Journal scans, replay admission, worker copies, and destination exceptions now map
   to bounded fail-closed states.
6. **Medium - codec reserved padding was not canonical on decode.** Non-zero reserved
   bytes are now rejected, and regular journal files must also have one hard link.
7. **Low - pwipe skipped critical worker teardown.** Critical coordinator shutdown now
   occurs independently of the legacy persistence-worker pwipe exception.

No unresolved blocking findings remain.
