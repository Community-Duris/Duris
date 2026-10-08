# Plan 5 independent previous-owner history — 2026-10-08

The independent reconciler could return zero exceptions for a broken custody
history even after verifying the retained original plan. Changing only the
opening owner from `[2,8,0]` to `[1,8,0]` left that clean result intact while the
next event still recorded previous owner `[2,8,0]`. The unchanged-reader
reproduction is retained. This slice closes that false-clean path, retains the
existing native previous-owner columns in SQL lineage/unattributed exports, and
makes missing historical evidence explicit. It does not establish release
completion, complete opening, genuine producer coverage or primary integration.

## Branch, files and narrow interface handoff

Sole local/remote branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `3a055bb6b1884fa8f1e469d5746dd65ff03202bb`. The containing commit is the solved issue's result;
post-push `delivery/result.json` records its exact SHA and the verified remote.
All seven previously consolidated branch tips and follow-ups remain ancestors.
No worktree/branch switch, rewrite or primary-branch push occurs.

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `scripts/economic_sql_audit_snapshot.py`
- `tests/async/test_reconcile_economy_accounting.py`
- `tests/async/test_economic_sql_uid_scope.py`
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`
- This report and additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.

No shared C source, coordinator, producer, writer registry/matrix, activation,
schema or original shared native recipe changes. No new native interface or
schema is requested. The owned JSON interface keeps the existing `from_owner`
field as `[kind,id,context]`, now also on lineage and unattributed events. It
comes only from existing `item_ownership_ledger.from_owner_type`, `from_owner_id`
and `from_owner_context_id`. Kind must be an exact integer 0..12, ID/context
unsigned 64-bit exact integers; booleans, partial NULLs and malformed values
refuse without private values in errors. Three NULL/omitted SQL columns preserve
legacy unknown by omitting the JSON field. A present malformed JSON tuple
refuses. Older snapshots with an absent field remain readable and explicitly
unverified. The consumer is the independent reconciler; raw retained snapshot
history also preserves the complete tuple for subsequent inspection. Existing
operator projections do not acquire private strings or mutation authority.

For each anchored UID, every known previous owner must equal the preceding
origin/event owner. A mismatch emits `broken_item_owner_history` with UID and
operation ID; omitted history emits `missing_item_owner_evidence` once per UID.
Subsequent comparisons use each recorded resulting owner, avoiding invented
repairs or cascades from a single disagreement. Native first-creation previous
owner `[7,0,0]` is checked explicitly against the model's absent creation origin;
model `[0,0,0]` is not substituted for that native sentinel. Unattributed rows
retain their coverage findings; no origin is inferred for them. Tests exercise
kind, ID and context independently, creation, multistep history, legacy omission,
malformation, lineage and selected paths, bounded global findings and read-only
behavior. Primary import/application/acknowledgement is not claimed.

## Exact tested source

| Source | Identity |
| --- | --- |
| Refreshed published primary for final snapshot checks | `ab8153878563a9e5d5c44f273423cf0290b5d449` |
| Published primary plus 22 owned overlays | `03e3ddb6ed6c7496ea95a49fc1ed3edc4575d960` |
| Archive SHA256 | `3d21d7bc9d8ce29e85986a501c06a73dc9ca9e4bea2e5138374b699640ce914b` |
| Published primary for original canonical regression | `257190ac149a86af59b1c3c2fe321abb382cd8ef` |
| Canonical composition | `d11bf5cac99e60a373ea501e9a38b947b503d883` |
| Canonical archive SHA256 | `7537f406bc81a8cfc492c8f118d65815f4b0ab0443a65fc87f3e778cc2f63f82` |
| Native tree, both compositions | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree, both compositions | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Schema head | 64 / 0064_auction_custody_history |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

6512 Git blobs, 6508 regular bodies and four exact link targets authenticate each
of three immutable archives. Before/after guards check all bodies, modes and
links. All 22 final overlay blobs equal the worktree and are rechecked against
the published result. `source01-to03-binding.json` authenticates the two
compositions: only primary docs, the separately executed manual snapshot runner
and cosmetic UID-unit-test indentation differ. The complete original canonical
regression invokes neither changed test. Its native sources, schemas, original
recipes, owned coin fixture, restore runner and both audit readers are identical.
It is reused only for that exact consumed source boundary. Required Plan 5,
finish plan, requirements and checkpoint are retained as raw bytes for the tested
primary; published AI_CONTEXT.md remains absent. Shared source in this worktree
is an older tree and is not used as the native test base.

The refreshed checkpoint's private 140-production-file birth/publication
candidate `a41d13a7a78b0eb7e4ff8e34281d8050fbad2c112433ddaa24a7ac404d0527b4`
is source-reviewed and unqualified. Its compiler/native/SQL/gameplay/persistence/
recovery work remains UNEXECUTED. This report tests the published source and
makes no inference about that private candidate.

## Commands, results and retained failures

Host commands execute only in the stated owned worktree, with D: temporary,
build and evidence storage:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-history/reproduce.py
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-history/freeze.py 03
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-history/authenticate03.py
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-history/launch.py checks03 03
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-history/launch.py canonical01 01
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-history/qualify.py
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-history/seal.py
```

The pinned tools containers have no network/ports, fresh private RAM source/tmp,
D: build binds, 2 CPUs and 5 GiB memory. Snapshot checks use a read-only root.
The unchanged canonical test requires its original `TemporaryDirectory(dir='/')`
layout, so that separate disposable container has a writable root. Neither stage
adds capabilities or changes original compiler/runner arguments, fixtures,
assertions or original per-command timeout bounds. The external observer records
original calls and preserves native lstat metadata before selected regular-only
copies. No physical database bodies, private keys or service runtime trees are
copied to Windows. Production data and existing services/volumes are untouched.

