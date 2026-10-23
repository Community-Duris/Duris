# Alatorin (`alatorin`) audit reconciliation

Reviewed October 12, 2026 against the current checkout and the source snapshot
in [coverage row 5](ZONE_COVERAGE_REGISTER.csv#L6). The
[Alatorin source story map](../zone-stories/ALATORIN.md) remains the full
native story and reset inventory. This supplement closes the title/alias,
source history, loaded WLD graph, and direct dependency review and makes the
remaining owner decisions explicit. It is source analysis, not live access,
combat, service, or accounting qualification.

## Identity and migration boundary

The active area key is `alatorin`, canonical zone 831. Its current normalized
ZON display title is **Alatorin - the Forge City**, unique among all 442
current ZON titles. The current native WLD, MOB, OBJ, ZON, QST, SHP, and story
paths remain under the `alatorin` stem. Followed path history and title review
do not establish a tracked alternate-stem rename, same-title active duplicate,
distinct predecessor, or whole-zone replacement.

Do not conflate this active seven-source contributor with the separate
unregistered `areas/obj/alatorin_objects.obj` object bank. That file has 408
object records: every VNUM overlaps active `alatorin.obj`, and 168 also overlap
different Nexus56 prototypes. Its saved-object identity, typed consumers,
editors, and runtime owners remain a separate review. Keep that component on
existing candidate [row 5](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6)
with **RETAIN IN PLACE**. It is not evidence that the active city is a
duplicate or replacement. Keep active `alatorin` **DO NOT MOVE**; no candidate
row or zone migration is proposed for it.

The full dossier classifies 495 native contracts into 18 story families, 72
requests, 163 supporting services, and four rewardless-return exclusions. Its
sidecar maps 90 achievement outcomes and 82 potential daily outcomes within
current journal limits. All 952 rooms, 425 mobiles, 602 objects, 3,596 resets,
fifteen shops, 296 usable addressed dialogue families, and selected foreign
proof prototypes have source coverage. Supplied exact proof remains valid;
the map does not invent personal kills, preparation, travel, learning, or a
single enforced pilgrimage from descriptive text.

## World graph and raw reachability

The 952-room `alatorin.wld` has 2,128/2,128 loader-resolved authored exits.
Its local weak graph has 47 components, with 288 rooms in its largest
component. The registered cross-area pair summary records 81 inbound and 85
outbound exit directions, with reverse directions on 74 records:

| Direction | Partner areas and record counts |
| --- | --- |
| Outbound | `surfacekeeps` 76; `surface` 4; `connectorzones` 1; `plane_air_one`, `plane_earth_one`, `plane_fire_one`, and `plane_water_one` 1 each |
| Inbound | `surfacekeeps` 74; `surface` 4; `connectorzones` 1; `wh` 2 |

The large `surfacekeeps` connection is intentional cross-file city geography,
not a missing local room target. Elemental planar exits, direct Surface paths,
and the Connector Zones edge are distinct authored boundaries. See outbound
pair-summary rows [7](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L7),
[8](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L8),
[9](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L9),
[10](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L10),
[11](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L11),
[12](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L12), and
[13](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L13), with inbound rows
[86](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L86),
[460](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L460),
[635](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L635), and
[777](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L777). The coverage census records no
negative or unresolved WLD targets and no unregistered WLD target.

A separate raw directed traversal over all 351 current active WLD sources
keys rooms by source stem and VNUM, assigns cross-area targets by the exact
resolved-owner register, and omits unresolved and negative targets. Treating
every remaining authored direction as passable, it reaches 213,687 active
room records from Alatorin entrance #83100. Within Alatorin it reaches 899 of
952 rooms; raw paths back to #83100 exist from 896, and 890 Alatorin rooms lie
in the same root strongly connected component. This graph includes cross-file exits and
does not use type-25 objects, spells, switches, reset state, sector movement,
door locks, combat, or safe-return rules. These numbers describe authored
reachability only; the source dossier's route and mechanism inventory remains
the authority for the separate qualification questions.

## Direct typed dependencies

The exact outgoing and incoming dependencies already gathered across the
coverage row and neighboring reviews are preserved here as a compact index:

| Direction | Exact source-backed edge | Disposition |
| --- | --- | --- |
| Heavens → Alatorin | Three local guard E resets equip Heavens object #283. | Preserve Heavens ownership and the three reset parents; this is not a local object definition or quest reward. |
| Nyneth2 → Alatorin | Two local G resets load Nyneth2 object #23045 on the aboleth slime mage. | Preserve as a typed reset import; see [Nyneth2 review](NYNETH2_AUDIT_REVIEW.md). |
| Nyneth3 ↔ Alatorin | Alatorin QST awards object #38779; its story receipt names the same prototype, and an active local reset also declares that exact item. | Preserve the shared object identity and distinct QST/reset producers; see [Nyneth3 review](NYNETH3_AUDIT_REVIEW.md). |
| Sea Kingdom → Alatorin | Alatorin QST offerings reference Sea Kingdom objects #31538 and #31544; the sidecar records #31538. | Preserve exact incoming proof kinds; see the [Sea Kingdom review](SEAKNGDM_AUDIT_REVIEW.md). |
| Valley of Crushk → Alatorin | Local G resets use Valley objects #2009 and #2020; a local SHP product also lists #2020. | Preserve stock/reset identity; see the [Valley review](VALLEY_CRUSHK_AUDIT_REVIEW.md). |
| Bandit Canyons → Alatorin | Alatorin QST and sidecar refer to serrated stone dagger #98505. | Preserve the exact foreign input; see the [Bandit Canyons review](BANDIT_CANYONS_AUDIT_REVIEW.md). |
| Ancient Bridge → Alatorin | Alatorin ZON line 2817 loads Ancient Bridge mobile #85617 into room #83361. | Preserve the foreign reset role; see the [Ancient Bridge review](ANCIENTB_AUDIT_REVIEW.md). |
| Faang → Alatorin | Three M resets load Faang bat #15224 into rooms #83717, #83721, and #83724 ([first](../../../areas/zon/alatorin.zon#L3691), [last](../../../areas/zon/alatorin.zon#L3693), [prototype](../../../areas/mob/faang.mob#L296)). | Preserve the active Faang mobile owner; see the [Faang typed audit](FAANG_STORY_REVIEW.md#checkpoint-796-current-typed-census-refresh). |
| Alatorin → Ard'gral | Type-25 object #83141 is reset in #83342 and declares destination room #30453. | Preserve object travel as separate from the authored WLD graph; see [Ard'gral review](ARDGRAL_AUDIT_REVIEW.md). |
| Alatorin → Astral Main | Type-25 object #83301 is reset at #83487 and targets #19721; the same object number also occurs in the unregistered object bank. | Preserve the active route and separately retain the duplicate source-only prototype; see [Astral Main review](ASTRAL_MAIN_AUDIT_REVIEW.md) and the object-bank candidate row. |
| Alatorin → Kobold | Ten local O resets load Kobold object #1460. | Preserve each exact reset role; see the [Kobold review](KOBOLD_AUDIT_REVIEW.md#bounded-typed-reference-closure). |

