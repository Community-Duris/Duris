# Player Save Pipeline

Ordinary player checkpoints use one revisioned pipeline:

1. The game thread marks the affected component bits and seals a bounded immutable
   snapshot only when dirty work exists.
2. A bounded dispatcher appends and syncs the typed journal record.
3. The keyed worker applies the snapshot through the revision-guarded repository.
4. The game pulse consumes typed completions; the worker checkpoints the journal only
   after durable revision evidence.

The game-thread checkpoint and completion paths perform no MySQL, Redis, or filesystem
operation. Redis remains available for reconstructible caches but is not player-save
durability state. The old Redis dirty set and player-save fork are disabled.

## Configuration And Health

`PLAYER_SAVE_JOURNAL_DIR` is required and must be an absolute, server-user-owned path.
Startup fails closed when the journal or worker cannot start. `world persistence`
reports bounded coordinator depth/bytes, high-water marks, captures, coalescing,
unchanged checkpoints, append failures, overload, dispatch, completion, and replay
state. Output contains no player identity or snapshot value.

## Terminal Saves And Process Drain

Destructive player transitions promote the newest cumulative revision behind a
fixed-capacity terminal fence. A caller may extract the character only after the exact
revision receives a database acknowledgement or, where explicitly allowed, after its
journal record has been synced. Older completions cannot release a newer fence. A
deadline failure keeps the fence and dirty revision retryable; later mutations advance
that same fence instead of becoming untracked.

Copyover and ordinary shutdown quiesce new checkpoint admission and wait to a bounded
deadline until every accepted snapshot is journal-durable. The drain includes a record
currently owned by the journal dispatcher, not only records still visible in its queue.
If the deadline expires, the transition is cancelled and the live server resumes
checkpoint admission.

`world persistence` reports admission, append-in-flight, terminal outcome, timeout,
and drain-failure counters without player identity. New legacy player flat-fallback
writes are retired; existing files remain untouched for compatibility and operator
recovery. Locker fallback behavior remains a separate compatibility boundary.

## Compatibility Boundary

New characters without a durable PID, locker characters, and Phase 02 critical
transactions retain their explicit legacy compatibility route for now. Synchronous
transactional compatibility saves advance `save_revision` in the same transaction,
fencing every older immutable snapshot. They are not treated as an exactly-once
gameplay command; Phase 02 replaces them with operation-keyed domains.
