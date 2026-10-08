# Plan 5 restore erasure propagation: current interface and remaining qualification

2026-10-08. This slice establishes the existing refusal boundary and hands the
missing durable propagation interface to the shared owner. It implements no
mutation adapter, schema, policy approval, activation, or relaxation of a guard.
Positive canonical erasure propagation remains unqualified. Independent Plan 5
work and previously qualified empty-ledger restores remain nonblocking.

## Branch and exact source

- Sole local and remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `499cb45c4bdcc98b18cc7eb89ce8238d5acd839d`. Result: this containing commit; exact result/remote,
  clean tree and preserved ancestry are recorded after push in
  `D:/Dev/Tests/Duris/accounting-plan5/erasure-handoff-20261008/delivery/result.json`.
- Refreshed and tested primary: `b55c688ec1790dbd86228ac1188bc579a543d474`.
- Primary plus 27 unchanged owned overlay blobs: `b4c3c8e14783a76a8e492579064044f158530d6f`.
- Archive SHA-256: `f5d1f663cc0c76aa77d654e44de6803b1f83209f8be564523a1b7a4e8112867c`.
- Native tree: `833d3085815b396861ad18a77635412212381e4b`. Migration tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`,
  canonical head 64. No earlier 0055 result qualifies this candidate.
- 6,517 regular source bodies/modes and four symlink targets authenticate before
  and after execution. `source00.json` binds every body and all 27 overlays.
- The composed difference from prior `3a7fe631eefeb0d5864b3e3419f72dac11e0e31e`
  is eight documentation paths only. Primary's latest private Smith/reset source
  candidate remains unpublished and uncompiled; its checkpoint gives source
  progress, not a tested combined release candidate.

Owned files are this report and the additive Plan 5 remote follow-up. Every
production script, original test, shared recipe, contract, producer, migration,
registry and matrix body remains unchanged in this slice.

## Established boundary

The current `account_erasure.TombstoneLedger` keeps tombstones in `_by_scope` in
memory. `write_current` can publish that object's current records atomically;
it does not reconstruct a durable source-wide ledger after restart.
`restore_preflight` filters a supplied list of dictionaries by
`account_scope_hash`. Published literal-call inventory finds its consumers only
in synthetic tests. That list filter is not a native SQL/flatfile adapter and
cannot prove removal from indirect value domains, journals, caches or exports.

Two independent existing guards preserve safety:

1. `persistence_restore.tombstone_preflight` validates a fresh independent ledger,
   then refuses every nonempty `tombstones` list with
   `erasure_propagation_required` before candidate creation/service. It runs again
   before qualification and requires the unchanged ledger digest.
2. `qualify_database_restore.main` requires
   `SELECT COUNT(*) FROM account_erasure_tombstones` to be zero. Supporting a
   nonempty external ledger alone would still leave SQL generations refused.
   Removing this generation invariant alone would prove no propagation.

The canonical inspector reports **281 stores**, all `retain`, with
`request_state=blocked_by_policy`. Destructive rules remain false. This is an
inspection result, not approval to erase anything. The shared account-erasure
guide's historical 194-store wording must not be used as the current inventory.
Account-menu operational deletion remains a separate narrower behavior; it
provides no canonical backup no-resurrection claim.

## Narrow shared-owner interface request

The shared owner must supply and own the durable source-wide adapter before
Plan 5 can qualify positive propagation. No new field or SQL table is installed
or assumed by this slice. The following is a proposed proof contract for owner
review; names are explicit so consumers/tests can bind the same values. Reuse
existing durable evidence where sufficient, rather than creating parallel state.

The existing external JSON envelope remains `version`, `captured_at`,
`policy_sha256`, `tombstones`. Existing tombstone records carry `request_id`,
`account_scope_hash`, `subject_token`, `manifest_checksum`, `completed_at`.
The SQL tombstone also carries `policy_id`, `policy_schema_version`,
`last_restore_generation`, `restore_apply_count`. Preserve their identities;
aggregate count and last-generation fields alone do not prove all source classes.

| Required proof value | Invariant and consumer |
| --- | --- |
| `source_class` | One exact existing class: `database_backup`, `pfile_backup`, `conversion_backup`, `journal_replay`, `cache_rebuild`, `export_spool`. Every reachable class needs completed evidence or an authenticated absence proof. A missing class cannot silently disappear from coverage. |
| `generation_name` and `generation_manifest_sha256` | Preserve the existing `[0-9]{20}-[0-9a-f]{32}` backup name and bind the exact verified manifest bytes. The synthetic filter instead accepts a 64-hex generation token; identify its digest domain explicitly. Do not cast, truncate or treat a directory name as that digest. |
| `ledger_sha256` and `policy_sha256` | Bind the complete fresh independent ledger and current lifecycle policy. Preserve each tombstone's original `manifest_checksum` and authenticate any approved policy transition. Changed evidence requires refusal or a newly bound pass before publication. |
| `source_inventory_sha256` | Bind the actual immutable input scan for this source class to its generation. Includes direct identities and the native joins needed to find indirect references. Unscoped records refuse; fabricated `account_scope_hash` columns on fixtures do not establish native coverage. |
| `store_evidence` | For each applicable manifest `store_id`, bind the approved `action`, `affected_count`, `remaining_direct_identifiers`, `evidence_checksum`, and completed `status`, using existing durable store evidence where possible. Counts/checksums must reconcile actual retained/disposed authority; a nonempty list of success flags is insufficient. |
| Complete propagation receipt | Durably bind the tuple above before login, load, replay, cache publication or export release. Restart and exact retry must recover the same proof. Changed generation, policy, ledger, inventory or store result must not reuse it. Receipt format/storage belongs to the shared owner; no new version is selected here. |

Retained non-personal account lifetimes, operation/root/child IDs, UIDs, source
claims, immutable postings and item history must remain exact. Removing a login
alias is not permission to drop money/item evidence, reconstruct new identities,
rebalance postings, mint/destroy an item, or reuse a durable source. Approved value
disposition belongs to existing transactional domain owners, with current fences
and expected-state proof. Audit tools remain read-only and do not execute it.

The owner must keep external-evidence freshness and change detection, source
quiescence, credential absence and unloadability as independent requirements.
A proof cannot be inferred solely from completed counters or the absence of a
current credential. Historical backups remain unchanged; approved propagation
applies to isolated candidates through owned adapters before they become usable.

## Consumers and acceptance handoff

- Shared erasure/domain owners persist and reconstruct tombstones, scopes, fences
  and per-store results, with exact retry and ambiguous-commit handling. They
  own adapters for all six source classes and the current policy decision.
- Plan 5's `persistence_restore.restore` consumes authenticated complete proof
  before service and rechecks it before `QUALIFIED.json`. Its current unconditional
  nonempty-ledger refusal stays until the genuine adapter is available.
- `qualify_database_restore.main` must consume the same proof before replacing its
  zero-tombstone generation invariant. Both consumers change coherently; neither
  can be waived by an empty test table or the synthetic list filter.
- Replay, conversion import, native SQL/flatfile loads, cache rebuild and export
  release must consume the appropriate proof before their own publication. They
  remain shared/native lifecycle integrations, not standalone audit mutations.
- Independent reconciler/export queries verify retained non-personal economic
  identity and report missing/conflicting proof without modifying authority.

Required native/disposable acceptance is a complete old-generation restore with
newer completed tombstones, plus a second cold restart, on MySQL 8 and MariaDB
and on flatfile. Exercise real direct/indirect records, journals, conversion,
cache and export sources. Verify exact retained economic lineage, source dedupe,
receipt replay, UID custody/history, credential absence and no login/load.

Damage every proof identity and omit each required source/store; inject pending
work, unknown scope, changed policy/ledger/inventory, partial application,
interrupted receipt persistence, lost reply, exact retry and conflicting retry.
Every uncertain/incomplete case must hold before service/publication. Run the
original failed/missing-evidence cases and complete maintenance/pause authority
journeys. Native fixtures, schema replay and a synthetic filter pass alone do
not qualify this acceptance. The current disabled policy is preserved throughout
this slice; no activation or approval request is made.

## Executed commands and results

Runtime: pinned tools image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
Python 3.12.3 (executable SHA-256
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`).
One task-specific container has network `none`, a read-only root, two CPUs,
2 GiB memory, a private extracted source tree and direct D: evidence mount.
It finished exit 0 without OOM. The exact Docker argv is `docker-command.json`.