The main dossier additionally maps the four Surface arrivals, 76/74
Surfacekeeps directions, direct plane routes, 336 foreign object prototypes
used by local I terms, fifteen local shops, and shared item/source competition.
Its scope statement correctly limits the wider foreign zones to selected
source paths rather than claiming a new exhaustive audit of each neighbor.

## Builder questions and conservative recommendations

These questions translate the remaining content and integration decisions in
the comprehensive dossier into direct, reviewable owner prompts. They record
recommendations, not owner answers or authorization to change gameplay.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| ALAT-01 | Is there a command-target alias intended beyond `alatorin` and the unique ZON title **Alatorin - the Forge City**? | Keep the current key and display title distinct from `alatorin_objects`; add another alias only after its exact target and scope are confirmed. |
| ALAT-02 | Are the unregistered `alatorin_objects.obj` copies current prototypes, saved-player-era variants, or both? | Keep all 408 source-only objects on candidate row 5 RETAIN IN PLACE until saved-item identity, active Alatorin/Nexus56 overlaps, typed consumers, editors, and runtime owners are reconciled. Do not merge it into the active zone by name. |
| ALAT-03 | Should discovery and district navigation treat the `surfacekeeps` rooms as part of one Forge City experience or as a separate owner? | Preserve current area ownership and cross-file WLD links. Choose a single navigation/presentation policy only after testing discovery and duplicate-credit behavior in both directions. |
| ALAT-04 | Are the 18 story families meant to remain independent accepted outcomes, or should selected families enforce an ordered city-wide campaign? | Keep the existing independent contracts by default. Add a campaign prerequisite only for a specifically named, durable earlier outcome; supplied final proof should continue to fit unless an exact recipe is deliberately changed. |
| ALAT-05 | Does the proposed district/story presentation need more than the current 253 displayed journal rows? | Keep the existing 256-row and other catalog limits. Expand or paginate only where a faithful mapped story requires it, and do not merge different recipients or rewards only to reduce counts. |
| ALAT-06 | Should the librarian's three-book exchange award the actual wisp of deadly mist, or a medal as the prose claims? | Preserve the current exact QST reward #83267. Correct only the clue/prose unless the builder selects and balances a different native output. |
| ALAT-07 | Are the five-versus-six badge statements and banquet/attendant reward descriptions intended to match the actual six-item offers and exact outputs? | Keep each current recipe and reward authoritative; correct each inaccurate count/output clue rather than changing payment by inference. |
| ALAT-08 | Which Xamora descriptions and Mundorno breastplate recipes are intended where the text conflicts with exact materials, price, or overlapping output? | Confirm the chosen material/reward for Xamora and whether Mundorno's two paid alternatives are intentional. Preserve the actual recipes, prices, and rewards until that decision is explicit. |
| ALAT-09 | Is there an intended source for tinker's token #83458 beyond the native ten-token fragment recipe? | Keep supplied tokens valid and expose the exact source blocker; add an ordinary producer only after the owner specifies its location, cap, recovery, and renewal. |
| ALAT-10 | Which item sources and reset caps are intentionally shared across local and foreign routes? | Preserve source kinds and exact offers. Qualify cap contention, force/ordinary reset, first recovery, and source ownership before changing stock or implying personal preparation is required. |
| ALAT-11 | What are the intended command, destination, and control target for the shrine/treasury mechanisms and malformed wall #83368? | Preserve current switch/portal targets pending owner intent; do not infer a ritual, map transition, wall destination, or reward from metadata alone. |
| ALAT-12 | Should mining, smelting, refining, salvage, forage, and paid-cleric services remain refused while active accounting is enabled? | Preserve mandatory guards. Re-enable a service only after one exact, recoverable transaction defines accepted inputs, actor/price, output, failure, and recovery behavior. |
| ALAT-13 | Which ground, death-born, fishing, forage, and random-scroll outputs should support source-history milestones? | Keep supplied valid proof sufficient for native delivery. Add source milestones only after durable generation, custody, first recovery, interruption, and restart evidence exists. |
| ALAT-14 | Is an ordered divine pilgrimage or eighth-avatar outcome intended, given the seven defined shard gifts and separate replacement shards? | Preserve seven actual gift exchanges and rewardless replacements as distinct receipts. Add an aggregate pilgrimage only if its stages, participants, replay, and final world effect are named. |
| ALAT-15 | Should `earthen`/`flaming` emissary exchanges and other transformation narration change persistent world state? | Treat the existing delivery outcome separately from the described transformation; define and qualify the actor/world-state endpoint before claiming it occurred. |
| ALAT-16 | Which of the 82 potential daily outcomes are suitable given capped materials, low-chance recipients, competing uses, and recipient retirement? | Keep daily eligibility separate from source-complete recipes. Qualify repeat supply, reset admissions, access, and giver renewal first; do not promise daily availability from ZON declarations alone. |

