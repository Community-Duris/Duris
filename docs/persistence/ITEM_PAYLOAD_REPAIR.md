# Guarded exact-UID payload repair (#526)

This delivers engineering tooling. It does **not** establish that a historical
item has been recovered or resolve an incident without real, reviewed evidence.
Technical acceptance uses generated synthetic snapshots and disposable databases.
No production data, migration, or historical recovery write is needed for it.

The repair commands in `scripts/player_death_restitution.py` restore a complete
missing physical player payload for an existing UID whose current SQL custody is
active. They do not transfer custody, allocate a UID, create a vnum replacement,
deliver to a recipient, clear a corpse, or modify currency/artifact authority.

## Supported boundary

MySQL 8 and MariaDB 10/11 with the current SQL item/death/restitution schema and
`player_item_runtime_state` are supported. Every relevant table must use InnoDB.
The item must belong to an active player with `owner_context_id=0` and a retained
owner revision. Top-level inventory and a nested item under an existing valid
container are supported. Its physical row must be absent from **all** current
projection tables. Existing scalar-only, corrupt, identical, or conflicting rows
are refused for separate review; absence does not authorize overwriting them.
Other payload gaps in the same player topology must be resolved with sufficient
evidence before this bounded single-item path can apply.

Flatfile authority, other owner domains, retired/quarantined UIDs, equipped
evidence, missing parents, cycles, inconsistent roots, non-container parents,
competing instances, coin payloads, and a UID with an existing restitution
delivery/runtime are refused. A real artifact additionally needs the existing
identity-bound canonical/baseline/binding/legacy checks to pass without
reconciliation, and its current player placement and timer must already agree
with the payload. Repair never seeds/rebinds a registry, extends a timer, or
changes artifact location. A name-marked unique item does not become an artifact.

## Evidence and preparation

Supply an operator-reviewed, owner-only JSON file (0600) containing original
complete **native player snapshot or death snapshot frames**, not a vnum, an item
name, a hand-authored item dictionary, or a bare unbound item list:

```json
{
  "format": "duris-item-payload-evidence-v1",
  "snapshots_hex": ["<hex of an original complete native snapshot frame>"]
}
```

The envelope accepts at most 16 frames and 32 MiB; each decoded frame has the
production codec's 4 MiB limit. The native bridge uses `player_snapshot_decode`,
the shared item-list encoder/decoder, and the production materializer's metadata
validation. The decoded frame binds PID/revision (and death operation ID when
present) to the embedded UID. All supplied occurrences must agree on complete
serialized state. Different property versions, duplicate/unbound identities,
unsupported metadata, and corrupt/future encodings are refusals. The evidence
digest covers the complete envelope, including every supplied version. Operators
must establish provenance and select sufficient retained evidence; a digest does
not prove that an invented or incomplete historical record is authentic.

The private plan carries evidence bytes/digest, native decoded payload, target
fingerprint, current owner/item/save revisions, topology/projection observations,
journal configuration, and optional native backup receipt. No SQL writes happen
during preparation. `applyable` denotes an exact repair candidate; apply still
requires explicit approval and every maintenance/backup guard. A refusal plan
contains `classification` and `note` and is never applyable.

Use an explicit environment file that names the exact database and **the same**
`PLAYER_SAVE_JOURNAL_DIR` and `CRITICAL_COMMAND_JOURNAL_DIR` used by its runtime.
Journal roots must be owner-only absolute directories. Pending journal bytes,
temporary/quarantine files, or pending/dead-letter inbox/outbox work are refused;
finish normal recovery before preparing a repair. This tool does not replay it.

```sh
python3 scripts/player_death_restitution.py --env-file /protected/dev.env \
  repair-prepare --item-uid 52603 --evidence /protected/item-evidence.json \
  --artifact /protected/item-repair.plan.json
```

Review `classification`, `note`, the exact UID and current custody in the protected
artifact. Preparation can run without a backup to classify a case. Before apply,
use the existing stopped-boundary `target-info` and native `backup` workflow in
[the restitution guide](PLAYER_DEATH_RESTITUTION.md), then repeat preparation with
`--backup-receipt /protected/item-backup.json`. Protect the backup and receipt;
they are required for this repair even on a development target. Production
selection retains the existing exact-name confirmation, literal-loopback address,
fingerprint and target-info requirements. Production application additionally
requires the stopped/masked service or immutable stopped-container controls from
that guide. These are tooling capabilities, not authorization to operate on
production as part of #526's technical acceptance.

## Explicit apply and receipts

```sh
python3 scripts/player_death_restitution.py --env-file /protected/dev.env \
  repair-apply --plan /protected/item-repair.plan.json \
  --evidence /protected/item-evidence.json --offline-proof /protected/quiescence.proof \
  --approve --actor reviewed-operator --reason exact-UID-payload-evidence
```

For a production-classified target, supply the existing `--target-info`,
`--confirm-production-target`, `--expected-fingerprint`, `--maintenance-kind`,
`--maintenance-id` and, for user-systemd, `--maintenance-owner` controls as well.
The tool never stops a service or infers approval from elapsed time.

