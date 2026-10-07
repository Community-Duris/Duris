# Quest implementation and execution handoff — 2026-10-07

Production corrections are published on `origin/codex/accounting-quest-prep`.
This active implementation goal is distinct from the completed prep pack below.
Import the following new bundles after the previously consumed prep commits:

1. `fd997ee4147ba58d835bf4bd61783b51307bc68c`: QP07 original-attempt callback protection and regressions. Parent `dd4aefe901d71fc64e3c9fff7d5e1ac3a741bcf6`; accounting base `aa252cd8134972548a953a0c5e500da2f8d06af7`. Production file: `src/specs/specs.world_quest.c` only.
2. `a19a67ad021de8e6bbfb31ca9ffea31e41cb6aa7`: QP02 native recipe reachability and regressions. Parent fd997ee. Production file: `src/world/quest.c`, `quest_native_completion_owner::prepare_original` only.
3. Execution tooling bundle follows: owned SELECT reader, fixtures, assertions and three local runners; additive shared quest/world driver hooks are listed separately below. No shared production fix is included.

Current executed binary source is a19a67a. Latest fetched accounting source is
`85f9e1b2ee9149bd5033869adf6305b1fc21f5c5`; comparison with aa252cd8 has no quest-owner change. The new shared changes reserve native-load pool capacity and restore locked coin endpoint revisions. Preserve history when importing; qualification of that integrated source follows the current batch.
Original accounting base `17c033d69316b21da8598791fc95cae79baa8dc2` and research PR678 revision `55905eac1906cf59405764407f9d22497cccfff3` remain pinned.

## Meaningfully verified production fixes

QP07 captures persisted quest_started at request time in the pointer-free context
(12 to 16 bytes, actual pending limit64). Existing create/share strictly advance
the watermark even on same-second replacement/clock rollback; reset preserves it.
Settlement rejects replaced, completed, reset, invalid or rejected-payment
contexts before map/abandon/history mutation. Original callbacks and repeated
application behavior are covered by the actual extracted callback regression.
Generic stale refund still uses a credit helper that refuses active authority;
no restitution qualification is claimed. Specs SHA256:
`4d102b8ec12497bc0ce9c0c8bf170932c99495911f6a12d8871514f88b2563f4`.

QP02 performs original independent ITEM/TYPE availability checks before a paid
recipe's authority refusal. Unavailable branches return not_matched; genuinely
available paid branches still refuse before root selection. Original loader,
recipe, dispatcher and partial-prefix overlap order remain. Actual owner and
dispatch regressions cover three-hide backpack reachability, four-hide paid gloves
refusal, coin-only, empty/short/spare/duplicate/wrong-kind stock, overlapping TYPE
and repeated evaluation without custody mutation. Catalog-wide actual component
check covers285 paid ITEM/TYPE recipes from production areas/AREA (2668 definitions;
catalog fingerprint `04d687493b02ed27a3b32d070a0a71e9d10c1643406d5fb95fe6b1538b4bb314`).
Quest SHA256 `747a43b7f27ff6a3d53cc7776097b9c361d7c6437e6d04bbe5da717895677f8c`.
These source components do not prove authentic native reset custody.

## Builds and executed evidence

