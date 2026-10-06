# Prioritized quest accounting acceptance

Every case uses accounting `17c033d69316b21da8598791fc95cae79baa8dc2` and
PR #678 research `55905eac1906cf59405764407f9d22497cccfff3`. Reverify producer
bytes on the integrated primary candidate before qualification. Research tests
are evidence for their own branch only.

Common static dispatch: `areas/AREA` → `make_qst.c` → `boot_the_quests` (Q/G/R
lists prepend) → `assign_mobiles`/`assign_the_questers` sets `qst_func=quester` →
`CMD_GIVE` → `submit_durable_quest_offering`. Reset M constructs the actor through
`read_mobile`, sets birthplace, applies zone modifier/shop binding, publishes to
room and calls the alchemist hook; G/E create inventory/equipment. The published
active reset path refuses fresh item issuance. A reduced mini fixture can test
selection but cannot certify this full birth/dispatch chain.

Common durable assertions: original input UIDs/payloads retire once; each output
has a fresh never-reused UID and one native owner; operation/source receipts,
current owner, immutable events and accounting references agree. Coin terms use
copper (1/10/100/1,000 for copper/silver/gold/platinum). Every money root balances
actual denominations. Loading/staging/cleanup does not mint. Replay repeats no
effect. Absence of a frame or NPC is not ACK evidence. XP has exact save receipts
but no double-entry money posting. Requirements are the original R1–R8 in
`REMAINING_REQUIREMENTS.md`, not additional gates.

## QP01 — P0 exact sapphire kinds and consumed Orb

- Research: `docs/design/zone-stories/LOST_TEMPLE_OF_TIKITZOPL.md`.
- Producer: `areas/qst/tikitt.qst`, crafty magician mobile44101, reset
  `M 0 44101 1 44313 100 ...` in `tikitt.zon`; common static dispatch above.
- Offer one each I43703, I43752, I43753 and I44164; receive one I44192;
  no coins/fee, D0. The three foreign necklaces share display names. Lost City
  `tikit.zon` equips them on worshipers; the Orb is a consumed input, not a catalyst.
- Try three43703+Orb, missing one kind, exact kinds, and exact kinds plus spare
  duplicate. Assert only the selected four roots retire; spare stays player-owned;
  output UID is new. Freeze the selected reward slot before later catalog drift.
- Durable evidence: native input tombstones, quest source/creation receipt and
  output owner/event/root links; replay/lost reply pays no second necklace.
  R1/R3/R4/R8.
- Smallest prep command:
  `python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP01`.
  Mini fixture command: `prepare_fixture.py --case QP01 --output <empty-dir>`.
- Current result: source/fixture verified and extracted selector component passes;
  no native journey qualified.
  Integration hook: primary's authentic native12/birth source and integrated
  SQL journey observations; no new source capability proposed.

## QP02 — P0 overlapping hide recipes and mixed costs

- Research: `docs/design/zone-stories/GAGGA_JOBO_CAVE_SYSTEM.md`.
- Producer: `goblincave.qst`, leatherworker19005 at19008 via
  `M 0 19005 1 19008 100 ...`; carried finished stock19007/8/9/10 is separate
  reset supply. Livestock19000 supplies G19006 hide and G19011 bone, cap6.
- File order: shoes1×19006+C1000→19007; shirt2×19006+C2000→19008;
  backpack3×19006/no fee→19009; gloves4×19006+C10000→19010. D0 throughout.
  Runtime Q order is gloves/backpack/shirt/shoes; runtime gloves G begins C10000.
- Give a hide with exactly three distinct eligible hide roots, then with one,
  two, four, nested/held roots and insufficient/exact fee. Current selector finds
  a matching hide in gloves, marks coins unsupported and stops before backpack.
  Record that RED without accepting a legacy handover or fabricating payment.
- Acceptance: primary's reviewed selection policy must make the ordinary supported
  backpack reachable with exact three roots, preserve unsafe paid refusal, and
  preserve complete paid terms when supported. For paid success, native wallet
  debit/items/reward/source share the intended boundary; balanced fee evidence,
  immutable roots and replay preserve one result. R1/R2/R4/R5/R6/R8.
- Smallest prep command: `test_native_selectors.py --case QP02`; add `--acceptance`
  to reproduce RED (exit1, native component code30) on this pin.
  `prepare_fixture.py --case QP02 --output <empty-dir>` preserves all four Q blocks.
- Current result: source/fixture and pinned-behavior component pass; acceptance
  is RED. No runtime
  fix or native journey. Primary owns recipe policy and mixed-fee hook. Alias
  `leatherworker&n` is also problematic; use actual `leather` numbered target after
  observing room order, not an assumed plain alias.

## QP03 — P0 disappearing Auriam and actual reset birth

- Research: `docs/design/zone-stories/PINE_HOLLOW.md`.
- Producer: `pineholl.qst` mobile16006, `M 0 16006 1 16077 100 ...`, followed by
  G16016 broken lance and G16015 gold scale, cap1 each; common static dispatch.
