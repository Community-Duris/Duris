# Death recovery visibility (#570)

This completes the engineering visibility scope for issue #570. PR #574 already
made payload-gap loads read-only and routed their deaths into a disputed
disposition. PR #585 already made custody take precedence over NORENT filtering
and guarded exact SQL snapshot writes. Those protections remain in force.

The diagnostic correlation is the first 16 bytes of SHA-256 over
`duris-death-recovery-v1` followed by the existing corpse owner ID as eight
little-endian bytes. That owner ID is `(pid << 32) | corpse_save_id`. The digest
contains no character name, account name, credentials, or item UID. It joins
corpse commands, death save traces, refusal/retry reports, terminal summaries,
and protected restitution inspection/plan artifacts. It is diagnostic only:
existing critical operation IDs, death operation IDs, revisions, custody rows,
and delivery receipts remain authoritative. No migration or wire format changes
are required. Retries derive the same correlation from the corpse relationship
while preserving each branch's existing operation semantics. The port to
experimental-accounting retains its existing sealed terminal request and exact
revision/payload retry rules.

## Refusal and batch context

The result `ITEM_TRANSFER_TOPOLOGY_CARDINALITY` (`0x445201`) has the stable name
`durable_topology_cardinality_mismatch`: the selected durable descendants and
captured transfer items have different cardinality. It is distinct from payload
size limits. Historical errno 90 remains readable as
`legacy_transfer_size_mismatch`; historical evidence is needed to establish
which old check returned it. Other named meanings include `invalid_evidence`,
`authority_or_payload_missing`, `stale_authority_revision`,
`identity_or_projection_conflict`, `authority_busy`, `allocation_unavailable`,
and `storage_io_failure`. Unknown results use `storage_or_integrity_failure` and
retain their numeric code.

`item_uid=0 scope=batch` explicitly means an aggregate corpse handoff. Reports
include captured item and root counts. It does not mean a missing item with UID
zero. Exact UIDs and operation relationships are available in the protected
artifact, where operators can inspect them without putting them in ordinary
status output.

## Protected recovery queries

Staff at the existing FORGER authorization boundary can run:

```text
restitution recovery
```

This displays at most 32 retained corpse commands and 32 held dead characters,
per live page, with total counts, correlation, elapsed time, attempts/report count, named
refusal, and recovery owner. Retry exhaustion stays visible under
`critical_command`; pending publication belongs to `item_movement_publication`;
held disputed deaths belong to `death_disposition`. This simulation-thread query
does no database or filesystem I/O. Follow `restitution recovery <offset>` when
a next-page prompt is displayed. Live pages may shift as recovery progresses. Use the protected offline query for durable
and historical cases:

```bash
python3 scripts/player_death_restitution.py --env-file /protected/dev.env \
  status --artifact /protected/recovery-status.json --limit 50

python3 scripts/player_death_restitution.py status \
  --flatfile-root /protected/state --artifact /protected/recovery-status.json
```

SQL uses the existing exact-target policy and production fingerprint/target-info
controls. The tool never sources `.env` implicitly. Artifacts use the existing
atomic private-file writer (mode 0600). Console output contains only counts.
The scan is bounded to 1â€“100 death records per page; follow `next_cursor` with
`--after-pid` and `--after-revision`. Resolved cases are omitted by default;
`--include-resolved` includes their terminal summaries. `scanned` and
`unresolved_cases` are page counts. Continue paging even if a page contains no
unresolved cases.

The query derives terminal custody from immutable death evidence, the existing
custody table/catalog, current authority, physical projections, and SQL delivery
receipt relationships. It makes no custody change and grants no restitution
approval. The flatfile reader uses production codecs under one authority lock,
refuses a pending authority transaction, and never replays or repairs it. Normal
startup/recovery must finish the existing transaction first.

| State | Evidence and recovery owner |
| --- | --- |
| `durable` | Active authority and a matching physical projection; owner `none`. |
| `restored` | Durable player delivery with a receipt tied to this death and recipient. An applied receipt remains open under `restitution_verification`; the existing protected `verify` must record verification before owner `none` and a resolved summary are possible. |
| `quarantine` | Existing quarantined custody; owner `reviewed_restitution`. |
| `unresolved` | Missing authority/projection, unresolved critical operation evidence, or an outstanding wallet obligation; owner `custody_reconciliation`. |
| `safely_retired` | Explicit retired state with destruction authority; owner `none`. Absence alone never proves retirement. |

