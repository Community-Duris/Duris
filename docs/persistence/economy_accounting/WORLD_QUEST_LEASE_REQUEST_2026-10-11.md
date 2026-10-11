# World quest caller ownership and runtime handoff request

Source: `6022c0ad7feab34c13e27738d4295b65b8d010ac`, 2026-10-11.
WORLD owner: chat `01a129b4-a57a-76e0-baec-42b3dd2fe62f`, branch
`codex/accounting-accel-world`. Route this request through orchestrator
`01a129b3-9c1a-77b3-a10a-3ddafdb5c0a0` to the non-connected primary.

The [acceleration charter](ACCOUNTING_ACCELERATION_CHARTER_2026-10-11.md)
still marks WORLD ownership PROPOSED. Before runtime edits, the primary must
publish its exact accepted source, paths, protected WIP and exclusions. This
request adds the existing selecting quest caller to the proposed allocation;
it grants no shared authority or implementation permission by itself.

## Exact requested ownership

Retain the proposed exclusive paths `src/world/db.c`,
`src/world/quest_mobile_native_birth.c` and
`src/world/quest_mobile_native_birth.h`. Also acknowledge `src/world/quest.c`
if WORLD is to implement the native quest caller closure. That file owns the
actual GIVE driver, ordered completion selection, frozen branch program,
reward continuation and parent/child recovery progression.

Primary retains common save/coordinator/item movement, contracts, common
headers, registry/matrix, migrations and activation. No new module or header
path is requested here. An architecture extraction needs its own exact-path
acknowledgment after the current combined checkpoint and primary's connected
lifecycle map.

## Current producer and consumer cuts

- `reset_zone` in `src/world/db.c` retains the original dispatcher frame through
  `quest_mobile_native_birth_owner::original_reset_locals`. Its M command calls
  `prepare_mobile`; the native owner later submits, publishes and binds the
  original actor. Reset locals, cursor, source/time/season/counters, constructor
  entropy, procedure binding and scheduler effects stay with their real owners.
- `submit_native_quest_give` in `src/world/quest.c` calls
  `quest_native_gameplay_owner::begin`. The SQL entry requires the game thread,
  `active_regular_sql`, matching live player/mobile identity and room, original
  offering custody, and a genuine `quest_mobile_native_reference_copy`.
  `item_native_quest_preparation_owner::begin_acceptance` and the existing item
  publication owner retain the actual token. WORLD must not fabricate it.
- `quest_native_gameplay_owner::pulse` feeds the frozen original branch and
  child operation plus the authentic acceptance command/receipt into
  `quest_native_completion_owner::prepare_original`. Selection observes actual
  native stock/cash and retains recipe order and quantities. `not_matched`
  advances the branch; `refused` blocks that original flow.
- `quest_native_gameplay_owner::restore_phase2` and
  `quest_native_frozen_continuation_owner` consume the exact original command,
  receipt, publication progress, child frames and pair transitions. Current
  flat `begin`, `prepare_original` and `restore_phase2` explicitly refuse.
  Flat closure therefore remains a required implementation/qualification gate.

## Needed primary binding and preserved invariants

Identify the real installed birth SOURCE/reference and native lifetime owner,
whole NPC stock/cash owner, accepting admission owner, and the supported
isolated runtime configuration on the handed-off source. Supply the connected
path through these existing consumers, with exact returned operation/revision,
receipt and publication/recovery/ACK ownership. If a binding is unavailable,
identify its primary-owned producer and the specific missing consumer contract.

The original `native_quest_recovery_context` carries `native_before`,
`player_before`, receipt outcome/durable revision/exact result bytes,
`publication_stage`, started/returned publication steps and hooks, frozen
branches/next branch, parent acceptance/next child operation, child handoff
stage, exact next-child command and latest acknowledged child prefix. Preserve
these fields and NQR compatibility. A started/unreturned effect remains
uncertain; it cannot authorize a second effect. Committed-but-unpublished work
retains its actual token and hold. Parent/child terminal ACK and retirement
require the original guarded physical/receipt proof, including later legitimate
reward custody changes. Bounds and one reservation/effect owner remain intact.

## First accepting journey and current runner limit

Use actual Kord mobile `29257`, production inputs `29262/29263/29264`, reward
`29237`, C3000 and E2500. The genuine first journey must observe original reset
birth/source, input UIDs and whole stock/cash, native acceptance/consumption,
durable receipts, once-only reward publication and one cold recovery. Preserve
failure custody, unsupported financial refusal and original XP policy. Repeated
publication/ACK, lost completion, pre-ACK movement, reset retirement,
disconnect/shutdown/copyover and SQL/flat parity remain explicit later gates.

Existing `tests/async/run_quest_reward_ack_crash.py --quest-case QP06` exercises
real Kord terms and offering/XP-ACK faults with two restarts; `--move-reward`
checks later custody. Its own `prepare_quest_fixture` documents relocated NPC
and supplied O stock, and its runtime setup leaves accounting inactive.
`tests/async/quest_accounting_prep/prepare_fixture.py` also states that genuine
reset birth/source and original child/receipt observations are still required.
These runners qualify their legacy journey scope, not active native birth or
accepting admission. A genuine connected accepting journey is still needed;
do not convert the legacy setup into authority by relabeling its results.

While the binding/ownership request is pending, the orchestrator accepted
ACC-03-RC: run existing `test_native_quest_recovery_context.py` on this frozen
source after its exclusive resource grant. The actual-provider component covers
transport/forests, bounds, receipt equivalence, exact child frames, allocation
refusal and strong outputs. It grants no native lifetime, SQL, publication,
ACK or release qualification. The next independent executable gate is existing
SQL quest recovery read budgets with Plan5 and isolated service ownership.
