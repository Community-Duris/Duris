# Persistence incident diagnosis

Use this workflow when a character cannot save, enters a degraded load, or is
refused play because its native save evidence is quarantined. Start with the
player report, follow the first failed revision to its item or operation, and
inspect the selected authority before considering recovery.

## Capture resident state

An avatar or higher can request one target at a time:

```text
world persistence
world persistence diagnose player 9001
world persistence diagnose item 7001
world persistence diagnose operation abababababababababababababababab
```

The first command includes aggregate diagnostic retention/loss counters. The
three detailed commands return paged JSON. Save the complete JSON object, without
client formatting, pager prompts, or the command prompt, in an owner-only file.
Player IDs are durable numeric PIDs, item IDs are native UIDs, and operation IDs
are the original 16-byte IDs written as 32 hexadecimal characters. Names and
command/result payloads are excluded. Numeric identities are still private
operational data: keep these reports out of public logs and Git.

The detailed command only copies resident metadata. It does not open files,
query SQL/Redis, initialize or replay a journal, acknowledge work, or alter a
fence. A player report includes current/queued/in-flight/acknowledged revisions,
dirty components, live/degraded status, save/login admission, pending save state,
archive counts, global/policy fences, and resident recovery disposition metadata.
Item reports include cached custody; operation reports include a cached result.
These copies occur separately and can change during capture. `complete:false`
and the monotonic observation interval make that limitation explicit. An absent
runtime entry does not mean the native row or operation is absent.
Journal and save coordinator copies use try-locks. Their availability flags
identify contention; unknown save/admission decisions become JSON `null`.
The detailed command avoids the legacy admission helpers that wait behind
journal I/O. Reports travel in bounded text chunks and bypass player-log recording.

## Assemble a private incident report

Run the stdlib-only doctor on the Linux server or an authorized clone. It can
classify a runtime report without accessing a database:

```sh
install -d -m 700 /tmp/duris-incident
# Place the captured JSON here, then restrict it:
chmod 600 /tmp/duris-incident/runtime.json
python3 scripts/persistence_doctor.py \
  --report /tmp/duris-incident/runtime.json \
  --binary bin/server/dms_new \
  --output /tmp/duris-incident/incident.json
```

For MariaDB authority, add an explicitly selected environment file:

```sh
python3 scripts/persistence_doctor.py \
  --report /tmp/duris-incident/runtime.json \
  --env-file /private/selected-authority.env \
  --binary bin/server/dms_new \
  --output /tmp/duris-incident/incident-with-sql.json
```

Without a runtime report, select `--pid 9001`, `--item 7001`, or
`--operation abababababababababababababababab`. A supplied target must match the
runtime report. The doctor never discovers or reads the checkout `.env`
implicitly. The selected file must be mode 0600 or stricter, and contain
`ENVIRONMENT`, `DB_HOST`, `DB_PORT`, `DB_USER`, `DB_PASSWD`, `DB_NAME`, and
`DB_ALLOWED_TARGETS`. The exact `host/database` pair must be in that allow-list.
Use the incident's actual `PERSISTENCE_MODE`; a flatfile report cannot be combined
with SQL authority. Use a read-only database account when available.

SQL capture uses one repeatable-read, read-only consistent transaction. It reads
scalar player revision, current custody/owner revisions, player/pet payload
topology, recent ownership ledger, original inbox receipt hashes, and direct
operation outbox hashes. It does not fetch item descriptions, names, raw
commands, results, or outbox payloads. SQL errors expose only their numeric code.
The runtime report and SQL snapshot remain separate observations; their union
is not a coherent recovery generation.

Outputs are new mode-0600 files in an existing private directory. Existing files
are never replaced. stdout contains aggregate finding/gap counts only. The
optional executable hash identifies the explicitly supplied binary; it cannot
prove that binary was the process that produced the runtime report.

## Follow the failure to the source

`assessment.last_failed_save` selects the most recent retained failed save.
`history.incidents` preserves the first terminal or exhausted observation for a
PID/revision or operation even when repeated retries wrap the ordinary history.
The witness carries the offending UID when available, snapshot/native presence,
expected and observed vnum/root/parent/slot, observed item revision, and the line
in `src/player/player_snapshot_repository.c` that refused the write. Use the
incident's source revision or executable hash when looking up that line.
Zero-valued fields can mean evidence was unavailable; read the presence flags.

