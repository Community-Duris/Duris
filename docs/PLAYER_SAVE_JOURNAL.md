# Player Save Journal

The revisioned player-save pipeline uses the absolute `PLAYER_SAVE_JOURNAL_DIR` path as
its local durable handoff location. A typical checkout uses
`runtime/player-journal/`; that directory is intentionally ignored by Git and must
contain only runtime records. Never copy journal or quarantine files into commits,
tickets, or logs. Startup fails closed when the variable is absent or not absolute.

## Safety Contract

- The directory must be owned by the server user with mode `0700`.
- Journal, temporary, and quarantine files use mode `0600` and reject symbolic links.
- The default journal quota is 256 MiB. New handoffs fail closed when the quota is full.
- Records older than seven days set an operator backpressure alert; they are not deleted
  without durable revision evidence.
- Acknowledged revisions are removed through a synced temporary rewrite, atomic rename,
  and parent-directory sync.
- Corrupt, truncated, oversized, and unsupported records are copied to the protected
  quarantine file before removal by a successful compaction.

`world persistence` reports only record counts, bytes, age, replay outcomes, corruption,
and backpressure. It never prints player IDs or payload values.

## Recovery

Replay validates framing, CRC32, payload schema, and every DTO bound before calling the
typed repository. Records are ordered by PID and revision; exact duplicates are skipped.
Applied, already-applied, and superseded records are checkpointed. A retryable database
or worker failure stops replay and leaves all remaining records intact.

Do not manually edit the journal. Preserve the protected directory for diagnosis when
replay reports corruption or an unsupported format.
