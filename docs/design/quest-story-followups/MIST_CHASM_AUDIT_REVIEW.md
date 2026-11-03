# The Chasm of the Misty Vale — Checkpoint 112 audit review

Source snapshot: `72ba3deff3bb30bde880018cdebe7c6db675b697`. This repository-wide supplement reconciles active world connectivity, inactive WLD approaches, bounded typed dependencies, title ownership, tracked history, and attic disposition with the [existing comprehensive Chasm story dossier](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md).

The active `mist_chasm` contributor remains **DO NOT MOVE**. The `surf`, `surface2011`, and `torr` source groups remain **RETAIN** pending map ownership and their remaining dependency/tooling review. No native source, area placement, database, or gameplay behavior changes in this checkpoint.

## Active graph and boundary

The normal active manifest contains 351 WLD contributors and 253,262 room VNUM records. `mist_chasm.wld` contributes 100 rooms and 209 exits: 208 internal and one outbound. All 209 targets resolve in the active world. The local undirected graph has one weak component of 100 rooms. In the directed authored-exit graph, assuming every recorded exit can be traversed, all 100 rooms are reachable from entrance #15000 and all 100 can return to #15000. This is a source graph result; it does not qualify door states, switches, SEARCH, magic, falling, player access, or safe traversal.

The active boundary is reciprocal: Chasm #15000 east to Surface #638825 and Surface #638825 west to Chasm #15000. See the Chasm source [exit record](../../../areas/wld/mist_chasm.wld#L11), Surface source [return record](../../../areas/wld/surface.wld#L2914758), [cross-area exit rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L267) and [#L1670](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1670), and [pair summaries](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L282) and [#L546](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L546).

There is no Chasm-specific active room-VNUM collision. The only duplicate active room VNUM in the full census is unrelated: #51800 occurs in `hell_three_minauros.wld` and `hell_two_dis.wld`. The selected Chasm graph still has nine fall-chance rooms and no currents; the detailed hazard and passage qualification remains in the existing dossier.

## Inactive approaches and attic dependencies

The all-95 unregistered-WLD sweep parsed 527,791 rooms and 2,080,773 exits with no parse issues and no VNUM overlap with Chasm rooms #15000–#15099. It found two source-only approaches:

| Inactive source record | Target | Qualification |
| --- | --- | --- |
| `surf.wld` #253287 west → Chasm #15000 ([exit](../../../areas/wld/surf.wld#L1118139)) | Active entrance | The source stem is absent from `areas/AREA`; this does not create current ingress. |
| `torr.wld` #79092 south → Chasm #15052 ([exit](../../../areas/wld/torr.wld#L1909)) | Active interior room | The source stem is absent from `areas/AREA`; this does not create current ingress. |

These findings raise the reviewed Surf subset from 136 to 137 and the curated source-only approach total from 1,524 to 1,526. Preserve the `surf.wld` record separately from the active Surface pair, keep `surf` and `surface2011` **RETAIN**, and record the new Chasm approach in the `torr` candidate. Do not merge, reactivate, delete, or relocate either source based on these exits. See the [Surf candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Torr candidate row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L107), and [attic plan](ATTIC_MIGRATION_PLAN.md).

## Typed references and outside dependencies

The bounded typed scan found these exact cross-area item references:

| Source | Typed dependency |
| --- | --- |
| `surfacemini.qst` line 32 ([source](../../../areas/qst/surfacemini.qst#L32)) and `surfacemini.story.json` ([scale exchange](../../../areas/story/surfacemini.story.json#L388), [potion consumer](../../../areas/story/surfacemini.story.json#L377)) | The Surface hermit consumes potion object #15008 for #97930; the Chasm shaman sidecar contract produces #15008 from scales #15017 and #15018. Supplied potion stock and the Chasm history remain separate evidence. |
| `wh.qst` line 2630 ([source](../../../areas/qst/wh.qst#L2630)) and `wh.story.json` line 1104 ([source](../../../areas/story/wh.story.json#L1104)) | Winterhaven Lancer Q2627 consumes necklace object #15012 for missing object #33719. The existing Winterhaven audit already excludes this incomplete reward path; retain its separate builder repair requirement. |

The Chasm reset at [`mist_chasm.zon` line 112](../../../areas/zon/mist_chasm.zon#L112) equips banshee #15008 with object #67273, whose prototype is defined in [`unique.obj`](../../../areas/obj/unique.obj#L1134). That is one imported object; local potion object #15008 is a different typed identity from mobile #15008.

The Chasm has no local SHP source. Bounded scans found no outside SHP product, no external active reset-role reference to Chasm prototypes, no duplicate external mobile/object prototype for local identities, and no additional exact local QST item input. The active-world scan reviewed 872 type-25 portal declarations and found no target into Chasm. These are source-scoped findings; arbitrary code-generated travel, editor tooling, and runtime circulation are outside this closure. Full potion, switch, trap, and custom binding limits remain in the [story dossier](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md).

## Title, history, and disposition

After color-code normalization, the exact active `.zon` title “The Chasm of the Misty Vale” has one owner among 351 active `.zon` titles: `mist_chasm`. Lexical neighbors such as `mistywood`, `myrloch_vale`, and `glacier` have distinct titles and source identities.

`git log --follow` for the WLD, ZON, MOB, OBJ, and QST paths shows same-stem history and the 2009 repository-wide `areas/` relocation, followed by ordinary same-path edits. The tracked story sidecar was added by `2757c81ca` on 2026-10-06. These followed paths and the current active manifest show no explicit predecessor, alternate-stem rename, or replacement claim. The bounded record does not decide undocumented map intent.

Keep active `mist_chasm` **DO NOT MOVE**. Keep inactive `surf`, `surface2011`, and `torr` **RETAIN** until their owners resolve map purpose and remaining dependency/tooling closure. Preserve the two inactive approaches and the active reciprocal Surface pair as distinct records.

## References

- [Coverage row](ZONE_COVERAGE_REGISTER.csv#L181)
- [World connectivity report](WORLD_CONNECTIVITY.md)
- [Duplicate and replacement history](DUPLICATE_AND_REPLACED_ZONES.md)
- [Attic candidate dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv)
- [Source-comprehensive Chasm dossier](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md)



## Checkpoint 283: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should the shaman continue requiring exactly one small scale #15017 and one large scale #15018 for potion #15008, with the selected shaman departing? | [Shaman Q11](../../../areas/qst/mist_chasm.qst#L11), [potion prototype](../../../areas/obj/mist_chasm.obj#L131) | Preserve the exact two-kind recipe as one receipt; don't split potion issuance or departure into extra achievements. |
| 2 | Should warrior small-scale and large-scale sales remain separate contracts with their current 2,000 and 4,000 coin rewards? | [Warrior Q35/Q47](../../../areas/qst/mist_chasm.qst#L35), [source index](../../reference/zone-story-audits/mist_chasm.md#native-exchanges) | Keep each accepted item and amount exact; qualification must use actual credited currency and the selected giver's departure. |
| 3 | Should the weaver continue requiring two copies of each scale kind for shield #15019 and 81,000 experience as one exchange? | [Weaver Q69](../../../areas/qst/mist_chasm.qst#L69), [shield](../../../areas/obj/mist_chasm.obj#L292) | Preserve the exact multiset and one combined result; four copies of one size or two separate outcomes do not satisfy it. |
| 4 | Should supplied scales remain valid without a personal salamander kill, first recovery, or prior warrior sale? | [Scale source/reset review](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#native-exchanges-and-exact-preparation), [native recipes](../../../areas/qst/mist_chasm.qst) | Keep exact supplied-item eligibility; track source identity and personal recovery only as separate optional history. |
| 5 | Should large and small salamander scales keep their distinct object VNUMs and source populations despite sharing the same short mob name? | [Reset sources](../../../areas/zon/mist_chasm.zon), [small scale](../../../areas/obj/mist_chasm.obj#L268), [large scale](../../../areas/obj/mist_chasm.obj#L280) | Preserve typed identities, parent/source generation, and count; names alone cannot substitute one size for the other. |
| 6 | Should all four mode-one native exchanges remain potential daily outcomes without promising fresh givers or fresh scales at rollover? | [Zone header](../../../areas/zon/mist_chasm.zon#L3), [reset coverage](../../reference/zone-story-audits/mist_chasm.md#reset-coverage) | Keep daily policy disabled until active READY reset access, usable source stock, giver renewal, reward/save/replay, and fair supply are qualified. |
| 7 | Should drinking the shaman potion remain a separate, optional effect outcome from receiving it in the scale exchange? | [Potion behavior review](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance), [native reward](../../../areas/qst/mist_chasm.qst#L11) | Record actual consumption and successful effect independently; a QUAFF command or extracted potion does not prove that a spell took effect. |
| 8 | Should the potion continue to remain optional bridge aid when combat spill, no-magic consumption, and fall suppression depend on actual actor state? | [Bridge and quaff review](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance), [fall rooms](../../../areas/wld/mist_chasm.wld) | Preserve the authored alternatives and qualify actual crossing and survival; don't make possession a mandatory shield or travel prerequisite. |
| 9 | Should NO_GROUND rooms #15079–#15086 continue to require actual flight for lateral movement rather than treating levitation as unrestricted flight? | [Air-room behavior](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance), [room controls](../../../areas/wld/mist_chasm.wld) | Preserve the distinct movement rules; measure admitted arrival, passage, and safe destination instead of inferring them from potion possession. |
| 10 | Should fixed head switch #15000 continue to respond to WAKE and operate shared room access without creating a personal-only switch achievement? | [Switch prototype and reset](../../../areas/obj/mist_chasm.obj#L1), [switch review](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance) | Record the actual before/after block state and actor; repeated use or shared open state is not personal activation evidence. |
| 11 | Should the two fixed flower switches #15016 and #15023 keep their separate touch directions and reciprocal door effects? | [Flower objects](../../../areas/obj/mist_chasm.obj#L254), [switch destinations](../../../areas/zon/mist_chasm.zon) | Preserve exact object, direction, and target-door state; fixed scenery does not become a carried offering or automatically prove traversal. |
| 12 | Is granite head #15003's stale-looking switch data and unsupported trap record intentionally inert? | [Head prototype](../../../areas/obj/mist_chasm.obj#L44), [trap/switch review](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance) | Confirm intended data and effect before a separate repair; don't activate an unqualified hazard or award switch credit from its numeric fields. |
| 13 | Should casket #15010 remain closed but unlocked, with key #15024 a consumable optional way to change that state? | [Casket resets](../../../areas/zon/mist_chasm.zon), [key/casket review](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance) | Keep current access semantics; track successful unlock, key break, open state, and child recovery separately. |
| 14 | Should the secret locked trapdoors with negative key values remain unavailable to ordinary UNLOCK/PICK, with their current pass-door conditions? | [Trapdoor and key review](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance), [world source](../../../areas/wld/mist_chasm.wld) | Preserve challenge until builders select a complete access/return policy; don't promise that the skeletal key or generic spell bypasses them. |
| 15 | Should Mother and young Pygmies remain reunion lore until an actual actor movement, consent, surviving destination, and saved result are authored? | [Pygmy placements](../../../areas/zon/mist_chasm.zon), [wider story limits](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#passage-and-potion-guidance) | Don't create reunion credit from proximity or descriptive relationships; define the actors, route, failure handling, and beneficiary first. |
| 16 | Should imported unique object #67273 remain banshee equipment stock, distinct from local potion item #15008 and mobile #15008? | [Imported reset](../../../areas/zon/mist_chasm.zon#L112), [unique prototype](../../../areas/obj/unique.obj#L1134), [typed namespace review](MIST_CHASM_AUDIT_REVIEW.md#typed-references-and-outside-dependencies) | Preserve typed item/mobile identity and imported ownership; don't infer an extra exchange or victory from equal VNUMs. |
| 17 | Should the Surface hermit's potion exchange remain independent from the Chasm shaman's history, so supplied potion stock still qualifies there? | [Surface Mini QST](../../../areas/qst/surfacemini.qst#L32), [foreign contract review](MIST_CHASM_AUDIT_REVIEW.md#typed-references-and-outside-dependencies) | Keep the Surface receipt in its owning zone; don't require personal potion production or collapse the two exchanges into one story. |
| 18 | Should Winterhaven's Lancer contract remain excluded while its reward object #33719 is missing? | [Winterhaven Q2627](../../../areas/qst/wh.qst#L2630), [Winterhaven source index](../../reference/zone-story-audits/wh.md) | Keep that external repair requirement with Winterhaven; resolve it with a verified reward and separate source/persistence evidence before enabling completion. |
| 19 | Should the Surf #253287 and Torr #79092 approaches remain source-only on their existing candidate rows rather than imply active ingress? | [Inactive-WLD approaches](MIST_CHASM_AUDIT_REVIEW.md#inactive-approaches-and-attic-dependencies), [Surf row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), [Torr row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L107) | Retain each source with its current owner; keep both separate from the reciprocal active Surface pair. |
| 20 | Should new bridge aid, switch, reunion, guardian, or conservation stories require explicit saved outcomes while the four native exchanges stay independent? | [Story integration boundary](../zone-stories/THE_CHASM_OF_THE_MISTY_VALE.md#source-closure-and-qualification), [native contracts](../../../areas/qst/mist_chasm.qst) | Use typed source, movement, effect, and beneficiary events with clear failure/replay rules; add no hidden chain to the native recipes. |
