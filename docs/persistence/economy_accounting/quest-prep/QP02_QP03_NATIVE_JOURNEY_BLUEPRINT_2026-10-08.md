# QP02/QP03 native journey blueprint — 2026-10-08

This is a source-verified implementation blueprint for two genuine SQL-first
journeys, not executed native acceptance. It supplies setup constraints, exact
call sites, cut boundaries and assertion changes beyond the existing
[CASES](CASES.md), [capture execution](CURRENT_CAPTURE_EXECUTION.md) and
[owner reassessment](OWNER_INTERFACE_REASSESSMENT_2026-10-07.md). Existing
component and legacy evidence retains its original scope. The continuing native
Goal remains BLOCKED; this finite preparation neither resumes nor completes it.

Only this document is added. No production, test, driver, schema, registry,
manifest or finish-plan edits, builds, DB/server operations or native runs belong
to this bundle. Primary owns the authentic initialized-world/activation,
publication, retirement and shared journal interfaces; Plan5 owns independent
audit/restore. Future test edits require their own bounded reservation.

## Pins and source provenance

| Input | Exact pin and disposition |
| --- | --- |
| Existing prep branch before this document | `9c14df4183e6633b0a6e99b563dc6b7451792817`; all published work preserved |
| Native accounting source inspected | `a6aec4c4a058a5174e5b679708103762fa05e05a` |
| Latest fetched primary/coordination head | `4b5b90646de108ef825a825937a424ac0dd71434`; only finish-plan/coordination documentation differs from the inspected pin |
| Maintained server and migration trees at both primary pins | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; also unchanged from reassessment pin `911e5789f8a18186181f78fafef9b4fd0155369d` |
| Original accounting base | `17c033d69316b21da8598791fc95cae79baa8dc2`; historical behavior is not the current native owner |
| PR #678 research revision | `55905eac1906cf59405764407f9d22497cccfff3`; Gagga Jobo and Pinehollow dossiers identify the native producers below |
| Producer verification | `areas/AREA`, both QSTs, both ZONs and both MOBs below are byte-identical between research and inspected accounting pins |
| Historical pack metadata | `SOURCE_FACTS.json` retains `275df7f626e12cb396a22da34317a4e7f355e9a1`; that metadata is not this document's binary/source pin |

All `src/`, production area, shared-test and primary-handoff line numbers below
refer to Git contents at `a6aec4c4a058a5174e5b679708103762fa05e05a`, not the
prep worktree's older production files. Owned helper line numbers refer to
`9c14df4183e6633b0a6e99b563dc6b7451792817`. Inspect with
`git show <pin>:<path>`; do not build the prep checkout and label it primary.

### Exact source index

| Path | Git blob | Relevant entry points / lines |
| --- | --- | --- |
| `areas/AREA` | `a7fa6022823563d9c1c6ad1995dc540d85fe1c6d` | Pinehollow86, Gagga Jobo95; production catalog input |
| `areas/qst/goblincave.qst` | `84bacb360972500dcaea69641ce4478446660f29` | leatherworker19005: shoes26–31, shirt32–38, backpack39–45, gloves46–54 |
| `areas/zon/goblincave.zon` | `2ae6c4d6d89c5e3711c7019081aaac3822028f4f` | recipient M49/G50–53; actual hide M/G producers68–86 |
| `areas/mob/goblincave.mob` | `fbd9d7c502fb6413f99e78e62a41206ac7cc33c9` | #19005 at92; aliases93; prototype cash108 |
| `areas/obj/goblincave.obj` | `da7482e5bd43237f682605a78927f1a284f3060c` | exact hide/reward kinds and command aliases |
| `areas/qst/pineholl.qst` | `4d9ca3564469a6baa480308e80ab5622fea4e567` | #16006 at54, original completion69, R78–79/G80–82/D83 |
| `areas/zon/pineholl.zon` | `76965dcc42965027c2f90ea98ab1a85ac313f476` | Auriam M258/G259–260; warrior M265/E266–267; shaman M444/G445 |
| `areas/mob/pineholl.mob` | `0b1adf33de29adc3e79ae19849e1bfdaf8aa50b6` | #16006 at108; aliases109; prototype cash125 |
| `areas/obj/pineholl.obj` | `3a4f6aab0b11b72a78605e08634bf07bb77754a4` | exact sword/armor/totem and reward prototypes |
| `src/specs/specs.assign.c` | `a0f1724469d4dd4c72d276ace11e03127fd3738d` | `assign_mobiles`: `boot_the_quests`1265 then `assign_the_questers`1268 |
| `src/cmd/interp.c` | `74281b6f20ccd88a588d63cdda143da275258e85` | real room recipient `qst_func` dispatch2836–2837 |
| `src/world/new_events.c` | `4bab396e1c0f60b080d7ac1f17b0fbb801be3815` | full boot reset2091; preserved-world boot exclusion2073–2088 |
| `src/world/db.c` | `da996337d17da9a0f81016e3f4c077405fd4fdee` | special assignment689; `reset_zone`6938; active gate6960; M7215/constructor7255, P7435, G7500/E7620, finish7918 |
| `src/world/quest_mobile_native_birth.c` | `cba793daac270b1c9fefd4b818139c5597e75025` | `begin_reset`591, `prepare_mobile`642, source674, item781/carry850/equip859/nest868, `original_object`897, `seal_mobile`939, `finish_reset`1029, publication2103, birth ACK2537/retire2547, pulse2788 |
| `src/world/quest_mobile_native_reference.c` | `f0308d6b79912af6e1b9233307b66ee66cc76497` | valid53 / encode64; immutable birth identity and current revisions are separate |
| `src/world/quest_mobile_native.c` | `6c84016ab5351f00b4a7f22fe8a10db979d1e671` | native cost before/after cash660–675; current mobile/stock advance677–679 |
| `src/persistence/quest_mobile_native_origin_sql.c` | `60a16a46a37f2ac1aca79a9a35f38684de6d142c` | `quest_mobile_native_origin_sql_lock`184: historical carrier200/auth214, current lock221, immutable reread230 |
| `src/world/quest.c` | `4ee99c6c1020250395cba9bdfa3e47f42a642cfe` | `quester`1885/GIVE1947–1986; loader2036/prepend2118,2166,2216; bind2276; `prepare_original`2440; frozen owner2868–3160/`retire_completed`2902; `finish_child`3970; `freeze_program`4657; gameplay begin4748/pulse4845; native submit5265 |
| `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` | consumption5398/cash check5450; runtime observe6248; `native_publish`7668/D refusal7694; physical effects7789–7968; native publish8245/observe8323/take8363/`observe_give`8400 |
| `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` | `publish_native_quest`5627; literal/hold/revision identity5660–5678; publication+census5704–5709; coordinator ACK5712 |
| `src/persistence/economic_sql_item_transfer_transaction.c` | `2da2b2b11979424e339389db89e9b7b6a981056c` | `economic_sql_native_quest_lock_publication`1673; exact borrowed SQL proof, not an independent test transaction owner |
| `src/economy/native_quest_cost.c` | `439ceade2196d33ac60e8428a12fda2f19564837` | `native_quest_cost_project`65; actual denomination/change calculation24–61; charged cash revision106 |
| `src/economy/item_transfer_accounting.c` | `b7012f63c3ce96573eaf23c0424d6447f71e74ac` | `item_native_mobile_cost_accounting_effects`456; native mapped wallet464–474, before/after493, sink497, postings524/527 |
| `src/cmd/actobj.c` | `a2114fddb6816f1534488ff11457a1001d47a14c` | `do_give`6233; numeric coin syntax6246–6256; real money acceptance6339; single object6375 |

