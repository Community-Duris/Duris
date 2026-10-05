# Plan 5: new native component build and contract handoff

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Refreshed primary is `0723e10a5a81615465e0727d18e9f837d9b93873`.
Frozen tested import/base is `cd89d4b0240fa1b484431a2af9a01012699c3b83`;
its native tree is `6313ff3d7a340788606e0ba7bc5015cdc72dc773` and migration
tree is `1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
Result commit and verified remote SHA are supplied in delivery.

The ordinary primary import conflicted only in the two owned reader/test files.
Both incoming blobs exactly matched their pre-index-repair d0d570 versions
(`bfdfb29e67be740071f5b15369c94a7ab39f28c7` and
`a910bface1503ad78d8e8375e8d894ad72afd9e2`). Resolution retained the complete
82a44c3 reader/test blobs byte-for-byte; no incoming owned-file change was lost.
Conflict snapshots and the exact resolution record are retained. The import
was pushed only to the Plan 5 branch and verified remotely.

Delivery refresh finds primary `078f8f8d2e6438a6c4d9b963158008ba62dce5d5`,
which adds flat NPC bundle preparation and shop insertion-order work. Its native
tree is `d6cc4e5e6f16ebaee18af9902a1ac57e02e44d37`; migrations are unchanged.
The offending quest translation unit, macro and prototype header remain exactly
unchanged there. This report qualifies the frozen attempted source above and
does not relabel it as execution of the newer candidate.

This candidate adds three maintained dormant components: NPC canonical values/
ordered capture, borrowed native NPC SQL image participation and complete shop
world observation. The earlier e018 build/restore proof cannot qualify them.
Fresh SQL and flatfile builds both fail at one established compiler defect;
no server link, service boot or current-candidate restore qualification is claimed.
Owned tracked file for this slice is this report only. Shared source, registry,
matrix, generator and contract tests are handed back to the primary owner.

## Native compiler defect and narrow primary repair

`src/world/quest_mobile_native.c:363` expands `GET_RNUM(mob)`. Its includes
provide `core/structs.h` and `core/utils.h`, but no declaration of
`panic_corruption_int`, which that macro references at `core/utils.h:371`.
The existing declaration is at `src/core/prototypes.h:3127`:

```cpp
[[noreturn]] int panic_corruption_int(const char *component, const char *fmt, ...);
```

Both maintained flag sets diagnose:

```text
./core/utils.h:371:24: error: 'panic_corruption_int' was not declared in this scope
```

Exact offending source SHA-256:
`871b247307723cbdae9e5aa29a21ed8f0f48bfb90ff5c1202b8f5b7bcfe8601d`.
Existing prototype-header SHA-256:
`70827ae9834b5755dcee777fa322dd36e3c0fe208e93a418b0ccedaae4aa1e1b`.

Requested primary change: include the existing `core/prototypes.h` in this
translation unit. Keep `GET_RNUM` and its PC/NPC corruption guard intact;
do not replace it with raw field access, suppress diagnostics or add a second
declaration. There are no new fields, schema or public API changes. Consumers
are both maintained backend builds and the native NPC capture component.

The repair proposal is made concrete in a copied-source compiler probe:
`quest_mobile_native-include-probe.c` adds just that include after the existing
structs include. It compiles successfully with each exact maintained command's
flags, producing separate SQL and flatfile objects. Original shared source
bytes remain unchanged. `include-probe-results.json` retains both argument
arrays, exit codes, original/overlay hashes and unchanged-source assertions.
This is proposal compilation, not a repaired maintained build or linked server.
Primary validation still requires formatting, both full maintained builds,
native component checks and applicable persistence/runtime qualification on
the published repaired source.

## Exact maintained commands and results

All runs use Docker image `duris-plan5-origin-sql-tools:local`, verified immutable
ID `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with Ubuntu 24.04.4 and GCC 13.3. Source is read-only at `/workspace`, `bin/`
is writable, networking is disabled. Recorder calls alone also mount `tmp/plan5/`
writable. No production environment, database, server or player state is used.
The existing maintained C++20 hardening and full `-Werror` warning profile are
preserved. Each initial build starts with a fresh object directory:

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  OBJDIR=/workspace/bin/tests/plan5-native-components-build-2026-10-05/objects-sql \
  DMS_BINARY=/workspace/bin/tests/plan5-native-components-build-2026-10-05/server-sql

