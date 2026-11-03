# Taliccicopiid (`thri`): source story review

**Scope:** source review of 155 WLD rooms, 66 mobile prototypes, 36 local
object prototypes, one shop record, zone resets, and the authored story
sidecar. This maps source narrative, static routes, and typed references; it
does not qualify live combat, shop behavior, special procedures, or safe
traversal.

## Story coverage state

`thri` is an active `areas/AREA` contributor, listed as `*676-677`, with zone
header 676, title Taliccicopiid, and last room 67755
([AREA entry](../../../areas/AREA#L256)). The coverage row records 155 rooms,
66 mobiles, 36 objects, no local QST giver records, and zero native Q/QA
contracts ([coverage row](ZONE_COVERAGE_REGISTER.csv#L301)). It also records
membership in the separate `areas/qst/AREA` list
([index entry](../../../areas/qst/AREA#L230)), but there is no
`areas/qst/thri.qst` source file.

The authored sidecar says `coverage: complete` and introduces the area as a
place to meet residents and “uncover local requests,” while its `contacts`,
`stories`, and `exclusions` arrays are empty
([coverage and introduction](../../../areas/story/thri.story.json#L5),
[contacts](../../../areas/story/thri.story.json#L11),
[stories](../../../areas/story/thri.story.json#L12),
[exclusions](../../../areas/story/thri.story.json#L13)). The source set therefore
maps no local quest contact or objective. That is a source inventory result;
it does not establish that no mobile procedure or other runtime behavior
exists.

## Environmental story

A source-text grouping finds 50 distinct exact room title/description pairs
across the 155 rooms. Repeated forest paths, tunnels, living areas, fields,
alcoves, and caverns shape a large colony map; those repeated descriptions do
not establish duplicate rooms or a replacement area.

| Setting | Source evidence | Story cues |
| --- | --- | --- |
| Ravine, rainforest, and human mission | [Large Ravine](../../../areas/wld/thri.wld#L2), [reception room](../../../areas/wld/thri.wld#L60), [missionary lounge](../../../areas/wld/thri.wld#L107), [missionary cabin](../../../areas/wld/thri.wld#L532), [river edge](../../../areas/wld/thri.wld#L555) | The entry sits in a crater-like ravine beneath enormous trees. A human religious mission built a reception room and lounge, but the rooms describe the mission's violent destruction. The resets still place a surviving and a terrified missionary in the former mission district ([resets](../../../areas/zon/thri.zon#L43), [survivor](../../../areas/mob/thri.mob#L797), [terrified missionary](../../../areas/mob/thri.mob#L811)). |
| Forest canopy and wildlife | [tree routes](../../../areas/wld/thri.wld#L40), [grove pond](../../../areas/wld/thri.wld#L622), [grazing field](../../../areas/wld/thri.wld#L674), [treehouse lookout](../../../areas/wld/thri.wld#L733) | Large climbing trees, branches, river, wildlife, and a concealed scout lookout create an overgrown surface approach. The grazing field says the thri-kreen seized livestock from nearby human settlements. The treehouse description warns of fatal or disabling falls, but the static census records no configured fall-chance rooms. |
| Thri-kreen settlement and hierarchy | [living cavern](../../../areas/wld/thri.wld#L643), [training area](../../../areas/wld/thri.wld#L659), [Queen's den](../../../areas/wld/thri.wld#L2332), [King's cavern](../../../areas/wld/thri.wld#L2897), [warrior cave](../../../areas/wld/thri.wld#L3035) | The underground colony has sleeping caverns, warriors, sentries, a queen, a king, and a named warrior teacher. Training, guard posts, a royal den, and a chief's cavern make the social and military hierarchy explicit. Mobile descriptions for the Queen and King contain coarse reproductive dialogue ([Queen](../../../areas/mob/thri.mob#L345), [King](../../../areas/mob/thri.mob#L358)). |
| Enslavement, farming, and raids | [corn fields](../../../areas/wld/thri.wld#L2608), [holding cell](../../../areas/wld/thri.wld#L3055), [second holding cell](../../../areas/wld/thri.wld#L3076), [cooking fire](../../../areas/wld/thri.wld#L3015), [loot niche](../../../areas/wld/thri.wld#L3239) | The fields and cells describe captives from several peoples being shackled, beaten, and forced to farm. The cooking-pit description includes arms and heads among the roasting food; a hidden niche stores goods taken in raids on settlements and merchant caravans. This is a strongly violent slaver-colony setting, not a mapped rescue or liberation quest. |
| Wild Underdark passages | [shaft](../../../areas/wld/thri.wld#L749), [twisting tunnels](../../../areas/wld/thri.wld#L827), [phosphorescent cavern](../../../areas/wld/thri.wld#L1877), [cracked tunnels](../../../areas/wld/thri.wld#L1939), [junk dealer](../../../areas/wld/thri.wld#L2593) | Natural shafts and long, repetitive gray tunnels descend through glowing slime, domed caverns, smoke-filled passages, and the inhabited colony. The single local shop is described as a junk dealer whose stock comes from the Queen's campaigns. |

The river prose says its current runs east, but the static coverage row records
zero configured current rooms. The same row records zero fall-chance rooms
despite the treehouse warning. Treat those descriptions as narrative cues, not
proof of a live current or falling mechanic, until route behavior is separately
qualified ([coverage row](ZONE_COVERAGE_REGISTER.csv#L301),
[river](../../../areas/wld/thri.wld#L555),
[treehouse](../../../areas/wld/thri.wld#L733)).

## Residents, reset placements, and the shop

The reset file has 213 typed commands: 145 `M`, 28 `E`, 28 `G`, 6 `D`, 4
`O`, and 2 `P`. It places the Queen in room 67712 and the King in 67738,
warriors around the colony, captive species in the cells and fields, and
missionaries in the former mission. The source descriptions and static resets
do not define player objectives or outcomes.

The shop file contains one record: keeper 67616 in room 67724, the Junk Smith
Dealer ([shop record](../../../areas/shp/thri.shp#L1),
[shopkeeper reset](../../../areas/zon/thri.zon#L186)). Its stock lists four
local steel weapons, plus Heavens prototypes. The zone reset's `G` commands
for the dealer also reference Heavens objects
`209`, `211`, `214`, `215`, `217`, `221`, `226`, `254`, `256`, `260`, `347`,
`363`, `364`, `368`, `369`, `376`, `377`, `384`, `429`, and `431`
([reset stock](../../../areas/zon/thri.zon#L191)). Every one resolves to an
object prototype in active Heavens sources
([AREA entry](../../../areas/AREA#L2); examples:
[209](../../../areas/obj/heavens.obj#L2426),
[254](../../../areas/obj/heavens.obj#L2804),
[347](../../../areas/obj/heavens.obj#L3600),
[363](../../../areas/obj/heavens.obj#L3783),
[376](../../../areas/obj/heavens.obj#L3925),
[429](../../../areas/obj/heavens.obj#L4546)). The shop stock overlaps the
reset-provided list but omits 221 and 226; preserve both declarations pending
owner review. The shop file's configured refusal and greeting strings also
contain profanity and a sexual threat ([first message](../../../areas/shp/thri.shp#L39),
[threat](../../../areas/shp/thri.shp#L40)).

## Keyed doors and static connectivity

Three paired door boundaries are declared by the zone resets:

| Boundary | Source state | Notes |
| --- | --- | --- |
| 67627–67636, ravine and vertical shaft | Closed | The comments name each side ([resets](../../../areas/zon/thri.zon#L8)). |
| 67686–67687, Wild Underdark tunnel | Closed and locked | Both WLD exits name object 67633 as their key ([first exit](../../../areas/wld/thri.wld#L1816), [return exit](../../../areas/wld/thri.wld#L1832)); 67633 is an iron key ([prototype](../../../areas/obj/thri.obj#L479)) reset with a warrior in room 67686 ([reset](../../../areas/zon/thri.zon#L151)). |
| 67754–67755, hidden loot niche | Closed and secret | The exit descriptions use a rock door, and the reset comments mark the route secret ([WLD exit](../../../areas/wld/thri.wld#L3236), [reset](../../../areas/zon/thri.zon#L24)). |

The census records 339 raw exits, all 339 loader-resolved, with all 155 rooms
in one weak local component. There is one outgoing and one incoming cross-area
exit, with a reverse record. The only static boundary pair is to Surface:
room 67600 exits east to room 603653, and Surface returns west to 67600
([outbound pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L656),
[inbound pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L608),
[Thri exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L838),
[reverse exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1581)). The static graph does
not establish that ordinary players can enter, leave, pass the locked doors,
or return safely under live conditions.

## Dependencies and replacement boundary

A current-source search across reset, QST, and shop files found no references
from other zones to Thri mobile or object VNUMs. The Thri source has no local
QST file. Its outbound reset and shop references import Heavens objects as
listed above; Heavens is an active area. This is a narrow typed-source
crosswalk. Special procedures, editor tooling, generated identities, and
runtime behavior remain outside the completed closure.

The normalized title scan records no repeated active area title for Thri, and
the replacement report identifies no Thri-specific retirement or replacement
([coverage row](ZONE_COVERAGE_REGISTER.csv#L301),
[duplicate/replacement report](DUPLICATE_AND_REPLACED_ZONES.md)). Its repeated
room descriptions belong to this large continuous colony map. Keep it separate
from other forest or underdark areas unless source history or a builder
establishes a replacement relationship.

## Builder questions and recommendations

| # | Question and source evidence | Recommended resolution |
| --- | --- | --- |
| 1 | The sidecar calls for local requests, but its `contacts`, `stories`, and `exclusions` arrays are empty ([sidecar](../../../areas/story/thri.story.json#L5)). Is Thri meant to be environmental-only? | Make the sidecar scope and introduction agree with the intended authored content; add no rescue, conversion, or liberation objective without a named owner and outcome. |
| 2 | `thri` is listed in `areas/qst/AREA` ([index](../../../areas/qst/AREA#L230)), but `areas/qst/thri.qst` is absent and coverage records zero local Q/QA contracts. Is the index entry intentional? | Confirm with the QST/index owner; change the list or add a contract only after its source owner and behavior are defined. |
| 3 | The reception room describes a massacre of missionaries, while resets still place a terrified missionary and a survivor ([room](../../../areas/wld/thri.wld#L60), [resets](../../../areas/zon/thri.zon#L43)). Is that timeline and survivor state intentional? | Keep the authored reset and account for the survivor in any revised story; do not infer a rescue chain. |
| 4 | The fields and cells describe captives from several peoples shackled, beaten, and forced to farm ([fields](../../../areas/wld/thri.wld#L2608), [cells](../../../areas/wld/thri.wld#L3055)). Is this intended as background setting? | Preserve it as lore unless a builder supplies an explicit objective and completion controller. |
| 5 | The cooking fire mentions body parts, and Queen/King descriptions contain coarse reproductive dialogue ([fire](../../../areas/wld/thri.wld#L3015), [Queen](../../../areas/mob/thri.mob#L345), [King](../../../areas/mob/thri.mob#L358)). Is the current level of detail intentional? | Ask the area owner to set the intended tone; revise prose only under that decision. |
| 6 | The shop refusal and greeting include profanity and a sexual threat ([messages](../../../areas/shp/thri.shp#L39)). Should these strings remain? | Confirm the intended tone and replace the strings if they fall outside it; keep stock and shop wiring separate from the prose decision. |
| 7 | The river description says its current runs east and the treehouse warns of dangerous falls, while coverage records zero current and fall-chance rooms ([river](../../../areas/wld/thri.wld#L555), [treehouse](../../../areas/wld/thri.wld#L733)). Do these warnings describe mechanics or atmosphere? | Keep prose and configured hazards distinct; describe active danger only after the relevant route behavior is qualified. |
| 8 | Thri has one reciprocal Surface boundary: #603653 west to #67600 and #67600 east to #603653 ([cross-area pair](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L608)). Is this the intended active WLD entrance and exit? | Preserve the pair unless its two area owners agree on another route; do not treat the inactive Surf record as loaded ingress. |
| 9 | Raw directed WLD traversal from #67600 reaches 145/155 rooms; ten mission and muddy-tunnel/cell rooms are outside that traversal. Should these pockets remain outside the authored WLD path? | Confirm whether their access is intentionally supplied by reset type-25 items or another documented mechanic; add no ordinary exit based on room prose alone. |
| 10 | Four local type-25 objects target #67734, #67713, #67624, and #67600; only objects #67600 and #67603 have local resets ([objects](../../../areas/obj/thri.obj#L1), [reset placements](../../../areas/zon/thri.zon#L33)). Are the unused declarations intentional? | Preserve the exact targets and reset owners; confirm activation and intended availability before describing them as player routes. |
| 11 | Zone resets close pairs #67627/#67636, lock #67686/#67687 with key #67633, and mark #67754/#67755 closed/secret ([reset flags](../../../areas/zon/thri.zon#L8)). Are all three gate states intended? | Keep each reciprocal state and key reference paired; do not unlock, expose, or make one-way changes without the builder's route decision. |
| 12 | The hidden loot niche is behind a secret, closed route from #67754 to #67755 ([exit](../../../areas/wld/thri.wld#L3236), [reset](../../../areas/zon/thri.zon#L24)). Is its discovery method sufficient? | Preserve the hidden route and reset flags; document discovery guidance only after the builder confirms intended clues. |
| 13 | Key #67633 is reset with a warrior at #67686, the same room as the locked tunnel pair ([reset](../../../areas/zon/thri.zon#L151)). Is that acquisition location deliberate? | Keep the key's source and locked directions together; don't promise availability or a safe retry from static resets alone. |
| 14 | The inactive Surf candidate has #250600 west to Thri #67600, while the same source VNUM in `surface2011.wld` has a different title and no such exit ([Surf source](../../../areas/wld/surf.wld#L1061752)). Should the historical approach remain candidate evidence? | Keep it source-only on Surf row 97 and retain the separate Surface2011 identity; do not load either source to restore ingress. |
| 15 | Compiled code assigns the `inn` procedure at room #67710, where reset mobile #67665 is placed ([assignment](../../../src/specs/specs.assign.c#L2359), [reset](../../../areas/zon/thri.zon#L179)). Is this the intended Thri inn? | Preserve the code assignment and mobile placement; qualify service behavior separately before changing either owner. |
| 16 | `hometown[]` maps the Thri town to #67723, but the corresponding current `avail_hometowns` row contains only zeros ([home](../../../src/core/constant.c#L1738), [availability](../../../src/core/constant.c#L1831)). Should players be able to select it? | Reconcile the town and availability tables with the race/home owner; do not infer eligibility from the single destination entry. |
| 17 | The class `guild_locations` table maps every Thri-town class slot to #67745 ([table](../../../src/core/constant.c#L1971)), even though the Thri hometown availability row is empty. Is that fallback still intended? | Keep the code mapping visible and ask its owner to confirm; do not relocate #67745 or change access from the table alone. |
| 18 | Thri's zone resets import twenty Heavens object prototypes, while the local shop lists eighteen of those alongside four local weapons ([resets](../../../areas/zon/thri.zon#L191), [shop](../../../areas/shp/thri.shp#L1)). Are reset stock and sale stock intentionally different? | Preserve Heavens prototype ownership; coordinate any stock edits with the Heavens and Thri owners. |
| 19 | Heavens objects #221 and #226 are reset onto the dealer but do not appear in the shop's product list ([resets](../../../areas/zon/thri.zon#L200), [shop list](../../../areas/shp/thri.shp#L1)). Is this omission intentional? | Keep reset and product records distinct; confirm whether these two items should be sold before changing the shop. |
| 20 | Maintained code names Thri town at #67723, maps guild locations to #67745, and assigns an inn at #67710, while the sidecar and QST source define no local request ([code references](../../../src/core/constant.c#L1738), [inn](../../../src/specs/specs.assign.c#L2359)). Which of these are active player-facing services? | Keep the current owners and source mappings; describe only services whose runtime availability the relevant code and area owners confirm. |

## Safe resolution summary

Keep Taliccicopiid and all current sources in place. Preserve the failed mission, thri-kreen hierarchy, captive labor, colony economy, hidden loot route, reciprocal Surface pair, code-level town/inn references, and Heavens stock owners. Keep raw WLD routes separate from item portals and runtime access. Reconcile the empty sidecar, absent QST source, content tone, and code-table decisions with their owners before changing story text, access, or stock. No gameplay change or attic move is proposed.

## Checkpoint 388: graph, identity, and typed dependency refresh

This refresh completes the source-backed review of physical coverage row 301.
The existing story review above remains the environmental dossier; the twenty
builder questions now separate owner decisions from static source findings.

### Active and inactive WLD boundaries

The active `thri` WLD contains 155 rooms and 339 authored exit directions:
338 resolve locally and one leaves the zone. There are no loader-unresolved
or negative targets. All 155 rooms form one weak component. The active
Surface arrival is #603653 west to Thri #67600, paired with Thri #67600 east
to Surface #603653 ([inbound register row 1581](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1581),
[outbound row 838](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L838),
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L656)).

Raw directed traversal from the active arrival room #67600 reaches 145 of 155
rooms. The ten not reached by authored WLD exits are #67602-#67604, #67624,
#67734-#67737, and #67746-#67747; every room in the map can reach #67600 in
the reverse graph. These are WLD-only results: local type-25 objects and
reset/door state are separate route sources, and no player access or safety
claim follows from the graph census.

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 directions
without issues or a room-VNUM overlap with Thri. It found one source-only
approach: inactive Surf #250600 west (D3) to Thri #67600
([source room](../../../areas/wld/surf.wld#L1061752),
[exit](../../../areas/wld/surf.wld#L1061768)). This is not active ingress.
Curated source-only WLD totals rise from 9,474 to 9,475 overall and from 292
to 293 for Surf. Surf's 145 active unresolved-reference records and 17
active source-area keys are unchanged on existing candidate row 97.

### Title, VNUM identity, and tracked history

After color-token removal, tilde trimming, whitespace normalization, and
case folding, “Taliccicopiid” is unique among all 442 current ZON files.
Active same-type WLD/MOB/OBJ overlap is zero; the all-95 inactive WLD scan
found no room overlap; inactive same-type MOB/OBJ overlap is also zero.
Followed `thri` WLD, MOB, OBJ, ZON, and SHP histories remain on the same
source stem through the tracked 2009 area-tree move and later same-stem
edits; no alternate-stem predecessor, explicit rename, or whole-zone
replacement appears in those bounded paths. The 2026 story sidecar is a
separate addition, not replacement evidence.

Inactive Surf and Surface2011 both declare room VNUM #250600 with different
normalized titles and topology; only Surf has the west exit into Thri.
Keep the source candidates distinct and retain both maps in place. No Thri
attic candidate or migration is supported.

### Typed source and maintained-code closure

The current reset file has 213 typed commands: 145 M, 28 E, 28 G, six D,
four O, and two P. Its mobile resets use local Thri prototypes. It imports
twenty Heavens object prototypes (#209, #211, #214, #215, #217, #221, #226,
#254, #256, #260, #347, #363, #364, #368, #369, #376, #377, #384, #429,
and #431). The one local shop lists four local steel weapons and eighteen
Heavens products; reset imports #221 and #226 are not shop products. The
shopkeeper, shop reset, and stock remain separate source owners.

No active or inactive external ZON reset, QST source, SHP product, or story
sidecar numeric field references a Thri mobile/object prototype or room.
There is no `thri.qst` source file, although `areas/qst/AREA` lists the stem.
Across 872 source type-25 declarations, none targets a Thri room. Thri itself
declares four type-25 destinations, all within the area (#67734, #67713,
#67624, and #67600); only local objects #67600 and #67603 have zone reset
placements. Treat these object routes separately from the WLD reach result.

All current-source literal Thri room-VNUM hits in `src/` are classified:
`specs.assign.c` assigns `inn` at #67710; `constant.c` maps the Thri town to
#67723, leaves the matching hometown-availability row empty, and maps every
class slot in the Thri guild-location row to #67745. No direct maintained
code reference to a Thri mobile/object VNUM was found. These are source-level
bindings, not proof that the service, hometown choice, guild command, shop,
or type-25 route is live or reachable in a running game.

No WLD, prototype, QST, sidecar, code, reset, shop, or migration change is
proposed. Keep active `thri` DO NOT MOVE and preserve the source-only Surf
approach on existing candidate row 97.

## Checkpoint 843: Current typed stock and room references refreshed

The current-source pass preserves the source boundary already recorded in
Checkpoint 388. Thri still has no local QST file; its complete sidecar has
empty `contacts`, `stories`, and `exclusions` arrays, while `areas/qst/AREA`
retains a `thri` entry. Do not turn the index or sidecar introduction into a
new quest contract without its owner. ([sidecar](../../../areas/story/thri.story.json#L5),
[QST index](../../../areas/qst/AREA#L230))

The reset file still imports twenty Heavens objects for the dealer. The local
shop lists eighteen Heavens products plus four local weapons; reset-only Heavens
#221 and #226 stay separate from sale stock. No outside ZON reset, native QST,
SHP product, sidecar item field, type-25 destination, or same-type prototype
reference into Thri was found. Keep Heavens prototype ownership and distinguish
reset stock from shop products. ([reset stock](../../../areas/zon/thri.zon#L191),
[additional reset-only objects](../../../areas/zon/thri.zon#L200),
[shop stock](../../../areas/shp/thri.shp#L1))

Four Thri OBJ type-25 declarations target local rooms #67734, #67713, #67624,
and #67600; only objects #67600 and #67603 have local ZON placements. These
item destinations remain separate from the WLD graph and do not prove that a
portal is available. Maintained code continues to bind the inn at #67710, list
hometown #67723 while its matching availability row is empty, and map guild
locations to #67745. The code scan still finds no direct maintained binding to
a Thri-local MOB or OBJ VNUM. ([portal declarations](../../../areas/obj/thri.obj#L1),
[portal placements](../../../areas/zon/thri.zon#L33),
[inn](../../../src/specs/specs.assign.c#L2359),
[hometown](../../../src/core/constant.c#L1738),
[availability](../../../src/core/constant.c#L1831),
[guild locations](../../../src/core/constant.c#L1971))

The reciprocal Surface pair remains the sole active cross-area WLD boundary.
Inactive Surf #250600 west to Thri #67600 remains source-only on candidate row
97; retain Surface and `surface2011` as separate map identities. No live stock,
portal activation, hometown eligibility, or player access is inferred. Keep
active Taliccicopiid **DO NOT MOVE**.
