# The Swamp Laboratory of Khul'Lor repository-wide source audit

Checkpoint 207 maps active `lab` against its authored story, active WLD
boundaries, reset-aware access, inactive-WLD approaches, typed dependencies,
title, and followed source history. This is a static source audit. It does
not inspect a live server, player state, database, editor, persisted world
state, or actual gameplay. The [source dossier](../zone-stories/LAB.md)
summarizes the environmental story and builder questions.

## Source identity and authored content

The active area list identifies `lab` with range note *823. The `.zon` header
is #823, reset mode 1, title The Swamp Laboratory of Khul'Lor. Its WLD has 51
rooms #82300-#82350, its MOB has 20 prototypes #82300-#82319, and its OBJ has
24 prototypes #82300-#82323. The `.zon` resets reference all local mobile and
object prototypes. The source is listed in both `areas/AREA` and
`areas/qst/AREA`, but there is no current local QST file, SHP file, story
sidecar, or native Q/QA contract ([area list](../../../areas/AREA#L295),
[zone header](../../../areas/zon/lab.zon#L1), [QST index](../../../areas/qst/AREA#L263),
[coverage row 153](ZONE_COVERAGE_REGISTER.csv#L153)). The similarly named
active `labyrinth` is a separate area, zone #50, “Labyrinth of No Return,”
with its own files and story sidecar.

The local story mixes a dark swamp and dead woods with a torture room,
submerged pond, tree interior, rotten mansion, bridge and ravine, mountain
keep, and a white porcelain castle. A corridor wonders whether a curse or
wizard causes the darkness; the pond room separately says the green mist
originates there and is not magical ([entry and corridor
rooms](../../../areas/wld/lab.wld#L1), [pond
room](../../../areas/wld/lab.wld#L93)). These descriptions leave the cause
and any cure unresolved.

A reset frog carries a necklace note saying an unnamed writer has turned
“prince Uklor” into a frog and calls him the reader's “beloved champion.” The
local area has no QST/contract source that names Uklor or resolves the note
([reset](../../../areas/zon/lab.zon#L66), [note
object](../../../areas/obj/lab.obj#L1)). The only other exact `Uklor` string
found in the scanned area source is in an unrelated BattleFields object that
calls a deity Uklor; the source does not establish that this is the same
person ([object](../../../areas/obj/battlefi.obj#L549)).

Local rooms and a mobile describe Khul'Lor as an archmage who built a
kingdom. Active `desert` source provides a named parallel: room #49572 says
Khul'Lor used his mine to fund a dwarven kingdom's tower and citadel, says
the location is haunted by his spirit, and identifies it as his abandoned
mine; hunter #49223 refers to the former mines, and object #49135 is named as
a lost token from them ([mine room](../../../areas/wld/desert.wld#L14358),
[hunter](../../../areas/mob/desert.mob#L3061), [token
definition](../../../areas/obj/desert.obj#L1972), [local
archmage](../../../areas/mob/lab.mob#L197)). This is a textual/lore link; no
local room, mobile, or object VNUM is referenced by those Desert records.
Whether the living lab archmage and the lingering mine spirit are the same
character or belong to different points in a timeline is unresolved.

The local reset also imports Heavens object #358 into the circular chamber.
It is a small rune-covered stone whose long description says it is one of
the world's mana nodes and that visiting such nodes grants abilities. Its
prototype is not in `lab.obj`; the zone reset places it in room #82344. The
source does not identify a local interaction for this imported object
([reset](../../../areas/zon/lab.zon#L62), [Heavens
definition](../../../areas/obj/heavens.obj#L3709)).

The positive-energy sphere #82318 is a closed and locked local container
whose value vector names #82319 as its key. The reset places marble cube
#82322 inside it and gives Khul'Lor negative-energy key #82319. This is a
source-defined item chain; the significance of the cube is not described
([container and key](../../../areas/obj/lab.obj#L249), [nested reset and
Khul'Lor inventory](../../../areas/zon/lab.zon#L60)).

## Active WLD graph and reset-state reach

The local WLD has 100/100 resolving exits: 99 local and one outbound. Its
WLD-only local graph has weak components sized 40, 5, 4, and 2. Directed
traversal with all local WLD exits available reaches 20/51 rooms from active
arrival #82350. The active boundary records are:

| Neighbor | Active inbound edge | Local outbound edge | Pairing |
| --- | --- | --- | --- |
| ConnectorZones | #53855 north (D0) to #82350 ([edge](../../../areas/wld/connectorzones.wld#L4779)) | None | One-way inbound |
| Kvarkpass Connector | #54690 north (D0) to #82350 ([edge](../../../areas/wld/kvarkpass_connector.wld#L2070)) | #82350 south (D2) to #54690 ([edge](../../../areas/wld/lab.wld#L1031)) | Reciprocal |

The source-to-target and reverse-boundary counts are two inbound, one
outbound, and one reciprocal boundary. No local exit has a negative,
positive-unresolved, or unregistered target.

The `.zon` contains ten unconditional directional door resets across five
local pairs. Four pairs start closed: the outer gate #82300/#82301, the
dungeon-to-swamp pair #82305/#82317, the treasure-room pair #82315/#82316,
and the porcelain-castle pair #82345/#82346. The keep gate #82343/#82344
starts closed and locked; both WLD door fields name key #82320, carried by
Khul'Lor in room #82349 ([WLD fields](../../../areas/wld/lab.wld#L896),
[door resets](../../../areas/zon/lab.zon#L32), [key
holder](../../../areas/zon/lab.zon#L111)). The dragon carries #82310 for the
dungeon-to-swamp key field, and a green golem carries #82305 for the treasure
room key field ([key fields](../../../areas/wld/lab.wld#L127),
[key resets](../../../areas/zon/lab.zon#L71)).

Those resets leave 89/99 local directed WLD exits open. The strict reset-open
graph has weak components sized 27, 6, 5, 4, 3, 2, 2, 1, and 1. Directed
reach from #82350 is 2/51 (#82350 and #82300); the first northbound gate is
closed at reset. No fall-chance or current fields are authored for the local
rooms. These counts model WLD exits and unconditional door-reset states only;
they do not qualify door commands, other dynamic behavior, safety, or return.

Seven local `ITEM_TELEPORT` objects separately encode `CMD_ENTER` and local
destination room VNUMs. The source object and reset pairs connect the tree,
tunnel, mansion, and pond subareas:

| Object | Reset room | Destination | Source |
| --- | --- | --- | --- |
| Misty pond #82301 | #82304 | #82330 | [object](../../../areas/obj/lab.obj#L16), [reset](../../../areas/zon/lab.zon#L49) |
| Tree of burden #82302 | #82312 | #82313 | [object](../../../areas/obj/lab.obj#L30), [reset](../../../areas/zon/lab.zon#L51) |
| Tree of wisdom #82303 | #82311 | #82318 | [object](../../../areas/obj/lab.obj#L42), [reset](../../../areas/zon/lab.zon#L50) |
| Tree of burden #82304 | #82313 | #82312 | [object](../../../areas/obj/lab.obj#L54), [reset](../../../areas/zon/lab.zon#L52) |
| Tree of wisdom #82308 | #82318 | #82311 | [object](../../../areas/obj/lab.obj#L110), [reset](../../../areas/zon/lab.zon#L55) |
| Ancient mansion #82312 | #82324 | #82325 | [object](../../../areas/obj/lab.obj#L164), [reset](../../../areas/zon/lab.zon#L56) |
| Ancient mansion #82313 | #82325 | #82324 | [object](../../../areas/obj/lab.obj#L176), [reset](../../../areas/zon/lab.zon#L57) |

The item links are separate from the WLD component and reach counts. All
four diagonal direction values appear in the WLD; the engine defines
northwest, southwest, northeast, and southeast as directions 6 through 9
([direction constants](../../../src/core/defines.h#L435)).

## Source-only inactive-WLD approaches

The current WLD inventory contains 446 files: 351 listed active area files
and 95 unregistered files. Scanning all 95 unregistered WLD files found two
non-overlapping source-only approaches to active #82350. Neither source map
is selected by `areas/AREA`, so these are not active ingress:

| Inactive source | Exit | Evidence |
| --- | --- | --- |
| `surf.wld` #269091 | West (D3) to #82350 | [Room](../../../areas/wld/surf.wld#L1449803), [exit](../../../areas/wld/surf.wld#L1449819) |
| `underworld2.wld` #421086 | West (D3) to #82350 | [Room](../../../areas/wld/underworld2.wld#L33408), [exit](../../../areas/wld/underworld2.wld#L33424) |

Neither source room overlaps #82300-#82350. Preserve the Surf approach on
candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) and the
Underworld2 approach on candidate row
[116](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L116). These records do not
change the active boundary or reset-aware arrival graph.

## Typed dependency reconciliation

The local zone resets reference all 20 local mobiles and all 24 local
objects, plus imported Heavens object #358. Current-source scans found:

| Consumer scan | Result |
| --- | --- |
| External ZON reset roles | No outside reset targets a local room, mobile, or object among 441 other current ZON files. |
| External QST numeric references | No exact local room, mobile, or object VNUM among 265 other current QST files. |
| External SHP products | No shop product targets a local object among 180 other current SHP files; `lab` has no local shop. |
| External story-sidecar VNUMs | No exact local room, mobile, or object VNUM among 232 current story sidecars. |
| External MOB/OBJ definitions | No duplicate definition of a local mobile or object VNUM among 445 other MOB and 441 other OBJ files. |
| WLD door-key fields | Six fields across the three local keyed pairs use local keys #82305, #82310, and #82320. No external WLD door-key field uses a local lab object. #82319 is used as the local sphere-container key, not as a WLD door key. |
| External type-25/type-29 room targets | The current object corpus has 872 `ITEM_TELEPORT` and 380 `ITEM_SWITCH` prototypes. None outside `lab.obj` targets a local room; the seven local type-25 objects target rooms within the area, and there is no local type-29 switch. |
| C/C++ exact local VNUM references | No exact room, mobile, or object VNUM was found under `src/`. |

These are bounded source scans. They do not cover editor aliases, server
memory, databases, persisted references, or runtime behavior.

## Title, followed history, and disposition

After color stripping and normalization, The Swamp Laboratory of Khul'Lor is
unique among the 442 current `.zon` titles. The similarly named active
`labyrinth` contributor is a separate zone and source identity. Followed
WLD, MOB, OBJ, and ZON histories retain the `lab` stem through the 2009
area-tree move commit `642da5400` and later same-stem edits. The histories
include a 2010 vault-room correction; no alternate-stem predecessor,
retirement, or whole-zone replacement is established.

Keep active `lab` **DO NOT MOVE**. Preserve its WLD boundaries, enterable
objects, reset-closed doors and key placements, local prototype VNUMs, and
named link to the Desert Mines of Khul'Lor. Keep Surf candidate row 97 and
Underworld2 candidate row 116 **RETAIN IN PLACE** with their source-only
approaches. No `lab` attic candidate row or migration clearance is added.

## Builder follow-up

1. Confirm whether the living Khul'Lor in `lab` is the same mage described in
   Desert's mine as a lingering spirit; if so, establish the intended
   chronology.
2. Identify prince Uklor and the “beloved champion” in the frog's note, and
   whether the BattleFields deity with the same name is related.
3. Confirm the intended opening or alternate entry for the outer gate. The
   active arrival graph reaches only #82350 and #82300 while the gate reset is
   closed.
4. Confirm the intended order and availability of the key-bearing doors and
   the `ITEM_TELEPORT` entrances that link WLD-disconnected room groups.
5. Explain whether Heavens object #358 is intended to function as a mana node,
   and what the locked sphere and contained cube are meant to do.

## Builder questions and recommended resolutions

The existing dossier identifies the major story questions. These direct
questions make the unresolved ownership, entry, door, and item outcomes
explicit without treating source descriptions or reset declarations as
implemented quests or proven live access.

| Question | Source evidence | Recommended resolution |
| --- | --- | --- |
| Are the corridor's possible magical darkness and the pond's explicitly non-magical mist separate phenomena, or intended to conflict? | The corridor speculates about a curse or wizard, while the pond room says the green mist originates there and is not magical ([rooms](../../../areas/wld/lab.wld#L1), [pond room](../../../areas/wld/lab.wld#L93)). | Preserve both statements as authored until an owner defines their relationship. Do not infer a cure, single cause, or quest from either description. |
| Is prince Uklor in the frog's necklace note the deity named Uklor in BattleFields, and who is meant to answer the note? | The reset places the note on a frog; local `lab` has no QST file or native Q/QA contract, and the matching deity name occurs in a separate area ([note](../../../areas/obj/lab.obj#L1), [QST index](../../../areas/qst/AREA#L263), [BattleFields object](../../../areas/obj/battlefi.obj#L549)). | Keep the identity and response unresolved until the story owner confirms them. A shared name alone should not create a cross-area quest or same-character claim. |
| Is the living Khul'Lor here the same archmage whose spirit is said to linger in the Desert mines? | `lab` describes an archmage who built a kingdom; Desert describes Khul'Lor's mine, kingdom tower, and lingering spirit, with no exact prototype link ([Lab profile](../../../areas/mob/lab.mob#L197), [Desert mine](../../../areas/wld/desert.wld#L14358)). | Keep the textual connection and leave identity and chronology open until the owners define them. Do not merge the local mobile with Desert's spirit by name alone. |
| Should `lab` remain in `areas/qst/AREA` while it has no `lab.qst`, sidecar, or native contract? | The area is listed in both active indexes but has no current local QST or Q/QA contract ([AREA](../../../areas/AREA#L295), [QST AREA](../../../areas/qst/AREA#L263)). | Confirm whether the QST membership is stale or reserves a planned feature. Remove the stale index entry or add a specifically owned contract only after its actor, inputs, outcome, and recovery are defined. |
| What is the intended admission path through the outer iron gate at #82300/#82301? | The gate pair starts closed; reset-aware WLD traversal from arrival #82350 reaches only #82350 and #82300, while all-edge traversal and local enterable objects produce different reach sets ([graph and resets](#active-wld-graph-and-reset-state-reach)). | Preserve the authored arrival and closed reset state until the builder defines the intended opening, admission, and return path. Do not count an item-teleport destination or an open-edge graph as proof the gate can be crossed. |
| Should the evil dragon's key #82310 open the torture-chamber to swamp pair #82305/#82317, and how should a player obtain it? | The WLD key field names #82310 and the ZON reset places that key on the dragon ([key fields](../../../areas/wld/lab.wld#L127), [dragon reset](../../../areas/zon/lab.zon#L71)). | Keep this exact key, pair, and holder together. Confirm drop, transfer, and return behavior before describing the route as usable. |
| Should the green golem's key #82305 open the treasure-room pair #82315/#82316? | The source associates #82305 with both treasure-door key fields and places the key on a green golem ([door fields](../../../areas/wld/lab.wld#L348), [opposite door field](../../../areas/wld/lab.wld#L370), [key reset](../../../areas/zon/lab.zon#L80)). | Preserve the current pairing and holder. Qualify key availability and both directions separately; do not treat the closed reset as proof of the intended puzzle solution. |
| Is Khul'Lor's key #82320 the sole intended route through the locked keep gate at #82343/#82344? | Both door fields name #82320, carried by Khul'Lor in #82349 ([door fields](../../../areas/wld/lab.wld#L896), [key-holder reset](../../../areas/zon/lab.zon#L111)). | Keep the key and gate association explicit. Define whether the player must defeat, pickpocket, or otherwise obtain the key and how the return works before claiming access. |
| How should the porcelain-castle door pair #82345/#82346 open, given that it resets closed without a named key field? | Both directions reset closed; the local review found the pair among five reset-closed door pairs ([door resets](../../../areas/zon/lab.zon#L32), [graph review](#active-wld-graph-and-reset-state-reach)). | Confirm whether ordinary open/close commands, another source mechanic, or an authored key is intended. Do not invent a key or mark the route blocked permanently from the reset alone. |
| Are the seven local `ITEM_TELEPORT` objects intended as the primary links between WLD-disconnected rooms, and is each enter/return path safe? | Seven reset-placed objects encode `CMD_ENTER` and local destinations, including tree, mansion, and pond routes ([objects and resets](#active-wld-graph-and-reset-state-reach)). | Preserve them as typed item actions distinct from WLD exits. Qualify each object's reset availability, entry action, destination, and return independently before calling the subareas connected or safe. |
| Is Heavens object #358 meant to grant the mana-node abilities described in its prototype when placed in the circular chamber? | The local reset imports the Heavens stone; its description says visiting mana nodes grants abilities, but no local contract or action is authored ([reset](../../../areas/zon/lab.zon#L62), [object](../../../areas/obj/heavens.obj#L3709)). | Keep the imported prototype under Heavens ownership. Confirm the intended local interaction and exact effect before promising an ability or adding a Lab-owned contract. |
| What is the intended purpose and recovery path for the locked positive-energy sphere #82318 and the marble cube #82322 inside it? | The sphere names #82319 as its key; the reset puts #82322 inside and gives #82319 to Khul'Lor ([container](../../../areas/obj/lab.obj#L249), [reset](../../../areas/zon/lab.zon#L60)). | Preserve the local key/container/content chain as authored. Do not infer that the cube is a quest reward, mana-node component, or progression proof until its owner defines that outcome. |
| If inactive maps are ever brought back into active service, are `surf.wld` #269091 and `underworld2.wld` #421086 intended to approach Lab at #82350? | Both are source-only inbound records from unregistered WLD files; neither contributes to active AREA membership ([inactive approaches](#source-only-inactive-wld-approaches)). | Keep both records source-only under Surf and Underworld2. Preserve their existing RETAIN IN PLACE candidate rows; route activation requires the map owners to confirm endpoint, arrival, and return intent. |
| Does the one-way ConnectorZones arrival at #82350 intentionally lack a Lab-side WLD return? | ConnectorZones has an inbound edge to #82350 without a paired Lab WLD exit; Kvarkpass has a separate reciprocal pair ([boundary table](#active-wld-graph-and-reset-state-reach)). | Keep the two boundaries separate. Do not add a ConnectorZones return or use another area's reciprocal edge as evidence of safe return. |
| Should the source's separate WLD components be described as one continuous playable route? | The authored WLD has four weak components; the seven item-teleport links join several subareas but remain typed item actions, and reset-open directed reach is only 2/51 from #82350 ([graph and teleports](#active-wld-graph-and-reset-state-reach)). | Report WLD and item-action connectivity separately. Keep player reachability, door interaction, item availability, and safe return unqualified until exercised. |

