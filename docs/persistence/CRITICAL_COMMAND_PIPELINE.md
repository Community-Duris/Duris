# Critical Command Pipeline

Phase 02 non-idempotent gameplay work uses one bounded command contract. Each command
has a cryptographically random 128-bit operation ID, a schema and payload version, a
categorical source site and deadline, sorted affected entity keys, optional expected
revisions, and owned payload bytes. It contains no live game pointers, SQL, Redis keys,
paths, account names, or character names.

The generic destination stores command identity and result in an InnoDB inbox, applies
a typed test-domain mutation, and creates its notification in the same transaction.
Production gameplay producers remain disabled until their individual Phase 02 domain
sessions. Outside mini mode, startup requires `CRITICAL_COMMAND_JOURNAL_DIR` and the
verified critical-command schema; failure leaves critical gameplay stopped.

## Acceptance and execution

The coordinator validates and normalizes an envelope before admission. Entity keys
are sorted and duplicates are rejected. It reserves bounded memory and queues the
encoded command on a serialized admission lane. `submit()` returns
`awaiting_durability` while that lane owns the independent checksummed journal append
and `fsync`; a RAM enqueue is not durable evidence and never publishes the command to
the execution queue. Only the worker's successful append acknowledgement crosses the
durability boundary. Records are never coalesced or replaced by a newer command.

The admission lane is bounded by the same 1,024-operation/64 MiB coordinator limits
as execution, and its queue plus one in-flight append are exposed as byte-counted
health fields. The coordinator mutex is not held while the worker waits on journal
I/O. A definitive append failure retains a terminal failure notification without
executing the command; an uncertain append retains the operation and fence until
replay/sync reconciliation either proves the record durable or produces a terminal
failure. The operation ID, sorted-key fences, exact acknowledgement, and original
command bytes are retained throughout.

The relevant states are explicit:

| State | Owner and next transition |
| --- | --- |
| Awaiting journal durability | Admission worker appends and syncs; then `Executing`, `Uncertain`, or `Admission failed` |
| Durable admission | Execution worker applies the command; then `Final notification retained` or retry/blocked |
| Uncertain admission | Admission worker replays/syncs and retries the original ID; then durable or terminal failure |
| Admission failed | Coordinator retains a terminal failure until the simulation thread delivers it |
| Final notification retained | Simulation-thread pulse delivers the exact completion, then releases the fence |

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
256-operation/8 MiB recent-completion cache. The admission queue counts against the
active-operation and command-memory bounds; accepted work is never dropped merely
because the worker is behind.

## Lifecycle and diagnostics

Copyover and ordinary shutdown quiesce admission and require a three-second drain
before later persistence gates. The drain covers admission, execution, retry, and
retained terminal notifications. Any failed transition resumes admission and leaves
the live server running. The game loop drains typed completions every two pulses.
Normal submission, pulse, and uncertain-recovery signaling perform no journal file
I/O; journal append, `fsync`, replay, and reconciliation are owned by the admission
worker. Shutdown joins that worker after the admission lane has drained, so no
detached append can outlive the coordinator or its journal lock.

`world persistence` exposes one metadata-only `critical_commands` line: state, queue,
in-flight and blocked counts, retained bytes, fences, recent completions, high-water
marks, accepts, attachments, outcomes, retries, ambiguous results, stale completions,
overloads, oldest age, and journal counts/bytes/status. It never prints command payloads
or entity identities.

The database inbox stores the canonical command/key hashes and authoritative result.
An identical duplicate returns that result; different bytes under the same operation ID
fail closed. Connection loss after `COMMIT` is reconciled by rereading the inbox before
the original immutable command can retry. Test-state rows are locked in normalized key
order. Deadlocks and lock waits are retryable with the same operation ID.

Each transaction also writes a bounded typed outbox record. The dispatcher reads at
most 64 records/4 MiB at a time, passes the stable outbox ID to a typed consumer, and
records consumer dedupe plus delivered state together. Retryable delivery uses bounded
backoff; the eighth failure or a terminal result retains a dead-letter row. Copyover and
shutdown drain commands first and outbox records second.

`world persistence` adds cached `critical_outbox` counts for pending age, dead letters,
incomplete inbox rows, committed operations missing outbox rows, delivery/retry/error
totals, and high-water records/bytes. `critical_outbox_reconcile()` is the typed
read-only discrepancy interface. `critical_outbox_retry_dead_letter(id)` is the sole
repair action: it can only reset one numeric dead-letter ID and never accepts SQL.

Treat `blocked>0`, growing oldest age, `journal=corrupt`, `journal=io_failure`, or
`journal_quota=1` as a stop condition for copyover/shutdown and affected gameplay.
Restore the underlying storage or destination, preserve the journal, and investigate
before restarting. Never delete or edit the journal to clear a fence.

Focused validation is `python3 tests/async/test_critical_command_admission.py`,
`python3 tests/async/test_critical_command_coordinator.py`,
`python3 tests/async/test_critical_command_journal_uncertain.py`,
`python3 tests/async/test_critical_completion_capacity.py`,
`python3 tests/async/test_critical_transaction_contract.py`, and, on an explicitly
guarded local development database, `tests/async/run_critical_command_schema_mysql.sh`.

