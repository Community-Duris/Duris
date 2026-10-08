# Independent SQL room-item custody diagnostics - 2026-10-08

Bounded operator-reader repair qualified on the published canonical64 source.
Full Plan5, original room recovery, retained-root authentication and R1-R8 release
qualification remain open. Accounting remains inactive.

## Branch, ownership and exact source

- Owned local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Owned base: `11e874facd842ad0cf89df04a1ba6c14b0028421`.
- Solved-issue code commit: `11d88805da5843eb463f5c12c1112ab0bcb3806c`.
- Documentation/publication result: exact SHA and verified remote are recorded in
  `D:/Dev/Tests/Duris/accounting-plan5/room-audit-20261008/delivery/result.json`.
- Refreshed/tested primary: `84a435c3deca43258c0c804ec16cc964197c2aea`.
- Tested primary plus 13 explicit owned overlays: tree `06d34b078ce2ee685ea134e4b855a97f3558cc71`.
- Frozen archive: `candidate04.tar`, SHA256 `e3dcaaf12ef296496f50837e3e1d87fcf3247a81b753d8f70a2505bef2306573`.
- Actual native source tree: `833d3085815b396861ad18a77635412212381e4b`.
- Actual migrations tree: `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; 64 canonical migrations,
  including incoming 0056 and later maintained migrations. Earlier 0055 results
  are not substituted for this composition.

`source04.json` binds every original source body, mode and link target and all
13 overlay Git blobs. The committed reader/test closure matches those overlay
blobs exactly. The owned branch has older shared source; qualification is of
this explicit composition, not an assertion that every branch file is primary.
The primary alone integrates and publishes its tested combined candidate.
All seven previously preserved branch tips remain required ancestors at delivery.
Earlier branch work and follow-ups continue on this same remote branch.

Exactly five owned code/test files change:

| File | Purpose |
| --- | --- |
| `scripts/economic_item_payload_audit.py` | Pure independent item decoder and modern room diagnostics |
| `scripts/economic_sql_audit_snapshot.py` | Bounded raw room capture and common coin decoder adapter |
| `scripts/reconcile_economy_accounting.py` | Invoke room diagnosis and refuse malformed/missing coverage |
| `tests/async/test_sql_room_item_custody_audit.py` | Pure controls, genuine native capture bytes and both canonical engines |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | Existing synthetic fixture column closure and new source-fence controls |

This report and the additive remote follow-up are the only documentation edits.
No shared coordinator, schema, producer, contract, registry/matrix, activation
owner, original native fixture or shared compile recipe changes.

## Defect, independent repair and acceptance boundary

The predecessor exporter captures a modern payload UID presence stub but omits
its bytes, original reference/ledger/receipt bindings and complete current graph.
On both canonical engines, truncating a real captured payload, appending a byte,
or deleting all modern payloads leaves the predecessor's entire `read_native`
result unchanged. The predecessor already refuses complete authority; this is
an established operator diagnostic omission, not a claimed release false-clear.
Exact predecessor bytes are retained in `predecessor.py` at the owned base.

The exporter now borrows the caller's existing RR read-only transaction and
captures all retained sidecars, all raw left-joined bindings, the raw season,
current roots selected by the maintained native classifier, and every current
UID row belonging to each selected root. It never filters away bad descendant
states or missing bindings. The classifier includes current-season sidecars and
the original successful typed player-drop fallback when all sidecars are absent.
Historical moved/prior-season payloads remain historical, with no invented
current missing-payload finding. Full snapshots also compare the selected graph
against their independently captured current UID census and exact positions.

The independent decoder consumes the original complete single-item byte format,
including all strings, values, timers, flags, dynamic rows, descriptions and
spell rows, with exact EOF. Ordinary room rules preserve non-money/non-corpse,
non-artifact literals, full string mask, root sentinel and equipment zero.
The existing coin consumer reuses this decoder while retaining its own UID,
VNUM, money type, denomination and original shared row checks. Wallet-root
ITEM_MONEY exclusions are unchanged.

Retained diagnostics check original operation IDs, UID/revision progression,
root/parent shape, child zero, player-to-room custody, reason, typed command and
success receipt. Current diagnostics separately check season/revision, owner
clock/custody/state/equipment, literal VNUM, current parent/root/room binding,
saved duplicates, complete reachability, cycles and depth. Historical bindings
do not require later custody to remain unchanged. A reproduced new-reader defect
allowed one valid proof plus a malformed duplicate: `cardinality-red/` retains
the failing source and test result. The final fix requires exactly one raw proof
row and exactly one valid binding, preserving malformed duplicates as findings.

Bounds are enforced before buffered LOB reads: at most 100,000 combined room
rows, 16MiB raw payload aggregate, and 131,072 bytes per item. Final JSON still
has its original 32MiB limit. Pure graph limits mirror maintained native limits:
4,096 roots, 3,000 items per graph, depth32, 143,072 aggregate graph bytes and
8,192 shared codec rows. The decoder keeps 4,096-byte string limits. Near-limit
100,000-row graphs are iterative; detail limits 0/1/100 retain full issue counts.
The whole SQL export now fences all 41 sources as InnoDB, adding the owner clock.

Every nonempty modern room/history packet explicitly emits
`room_item_full_runtime_authority_unqualified` and
`room_item_retained_root_authority_unqualified`. Storage-consistent synthetic
capsules never authenticate an original root or runtime publication. These tools
perform no repair, admission, activation or source mutation.

## Exact execution and results

Raw evidence: `D:/Dev/Tests/Duris/accounting-plan5/room-audit-20261008/`.
Helpers: `D:/Dev/Temp/accounting-plan5-room-audit/`.
New writable build outputs:
`D:/Dev/Builds/Duris/accounting-plan5-room-audit-20261008/`.
The network-none Docker image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Sources and private POSIX scratch use RAM `/workspace` and `/tmp`; output/evidence
directories bind D: directly. Each attempt has its own unchanged source archive,
build directory, logs and stopped container. No terminal run was restarted.

Final commands, from the owned worktree:

```text
python -B D:/Dev/Temp/accounting-plan5-room-audit/freeze.py 04
python -B D:/Dev/Temp/accounting-plan5-room-audit/launch.py checks04 source04.json
python -B D:/Dev/Temp/accounting-plan5-room-audit/commit_code.py
python -B D:/Dev/Temp/accounting-plan5-room-audit/seal.py
```

`checks04/docker-command.json` records the exact Docker invocation. Its pinned
entry point is `python3 -u -B /evidence/checks04/checks.py checks04 source04.json`.
That unchanged recorder loads each named unittest module with
`unittest.defaultTestLoader.loadTestsFromName` and executes its complete suite.
It then runs `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
once per fresh private engine. `checks04/commands.json` retains all 795 nested
commands with raw inputs/outputs and actual return codes.

