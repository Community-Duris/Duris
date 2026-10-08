# Native quest fee-only owner boundary — 2026-10-08

Source/design acceptance reservation only. The public source implements a distinct
fee-only child (payload14, continuation6), with genuine parent/SQL/publication/ACK
owners. The existing QP03 reader deliberately covers non-fee payload12/continuation5;
it must not be treated as fee acceptance evidence. Reuse the maintained interfaces
below; no new authority façade or shared codec is needed for a future value test.

This document is the sole repository change. No build, component runtime, SQL,
journal access, fixture execution, native journey, migration or activation ran.
The continuing Goal remains **BLOCKED**. Neither this review nor earlier modeled
QP03 controls qualify the primary's separately owned private 128-file candidate.

## Pins and delivery boundary

| Input | Exact revision / scope |
| --- | --- |
| Reviewed public accounting candidate | `cda8aa6f65c72121e92d7c07efae7e91164d1050` |
| Public source tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migrations source tree; no applied-schema observation | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Preserved prep parent | `b4261edc5fc50e81bcc5d16d0a33a5c9344af2d5`; source tree remains `3ebd0dd0ec5e12dfaa5edf83a906aea4a6ebcc7d` |
| Original accounting / producer research | `17c033d69316b21da8598791fc95cae79baa8dc2` / `55905eac1906cf59405764407f9d22497cccfff3` |
| Accepted QP03 value reader / required Python correction | `0ee2549cd0ddf9a8efb6315a65dd99e0fff43c69` / `adf64fcf432b2787d98a1a42c44e385141e74680` |

The reviewed source comes from authenticated Git blobs, not the older prep checkout's
production files. The published document commit is identified by the delivery reply
and `git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/NATIVE_QUEST_FEE_ONLY_OWNER_BOUNDARY_2026-10-08.md` on the fetched prep branch. Canonical
`HANDOFF.md`, production, tests, registries, manifests and all other shared files
are unchanged. Prior reader results retain their original candidate/binary labels:
[QP03 adapter handoff](QP03_HELD_PAIR_ADAPTER_HANDOFF_2026-10-08.md).

## Actual trigger, production candidate and QP02 separation

