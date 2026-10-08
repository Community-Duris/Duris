# Quest reward completion authority boundary â€” 2026-10-08

The published static offering path retains original reward terms, an obligation,
per-recipient XP entitlements and exact XP save receipts. Returning from its
native reward handoff is explicitly not proof that rewards succeeded or were
saved. The dynamic world-quest item callback validates the current task and
carried reward, then performs progression, history and reset through a copied
in-memory business context. The inspected dynamic path does not retain an
equivalent durable business continuation. An item receipt therefore cannot, by
itself, prove the latter work completed or remains recoverable.

This is a source/design boundary, not an implementation or qualification result.
It introduces no owner, context, state machine, reward policy or completion claim.
Only this document changes. Production, shared drivers/contracts, migrations,
registries, canonical HANDOFF, finish plan and Plan 5 remain with their owners.
The continuing actual native Goal remains **BLOCKED**.

## Pins and reviewable delivery

| Input | Exact revision / scope |
| --- | --- |
| Preserved prep parent | `f9c7f6d33f162bc58083b037f39697f2513ff356` |
| Fetched public accounting candidate | `f679ee312baccbe077267aedd36fceaa2f97b14f` |
| Public source / migration trees | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Compared earlier public revisions | `78d71393ee625a975b66d583b447577267e3616c`, `257190ac149a86af59b1c3c2fe321abb382cd8ef`; same source/migration trees |
| Original seven-case accounting / research pins | `a5a1f4b196496d50a6f46afecfec03aba3e66190` / PR #678 `55905eac1906cf59405764407f9d22497cccfff3`; historical producer dossier scope only |
| Delivery commit | Containing commit: `git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/QUEST_REWARD_COMPLETION_AUTHORITY_BOUNDARY_2026-10-08.md` |

Thirty-one exact public file bodies were exported with Git blob IDs and SHA256s
to `D:\Dev\Temp\quest-reward-completion-boundary-20261008\public`; the adjacent
`source-manifest.json` identifies each. Source links below pin that public commit,
not local prep production files. Reused owned inputs are pinned separately to the
prep parent. Private flat-shop reports `87c253f4/140prod/75C` supply no inspectable
quest API here; their compiler, native, SQL, gameplay, persistence and recovery
qualification is unexecuted here. They do not change this public source boundary.

## Existing evidence to reuse

- [CASES.md](CASES.md) already defines QP01 exact sapphire kinds/consumed Orb,
  QP05 distinct repeated skins and honest reset supply, and QP06 Kord mixed
  item/coin/XP completion. Preserve their existing production producer, dispatch
  and full-boot/reset binding requirements. Do not repeat the recipe survey.
- [CURRENT_CAPTURE_EXECUTION.md](CURRENT_CAPTURE_EXECUTION.md) records genuine
  **legacy SQL** Kord offering/XP-ACK/later-move evidence at source
  `36bfef3c9e9a97b5dd94fbf620ffe2f21e02a8d1`, ELF SHA256
  `c997fabfb1373f86f84dc8b33e5202f7f8ef2ac4af2646c800f0649c1fb319b6`, operation
  `74efcd77295fee7b1bcd55aafe776566`: inputs4/29262,6/29263,8/29264;
  reward820/29237; C3000; frozen XP200/slot2/mask4; actual XP1â†’521 once.
  Acknowledged0 at crashâ†’1 after recovery, then the same reward moves to room22800
  and survives another cold recovery. Keep the original aggregate failure and
  later postprocessing repair distinct. This is not active native origin proof.
- [QP03 held-pair reservation](QP03_HELD_PAIR_PROOF_RESERVATION_2026-10-08.md)
  already maps exact original parent/child receipts, original obligation ACK,
  same-session cleanup and terminal pair transition. Auriam D1/reset is outside
  the D0 frozen publication predicate below. Do not infer retirement from NPC
  absence or from a reward's later current location.
- [Fee-only owner boundary](NATIVE_QUEST_FEE_ONLY_OWNER_BOUNDARY_2026-10-08.md)
  retains QP02 overlap and real fee authority; [world-service boundary](WORLD_QUEST_SERVICE_AUTHORITY_BOUNDARY_2026-10-08.md)
  retains QP04/QP07 map/abandon attempt and restitution scope. Item completion is
  not a substitute for those financial owners. No fee codec/math is repeated here.