| Final stage | Exact scope | Result |
| --- | --- | --- |
| `test_sql_room_item_custody_audit` | Six pure methods plus both-engine native-byte SQL method | 7 PASS, zero SKIP; 52.101s |
| `test_economic_sql_audit_origins` | Complete unchanged module | 55 PASS, 3 existing opt-in SKIP |
| `test_reconcile_economy_accounting` | Complete unchanged module | 131 PASS, 3 existing opt-in SKIP; 64.909s |
| `test_sql_saved_ground_custody_audit` | Complete unchanged module | 10 PASS, 1 existing opt-in SKIP |
| Whole snapshot exporter, MariaDB | Existing full synthetic SQL/operator suite plus all38 missing/MyISAM source controls | exit0 |
| Whole snapshot exporter, MySQL | Same complete suite and38 source controls | exit0 |

Final composite: exit0 in 266.979665579s under the original declared
1,200-second batch/600-second whole-runner budgets; 203 passing tests and seven
explicit skips. Four actual private database services shut down with exit0;
both MySQL version checks and both initialization processes also exit0. Frozen
source bodies, modes and link targets pass before/after guards. These are not
new compiler or full-server builds of the primary's private candidate.

Backends are MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`. Both room schemas are fresh bootstrap plus the actual
canonical migration runner/adoption and all64 migrations, with real constraints.
No migration/check is removed to enable a negative control. The whole existing
exporter suite retains its explicitly synthetic manual schema and is not
misrepresented as a second canonical migration/upgrade journey.

The native-byte fixture extracts unchanged `item` and `command` construction
helpers from `tests/async/sql_room_item_payload_test.cpp`, compiles the actual
`src/persistence/sql_room_item_payload.c` and
`src/player/player_snapshot_codec.c`, and calls the actual capture helper. Three
348-byte nested-item payloads retain exact literal/dynamic/description data.
Its harness adds only output and static assertions for the unchanged limits;
no original native assertion, recipe or test is edited. Exact compile command:

```text
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -D__NO_MYSQL__ -fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -ffunction-sections -fdata-sections -Wl,--gc-sections -Isrc /workspace/bin/tests/sql-room-item-audit/native.cpp src/persistence/sql_room_item_payload.c src/player/player_snapshot_codec.c -o /workspace/bin/tests/sql-room-item-audit/native
```

ASan leak detection/halt-on-error and UBSan halt/stack traces are enabled.
GCC13.3.0/Python3.12.3 and exact tool executable hashes are retained in
`seal/native-build-inventory.json`. Native executable SHA256:
`dab374df95e7d9685d5cd3e6fc8271caef5accc6b2a43a3567c0e4c604445154`.
All three payload hashes, full derived C++ source and actual executable remain
under `checks04/bin/tests/sql-room-item-audit/` in the D: build directory.

`evidence.json` there retains34 both-engine observations:30 read-only provider
cuts and four canonical-FK-refused UPDATE attempts. Each provider cut uses a
SELECT-only account, checks all tables unchanged before/after, and compares
complete counts at detail limits0/1/100. Controls cover truncation/trailing bytes,
stale current revision, failed receipt, foreign/quarantined descendants, parent
cycle, zero owner clock, saved duplicates, complete sidecar loss, moved/prior
season history, late-writer RR isolation and restored baseline. Two invalid
reference updates per engine are refused by canonical composite FKs; they are
not reported as reader-detected corruption. Pure packet controls diagnose each
raw binding field, absent/malformed/duplicate proofs and exact current census.
An aggregate129 maximum-size payload insertion is refused before audit LOB reads;
the SELECT-only account's attempted UPDATE is denied. The root capsules are
deliberately synthetic; this is not a genuine publication/ACK or recovery seed.

## Failed attempts and evidence integrity

| Attempt | Terminal result | Retained cause |
| --- | --- | --- |
| `checks` | exit1, 29.272463253s, 71 commands | New fixture omitted mandatory reference `line_index`; canonical INSERT refused |
| `checks02` | exit1, 29.119047995s, 71 commands | Canonical composite FK refused the proposed damaged reference UPDATE |
| `checks03` | container exit1 | Individual suites and both whole exporters pass; terminal JSON recorder fails serializing skipped unittest objects |
| `cardinality-red` | expected test exit1 | One good plus one malformed duplicate escaped binding diagnosis |
| `checks04` | exit0 | Final fixture, reader and serializable recorder; all selected checks above pass |

The fixture now supplies the actual required column and asserts canonical
reference refusal without disabling constraints. The recorder now serializes
test names/reasons. `checks03` has no valid terminal JSON and is not relabelled
a passing batch. Raw console logs, source archives and stopped database copies
remain separate. The earlier room observer/seed result and its unresolved
link failure also remain unchanged in the preceding report.

Raw seal: `seal/evidence.json`, SHA256 `5528e7983ed3009da2d6f419d88b5c389fbdb98a074c710aa4c164f92583411f`.
It binds7,227 regular files/3,304,746,568 bytes, including1,198 native build files,
raw attempts, copies, source exports and all stopped container observations.
No data links/reparse points are followed or copied; original POSIX metadata
inventories remain. Final delivery rechecks every sealed body, committed overlay,
remote SHA, clean worktree and seven preserved ancestors.

## Narrow primary handoffs and remaining gates

No shared schema or gameplay interface change is required for this repair.
Primary registry/matrix owner should register the new owned test path and its
`DURIS_RUN_SQL_ROOM_AUDIT=1` canonical-both-engine requirement, preserving every
old row/budget and distinguishing six pure methods from the enabled seven-method,
zero-skip qualification. The external qualification also sets
`DURIS_ROOM_AUDIT_PREDECESSOR=/evidence/predecessor.py` to the pinned old exporter.
Its source fixtures require the two maintained native codec providers and exact
limits above; pure inventory cannot replace native/database acceptance.

The original shared room-seed compile closure request remains in
[the preceding exact handoff](PLAN5_ROOM_OBSERVER_BUILD_AND_SEED_HANDOFF_2026-10-08.md):
41 unresolved functions mapped to21 real maintained providers. The primary owns
that repair and original both-engine/full-world/two-cold-boot journeys. The real
observer server already builds on native833, but no qualified original room seed
or cold journey is produced here. This slice uses actual capture-helper bytes
with synthetic storage roots and does not bypass that blocker.

All seven existing skips remain explicit in `checks04/terminal.json`: three native
origin opt-ins, two mapping/price budget opt-ins, native stake SQL and the saved
native SQL opt-in. Their prior evidence is not a fresh rerun in this slice.
No production C/C++ is edited; full Make is not rerun for this Python-only fix.

Primary published84a435c3 documents private115 warm-forest/SHOP prerequisites,
candidate c833d941d8f57c580b3d2fc37c1015bef97a08d114cc418a3984e7bc5333aeff,
with compiler/native/SQL/recovery still UNEXECUTED. Full raw/current keeper cash
disagreement must remain until its positive classification/current-image and
historical wallet/treasury owner is complete; no VNUM/tag inference or suppressed
finding is introduced. No private implementation is imported or qualified.

Complete runtime/current-world/collector/coin/prototype/UID/origin/writer census,
full retained-root authentication, callback installation/recapture, active-erasure
and backup/retention continuity, populated upgrades/reruns, original Plans1-4
and native/gameplay/fault/load/restart qualification, and the primary's published
tested combined candidate remain required. Full Plan5/R1-R8 remains unproven.
No activation, deployment, PR merge, production access/data or audit autocorrection.
Inactive behavior, wallet-root exclusion and declined inactive spell path stay.

This report, raw seal, additive remote follow-up and post-push delivery receipt
form the curator-ready packet for the primary's locally maintained notebook.
That notebook is nonblocking; curator application/acknowledgement and primary
adoption are not claimed. This is progress; the full goal stays active.
