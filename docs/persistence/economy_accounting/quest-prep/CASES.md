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
- Smallest prep command (being prepared):
  `python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP01`.
  Mini fixture command will be `prepare_fixture.py --case QP01 --output <empty-dir>`.
- Current result: source verified, not component/native journey qualified.
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
- Smallest prep command: `test_native_selectors.py --case QP02`; future acceptance
  mode must fail this pin. `prepare_fixture.py --case QP02 --output <empty-dir>`
  will preserve all four Q blocks and original order.
- Current result: dossier/source agree on concrete dispatch blocker; no runtime
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
  equips armor and sword; Altrucali16086 supplies G16080. These are reset stock,
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
- Smallest prep: `prepare_fixture.py --case QP03 --output <empty-dir>` plus
  production-terms check. Full qualification needs primary's real birth/native12
  integrated candidate and fault journey; reduced fixture alone is insufficient.
- Current result: source verified; publication/disappearance/birth journey pending.
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
- Smallest prep: `test_bartender_settlement.py --case QP04` will execute the actual
  callback with isolated state. Use existing `run_world_quest_dual_backend.py`
  full-world setup for primary SQL fault journey, not the static mini catalog.
- Current result: source verified only. Current payment context is action/fee/giver;
  `world_quest_refund_payment` prints then calls ADD_MONEY. Active generic wallet
  spend/refund authority is blocked; an injected callback is component evidence,
  not proof an active player was charged. Primary hooks: durable debit/refund
  ownership and frozen attempt, genuine active SQL admission/publication/recovery.

## Remaining selected specifications

QP05–QP07 and executable results will be appended in the second owned bundle.
Final qualification uses the integrated primary candidate at its batch boundary.
