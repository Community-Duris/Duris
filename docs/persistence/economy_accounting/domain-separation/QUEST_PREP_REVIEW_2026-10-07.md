# Coordinator review of quest-prep follow-up inputs - 2026-10-07

The published quest-prep pack is a useful input to primary integration, with
independently passing component evidence for the two narrow production fixes.
The follow-up diagnosis, creation-watermark assessment and selected ordinary
full-world cold-room control below are reviewed with no actionable owned defect.
They do not qualify integrated gameplay or close the retained mini-mode flat
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

### Completed follow-up review and selected cold-room control

The selected control is implemented at
`0ceae3a847f6573ec326bac3f36fb632ef5c2fe9`, with final canonical handoff
`5e019a44ebb805feadc5101534ce404d4aaa53ad`. The coordinator inspected the actual
two-process execution, reviewed the published runner against its executed SHA256
`3adc0b2647e1cf657d3a3dafccda880fac3c3dac8c6000e3876a717b0968515a`, and independently
recomputed all custody/revision/conservation assertions from private result
SHA256 `5c09a23e11ebbdb45d70c65c22ef9a67d79f3e331b95f91f6c46490cf84f3568`.
The 75.083-second journey is a terminal PASS. This review does not repeat it.

Original mace677 UID68076 moves player to room revision1 and survives genuine
full-world cold restoration unchanged. Before any GET, explicit `drop 1.sword`
moves original starter sword1108 UID68097 with player/room revisions3/1 to4/2;
all unrelated roots, wallet and XP remain exact. Genuine bulk stock release
then conserves every original identity and advances counters once, allowing
the original mace to be retrieved at the same UID. Both full processes contain
normal shutdown and restoration-stage evidence. The maintained camp calibration
and stock release happen in owned fixtures; no area/special, authority or deadline
was changed. The strict selected drop/get deadlines remain20 seconds.

The coordinator independently read the retained ELF hash from the task container,
matched provider/boot/restoration sources to binary source6db, and checked every
recorded world input: 2224 source files on the host and six generated world files
copied read-only from the stopped executed container. All2230 match private
manifest SHA256 `bd2353340862eea330e74e4e6b301dde74c038d7fc5347f464809c718c9b1675`.
Generated outputs stay private/ignored. The published runner parses and matches
executed source exactly. Four earlier fixture failures remain retained: omitted
camp calibration, response literal, wrong shared-alias item UID, and genuine carry
capacity. The coordinator independently detected the wrong-item cut and sent
exact corrective guidance; the final run preserves the UID assertion.

Disposition: delivered, qualified and reviewed for this **ordinary nonempty
full-world cold-room control**. The new worker Goal's tool-confirmed COMPLETE
status is recorded in its final handoff. Original mini QP06 remains RED;
Kord reward/XP-ACK, active native SQL, empty-room/third-cold variants and native
birth/retirement/refund remain unqualified and primary-owned. No additional owned
control, missing handoff or actionable review defect remains in this finite
selected follow-up. The reservation/checkpoint text below is historical.

Candidate reconciliation is published in `435a929a6`; the current accounting
candidate's changes remain documentation-only relative to the examined source.
Publication/applicability does not establish that the remote primary imported
the two quest fixes or executed their combined candidate.

The real-provider refusal diagnostic is published at `3a7571afa`. The
coordinator independently inspected the retained diagnostic JSON (SHA256
`d7a77d3cbd6ee6834b5ab7bbe957f08a408a19b72278d587a9ea16ea4a7488b7`), matched
provider/movement/observer hashes with published Git bytes and reviewed the
predicate trace. On retained source `6db65f624836f150ed3dfe33508e1f1719145fdb`,
flat ELF `8aedc856f2ff9cfee15ce694b16d28540daba1a5f41dedd40d59de91ad1b1ea5`,
expected player revision8 matches observed8. Destination room revision is
expected0/observed7; item UID/root821, parent0, revision1, VNUM29237 and state1
match captured facts. Seven genuine earlier room transfers explain durable7.
Legacy admission is selected: accounting inactive and no live-drop token.
The provider refuses ESTALE at the room prerequisite before effects.

Source inspection connects that result to mini-mode cold boot skipping normal
room restoration: the uncached runtime room revision installs zero while durable
room revision remains seven. Full restoration normally hydrates that authority.
The diagnostic preserves the original 20-second later-drop assertion and its
RED result. Its own terminal success means the failure was observed and explained,
not repaired. No counters, authority, journals or shared drivers changed. This
does not establish a general full-world defect or active SQL drop behavior.

Creation review `9ef7b5f09` publishes owned tests of actual callback/reset and
input-queue slices. The coordinator independently passed both
`diagnose_creation_watermark.py` and `test_creation_input_gate.py` at that pin.
Creation captures but does not compare a watermark; reset retains it. A controlled
replacement/reset allows the old callback. Real queue classification defers ASK
while busy and permits QUEST. Default sharing is disabled (`0.000`); positive
cross-actor sharing plus reset is a conditional candidate, not a demonstrated
held-debit race. Persistence/debit boundaries remain stubbed; the unchanged full
queue runner retains unrelated missing-provider/coin fixture compile failures.
No production guard/refund fix is authorized or claimed by this evidence. An
authentic lifecycle/debit fixture and owner decision are explicit dependencies.

The coordinator selected one finite additional acceptance gap: a genuine ordinary
full-world cold-room movement control, reserved before edits at
`8be236a150075a9f28de7a9991e4ad40705be779`,
[COLD_ROOM_CONTROL.md](https://github.com/Community-Duris/Duris/blob/8be236a150075a9f28de7a9991e4ad40705be779/docs/persistence/economy_accounting/quest-prep/COLD_ROOM_CONTROL.md).
Existing owned fixture interfaces suffice. The control must cold-restore a real
dropped starter mace, then drop an untouched sword before GET can modify the
runtime room revision. Preserve exact UIDs, original deadline and genuine durable
custody; no shared hook or fake room state. This is pending execution/review and
does not claim Kord reward, XP-ACK, birth, retirement or empty-room proof. The
worker is to verify a new actual Goal for this additional control after its
previous finite follow-up Goal completed. Keep usable prior handoffs published.

The original queue instructions below are historical; their diagnosis and
bounded creation question are discharged at these explicitly limited scopes.

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