The catalog check used the repository's `scripts/zone_story_quest_catalog.py`
(blob `91d55a189c1bb1d1aa413cbebde9107746425a3e`) with production `areas/AREA`:
2,668 definitions, valid, zero diagnostics. It verifies static definitions;
runtime identity, funds, birth, physical effects and ACK require the real owners.
Dynamic bartender quests remain outside this static catalog and this two-case
blueprint; their existing QP04/QP07 pack is preserved.

## Common native setup and observations

Use an isolated integrated primary candidate, full production areas, disposable
loopback SQL schema, test-only account and genuine maintained migration/lifecycle
installation. Record source commit, ELF SHA256, both schema manifests/history,
actual lineage/current epoch, backend/version and complete effective runtime
paths. Use D: for new scratch/evidence/build outputs; in WSL use `/mnt/d` and
`BIN_ROOT=/mnt/d/Dev/Builds/Duris/quest-native-<batch>/bin`. Preserve required
runner layout, flags and deadlines; direct-mount D: into Docker. Existing
historical C: evidence and jobs remain in place.

The executable preparation helper `prepare_fixture.py:14,61–95 --layout world`
calls maintained `run_world_quest_dual_backend.setup_run_root:190`, linking the
production `areas` and copying isolated `lib`, journals and TLS. It creates no
account, schema, active epoch, UID, recipient, source or receipt. Its world mode
rejects reward/supply overrides. Mini mode relocates recipients to room22800 and
uses synthetic O ingredients; retain that legacy calibration, never use it as
native birth proof. `run_static_execution.py:23–32` rejects coin/XP/TYPE terms
and uses the mini legacy path. It cannot execute either journey described here.

The smallest currently callable setup commands, **specified, not run here**, are:

```text
python -B tests/async/quest_accounting_prep/prepare_fixture.py --case QP02 --layout world --output D:/Dev/Temp/quest-native-QP02-<batch>
python -B tests/async/quest_accounting_prep/prepare_fixture.py --case QP03 --layout world --output D:/Dev/Temp/quest-native-QP03-<batch>
```

On a runnable candidate, extend the existing world run-root/boot/client helpers
additively in owned tests; preserve the crash driver's synthetic and QP06 defaults,
deadlines, fault assertions and backend choices. Its existing CLI supports only
`--quest-case synthetic|QP06` and `--fault-phase offering|xp-ack`; a QP02/QP03 or
native ACK option does **not** exist. The SQL lifecycle harness's
`verify_synthetic_routes:57` / `activate_verified:559` is a component seam, not a
genuine initialized-world installer. Do not copy that activation into a journey.

The authentic path to observe is full boot special binding → initial
`reset_zone(j,2)` → native reset admission → actual M constructor and G/E/P
construction/placement → original birth receipt → world publication → original
publication ACK → input acquisition → interpreter `qst_func` → `quester`
active-SQL GIVE → `quest_native_gameplay_owner::begin/pulse` → acceptance and real
GIVE hooks → frozen original branch program → selection → original consumption
child → held publication → frozen reward continuation → exact reward ACK → paired
parent/child retirement. Record each actual original operation relationship;
do not call the selector/publisher directly to simulate this path.

