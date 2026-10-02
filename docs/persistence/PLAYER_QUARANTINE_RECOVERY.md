# Stopped player quarantine recovery

The #664 recovery transition is restricted to a coherent isolated restore with
the server and all other writers stopped. It does not expose an in-game command
or a general fence override. Original creation commands are verified, never
executed again. No production instance was recovered during qualification.

## Supported first slice

A candidate needs every original quarantined ordinary player frame, the complete
original creation command envelopes and frozen item payloads, their successful
native receipts, and a complete healthy native player projection. SQL verifies
the original inbox identity, exact outbox result, custody ledger events and, for
schema 2, retained accounting references and source claim. Flat-file verifies its
original operation digest/result and the schema-2 accounting record, references
and source claim. Flat-file accounting records retain the original command;
SQL hashes/results alone do not reconstruct it.

The builder overlays all retained components in increasing revision order. It
keeps current native economic authority, preserves pet custody, and restores
only omitted item UIDs proved by the original creation commands. It refuses
changed grant payloads, topology, recipient/name, duplicate or tied revisions,
unproved missing items, later item revisions, incomplete native reads and
death/quest XP/spell/craft receipt obligations. This slice also refuses grants
inserted into an existing container and changed pet projections. Repeated
historical failures whose frames precede the current baseline stay fenced.

Missing original commands or genuine custody conflicts need separate evidence
or separately authorized restitution. The diagnosis and a higher save revision
provide neither a replacement item nor permission to reopen the player.

## Native owner sequence

The public declarations are in `src/player/player_quarantine_recovery.h` and
`src/player/player_save_journal.h`. A stopped recovery caller supplies the
selected isolated backend and original decoded `critical_command` values:

1. Initialize the copied player journal with `player_save_journal_init`.
2. Inspect one fenced PID using `player_save_journal_recovery_inspect`. Policy
   fences and unsupported archive evidence refuse this operation.
3. Call `player_quarantine_recovery_prepare_flatfile` or
   `player_quarantine_recovery_prepare_sql`. This validates the native evidence,
   freezes the complete baseline/replacement and expected authority, generates
   a recovery identity, then durably publishes the preparation before any
   replacement write. A nonempty expected backend identity must match the
   identity derived from the first verified original command.
4. Call the corresponding `player_quarantine_recovery_resume_*` with the same
   PID and backend identity. It reads the durable record, rechecks the frozen
   baseline and original receipts under native locks, applies the existing
   native component writers, verifies the exact result, and publishes resolution
   before removing this PID's admission fence. Repeating resume is safe.

SQL callers need a dedicated autocommit connection with no existing transaction.
The SQL native load owner can join only the transaction associated with the
matching prepared record. Flat-file uses the existing identity/player/authority
locks and authority transaction. Ordinary load/save entry points remain fenced
throughout preparation and uncertain commit or resolution. A failed operation
keeps evidence; reinitialize from disk before retrying uncertain archive writes.

The backend identity is `sql:<original operation ID>` or
`flatfile:<original operation ID>`. Exact command bytes additionally bind the
schema-2 lineage/epoch/source. Identity is independent of a host path or database
name, allowing a coherent isolated clone. Mixing backend, journal, command or
receipt generations fails verification.

## Storage and restart contract

Archive version 2 retains every original version-1 frame byte and PID manifest.
Its hashed extension stores prepared/resolved/revoked records, including account,
backend identity, complete ordinary baseline/replacement, expected authority and
original encoded commands. Publication uses the existing private temporary file,
fdatasync, rename and directory-fsync protocol. Version 1 remains readable.
Older archive readers refuse version 2; rollback requires a compatible reader.

The replacement commits with an immutable 72-byte proof binding recovery ID,
canonical prepared-record digest, PID and replacement revision:

- SQL uses the existing `critical_operation_inbox` with private discriminator
  `65000`, schema/payload version 1. This is a recovery receipt, never a command
  admitted by the coordinator or replayed from its journal. No migration or SQL
  schema change is required.
- Flat-file uses `metadata/player-recovery-<pid>-<recovery-id>.receipt`, written
  with the replacement snapshot by the existing authority transaction.

Prepared recovery always requires the exact baseline or exact committed result.
Once resolution is durable, the native commit proof and original grant evidence
are reverified before trusting healthy current state at or beyond that revision.
This permits later native economic changes, transfers and ordinary saves without
requiring the player to remain identical to the old replacement. A revision with
missing or conflicting proof never releases the fence.

Boot starts resolved PIDs fenced and verifies them before ordinary journal
replay and player admission. It never applies a merely prepared record. A new
terminal failure revokes an old resolution and fences the PID again. Active
frames for an unverified resolved PID stay byte-exact in the journal while
unrelated PIDs can replay. A policy fence retains precedence.

The existing lifecycle manifest protects the archive, commit receipts, native
player state and transaction evidence together. The flat-file drained restore
qualifier checks resolved records against native proof. Isolated service restore
also refuses the fixed `player recovery revalidation incomplete` diagnostic,
including SQL backend mismatch or missing proof; copied archive bytes remain
unchanged during qualification.

## Bounds

The existing archive cap is 512 MiB, including the new extension. It admits at
most 65,536 recovery records. Each record has 1–64 commands, each bounded by the
existing 512 KiB critical-command codec; baseline and replacement each use the
4 MiB player codec bound; authority evidence is at most 1 MiB; backend identity
and account are at most 256 and 50 bytes. Every archived frame passes its native
header/checksum/codec checks; one candidate is limited to 16,384 frames.

Boot uses a three-second budget checked between records. Native reads retain
their request deadlines; it does not interrupt a filesystem operation or native
SQL call already in progress. Records not verified within that budget stay
fenced for stopped recovery. `quarantined_bytes` continues to report preserved
original frame bytes, not the complete archive file size. This is bounded recovery
qualification, not #490's integrated latency/storage workload qualification.

## Reproducible qualification

- `python3 tests/async/test_player_quarantine_recovery.py`: native archive protocol
  under ASan/UBSan; exact proof refusal, bounded input, verifier exception,
  healthy-PID replay, later active-frame preservation, revocation, ENOSPC/EIO,
  and actual SIGKILL before/after preparation and resolution archive rename.
- `python3 tests/async/test_flatfile_player_repository.py`: native legacy and
  accounting grant recovery; full ordinary component and UID preservation;
  authoritative wallet; conflict refusal; authority-journal/file interruptions;
  repeat, native economic continuation and later-save restart; missing proof.
- `python3 tests/async/test_player_quarantine_restore.py`: native version-1 and
  version-2 restore, original-byte preservation, resolved later-save proof,
  missing/mixed proof refusal and boot-order contract.
- `python3 tests/async/run_player_quarantine_recovery_loopback.py`: native stopped
  recovery on disposable MySQL 8.0 or MariaDB 10.11. It requires explicit
  `TEST_DB_DISPOSABLE=1`, `PLAYER_QUARANTINE_RECOVERY_DISPOSABLE_SERVER=1`, loopback
  `DB_HOST=127.0.0.1`, explicit port/user/password and no socket. It never reads
  checkout `.env`; it creates, migrates and removes only its generated test
  schema. The native fixture checks retained legacy/accounting commands, exact
  components/UIDs/economic authority, real pre/post-COMMIT SIGKILL, conflicting
  evidence, repeat/resume, later ordinary save and missing proof refusal.

Qualification uses synthetic isolated state. It does not establish that original
commands exist for any historical live PID, or qualify death/receipt recovery,
all gameplay accounting routes, full enforcement or production deployment.