| Check | Result |
| --- | --- |
| Corrected RED against unchanged reader | 16 assertion failures, zero test errors; actual defect reproduced |
| Reconciler module, including both original budget methods | 140 loaded, 139 PASS, one original opt-in native stake skip |
| SQL UID scope module | 13 PASS, zero skips |
| SQL origin reader module | 58 loaded, 55 PASS, three original opt-in native SQL skips |
| Complete manual snapshot runner | PASS on both actual private engines, all original controls retained |
| New actual SQL previous-owner controls, per engine | Five captures, three owner disagreements, one legacy unknown, one partial-NULL refusal; 30 CLI checks |
| Complete original canonical coin/restore regression | One test PASS, zero failures/errors/skips; SQL and client-free native modes, both canonical engines |
| Final snapshot stage | 143.466220s; 676 recorded run commands, 678 launches |
| Original canonical stage | 528.701892s; 62 recorded run commands, 64 launches |

Actual engine versions: MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 and MySQL
8.0.46-0ubuntu0.22.04.4. The whole manual runner calls existing capture logic
with a SELECT-only user, checks rollback/close and unchanged authority, then
runs original source grammar (27 corrupt cuts), source policy (66 mismatched
kinds), orphan/interrupted cuts, uint64 values/revisions, provenance, compound
creation/retirement, quarantine, supply outcomes and all operator views. Setup
mutations exist only in each fresh disposable modeled database. The new controls
restore their original NULLs and re-capture the unchanged baseline. They do not
claim native producer invocation, complete opening or active accounting.

The six new pure tests include 21 real CLI runs across all seven views and
limits 0/1/100. Both original synthetic near-32-MiB audit budgets execute all
12 real CLI workloads. Maximum elapsed 1.106419s,
maximum peak 157,270,016 bytes; every input hash stays
unchanged and totals are invariant under detail limits. These measure component
budgets on this host, not real-root release throughput or release-host limits.

Original canonical strict native recipes and ASan/UBSan runs remain exact. Both
modes yield byte-identical coin/claim frames; 3026 native decoder cases agree
with the independent Python decoder, 1054 accepted. Each SQL engine passes
109 canonical cuts, including 58 full-entry cuts and 90 expected refusals;
25 partial-claim cuts, 30 audited coin cases and two canonical constraint
refusals. All retained authority remains unchanged. Genuine native capsule
encoding and modeled SQL/history projections are distinct from authentic
financial producers, native full opening and real player journeys.

The original initial RED log had a test method-name error, corrected before the
16-failure RED. The initial full host pure run then exposed five healthy test
models that omitted owner fields; those helpers now declare their known modeled
previous owner, keeping original assertions. Native `checks01` and `checks02`
each terminate exit 1, preserved separately: the craft-creation phase resolves
the baseline's legacy omission, while the uint64 revision case adds a second
unknown item. Exact expected missing-owner counts now reflect those actual
fixtures. Neither failed stage is restarted/relabelled or used as a green result;
MySQL was not reached in those failed attempts. Complete `checks03` runs both
engines and all original controls. Exact Docker/compiler/client/daemon/test argv,
timeouts, exits, logs, source guards and temporary-copy metadata are retained.

## Evidence, curator workflow and remaining gates

Raw evidence `D:/Dev/Tests/Duris/accounting-plan5/owner-history-20261008`;
builds `D:/Dev/Builds/Duris/accounting-plan5-owner-history-20261008`;
helpers `D:/Dev/Temp/accounting-plan5-owner-history`. Key records are
`unchanged-reader-reproduction.json`, corrected RED, each failed terminal/log,
`source03.json`, transport authentication, source01-to03 binding,
`checks03/terminal.json`, both whole-snapshot logs,
`canonical01/terminal.json`, `qualification.json` and `seal/evidence.json`.
Native regular bodies are authenticated before Windows read-only rehashing.

Seal SHA256 `88cb0537702429817063497b04e57c05eaf0e438bb7ed0f60861abaca07635e9`: 1230 retained regular files,
1,165,525,462 bytes, 945 native build files;
zero copied links/reparse points. All five stage/seal containers are stopped,
not OOM-killed; terminal recorded child-process sets are empty. Post-push
`delivery/result.json` rehashes every sealed entry and binds the result SHA,
remote, all 22 tested overlays and seven preserved tips. It is delivery evidence,
not a test rerun.

This report plus the additive remote follow-up are curator-ready notebook input.
The primary's locally maintained shared notebook is nonblocking; its curator
must reconcile these source/evidence boundaries and still-open gates. No notebook
application, coordinator adoption or acknowledgement is claimed.

No shared-provider handoff is newly introduced. Prior genuine shared recipe
blockers remain: original flat custody/auction codec-publication providers,
room-seed UID owner/transitive provider collision, and original native SQL
baseline's missing four providers/head64 recipe. They do not block this owned
reader fix, and they still block their full original native qualification paths.
The complete 12-case managed backup module, full opening/source admission,
authentic producers, real player/persistence/recovery journeys, fault/load and
route coverage, current private combined candidate, activation and Plan 5/R1–R8
remain unqualified. No tracked C changes occur, so no new full-server Make or
service boot is claimed here. Existing inactive behavior, wallet-root money-item
exclusions and the declined inactive spell change are preserved. No activation,
autocorrection, production write, deployment, PR merge or primary-branch push.
