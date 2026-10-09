# Coordinator current retained storage - 2026-10-09

The coordinator now measures its actual owned command, identity-table, fence/pending/completion queue, worker reservation and cached completion storage while holding its own mutex. The completion queue owns its original deque storage and can report its allocated map and blocks, including empty retained capacity. Parked participant capacity reports were captured after complete preparation; subsequent worker paths only mutate scalar state and borrow the immutable command/vector/string storage. No worker-owned mutable object is dereferenced by this census.

The private outside-coordinator current-storage observer takes that same mutex and returns a strong passive snapshot. It must only be used outside the coordinator and journal locks. Its result is not an admission lease. The full submit path and once-only same-lock ROOT/journal handoff remain unfinished.

Independent complete RAW and final formatted source review passed. Tokens/logical preprocessing, exact original inverses, participant source pins and three protected unrelated files passed. The writer registry authenticates 400 source pins (the completion header is newly pinned), retains all 931 writer policies and reports zero new/unmapped sites. Evidence: tmp/coordinator-retention-integrated-20261009 and tmp/coordinator-retention-owner-20261009.

Native builds, gameplay, persistence and recovery checks remain deferred to the requested major-plan readiness batch. Accounting remains inactive/CLOSED, coverage incomplete and release BLOCKED. The primary goal remains ACTIVE.
