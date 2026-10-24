# Nax: source story review

**Scope:** source review of Nax's 245 rooms, 66 mobile prototypes, 62 object
prototypes, 286 zone commands, eight shop records, and authored story sidecar.
This maps local environmental narrative, static exits, and typed source
references. It does not establish live reachability, dynamic procedure
behavior, or safe player traversal.

## Story coverage state

Nax is an active full-world contributor with area note `*377-379 (minotaur
hometown -scip)`, zone header 377, and normalized title “Nax.” It appears in
the production [`areas/AREA`](../../../areas/AREA#L181) list and the separate
historical [`areas/qst/AREA`](../../../areas/qst/AREA#L168) index, but there is
no `areas/qst/nax.qst` source. The coverage row records 245 rooms, 66 mobiles,
62 objects, and zero native Q/QA contracts
([coverage row](ZONE_COVERAGE_REGISTER.csv#L199)).

The sidecar says `coverage: complete` and invites players to meet Nax's people
to uncover local requests, but its `contacts`, `stories`, and `exclusions`
arrays are all empty ([introduction](../../../areas/story/nax.story.json#L6),
[contacts and stories](../../../areas/story/nax.story.json#L11),
[exclusions](../../../areas/story/nax.story.json#L13)). The reviewed static
zone sources therefore provide no mapped local quest dialogue or objective.
Compiled procedures and other external story sources have not been qualified.
The sidecar's “complete” label is stronger than the evidence currently mapped.

## Environmental story

| Setting | Source evidence | What the authored text establishes |
| --- | --- | --- |
| Fortified minotaur town | [large gate approach](../../../areas/wld/nax.wld#L23), [town center](../../../areas/wld/nax.wld#L275), [barracks](../../../areas/wld/nax.wld#L187), [warrior training](../../../areas/wld/nax.wld#L1058) | Nax is presented as an organized, heavily defended settlement. It has an iron-gated approach, barracks and a mess hall, a war tower, training rings, armories, and a general who studies plans. The town center joins paved roads and public buildings. |
| Work and captivity | [plowed fields](../../../areas/wld/nax.wld#L2304), [slave mobile](../../../areas/mob/nax.mob#L228), [head taskmaster](../../../areas/mob/nax.mob#L348) | The fields grow wheat through forced labor. Room prose says minotaurs disdain farming and that captured humans, gnomes, halflings, and other peoples must work until death; the reset profiles describe prisoners scarred from beatings, with taskmasters and mules working beside them. This is a central social fact of the settlement, not a passing detail. |
| Commerce and civic decline | [merchant intersection](../../../areas/wld/nax.wld#L2944), [meat shop](../../../areas/wld/nax.wld#L446), [abandoned courthouse](../../../areas/wld/nax.wld#L1363), [burnt dwelling](../../../areas/wld/nax.wld#L981) | Merchants of many races gather at a crowded caravan market, while pickpockets work the crowd and discarded goods accumulate. Elsewhere the courthouse is dusty and unused, a dwelling has burned down, and poorer homes show neglect. The text combines active trade with signs of an older, damaged city. |
| Faith and historical conflict | [unholy church](../../../areas/wld/nax.wld#L885), [burial ground](../../../areas/wld/nax.wld#L773) | The church's murals celebrate minotaur victories over surface peoples but show stalemates against the Underdark. Worshippers visit before battle to seek the strength of their ancestors. A nearby burial ground is described as sacred and holding fallen warriors. |
| Swamp, forest, and river danger | [swamp vines](../../../areas/wld/nax.wld#L3030), [rope bridge and rapid](../../../areas/wld/nax.wld#L2040), [wild rapid](../../../areas/wld/nax.wld#L2086), [river delta](../../../areas/wld/nax.wld#L5123) | Trails leave the city for a wet forest and swamp where thorny vines trap travelers, pools hide their depth, and animals include snakes, crocodiles, spiders, worms, and carnivorous plants. The river becomes white water; room prose warns that the current, rocks, and flesh-eating fish can kill, and the delta can carry a poor swimmer out to sea. |
| Underdweller labyrinth | [large pit](../../../areas/wld/nax.wld#L2969), [labyrinth center](../../../areas/wld/nax.wld#L3407), [cavern fauna](../../../areas/mob/nax.mob#L544) | The source describes an underground pit and extensive maze of magically shaped stone barriers, stalactites, stalagmites, bones, broken armor, and dangerous cave creatures. Its prose presents the caverns as a dark habitat and grave for failed challengers. This 74-room cluster is a separate static exit component; its intended ordinary entrance is not evident in the WLD graph. |
| Lost and restored sorcery | [tower base](../../../areas/wld/nax.wld#L5151), [magic shop](../../../areas/wld/nax.wld#L5199), [archmage chamber](../../../areas/wld/nax.wld#L5229), [Asterion](../../../areas/mob/nax.mob#L901) | The tower is said to have been built by rare minotaur mages and to reject outsiders. Its upper chamber contains magical tomes and alchemical supplies. A tapestry recounts the minotaurs' original command of arcane magic, their loss of that power, and an evil clan's dark pact with Zion; Asterion is said to have received Zion's dark gift and survived the great cataclysm. |

Together, these scenes depict an armed minotaur city whose trade, religious
identity, and magical hierarchy sit alongside slavery, civic decay, and
dangerous wilderness. The ordinary town and the extensive underworld are both
present in the WLD source, but the latter is not statically joined to the town
component.

## Residents, resets, and shops

The eight shopkeepers are each reset in the room associated with their shop
record:

| Keeper | Role in mobile text | Shop record | Reset placement |
| --- | --- | --- | --- |
| 37700 | Vax, weapons dealer | [shop](../../../areas/shp/nax.shp#L1) | [room 37703](../../../areas/zon/nax.zon#L140) |
| 37708 | Brukish, meat grinder | [shop](../../../areas/shp/nax.shp#L42) | [room 37719](../../../areas/zon/nax.zon#L169) |
| 37709 | Bergin, weaponsmith | [shop](../../../areas/shp/nax.shp#L81) | [room 37745](../../../areas/zon/nax.zon#L205) |
| 37724 | Brok, merchant | [shop](../../../areas/shp/nax.shp#L122) | [room 37730](../../../areas/zon/nax.zon#L180) |
| 37726 | Micheral, veteran weapon mender | [shop](../../../areas/shp/nax.shp#L162) | [room 37777](../../../areas/zon/nax.zon#L237) |
| 37754 | Vanailiux, magical-goods seller | [shop](../../../areas/shp/nax.shp#L203) | [room 37942](../../../areas/zon/nax.zon#L365) |
| 37755 | Ghugarn, armorer | [shop](../../../areas/shp/nax.shp#L241) | [room 37744](../../../areas/zon/nax.zon#L193) |
| 37762 | Reeling minotaur shaman | [shop](../../../areas/shp/nax.shp#L288) | [room 37943](../../../areas/zon/nax.zon#L371) |

The ZON file contains 286 typed commands: 166 M, 66 G, 30 D, 14 E, and 10 O.
The ZON/SHP references import mobile 101 and objects from active source areas.
Mobile 101 is the “local drug dealer” defined in Heavens and is reset in Nax's
Brawling Foot tavern with Heavens objects 829–838
([mobile](../../../areas/mob/heavens.mob#L1228),
[Nax reset](../../../areas/zon/nax.zon#L158),
[example stock object](../../../areas/obj/heavens.obj#L7799)). Confirm that
cross-area character and equipment placement with the area owner.

The other external object references are Heavens 108, 109, 111, 143, 144, 338,
361, 363, 368, 369, 429, 431, and 826; Verz1 3097; Twin Towers Forest 13563;
Realm 14029; and Clfhaven 21585. Their sources are active `areas/AREA` entries:
[Heavens](../../../areas/AREA#L2), [Verz1](../../../areas/AREA#L15),
[Twin Towers Forest](../../../areas/AREA#L76), [Realm](../../../areas/AREA#L78),
and [Clfhaven](../../../areas/AREA#L110). Representative definitions are
[Heavens 108](../../../areas/obj/heavens.obj#L1582),
[Verz1 3097](../../../areas/obj/verz1.obj#L1199),
[Twin Towers Forest 13563](../../../areas/obj/twin_towers_forest.obj#L713),
[Realm 14029](../../../areas/obj/realm.obj#L387), and
[Clfhaven 21585](../../../areas/obj/clfhaven.obj#L982). Heavens objects 108 and
109 also have unregistered Mavulsk source copies
([108](../../../areas/obj/mavulsk.obj#L1798),
[109](../../../areas/obj/mavulsk.obj#L1818)); the active `areas/AREA` list
supplies the Heavens definitions. Keep those VNUM identities intact.

One active inbound typed dependency points the other way: an Alatorin `G` reset
gives Nax object 37742, a liquid potion, to a mobile
([Alatorin reset](../../../areas/zon/alatorin.zon#L3798),
[Nax prototype](../../../areas/obj/nax.obj#L578)). No external active or
unregistered ZON/SHP source was found referencing a Nax mobile prototype, and
no other current QST source contains a numeric reference in Nax's room/mobile/
object range. These are static source scans; code, editor, and saved-data
dependencies are not exhaustively qualified.

## Doors and static connectivity

The full-world census records 526 authored direction records for Nax, all 526
with targets present in the active source world. Nax has one active cross-area
edge: room 37800 leads east to Surface room 625238, whose west exit returns to
37800 ([Nax edge](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L540),
[Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1636)).

The local weak-exit graph has two components: 171 rooms in the larger
town/forest/swamp group and 74 rooms in a second pit/cavern/labyrinth group.
The second group contains room 37834 and rooms 37855–37920 and 37924–37930; no
WLD exit joins it to the larger component
([pit](../../../areas/wld/nax.wld#L2969),
[labyrinth](../../../areas/wld/nax.wld#L3407)). It may depend on a special or
external entry mechanism, but none is established by this static map. Confirm
whether the detached component is intentional and document its intended entry
before using the map as a normal player route.

An unregistered `surf.wld` record also enters active Nax: room 256920 has a
west exit (`D3`) to room 37800
([source-only `surf` edge](../../../areas/wld/surf.wld#L1194408)). This is not
part of the active Surface pair or active-world exit totals. It is recorded
separately in the [`surf` retention note](ATTIC_MIGRATION_PLAN.md#L27).

Nax's census records eight current rooms and one fall-chance room. Current
markers appear in wild-rapid rooms 37796, 37838, 37931–37935, and 37937; room
37794, a rope bridge over white water, has an `F 25` record
([37794](../../../areas/wld/nax.wld#L2040),
[37796](../../../areas/wld/nax.wld#L2086),
[37931](../../../areas/wld/nax.wld#L4992),
[37937](../../../areas/wld/nax.wld#L5104)). The river prose warns of fatal
currents, a dangerous descent, and being swept toward the sea. These are
authored hazard cues and static records, not a live hazard or safety test.

One exit flag deserves a source-level check. Burial-ground room 37733 points
down to collapsed cavern 37734 with blank exit description and keyword, key
VNUM 0, and WLD door-info value 4
([37733 exit](../../../areas/wld/nax.wld#L773)). In the source constants,
`EX_LOCKED` is bit 3 (value 4), while `EX_ISDOOR`, `EX_CLOSED`, and `EX_SECRET`
are separate bits ([exit flags](../../../src/core/defines.h#L442)). The
destination prose calls the opening a secret entrance, but the exit is not
marked as a door, closed, or secret, and there is no matching ZON D reset. The
owner should confirm whether this should be a secret passage, a normal exit, or
a procedurally managed opening. No runtime behavior was exercised.

## History and disposition

The normalized active title scan reports no repeated Nax title; the duplicate
and replacement register has no Nax entry. The followed WLD history includes
the 2009 `areas/` path migration and content changes in 2011 and 2013, but no
direct retirement or replacement link was found. The `-scip` annotation in the
active area note is not itself replacement evidence. Broader semantic
comparisons remain open.

Nax is an un-commented active `areas/AREA` contributor and is **not an attic
candidate**. Retain it in place. Preserve the Alatorin dependency on Nax object
37742, the active Surface pair, and the source-only `surf` reference while
reviewing any future path or content proposal.

## Questions and recommended resolutions

- Should the sidecar's `coverage: complete` and “local requests” wording remain
  when contacts and stories are empty and there is no local QST file? Have the
  area owner either supply the missing story mapping or clarify the intended
  no-local-quest state before calling its narrative coverage complete.
- Is the 74-room pit/cavern/labyrinth component intended to be entered through
  an external or special route? Keep it distinct from the town map until that
  route is supported by source evidence.
- Should the burial-ground exit to the “secret entrance” be authored as a
  door/secret passage, or is a special procedure intended to manage it?
- Is the Heavens drug-dealer mobile and its equipment intentionally reset in
  the Brawling Foot tavern?
- Should the area description or route notes explicitly warn about the
  river's configured current/fall markers and its no-return sea prose?
- The prose alternates between “minotaur dwelling” and “orc dwelling,” calls
  minotaurs “cows,” and includes repeated spelling/grammar errors. Confirm the
  intended voice and naming before a separate prose-edit proposal.
