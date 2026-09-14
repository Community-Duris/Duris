# Critical completion capacity (#337)

The pre-fix coordinator removes a ninth-attempt retryable/ambiguous result even
when the caller's completion array is full. The operation remains fenced, but
the gameplay consumer never receives its final failure notification.

The coordinator now decides whether it can retry before removing the result.
Only a retry with attempts remaining may proceed without an output slot. Every
final outcome waits in the result queue until the caller can receive it. Existing
blocked/fenced, retry limits, completed-cache and journal semantics are preserved.

## Recent-change check

Checked open PRs and `origin/master` at `1db71f7211f12bfc79669d87809ef0bffb93f248`.
No open PR addressed this path. The most recent coordinator change was #242;
the exhausted-result loss remained present. The new regression fails on that
master at the assertion requiring the final result to remain queued.

## Local validation

- MariaDB and flat-file development servers build with the maintained warning
  profile (`make -C src -j4`, plus `PERSISTENCE_BACKEND=flatfile` and a separate
  `DMS_BINARY`).
- `python3 tests/async/test_critical_completion_capacity.py` runs the actual
  coordinator, worker threads and journal under ASan/UBSan. Controlled repository
  callbacks force retryable-failure and ambiguous-commit exhaustion at capacities
  0, 1 and 64. Mixed success/terminal results fill the buffer before exhaustion.
  Assertions cover zero-capacity retry progress, repeated zero-capacity pulses,
  exact identities/outcomes/revisions/error codes/attempts, exactly-once delivery,
  health counters, retained fences and duplicate submission attachment.
- Existing coordinator identity/order/replay/bounds, journal fault and uncertain
  admission tests pass. The repository formatting check and `git diff --check`
  pass.
- The existing real flat-file combat journey exercises synthetic account and
  character creation, NPC combat, equipment and currency pickup, player death,
  corpse recovery, disputed custody, save, reconnect and restart. It is run with
  normal coins, reset-created coins and boons enabled.

The capacity regression uses a controlled repository adapter to reproduce worker
outcomes deterministically. The Telnet journeys exercise normal production
gameplay and persistence; they do not inject retry exhaustion through the network.
No production state is used. Hosted CI and the full local `make test-all` gate are
not acceptance requirements for this focused correction.