From the authenticated tree, `PYTHONPATH=/work/scripts:/work/tests/async`,
`PYTHONDONTWRITEBYTECODE=1`, `TMPDIR=/tmp`, `ENVIRONMENT=test`:

```sh
/usr/bin/python3 -B -m unittest -v \
  test_account_erasure \
  test_persistence_backup.RestoreTests.test_tombstone_preflight_fails_closed \
  test_persistence_backup.RestoreTests.test_restore_requires_a_version_one_tombstone_object \
  test_persistence_backup.RestoreTests.test_restore_rejects_tombstones_before_candidate_or_service \
  test_persistence_backup.RestoreTests.test_erasure_evidence_changed_during_restore_never_qualifies \
  test_backup_review_remediations.BackupReviewRemediationTests.test_old_empty_tombstone_evidence_is_rejected
/usr/bin/python3 -B scripts/account_erasure.py inspect
```

**12 original methods PASS; zero failures, errors or skips.** Seven original
synthetic erasure methods plus five restore-boundary methods execute. Tests take
0.409 seconds; the test command takes 0.518273 seconds,
inspection 0.068719; full extraction/authentication stage
7.683605 seconds. Freshness/policy/schema/refusal tests preserve their
original assertions, service mocks and authority inventory checks.

These are Python component/refusal checks. No native compiler, server/service,
database daemon, migration, real erasure adapter, gameplay or producer journey
runs in this slice. Those positive checks require the missing owner adapter and
published combined source. No unchanged known failed build is rerun.