Application validates the opened evidence again through native code, rechecks
the backup/target/journals and live process/session/transaction visibility, and
acquires the runtime's database advisory exclusion. A SERIALIZABLE transaction
locks authority, receipt and physical projection ranges. It rechecks the bound
row cardinalities/digests and immutable evidence bytes before mutation and checks
exclusion ownership before COMMIT. Missing or stale fences result in ROLLBACK.
SQL errors or a disconnect before commit roll back the complete transaction.

The transaction inserts `player_items`, its complete `player_item_runtime_state`,
canonical affects/extra descriptions, and the durable repair receipt atomically.
Native state includes generated modifiers/key, every supported flag/string,
timers, static/dynamic affects and extra descriptions (including spellbooks).
Custody supplies root/parent placement; the repaired physical item is inventory
(`equip_slot=0`). Placement is the only serialization projection. No ownership
revision, item revision, player save revision, ownership ledger, UID allocator,
currency row or artifact authority is changed. Subsequent ordinary saves retain
their existing revision and accounting rules.

The existing receipt tables are reused without a migration. The receipt ID is
namespaced from the complete plan digest; `player_death_restitution_item` uses
`classification='payload_repair'`, offline disposition **30**, immutable metadata
bytes/digest and equal before/after custody revisions/topology. The parent receipt
stores source snapshot PID/revision/operation identity (a snapshot digest-derived
identity for a non-death frame), target-bound plan/evidence digests, actor/reason,
`candidate_count=1`, `delivered_count=0`, and status **2 (applied)**. No delivery or
death-restitution runtime row is inserted. These legacy column names do not imply
that a non-death snapshot was a death or that the item was delivered.

A matching completed plan is idempotent, including an uncertain response after
COMMIT. The transaction's item/receipt locks serialize the per-UID repair guard.
Another plan or conflicting evidence for that repaired UID is refused. Repeated
apply does not certify a payload that has subsequently disappeared or changed;
verify reports that independently. A second loss needs separate reviewed repair
bookkeeping, not deletion of an old receipt.

## Verification and restart procedure

`repair-verify` separately checks exactly one global instance, expected active
custody and parent projection, every canonical serialized field, the complete
native runtime bytes, affects/extra descriptions and the immutable receipt.
It requires the same stopped/backup/target/evidence/journal controls. Only after
the checks pass does a fenced transaction record status **3 (verified)**:

```sh
python3 scripts/player_death_restitution.py --env-file /protected/dev.env \
  repair-verify --plan /protected/item-repair.plan.json \
  --evidence /protected/item-evidence.json --offline-proof /protected/quiescence.proof \
  --actor reviewed-operator --reason exact-payload-verification
```

For an authorized real repair, also complete the existing gameplay/persistence
journey: load the player through the production repository, check the exact UID
and container chain and generated state, perform an ordinary save, stop/restart
the process, then cold-load and compare again. Stop/drain the runtime before the
final offline verification. An applied receipt is never sufficient evidence of
historical recovery. If legitimate gameplay changes serialized state before
exact verification, the original plan no longer certifies exact fidelity.

The SQL loader preserves original affect slots and extra-description/spellbook
encoding from the complete companion when they still agree with canonical SQL
metadata. It continues to use canonical metadata when it differs; the companion
does not overwrite a later authoritative metadata edit.

## Classified refusals and executable acceptance

Preparation reports `missing_evidence`, `missing_uid_evidence`,
`insufficient_evidence`, `conflicting_evidence`, `ambiguous_evidence`,
`corrupt_or_unsupported_encoding`, `custody_absent`, `retired_or_inactive`,
`unsupported_owner`, `unsupported_backend`, `unsupported_placement`,
`owner_missing`, `identity_conflict`, `invalid_topology`, `competing_instance`,
`existing_payload_conflict`, `existing_delivery`, `currency_refused`,
`artifact_reconciliation_required`, `artifact_identity_conflict`,
`pending_authority`, `pending_authority_unproven`, `state_limit`,
`conflicting_repair_receipt` or `already_applied`, with a reason. Apply also refuses
changed evidence, stale plans, missing approval, target/backup/maintenance changes,
and failed transaction fences. Verification distinguishes receipt, custody,
projection, topology and payload-fidelity failures. Do not bypass a refusal by
editing the plan digest or removing authority/receipt rows.

Run the focused disposable acceptance on both engines:

```sh
DURIS_REPAIR_DB_IMAGE=mariadb:10.11 \
  bash tests/async/run_player_item_payload_repair_mysql.sh
DURIS_REPAIR_DB_IMAGE=mysql:8.0 \
  bash tests/async/run_player_item_payload_repair_mysql.sh
```

`DURIS_TEST_TOOLS_IMAGE` selects the repository's existing SQL build image.
The suite creates its own containers, schema, account, player, native snapshot
evidence and actual native dump backup; it never reads `.env`. It covers generated
and nested repair, evidence refusals, changed/retired custody, competing physical
instances, invalid topology, pending work, stale item/owner/save fences inside SQL,
statement failure/client interruption rollback, explicit approval, idempotency,
verification failures, and exact production codec/load/save fidelity across
separate process invocations. The tools container's disposal removes all synthetic
player/evidence/backup data.
