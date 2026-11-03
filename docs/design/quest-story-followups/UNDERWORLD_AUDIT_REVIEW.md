# The Underworld repository-wide audit supplement

This supplement reconciles active and inactive connectivity, typed dependencies, title ownership, and bounded source history for the [source-comprehensive Underworld dossier](../zone-stories/UNDERWORLD.md). The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation audit; no area or gameplay source is changed.

## Active graph and boundary records

The active `underworld` WLD has 232 rooms and 511 authored exits. Of these, 484 are local, 26 are valid outbound exits, and one north exit from #4645 is unresolved. The active loader resolves 510/511 records. The raw weak components contain 154, 70, 6, 1, and 1 rooms. With local authored exits treated as available, #4401 reaches and has a directed return from all 154 rooms in its component; this does not include the other four components. The dossier's selected scenarios begin from 22 distinct inbound boundary targets and model doors, keys, and pools: OPEN reaches 215 rooms, SEARCH adds no rooms, PICK reaches 222, usable selected keys reach 224, and a stocked and successfully used pool reaches 225. These are source-defined route scenarios, not live availability or safety findings.

There are 30 active inbound boundary records, 26 valid outbound records, and 26 reverse records. The 26 reverse records form reciprocal pairs; four more inbound records have no matching outbound record from Underworld:

| Inbound-only record | Evidence |
| --- | --- |
| Connectorzones #53890 down → #4604 | [Active cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L693). |
| Undermountain #92518 down → #4557 | [Active cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1122). |
| Surface #660003 south → #4575 | [Active cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1683). |
| Surface #660003 west → #4562 | [Active cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1684). |

The valid outbound rows are recorded in [the active cross-area exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L59) and [pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L745). The authored north exit at #4645 targets #4646, which is absent from active WLD; the loader discards it, so it is not a working exit ([source](../../../areas/wld/underworld.wld#L6432), [unresolved register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L7)). The source has 13 current fields and no fall-chance fields. See the dossier for its conditional doors, pools, staging rooms, and route details.

## Inactive-WLD approaches and attic evidence

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. No inactive source room overlaps the 232 active Underworld room VNUMs. It found three non-overlapping source-only approaches:

| Inactive source-only approach | Evidence |
| --- | --- |
| `ud_map0.wld` #1105558 north → Underworld #4469 | [Source exit](../../../areas/wld/ud_map0.wld#L37418). |
| `ud_map2.wld` #1120813 north → Underworld #4469 | [Source exit](../../../areas/wld/ud_map2.wld#L2977). |
| `ud_map2.wld` #1127503 up → Underworld #4608 | [Source exit](../../../areas/wld/ud_map2.wld#L96465). |

These exits are not active ingress because the map WLD sources are outside `areas/AREA`. They raise the curated source-only approach total from 2,004 to 2,007; the reviewed Surf subset remains 151. Preserve the source paths and all three records while map ownership and dependency/tool closure remain pending. Keep `ud_map0`, `ud_map1`, `ud_map2`, `surf`, and `surface2011` **RETAIN**; no source move is cleared.