## Evidence, skips and remaining gates

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/erasure-handoff-20261008`. `source00.json`, `interface-audit.json`,
`generation-binding.json`, `literal-callsites.txt`, captured original inputs,
`qualification.json`, raw stdout/stderr, container/runtime records and helper
bodies remain available. Raw regular-file seal SHA-256: `f849b656301d63a0d5545143dff9bab03865aa152cee9eaa4d4b0d5b8895a629`.
`delivery/result.json` separately binds committed document blobs, remote tip,
all seven preserved historical tips, all 27 overlays and rehashed sealed files.
No test scratch or archive is committed.

The [prior backup/restore report](PLAN5_BACKUP_RESTORE_REMAINING_QUALIFICATION_2026-10-08.md)
records eight new and three previously bound original passes: 11 distinct
original methods on its exact same-code source, not a full 12-method pass.
This slice proves eight documentation-only differences, unchanged native/schema,
27 overlays, original fixture/recipe/cache/operator headers and five genuine
provider bodies. The prior seven missing-symbol link failure therefore still
blocks nine original missing-file refusal controls. Its qualification SHA-256
is `554615ee5a33e1c860b6d516211ef2d02366d5781b9d210c9e9b60a031e9851c`;
`interface-audit.json` binds those inputs. The existing genuine-provider owner
handoff is retained without stub providers or relaxed sanitizer/link flags.

The latest broad audit component run's 17 explicit native/SQL opt-ins remain
unexecuted in that run after three separately completed budgets. They are not
new skips in these 12 selected checks; their raw list stays in
`D:/Dev/Tests/Duris/accounting-plan5/snapshot-input-20261008/units04/terminal.json`.
Historical native/canonical passes retain their own exact reader/source scopes.
Current combined producer/player/opening/cutover, all actual native routes,
full original backup module, erasure propagation, remote backup custody,
release-host workload/storage budgets, guarded activation and Plan 5/R1-R8/full
release remain open. Private source milestones and inventory counts do not
qualify them.

Curator-ready additive handoff: the primary's locally maintained notebook is
nonblocking. Application, import and acknowledgment are unclaimed. This slice
publishes only to `codex/accounting-plan5`; no primary push, cross-chat message,
accounting activation, production access, autocorrection, merge or deployment.
Inactive behavior, wallet-root ITEM_MONEY exclusions and the declined inactive
spell-path change retain their existing owners and exact behavior.