For every cut retain both SQL and the actual owner-supplied native observation:

- Original runtime ID, native instance ID, encoded birth reference, birth operation,
  reset invocation/source generation, zone/room/slot, constructor/build digest,
  original G/E/P forest and birth ACK. Source slot is actual `cmd_no`, not the
  dossier's text line. Instance/UID allocation is genuine, never fixture constants.
- Full UID forest: root/parent, kind/VNUM, equipment slot, literal payload,
  item/current-owner revisions and owner identity. Include selected inputs,
  unselected recipient stock, acquired spares, descendants and reward UIDs.
- Current native mobile/stock/cash revisions and all four signed denominations;
  original authenticated wallet mapping identity and lineage. A wallet mapping ID
  is not the native instance ID, even if their values happen to equal.
- Original acceptance/child command bytes, frozen program/continuation,
  terminal result bytes, inbox/root/children, source claims, effects/postings,
  ownership events and outbox/publication evidence. Keep exact original IDs on
  retry; terminal execution result, physical publication and reward ACK are
  separate facts.
- Actual held player save revision/body pair and ownership generation, native
  effect started/returned journal state, original parent/child attachments and
  paired transition outcome. A SQL SELECT reader supplies none of these live
  ownership rights. Absence of a mobile/frame/obligation is not a successful ACK.

`capture_quest_cut.capture:49` is reusable SELECT-only RR capture, bounded to
2,048 rows per selected query and 16MiB encoded output with BLOB preflights and
confirmed rollback. Pass actual mobile IDs and explicit original operations.
Its current schema projects player cash/XP/task/history, custody/events, native
images/origins, economic receipts/effects/postings and reward obligations. It
does **not** capture live world/journal/holds, player save revisions, actual
mapping rows/current account balances or the lineage-head pointer. Its
`currency_ledger` query selects only the player PID. Its item selector uses
player ownership, case VNUMs or current NPC ownership: destroyed descendants of
other VNUMs can fall outside a later cut. A later additive capture reservation
must preserve the observed UID forest across cuts and add owner-authenticated
same-cut money/revision metadata with existing bounds; no generated rows, budget
relaxation or copied authority codec. Continue using maintained mobile grammar.

## Journey 1 — QP02 paid/unpaid original recipe order

### Production terms and reachability

Recipient is the original leatherworker19005, room19008, zone190, aliases
`goblin leather leatherworker`; M limit1, four separate reset G stock roots
19007/19008/19009/19010. Those stock UIDs are not quest rewards. Hide19006
(`chothe hide strip`) comes from actual livestock19000 M/G pairs in rooms
19018,19019,19020,19021,19022, global cap6; bone19011 is a different kind.
Preserve each actual producer birth/stock/acquisition chain through the native
loot/custody route. That route must be demonstrated by the integrated owner;
`get` prose or direct inventory seeding supplies no chain.

| Runtime completion order (loader prepends Q) | Required loose NPC roots / fee | Newly issued reward |
| --- | --- | --- |
| 0 gloves | 4 distinct19006 + C10000 | 1×19010 |
| 1 backpack | 3 distinct19006, no fee | 1×19009 |
| 2 shirt | 2 distinct19006 + C2000 | 1×19008 |
| 3 shoes | 1×19006 + C1000 | 1×19007 |

Each duplicate is an exact root identity, not an aggregate prototype count.
G also prepends: gloves begins with C10000 before its four ITEM goals. The
current preliminary pass reads genuine NPC cash metadata once, returns
`not_matched` for insufficient funds or an ingredient shortage, and `refused`
for invalid/unavailable cash authority. ITEM pass then TYPE pass selects roots
without reusing an object; frozen goal order and partial-prefix semantics stay
unchanged (`quest.c:2491–2582`). Pulse advances only `not_matched`; a refusal
blocks that original operation (`5179–5185`). The old blanket paid-refusal
defect has been replaced by current cash authority. A current paid test must
exercise that owner, not expect the old refusal unconditionally.

**Native setup constraint:** prototype19005 starts with `0.0.0.0` cash and no
hide G stock. Actual born/current cash still requires constructor/current-image
proof. Giving four hides sequentially does not establish a four-hide gloves
selection: funded at C1000 or more, the first hide can complete shoes; unfunded,
the third completes backpack. Numeric GIVE is coin syntax, not bulk item GIVE
(`actobj.c:6246–6256`); the quest gameplay owner accepts one offering and excludes
overlapping pending operations on that NPC. Nested hides do not count as loose
roots. Money acceptance is a separate path; it is not evidence of an item-triggered
recipe evaluation. Therefore S4 below is a required **target preselection cut**,
not a demonstrated reachable fixture. Primary must provide a genuine acquisition/
handover chronology that reaches it under the unmodified production quest family.
Do not suppress intermediate evaluations, temporarily replace Q records, invent
batch GIVE, load four roots directly or retain a synthetic O supply to claim it.

### Stepwise execution design

1. Boot through the common authentic setup. Capture original L birth and
   post-publication stock/cash (Q2-B0), and the player's real funded/acquired
   holdings. Ensure no other character/tasks give L inputs. Resolve L by its
   authenticated instance/runtime reference, not only room/VNUM/name.
