# Player Save Journal

The revisioned player-save pipeline uses the absolute `PLAYER_SAVE_JOURNAL_DIR` path as
its local durable handoff location. A typical checkout uses
`runtime/player-journal/`; that directory is intentionally ignored by Git and must
contain only runtime records. Never copy journal or quarantine files into commits,
tickets, or logs. An absent or non-absolute path prevents player-save pipeline
initialization. Server boot logs and reports the failure and continues, with
nonterminal saves unavailable. Ordinary SQL snapshot reads and the shared
character-materialization path remain blocked until the pipeline initializes and
startup replay completes successfully. See the
[player save pipeline guide](PLAYER_SAVE_PIPELINE.md#configuration-and-health).

## Safety Contract

- The directory must be owned by the server user with mode `0700`.
- Journal, temporary, and quarantine files use mode `0600` and reject symbolic links.
- The default journal byte quota is 256 MiB. New handoffs are refused if they would
  exceed that quota or the active journal already contains 16,384 records. Existing
  journals above the record-count limit are still scanned in full within the byte
  quota; the count limit does not authorize truncation or evidence removal.
- Records older than seven days set the `age_limit_exceeded` health flag reported by
  `world persistence`. Age alone does not reject handoffs or delete journal records.
- Acknowledged revisions are removed through a synced temporary rewrite, atomic rename,
  and parent-directory sync. Revision-only checkpoints retain death and quest XP,
  spell, or craft receipt records; retirement requires the proofs described under
  [Recovery](#recovery).
- Corrupt, truncated, oversized, and unsupported records remain in the active journal.
  Scanning also attempts to copy the rejected bytes to the protected
  `player-save.journal.quarantine.archive`. These errors fail journal initialization
  or scanning and fence all player load/save admission, including across restart:
  a corrupt header cannot reliably identify an affected player. Archiving does not
  authorize compaction or release. An entire journal exceeding its byte quota is
  refused before scanning, without automatically archiving the file.

The `player_journal` row in `world persistence` reports aggregate journal state,
counts, bytes, age, append/checkpoint/replay outcomes, corruption, quarantined bytes,
backpressure, and byte-quota/age flags. It prints no player IDs or payload values.
The row does not display `record_limit_exceeded`; compare `records` with the 16,384
admission limit when diagnosing capacity refusals.

## Recovery

Replay validates framing, CRC32, payload schema, and every DTO bound before calling the
typed repository. Records are ordered by PID and revision; exact duplicates are skipped.

Ordinary snapshots without death or attached operation receipts can be checkpointed
after durable application or a superseding durable revision. Death and quest XP,
spell, or craft receipt records require exact `applied`/`already_applied` success at
the requested revision. For obsolete non-death receipt records only, replay may also
retire the exact payload after the repository explicitly verifies every attached
operation receipt and reports `stale_revision` with a newer durable revision. That
replay-only proof does not acknowledge a live save. A higher revision alone does
not prove a death or receipt committed.

A retryable backend failure or ambiguous commit stops replay and keeps the global
readiness gate closed. Unproven records remain active; already-proven work may be
checkpointed before replay returns failure. A terminal repository failure,
repository exception, or unproven death/receipt result instead archives the player's
unresolved records and persists that player's admission fence before removing those
records from active replay. This preserves evidence without acknowledging a
successful save. Unrelated players can continue replay, and global readiness can
open while the affected player remains quarantined. If that quarantine archive or
compaction fails, all player load/save admission remains fenced.

Eligible per-player quarantine recovery follows the
[stopped recovery guide](PLAYER_QUARANTINE_RECOVERY.md), which requires a coherent
isolated restore with the server and all other writers stopped. This is not a
general fence override: the supported recovery path refuses death and quest XP,
spell, or craft receipt obligations and does not cover unassigned corrupt evidence.

Do not manually edit the journal. Preserve the protected directory for diagnosis when
replay reports corruption or an unsupported format.
