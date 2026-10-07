# Coordinator review of quest-prep follow-up inputs - 2026-10-07

The published quest-prep pack is a useful input to primary integration, with
independently passing component evidence for the two narrow production fixes.
This review does not qualify integrated gameplay or close the retained flat
reward-drop failure, native handover/birth, D retirement or durable restitution.

## Examined source and import compatibility

- Quest-prep branch tip examined:
  `1a0782f95339252b3c90a73ac102a437933a30c8`.
- QP07 callback protection:
  `fd997ee4147ba58d835bf4bd61783b51307bc68c`, production
  `src/specs/specs.world_quest.c`.
- QP02 recipe reachability:
  `a19a67ad021de8e6bbfb31ca9ffea31e41cb6aa7`, production `src/world/quest.c`.
- Current accounting source examined:
  `a7b3181edb80bf188f616c39b8e7cb144e11cb18`. Its `src/` diff from the worker's
  accounting source pin `f04317d9d72aa5594448809baad6041936b09801` is empty.
  This does not make the worker's fixes upstream-integrated.
- Each production-only patch passed `git apply --check` independently against
  that current accounting checkout. No patch was applied. The preimages are Git
  blobs `797f840fd1ddd886460de175960fd5146713b05b` (specs) and
  `8db42ef9086776b0dfd93431420ff23bd1886a8b` (quest).

QP07 captures the original persisted attempt watermark at request time and checks
it before map/abandon effects. QP02 checks original duplicate/availability facts
before unsupported paid-operation refusal: an unavailable earlier paid recipe
no longer prevents selection of a supported later recipe, while a matching paid
recipe still refuses without consuming roots. Neither fix grants native authority
or proves durable refund completion. Review current preimages and unpublished
primary work before importing; the primary never waits for sidework.

## Separately executed verification

The coordinator reused its clean isolated review checkout, detached at the exact
worker tip above. The earlier Collector review remains pinned in its published
report; switching this clean review checkout did not change worker branches.
Commands ran through WSL Ubuntu-22.04 from that checkout:

```bash
python3 tests/async/quest_accounting_prep/test_native_selectors.py
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP02 --acceptance
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case catalog
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP07 --acceptance
python3 tests/async/quest_accounting_prep/test_quest_cut_checks.py
```

All commands passed. Selection checks cover QP01/02/05/06 and all 285 production
paid ITEM/TYPE recipes. QP02's acceptance mode requires the supported backpack
branch. QP07 exercises actual extracted callback bodies with isolated boundaries,
including original attempt, replacement, completion/reset, repeated callback,
rejected payment and invalid context. All 16 oracle tests passed.

Production source SHA256 values match the worker handoff:

| File | SHA256 |
|---|---|
| `src/world/quest.c` | `747a43b7f27ff6a3d53cc7776097b9c361d7c6437e6d04bbe5da717895677f8c` |
| `src/specs/specs.world_quest.c` | `4d102b8ec12497bc0ce9c0c8bf170932c99495911f6a12d8871514f88b2563f4` |
| `src/core/utility.c` | `1930fb6bbcea509818c5a9986d47086ef1033adf124e8d396cd73df7a7b60668` |

These are component and oracle checks. The selector constructs NPC stock and
does not execute native custody/SQL/publication. Bartender debit, SQL and quest
persistence boundaries are stubbed; it does not establish a real committed debit
or refund. The coordinator did not rerun maintained builds or server/database
journeys for this review. The worker's pinned build/runtime evidence remains
separate in its canonical handoff and EXECUTED_JOURNEYS.md.

## Follow-up queue and unresolved review point

The worker has resumed with an actual active follow-up Goal, as reported by its
Goal-tool check. Its next assignment reconciles the candidate and observes the
retained flat QP06 later-drop refusal at actual provider predicates in a fresh
isolated reproduction. The retained failed command is absent from the journal
after definite refusal, so the existing error alone cannot identify the failed
precondition. Preserve original assertions and deadlines; do not alter shared
owners, counters or journals to make the test pass.

A queued review question concerns action `quest`: its context now captures the
watermark but its creation callback does not compare it. The published QP07 tests
exercise map/abandon protection. Establish whether a creation debit could settle
after a later attempt was created and then reset under the real queue/lifecycle
contracts. A hypothetical injected callback alone is insufficient evidence of
production reachability. The coordinator sent this bounded question to quest prep
after its current diagnostic; no production defect or requested shared-owner fix
is claimed here. Publish a reproducer and exact owner request if a real unmet
requirement is found.

The architecture worker independently inventories stable preparation extractions
and is considering Craft/Forge's material quote. It must avoid quest-prep paths
and shared craft execution/progression/ACK owners. Both streams' actual Goals,
progress, ownership and handoffs are checked by the coordinator under the
[continuing charter](WORKSTREAM_COORDINATION.md). This review completes these
specific source/component checks, not the continuing coordination Goal.