Both complete maintained strict a19a67a builds passed; no compiler/sanitizer
flags or original journey deadlines were weakened. Docker image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`, Ubuntu24.04,
GCC13.3, private network-none runtime, MariaDB10.11.14 on loopback33306.
Schema manifest `1fddd009cc67bba940efba28c2afc3d16d65ea8a476410034cd77c90a0a9aea2`,
maintained fresh canonical0062 migration runner, original calibration setup.
SQL binary SHA256 `6e4962a1fd38118d4f538157f70407293de7ec6840a3b57493fae972caf7e81a`;
flat binary `aaf8f44b534df5d00a88b49d1c8990affd54505746f6ec485f46bf65842b5bb8`.
QP07-only full SQL build also passed: fd997ee source, binary
`c13ad174fa10b6ab0972672411dff65a82abcf9fa083baecbfaf419237a97f3a`.

Private raw evidence is retained under
`bin/tests/quest-implementation-20261007/` in this worktree, ignored by Git.
No credentials, generated worlds, player data, logs or binaries are committed.
Exact commands, evidence directories and diagnoses: [EXECUTED_JOURNEYS.md](EXECUTED_JOURNEYS.md).

Passing actual journeys to date: original synthetic SQL offering calibration;
flat QP06 genuine secret-input search, offering crash and two cold recoveries;
flat QP02 explicit durable refusal with three original hide UIDs retained through
cold load; flat QP05 white-bear reward16048 UID820 through cold load; full-world
Woodseer quest creation/map/queued abandonment/replacement/final abandon on
both SQL and flat backends. Woodseer is actual giver16553/room16633/level56,
not Quietus giver1709/room1734/level11. It exercises the same bartender callback.
Stale replacement protection is qualified separately by the actual callback component.

Real SELECT-only reader execution captured before/offering-crash/recovered/second
cold cuts in `sql-qp06-xpslot-offering`, observed read-only transaction1 then0,
and exercised current custody/ledger/obligation/root/reference/inbox/effect/posting
queries. Missing native epoch evidence remains a refusal. Its final owned XP
assertion failed because it used absent native entitlement rows on the legacy path;
the maintained journey itself passed all original recovery assertions. This is
an owned oracle defect, not evidence of duplicate XP. The genuine v5 continuation
freezes200 XP at runtime reward slot2 (mask4), and the real log records one520 XP
award (200 x well-rested2 x Human1.3); experience1 to521 is stable on second cold.
The tool now calls the maintained continuation decoder and requires the actual
player race/level/rested affect and pinned production properties. Native default
assertions retain their original entitlement requirement. The corrected SQL run `sql-qp06-corrected-xp` passed, exit0,153.865s: actual history metadata (62 applied migrations), missing-player and row/BLOB controls, exact input/reward custody, C3000, effective520 XP, ACK and second-cold replay. Actual read-only snapshot transaction1 then0; cleanup confirms zero remaining schemas. This closes actual reader execution for the supported legacy path; active native authority remains separately required.

## Driver changes and boundaries

`run_quest_reward_ack_crash.py`: optional observer/private evidence retention and
SQL journey callback; original defaults, synthetic slot0 mask1, backend options,
assertions and deadlines retained. Kord search uses unmodified secret prototypes;
its XP mask derives from real head-linked production reward order. Exact schema
cleanup count is retained even on failure. Optional recovery-failure cut does not
mask the original error.

`run_world_quest_dual_backend.py`: optional retention and actual map request;
original default journey remains. Owned runners bind server hashes/source commits.
The flat inspector invocation adds existing baseline codec/adapter link inputs
locally; the shared inspector manifest is not edited. Fixture mob/object indices
are sorted for actual binary lookup; complete production records and quest order
are preserved.

## Retained frontiers with their actual owner

- Flat QP06 XP-ACK recovery passes, but subsequent ordinary `drop dagger` produces
  `The item remains in your inventory; its drop did not commit.` Original move
  assertion stays red. `src/cmd/actobj.c:item_drop_completion` and shared movement
  transaction own the refusal; no successful later move or move-before-lost-ACK
  is claimed. Preserve pending-journal/shared recovery work with primary.
- QP02 three-hide mini fixture reaches the real legacy durable dispatcher, whose
  `submit_durable_quest_offering` stops at an unsupported paid recipe. This is a
  separate legacy owner from corrected active_regular_sql native completion.
  Refusal preserves actual UIDs; active native handover requires the genuine
  integrated epoch/birth/context setup. Do not bypass financial guards.
- QP01 production secret Orb44164 invokes its real FOUND special and teleports
  the actor before the mini fixture's get assertion. Exact-success journey needs
  real producer context; partial three-statue refusal is being executed without
  clearing flags or suppressing specials.
- QP03 actual recipient16006 is invisible to the test player. Original success
  expectation fails `No-one by that name here.` Recipient-unavailable refusal
  with original inputs/cold load is being executed; authentic disappearing/reset
  recipient birth, original retirement, whole-stock cash and replacement proof
  stay primary-owned.
- Active refund/held obligations, native three-item handover, original D and
  lost-ACK movement remain dependencies; no seeded epoch, UID, birth, receipt or
  successful ACK substitutes for authority. Final qualification uses primary's
  integrated candidate at its major batch boundary.

## Earlier preparation history (superseded status statements)

# Quest accounting prep handoff — 2026-10-06

The seven-case independent prep pack is reviewable on
`origin/codex/accounting-quest-prep`. Accounting source:
`a5a1f4b196496d50a6f46afecfec03aba3e66190`; preserved-history merge:
`920a7bb8d7aae5400725fad0c70b505e4d263052`. Original accounting base:
`17c033d69316b21da8598791fc95cae79baa8dc2`; previous tested candidate:
`2c4e17f363ecff0f0d7eb3451ddb229abdd63d9a`; research PR678:
`55905eac1906cf59405764407f9d22497cccfff3`. Schema manifest SHA256:
`1fddd009cc67bba940efba28c2afc3d16d65ea8a476410034cd77c90a0a9aea2`.

Primary already consumed original bundles through
`3e9ce549a0a14c6414242aae231b69da64b10cce`; do not re-import them. New:

1. `6d5ec67a24dd052fd8c9e699c1a89abbf7716d9f`: recovery cut oracle/SELECT reader,
   actual shared QP06 interface check, current frontier/batch instructions.
   Metadata: `aacef7b38dd15c42b2fa5d8939f26e1c069e475d`.
2. `8b02790c5fc50fda76828d6f9b7ae94018b53b30`: supply/full-world variants,
   D/held/original-custody controls and completed batch instructions. Its parent
   is `aacef7b38dd15c42b2fa5d8939f26e1c069e475d`. Import in this order on a
   compatible integrated candidate. Merge920a7 imports upstream history only.

## Usable now

- Five static mini preparations retain exact production bytes/recipe order.
  --reward-vnum and --supply exact/shortage/spares select supplied roots without
  deleting competing recipes; QP01/QP05 wrong-kind variants are available.
  Thirty verified variants cover nine original contracts. These are supplied
  synthetic O goods, distinct from authentic reset/source custody.
- --layout world prepares unchanged production run-roots for all seven cases,
  including dynamic Quietus runtime producers; no stock, task, epoch, binding,
  UID or receipt is inserted. QP05 production huge-skin cap remains one.
- Executable native-row expectations cover exact input retirement/reward UID,
  spares, original incarnation, fees/C3000/frozen XP, receipt/source/reference/
  posting links, definite refusal, replay, linked refund, historical ACK presence,
  ordinary later room move, original D stock/cash and replacement independence,
  and stable accepted held obligation. Fourteen corruption-control unit tests
  pass; their modeled cuts provide no native authority.
- SELECT-only reader guards disposable loopback schema and explicit actual
  integrated source/binary pins, bounds rows/BLOBs and captures actual native
  tables in one read-only snapshot. Actual SQL execution is pending the batch.

## Current acceptance frontier

| Case | Current executed result | Remaining primary qualification |
|---|---|---|
| QP01 P0 exact sapphire/consumed Orb | Source/variants/current selector PASS | Authentic birth/source and four exact retirements/fresh reward/recovery |
| QP02 P0 overlapping hides/fees | Observation PASS; acceptance RED component30 | Actual first paid branch blocks backpack; native coupled fee owner |
| QP03 P0 Auriam D/reset | Replacement-generation predicate PASS; D oracle PASS on counterfactual controls | native_publish currently refuses successful disappear=true; whole original stock/cash lifetime, absent-body adoption/historical ACK/pair |
| QP04 P0 dynamic paid failure | Observation PASS; acceptance RED component30 | Active generic refund refuses injected callback; genuine debit/linked refund or visible held liability |
| QP05 P1 repeated skins | Source/sufficient/shortage/wrong-kind/selector PASS | Actual supplied custody/source; ordinary cap-one huge availability remains limited |
| QP06 P1 Kord mixed/lost ACK | Shared QP06 interface/source/selector/row controls PASS | Actual native route/XP-ACK/cold evidence; original later move and harder move-before-ACK/pair |
| QP07 P1 stale map/abandon | Observation PASS; acceptance RED component31 | Authenticate original A attempt; callback currently mutates replacement B |

The four diagnostics retain one component PASS and three RED; requirements were
not weakened. Current owner/producer files are unchanged from2c4e17. Original
selector/prototype-room seams were superseded by actual prepare_original and
observe_give in the prior refresh; that resolves the generation predicate only.
Shared QP06/--move-reward options close the old driver interface gap. Current
move occurs AFTER recovery; legacy single-ear invocation/fault boundaries cannot
qualify active native three-item handover or pair retirement.

## Exact checks and remaining shared requests

From repository root under Linux/WSL:

```bash
python3 tests/async/quest_accounting_prep/test_production_terms.py --variants
python3 tests/async/quest_accounting_prep/test_native_selectors.py
python3 tests/async/quest_accounting_prep/test_recipient_retirement.py --acceptance
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py
python3 tests/async/quest_accounting_prep/test_quest_cut_checks.py
python3 tests/async/test_native_quest_recovery_context.py
```

All pass within their stated source/fixture/component scopes. Native context
uses actual providers with ASan/UBSan; retained ignored local artifacts:
`bin/tests/native-quest-recovery-context-0vb_ww_n/`. Exact failures/commands and
artifact source/binary hashes: [RESULTS.md](RESULTS.md). No server, SQL, fault,
activation or native cold journey executed by prep.

[RECOVERY_BATCH.md](RECOVERY_BATCH.md) supplies all fixture/capture/oracle and
maintained journey commands, cwd/source/binary/schema/env prerequisites, genuine
producer/reset anchors, expected effects and external evidence locations. Its
shared request table names exact files/functions, current/required behavior,
original requirements, minimal interface and reproducer: maintained runner
case/native/capture controls before cleanup; backpack selection; genuine paid
refund/attempt ownership; D whole-stock/cash; exact historical original ACK
and genuine paired return. Those capabilities require the primary's actual
integrated candidate, not prep stubs. Fresh SQL activation and Plan5 release/
audit/restore ownership remain with their existing owners.

## Exact owned diff

New executable files: `quest_cut_checks.py`, `capture_quest_cut.py`,
`test_quest_cut_checks.py`, all under `tests/async/quest_accounting_prep/`.
Updated executables: `case_data.py`, `prepare_fixture.py`,
`test_production_terms.py`. Existing diagnostic scripts are retained unchanged.
Owned docs: this file, GAPS.md, CASES.md, RESULTS.md, SOURCE_FACTS.json and new
RECOVERY_BATCH.md, all under `docs/persistence/economy_accounting/quest-prep/`.
No authored file lies outside those two paths.

Every identified independently completable fixture/assertion is implemented and
small checks executed; remaining genuine shared capabilities are specified.
The primary can import the new owned bundles and qualify at its agreed major
batch boundary. This handoff claims completion of independent preparation only,
with integrated native gameplay/persistence/activation/release still pending.

Second bundle's exact owned files: docs CASES.md, GAPS.md, HANDOFF.md,
RECOVERY_BATCH.md, RESULTS.md in the owned docs directory; executable
prepare_fixture.py, capture_quest_cut.py, quest_cut_checks.py,
test_production_terms.py, test_quest_cut_checks.py in the owned tests directory.
First bundle additionally updates SOURCE_FACTS.json and case_data.py. The final
handoff-only commit records these SHAs; published branch tip is independently
resolvable after fetch. No shared-file patch is included.
