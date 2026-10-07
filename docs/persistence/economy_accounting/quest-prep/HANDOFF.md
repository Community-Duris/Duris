# Quest implementation and executed evidence handoff — 2026-10-07

Both independently owned production fixes and the supported execution pack are
committed and pushed to `origin/codex/accounting-quest-prep`. This delivers the
implementation/execution assignment; complete active accounting and release
qualification remain with the integrated primary candidate.

## Import order and exact commits

| Bundle | Result SHA | Base and authored scope |
|---|---|---|
| QP07 original-attempt callback protection | `fd997ee4147ba58d835bf4bd61783b51307bc68c` | Parent `dd4aefe901d71fc64e3c9fff7d5e1ac3a741bcf6`; production `src/specs/specs.world_quest.c`; owned callback regression/pins/handoff |
| QP02 native recipe reachability | `a19a67ad021de8e6bbfb31ca9ffea31e41cb6aa7` | Parent fd997ee; production `src/world/quest.c:quest_native_completion_owner::prepare_original`; owned selector regression/handoff |
| Genuine SQL capture and executable journeys | `cab408e566ff7f1357c1fc783be7a095c75f4e78` | Parent a19a67a; owned fixtures/reader/oracle/runners, two additive quest drivers, execution docs |
| Current accounting compatibility merge | `6db65f624836f150ed3dfe33508e1f1719145fdb` | Parent cab408e5 plus accounting `f04317d9d72aa5594448809baad6041936b09801`; upstream history preserved, no authored shared production changes |
| Refreshed seven-case pins/diagnoses | `88fc052630932492ffff148b474f075f72423fb9` | Parent6db; owned case/source facts and exact invisible-recipient response; docs |
| Successful bartender transcript/fee evidence | `429acc4a695db65997e0f2dcfcb4727b3dc407a8` | Parent88fc; eight additive Python lines in owned world runner/shared world driver; verified on both current backends |

Earlier accounting pin `aa252cd8134972548a953a0c5e500da2f8d06af7`; original base
`17c033d69316b21da8598791fc95cae79baa8dc2`; PR678 research
`55905eac1906cf59405764407f9d22497cccfff3`. Latest fetch is f04317d9d. Its production
delta from aa252cd8 adds native-load pool reservations and exact coin endpoint
recovery counters; both quest fixes stay byte-exact. SOURCE_FACTS.json binds all
seven cases to f04317d9d plus actual production candidate6db65f6. Later tool/docs
commits do not alter that binary's production source. No force push.

## Fixed and meaningfully verified

QP07 captures persisted quest_started at request time and checks it before
map/abandon/history effects. Context12→16 bytes fits actual64-byte pending bound.
Existing create/share strictly advance the watermark for same-second/clock
rollback replacement; reset preserves it. Actual callback regressions pass for
original task, same/different target replacement, completed/reset task, repeated
callback, rejected payment and invalid contexts. Generic active credit refusal
remains; stale-effect protection does not prove restitution. Specs SHA256:
`4d102b8ec12497bc0ce9c0c8bf170932c99495911f6a12d8871514f88b2563f4`.

QP02 runs original independent duplicate ITEM/TYPE availability checks before
unsupported paid-operation refusal. Unavailable earlier branches return
not_matched; genuinely matching paid branches still refuse before root selection.
Original loader/dispatcher/recipe and partial-prefix overlap order stay intact.
Actual-owner regressions pass for reachable three-hide backpack, four-hide paid
gloves refusal, no branch, multiple matches, coin-only, duplicate/short/spare/
wrong-kind stock, overlap and repeated evaluation without mutation. All285 paid
ITEM/TYPE recipes from production areas/AREA pass the current component check.
Quest SHA256 `747a43b7f27ff6a3d53cc7776097b9c361d7c6437e6d04bbe5da717895677f8c`.
Before fixes the original QP07 component31 and QP02 component30 failures reproduced.
Current QP03 generation/root guard passes; QP04 financial component30 stays RED.

## Current candidate builds, schema and terminal results