2. In an unfunded recipient run, acquire at least three distinct hide UIDs by
   the genuine production route; retain an additional acquired hide on the
   player as a spare. Record the full before forest. Arrive at19008 using the
   candidate's existing test-account travel facility, retaining actual birth
   place/source. `give hide leatherworker` one root at a time, recording the
   selected visible ordinal's UID before each command. Wait for each actual
   acceptance/recovery owner to settle before the next command.
3. Capture first/second acceptance (Q2-A1/A2): hides are L-owned, no consumption
   child or new reward; no expense, player money or XP change. On third acceptance
   (Q2-S3), gloves lacks funds; backpack selects exactly the three eligible roots.
   Separate the acceptance cut from consumption so a legitimate transfer is not
   misreported as an unchanged spare. Capture Q2-C3 committed receipt and Q2-P3
   physical publication, then reward Q2-R3, actual ACK Q2-K3 and pair cleanup Q2-T3.
4. In a separate run reserved for the paid branch, first obtain the missing
   authentic S4 route above. Record four loose19006 roots in L, genuine current
   NPC cash worth at least10000, and the final triggering acceptance/receipt.
   If player money-GIVE funds L, capture it as its own operation and wait for
   its publication/ACK. An exact10000 payment is `give 10 platinum leatherworker`
   only when the real player's original denominations support that command;
   do not seed L cash or infer the actual vector from copper value.
5. At Q2-S4, assert gloves precedes backpack even though both item quantities
   match. Consumption selects exactly four19006 UIDs, charges C10000 against
   L's actual wallet mapping, and freezes reward19010. Capture C4/P4/R4/K4/T4
   separately as in the unpaid run. With more eligible roots, unselected roots
   remain with their genuine original owner. Reset-stock gloves UID remains
   distinct from the fresh quest-issued gloves UID.
6. At the supported fault cuts below, retain original IDs and perform recovery,
   then two cold boots. Repeated recovery issues no additional backpack/gloves,
   debits no second fee, consumes no spare and does not borrow a later L birth.
   After actual ACK, optionally move the original reward through a genuine
   ordinary player move and verify historical ACK/pair cleanup still succeeds
   without requiring the old current player image.

### Exact expected money, ownership and evidence

For S3, three roots become destruction-owner8 tombstones under one consumption
operation, with matching immutable events/accounting references. One new19009
root is uniquely player-owned with its actual creation source; original reset
stock and player spare remain. There is no fee, coin reward, XP or D in this
branch. Acceptance legitimately advances the NPC stock/current body; compare
unchanged stock items individually, not an unchanged whole NPC image.

For S4, four roots retire once, one new19010 issues once, and the genuine NPC
cash vector changes by the maintained `native_quest_cost_project`, total
value −10000, charged cash revision +1. Preserve exact change denominations;
do not substitute a scalar debit. The economic native-wallet effect uses
context12 and authenticated **wallet mapping ID**, `quest_cost`/`quest_action`
and the requirement sink; corresponding postings sum to zero. This fee is
not a second player-wallet debit. Any earlier player→NPC funding is a separate
balanced operation with both native vectors/revisions and its own original
receipt. Native/body/custody and accounting projections must agree at publication.
No acceptance result alone proves successful cost publication or reward ACK.

The accounting requirements exercised are R1 exact original operation/replay,
R2 denominations/mapped cash lifetime, R3 expense/issuance source policy,
R4 exact UID/custody/tombstones, R5 existing item-plus-cost boundary and R8
restart/lost-reply qualification. Use Plan5's existing R7 output where needed;
this journey does not implement another independent auditor.

## Journey 2 — QP03 original Auriam, D and later replacement

### Production terms and identity

Auriam16006 (`dragon gold auriam`), room16077, zone160, has M cap1 and original
G16016 broken lance + G16015 gold scale. The completion consumes exactly one
each16013 black sword,16014 black platemail,16080 dark-spirit totem; newly
issues16015 and16075; no C or XP terms; D is true. Rewards prepend during load:
frozen receive order is16075 then16015; input traversal is16080,16014,16013.
Record actual selected root order and continuation slot IDs; payload item arrays
may be UID-sorted separately. Original stock16015 cannot become the reward merely
because its VNUM matches. Prototype cash row is `6.6.9.22 486000`; actual current
four-denomination cash/revision comes from authentic construction/current custody,
not conversion of prototype text or an assumption that retirement cash is zero.

Inputs come from cruel warrior16005 at16081, M265/E16014/E16013, and dark
shaman16087 at16161, M444/G16080. Preserve the exact source NPC births,
equipment/container topology, genuine defeat/loot/transfer receipts and current
player ownership before handover. Production room O origins do not supply these
E/G births. A native acquisition route for these items is still an owner input.

Define A as the original authenticated recipient and B as a later actual reset
birth with the same VNUM/room. Require distinct instance ID, birth operation and
complete reset source (new invocation/generation; the same M slot is legitimate);
current room and prototype are not attempt
identity. Historical A birth remains byte-identical through moves/retirement;
current A cash/stock/mobile revisions legitimately advance. The ordinary M cap1
does not establish cold adoption correctness: the initial reset passes force2
and the reset code has an explicit force override. Never force-spawn a second
Auriam beside A to satisfy a two-actor component seam.

### Current hard boundary

