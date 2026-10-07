# Quest implementation and execution handoff — 2026-10-07

Production corrections are published on `origin/codex/accounting-quest-prep`.
This active implementation goal is distinct from the completed prep pack below.
Import the following new bundles after the previously consumed prep commits:

1. `fd997ee4147ba58d835bf4bd61783b51307bc68c`: QP07 original-attempt callback protection and regressions. Parent `dd4aefe901d71fc64e3c9fff7d5e1ac3a741bcf6`; accounting base `aa252cd8134972548a953a0c5e500da2f8d06af7`. Production file: `src/specs/specs.world_quest.c` only.
2. `a19a67ad021de8e6bbfb31ca9ffea31e41cb6aa7`: QP02 native recipe reachability and regressions. Parent fd997ee. Production file: `src/world/quest.c`, `quest_native_completion_owner::prepare_original` only.
3. `cab408e566ff7f1357c1fc783be7a095c75f4e78`: execution tooling bundle; owned SELECT reader, fixtures, assertions and three local runners; additive shared quest/world driver hooks are listed separately below. No shared production fix is included.

Current executed binary source is a19a67a. Latest fetched accounting source is
`f04317d9d72aa5594448809baad6041936b09801`; comparison with aa252cd8 has no quest-owner change. The new shared changes reserve native-load pool capacity and restore locked coin endpoint revisions. Preserve history when importing; preserved-history merge `6db65f624836f150ed3dfe33508e1f1719145fdb` is pushed and both maintained full builds are in progress. All seven current source/component cases and285 paid recipe checks pass; QP04 financial component30 remains red. No current-binary journey PASS is claimed before terminal build/run results.
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
  real producer context; partial three-statue refusal passed (exit0,87.380s), preserving original input UIDs through cold load without clearing flags or suppressing specials.
- QP03 actual recipient16006 is invisible to the test player. Original success
  expectation fails `No one by that name around here.` The owned runner expected a different literal response; it is corrected to the actual message. Recipient-unavailable refusal with original inputs/cold load follows on the current candidate; authentic disappearing/reset
  recipient birth, original retirement, whole-stock cash and replacement proof
  stay primary-owned.
- Active refund/held obligations, native three-item handover, original D and
  lost-ACK movement remain dependencies; no seeded epoch, UID, birth, receipt or
  successful ACK substitutes for authority. Final qualification uses primary's
  integrated candidate at its major batch boundary.

## Earlier prep import history

The original seven-case prep was completed and previously consumed through
`3e9ce549a0a14c6414242aae231b69da64b10cce`; published old tip
`f6e009282805d9be5e108fec08f9d923adf97f5c`. Later oracle/fixture prep bundles
`6d5ec67a24dd052fd8c9e699c1a89abbf7716d9f` and
`8b02790c5fc50fda76828d6f9b7ae94018b53b30` remain in preserved history.
Do not duplicate already consumed bundles. [RESULTS.md](RESULTS.md) retains the
historical pre-implementation outcomes; CASES/GAPS/RECOVERY_BATCH retain actual
producer terms and native acceptance requirements. Current source snapshots
bind all seven cases to the latest accounting base and preserved-history merge.
