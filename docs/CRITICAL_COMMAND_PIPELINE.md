# Critical Command Pipeline

Phase 02 non-idempotent gameplay work uses one bounded command contract. Each command
has a cryptographically random 128-bit operation ID, a schema and payload version, a
categorical source site and deadline, sorted affected entity keys, optional expected
revisions, and owned payload bytes. It contains no live game pointers, SQL, Redis keys,
paths, account names, or character names.

This first implementation is the coordinator foundation and deliberately has no
production destination adapter or gameplay producers. The transactional database
inbox/outbox introduced by the next session will initialize it. Until then its health
line reports `state=stopped` and lifecycle calls are safe no-ops.

## Acceptance and execution

The coordinator validates and normalizes an envelope before acceptance. Entity keys
are sorted and duplicates are rejected. It reserves bounded memory, appends and
`fsync`s an independent checksummed journal record, then publishes the command to the
worker queue. A successful append is the acceptance boundary; records are never
coalesced or replaced by a newer command.

Conflicting commands are admitted in acceptance order for every affected key. A
command may execute only when it is first for all its keys, which avoids deadlock while
letting unrelated keys run on separate workers. The fence exists from acceptance until
an exact terminal completion. Retryable and ambiguous results retain the same ID,
journal record, and fence. A completion with the wrong operation ID or attempt is stale
and cannot release anything.

An identical duplicate submission attaches to the active operation or the bounded
recent-completion cache. Reusing an ID with different bytes fails closed. Accepted
commands cannot be cancelled. A terminal destination failure is checkpointed and
reported; exhausted retryable work stays blocked and fenced for operator recovery.

## Journal and recovery

The journal directory must be owned by the server user and mode `0700`; its regular
file is mode `0600` and opened without following symlinks. Records have magic, version,
length, operation ID, canonical command bytes, and CRC32. Appends are synchronized and
durable before returning. Exact checkpoint rewrites a temporary file, syncs it, renames
it, and syncs the directory.

Startup validates the complete journal before replay. Truncation, bad framing,
unsupported versions, checksum mismatch, unsafe ownership or permissions, I/O failure,
or quota exhaustion fails closed. Identical repeated frames replay once; conflicting
bytes for one operation ID are corruption. Replay retains the original operation ID.

Default bounds are 1,024 active operations, 64 MiB of command memory, 2,048 pending
completion records, 4,096 journal records, a 256 MiB journal, eight retries, and a
256-operation/8 MiB recent-completion cache.

## Lifecycle and diagnostics

Copyover and ordinary shutdown quiesce admission and require a three-second drain
before later persistence gates. Any failed transition resumes admission and leaves the
live server running. The game loop drains typed completions every two pulses; the pulse
path performs no database, Redis, or filesystem work.

`world persistence` exposes one metadata-only `critical_commands` line: state, queue,
in-flight and blocked counts, retained bytes, fences, recent completions, high-water
marks, accepts, attachments, outcomes, retries, ambiguous results, stale completions,
overloads, oldest age, and journal counts/bytes/status. It never prints command payloads
or entity identities.

Treat `blocked>0`, growing oldest age, `journal=corrupt`, `journal=io_failure`, or
`journal_quota=1` as a stop condition for copyover/shutdown and affected gameplay.
Restore the underlying storage or destination, preserve the journal, and investigate
before restarting. Never delete or edit the journal to clear a fence.

Focused validation is `python3 tests/async/test_critical_command_coordinator.py`.