- Give I16013 black sword, I16014 black armor, I16080 dark totem (one each);
  receive I16015 gold scale and I16075 gold dagger (one each), fee0, D1.
  A retained reset lance16016 is not a third reward. Cruel warrior16005 at16081
  equips armor and sword; dark shaman16087 at16161 supplies G16080. These are reset stock,
  not encoded personal kills.
- Capture authentic birth/source/cash and remaining inventory/equipment forest.
  Kill the process after accepted inputs, after first reward, after D begins,
  after reward ACK and before paired retirement. Remove/move the original actor;
  separately reset a new16006 at the same room. The new incarnation must not
  receive the old disappearance or lend its binding/stock to old cleanup.
- Expected: original three inputs retire once, two fresh player output UIDs,
  original residual lance/scale and cash disposition with explicit native evidence;
  replacement stock remains independently owned. Missing/unproven birth or
  started/unreturned callback retains visible recovery. Original acknowledged
  receipt authenticates terminal pair cleanup without reward repetition.
  R1/R3/R4/R6/R8.
- Smallest prep: `prepare_fixture.py --case QP03 --output <empty-dir>` and
  `test_recipient_retirement.py`. The latter executes actual lookup/D cleanup,
  with reward/tracking/extraction authority stubbed. `--acceptance` is RED
  (exit1, component32): original prototype/room lookup accepts the replacement.
  Full qualification needs primary's real birth/native12
  integrated candidate and fault journey; reduced fixture alone is insufficient.
- Current result: source/fixture and pinned-behavior component pass;
  incarnation acceptance RED; publication/disappearance/birth journey pending.
  Hooks: authentic NBC2/NMB3 source binding, frozen residual forest/cash,
  acknowledged historical receipt reader and exact paired journal retirement.

## QP04 — P0 dynamic bartender charged failure/refund

- Research: `docs/design/zone-stories/QUIETUS_QUAY.md`.
- Producer: `specs.world_quest.c::world_quest`, literal1709 assignment in
  `specs.assign.c`, `quietus.zon M 0 1709 1 1734 100 ...`; shop assignment retains
  world_quest in `SHOP_FUNC`. ASK quest → committed wallet callback →
  `createQuestForGiverVnum` → runtime policy/catalog; absent from static catalog.
- At level11 with normal difficulty and default20/level: fee220 copper;
  no ingredient/reward VNUM on creation failure. Freeze fee/config and deny all
  target availability after fee admission. Test debit rejection, no eligible
  zone, no eligible target and lost reply after committed debit/refund.
- Expected: no created task/item; one exact debit and named restitution tied to
  that original debit or a durable visible held refund; net wallet unchanged after
  resolution, balanced denomination evidence and no duplicate quota consumption.
  Cold recovery must neither repay twice nor claim success from refund prose.
  R1/R2/R3/R5/R8.
- Smallest prep: `test_bartender_settlement.py --case QP04` executes the actual
  callback and ADD_MONEY helper with isolated state. `--acceptance` is RED
  (exit1, component30): active refund credit is not submitted.
  `prepare_fixture.py --case QP04 --output <empty-dir>` reuses the existing
  isolated full-world run-root/TLS/journal setup. Use `run_world_quest_dual_backend.py`
  full-world setup for primary SQL fault journey, not the static mini catalog.
- Current result: source and pinned-behavior component pass; refund acceptance
  RED. Current payment context is action/fee/giver;
  `world_quest_refund_payment` prints then calls ADD_MONEY. Active generic wallet
  spend/refund authority is blocked; an injected callback is component evidence,
  not proof an active player was charged. Primary hooks: durable debit/refund
  ownership and frozen attempt, genuine active SQL admission/publication/recovery.

## QP05 — P1 repeated skins and honest fresh-world supply

- Research: `docs/design/zone-stories/PINE_HOLLOW.md`.
- Producer: Darlene16080, `pineholl.qst` Q at124/146, M16080 cap1 at16110;
  common static dispatch. Jacket: three I16019 → one I16048. Coat: two I16021 →
  one I16050. Fee0, D0. These are independent contracts; prior garments or
  personal hunts are not admission requirements.
- Supply: six E16019 skin declarations (cap6) on brown bear16019 in forest
  rooms16000/16001/16025/16027/16039/16051. The only huge-skin declaration is
  `M 0 16021 1 16040 ...; G 1 16021 1 ...`. While the first huge skin remains live,
  ordinary reset cannot supply the second under that cap. Forced repop/recovered
  or supplied stock is a separate episode, not proof of ordinary fresh accumulation.
- Test one/two/three loose brown skins, one/two huge skins, same UID represented
  twice at native authority, wrong-kind substitution, nested versus loose goods,
  held roots and one spare. Correct supplied roots may complete; an incomplete
  recipe preserves everything. Do not raise the cap or reduce the two-skin demand
  in a test. Primary/builder must resolve ordinary fresh supply separately.
- Expected: exact distinct inputs retire; one fresh garment player UID, stable
  source/economic references; Darlene remains. Reset births/stock origins retain
  separate durable lifetimes and cap refusal does not mint evidence. R1/R3/R4/R6/R8.
- Smallest commands: `test_native_selectors.py --case QP05` and
  `prepare_fixture.py --case QP05 --output <empty-dir>`.