`item_native_quest_publication_owner::native_publish:7689–7695` explicitly
returns false for successful completion whose frozen terms require D, because
the original whole-stock/cash/lifetime retirement owner is required. This is a
**publication refusal**, after the receipt may already say applied/already_applied.
It does not prove no economic commit: inputs may already be SQL tombstones and
the reward obligation may exist while the native forest remains before-publication
and held. Capture both cuts and preserve the original hold. Do not assert global
unchanged state, successful rewards or ACK from the refusal. Generic held-retirement
lockpick code at8451 onward is not the quest D owner and is not a substitution.

The full journey below is implementation-ready once primary supplies that real
owner and initialized-world/acquisition hooks; the current candidate can at most
qualify the genuine committed-held/refusal prefix if authentic setup is available.

### Stepwise execution design

1. Full boot creates A through original M/G birth and publication. Capture
   Q3-B0 including A's complete stock forest, current cash/mapping/revisions,
   immutable source, original birth result and ACK. Acquire and capture the
   three inputs on the actual test player; exclude accidental gifts to A.
2. Arrive at16077, resolve A's actual reference and issue `give sword auriam`,
   `give platemail auriam`, `give totem auriam`, waiting for each real acceptance
   to finish. Capture before/after each acceptance. The first two incomplete
   handovers must not issue rewards or retire A. After the third, Q3-S has the
   exact three selected loose roots and complete residual original stock/cash.
3. Freeze the actual parent/consumption child and D decision. Capture Q3-C,
   the original committed receipt, tombstones/root references and frozen
   two-slot obligation. At the current D publication refusal, capture Q3-H0/H1:
   unchanged physical A/input forest across stable retry, original held player
   body and both attachments, no successful publication ACK or pair deletion.
   SQL and physical divergence is a held condition to resolve, not success.
4. When the authentic D owner is integrated, observe its actual phase ordering.
   Capture Q3-P (selected input removal/publication), Q3-R (both distinct fresh
   rewards), Q3-D (whole residual-stock destruction, exact remaining cash sink
   and A live→retired), Q3-K (actual obligation ACK) and Q3-T (pair cleanup).
   These labels name evidence conditions, not an invented sequence/transaction.
   If the owner atomically combines conditions, use its actual cut and assert
   the combined receipt; do not fabricate a D-only intermediate state.
5. Only after A's actual terminal disposition permits normal reset, observe a
   genuine later reset birth B. Record B's own source generation, constructor,
   new stock UIDs, cash mapping/current image and publication ACK (Q3-B1).
   Keep A's terminal evidence available. Capture B-before/B-after recovery of
   A's original pending/replayed command (Q3-X0/X1), not a new player GIVE to B.
   Native runtime IDs may change at cold recovery; durable A identity may not.
6. Run the chosen recovery cuts and two cold boots on the integrated batch.
   Verify A is not resurrected, B remains its own lifetime, rewards keep their
   original UIDs, no residual stock/cash is taken from B and no original D,
   issuance or ACK is repeated. An ordinary move of a published reward before
   historical pair cleanup must remain compatible with the current owner at
   `quest.c:3016–3018`; current reward location is not the immutable receipt.

At Q3-D, all actual residual A roots **and descendants**, including the original
16015 and16016 if still present, have exact destruction tombstones/events under
the genuine D operation. No live native A stock remains. A's current cash vector
is zero with its real terminal wallet effect/counterparty and balanced postings;
expected debit is the captured current vector, not a fixed prototype amount.
Cash/mobile/stock revisions follow the authentic owner result. B has a distinct
birth and mapping; A's cleanup cannot debit or destroy any B holding. Require
the original birth carrier and exact original terminal result/service evidence
after native journal retirement; actor or frame absence is insufficient.

Both rewards must be new UIDs with one original creation event each, uniquely
player-owned after publication, linked to the frozen continuation slots and
actual source/receipt. No coin reward, XP payment or reclassification of A's
preexisting scale is allowed. R1/R2/R3/R4/R5/R8 apply to the actual consumption,
reward and D boundaries; this does not create a new compound transaction across
separate owner operations or replace Plan5's R7/restore qualification.

## Fault cuts tied to maintained owners

All rows are proposed fault experiments, **not executed hooks**. A debugger
breakpoint alone is not an observation. The future runner must stop on the exact
original operation/actor, record the actual owner state and receipt, confirm the
cut, kill only its isolated process, then recover with unchanged journals/schema.
Keep existing monotonic deadlines and bounded waits. If a boundary cannot be
observed without a new shared hook, report that row unavailable and run another
supported cut; do not treat missing symbols, timing guesses or a timeout as a hit.