Both original strict maintained `make -C src -j2` profiles compile all740 providers
and link successfully on candidate6db65f6; no compiler/sanitizer flags or journey
deadlines were weakened. Docker image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`,
Ubuntu24.04/GCC13.3, private network-none runtime; actual MariaDB10.11.14.
Source archive SHA256 `9f95168377ec1fe39bec1dad0ad972555e6a3625a6ed2721e58a750c8c375d85`.
SQL ELF `0d6514c7222dca25cb30e9e5875036cf2088461ebb07731a66153be07683907b`;
flat ELF `8aedc856f2ff9cfee15ce694b16d28540daba1a5f41dedd40d59de91ad1b1ea5`.
Canonical0062 schema manifest
`1fddd009cc67bba940efba28c2afc3d16d65ea8a476410034cd77c90a0a9aea2`.

| Current execution | Terminal result and precise scope |
|---|---|
| SQL QP06 XP-ACK + later reward drop + second cold | PASS,152.032s; three exact Kord roots,29237 reward UID820,C3000,frozen200/effective520 XP once,slot2 mask4,ACK,drop to room22800/revision2,exact UID/custody after second cold |
| Flat QP06 XP-ACK + later drop | RED,135.663s; recovery passes original assertions; drop refuses error116/stale_authority_revision. Retained reward UID821 remains player-owned,XP521,C3000,player owner revision8/room7 |
| Flat QP01 shortage | PASS,51.784s; three genuine statues, actual incomplete-offering response, original UIDs unchanged through cold load |
| Flat QP02 three hides | PASS refusal,68.361s; legacy durable dispatcher refuses paid recipe, original UIDs unchanged through cold load; native reachability is separately component-qualified |
| Flat QP03 invisible recipient | PASS refusal,67.786s; real `give sword dragon` returns exact `No one by that name around here.`, all three original UIDs retained through cold load; no D/birth claim |
| SQL QP05 white-bear reward | PASS,113.173s; actual three skins/give,16048 reward UID819 preserved through cold load; captured custody/ledger/replay assertions pass |
| SQL and flat full-world bartender | PASS,90.523s/35.597s; genuine creation/queued abandon+replacement/final abandon/clean shutdown. Actual Woodseer16553/room16633/level56; both current selections mapless (no map issuance/debit claim). Actual quoted creation1120 and abandonment43464 copper retained |

The actual SELECT-only reader now executes every current query in genuine
legacy activity, in a read-only repeatable-read snapshot (transaction1 then0).
It captures62 migration history rows, exact custody/ledger/continuation/receipts,
checks missing-player and row/BLOB refusal controls, and confirms schema cleanup.
Native default assertions refuse these epoch-free cuts. Original failing
`sql-qp06-xpslot-offering` is retained: maintained recovery passed, but owned
oracle incorrectly expected XP0 from absent native entitlement rows. Corrected
reader calls the maintained v5 decoder, observes actual human/level1/well-rested
2108 and pinned properties, requires frozen200/slot2 mask4, then asserts520 once.
Corrected a19a67a offering run passed153.865s; current lost-XP-ACK/move run also passes.
No modeled rows, epoch, birth, UID, receipt or successful ACK substitute is inserted.
All three current disposable SQL schemas and the final owned-schema census are0.

## Executable files and evidence

Owned test changes: case_data.py, prepare_fixture.py, capture_quest_cut.py,
quest_cut_checks.py, test_production_terms.py, test_quest_cut_checks.py;
new run_quest_execution.py, run_static_execution.py, run_world_execution.py,
legacy_xp.py and read_quest_continuation.cpp. Production fix regressions are
owned test_native_selectors.py and test_bartender_settlement.py.

Additive shared drivers: run_quest_reward_ack_crash.py offers optional observer/
retention/SQL callback and real Kord search; its mask follows original head-linked
reward order (synthetic1 unchanged, Kord4). run_world_quest_dual_backend.py offers
optional map request/transcript/retention. Original defaults, assertions, deadlines
and backend coverage remain. Sorted mob/object fixture indices fix actual binary
lookup while retaining complete production records and quest order. Flat inspector
link augmentation stays in the owned invocation, not the shared manifest.

Exact commands, prior binary pins, retained setup failures and current results:
[EXECUTED_JOURNEYS.md](EXECUTED_JOURNEYS.md). Private evidence lives under
`bin/tests/quest-implementation-20261007/` in this worktree; nothing private,
generated, credential-bearing or compiled is committed. Current component pack
passes seven source cases/30 variants/nine contracts/seven world layouts,285 paid
recipes, QP03 guard/QP07 callback,16 oracle controls and three existing world tests.
The major-candidate cadence is preserved. Owned runtime/build containers are
stopped after evidence/cleanup verification; other agents' resources are untouched.

## Remaining shared dependencies

Flat later drop needs the actual shared movement/recovery owner: actobj.c
item_drop_completion → item_movement_transaction → flatfile_item_repository.
ESTALE is observed; its precise internal prerequisite is not inferred. Do not
rewrite the journal or alter counters to make the case pass. The primary retains
that fix, pending-journal qualification, activation, schemas and central manifests.

Active native three-item handover, exact original NPC birth/source custody,
whole-stock/cash retirement, paired original ACK, move-before-lost-ACK and durable
stale-service restitution/held obligations still need genuine integrated authority.
Legacy QP02 stops in submit_durable_quest_offering at an unsupported paid recipe;
that owner differs from corrected active_regular_sql native completion. QP01
exact mini success is blocked by the real secret Orb44164 FOUND teleport special;
do not clear flags or suppress it. Quietus1709/1734/level11 source specifications
remain distinct from the genuine Woodseer calibration (Quietus creation220/map110;
Woodseer creation1120/map560 when offered; abandon follows real age/level formula).
The shared flat inspector manifest needs its existing baseline codec/adapter link
inputs; owned runners preserve the original manifest and add them locally.

Earlier prep history remains; primary previously consumed through3e9ce549a0a14c6414242aae231b69da64b10cce.
Do not duplicate consumed bundles. Final native/accounting qualification must use
primary's integrated candidate at its agreed batch boundary; no release gate is closed here.