- Results: source/fixture, count/held/capture/submission-refusal selection checks
  pass. Source check verifies the one cap-one huge-skin row and actual parent.
  No native birth/reset/cold garment journey. Primary hook: authentic reset
  supply and existing owner/native12 journey; builder policy stays external.

## QP06 — P1 real mixed reward after lost ACK and later custody

- Research: `docs/reference/zone-story-audits/newbie.md`, Kord bounty Q1836.
- Producer: Kord29257, M29257 cap1 at29217 in `newbie.zon`; QST common dispatch.
  Give one each ear29262, scalp29263, toe29264; receive one item29237,
  C3000 (3 platinum), E2500 nominal, fee0, D0. Runtime reward order reverses
  file E/C/I into I/C/E. Frozen XP respects the admitted level cap; do not assert
  2500 if the actor's admitted cap is smaller.
- Actual inputs are G stock at room29290: mountain dwarf29279 carries ear,
  rugged barbarian29278 scalp, grubby bandit29277 toe, all cap1. No personal kill
  prerequisite is encoded. Reward item29237 is an object, distinct from mobile
  Lapney29237. Kord's reset E29278/G53 is not quest reward stock or issuance.
- Extend the maintained offering/XP-ACK crash journey with exact ear/scalp/toe
  aliases and native item29237 assertions. After one reward receipt, move the
  original item to another legitimate owner, or retire it in a distinct native
  operation; then lose the completion and cold-load twice. Do not require current
  player custody to remain the pre-transfer reward image for terminal cleanup.
- Expected: original three input tombstones; one item creation lifetime/slot
  receipt (later native owner or tombstone accepted), one C3000 economic credit
  with original operation/source, exact XP/save receipt and original acknowledged
  obligation. Original parent and final child retire together with authentic
  historical receipt proof. Changed intent/conflicting or missing ACK retains
  visible recovery and original frames; no new rewards or generic later-save proof.
  R1/R2/R3/R4/R5/R8; XP itself remains outside money accounting.
- Smallest prep: `prepare_fixture.py --case QP06 --output <empty-dir>` and
  `test_native_selectors.py --case QP06`.
- Results: source/fixture and selection/refusal component pass; real mixed output,
  lost reply/cold ACK/current-custody drift/retirement journeys pending. Primary
  hooks: select this fixture in the maintained crash driver (it currently fixes
  synthetic VNUMs/aliases), actual acknowledged historical reader and paired owner.

## QP07 — P1 dynamic stale map/abandon attempt

- Research/producer/binding: same Quietus1709 full-world world_quest/shop chain
  as QP04. Runtime task start/target/zone and giver are player state, not QST
  ingredients. No physical ingredient/reward VNUM for map or abandon.
- Level11 map fee: C110. Abandon fee: default level³ = C1331 at zero elapsed time;
  elapsed-time modifier is bounded1–100 percent over default24 hours, so freeze
  clock/config and exact quoted charge. Progressed FIND_AND_KILL requires confirm;
  source counts that abandoned task against history/quota. No level assumption
  permits unquoted debit or silently changes the price.
- Conceptually admit payment for A(start100,target50), replace with B(start200,
  target60), both active and map-not-bought. Deliver A callback. The actual old
  context carries only action/fee/giver, so old map updates B; old abandon can
  finish/reset B. Identical VNUM target with a different start must also be tested
  by the native owner, as must absent/reset giver and original-giver completion.
- Expected: old debit remains bound to A; B's map/status/history/quota stay
  unchanged. Stale A settles through exact durable restitution or a visible held
  obligation; original operation/attempt evidence survives cold restart and lost
  reply, and a replay cannot mutate B or refund twice. Current valid A map/abandon
  still works at the quoted price. R1/R2/R3/R5/R8.
- Smallest component: `test_bartender_settlement.py --case QP07`;
  `--acceptance` reproduces RED (exit1, component31). The pinned observation also
  executes abandon on B and demonstrates its current history/reset effect.
  `prepare_fixture.py --case QP07 --output <empty-dir>` prepares the maintained
  full-world run-root/TLS/journals and exact service commands.
- Results: source and pinned-behavior component pass; attempt acceptance RED.
  Native driver must reuse `run_world_quest_dual_backend.py` full-world setup;
  primary hooks: attempt-bound durable paid service, fault control before callback,
  exact native SQL task/wallet/history/root evidence and restart recovery.

## Scope of executable acceptance modes

Commands above use `python3 tests/async/quest_accounting_prep/<script>` from the
repo root, on Linux. `--acceptance` asserts a missing local predicate against the
old extracted entry point; it is a RED reproducer, not the final integration API
or full accounting gate. The primary may move ownership to its reviewed native
owner. Adapt the extraction/submission seam to that actual owner while retaining
the scenario and native evidence requirements; do not require implementation
inside an obsolete helper solely to turn a component green.

Source agreement, prepared reduced fixtures, passing observation components and
completed native journeys are distinct. All seven native journeys remain pending
on the integrated primary candidate at its major-batch boundary. No case promotes
writer coverage, accounting activation or release readiness.