See the [main source story map](../zone-stories/ALATORIN.md) for the exact
receipts, all-story classification, qualification boundaries, and detailed
technical repair proposals. See the separate
[object-bank audit](ALATORIN_OBJECTS_AUDIT_REVIEW.md) and its
[builder questions](../zone-stories/ALATORIN_OBJECTS_BUILDER_QUESTIONS.md) for
the unregistered candidate component.

## Checkpoint 775: active Braddistock reset import

Alatorin ZON line 1469 has a P reset reference to Braddistock zone-13 object
#1310, the ring of protection. Preserve Alatorin's reset role and
Braddistock's active prototype ownership; see the
[Braddistock zone-13 audit](BRADDISTOCK_ZONE13_AUDIT_REVIEW.md).

## Checkpoint 777: Breale QST items

Active Alatorin QST G I commands at lines 51 and 2823 reference Breale objects #2657 and #2658. Preserve the current Breale object owner and the authored Alatorin item requirements; see the [Breale typed-dependency audit](BREALE_AUDIT_REVIEW.md#inbound-active-consumers).

## Bloodstone Checkpoint 778: reset and shop consumers

Active Alatorin ZON imports ten distinct Bloodstone objects across 12 G rows; its shops list #74021, #74132, and #74181. Preserve these Bloodstone-owned prototypes; see the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#inbound-active-consumers).