make -C src -j2 PERSISTENCE_BACKEND=flatfile \
  OBJDIR=/workspace/bin/tests/plan5-native-components-build-2026-10-05/objects-flatfile \
  DMS_BINARY=/workspace/bin/tests/plan5-native-components-build-2026-10-05/server-flatfile
```

| Backend | Initial exit/time | Initial compilation commands | Continued exit/time | Continued compilation commands | Unique units attempted | Objects |
| --- | --- | ---: | --- | ---: | ---: | ---: |
| SQL | 2 / 347.1675356 s | 494 | 2 / 115.5728909 s | 226 | 719 | 718 |
| Flatfile | 2 / 363.8866325 s | 494 | 2 / 123.8470912 s | 226 | 719 | 718 |

After the original failure, each command is rerun with `make -k` and the same
paths solely to collect remaining translation-unit diagnostics. These continued
runs reuse already successful objects and are explicitly not clean builds.
Each retries the same failed unit and compiles the remaining units. No further
compiler defect or warning is found. The other two new component objects compile
under both flag sets. No server binary exists for either backend.

The copied-source include proof runs
`python3 tmp/plan5/probe-native-components-build-handoff.py`, exit 0;
both native compiler invocations within it exit 0. Exact native flags and output
paths are in `include-probe-results.json`. No database service or SQL/flatfile
mutation participant is executed by these compilation checks.

## Shared contract findings and requested primary reconciliation

The initial command omitted `PYTHONPATH` and stopped with an `_paths` import
error. Its original log and exit are retained; it establishes no source defect.
The corrected command is:

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
python3 -u -m unittest -v \
  test_economy_writer_coverage_contract test_audit_accounting_invariants
```

It runs 71 tests with zero skips: 68 pass and three fail, in 12.424 unittest
seconds (13.4497254 outer seconds), exit 1. All three failures belong to
`SplitEconomyActivationContract` in the primary-owned coverage test.
Exact tested coverage-test SHA-256:
`50970db7b51ba6fbc7d4bea3101c3808822e76ce9f950554df34e7fdfd6db139`.
Audit-invariants test SHA-256:
`a5c0250fb9e40c77f213820c837747e14d4ff8be70745ec5b514b449454657f1`.
Generator SHA-256:
`dd8dc8d31ec9d8eab3617a8333dcd36154337722a8618566354e3031b1462a6b`.

1. `test_checked_item_placement_refactor_keeps_all_sites_classified`: the test
   expects nine checked-publication sites; the current census has ten unique
   path/line sites (11 lexical records because the remove call is duplicated).
   Its six actobj literal anchors have also moved. The exact current sites and
   their existing single owners are below. Request: reconcile the strict expected
   owner set with the reviewed shop call and resolve anchors from function/
   excerpt identity rather than obsolete absolute lines. Keep declarations and
   runtime calls separately classified; deduplicate physical sites without
   discarding real writers or advancing execution/coverage status.

   | Current site | Existing owner |
   | --- | --- |
   | src/cmd/actobj.c:482 | item.command_publication |
   | src/cmd/actobj.c:761 | item.command_publication |
   | src/cmd/actobj.c:820 | item.pet_give_publication |
   | src/cmd/actobj.c:1393 | item.legacy_get |
   | src/cmd/actobj.c:6496 | item.legacy_give |
   | src/cmd/actobj.c:8126 | item.equipment_remove |
   | src/economy/shop.c:290 | shop.buy_produced |
   | src/world/handler.c:1740 | item.obj_to_char_admission |
   | src/world/handler.c:1913 | item.obj_to_char_admission |
   | src/world/handler.h:15 | macro.checked_item_publication_declaration |