| Cut | Maintained boundary / exact future observation | Required recovery result / present limitation |
| --- | --- | --- |
| F1 acceptance committed, reply lost | Completion handler `item_movement_transaction.c:8004` and actual held-body acquisition8125 are pre-effect observation candidates; require original command and independently confirmed SQL committed result before native effect/ACK. `observe_completed:8323`/`take:8363` and pulse4911–4946 are post-publication handoff indicators:8340 requires ACK, so they cannot stand in for this cut. | Recover same original acceptance once, correct player→original-NPC custody and revisions, no duplicate item/recipe. Existing `fault_boot:47` has only legacy offering/XP-ACK cuts; native capture/stop extension and actual lost-delivery boundary are missing. A completion-handler breakpoint proves delivery, not that the reply was lost. |
| F2 child persisted, execution reply uncertain | gameplay `quest.c:5223–5256` prepares child, persists child handoff before submit, distinguishes known-unavailable/uncertain; SQL commit may precede completion delivery. | Reuse original child/receipt. No fresh operation/source/root, duplicate expense or second tombstone. Need shared journal/held command observation; a before-submit cut alone does not prove lost SQL reply. |
| F3 committed child, before native publication | `native_publish:7680` exact sealed receipt before effects; QP03 successful-D refusal at7694 is an explicit current boundary. | Observe committed SQL and retained physical before state/hold separately. QP03 remains held with no ACK until D owner exists. Do not apply global `unchanged` to precommit→committed cuts. |
| F4 physical effect started, return uncertain | native acceptance detach `recovery_begin(state,1):7789`, `obj_from_char:7796`, returned7798; consumption root effect7899/extract7909/returned7911. Registry stage3 at7924 and binding stage4 at7953. | Started/unreturned is uncertainty, not permission to repeat. Capture authentic journal preimages and full native forest. Cold owner must prove original effects or retain hold. Native effect injector/readback is unavailable in current quest driver. |
| F5 frozen reward recovery, lost ACK | frozen drive `quest.c:3079/3086` journals original effect begin/return; recovery call3112; exact obligation probe3146–3159. Reward constructors1525/source1542/actual creation submit1561. | No second reward UID/creation or fee. Preserve byte-identical original continuation and original reward source. Actual held publication/SQL ACK probe needed; legacy XP-ACK does not establish this cut. |
| F6 exact ACK present, pair retirement uncertain | `retire_completed:2923` copies original parent, validates pair2936, re-verifies parent+child2979–2985 and exact obligation ACK3002; `transition_pair:3021` sets latch only on success3024. | Both full preimages survive uncertain journal I/O. Eventual pair cleanup once; missing parent is not proof. Later player/reward moves remain legitimate. Need original pair/envelope observation and stop around actual journal write, not deletion of files by the test. |
| F7 QP03 D/replacement cut | Current refusal7694 is observable; successful whole-stock/cash/lifetime D publication and its post-return cut have no supplied owner/hook. | No native success claim until owner provides exact function/receipt/terminal-service phase. Observe real later B and replay A; no fake simultaneous reset. Retain committed-held evidence while D is unavailable. |

QP02 S3 is the first native execution priority once common setup works; its
unpaid branch avoids the missing S4 chronology. QP03 committed-held prefix is
next only with genuine inputs and holds. Paid S4 and full QP03 D/cold replacement
remain dependent on their named owner inputs. Do not rerun broad qualification
on unchanged source or force activation to manufacture these cuts. Final native
qualification belongs at the integrated primary's agreed major-batch boundary.

## Assertion reuse and required bounded extensions

| Expectation | Existing helper and usable scope | Missing native assertion / exact owner input |
| --- | --- | --- |
| Exact inputs, one consumption, spare identity, fresh reward slots | `quest_cut_checks.static_complete:150–207`; S3 and QP03 item portions; `book:96` correlates roots/refs/postings | S4 cannot pass unchanged complete helper: its money check208–213 expects −G coins on the **player**. Add a native NPC-cost assertion with genuine mapping/current cash and separate funding interval; preserve legacy default. Capture selected/spare forests at the appropriate post-acceptance cut. |
| Recipe order / no premature result | current `test_native_selectors.py` covers constructed owner selection; source branch order verified above | Native driver must observe original accepted stock, frozen branch index, child/result and no earlier consumption on A1/A2. Component-created stock does not prove S4 reachability. Primary supplies authentic S4 chronology. |
| Actual NPC birth and current revisions | `mobile:296` validates maintained image grammar/header; capture origins157–168 | Authenticate original carrier/receipt via maintained origin owner, then current image/mapping under the real same-cut borrow; add live birth ACK, forest and constructor/source observations. Historical bytes alone do not establish current custody or grant publication. |
| Native cash and economic conservation | capture effects210/postings214; `book:96`; `money:142` checks player ledger only | Compare exact native before/after vectors/revisions, authenticated mapping/account effects and root counterparties with real physical publication. Include actual account/current-book and lineage-head rows through owner interface; never infer mapping from native UID. |
| Original A terminal and replacement B untouched | `retired:310` checks residual stock/cash, birth identity, terminal events and mapped wallet | Existing helper requires A and B in **before** metadata and assumes a D-only cut. Add a temporal native assertion over A-before, A-terminal, later B-birth, B-before-stale/B-after-stale and cold cuts. Preserve component contract; primary supplies actual D transaction/receipt and terminal service body, plus reset/adoption chronology. |
| Stable committed-held retry | `held:375` checks stable SQL replay, unacknowledged obligation and committed receipt; `replay:253` | Use H0→H1 after commit, plus actual original held bodies/attachments/effect state from shared owner. It cannot compare native world or prove permission to retry a physical effect. |
| Actual reward ACK | `acknowledged:388` checks original durable receipt, continuation and acknowledged obligation; CLI `--check ack` compares frozen bytes | Add real publication/held-body/owner ACK evidence and unchanged reward identities. SQL ACK assertion is necessary but not paired journal retirement. Neither journey has XP; do not borrow QP06 XP proof. |
| Pair cleanup after advancement | `later_move:259` covers original reward movement; frozen owner source allows historical cleanup | Need actual parent/child retained envelopes, receipt equality and successful paired transition. Empty directories, no NPC or a callback return do not satisfy it. |
| No effects on cold/repeated recovery | `replay:253` on suitable stable SQL cuts, existing two-boot journey patterns | Add full actual native census/hold/journal comparison and terminal evidence. Legitimate new B birth is a separate interval; compare B only after that birth settles. Do not require every global table unchanged while normal reset legitimately runs. |