Each item retains its own state and owner. The aggregate gives unresolved cases
precedence over quarantine, then restored/durable custody, then retirement.
An applied but unverified delivery sets `verification_requires_review=true`
and keeps the aggregate unresolved under `restitution_verification`, even when
its item has durable restored custody. Only the existing protected verification
workflow can record the verified receipt; this read-only query cannot do so.
`death_disposition=completed` is independent of `recovery_required`: recording a
death cannot certify unresolved items as delivered. Wallet amounts and unresolved
operation IDs remain protected evidence and keep the case open. Currency is
still outside automatic item restitution. Unsupported projection kinds remain
conservatively unresolved until their existing recovery workflow establishes
custody; the query does not infer safe delivery from a receipt alone. Flatfile
restitution delivery is not introduced by this change.

Before quarantine, SQL stores durable-only descendants under captured roots in
the existing `player_death_custody` transaction; flatfile adds them to the
existing death custody afterimage in the same authority transaction. Thus their
UID evidence survives later movement or retirement of the root. Payload bytes
are not manufactured for missing descendants. Unrelated owners/roots stay under
their existing custody rules.

## Alert bounds and verification

Recovery reports retain occurrence counts and elapsed time. The first report and
first failure are emitted; repeated reports emit at powers of two and no more
than once per 30 seconds. Suppression does not discard counts. Terminal summaries
bypass suppression. Exhausted critical commands retain their existing attempt
count and durable operation identity; replay reconstructs the same correlation.
Live report counters reset on process restart. Durable elapsed time comes from
the existing SQL death timestamp or flatfile death file timestamp; retained
critical command elapsed time uses its existing acceptance clock.

Focused executable coverage:

- `test_death_recovery_visibility.py`: production batch refusal callback, named
  results, correlation parity, alert bounds, and all five terminal states.
- `test_critical_command_coordinator.py`: exhausted corpse commands, retained
  ownership, restart/replay identity, and successful reconciliation.
- `test_flatfile_player_repository.py`: durable-only missing descendant, degraded
  cold load, disputed transfer, interrupted authority commit, restart, retained
  custody evidence, private status output, and refusal while replay is pending.
- `run_player_death_disposition_mysql.sh`: native SQL disposition and retained
  missing descendant, followed by `test_death_recovery_visibility_sql.py` for
  protected pagination, pending verification, reconciliation, delivery loss, retirement, and the
  independent wallet obligation. Use `DEATH_DISPOSITION_DB_IMAGE=mysql:8.0` or
  `mariadb:10.11`; only disposable synthetic databases are used.
- Existing custody, transfer compatibility, save pipeline, staff authorization,
  canonical operator handoff, and restitution CLI/reconciliation tests preserve
  admission, fencing, and exact verification rules.

## Historical obligations

Validation on 2026-10-03 passed native disposition fixtures and protected status
journeys on MariaDB 10.11.19 and MySQL 8.0.46 for master and the accounting port.
Both branches passed `make -C src`, the flatfile build, the relevant executable
regressions above, changed-line formatting and formatting checks, and
`git diff --check`. Database/player fixtures were synthetic and disposable;
live gameplay and historical production recovery were not exercised.

No historical assets have been restored and no production migrations or recovery
writes have been performed. The incident's durable-only descendant still needs
payload evidence or an explicitly reviewed disposition; quarantining it does
not reconstruct its generated state. Existing pre-change deaths may lack the
extra immutable descendant UID evidence. The query can discover current rows
under their captured roots but cannot invent a missing historical relationship
after those roots have moved. Preserve incident snapshots, ledger history, death
payloads, and backup evidence for review.

Historical restitution and source repair remain tracked by #526 and #331; the
upstream reward-order investigation in #569 remains separate. Existing artifact,
currency, deleted-character/locker, approval, and maintenance-boundary rules
continue to apply. Issue #570's discussion was moved to #631 as taxonomy cleanup;
that closure is not evidence of historical recovery completion.
