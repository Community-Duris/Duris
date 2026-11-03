# Juiblex: repository-wide connectivity and typed-closure supplement

Status: static review supplement to the [source-comprehensive story dossier](../zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md) and its [native evidence index](../../reference/zone-story-audits/juiblex.md). It reconciles the current active WLD graph, non-WLD routes, cross-area typed references, code/tooling references, and tracked title history. It does not qualify live access or player state.

## Active WLD graph and reachability

The active `juiblex.wld` contains 183 rooms and 447 authored exits. All 447
targets resolve: 442 exits are local and five leave the area. The local weak
components have sizes 169, 5, 2, 2, and five single rooms. Directed analysis
finds strongly connected components of 157, 5, 4, four components of 2, and
nine single rooms.

Two active WLD records enter Juiblex at distinct rooms. Underdark #772524 goes
down to #87500 and Juiblex #87500 returns up to #772524. Realm room #14208 goes
west to #87628; it is labeled an administrative dispersal room in the source,
not an ordinary public entrance. With every authored local exit treated as
available, #87500 reaches five rooms, while #87628 reaches 157 rooms and 168
rooms have authored paths back to #87628. These counts exclude object portals,
scripted transfers, exit flags, reset availability, and fall behavior.
[Juiblex entrance and exits](../../../areas/wld/juiblex.wld#L8) ·
[Underdark reciprocal edge](../../../areas/wld/underdark.wld#L115304) ·
[Realm administrative entry](../../../areas/wld/realm.wld#L4656) ·
[active cross-area exit rows](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1086) ·
[pair summary rows](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L213)

Five outbound WLD records lead from Juiblex #87500 to Underdark #772524 and
from staging rooms #87671–#87674 to Fields Between #71106, Torg #29073,
Mountain #9106, and Scorched Valley #71221. The four staging exits are down
exits to foreign load rooms; their source rooms have no F fall metadata and
sector zero, so they are not automatic chutes. Ordinary NPC wandering may
carry eligible fugitives across them, which still does not prove an intended
player escort or return. The connected areas retain separate owners.
[outbound exit records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1087) ·
[Juiblex staging and wandering review](../zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md#access-controls-and-actual-procedures)

The all-95 inactive-WLD census recorded no target into rooms #87500–#87682 and
no overlapping source room. The area adds no Surf or other source-only WLD
approach; curated source-only totals remain 3,471 overall, 229 for Surf, 18 for
`ud_map2`, and eight for Underworld2. These are source-graph results, not proof
of admission, door state, survival, return, or safe travel.
[coverage row 141](ZONE_COVERAGE_REGISTER.csv#L141) ·
[unregistered-WLD coverage](../zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md#evidence-and-review-boundary)

## Typed dependency closure

The local QST file has 23 exact item contracts. It imports Fields Between fez
#71024 and Torg legend halves #55364/#55365, and one contract returns Torg item
#55279. The Juiblex story sidecar also maps brewer #71037 as an optional
producer: Fields Between manuscript #71005 and overripe banana clump #71016
yield fez #71024. These items retain their source owners and optional producer
history.
The only active outside QST item consumer found across all current quest files
is Winterhaven giver #55265, which consumes Juiblex crystal #87510 alongside
two Winterhaven items and returns its own rewards. Its authored sidecar carries
the same exact crystal reference.
[Juiblex Q177 and Q314](../../../areas/qst/juiblex.qst#L177) ·
[Torg legend source](../../../areas/qst/torg.qst#L188) ·
[Winterhaven consumer](../../../areas/qst/wh.qst#L3600) ·
[Winterhaven sidecar](../../../areas/story/wh.story.json#L1067) ·
[Juiblex crystal](../../../areas/obj/juiblex.obj#L137) ·
[optional brewer mapping](../../../areas/story/juiblex.story.json#L689) ·
[Fields Between manuscript](../../../areas/obj/fields_between.obj#L54) ·
[Fields Between banana clump](../../../areas/obj/fields_between.obj#L205) ·
[Fields Between fez](../../../areas/obj/fields_between.obj#L325)

Juiblex resets import five currently defined outside objects: Heavens #360 and
#922, Unique #67274, and Winterhaven #55206 and #55336. No active outside ZON
reset directly consumes a Juiblex mobile or object. One unregistered
`brass-old-1.zon` G command references Juiblex object #87568, the soulful
mandolin; it is source-only and must retain the Brass owner's disposition.
[Heavens and Unique imports](../../../areas/zon/juiblex.zon#L221) ·
[Winterhaven imports](../../../areas/zon/juiblex.zon#L346) ·
[Brass source-only reset](../../../areas/zon/brass-old-1.zon#L889) ·
[mandolin prototype](../../../areas/obj/juiblex.obj#L991) ·
[Brass candidate row 17](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L17)

The only Juiblex SHP file has an empty product list; no other current shop
offers a Juiblex object prototype. A full current prototype scan found no
duplicate owner for any of its 117 mobiles or 113 objects. All 231 outside
story sidecars parsed; the only exact Juiblex item reference is Winterhaven's
crystal input above.
[Juiblex shop and placements](../../../areas/shp/juiblex.shp#L1) ·
[Juiblex reset census](../../../areas/zon/juiblex.zon#L1)

The current catalog contains 872 type-25 object declarations. The only outside
destination value into a Juiblex room is active Ravenloft 2 portal #59082 to
#87528, the fortress approach behind the dossier's locked/pickproof door
contract. Ravenloft 2's reset places this portal in room #59169. This authored
object route is not a WLD boundary and does not establish access through the
door or a completed journey.
[Ravenloft 2 portal destination](../../../areas/obj/ravenloft2.obj#L1239) ·
[active portal reset](../../../areas/zon/ravenloft2.zon#L918) ·
[fortress gate](../zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md#access-controls-and-actual-procedures)

## Direct code and tooling references

The literal assignment catalog binds Doombringer #87612, lake #87542, first
Juiblex #87543, wildmagic mask #87546, Flow #87601, and the grid generator
#87611. Ebb #87600 remains commented out. The Juiblex procedure header defines
the lake, sheen, wormhole, mask, and transfer room constants; the generator
lists its ten current prototype choices. `randomeq.c` lists the Juiblex-area
demon mobiles in its randomized-equipment table. CHAOS equipment profiles use
these Juiblex objects: #87523, #87530, #87538, #87540, #87548, #87560, #87564,
#87575, #87583, #87584, #87585, #87594, #87600, #87601, and #87607. The static
Chaos report script also names #87546 and #87612. These checked-in references
are catalog, assignment, procedure, and reporting evidence; persisted account
or player records were not queried.
[literal assignments](../../../src/specs/specs.assign.c#L2597) ·
[Juiblex constants](../../../src/specs/specs.juiblex.h#L4) ·
[generator choices](../../../src/specs/specs.juiblex.c#L320) ·
[randomized-equipment roster](../../../src/item/randomeq.c#L136) ·
[CHAOS equipment profiles](../../../src/account/chaos_eq_data.h#L80) ·
[static Chaos report](../../../scripts/chaos_eq_report.py#L13)

An exact repository-wide numeric search outside area data and audit documents
found references only in the source, report script, and regression fixtures
linked here; it found no additional editor or command-tool target reference.
The fixtures cover the local catalog, contracts, story mapping, and procedures;
they were not run as part of this documentation-only review.
[production catalog fixture](../../../tests/async/test_zone_story_quest_production_catalog.py#L2087) ·
[native journey harness](../../../tests/async/zone_story_quest_story_harness.cpp#L4414)

## Title and tracked history

The normalized active title “The 222nd Layer of the Abyss” is unique among the
351 current non-empty active titles. Followed WLD, MOB, OBJ, QST, SHP, and ZON
histories identify one contributor. The June 2026 `Jubilex`→`Juiblex` change
corrected the same contributor's source stem; the August 2026 cleanup changed
intermediate `juiblex.*` filenames to their normal extensions. The tracked
2009 area-tree move and the 2026 story-sidecar addition do not identify a
distinct predecessor, replacement, or title owner.

Astral Tiamat's separately placed “Juiblex” is described as an astral form;
Ravenloft 2 owns the portal object that targets the fortress approach. The
Underdark, Faerie Realm, Fields Between, Torg, Mountain, and Scorched Valley
retain their own titles, source ranges, and authored route or item
relationships. The four named Hell layers are also separate contributors.
These references establish a connected story network, not a duplicate area or
replacement claim.
[active title](../../../areas/zon/juiblex.zon#L2) ·
[Astral Tiamat's Juiblex form](../../../areas/mob/astral_tiamat.mob#L119) ·
[Astral Tiamat title](../../../areas/zon/astral_tiamat.zon#L2) ·
[Ravenloft 2 title](../../../areas/zon/ravenloft2.zon#L2) ·
[Fields Between title](../../../areas/zon/fields_between.zon#L2) ·
[Mountain title](../../../areas/zon/mount.zon#L2) ·
[Scorched Valley title](../../../areas/zon/scorchvalley.zon#L2)

## Audit boundary

This supplement records checked-in static sources and code. No server,
database, persisted account/player state, editor session, portal trip, reset,
quest, or gameplay journey was exercised. Actual access, random availability,
retirement, safety, return, and phase-transfer behavior remain subject to the
qualification plan in the [Juiblex dossier](../zone-stories/THE_222ND_LAYER_OF_THE_ABYSS.md).
