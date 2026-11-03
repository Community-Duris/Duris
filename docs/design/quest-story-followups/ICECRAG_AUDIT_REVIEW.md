# IceCrag Castle: source-only boundaries and typed-reference supplement

Status: repository-wide static supplement to the existing
[IceCrag Castle dossier](../zone-stories/ICECRAG_CASTLE.md). It reconciles the
active WLD boundary, current unregistered WLD inputs, direct external numeric
references to local prototypes, and the active-title/history boundary. No zone
source, story sidecar, or gameplay code was changed or qualified.

## Active and source-only WLD boundaries

IceCrag has 628 authored WLD exit records, 627 loader-resolving records, three
active inbound and three active outbound cross-zone records, two reciprocal
boundary pairs, one positive unresolved destination, and one local weak
component spanning all 243 rooms. The two reciprocal active boundaries are
Surface and Voluntown. Winterhaven has a one-way active inbound to room 97121;
room 97153 has an outbound to Lava Tubes. Room 97085 also has a down exit to
absent room 97352. These records are in the
[active cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) and
[unresolved-exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv).

| Active boundary record | Evidence and structural reading |
| --- | --- |
| IceCrag room 97001 west ↔ Surface room 511914 east | [IceCrag exit](../../../areas/wld/icecrag.wld#L20), [Surface exit register row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1322) |
| IceCrag room 97132 north ↔ Voluntown room 142400 south | [IceCrag exit](../../../areas/wld/icecrag.wld#L3415), [Voluntown exit](../../../areas/wld/Voluntown.wld#L12) |
| Winterhaven room 55634 northeast → IceCrag room 97121 | [Winterhaven exit](../../../areas/wld/wh.wld#L13121); no reverse WLD record |
| IceCrag room 97153 south → Lava Tubes room 12049 | [IceCrag exit](../../../areas/wld/icecrag.wld#L4043); no reverse WLD record |
| IceCrag room 97085 down → absent room 97352 | [unresolved target](../../../areas/wld/icecrag.wld#L2160); the target is not in the active room table |

### Local directed graph

The 243-room local graph has one weak component and 23 strongly connected
components, sized 219, 3, and twenty-one singletons. From the Surface-side
analysis seed #97001, 220 rooms are reachable by authored local WLD exits and
239 rooms have a local WLD path back to #97001. The 23 rooms outside forward
reach are #97005, #97131, and #97133–#97153. Rooms #97121, #97133, #97149, and
#97153 have no authored local WLD return to the seed. These counts exclude
type-25 portal movement and do not model door/reset state, so they describe
source topology rather than player access or safety. See the [local WLD
source](../../../areas/wld/icecrag.wld).

I parsed the authored D0–D9 records in all 95 current unregistered WLD files:
2,080,773 exits, with zero parse issues. Two source-only records target the
active room set:

| Unregistered source record | Direction and active target | Evidence |
| --- | --- | --- |
| `surf.wld` room 215567 | East (`D1`) to IceCrag room 97001 | [exit target](../../../areas/wld/surf.wld#L326610) |
| `underdark2011.wld` room 224716 | East (`D1`) to IceCrag room 97001 | [exit target](../../../areas/wld/underdark2011.wld#L109754) |

These exits come from unregistered WLD source, not the normal active-area
input. They are not active-world entrances and do not change the 628/627 active
census. They add two individually reviewed records to the curated source-only
list. Later repository-wide checkpoints have continued to add reviewed
approaches; current totals are summarized in the [handoff](HANDOFF.md). These
records do not prove ordinary access, safe travel, or a return route.

## External typed references to IceCrag prototypes

The existing dossier maps IceCrag's local requests, foreign recipe inputs,
shops, reset groups, imported objects, procedure bindings, and travel
mechanisms. This exact numeric scan adds or cross-checks the following direct
external records. Reset and quest declarations identify possible source or
consumption paths; they do not prove current stock, access, acceptance, or
completion.

| Current source | IceCrag prototypes and typed role | Evidence |
| --- | --- | --- |
| Alatorin quest | Two forge QA blocks each declare results 97092, 97051, and 97052: three local armor prototypes across six result rows | [Alatorin QST results](../../../areas/qst/alatorin.qst#L4213), [Alatorin story mapping](../../../areas/story/alatorin.story.json#L1835) |
| Alatorin reset | Closet object 83114 in room 83769 contains IceCrag hat 97041, jacket 97047, and pants 97048 | [reset rows](../../../areas/zon/alatorin.zon#L1739), [IceCrag objects](../../../areas/obj/icecrag.obj#L575) |
| Barovia2 reset | Ten `P` rows declare local coin-pile object 97097 inside storage chest 58832 | [reset group](../../../areas/zon/barovia2.zon#L107), [coin pile](../../../areas/obj/icecrag.obj#L1446) |
| Ravenloft reset | A 30-percent `G` row declares local object 97118 in the selected Bram Burns follower load | [reset](../../../areas/zon/ravenloft.zon#L1188), [assigned object](../../../areas/obj/icecrag.obj#L1770) |
| Air quest | The local ethereal shroud 97066 is a required item in Air's nine-kind relic contract | [Air QST requirement](../../../areas/qst/airp.qst#L116), [Air story mapping](../../../areas/story/airp.story.json#L495), [shroud](../../../areas/obj/icecrag.obj#L945) |
| Sarmiz quest | King Rodev's contract consumes local root vial 97135 and dusty ampoule 97099 with Sarmiz vial 9445 | [Sarmiz QST requirements](../../../areas/qst/sarmiz.qst#L296), [Sarmiz story mapping](../../../areas/story/sarmiz.story.json#L458), [root vial](../../../areas/obj/icecrag.obj#L2028), [ampoule](../../../areas/obj/icecrag.obj#L1472) |
| Chaos equipment tables | Standard and enhanceable kit tables reference the IceCrag red earthstone ring 97015; Bard kit tables reference iron-bands belt 97122 | [kit entries](../../../src/account/chaos_eq_data.h#L106), [Bard entry](../../../src/account/chaos_eq_data.h#L202), [ring](../../../areas/obj/icecrag.obj#L241), [belt](../../../areas/obj/icecrag.obj#L1829) |

An inactive source adds one separate legacy record: unregistered
`brass-old-1.zon` has a `G` reset for IceCrag book 97006. It is not an active
reset consumer and remains a source-path dependency to retain in a separate
candidate review. [Legacy reset](../../../areas/zon/brass-old-1.zon#L823) ·
[book object](../../../areas/obj/icecrag.obj#L96).

Outside IceCrag's own source group, the exact numeric scan found no other
direct references in active MOB, OBJ, ZON, SHP, QST, or story-sidecar files
beyond the records above and the separate inactive Brass reset. The code scan's
other IceCrag-number matches are the local assignments and behaviors already
mapped by the dossier. Editor/tooling references, persisted-player state, and
indirect or dynamically constructed references remain outside this bounded
closure. Current availability, shop transactions, and gameplay outcomes also
remain outside the checked-in typed-reference review.

## Active title and tracked history boundary

The normalized active title “Ice Crag Castle” has one owner: area `icecrag`,
zones 970–972. Its tracked WLD/ZON path history shows continued edits on the
same stem and no named distinct predecessor or replacement.

The title and source comparison also reviewed the similar active names. `Ice
Tower` is a separate title and contributor. The closest named and connected
contributor is `Voluntown` zone 1424, titled “The Royal Mausoleum of Castle
IceCrag,” with its own rooms #142400–#142532, source dossier, QST, resets, and
story. The reciprocal IceCrag #97132 / Voluntown #142400 boundary records the
connection between the contributors; it does not combine their ownership or
make one a predecessor or replacement. Other Castle-titled areas in the
active inventory, including Bastine, Apocalypse, Quintaragon, Du'Maathe,
Ravenloft, and the Valoisian Castle, have distinct titles and source owners.
See the [Royal Mausoleum dossier](../zone-stories/THE_ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG.md),
[Mausoleum audit](ROYAL_MAUSOLEUM_OF_CASTLE_ICECRAG_AUDIT_REVIEW.md), and
[title/history register](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-233-icecrag-title-history-and-related-contributor).

[Active title](../../../areas/zon/icecrag.zon#L2) ·
[active-area entry](../../../areas/AREA#L348) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Audit boundary

These are static source findings. They do not establish the use or safety of
any active boundary, the completion of a foreign recipe, current reset stock,
kit selection or delivery, or a working return path. Door/reset state, live
acceptance and settlement, editor/tooling, persisted-player references, and
indirect or dynamic closure remain separate work.

## Checkpoint 796: Faang cuisine item input

IceCrag's QST lists Faang brain #15252 as a `G I` item input for Masha's
cuisine project; the story sidecar repeats it in that excluded contract's
`give` list ([QST](../../../areas/qst/icecrag.qst#L152),
[sidecar](../../../areas/story/icecrag.story.json#L784),
[Faang object](../../../areas/obj/faang.obj#L482)). This is an outgoing typed
dependency from IceCrag to Faang, not a local IceCrag prototype. The recipe
remains excluded pending a complete source-backed item set; see the
[Faang typed audit](FAANG_STORY_REVIEW.md#checkpoint-796-current-typed-census-refresh).

## Checkpoint 802: Foggy Woods wine input

Icecrag's active QST requires two Foggy Woods bottles of cheap white wine
#90017 in `G I` rows at [line 320](../../../areas/qst/icecrag.qst#L320) and
[line 321](../../../areas/qst/icecrag.qst#L321). Its story
sidecar maps the same two-item input through `item_vnums` and the contract
`completion_key` ([sidecar](../../../areas/story/icecrag.story.json#L507),
[item list](../../../areas/story/icecrag.story.json#L515)). Foggy Woods remains
the prototype owner; these source references do not establish active stock or
successful settlement. See the
[Foggy typed audit](FOGGY_WOODS_AUDIT_REVIEW.md#checkpoint-802-typed-census-and-compiled-bindings).