Example existing SQL capture invocation for a future confirmed disposable batch
(placeholders must be populated from actual owner evidence; **not run here**):

```text
python -B tests/async/quest_accounting_prep/capture_quest_cut.py --case QP03 --database <accepted-generated-quest-schema> --pid <actual-pid> --mobile-instance <A-instance> --operation <acceptance-op> <consumption-op> --lineage <actual-lineage> --epoch <actual-epoch> --server <actual-ELF> --server-sha256 <actual-SHA256> --source-commit <integrated-source-SHA> --output <new-D-evidence-path>/Q3-C.json
python -B tests/async/quest_accounting_prep/quest_cut_checks.py --case QP02 --check complete --before <Q2-S3.json> --after <Q2-R3.json> --selected <three-actual-UIDs> --rewards <new-backpack-UID> --spares <actual-spare-UIDs> --reward-vnum 19009 --original-mobile <L-instance>
```

Capture requires `TEST_DB_DISPOSABLE=1`, `TEST_DB_HOST=127.0.0.1`, credentials
from the isolated runner and its allowed generated database-name pattern. Never
use `--legacy-no-epoch` to bypass a native setup gap. The command is an existing
SQL predicate interface, not an end-to-end native journey command. No current
one-command genuine QP02/QP03 journey exists; the smallest future addition is
an owned adapter using these captures and existing world/boot/client/crash
helpers after shared setup/observation contracts are supplied.

### Exact helper / shared-driver pins

| Revision | Path | Git blob |
| --- | --- | --- |
| prep `9c14df4183e6633b0a6e99b563dc6b7451792817` | `tests/async/quest_accounting_prep/prepare_fixture.py` | `0189ad5260f39e0d5163858c41afbc399936bcb7` |
| same prep | `tests/async/quest_accounting_prep/run_static_execution.py` | `ed2333bfb2adcc24316c7928bacf535f1f00eca1` |
| same prep | `tests/async/quest_accounting_prep/capture_quest_cut.py` | `ecf52e1ec9dbdd0a72e0eb01890f4e099aa5f757` |
| same prep | `tests/async/quest_accounting_prep/quest_cut_checks.py` | `20ec1fa587ee00cfee1a040236df1dff800cb4ad` |
| primary inspected pin | `tests/async/run_world_quest_dual_backend.py` | `bbba7ad362e26207b222c052ce5768d97ff4d65c` |
| same primary | `tests/async/test_static_quest_reward_journey.py` | `f1525a62ea08f7546aadda7c2ff6f09e4a6a011c` |
| same primary | `tests/async/run_quest_reward_ack_crash.py` | `8a8476751a9119ced9e19e25cd1182cb5befdca1` |
| same primary | `tests/async/native_quest_publication_mysql_harness.cpp` | `c92476bfb905eb7cc2a1987c501a2b05f3be5976` |
| same primary | `tests/async/economic_sql_lifecycle_owner_mysql_harness.cpp` | `240489225b4434703807059a97919c9f76eb58d0` |

The publication harness explicitly retains the genuine boot-provenance gap
around296; synthetic publication roots/activation are component calibration,
not setup substitutes. Existing `test_recipient_retirement.py` extracts the
production `observe_give` boundary with census/lookup seams; it does not execute D.

## Reconciliation with private reset / initialized-world contracts

The primary-pinned
`DAY1_ROOM_RESET_IMPLEMENTATION_HANDOFF_2026-10-07.md` blob
`3d0feec9a0c46f6f8803dc3f9ad44cde43195f2e`, lines3–57, and
`ZONE_RESET_ORIGIN_PLAN5_INTERFACE_2026-10-07.md` blob
`ec633a3d78205bea6275ac5b4b51fd1c9ae34cbf`, lines128–200, report private
candidate `tmp/lifecycle-npc-money-candidate-primary-20261008`, SHA256
`2b09c85daa4f824b201a5ebbfe7ff21360613f4838f819f390eaca78a76d04b8`:
111 production files,23 original fixtures,5 schema/manifest files. These are
reported source-review/formatting results, **not private bodies inspected here,
published callable APIs or executed compiler/native/SQL/migration/gameplay proof**.