- [Optional replacement-task assertion handoff](WORLD_QUEST_TASK_ASSERTION_HANDOFF_2026-10-08.md)
  retains its accepted four-cut agreement scope. Do not reopen that helper or
  promote captured PID/start/target values into authenticated runtime authority.

## Published boundaries and transitions

The table separates physical item publication, business completion and terminal
cleanup. Each row identifies an existing owner/callsite and what its evidence
does and does not discharge. Static zone-story history and dynamic
`world_quest_accomplished`/player task state are different persistence domains.

| Boundary | Static NPC offering / original obligation | Dynamic world-quest item reward |
| --- | --- | --- |
| Dispatch and original identity | `quester` routes active regular SQL item GIVE to native submission, other durable items to retained durable offering, and only inactive ordinary GIVE to direct `quest_completion`/`finish_quest_reward`. Native `prepare_original` retains original operation, NPC/player identities, branch and selected roots. [quest.c1949](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L1949), [2440](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L2440) | Actual `quest_ask`/`quest_kill` call full/final-kill reward producers. The reward context retains PID, source `(PID<<32)|quest_started`, target VNUM, type, full-reward flag and actual item UID. No original player runtime ID or original offering operation is in that context. [world_quest.c236](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L236), [446](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L446), [470](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L470) |
| Capture versus mutable world | Admission captures eligible in-room credited PIDs and levels, actor level/racewar/name and party context. Version5 stores literal reward slots and reward-major PID/slot/amount XP tuples. Owner cap is original next-level XP/10; peer cap is original next-level XP. Version6 has its existing fee-owner specialization. A decoder proves value shape, not the authentic NPC birth/source, retained participant or current world. [quest.c640](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L640), [707](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L707), [812](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L812), [861](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L861) | `quest_item_reward` selects an eligible random zone item using stored quest level, with current actor level fallback, then random-equipment fallback. The actual UID is retained in memory. XP is recalculated from current level/EXP_NOTCH/properties; epic eligibility uses current actor level and stored quest level, with fresh RNG for levels46â€“55. None of those progression outcomes is frozen in this business context. [world_quest.c152](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L152), [194](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L194), [210](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L210) |
| Durable admission | Existing successful item transaction inserts original obligation/literal continuation. For credited group size>1 it also inserts full original XP entitlement tuples. Solo version5 uses owner's XP mask without group entitlement rows. [critical_command_repository.c760](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/persistence/critical_command_repository.c#L760), [3075](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/persistence/critical_command_repository.c#L3075) | Active reward submission uses existing item creation owner plus copied callback context. Its economic source ID is not an authenticated NPC birth, a bank operation, or a serialized business completion obligation. Active mercenary compound coin/item branches refuse under their existing guards. [world_quest.c333](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L333), [382](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L382) |
| Physical publication | Offering publication retires original consumed roots and marks player components dirty. Reward recovery separately submits item/coin successors with original slot/source linkage and skips verified applied economic slots. Item source includes original consumed root/kind/duplicate ordinal, or version6 original action. [quest.c969](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L969), [1498](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L1498), [1525](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L1525) | `creation_grant_completion` validates active queue/front UID, publishes physical custody and retains the queue if physical publication fails. On success it pops the request before calling the void business callback; no business success result is requeued. This path's movement completion retains physical-publication failure, not a durable world-quest business ACK. Do not borrow ordering from another item-publication branch. [item_movement_transaction.c1338](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/item/item_movement_transaction.c#L1338), [2418](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/item/item_movement_transaction.c#L2418) |
| Business gate | `recover_pending` requires original PID/op, valid frozen roots, verified economic history and version6 fee readiness. Duplicate in-memory original recovery is suppressed. Frozen native `publish` additionally checks original runtime actor, exact command/continuation/op, consumption action and D0; it accepts version5 or the existing version6 fee-only case. [quest.c1272](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L1272), [3187](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L3187) | Callback requires PC/context shape, matching PID/start-derived source/target, committed result, exact root UID, positive item count and actual carried reward. Mismatch returns after logging. Rejection can roll final-kill count back to totalâˆ’1 and leaves task active. These predicates do not prove original runtime actor or durable business state. [world_quest.c260](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L260) |
| History / reset | Static zone-story tracking uses frozen definition/context and stable original tracking ID; acknowledgement eligibility requires tracking success. Tracker still requires current catalog definition and uses current season/content revision. `record_authoritative_completion` accepts applied/already-applied and saves real state; save failure restores prior serialized state. It does not reset the player's dynamic task. [quest.c1344](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L1344), [zone_story_quest_runtime.c252](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/zone_story_quest_runtime.c#L252) | After reward publication: final-kill remainder XP if applicable, epic, full-reward XP if applicable, `sql_world_quest_finished`, status dirty, `resetQuest`, GMCP. SQL history helper returns void; its INSERT carries no original operation/attempt key, and callback receives no durable history result before reset. Reset clears target while preserving task-start watermark. [world_quest.c127](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L127), [317](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/world_quest.c#L317), [sql.c3343](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/sql/sql.c#L3343) |
| XP application versus actual delta | Frozen amount is the input to `gain_exp`, not a guarantee of actual XP delta. Current room, race/rest/difficulty, level and global caps still apply. Recovery retains `(original op, slot, frozen amount)` receipts and status/trophies save work. Static supported effect set contains item/coin/skill/XP, no epic award. [quest.c1444](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L1444), [limits.c1163](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/limits.c#L1163), [1511](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/limits.c#L1511) | `gain_exp` has those actual-runtime policies too. Callback ignores its return and keeps no exact XP/epic application receipt. Current level changes during progression can affect later work. A nominal quest XP number or item UID cannot prove progression applied exactly once. Same public `gain_exp`; business call order is above. |
| Save and acknowledgement | Save request waits until dispatch and pending item/coin callbacks complete. XP recovery clears its wait only on the same PID, a sufficient revision and **every exact expected original-op/slot/amount receipt**; a generic later revision alone cannot discharge XP. Snapshot repository applies components and XP markers before save-revision CAS and same-session COMMIT. [quest.c268](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L268), [311](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L311), [player_snapshot_repository.c3071](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/player/player_snapshot_repository.c#L3071), [player_save_pipeline.c3950](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/player/player_save_pipeline.c#L3950) | Physical grant marks inventory dirty; callback marks status dirty. These are save work requests, not a joined durable task/history/XP/epic ACK. No corresponding business save receipt or native savehold-release proof is exported by this callback. Shared save/coordinator ownership remains with primary. |
| Group recipients / disconnect | Admission does not rediscover the group at completion. Original secondary PID/slot/amount must match full frozen awards; recipient recovery uses its own receipt/save, with `acknowledge=false`. In-world linkdead PCs remain findable by PID; an absent recipient is not fabricated. [quest.c260](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L260), [1584](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L1584), [3112](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L3112) | Final-kill eligibility/group dispatch is runtime world-quest logic. The item callback's original actor binding is only PID/task/target plus actual carried UID, not the static group's durable entitlement set. No inference from one recipient's callback to other players' saved state. |
| Obligation ACK versus pair retirement | Recovery submits obligation ACK only after supported/tracked effects, callbacks and required save; submit failure is not a durable success, and the in-memory attempt is erased. SQL ACK DML guards unapplied entitlements; it does not itself reread every item/coin/history witness. Strong exact reader and native retirement supply further original receipt/literal/pair checks. Version6 selects its real fee owner. [quest.c299](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L299), [quest_reward_obligation_repository.c605](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/persistence/quest_reward_obligation_repository.c#L605), [718](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/persistence/quest_reward_obligation_repository.c#L718), [quest_reward_obligation_pipeline.c148](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/persistence/quest_reward_obligation_pipeline.c#L148) | No equivalent world-quest business obligation ACK/pair retirement is present in the inspected path. Item-owner terminal state only discharges its own item obligation. Financial service restitution and original native journals retain their separate owners. |
| Lost reply / cold recovery | Native drive checkpoints started/returned boundaries and refuses to repeat an interrupted started call; returned means handoff. Original unACK obligation/recipient entitlement loading provides independent recovery. Player materialization invokes it only when item/pet/recovery loading is not degraded. Flatfile has separate catalog/receipt authority, not SQL proof by analogy. [quest.c3035](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L3035), [3126](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/world/quest.c#L3126), [player_load_materialize.c908](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/player/player_load_materialize.c#L908), [flatfile_item_repository.c2240](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/src/flatfile/flatfile_item_repository.c#L2240) | Successful reset normally makes repeated old callback fail target match; do not claim unconditional double award. Physical retry does not reconstruct lost business context across process death. A captured decoder result or receipt alone cannot activate a cold native world or recreate an extracted original participant. |

## Source-supported partial states and remaining owner work

These are consequences of the inspected ordering/predicates, **not newly executed
fault results or proof that every cut is reachable in a native schedule**:

1. Static item publication can precede required player XP save and obligation
   ACK. Zone-story history is attempted before reward dispatch. History success
   therefore does not imply all item/coin/XP work succeeded; item success does not
   imply tracking/save/ACK/pair cleanup. Preserve each original identity and hold.
2. Group owner item/coin work and own XP may be saved while a secondary entitlement
   remains unapplied. SQL ACK's `NOT EXISTS` guard must keep the original obligation
   pending. A peer's receipt cannot discharge a different PID/slot/amount, and the
   secondary recovery attempt cannot ACK the owner's obligation by itself.
3. Generic recovery lookup permits an in-world linkdead PID; native frozen handoff
   separately requires the original runtime actor. Extraction, a recreated actor
   with the same PID, current NPC reset birth and decoder-valid context are not
   interchangeable. D1 publication is explicitly excluded by this frozen owner;
   use the existing QP03 reservation for authentic disappearance/reset handling.
4. Dynamic physical publication can succeed before a stale task/target or missing
   carried UID causes business callback refusal. Refusal does not reverse that
   item's durable custody or reconstruct the old task. It also proves no refund.
5. Dynamic interrupted matched business execution could separate XP, epic, history,
   dirty status and reset. SQL history failure is not returned to this callback;
   no original attempt key in that INSERT establishes idempotent history here.
   Completed reset blocks ordinary duplicate callback by target mismatch, but is
   not an exact durable business application receipt after an earlier partial cut.
6. Static ACK enqueue can fail after in-memory recovery is erased; the durable
   unACK row remains the recovery source. Worker retry/lost ACK reply, exact
   original readback and final pair cleanup must be proven through existing owners,
   not inferred from callback return or from absence of a pending memory entry.

Primary integration must supply actual original native participant/world lineage,
real input/reward custody and guarded save/ACK/journal lifecycle observations for
those schedules. This document requests evidence from existing owners, not a new
completion flag, new reward implementation, substitute context or parallel journal.
Plan 5 retains independent audit/restore and whole-pair authority. Final gameplay,
persistence and recovery qualification uses the integrated primary candidate at
the agreed major-batch boundary. No new batch is started for this document.

## One reserved future owned-input slice

Reserve only **captured multi-recipient frozen-XP entitlement agreement**. Existing
[`test_durable_quest_offering.py`](https://github.com/Community-Duris/Duris/blob/f679ee312baccbe077267aedd36fceaa2f97b14f/tests/async/test_durable_quest_offering.py#L727) already covers frozen group recipients/caps,
changed group membership and linkdead delivery; do not add another component
model of those behaviors. Existing owned `legacy_xp.kord` deliberately accepts
only its solo version5/party1/slot2/amount200 fixture. Do not broaden its policy or
rerun the accepted solo Kord journey to obtain a new claim.

Proposed future paths, **not added by this delivery**:

- `tests/async/quest_accounting_prep/quest_reward_xp_checks.py`
- `tests/async/quest_accounting_prep/test_quest_reward_xp_checks.py`

Actual available input recipe at prep parent: the maintained
`quest_reward_continuation_decode` plus owned
`read_quest_continuation.cpp` emits the **full** PID/index/amount `xp_awards` list;
`capture_quest_cut.py` emits owner's original literal obligation and XP mask and
selects entitlement rows by **selected recipient PID**. Separate recipient cuts
are required. A collection of independently timed cuts is not a coherent
multi-player snapshot or a complete census. No shared capture change is made.
The maintained crash driver offers synthetic/QP06 and offering/xp-ack options;
it has no group-setup CLI. Its existing solo invocation is not this future schedule.

An optional assertion could compare explicit original op, decoded full awards,
owner cut and explicitly observed recipient cuts: exact tuple identity, quantities,
recipient mask/disposition, and original ACK remaining pending while a genuinely
observed entitlement is unapplied. It must refuse missing/extra/duplicate/conflicting
tuples, wrong PID/op/slot/amount and unsupported/incoherent observations. It must
report **captured tuple agreement only**, never effective XP, authenticated origin,
save completion or native retirement. Reuse existing cut binding/error helpers;
do not create another decoder or calculate effective XP in this helper.

Manual fixture expectation uses the existing Kord production recipe from the
case/execution records: original Kord29257, distinct29262/29263/29264 inputs,
one29237 reward, C3000, E2500 nominal, no fee, D0, runtime I/C/E order. Freeze both
genuine credited PIDs and original levels. Owner input amount is
`min(2500, original_next_level_xp/10)` and each peer's is
`min(2500, original_next_level_xp)`. Independently record the actual progression
policy and returned/saved delta; the legacy solo200â†’520 delta is only its recorded
Human/rest/difficulty fixture, not a universal rule for a peer or another level.

Required future native setup/schedule, currently **unexecuted**: authentic full
boot/reset and original birth/source/stock; two genuine eligible in-room PCs;
original item handover/admission; retained original continuation and full award
set before business handoff; membership change or secondary disconnect after
admission; real cut after owner's rewards/save but before peer exact XP save;
then actual peer save receipt, obligation ACK, lost reply and two cold recoveries.
Observe original tuples unchanged, owner ACK held until peer application, no
extra item/coin/XP, and existing original pair lifecycle. Linkdead and absent/cold
materialized recipients must be distinguished. Do not manufacture PID, epoch,
birth, UID, save receipt, callback or successful ACK to force a schedule.

Missing execution exports are authentic multi-PC native setup/activation, an
owner-coherent full-recipient observation and actual exact XP save/ACK fault
boundaries with cold materialization. Existing per-PID SELECT cuts and decoder
values do not supply them. Until those inputs exist, only the optional owned-input
agreement could be implemented; **no native journey is reserved as executable**.
No dynamic completion implementation/test reservation is added on this delivery.

## Checks and delivery commands

Run from the preserved prep worktree. Proof files remain on D:, outside Git:

```powershell
python -B D:\Dev\Temp\quest-reward-completion-boundary-20261008\pin.py
python -B D:\Dev\Temp\quest-reward-completion-boundary-20261008\verify.py
git diff --check
git add -- docs/persistence/economy_accounting/quest-prep/QUEST_REWARD_COMPLETION_AUTHORITY_BOUNDARY_2026-10-08.md
git commit -m "docs: define quest reward completion authority boundary"
git push origin codex/accounting-quest-prep
git rev-parse HEAD
git ls-remote origin refs/heads/codex/accounting-quest-prep
```

Verification authenticates public blobs/tree comparison, immutable source anchors,
reused prep inputs, local document links, one-file ownership and design scope.
Result: **PASS** for31 public bodies,48 immutable source anchors,6 preserved local
links,10 prep dependencies,24 design/scope assertions and the three public
source/migration tree comparisons. `git diff --check` also passes. The first
anchor-check run used an incorrect token for the XP encoding excerpt; the check
was corrected to the actual `awards_written` expression. This was a documentation
verification correction, not a quest behavior failure or component result.
The D: `verification.json` and `artifact-index.json` give exact check counts and
SHA256s; `publication.json` records containing commit/remote equality and final
document hash. No compiler, component, database, server, journal or native journey
is executed for this delivery. Existing passing components and legacy journey
results retain their original pins and limits. Import this document independently;
it requires no production/helper/test adoption and makes no private candidate claim.