## Checkpoint 779: Caertannad typed dependency

Active Alatorin has two mobile reset rows into Caer Tannad #78438 and four object reset rows into #78414/#78484; its QST has 36 item commands over seven Caer Tannad objects. See [Caertannad inbound consumers](CAERTANNAD_AUDIT_REVIEW.md#active-inbound-consumers).


## Checkpoint 782: active Cosmic object consumers

Alatorin ZON has two G reset rows for Cosmic object #76067 and one for Cosmic
object #76060 ([lines 4080, 4514, and 4518](../../../areas/zon/alatorin.zon#L4080)).
Preserve Cosmic as the current prototype owner; these reset rows do not
establish successful admission or stock. See the
[Cosmic typed audit](COSMIC_AUDIT_REVIEW.md#active-and-candidate-inbound-dependencies).

## Checkpoint 789: Dreamfoil ingredient import

Active `dream.zon` G row 139 imports Alatorin dreamfoil object #83283
([reset](../../../areas/zon/dream.zon#L139),
[prototype](../../../areas/obj/alatorin.obj#L2864)). Keep Alatorin as its prototype owner
and keep Dream's ingredient reset separate from the four-flower Alatorin QST receipt.
See [audit](DRIFTING_REALM_AUDIT_REVIEW.md#checkpoint-789-shop-and-sidecar-closure-refresh).

## Checkpoint 790: Stoutdorf typed consumers

Active Alatorin QST rows [5486](../../../areas/qst/alatorin.qst#L5486) and
[7145](../../../areas/qst/alatorin.qst#L7145) each use Stoutdorf DwarfSlayer axe #34226.
The story sidecar repeats the typed item in two alternative five-item returns at
[lines 2809](../../../areas/story/alatorin.story.json#L2809) and
[3853](../../../areas/story/alatorin.story.json#L3853). Separately, active Alatorin P
row 1679 resets Stoutdorf gauntlets #34217 inside display case #83367
([reset](../../../areas/zon/alatorin.zon#L1679)). Preserve Stoutdorf as prototype owner;
the gauntlet reset is separate from the axe receipts. See the [Stoutdorf static typed
audit](DRST_AUDIT_REVIEW.md#checkpoint-790-current-static-typed-closure).

## Checkpoint 802: Foggy Woods armor-service output

Alatorin's active armor-service QST produces Foggy Woods white dragon
scalemail #90018 in `R I` row [5724](../../../areas/qst/alatorin.qst#L5724).
The story sidecar records `receive=I:90018` in its contract
([sidecar](../../../areas/story/alatorin.story.json#L2959)). Foggy Woods
remains the prototype owner; the source recipe does not establish successful
settlement or live supply. See the
[Foggy typed audit](FOGGY_WOODS_AUDIT_REVIEW.md#checkpoint-802-typed-census-and-compiled-bindings).