[prepare_original](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/quest.c#L2582) selects
fee-only only when ordered consumed roots are empty and attempted COINS requirements
are nonempty. It requires an actual original child operation, original item-acceptance
command and successful typed48 receipt. Ordinary preparation without that triggering
pair refuses. The preliminary pass observes genuine NPC cash metadata when it first
reaches COINS, compares each requirement to the original NPC cash independently,
and returns `not_matched` for shortage; missing cash metadata refuses. The later
projection preserves ordered goal slots and sequential charged/insufficient/nonpositive
outcomes. This is a charge against NPC cash, not proof of a player-to-NPC payment.

The preserved production catalog has 2668 definitions and
35 coin-only definitions after filtering its existing decoded keys.
It was generated with `scripts/zone_story_quest_catalog.py` and production `areas/AREA`;
no new catalog survey ran. [Production catalog selection](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/scripts/zone_story_quest_catalog.py#L28)
excludes dynamic bartender producers; those remain separate world-quest cases.

One small, authentic **source candidate**, not an executed fixture, is Gorblag:

| Binding | Verified public source fact |
| --- | --- |
| Production definition | [goblinht.qst #70023](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/areas/qst/goblinht.qst#L97): sole QA completion `G C 5`, `R I 70018` once, no D, no XP |
| Area / reset | [production AREA](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/areas/AREA#L261); [zone M command](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/areas/zon/goblinht.zon#L433) creates mobile70023 in room70029, limit1, chance100; no following G/E for this mobile |
| Actual producer identity | [mobile prototype70023](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/areas/mob/goblinht.mob#L327): Gorblag, prototype cash `0.0.0.0`; this is not an authenticated funded runtime lifetime |
| Reward identity | [object70018](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/areas/obj/goblinht.obj#L290), one plastic package; genuine new reward UID/source/custody still required |
| Quest binding / dispatch | [assign_the_questers](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/quest.c#L2270) installs `qst_func=quester`; [boot assignment](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/specs/specs.assign.c#L1265) loads then assigns; [command dispatch](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/cmd/interp.c#L2836) invokes it |

Full boot/reset and genuine native birth/admission/ACK requirements remain in the
[native journey blueprint](QP02_QP03_NATIVE_JOURNEY_BLUEPRINT_2026-10-08.md).
[Full boot reset](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/new_events.c#L2091) is skipped for
preserved copyover/Redis world recovery. A raw load, same VNUM, reset token, or
synthetic reference does not establish this birth. A successful fee journey also
needs real funded native cash/origin and an actual item acceptance that reaches
the branch. Neither has been demonstrated for Gorblag; the prototype starts empty.
No offered ingredient VNUM is invented for this coin-only branch. Its parent still
owns the actual accepted item UID/VNUM and custody move; the fee child consumes none.

QP02's leatherworker19005 recipes consume hide19006 and may charge C1000/C2000/C10000;
they are **item-plus-cost**, not this route. Their source binds birth operation and
pre-charge stock revision. Fee-only binds fee operation and pre-charge mobile revision
in the [actual source capture](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/economy/economic_gameplay_authority.c#L995).
Use the existing [NPC-cost handoff](NATIVE_NPC_COST_ASSERTION_HANDOFF_2026-10-08.md)
for QP02; do not broaden its assertions or the QP03 reader implicitly.

## Original evidence and exact acceptance requirements

| Requirement | Required evidence / actual maintained owner |
| --- | --- |
| Full parent and child | Original encoded command bytes, attachments, phases, envelope revisions and exact parent `next_child_command`. Parent is accounted v12 item acceptance; child is accounted v14 fee-only with `publication_required`, accepted timestamp, actual PID/native lifetime, original goal slot and literal accounting intent. [Pair validator](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/native_quest_recovery_context.c#L774) is structural validation, not proof that either carrier was retained. |
| Zero consumption | No selected/target UID, item count/blob, multi-root, consumed roots, consumed steps or GIVE hooks/messages. The NQF2 child's `parent_acceptance` field is zero; parent correlation is the actual pair and QRF6 triggering acceptance, not the QP03 field rule. [Fee payload shape](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/item_transfer_command.c#L1689); [fee context shape](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/native_quest_recovery_context.c#L254). |
| Original parent receipt | Exact successful typed48 item-transfer result including accepted root/count, from/to revision+1, max item revision+1, no corpse/collector flags; full actual parent command still required. [Trigger binding](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/quest_reward_continuation.h#L589) checks same PID/mobile/VNUM/generation, distinct parent/action, and fee mobile sequence at least parent mobile revision+1. Do not replace that inequality with equality. |
| Fee continuation / obligation | Literal v6/QRF6 continuation, no consumed roots, actual action operation/source/mobile instance, original parent ID and typed48 result, frozen reward slots/flags/quantities/credits/XP awards. [Maintained fee encoder](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/quest_reward_continuation.h#L637) already exists. A successful branch owes its actual stored literal obligation; prefix/failure with continuation `none` must not acquire a fabricated reward obligation. |
| Fee source / wallet | Source kind `quest_action`, source=fee operation, generation=actual native birth generation, sequence=pre-charge mobile revision, slot=original completion index. Wallet mapping is actual origin mapping, native wallet context12, not native instance ID. Original lineage/epoch, writer, intent/domain/plan digests and source claim must agree. [Fee repository](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/item_transfer_repository.c#L4411) verifies the exact source and native lifetime. |
| Charge and ownership | Recompute the original denomination projection with [actual cost provider](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/economy/native_quest_cost.c#L65), including change and ordered attempts. Fee success advances mobile revision once, preserves stock/custody/item forests, and uses the projected cash revision (advance once iff a charge occurred). [Native fee transition](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/quest_mobile_native.c#L694). |
| Durable financial effects | Quest-cost policy1, sink44/context0, writer ITEM_TRANSFER, native wallet context12; exact account effects/postings for charged attempts, balanced denomination deltas/copper values. [Actual effect compiler](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/economy/item_transfer_accounting.c#L456); [policy constants](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/economy/native_quest_cost_policy.h#L10). The fee plan has zero children/item events/item references/ownership-ledger rows. Successful source claim count1; failure count0 and no account/posting effects. [Financial row verification](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/economic_sql_item_transfer_transaction.c#L3064). Reward item/cash operations are separately verified obligations, not fee-root item effects. |
| Fee result | Exact canonical64-byte NFR1: mobile instance, player PID, cash/mobile/stock/native-custody/player-custody revisions; durable revision=max of the five clocks, all remaining result-buffer bytes zero. [Result provider](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/item_transfer_command.c#L2847). Terminal failure binds pre-charge cash/mobile revisions and nonzero error; applied/already-applied binds projected cash and mobile+1. Do not decode this as the parent's typed48. |
| Historical proof | Original inbox command/intent identity, committed result, source/account effects and success outbox, original acceptance proof, literal obligation. Reuse [acceptance verifier](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/critical_command_repository.c#L5516), [receipt verifier](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/critical_command_repository.c#L5625) and [obligation verifier](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/critical_command_repository.c#L5742); a matching present-day row or hash-only command is insufficient. |
| Held bodies / current projection | Literal player BEFORE=AFTER forests, native forest, acknowledged held save revision and original body/binding bytes from [save checkpoint owner](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/player/player_save_pipeline.h#L330); publication has a distinct private held-body accessor. [Fee publication lock](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/economic_sql_item_transfer_transaction.c#L2611) authenticates historical result before current native lifetime/origin, save revision, canonical owner and full custody locks. The generic recovered-continuation helper is v12-gated by [its command predicate](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/economic_sql_item_transfer_transaction.c#L310); do not route v14 through it. |
| Actual world | [Fee world observation](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/item_movement_transaction.c#L6183) compares full runtime UID/owner/root/parent/kind/item revision/owner revision/state census, real actor/mobile, native reference and cash mapping/lineage/birth epoch/revision/denominations. SQL proof alone does not install a world or grant publication. Unrelated inventory is part of the cut. |
| Reward ACK / exact readback | [v6 ACK transaction](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/quest_reward_obligation_pipeline.c#L59) rereads the same literal, authenticates the genuine carrier and immutable historical proofs before ACK DML, and requires original-session cleanup. [Exact obligation read](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/quest_reward_obligation_repository.c#L718) checks full XP entitlement set and verified economic receipts; economic mask is derived, not a SQL column. Compute required masks from actual reward slots; QP03 XP0/economic3 constants do not apply. |

## Retry, cold recovery and retirement boundaries

[Fee obligation owner](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/quest.c#L1155)
borrows the actual physically released original phase2 child and exactly one matching
parent through [coordinator-owned copy](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/persistence/critical_command_coordinator.c#L2803).
Missing/unreleased/uncertain carriers, duplicate parents and mismatched literal terms
refuse; reconstructing them from SQL rows is not an alternative. The execution side
already has [original_fee_acceptance](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/player/player_save_pipeline.c#L5486)
and the private coordinator parent accessor. Reuse them within their real owners;
prep must not expose their capabilities or install replacement observers.

[ready](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/quest.c#L1226) proves on one
reconnect-disabled transaction/session and returns success only after confirmed rollback,
no cleanup error and idle verification. [fee_publish](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/item_movement_transaction.c#L7504)
requires actual held bodies, exact sealed completion, original registry/generation,
current SQL cut, real world/cash observation, before/after checkpoints and same-session
cleanup. A cold effect started without a returned checkpoint is refused even if an
endpoint resembles AFTER. [rebind_fee](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/item/item_movement_transaction.c#L7025)
restores only a genuine original checkpoint after SQL/world proof and confirmed rollback;
missing/mixed lifetimes or SQL AFTER versus invented BEFORE must not be repaired.

Execution lost reply, physical publication ACK, reward-obligation ACK and paired
retirement are separate cuts. [Fee physical ACK validation](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/native_quest_recovery_context.c#L743)
binds the full receipt including padding; it supplies no hold capability. Reward ACK
must record its actual transaction outcome/session/commit/cleanup, including ambiguity,
and its exact readback; refusal alone does not prove no DML. Repeated recovery must
preserve original operation/source and avoid duplicate charges/rewards. A lost COMMIT
reply needs the maintained original-history retry, not a new operation ID.

[retire_completed](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/src/world/quest.c#L2902)
authenticates the original successful parent, fee child and exact acknowledged obligation
on one transaction, confirms rollback/idle cleanup, then performs actual paired journal
transition to null. Live handoff1 waits for the original handoff2/take. Only actual paired
success sets the retirement latch; absence of a parent is refusal. Retain both original
preimages through uncertain journal I/O. Current rewarded inventory may have advanced;
historical ACK cleanup must not rebind, republish or repeat reward effects.

Disappearance is explicitly outside the usable fee route: acknowledged
`native_fee_shape` requires `!publication_terms.disappear`; restored frozen reward paths
also reject D. Coin-only production definitions with D (for example
[97010](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/areas/qst/icecrag.qst#L201)'s two 97146 rewards and C800)
therefore do not supply positive fee fixtures. No D bypass, whole-stock
retirement substitute or inferred successful disappearance is reserved here.

## Smallest future owned addition and rejection controls

First reserve only an **actual-provider, modeled value component**, in two new owned paths:
`tests/async/quest_accounting_prep/native_quest_fee_owner_boundary_test.cpp` and
`tests/async/quest_accounting_prep/test_native_quest_fee_owner_boundary.py`.
Proposed command, after implementation in an isolated composed candidate on D:,
`python3 tests/async/quest_accounting_prep/test_native_quest_fee_owner_boundary.py`.
It does not exist yet and was not run. No native journey command is ready for this branch.

Reuse maintained [context recipe](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/tests/async/test_native_quest_recovery_context.py#L17)
flags, sanitizers, build/artifact guards and 20-second runtime deadline; retain all its
controls unchanged. The accepted [QP03 wrapper pattern](https://github.com/Community-Duris/Duris/blob/b4261edc5fc50e81bcc5d16d0a33a5c9344af2d5/tests/async/quest_accounting_prep/test_read_native_quest_pair.py#L185)
already resolves the general payload decoder's actual provider closure by adding
`native_quest_cost.c`, `native_quest_coin_give.c` and `lockpick_retirement_continuation.c`.
Reuse those real nodes and existing critical-command, item-transfer, native reference,
snapshot, recovery context and accounting providers; no selector stub, copied codec,
replacement authority or shared recipe edit. Link feasibility is not freshly tested here.

The public [maintained context fixture](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/tests/async/native_quest_recovery_context_test.cpp#L1237)
is v12 value coverage, and [publication MySQL runner](https://github.com/Community-Duris/Duris/blob/cda8aa6f65c72121e92d7c07efae7e91164d1050/tests/async/run_native_quest_publication_mysql.sh#L11)
is an isolated publication seam, not established fee coverage. Authoritative search of
public `tests/async` for `fee_only|native_fee|quest_fee` finds only unrelated bartender
difficulty scaling. No dedicated public fee-only test/fixture was found; none is claimed
to pass. Existing QP03 tests remain unchanged and deliberately reject v14/v6.

Future controls, each classified as value validation rather than native execution:

1. Valid modeled v12 parent + zero-root v14/NQF2 child + canonical v6/QRF6 literal,
   typed48/NFR1 results, structural pair/ACK validation and exact projection agreement.
   Test empty player/native inventories with the real forest codec as well as unrelated
   roots; never equate an encoded empty forest with zero bytes.
2. Wrong payload/continuation versions; nonzero consumed root/blob/UID/hook or child
   `parent_acceptance`; wrong parent/action/PID/mobile/VNUM/generation/slot/sequence;
   wrong typed48 root/count/revisions/flags; modified original child-command bytes.
3. Invalid denominations/order/revision overflow; altered charged amount/change/cash
   projection; wrong NFR1 clocks/size/padding/outcome; pending versus physically proven
   context; frozen D; partial/ambiguous effect checkpoints. Preserve provider-defined
   strong failure output; valid nonpositive/insufficient attempts are not blanket errors.
4. Exact literal reward quantities/duplicate ordinal/XP-credit terms and required masks;
   modeled missing/extra/changed ACK/receipt observations and original-operation retry
   agreement. A self-consistent forged pair can pass pure value checks: report that
   external owner authentication remains unproven instead of inventing a trust seal.

After genuine owner exports exist, separately reserve a passive fee reader/assertion
with a reviewed input contract. Do not broaden the current QP03 executable as a shortcut.
The native acceptance evidence still missing is precise: original retained parent/child
bytes/revisions/phases; real held forests/save stage; original SQL historical result and
financial/source/obligation readback; exact native origin/lifetime/cash and current full
world/custody census; same-session transaction/rollback/idle dispositions; actual physical
and reward ACK outcomes; actual paired retirement attempt/preimages/return. Those owners
exist internally; a safe authenticated evidence export and Gorblag funded/item-parent
setup have not been demonstrated. Missing prep exports are not missing production
decoders or permission to replace the existing owner methods.

## Source-check results and review handoff

Evidence directory: `D:/Dev/Temp/native-quest-fee-only-boundary-20261008`.
`prepare.py` authenticates literal source bodies/Git blob IDs/SHA256 against the pins,
checks every generated source line anchor and existing relative document link, and
writes `source-proof.json`. It only reads Git/source/catalog and writes this document
plus task-local proof. It invokes no compiled provider, journal or database.

Commands: `python D:/Dev/Temp/native-quest-fee-only-boundary-20261008/prepare.py`;
`git diff --check`; single-path staged diff review; normal commit and
`git push origin codex/accounting-quest-prep`, followed by remote-tip/clean-tree checks.
Source validation: **PASS**, 32 authenticated pinned bodies, 46
verified source line anchors and all three relative document links. Exact publication
results accompany the delivery reply. The whole
pack is a source-verified reservation; **zero newly passing component tests and zero
completed native journeys** are asserted. Final gameplay/SQL qualification remains at
the primary's integrated-candidate batch boundary. This document adds no recurring work
or unrelated auction observer task and does not alter the blocked continuing Goal.