## Typed dependencies and bounded negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Zorta Q18 input and reward | [Underworld Q18](../../../areas/qst/underworld.qst#L23) consumes imported object #88807, defined in [Menden](../../../areas/obj/menden.obj#L91); the local [sidecar contract](../../../areas/story/underworld.story.json#L79) records the same input and S113 Cause Light reward. The local QST produces no object item. |
| Outside QST consumers of local objects | Alatorin has two exact `G I 4505` commands ([first](../../../areas/qst/alatorin.qst#L5487), [second](../../../areas/qst/alatorin.qst#L7146)); Menden consumes #4402 ([QST](../../../areas/qst/menden.qst#L26)); Minopass consumes #4402 ([QST](../../../areas/qst/minopass.qst#L138)); and Mobs Underdark consumes #4660 ([QST](../../../areas/qst/mobs_underdark.qst#L100)). Their Underworld definitions are [#4505](../../../areas/obj/underworld.obj#L246), [#4402](../../../areas/obj/underworld.obj#L30), and [#4660](../../../areas/obj/underworld.obj#L496). Minopass also consumes external #88807; that is not a local Underworld item. |
| Foreign story-sidecar references | Alatorin's two request rows explicitly name item #4505 ([first](../../../areas/story/alatorin.story.json#L2810), [second](../../../areas/story/alatorin.story.json#L3854)); Menden names #4402 ([sidecar](../../../areas/story/menden.story.json#L102)); Minopass names #4402 and imported #88807 ([sidecar](../../../areas/story/minopass.story.json#L183)); Mushroom Caverns names contacts #4680/#4530 ([#4680](../../../areas/story/mushroom_caverns.story.json#L67), [#4530](../../../areas/story/mushroom_caverns.story.json#L109)) and item #4660 ([sidecar](../../../areas/story/mushroom_caverns.story.json#L160)). Other raw number hits are request IDs or prose, not typed VNUM fields. |
| Foreign reset references to local mobiles | Six active `.zon` files contain 244 exact `M` reset references to Underworld mobile prototypes: Alatorin ([example](../../../areas/zon/alatorin.zon#L3349)), Caertannad ([example](../../../areas/zon/caertannad.zon#L1403)), Kvarkpass Connector ([example](../../../areas/zon/kvarkpass_connector.zon#L400)), Malch ([example](../../../areas/zon/malch.zon#L315)), Mobs Underdark ([example](../../../areas/zon/mobs_underdark.zon#L10)), and Mushroom Caverns ([example](../../../areas/zon/mushroom_caverns.zon#L38)). Four inactive `.zon` candidates add 289 such commands: `ud_map0` 13 ([example](../../../areas/zon/ud_map0.zon#L12)), `ud_map1` 4 ([example](../../../areas/zon/ud_map1.zon#L10)), `ud_map2` 267 ([example](../../../areas/zon/ud_map2.zon#L15)), and `surf` 5 ([example](../../../areas/zon/surf.zon#L244)). These source reset commands do not prove current mobile placement or an active map route. |
| External portals and duplicate prototypes | The bounded scan found no type-25 portal targeting Underworld rooms, no external duplicate mobile/object prototype definitions, no local SHP source, and no outside SHP product referencing a local Underworld object. These scoped negatives do not prove installed shop state. |
| Compiled room routes and bindings | SHIFT PRIME and MIND TRAVEL each contain conditional destination #4437 ([innates](../../../src/classes/innates.c#L2008), [psionics](../../../src/classes/psionics.c#L614)); eligible use and arrival conditions still apply. [Zorta #4401](../../../src/specs/specs.assign.c#L426) is assigned no mobile special procedure. The selected active assignments bind local handlers to worm #4480 ([line 579](../../../src/specs/specs.assign.c#L579)), piercer #4530 ([line 582](../../../src/specs/specs.assign.c#L582)), trackers #4520/#4510 ([lines 583–584](../../../src/specs/specs.assign.c#L583), [line 584](../../../src/specs/specs.assign.c#L584)), hammer #4505 ([line 1814](../../../src/specs/specs.assign.c#L1814)), and pools #4403/#4404 ([lines 1815–1816](../../../src/specs/specs.assign.c#L1815)). A room-number match without an effective binding is not an active route or gate. |

### Underworld3 room overlap and the compiled inn binding

The active Underworld3 rooms #30100–#30109 overlap unregistered `cabal_hall` room VNUMs. In particular,
Underworld3 calls #30104 a river room while Cabal Hall calls it a gate, and the compiled inn binding
names that same number. See the [Checkpoint 459 Cabal Hall audit](CABAL_HALL_AUDIT_REVIEW.md#active-underworld3-ownership-of-room-vnums) for the source and shop-loader review.

The active and inactive reset counts are line-level `M` commands grouped by source `.zon` file. They differ from parent-normalized room/reset group counts in the comprehensive dossier. None establishes live stock, availability, a successful quest, or a guaranteed route.

## Title, history, and disposition

The normalized active `.zon` title “The Underworld” has two owners among 351 active titles: `underworld` and `underworld3` ([Underworld title](../../../areas/zon/underworld.zon#L2), [Underworld3 title](../../../areas/zon/underworld3.zon#L2), [active area rows 23 and 145](../../../areas/AREA#L23), [duplicate register](DUPLICATE_AND_REPLACED_ZONES.md#L19)). Both are separate active contributors with distinct ranges: #4401–#4645 and #30000–#30282. Treat this as a title overlap only; the `3` suffix does not establish a predecessor, merge, or replacement.

Bounded same-stem WLD/ZON history goes back to the 2009 area relocation and shows later same-stem edits through 2016; the story sidecar was added in commit `5624fd43f` on 2026-10-06. No explicit predecessor or whole-zone replacement is established by these paths. Keep active `underworld` **DO NOT MOVE** and preserve `underworld3` as an independent active contributor. No source placement changed.

## Builder recommendations

1. Keep the raw five-component WLD graph distinct from the dossier's door, key, pool, and conditional route scenarios. Resolve #4645's missing target only after builder intent for the affected staging rooms is established.
2. Preserve the three inactive `ud_map0` / `ud_map2` approaches and retain those source paths pending map ownership review.
3. Keep #88807's Menden origin separate from local returned items #4402, #4505, and #4660. Review the named outside consumers before changing any of those prototypes or source quests.
4. Retain all active and attic-candidate reset references to Underworld mobiles until their source-generation and map roles are resolved; reset syntax alone does not prove live placement.
5. Keep the duplicate title as naming evidence and retain `underworld` and `underworld3` as separate active sources unless additional ownership or replacement evidence is found.

## Checkpoint 413: explicit builder-question coverage

Physical coverage row 326 now has exactly twenty numbered, direct-source
builder questions with conservative resolutions in the [Underworld
dossier](../zone-stories/UNDERWORLD.md#explicit-builder-questions-and-recommended-resolutions).
They keep Zorta's encoded skill outcome distinct from resurrection prose,
source recovery, relic custody, route admission, current item stock, foreign
consumers, and conditional arrival. No area or gameplay source changed.

## Checkpoint 514 supplement: Nah room identities

Active Underworld3 owns rooms #30100–#30112 in the current full-area source.
Unregistered Nah also defines those numbers for a castle guildhall, temple,
shop, library, cottages, and wizard tower. These are distinct room records:
active Underworld3 uses lake/river content, and its compiled inn at #30104
remains bound to the active room owner. See the
[NAH audit](NAH_AUDIT_REVIEW.md) and
[Cabal Hall room-overlap review](CABAL_HALL_AUDIT_REVIEW.md#active-underworld3-ownership-of-room-vnums).

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has ten mobile reset rows for Underworld mobile #4530. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).
## Checkpoint 868: current source-only approaches and dependencies carried forward

CP868 carries forward the Checkpoint 128 graph and typed-dependency audit against the unchanged pinned area/code snapshot. The active Underworld remains 232 rooms and 511 authored exits: 510 loader-resolved (484 local, 26 outbound), one unresolved #4645 north target, five weak components sized 154/70/6/1/1, 30 active inbounds, and 26 outbound records forming 26 reciprocal pairs. Four active inbound-only records remain as listed above. Thirteen current fields and no fall fields are authored; route scenarios through door/key/pool state do not qualify live access or safety.

Three unregistered exits remain source-only: `ud_map0` #1105558 north and `ud_map2` #1120813 north both target #4469; `ud_map2` #1127503 up targets #4608. No inactive room overlaps the active Underworld VNUMs. Preserve these records on their existing `ud_map0` and `ud_map2` candidate rows 112 and 114; they do not add active ingress. The CP128 curated approach subtotal of 2,007 overall / 151 Surf is historical; the current cumulative source-only total is 11,993 overall / 415 Surf.

The typed closure remains unchanged: local QST Q18 imports Menden object #88807; outside QST consumers use local #4402, #4505, and #4660; Alatorin has two #4505 item commands; 244 active reset commands across six `.zon` files reference local mobiles; four source-only candidate groups add 289 such commands; and the known sidecar/code bindings and duplicate/title limits remain as detailed above. Keep active `underworld` **DO NOT MOVE** and `underworld3` as a distinct active map despite the shared normalized title. Preserve map-source identities and existing `ud_map0`/`ud_map2` candidate dispositions; add no Underworld candidate or migration clearance.
