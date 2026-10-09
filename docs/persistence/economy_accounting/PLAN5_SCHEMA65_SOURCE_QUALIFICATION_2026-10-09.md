# Plan 5: schema-65 source compatibility and independent release-gate review

## Outcome and exact scope

The 17 existing independent Python reader/operator modules pass 456 methods,
with 20 explicit skips and no failures/errors against this new source composition.
The published candidate's two original source-contract modules have 71 passes,
one failure and one error. The two defects are stale shared test selectors;
neither is caused by the owned overlay. Published provenance is now exact:
all 393 selected source pins match raw Git bodies, and its matrix is current.
This supersedes the earlier 242-pin CRLF/older-content finding for this published
source only. No native/compiler/database/server/gameplay/recovery execution ran.
The whole Plan 5, original Plans 1–5/R1–R8 and release remain incomplete.

Owned branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Owned base: `3328f4c194cc0e2184dcb33f92808a3510dfe67d`. Result commit and remote verification are bound
by `delivery/result.json` after publication. No branch switch, local merge, rebase,
force push or primary-branch push is performed. All seven historical tips and
27 owned file bodies remain preserved. Primary shared files are not edited.

## Frozen inputs and execution

Primary source: `c5910719018c42260bc54166a81462c212b22d08`.
Native tree: `b098d2bbf1403a8dfc27af0abb15f4bd903c598e`.
Migration tree: `a22d54a28286200f09d91d11cb0cbd8c782b0b82`; canonical manifest terminates at 0065.
Published complete tree: `2e6c1d152ebc6840aa47027e7a67885377bf6bcb`.
Published archive SHA-256: `eeeefb568a83765fab787a8fa0c709983aea3540fc4722b5c993047fc3d126ed`.
Owned 27-overlay composition tree: `a61119d9ba8d8491ce0c4a408d8aa34f9e574e75`.
Composition archive SHA-256: `293d82e3e4c1f9d93f7ea9ef2121df7ed1020de5fe3b8cd71bff4d7df5814464`.

The published archive contains 6,619 regular files and four link targets; the
composition has 6,630 regular files and the same four targets. Every regular
body/mode and each link target is checked before and after execution. Private
candidates, runtime `.env`, unrelated WIP and production data are not imported.
The composition substitutes the exact existing 27 owned bodies; it is not the
primary's smaller candidate and is not reported as qualification of that binary.
Eleven owned paths are absent from the primary; exact paths/blobs/hashes are in
`pin-composition-audit.json`. No missing implementation is concealed by inventory.