## Epic balance destination

Epic awards and spends use command type `epic` with one player key, a signed delta,
typed reason, optional reason ID, and a funds-required flag. The repository creates a
baseline lazily when needed, locks `player_data`, validates the revision and funds,
updates balance/revision, inserts one immutable ledger row, stores the exact result,
and emits its outbox row in the same transaction. Duplicate and ambiguous replay return
the stored balance/revision without another delta.

The game thread owns a bounded operation-keyed continuation table. It publishes the
exact committed balance and revision before invoking a typed staged effect. Offline
completions remain retained until the player enters or reconnects. `world persistence`
reports aggregate `epic_transactions` pending, retained, outcome, submission-failure,
and malformed-completion counters without operation or player identity.

Player checkpoints, legacy flat-file replay, and ordinary status updates do not write
the epic balance. New-character initialization and authoritative SQL hydration are the
only non-transactional in-memory assignments. Focused validation is
`python3 tests/async/test_epic_transaction_contract.py` and, on a guarded development
database, `tests/async/run_epic_transaction_schema_mysql.sh`.

## Currency receipt and live-publication boundary

The currency adapter gives every in-process continuation an explicit publication
state: awaiting coordinator completion, ready, waiting for its player, retrying a
bounded coin callback, or blocked on an unresolved receipt. It retains the
original operation ID and continuation when a receipt is ambiguous,
retry-exhausted, or acknowledges a commit whose result or live balances cannot
be validated. These states are **not** terminal rejection; they must not trigger
a failure/refund callback. A blocked entry is not scanned again on every pulse.
It emits one operation-ID-bearing diagnostic and sleeps until the coordinator
delivers another exact receipt. See [issue #380](https://github.com/Community-Duris/Duris/issues/380).

Coordinator completion and live publication have different lifetimes. A
non-rebasable debit must respect the domain's player/account busy state even
after the coordinator releases its execution fence. Rebasable rewards may queue
behind ordinary in-flight work because they do not read the live balance, but
stop for an affected player/account once publication is blocked. This prevents
a single unresolved receipt from filling the global `CURRENCY_PENDING_MAX`
table. Unrelated accounts retain their existing admission behavior.
Successful publication, or a known terminal rejection, removes the completed
pending entry before invoking its continuation. An extracted node owns callback
context across re-entrant submissions; no pending-map iterator survives that
callback.

A corrected exact receipt can finish a retained operation once without issuing a
new debit/credit. This is not automatic reconciliation tooling: an unresolved
receipt can continue to fence dependent gameplay until the original result is
recovered or the underlying fault is repaired. The fence deliberately includes
every online character for the same account and racewar: those characters share
one bank row, so a timeout or per-character bypass could spend an unpublished
balance. Do not clear the pending operation, add a timeout, or create a replacement
operation ID to conceal the fault. This in-process retention does not claim that
callback context becomes durable across restart; durable continuation ownership
belongs to the larger persistence refactor.

`world persistence` reports `currency_transactions` pending, retained-offline,
blocked-publication, callback-retry, outcome, malformed, submission-failure, and
abandoned-publication counts. Blocked, malformed, failed-submission, or abandoned
states make that line degraded; it exposes no account, player, or operation ID.

`python3 tests/async/test_currency_completion_retention.py` links the actual
adapter and codecs with controlled coordinator/live endpoints under ASan/UBSan
in both build modes. It covers malformed/ambiguous receipts, range validation,
offline re-entry, corrected/duplicate delivery, payload-free known rejection,
account/racewar guards, re-entrant callback chaining/rehashing, normal rebasable
admission, and blocked-publication admission.
`test_currency_input_queue.py` additionally covers real command-selection and coin
publication adapters. These tests do not by themselves prove SQL/flatfile storage
or complete player-journey parity.

## Physical coin custody

`coin_transfer_command` and the currency coordinator commit wallet and physical
pile changes together on both SQL and flat-file authority. Payload amounts,
UID/custody, owner revisions, conservation, overflow, and operation-ID replay
are checked before publication. The SQL parent receipt identifies both child
operation IDs in the same transaction. Saved item `coin_payload` preserves the
pile denominations for reload; ordinary snapshots do not create custody.

An untracked NPC-wallet or reset-created pile first passes the existing absent-item
admission path. Admission grants no money: wallet credit follows the separate
atomic pickup commit. Its continuation rechecks the original container UID,
location/accessibility, and custody, so moving the source during admission cannot
publish a stale pickup. Existing active/retired durable UID conflicts fail closed.

Flat-file coin publication updates the affected room projection in the same
authority transaction, including partial piles and container weights. Otherwise
a successful coin pickup could advance custody while leaving the next ordinary
item pickup unable to materialize the room revision.

Coin publication callbacks have at most eight attempts. On permanent publication
failure, `EOWNERDEAD` cleanup clears retained command context and retires pending
work without refunding an already committed debit or reporting it as rejected.
Durable custody and command evidence remain the recovery source. The focused
`test_coin_custody_lifecycle.py` and `test_currency_input_queue.py` harnesses and
`run_currency_transaction_schema_mysql.sh` cover this boundary.