| Code | Diagnosis | Next investigation |
| --- | --- | --- |
| 1–3 | Invalid snapshot item/parent or duplicate UID | Inspect capture and the sealed graph. |
| 4–5 | Malformed custody or duplicate equipment slot | Inspect native current rows and the original operation that created them. |
| 6 | Active custody absent from snapshot | Follow the UID's creation/transfer result and publication relative to save capture. |
| 7–9 | Vnum mismatch, duplicate match, or snapshot UID absent from custody | Compare the frozen payload with current custody and original ledger/receipt. |
| 10 | Invalid custody topology | Inspect root/parent chains, cycles, and depth. |
| 11–12 | Invalid death payload or saved UID missing from it | Evaluate the original death disposition and corpse evidence. |
| 13–14 | Orphaned player/pet saved row | Preserve the sole payload and investigate the missing custody history. |

The rolling history correlates `save_capture → save_journal → save_apply →
save_result → save_checkpoint → save_ack`, plus fence and timeout observations.
`save_checkpoint` is the worker's journal acknowledgement attempt;
`save_ack` is the successful game-thread revision acknowledgement. Command
history correlates admission, journal, apply, result, and publication checkpoint.
The operation ID and entity keys connect item movement to a player's save.
Load results include the request ID and degraded component mask. Outcomes retain
the numeric values of their source enums; stage names identify which enum applies.
Observations are diagnostic context and do not establish durable authority.

For code 6 after a creation grant, compare the ordering with
[creation/save ordering](economy_accounting/CREATION_GRANT_SAVE_ORDERING.md).
For native graph findings, inspect the reported UID and its parent/root. A
topology lag can have an existing safe reconciliation path; an owner conflict,
orphan, or missing payload requires its original evidence. Turn a confirmed
ordering or projection bug into a focused executable regression before changing
the save path.

## Decide the next recovery step

The assessment distinguishes a global journal fence, operator policy fence,
quarantined PID, degraded/conflicting graph, and pending durability/publication.
It always reports `recovery_verified:false`. Prepared/resolved resident metadata
alone does not verify a recovery candidate. Follow
[native quarantine recovery](PLAYER_QUARANTINE_RECOVERY.md) for a coherent stopped
clone, all original archive frames and integrity checks, exact original creation
envelopes/frozen payloads/receipts, a complete healthy player projection, unchanged
custody history, and separate death/quest/spell/craft obligations.

The doctor offers no repair or override mode. Keep the native bytes and original
operation identity while investigating. Raising a revision, recreating an item,
or clearing quarantine from the reason code would bypass the required proof.

## Bounds and coverage

The recorder retains 4096 rolling events and 64 first incidents in less than
2 MiB of fixed storage. Writers use a try-lock, copy scalars only, and perform
no allocation, external I/O, or wait. Contention drops a diagnostic event and
increments a counter. Reports retain at most 64 recent matching events plus
64 incidents. Commands retain their first 8 entity keys and flag truncation.
Overwrites, dropped events, incident evictions, and shortened target windows
appear in the report and assessment. History resets on process restart/copyover;
the run identifier prevents joining sequence numbers across runs.

The doctor caps input/output at 8 MiB, current custody and each payload collection
at 8192 rows, recent ledger/receipt/outbox collections at 256, and SQL capture at
30 seconds. Collection truncation is an evidence gap. Payload inspection covers
player and pet projections; other owner stores, complete native command content,
and flatfile authority decoding remain outside this inspector. Both backends
support the resident report. Every result remains partial, including an otherwise
healthy-looking graph.

Focused validation lives in `test_persistence_diagnostics.py`,
`test_persistence_doctor.py`, `test_player_save_worker.py`,
`test_persistence_severity.py`, and `test_player_save_item_reconcile_mysql.py`
under `tests/async/`. The optional doctor's SQL fixture requires an explicitly
disposable loopback `economic_schema_test_*` database and does not read `.env`.