Image: `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Python: 3.12.3, binary SHA-256
`e50d468e8b0adfb05733f5b87b3cff34829c4a8c1aea50c865aa8bdfe4bb150f`.
Each Docker command uses `--network none --read-only --cpus 2 --memory 2g`, a
D:-backed evidence bind, private `/work` and `/tmp` tmpfs roots and no native opt-ins.
Exact host Docker argument arrays are retained in both `docker-command.json` files.
No database backend is executed. SQL/flat rows below are registry metadata only.

```sh
python3 -u -B /evidence/observer.py
python3 -u -B /evidence/published/observer.py
python3 -B scripts/validate_economy_accounting.py --root /work
python3 -B scripts/validate_economy_accounting.py --root /work --release
python3 -B scripts/generate_economy_writer_coverage.py --check
```

Observers load each named original unittest module in-process with the existing
loader/runner; no assertions, deadlines, native recipes or provider lists are
changed. Container collection exit 0 is not an all-green test result.

| Composition module | PASS | FAIL | ERROR | SKIP |
| --- | ---: | ---: | ---: | ---: |
| test_reconcile_economy_accounting | 170 | 0 | 0 | 3 |
| test_economic_sql_uid_scope | 13 | 0 | 0 | 0 |
| test_economic_sql_audit_origins | 55 | 0 | 0 | 3 |
| test_item_equipment_reconciliation | 17 | 0 | 0 | 0 |
| test_economic_sql_canonical_audit | 63 | 0 | 0 | 4 |
| test_plan5_child_identity | 21 | 0 | 0 | 2 |
| test_ship_coffer_audit | 5 | 0 | 0 | 0 |
| test_guild_treasury_audit | 5 | 0 | 0 | 0 |
| test_sql_auction_custody_audit | 6 | 0 | 0 | 1 |
| test_sql_shop_custody_audit | 6 | 0 | 0 | 1 |
| test_sql_player_custody_audit | 7 | 0 | 0 | 1 |
| test_sql_corpse_custody_audit | 7 | 0 | 0 | 1 |
| test_sql_locker_custody_audit | 7 | 0 | 0 | 1 |
| test_sql_siege_custody_audit | 7 | 0 | 0 | 1 |
| test_sql_saved_ground_custody_audit | 10 | 0 | 0 | 1 |
| test_sql_room_item_custody_audit | 6 | 0 | 0 | 1 |
| test_persistence_backup | 51 | 0 | 0 | 0 |
| test_economy_writer_coverage_contract | 54 | 2 | 1 | 0 |
| test_audit_accounting_invariants | 16 | 0 | 0 | 0 |
| Total, 549 methods | 526 | 2 | 1 | 20 |

The 20 skips comprise 17 explicit native/SQL methods and three budget methods;
full names/reasons are retained in `qualification.json`. Prior eight native
custody passes retain their exact historical schema-64/provider scope; they
are not requalified against this new native tree. Composition stage: 173.269 s.
Published control: 57 writer-contract methods (55 pass, one failure, one error)
and 16 invariant methods (all pass), zero skips; stage 31.165 s.

| CLI | Published | Composition |
| --- | --- | --- |
| Default contract validator | exit 0 | exit 0 |
| Release validator | exit 1, executable evidence missing | same |
| Generated matrix check | exit 0 | exit 1, composition coordinates changed |

Both default validators record 14 fixtures, 931 routes and 2,911 lexical candidate
occurrences; release_ready=false. Draft census/registry and coverage remain false.
The release assessment has 791 blocking routes; overlapping reasons are 772
missing executable evidence, 788 unqualified coverage and 791 unqualified backend
results. First missing-evidence route: account.item_reward. Each backend's 926
unverified registry rows include dormant/nonwriter candidates and are not a count
of unsupported live writers. Inventory/lexical mapping is not semantic coverage.

## Narrow primary-owned test handoffs

File: `tests/async/test_economy_writer_coverage_contract.py`.

1. `test_exact_room_payload_component_is_scoped_and_unqualified` at line 113
   calls `current_site` with the common INSERT text, demanding one hit. Current
   source has two genuine sites: line 724, `sql_room_item_payload_record_creation`,
   owned by `item.sql_room_payload_creation`; line 1430,
   `sql_room_item_payload_record`, owned by `item.sql_room_payload_record`.
   Use the existing function-owned `source_site` selector for the intended
   original component and verify the second site independently. Preserve unique
   registry ownership, empty executable evidence, legacy/unverified disposition,
   closed admission and all original component/refusal assertions. Do not remove
   either real site or promote coverage to make this check pass.
2. `test_bandage_consumption_sites_keep_authority_and_live_projection_distinct`
   at line 735 still indexes lines 1214/1011/1230. The current sites are
   1277/1074/1293: `do_bandage` submission / `publish_bandage_consumption` live
   projection / `do_bandage` legacy extraction. Reuse the existing function-owned
   `source_site` lookup for these expressions, preserving the three distinct
   route IDs and activation-blocking assertion. Avoid another hardcoded offset.

Both defects reproduce on the untouched published source. Run the same complete
original writer-contract module, not only the failing methods, after repair;
retain `test_audit_accounting_invariants`, default/release validators and matrix
check as the existing consumers. No shared test or schema change is installed here.

## Composition provenance handoff

Published `writers.json:candidate_worktree_evidence.source_pins` and the matrix's
copied metadata now correctly bind 393 raw Git files. The overlay changes seven
of those files; its provenance failure is therefore expected, not stale primary
metadata. Before any owned integration, independently review the existing owned
slices and actual resulting bodies, then refresh those exact pin fields and the
matrix. Do not bind unadopted files to the published candidate or weaken the raw
SHA-256 assertion at source-contract line 145. The selected composed hashes are:

| Changed pinned path | Composed raw SHA-256 |
| --- | --- |
| scripts/economic_sql_audit_snapshot.py | `85a56c7f6aaf50f00fa8053e394160068c71f926109a1d986fa860ce4d58c5ee` |
| scripts/reconcile_economy_accounting.py | `672c9e4af63ada9b529dae23d56d9ae8863ee251b293484878bfd52c2a1bf784` |
| scripts/persistence_restore.py | `d99f13f323a2558a2cac6884145bba501f17be623d6e43322a045b695dbff7f1` |
| tests/async/test_reconcile_economy_accounting.py | `3d94a5fb9cc357712918635a9b8c0ac8153ad53e455516ba832f51a18e3b1d35` |
| scripts/persistence_backup.py | `1e22f6c46c7f065b5679bf03c6c15cf1219293eb4444039cef59c4efbb6c7f92` |
| tests/async/run_economic_sql_audit_snapshot_mysql.py | `84e7ea63c7c41881be5f3bdfd38ec017965e51c1237a0f07b399ac9c75d9d88b` |
| tests/async/test_persistence_backup.py | `11c121a447686b5cb114bb20b73494da0752995ccbdfd6b407871761cd8ea74f` |

Only `backup.capture.source.definition_lines` (715 to 741) and
`backup.retention.source.definition_lines` (497 to 518) differ in the composed
matrix. All other route fields and all non-route metadata remain exact. The
published matrix needs no coordinate repair. This is an adoption handoff only;
no registry/matrix/schema/interface edits are made.

## Remaining qualification gates

The old missing `persistence_mode_sql_enabled` call has zero occurrences in the
current published src; it is no longer reported as the same source blocker.
Published native build/boot results remain bound to their recorded ec632/8e5
commits, not this newer native tree. The current migration tree is the repaired
schema-65 input in the local combined-candidate review. That review reports two
SQL prerequisite failures before service boot: MariaDB nullable-default metadata
representation and runtime manifest/compiled contract still sealed at schema 64.
Those observations are retained as a published owner report, not newly executed
Plan5 DB proof. Primary owns their guarded migration/runtime-contract repair.

Broad native/database/gameplay/recovery qualification remains in the original
major-plan batch. Required original recipes and genuine observations, complete
backup/restore/retention custody, durable six-source erasure propagation, full
32 MiB prospective overlap, measured operations/storage/checkpoint/reconciliation
budgets, real producer/replay/restart/lost-reply journeys and both-backend writer
qualification remain unproven. The named SQL post-ACK room-coin boot defect is
already solved at its recorded scope and is not presented as a current blocker.
No activation, production mutation, audit autocorrection, deployment or merge
occurs; inactive behavior, wallet-root ITEM_MONEY exclusions and the declined
inactive spell change are preserved. Current source tests do not complete release.

## Evidence and curator disposition

Evidence: `D:/Dev/Tests/Duris/accounting-plan5/combined-reader-source-20261009-c591071`.
Helpers: `D:/Dev/Temp/accounting-plan5-combined-reader-source-20261009-c591071`.
No new build output is created. `source00.json`, published/source00.json, both
archives, original module logs, both qualification.json files, assessments,
exact site-owner/coordinate records, raw provenance comparison, terminal container
identities and tool/command records are retained. Raw evidence seal SHA-256:
`7274780063ceb007724386d4a2531841f673478a063dbbffa1983ac99f4800bf`. Post-push delivery rehashes every sealed regular file and binds
both owned docs, resulting commit/remote, 27 bodies and seven ancestor tips.

This report and the additive remote follow-up are curator-ready. The primary's
local notebook remains nonblocking. Notebook import/application, primary source
adoption and acknowledgment are unclaimed without evidence. No cross-chat message
is sent. Full goal remains active and unfinished after this source progress.
