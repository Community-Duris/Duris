# Du'Maathe Castle: comprehensive source map

Priority 75 of the original roadmap. **Source-comprehensive; played qualification
remains pending.** The new [journal](../../../areas/story/mntcastl.story.json) is
schema three, revision one: four independent lord offerings and four supporting
potion services, 15 local contacts, all 12 addressed topic families and 15 aliases.
Fourteen optional checks separate 11 present materials from three earlier receipts.
The four lord offerings are potential daily candidates; preparation services earn
neither achievement nor daily credit. Discovery retains its own achievement.
All new discovery, encounter, journal, achievement and daily credit requires
**active, ready accounting**; frozen obligations retain separate recovery.

One actual repair ships separately in
[45bb3c948](https://github.com/Community-Duris/Duris/commit/45bb3c948): the northwest
parapet's description now says north rather than south. All other native bytes
remain unchanged. Supply, gate, magic, recipe-price and campaign findings below
are proposals, not shipped repairs.

## Complete source closure

The active `areas/AREA` entry is `mntcastl *371-376`; zone 371 spans
37100–37605 with reset mode one. Its header records native levels 15–25;
those numbers do not assert an enforced admission or encounter difficulty.
Physical membership is all 506 rooms in that range. Review covers:

- [All 21 native blocks](../../../areas/qst/mntcastl.qst): 13 M and eight Q.
  Twelve M families are addressed, with 15 aliases. The ambient `qc_action80`
  family is not another learned keyword or achievement. No QA, fee, XP output
  or disappearing recipient occurs in these eight local offerings.
- [All 506 complete rooms](../../../areas/wld/mntcastl.wld): 126 exact title/prose
  families, 20 complete numeric headers, 1,224 exits/numeric families and 206
  complete exit-description/keyword families. No room E/F/T block or unknown
  trailing field occurs. Plateau lake, forest and waterfall; cliff and bridge;
  walls, quarters and spire; prison and catacombs; river road; Cheltenham,
  laboratory, pools, fields and mine maze are covered, including all repeat
  memberships and exact direction/state/key/destination bindings.
- [All 92 mobiles](../../../areas/mob/mntcastl.mob) and
  [49 objects](../../../areas/obj/mntcastl.obj): full prose, flags, values,
  affects and extra descriptions. Mobiles are 37100–37191; objects are
  37100–37147 plus decayed key 37346. Hidden loaded inputs are not ordinary
  CARVE products; same-name inventory and shared material use stay distinct.
- [All 419 reset commands](../../../areas/zon/mntcastl.zon): D12/O3/M352/E35/F5/G12,
  303 exact and 312 parent-aware families. Followers retain the original lord
  leader rather than each preceding follower. No P/R reset occurs. Both imported
  prototypes, Verspin amethyst 28146 and castle memory 55171, were read fully;
  their presence supplies no additional local offering.
- [The complete shop](../../../areas/shp/mntcastl.shp): T'Sharenoa 37158 at
  37301 sells wand 37140. It is not protective input 37110. Both literal inn
  assignments (37313/37434), their loaded innkeepers and shared RENT path were
  distinguished from ROOM_INN and ACT_TEACHER flags, which are absent locally.
- Carmotee 37145 has a separate automatic epic-teacher binding through
  [the table](../../../src/classes/epic_skills.c#L201) and
  [assignment loop](../../../src/world/epic.c#L308). PRACTICE and its level,
  eligibility, cost, skill and accounting guards were reviewed. Native epic
  skill purchases remain unavailable under active accounting; no smith repair
  quest is inferred from his description.
- Shared native offer matching, selected inputs, canonical receipt ownership,
  indexed reward allocation/settlement/recovery, current loose inventory,
  SEARCH, OPEN, UNLOCK, key breakage, potion use, CARVE, loaded-exit decoding
  and door resets were traced. Native source completeness does not qualify
  admission, persisted source acquisition or a played transaction.

Bounded foreign closure includes the full humble hermit, cabin 97907, recipe
97903 and exact clothing producer blocks; both clothing prototypes, their
material prototypes, the hunched podaling and workshop 28790, and representative
active loaded feather/hide sources. Those crafts accept six dark feathers plus
one hide plus **1,000 copper**, or three hides plus **500 copper**. Their dialogue
instead quotes nine/six platinum; this discrepancy remains a builder decision.
Coin/mixed input remains refused with active accounting. Supplied clothing
can satisfy the hermit's item-only exchange, and supplied recipes skip it.
This is bounded dependency review, not a comprehensive new Podaling-area pass.

The full Alatorin totem-consumer block, goblin traveler 83385, Frothing Mug
83379, active placement and skull-belt reward 83495 were reviewed. Two full
direct foreign boundary rooms and four directed edges connect the Surface
road and Pit of Dragons. The additional full hallway 15967, both tooth-key
door resets and every foreign exit keyed by any of the four local keys were
checked. Only the two reciprocal Pit of Dragons exits use a local key.
All 713 type-25 portal prototypes were scanned; none targets a local room.
No foreign active reset group loads a local input/actor or required recipe,
and no local item-teleport or switch procedure supplies an omitted route.
The [generated review index](../../reference/zone-story-audits/mntcastl.md)
retains source locations and every native classification.

## Exact accepted exchanges and progression

All eight local Q recipients remain after acceptance. Each offering has its own
exact terminal receipt, not a stage prerequisite imposed by conversational order.
All eight definitions are native daily-eligible item exchanges; journal categories
restrict achievement/daily candidates to the four lord outcomes.

| Outcome | Exact loose input | Declared reward | Progression and boundary |
| --- | --- | --- | --- |
| [Lord: horn](../../../areas/qst/mntcastl.qst#L70) | Blue-tinged dragon horn 37103 | 200,000 copper | No active exact producer found. Sapphire-dragon kill and CARVE do not manufacture this kind; supplied exact stock remains accepted |
| [Lord: potion](../../../areas/qst/mntcastl.qst#L83) | One granular potion 37105 | 200,000 copper | Optional granular-batch history; supplied potion skips it. Narrated rehabilitation is not an NPC stat change |
| [Lord: gem](../../../areas/qst/mntcastl.qst#L137) | Small black gem 37104 | Golden black-gem key 37100 | Lich stock at spire top; optional onyx-key access. Reward unlocks catacomb gate and survives ordinary unlock |
| [Lord: tooth](../../../areas/qst/mntcastl.qst#L105) | Small dragon tooth 37114 | Dragon-tooth key 37115 | Spectral-warrior stock in catacombs; optional gem/key route. Foreign Pit of Dragons gate uses tooth key, which breaks on ordinary successful unlock |
| [Granular preparation](../../../areas/qst/mntcastl.qst#L237) | Sand 37106 **and** recipe 97903 together | Three separately declared granular potions 37105 | Both inputs consumed, including scroll. Item count and indexed outcomes must be retained; service is not lord completion |
| [Frost preparation](../../../areas/qst/mntcastl.qst#L255) | Pure white dragon horn 37108 | Frost potion 37109 | Crystal dragon's exact hidden stock; blue and blackened horns do not substitute |
| [Protective preparation](../../../areas/qst/mntcastl.qst#L272) | Curved iron rod 37110 | Protective potion 37111 | One prison golem wields this rod. The shop wand is a different kind |
| [Home preparation](../../../areas/qst/mntcastl.qst#L286) | Small white flower 37121 | Home potion 37122 | Hidden O stock in youth hiding place 37506. Pink shrub flowers, leather belt and garden-access story are unrelated |

The lord and frail man begin at 37261 and 37440. The lord's own scene has three
mage followers and two elite followers. Dialogue does not require killing the
giver or companions; no personal defeat is required by any Q binding.

Local sources are finite, cap-bound reset stock. Water dragon 37101 at 37457
wears the hidden sand. Crystal dragon 37134 at 37548 carries the white horn;
onyx dragon 37133 at 37526 carries an unrelated blackened horn. Sapphire
dragon 37132 at 37565 has no horn G/E. Lich 37105 at 37342 carries the black
gem. Six stair guards 37106 begin around 37288–37290, but only the second of
four at 37288 carries onyx key 37112. Two golems 37120 at 37346 hold different
items: one G decayed key 37346, the other E curved rod 37110. Spectral warrior
37108 at 37388 carries tooth 37114. The plaque O at 37391 gives prophecy,
not an accepted READ receipt. Flower O at 37506 is hidden and needs actual
successful discovery/recovery; its neighbouring shrubs' pink blooms are prose.

The hermit's [recipe exchange](../../../areas/qst/surfacemini.qst#L33) consumes
both exact clothing kinds. Its ownership stays in `surfacemini`, even when
an optional receipt explains local preparation. The lord consumes only one
granular potion, leaving the other output roots subject to actual custody.
Worn, nested, consumed or traded ingredients remain missing despite earlier
history. Donor transfers are valid supplies; they are not first personal recovery.

## Loaded access, keys and services

The Surface boundary is 563532E ↔ 37416W. The road leads to Cheltenham's river
bridge and Chiltern Way. From village-square room 37422, enter the inn west,
then south, west, north, down twice and west to laboratory 37440. Further west
leads into limestone caves and the described underwater passage to sand source
37457. These passage rooms retain indoor sector zero rather than an enforced
underwater sector: do not infer a swimming prerequisite from prose alone.

Plateau waterfall 37117 has a downward exit to mines 37523 and an ordinary
return. Raw state four is masked to zero in
[the loader](../../../src/world/db.c#L1511), and neither side has a D reset.
Thus this exit starts open and visible despite hidden-opening prose; SEARCH
is not required to enter. Similarly, prison north 37346 uses raw seven for
door/pickproof but its D reset is only closed/locked, without SECRET. Reverse
37347S is a pickable door keyed zero. A successful unlock synchronizes the
reciprocal lock; key zero does not remove the loaded doorway. This asymmetric
metadata is recorded for builder review rather than called a broken return.

The onyx-key stair 37288S begins closed/locked; its north reverse starts
closed. Onyx key value[1]=100 means ordinary successful unlocking breaks it.
The prison's northern hidden wall 37353N ↔ 37356S has D state five, so SEARCH
and OPEN matter there. Descend the dark stair into neglected tunnels. At
37384E, D state six makes the golden-key catacomb gate secret/closed/locked;
its reverse 37385W starts closed/locked. Golden key value[1]=0 survives ordinary
unlocking. Later reading the plaque or recovering the tooth is separate.

Lord room 37261S leads through 37574 and the foreign boundary 15968. The tooth
key fits 15968S ↔ 15967N, whose foreign resets make it closed/locked. Value[1]=100
breaks that key on ordinary successful unlocking. Public/opened routes, picking,
supplied materials and prior receipts must not be forced into one personal
key-acquisition sequence. Foreign travel/discovery belongs to actual rooms,
not to a local receipt or the mere existence of a boundary.

Both room inn bindings remain services independent of absent ROOM_INN flags.
Carmotee's table-derived epic teacher is separate from ACT_TEACHER. Shop rod
37140 is a wand, not golem weapon 37110. The memory, amethyst, blank notebook,
village well prototype, prisoners, diplomacy and carved scenery create no
additional native accepted offering. Gruneegth totem 37117 is worn by the
prison shaman and [accepted in Alatorin](../../../areas/qst/alatorin.qst#L6323)
for belt 83495. Neither exchange nor presence proves prisoner liberation.

## Capability gaps and fair repair proposals

| Finding | Evidence and practical limit | Plan before objective or repair |
| --- | --- | --- |
| Missing blue-horn supply | Exact accepted prototype exists; no active reset/reward/custom producer found. Ordinary CARVE creates prototype 8 and is humanoid-only | Builder confirms intended sapphire source, frequency and cap, then adds an isolated reset/grant repair with accepted recovery and donor alternatives. Do not silently create a drop or remove accepted supplied stock |
| Recipe dependency and foreign fee discrepancy | Hermit requires both clothes; podaling quotes 9/6 platinum but accepts 1,000/500 copper. Paid routes refuse active accounting | Decide intended prices separately; repair captions or contracts in a named fix commit. Qualify atomic fee/item admission and settlement before removing guards. Supplied clothes/recipes remain valid branches |
| Exact batch lineage | Sand plus consumed foreign scroll produces three identical-kind roots, then lord consumes one | Freeze both selected input roots and all three indexed output identities. Persist separate source, transfer, consumed-recipe and batch lineage; qualify allocation failure, replay, retry and cold recovery |
| Loader versus reset state | Raw hidden flags are stripped; waterfall and prison portal start visible, northern wall and catacomb gate are secret through D | Extend audit to report decoded door type plus reset CLOSED/LOCKED/SECRET/BLOCKED state. Builder selects visibility intent before changing resets; qualify public routes and reset conflicts |
| Described water and mechanisms | Indoor passage sector; drawbridge chains and bridge-collapse warnings lack bound controllers; prison cell wards have ordinary exits | Builder chooses atmospheric prose or explicit mechanics. Add accepted movement/door/controller/hazard events and fail/restart tests; do not fabricate belt, PULL, ward-word or swim prerequisites |
| Potion effects and after-use | Potion values are real; Q narration alone does not quaff them, heal the lord or damage the player during tooth acceptance | Distinguish accepted craft from actual admitted consumption, spell outcomes, movement and survival. Review potion economic mutation coordination before new durable use milestones; do not infer a guaranteed successful home journey |
| Larger dragon campaign | Lord's repentance/prophecy and plaque imply a campaign; Qs impose no order and tooth key opens foreign content | Builder defines optional local/supplied paths, specific foreign encounter/victory predicate, owned ALL/ANY completion and any NPC restoration. Keep four independent outcomes until real campaign endpoints are qualified |
| Ancillary lore and unfinished leads | Notebook is blank; well lacks local O stock; repair/diplomacy/prison stories have no accepted predicates | Record these as intent leads, not broken quests by default. Builder decides to retain scenery, author endpoints, or repair demonstrably missing source with a separate fix and news evidence |
| Accounting and renewal | Item-only offers qualify as candidates; fresh paid crafts/epic purchases remain guarded. Source and key stock are cap-bound | Active, ready accounting remains required for new tracking. Qualify native offer/settlement, sender provenance, reset generation, recipient availability and daily renewal before claiming played completion |

## Actual repair and news handoff

**Fix commit:** [45bb3c948](https://github.com/Community-Duris/Duris/commit/45bb3c948),
**Fix Du'Maathe northwest parapet direction clue**.

**Trigger:** LOOK at room 37248. Its title and the intersecting northern/western
wall roads identify the northwestern corner, but its copied prose said south.
**After:** that one description word now agrees with the existing title and
reciprocal routes. The actual southwestern corner 37238 keeps its correct clue.
No exit, door, reset, quest, reward, prototype or executable behavior changes.

**Proof:** the [focused regression](../../../tests/async/test_mntcastl_parapet_clue.py)
fails on the original description and passes on the corrected title/prose and
both reciprocal wall-road routes. Exact byte comparison confirms a single-word
native change. Live LOOK/traversal remains unqualified. This repair does not
restore the horn source, alter visibility, correct foreign prices, add a controller
or implement a dragon campaign.

**News sentence:** “Du'Maathe Castle's northwest parapet now correctly identifies
its position in the room description.”

## Qualification and publication boundary

Focused source, catalog and Python/C++ projection tests cover all eight exact
contracts, 15 aliases, optional foreign/local history, sand-only missing recipe,
three declared outputs, supplied potion/tooth, wrong horns/rod, worn materials,
spent keys/history, independent outcomes, canonical foreign ownership, replay
and cold recovery. Full production regression, maintained build, changed/staged
formatting, source links, original queue and prior-journal/definition preservation
are required before publication. Synthetic receipts qualify projection behavior,
not a played craft, source recovery, door unlock, wallet operation or reward settlement.

No accounting activation, account/player operation, live database qualification,
server operation, migration, deployment or merge is part of this checkpoint.
Catalog: **95 journals, 1,597 achievement units, 1,449 potential dailies, 2,203 rows**.
All 2,668 definitions, fingerprint, content revision, registry and prior 94
journals remain intact. Queue: **75/220 source-comprehensive, 145 pending;
Tundra next**. The full roadmap goal remains active.