| Reported contract | Consequence for these journeys / boundary still open |
| --- | --- |
| Mobile P chooses authentic constructor chronology, exact current birth/stage/UID/prototype/zone; foreign/incomplete winner refuses without older fallback | Preserve once-constructed choices and full original G/E/P topology; do not use current public `original_object:897` fallback as proof of the private successor. The two recipients' listed stock is G and source warrior equipment E; a room O contract does not establish any of these births or foreign custody. |
| Initialized verifier borrows actual retained SQL/world view synchronously; authenticates historical birth inbox before current mapping locks; moves lifetime metadata only after complete mapping validation | Request the existing owning batch's authenticated borrow/output, not a separately assembled test packet or new transaction. Retain original raw/cumulative limits, savepoint cleanup, failure latch and callback lifetime. Read-only snapshots/DTOs are observations, not publication authority. |
| Current native raw6/body/cash/current-image comparison uses actual native UID, wallet mapping ID, lineage, historical birth epoch and creating operation | Assert exact current cash/revision and full reference; do not equate immutable birth epoch with current active epoch or terminal/current image with original birth image. Missing/duplicate/unknown/originless/unloaded/retired states remain explicit; truncated diagnostics cannot accept an omitted defect. |
| Room O retained ZRO1/private0065 and original terminal-body retention | ZRO1 records committed room issuance; terminal service evidence and native publication/ACK are separate. Room owner3, command22/writer16 and absent-before creation contract are not native owner12 M/G/E quest authority. No private0065 row/table is presumed installed in maintained schema64. |
| Original terminal body retained before native journal retirement; NULL/missing remains unknown; cold owner requires authenticated original locked root and actual terminal service body | Require A's authentic terminal evidence after pair retirement. Do not invent envelope phase/revision/process generation on read, backfill from current holdings or use B to regenerate A. This is an input from the original owner, not another prep persistence format. |
| Admission/adoption/warm world/source/custody, originless opening, activation and major both-backend qualification remain open | Full-world helper plus latest published documents is still not executable genuine setup. The original inactive installation/activation owner must supply exact candidate, command and successful guarded output before dependent journeys run. |

## Next implementation reservation and handoff

There is no independently implementable genuine native journey on the supplied
published interfaces today. Reserve **only after** primary supplies the following
concrete inputs; preparation is complete without waiting for them:

1. Authentic full-world SQL-first installer/admission/adoption command and
   initialized-world same-cut observation contract, with exact candidate/binary/
   schema pins, actual lineage/epoch and original M/G/E birth/source/ACK proof.
2. Genuine native source-NPC→player acquisition route for hide19006 and
   sword16013/armor16014/totem16080, with full UID forest/custody and real receipts.
3. QP02 S4 reachability chronology under unmodified production order plus current
   NPC cash/funding/publication metadata. S3 does not depend on that chronology.
4. QP03 actual whole-stock/cash/lifetime D owner, phase/result/terminal-service
   evidence, post-D normal reset/cold adoption and replacement identity contract.
5. Actual held player/native/effect journal observation and original parent/child
   pair transition observation for F1–F7; no new coordinator ownership in prep.

With those inputs, propose an additive owned native runner/capture/assertion
bundle under `tests/async/quest_accounting_prep/`, first S3, then supported
QP03 prefix, then S4 and full D at the same major candidate cadence. Shared
drivers remain primary-owned; document any required new fault hook instead of
editing them in this preparation assignment. Preserve synthetic/legacy component
controls, established deadlines and both-backend coverage. An implementation
reservation must name exact new files and supplied shared interfaces before code.

### Work actually performed for this bundle

- Fetched `origin/experimental-accounting` normally; final observed head is
  `4b5b90646de108ef825a825937a424ac0dd71434`. No merge/rebase/reset of prep.
- Inspected exact primary Git sources, relevant private-contract handoff text and
  pinned owned helpers using `git show`, `git grep`, `git rev-parse`, `rg` and
  bounded source reads. Temporary inspection helper lives only under
  `D:/Dev/Temp/quest-native-blueprint-20261008/inspect.py`.
- `git diff --exit-code 55905eac1906cf59405764407f9d22497cccfff3 a6aec4c4a058a5174e5b679708103762fa05e05a -- areas/AREA areas/qst/goblincave.qst areas/qst/pineholl.qst areas/zon/goblincave.zon areas/zon/pineholl.zon areas/mob/goblincave.mob areas/mob/pineholl.mob`: PASS, unchanged native research producers.
- `git diff --exit-code a6aec4c4a058a5174e5b679708103762fa05e05a 4b5b90646de108ef825a825937a424ac0dd71434 -- src migrations areas tests/async/run_quest_reward_ack_crash.py tests/async/run_world_quest_dual_backend.py`: PASS, no changed dependencies in this advance.
- `git diff --exit-code 911e5789f8a18186181f78fafef9b4fd0155369d a6aec4c4a058a5174e5b679708103762fa05e05a -- src migrations`: PASS, no new native prerequisite since blocked reassessment.
- `python -B scripts/zone_story_quest_catalog.py --source-root . --production-output D:/Dev/Temp/quest-native-blueprint-20261008/catalog.json --check`: PASS,2,668 static definitions. Script/producer bytes are pinned above; this is source-data validation, not gameplay.
- `python -B D:/Dev/Temp/quest-native-blueprint-20261008/verify_blueprint.py`:
  PASS,33 table blob pins,58 exact function/call-site anchors, all relative
  document links present and catalog terms confirmed (four19005 recipes,
  one16006 recipe). This temporary source/document checker is outside the repo;
  the tables preserve the independently reproducible Git inputs. During review
  it caught and corrected a completion-handler line reference; no native test
  failure or runtime result follows from this check.
- `git diff --check`: PASS; staged one-document diff also checked before commit.

No new component test, build, DB/server or native journey
was executed; earlier results are not relabeled. Import this one documentation
commit independently of old production/test bundles. The commit containing this
file is the result pin (`git log -1 --format=%H -- <this-path>`); final delivery
records its exact SHA and remote publication. Canonical historical `HANDOFF.md`
is preserved unchanged under this one-document assignment.