2. `test_legacy_file_item_sites_separate_save_unload_restore_and_dead_code`:
   its two permitted shared terminal-unload anchors are hardcoded at 1764/1770;
   the current shared sites are 1769/1775. Both retain exactly the existing
   `player.flat_terminal_inventory_unload` and
   `player.sql_terminal_inventory_unload` owners. Request: resolve these exact
   function/excerpt sites and retain the strict two-owner exception, with one
   owner required for other sites. The native call sites and this test were
   already unchanged between 82a44c3 and this import; no regression is attributed
   to the three new native components.

3. `test_source_provenance_distinguishes_candidate_from_published_source`:
   `writers.json.candidate_worktree_evidence.status` and its matrix copy are
   `source_integrated_unqualified`. The contract accepts only
   `unpublished_candidate_worktree` and its older `dirty_candidate`,
   `published_base_commit` and `scanned_source_tree_sha256` fields. The current
   record instead carries `base_commit`, `scope` and exact `source_pins`.
   `generate_economy_writer_coverage.py` still chooses matrix `source_state`
   solely by the presence of a candidate record, yielding
   `published_base_with_unpublished_candidate_worktree` for integrated source.
   Request: explicitly reconcile these two known provenance shapes and derive
   source state from the actual status. Verify component pins for integrated
   source while retaining unqualified execution and incomplete release coverage;
   do not invent dirty/scanned fields or promote source review to executed proof.
   Consumers are the generator, registry/matrix metadata, operator route/source
   status and this provenance contract. No new schema is proposed independently.

The usual contract validator still passes:
`python3 scripts/validate_economy_accounting.py` reports 14 fixtures,
887 writer routes and 2,843 candidate sites, `release_ready=False`.
`python3 scripts/generate_economy_writer_coverage.py --check` passes with
`coverage_complete=False`, release `BLOCKED`. These successful consistency
checks do not waive the three executable contract failures or the native build.

## Evidence, skipped stages and remaining gates

All evidence is under `bin/tests/plan5-native-components-build-2026-10-05/`:
initial and continued build logs/command JSON, all successful object/dependency
outputs, include probe/source/objects, contract logs, `findings.json`, exact source
trees, import conflict/resolution snapshots, recorder/drivers and preservation.
The source/component/metadata description is regenerated from the frozen inputs
by `python3 tmp/plan5/describe-native-components-build-findings.py`.

The planned managed-input pin helper initially refused a tracked symlink.
Its source and failed preparation metadata are retained. The corrected helper
verifies all 6,107 regular Git files and records four link targets separately;
it creates no private checkout and executes no restore test. This pin inventory
is preparation evidence only, not database or runtime qualification.

Sealed manifest: `tmp/plan5/native-components-build-evidence.json`, SHA-256
`0122dde763510913dceba222e39b1f2e5746c9262f303a3b6342a230b3ab968c`.
It records 2,918 fresh artifacts. Preservation verifies all 1,496 native/migration
inputs (1,260 native and 236 migration files), all script/test inputs and 36,831
prior artifacts unchanged. The sealed evidence directories are not modified.
The owned reader/test SHA-256 values remain exactly the index slice's
`b64686ac080878871b85cccb5555ae292753cdc1058855c4fae328bea527a51b` and
`74a5aaa493d5451275e89616d8d0ff025916bc2af3272ee73566c6771339c325`.

Current-candidate managed dump/cold-import, flatfile recovery/retention and
isolated service boots are not run because both fresh maintained builds fail
and produce no server. Historical e018 build/restore and earlier v3 evidence
retain their exact historical scope. After primary publishes the native include
repair, fresh maintained builds and current-binary persistence checks are required.
The new borrowed SQL definition also needs primary-owned coherent0057/0058/0059
migrations, an admitted producer/custody/lifecycle caller and flat participant
parity before real runtime qualification. Original admission-time baseline CCM1
binding, full native capture, Plans 2–4 producer/gameplay journeys, workload and
retention/replica budgets, combined-candidate acceptance and R1–R8 remain open.

No shared repair, accounting activation, experimental-accounting push, PR merge,
deployment, production write or automatic correction of audit findings is made.
Inactive behavior, wallet-root item exclusions and the declined inactive spell
path are preserved. This report is the exact-input handoff to the primary's local
notebook curator; that workflow does not block further independent Plan 5 work.
