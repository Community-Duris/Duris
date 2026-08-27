# Code Review

## Findings Repaired

1. **High - process-local and Redis counters could collide after restart or across
   processes.** Boot now atomically reserves a database range above every retained UID,
   and Redis cannot move the allocator.
2. **High - a declared item list could omit descendants or encode a disconnected cycle.**
   The repository locks the whole indexed root range and compares its exact ordered set;
   the codec independently proves every parent chain reaches the declared root.
3. **High - creation initially checked only the root identity.** Every declared UID is
   now checked and gap-locked in item order, returning stable `EEXIST` without mutation.
4. **High - arbitrary system/destruction transitions could resurrect terminal items.**
   System is source-only for creation, destruction is destination-only and terminal,
   and every ordinary transfer requires active source state.
5. **Medium - owner insertion initially preceded canonical ordering.** Both owner row
   creation and row locking now follow the same typed identity order; player keys also
   share the canonical PID fence used by other critical domains.
6. **Medium - legacy import could silently ignore conflict with an existing authority.**
   Existing-owner disagreement, duplicate UIDs, missing/broken parents, tainted
   descendants, and opaque auction blobs are all quarantined before baseline insertion.
7. **Medium - overflow and optimistic-update failures needed explicit outcomes.** Item
   and owner maximum revisions return stable `ERANGE`; zero-error affected-row failures
   are surfaced as repository I/O failures and roll back.
8. **Low - the migration runner's displayed total lagged the two new steps.** The total
   is synchronized and covered by the existing invocation regression.

No unresolved blocking findings remain. The retained 13 local duplicate-UID rows are
intentional quarantine evidence for operator repair, not an implicitly selected owner.
