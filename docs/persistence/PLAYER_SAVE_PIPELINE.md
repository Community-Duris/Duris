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

## Persistence reporting severity

`persistence_report(severity, level, domain, owner, item_uid, event_id, action, format, ...)`
separates event severity from the immortal audience selected by `level`:

| Severity | Structured outcome | Routing |
| --- | --- | --- |
| `persistence_severity::ok` | `outcome=ok` | `LOG_FILE` and `LOG_WIZ` file records |
| `persistence_severity::info` | `outcome=info` | `LOG_FILE` and `LOG_WIZ` file records |
| `persistence_severity::alert` | `outcome=alert` | Both file records and the existing red immortal broadcast |

`persistence_alert(...)` remains an alert-only compatibility entry point. Unknown
severity values also alert. Both entry points use the same category sanitization
and numeric-only detail filtering; owner, item UID and event ID arguments are
omitted from the output at every severity.

Use `ok` after a successful durable operation and `info` for expected progress.
Death recovery/disposition completion and durable disposition recording are `ok`.
Ordinary custody waits, undisputed in-flight transfers and successfully submitted
corpse-item restarts are `info`. Automatic raising skipped while corpse ownership
is pending is also informational. Failed restart submissions, disputes, missing
corpses, abandoned recovery, event scheduling failures and failed saves remain
alerts. A custody wait still escalates after 30 seconds and once per subsequent
30-second window; normal polls now use `outcome=info` in the file records.

Successful deferred-save flushes, flat fallback writes and complete legacy replays
also use `ok`; failed flushes and partial replays retain alerts. Retired raw-worker
and raw-replay status reports use `info`. Failed I/O, rejected mutations, dropped
or undrained work, unavailable workers and automatic restarts after worker failure
continue to alert even when a recovery path is available.

File delivery runs on a dedicated worker started during boot. Admission uses a fixed
128-record queue and a try-lock: the game loop never opens, writes, closes, or waits
for a reporting file. Each record is bounded to 4095 bytes; the reporter bounds
numeric details to 1023 bytes and the formatted event to 2047 bytes. Formatting may
truncate long numeric details. The worker receives only copied text and enqueue
time; it never accesses characters, descriptors, or the legacy `logit` formatter.

A full or contended queue rejects the new file record and increments `rejected`;
there is no synchronous fallback. Alert broadcasts still run immediately even if
file admission fails. `world persistence` exposes cumulative accepted/completed,
rejected, and independent file/wiz failure counters. The game pulse broadcasts a
reporting-delivery alert when failures increase or pending delivery makes no progress
for 30 seconds, at most once per 30 seconds. These notices do not re-enter the queue.
Counters remain inspectable when no immortal was online for the notice.

The worker creates missing parent directories, opens each sink with append and
close-on-exec, and accepts only regular files. Open, short-write, write, and close
failures are counted per sink; the other sink is still attempted. No ambiguous write
is replayed, so a partial write may leave a truncated record. Rename/create rotation
is supported: a record goes to the file opened for that append and subsequent opens
follow the new path. Use rename/create rotation; concurrent copytruncate cannot
promise lossless records. The two sinks are independent, not an atomic transaction.

Shutdown and copyover wait up to three seconds for queued and in-flight attempts.
A timeout reports possible diagnostic loss to stderr and does not veto authoritative
save/recovery gates. A failed exec leaves the worker available. Worker storage lives
until process exit, avoiding an unbounded destructor join if filesystem I/O hangs.
These diagnostic records are not a durable gameplay journal: no fsync or crash replay
is promised. Failure counts are also printed on ordinary shutdown.

Validate bounded admission, blocked I/O, independent sink failures, rotation, and
drain behavior with `python3 tests/async/test_persistence_log.py`.

Validate routing and privacy with `python3 tests/async/test_persistence_severity.py`;
`test_death_recovery_alert_level.py` also verifies the timed stall escalation.

## Terminal Saves And Process Drain

Destructive player transitions mark and capture a fresh full revision with the
current terminal intent and room behind a fixed-capacity terminal fence. An ACKed
nonterminal retry or an older pending full snapshot cannot authorize a new camp.
A caller may extract the character only after the exact
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

## Deferred and manual saves

`src/cmd/actoth.c::persistence_pulse_character_saves()` services deferred capture
and manual acknowledgement checks from the game-loop persistence path, independent
of world-event debt. It attempts at most 32 due deferred saves per call with a
round-robin cursor and checks the bounded 512-slot manual-status table. Deferred
slots use monotonic due times and character runtime identities so storage reuse
cannot apply work to a different character. Manual completion still requires
acknowledgement within the existing 30-second deadline.

A failed camp retains the live character and permits automatic nonterminal retry;
a later camp must capture its own intent again. Flat-file terminal saves require
authority acknowledgement; SQL-backed callers may explicitly permit a synced
journal handoff. These guarantees do not prevent legitimate storage timeouts or
operating-system starvation. The controlled retry/crash modes are documented in
[Testing](../guides/TESTING.md#full-world-save-diagnostics).

## Disputed player deaths

A refused corpse handoff records a per-player runtime dispute. The death retry
captures the corpse, refused inventory, wallet-conversion evidence, and observed
custody in one bounded death snapshot. SQL applies `player_death_disposition`,
`player_death_custody`, quarantine of remaining player-owned custody, and the
cleared player snapshot in one transaction. Flat-file authority publishes the
same effects with `player-deaths/<pid>-<revision>.death` in its recoverable
authority transaction. Successfully transferred corpse-owned items stay active;
quarantine includes remaining durable-only descendants.

Release requires durability for the death revision. Capture/admission failure,
a missing corpse, or a failed durability fence retains live assets for retry.
If event admission fails, the live player's fallback due time is serviced by the
game pulse. A two-second terminal wait is one attempt's budget, not a bound on
total death recovery. Restart replay applies journaled death evidence idempotently;
the runtime dispute flag itself is not durable evidence before journal admission.

A death disposition preserves evidence, not automatic restitution. Later ordinary
saves cannot overwrite it. SQL and file death stores are protected recovery data
in the [lifecycle manifest](../../migrations/data_lifecycle_manifest.json); retention
continues until recovery is resolved and the controller approves a purge horizon.
Do not treat these files as rotating logs. See `test_death_item_custody_contract.py`,
`test_player_snapshot_capture.py`, `test_player_save_pipeline.py`, and the isolated
`run_player_death_disposition_mysql.sh` suite for the relevant boundaries.
