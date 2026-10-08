# Plan 5 previous-owner dependent test follow-up — 2026-10-08

The previous-owner reader fix 8c1c77f646ada5aca6b703218cb8f76b4bfae62c passed
its reported focused/native/SQL checks, but those focused checks omitted two
existing dependent test expectations. This follow-up corrects those expectations
without weakening reader behavior or changing production code. The complete
16-module reader consumer run now passes: 383 loaded, 365 PASS, 18 explicit
original opt-in skips. This is component evidence, not Plan5/R1–R8 completion.

## Branch, change and exact source

Sole branch and remote `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `8c1c77f646ada5aca6b703218cb8f76b4bfae62c`. The containing commit is the result;
`delivery/result.json` binds its exact SHA and verified remote. All seven earlier
tips and their follow-ups remain ancestors. No branch switch or primary push.

Owned files are `tests/async/test_item_equipment_reconciliation.py`,
`tests/async/test_plan5_child_identity.py`, this report and additive
`PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`. No production reader, native source, schema,
coordinator, contracts, producers, registry/matrix, activation or original native
recipe changes. No shared interface request. The child test file's AST is exact
except its one pure test method; native probe, helpers, imports and recipes stay.

The equipment history model always represented a live player-owned item moving
between slots. It now explicitly declares its known `[1,7,0]` previous owner at
each step. Original equipment assertions remain unchanged. A new test removes
those fields and proves one missing-owner finding per UID on selected and lineage
paths, input immutability, and separate simultaneous missing-equipment findings.
No owner is invented in a real export. In the existing child projection test,
valid integer custody disagreements still require the original mismatch finding;
Boolean previous-owner context now requires the documented malformed-tuple refusal
at all three detail limits, preserving the source snapshot.

| Tested source | Identity |
| --- | --- |
| Refreshed published primary | `ab8153878563a9e5d5c44f273423cf0290b5d449` |
| Primary plus 24 owned overlays | `a470a00486c7eabc2baf28654c3c0f0adf790493` |
| Archive SHA256 | `8643d1b0b2b5de282637cad48cb562f150f0fd7e73866831e7526cebbdde713a` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Canonical schema boundary | 64 / 0064_auction_custody_history |
| Tools image | sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a |

Both immutable source archives authenticate 6512 Git blobs, 6508 regular bodies,
four exact link targets/modes. Before/after guards preserve every body/mode/link.
Required Plan5, finish plan, requirements and checkpoint are retained raw; published
AI_CONTEXT.md remains absent. The primary's source-reviewed private 140-file
candidate a41d13a7 remains unexecuted and unqualified, outside this source.

## Reproduction and execution

The unchanged equipment module loads seven cases and fails 27 subtest/assertion
checks because its healthy history omitted previous owners. `equipment-red.log`,
`red-command.json` and the exact preimage are retained. The first broader Linux
stage `checks01` completes the equipment fix but terminates exit 1 at the child
module's Boolean expectation: 23 child cases, one error, two original skips.
That failure is preserved; later modules were not reached in that attempt.

Fresh `checks02` executes all 16 modules under standard unittest without replacing
assertions, source, compiler flags or subprocess arguments. It takes
112.710602s, records 811 run commands/812 launches,
and terminates exit 0 with all recorded processes stopped and source unchanged.
No C compiler, database daemon, service boot or full-server Make is invoked in
this test-only follow-up. The Linux container has no network/ports/capabilities,
a read-only root, private RAM source/tmp and direct D: build/evidence binds.

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-dependent/red.py
python -X utf8 tests/async/test_item_equipment_reconciliation.py
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-dependent/freeze.py 02
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-dependent/authenticate02.py
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-dependent/launch.py checks02 02
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-dependent/qualify.py
python -X utf8 D:/Dev/Temp/accounting-plan5-owner-dependent/seal.py
```

| Module | PASS | Original skips |
| --- | --- | --- |
| test_reconcile_economy_accounting | 139 | 1 |
| test_economic_sql_uid_scope | 13 | 0 |
| test_economic_sql_audit_origins | 55 | 3 |
| test_item_equipment_reconciliation | 8 | 0 |
| test_economic_sql_canonical_audit | 63 | 4 |
| test_plan5_child_identity | 21 | 2 |
| test_ship_coffer_audit | 5 | 0 |
| test_guild_treasury_audit | 5 | 0 |
| test_sql_auction_custody_audit | 6 | 1 |
| test_sql_shop_custody_audit | 6 | 1 |
| test_sql_player_custody_audit | 7 | 1 |
| test_sql_corpse_custody_audit | 7 | 1 |
| test_sql_locker_custody_audit | 7 | 1 |
| test_sql_siege_custody_audit | 7 | 1 |
| test_sql_saved_ground_custody_audit | 10 | 1 |
| test_sql_room_item_custody_audit | 6 | 1 |

The existing mapping and price near-32-MiB budgets execute all 12 actual CLI
workloads. They are synthetic component budgets, not real-root throughput or
release-host qualification. The complete skip identities/reasons are retained in
qualification.json: stake (1), origins (3), canonical SQL (4), child budget/native
(2), and each of eight native SQL custody families (8). No skip is relabelled green.

The prior complete manual SQL snapshot and canonical coin/restore results remain
reused only for their unchanged consumed inputs, explicitly bound in
previous-source02-binding.json. Source comparison finds exactly these two test
files changed. All production/native/migration/runner inputs are identical. The
child module's AST is exact except the uninvoked test method; its imported
integer_snapshot/NATIVE_PROBE/child helpers remain exact. The equipment module is
not invoked by the original SQL/canonical paths. This follow-up neither reruns
those databases nor claims new native/SQL proof. Their earlier MariaDB10.11.14/
MySQL8.0.46, canonical64 results and limits remain recorded in the previous-owner
report/evidence. Delivery rehashes that earlier raw seal as well as this one.

The first binding verifier incorrectly treated importing ChildIdentityTests's
unchanged integer_snapshot helper as invoking the changed test. Its retained
failure is corrected by checking the actual test method and complete remaining
AST. No source or native result is changed to satisfy that verifier.

## Evidence, curator and gates

Evidence `D:/Dev/Tests/Duris/accounting-plan5/owner-dependent-20261008`;
helpers `D:/Dev/Temp/accounting-plan5-owner-dependent`;
builds `D:/Dev/Builds/Duris/accounting-plan5-owner-dependent-20261008`.
Seal SHA256 `8b546ef34bd63be231654b48c78e84b03bf368692ed8545610c9d5b01b46ddc5` covers 269 regular files,
731,587,541 bytes, two native-inventoried build .gitignore bodies and zero
copied links/reparse points. Both test containers and the read-only seal container
are stopped/not OOM-killed. Source guards pass; no recorded live processes.
Exact argv, timeouts, exits, logs, source transport and native metadata are kept.
Post-push delivery verifies all 24 overlays, expected four owned files, all seven
tips, remote/clean worktree and both raw evidence seals.

This report and additive remote follow-up are curator-ready input. The primary's
local notebook is nonblocking; application/acknowledgement is not claimed. Prior
shared flat codec/publication, room-seed UID/transitive-provider and native SQL
baseline recipe/head64 blockers remain. The skipped native paths, whole 12-case
managed backup module, authentic producers/full opening, real player and recovery
journeys, fault/load/coverage, private combined candidate and full Plan5/R1–R8
remain unqualified. No accounting activation, audit autocorrection, production
write, deployment or PR merge. Inactive behavior, wallet-root item exclusions
and the declined inactive spell change remain exact.
