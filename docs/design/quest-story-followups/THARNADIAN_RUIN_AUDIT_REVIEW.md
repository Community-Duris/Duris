# The Tharnadian Ruin: repository-wide connectivity and dependency audit

**Checkpoint 89; priority 129 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/THARNADIAN_RUIN.md)
with active WLD topology, all inactive WLD sources, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored exits
and static code destinations do not establish ordinary availability, safe
travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. The Tharnadian Ruin has 179 rooms
and 371 authored exits; all 371 resolve. The internal graph has weak components
of 168, 10, and 1 room. The ten-room component is #5602–#5611; singleton room
#5592 is titled “A Bunch of Happy Little Pets.” All three cross-area edges have
reciprocal active returns:

| Ruin edge | Active return | Cross-area register rows |
| --- | --- | --- |
| #5501 west to Caertannad #78400 | Caertannad #78400 east to #5501 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L90), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L895) |
| #5588 west to Cityruin #66200 | Cityruin #66200 east to #5588 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L91), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L830) |
| #5616 north to Braddistock Mansion #1300 | Braddistock Mansion #1300 south to #5616 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L92), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L21) |

There are no negative or positive unresolved exits. Room #5651 carries the
area's one F25 fall-chance field; no room has a current field. The unrelated
global active-WLD duplicate, room #51800 in Hell Two and Hell Three, is outside
this area. Static topology does not qualify live door state, reset behavior,
access, route safety, or dynamic travel.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. It found no
inactive source-room VNUM overlap with the Ruin and no inactive exit into an
active Ruin room. The curated source-only reference total remains 1,092, and
the reviewed Surf subset remains 121. There are no source-only approaches to
add.

## Typed references and compiled numeric matches

The typed scans found no external QST item command, no valid external SHP
product, and no external mobile or object prototype definition matching the
Ruin. The two source-only SHP product matches for #5541 in
`player_guilds.shp` and `sylvandawn.shp` are namespace collisions: #5541 is a
local room, not a local mobile or object. Ixarkon's active `R E 5500` QST line
is an XP reward amount, not an item consumer. Four active Heavens `P` reset
records reference local gate-key object #5504 in container #1212
([first record](../../../areas/zon/heavens.zon#L221)).

The external type-25 scan found no destination into the Ruin among 871
declarations (712 active and 159 source-only). All 231 external story sidecars
parsed without errors and none has an exact `item_vnums` reference to a local
Ruin object. Two structured references in
Caertannad's active story map point to local giver mobiles #5506 and #5528 at
[lines 26–27](../../../areas/story/caertannad.story.json#L26). Their
`completion_key` strings encode item #5504 as the receive key while each
contract gives external item #66201; these are giver references and encoded
contract rewards, not `item_vnums` hits.

The compiled numeric review classified these exact code references:

| Local or source target | Static reference | Classification |
| --- | --- | --- |
| Room #5569 or #5583 | [active object #500055 binding](../../../src/specs/specs.assign.c#L1277); [`tharnrifts_portal`](../../../src/specs/specs.zion.c#L37) | On ENTER of “portal” or “shadowrift,” the handler sets the destination according to the actor's race-war side. This is a dynamic code route, not an ordinary WLD edge or a guarantee that a portal is present or usable. |
| Mobile #5503 and object #5503 | [active mobile binding](../../../src/specs/specs.assign.c#L2216); [standard](../../../src/account/chaos_eq_data.h#L98) and [enhanceable](../../../src/account/chaos_eq_data.h#L350) Paladin CHAOS profiles | The mobile is bound to `undead_howl`; the object also appears in both loadout profiles. A profile declaration does not prove an issuance. |
| Legacy Verzanan candidates | [mobile bindings inside `#if 0`](../../../src/specs/specs.assign.c#L626); [object bindings inside `#if 0`](../../../src/specs/specs.assign.c#L1823) | These assignments are compiled out. Raw room-number references in the corresponding handlers are not active Tharnadian consumers. |

Other raw numeric matches are strings, constants, comments, decimal values, or
references in the wrong VNUM namespace. The scans classify source context
rather than treating every matching integer as an item, room, or active route.

## Title, history, and disposition

The color-stripped title in
[tharnadian_ruin.zon](../../../areas/zon/tharnadian_ruin.zon#L2) normalizes to
“The Tharnadian Ruin” and is unique among 351 active contributors. The followed
WLD and ZON histories contain older edits and the 2009 area-tree path move; the
curated [replacement register](DUPLICATE_AND_REPLACED_ZONES.md) identifies no
distinct predecessor or replacement. The current un-commented
[areas/AREA row](../../../areas/AREA#L110) remains **DO NOT MOVE**.

The existing dossier maps the area's five QST histories, three story cards,
actors, objects, resets, and selected foreign sources. This repository-wide
scan adds the three reciprocal boundaries, confirms the absence of inactive
WLD approaches, and classifies the external key, sidecar, SHP, and code
references. Dynamic route availability, editor/tooling closure, persisted
identity, door and reset state, safe travel, and played qualification remain
separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 385: directed reachability, reset state, and builder questions

The source recheck retains 179 rooms, 371/371 resolving authored exits, three
local weak components (168, 10, and 1), and the same three reciprocal active
boundaries. The all-95 inactive-WLD scan finds no room overlap and no exit into
the Ruin. The curated source-only total remains 8,148; the reviewed Surf subset
remains 291.

Raw directed traversal from the three WLD arrival targets (#5501, #5588, and
#5616) reaches 165/179 rooms. Fourteen rooms are outside that traversal:
#5592, #5602–#5611, and #5661–#5663. Raw reverse traversal from the three
outward edge sources (#5501, #5588, and #5616) finds 167/179 rooms that can
reach an active boundary; #5592, #5602–#5611, and #5662 have no such path.
The first two groups are separate weak components; #5661–#5663 remain in the
largest weak component but have direction-sensitive WLD connectivity. These
are authored-edge counts. They do not model doors, resets, the dynamic portal,
or a played route.

The 26 authored door directions each have a matching local D reset, and no
reset targets a non-door direction. Two reciprocal room links merit builder
confirmation: [#5511 down](../../../areas/wld/tharnadian_ruin.wld#L300) ↔
[#5528 up](../../../areas/wld/tharnadian_ruin.wld#L686) declares WLD flags 5/1
and reset values 5/1 ([resets](../../../areas/zon/tharnadian_ruin.zon#L32),
[paired reset](../../../areas/zon/tharnadian_ruin.zon#L36)); [#5671 east](../../../areas/wld/tharnadian_ruin.wld#L4004) ↔
[#5672 west](../../../areas/wld/tharnadian_ruin.wld#L4028) declares flags 3/3
and reset value 2 on both sides but uses key VNUMs #5527/#5528
([resets](../../../areas/zon/tharnadian_ruin.zon#L104),
[paired reset](../../../areas/zon/tharnadian_ruin.zon#L108)). This records
source asymmetry for review; it does not establish a defect.

Static route types remain distinct. Three reciprocal WLD boundaries join
Caertannad, Cityruin, and Braddistock Mansion. Their cross-area register pairs
are [Caertannad outbound/return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L90) / [return row 895](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L895),
[Cityruin outbound/return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L91) / [return row 830](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L830), and
[Braddistock outbound/return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L92) / [return row 21](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L21). Surface
room #568439 (“The Impact Crater”) resets object #500055, whose active handler
sets its destination to local room #5569 or #5583 according to the actor's
race-war side when used. The Ruin's local object #5539 at #5659 is a type-25
route back to Surface #568439. These source paths do not establish that the
objects are present, usable, safe, or available to every actor (Surface reset
at [surface.zon](../../../areas/zon/surface.zon#L32), object binding at [specs.assign.c](../../../src/specs/specs.assign.c#L1277), handler at [specs.zion.c](../../../src/specs/specs.zion.c#L37), local return object at
[tharnadian_ruin.obj](../../../areas/obj/tharnadian_ruin.obj#L539)).

The typed closure now includes the previously recorded four Tharnadian Ruin
resets to Dungeon mobile #93015 at #5641/#5643/#5652/#5653 and four to Plane of
Earth mobile #23803 at #5667/#5670/#5674 (two placements). Their current
owners stay in dungeon and plane_earth_one (Dungeon reset at
[tharnadian_ruin.zon](../../../areas/zon/tharnadian_ruin.zon#L263), Plane of Earth reset at
[tharnadian_ruin.zon](../../../areas/zon/tharnadian_ruin.zon#L299)). In the opposite direction, four
Heavens resets place local gate-key object #5504 in container #1212 (Heavens at [heavens.zon](../../../areas/zon/heavens.zon#L221)); Caertannad's story records refer to local
givers #5506/#5528 and encode #5504 as the received key while offering foreign
item #66201 (story contracts at [caertannad.story.json](../../../areas/story/caertannad.story.json#L26)). The old player_guilds.shp
and sylvandawn.shp matches for #5541 remain room namespace collisions. No
external same-type mobile/object definition, valid SHP product, external QST
item command, sidecar item_vnums match, or declared type-25 destination into
the Ruin was found. Ixarkon's R E 5500 remains an experience amount. Active
same-type room, mobile, and object overlap scans each return zero.

### Builder questions and recommended resolutions

The recommendations below keep source facts separate from design intent. The
[source-story dossier](../zone-stories/THARNADIAN_RUIN.md) contains the full
native quest and sidecar mapping, including fourteen detailed story follow-up
topics; this list closes the zone-level builder review.

| # | Builder question and source evidence | Recommended resolution |
| --- | --- | --- |
| 1 | Is singleton room #5592 (“A Bunch of Happy Little Pets”) intentionally disconnected from local WLD exits? | Confirm its intended access owner and preserve it as an isolated room until that route is identified. |
| 2 | Is the ten-room #5602–#5611 component an intentional pocket? | Review its entrance and intended audience against the original area plan; do not add a connector solely to make the graph connected. |
| 3 | Are #5661–#5663 deliberately outside directed reach from the three active WLD arrivals? | Identify the intended one-way, reset, or other authored access before changing any exit. |
| 4 | Should #5662 remain unable to reach any active outbound boundary in raw WLD traversal? | Confirm whether this is a one-way branch or a missing return; preserve the current map until the builder resolves intent. |
| 5 | Should the three active reciprocal boundaries remain the Ruin's WLD ingress and egress set? | Keep the paired links to Caertannad, Cityruin, and Braddistock synchronized and retain their cross-area register entries. |
| 6 | Do the surface portal's race-war destinations #5569/#5583 remain the intended alternate arrivals? | Preserve the handler's actor-side distinction and qualify the live object and both destinations before describing access as available. |
| 7 | Is local shadowrift #5539's return to Surface #568439 the intended escape from the Ruin? | Confirm its actual ENTER behavior and destination restrictions; do not promise a safe or guaranteed return from static object data. |
| 8 | Do the 26 authored door directions and 26 matching D resets represent the complete intended door set? | Keep the one-to-one source match and have any future door addition update WLD flags, keys, reset state, and the paired side together. |
| 9 | Are the #5511 down and #5528 up door flags/reset values intentionally different? | Ask the builder to confirm both sides' desired state; change neither direction without an authored design decision. |
| 10 | Should #5671 east and #5672 west require different key VNUMs (#5527 and #5528) across the same reciprocal door? | Confirm that the side-specific keys are deliberate; retain each exact key identity pending that review. |
| 11 | How should reset-open, reset-closed, locked, secret, and pickproof directions affect a future access map? | Publish a separate reset-aware view only after door semantics and reset execution are qualified; keep this raw directed count clearly labeled. |
| 12 | Does room #5651's F25 field represent a player hazard, falling transition, or only legacy area data? | Verify the runtime handler and actual landing/damage behavior before attaching a hazard or completion milestone. |
| 13 | Should fissures and the lack of current fields affect route or hazard descriptions? | Keep source prose distinct from travel behavior and add no current-based route assumption. |
| 14 | Is the #5539 type-25 route's negative charge value intentional for its current use? | Preserve the value until its engine meaning and intended reuse behavior are confirmed. |
| 15 | Which of the three local story cards, if any, should depend on reaching the disconnected or one-way branches? | Keep accepted native returns independent as currently authored unless a builder names the exact route prerequisite and authority. |
| 16 | Should the five distinct armoury proofs remain one story card with either local recipient accepted? | Preserve the five exact item identities and alternative Q71/Q131 recipients; do not collapse evidence into generic combat credit. |
| 17 | Is necromancer item #66201 intended to yield local key #5504 independently of armoury clearance? | Keep the native Q57/QA143 returns separate unless the builder explicitly chooses and implements a prerequisite. |
| 18 | Is spirit #5514's black-key return independent from the other two stories? | Retain its exact #5513 offering and Q87 return, and do not infer area restoration from departure or XP. |
| 19 | Should external Heavens, Caertannad, Dungeon, and Plane of Earth contracts keep these exact local IDs? | Preserve object #5504, giver mobiles #5506/#5528, and external reset ownership for #93015/#23803; coordinate any future VNUM change across each consumer. |
| 20 | Does any separately named predecessor or intended replacement exist outside the same-stem history? | Keep tharnadian_ruin DO NOT MOVE on current evidence and request an explicit source from a builder before claiming a successor or migration relationship. |

The color-stripped title is unique among all 442 current .zon titles, and
active same-type room/mobile/object comparisons find no overlap. Followed
WLD/ZON history remains on the same stem: the 2011 commits 8e4a056ff and
677a9f877 discuss quest re-entry and Tharn Rifts preparation; 642da5400 is the
2009 area-tree path move. No distinct predecessor or whole-zone replacement
is established. Keep the active areas/AREA contributor DO NOT MOVE. No
candidate row or source-only approach count changes.

This remains a source audit. Runtime route presence, actor access, reset-open
reachability, route safety, and a played journey are not qualified here.

## Checkpoint 775: Braddistock quest item reference

Tharnadian Ruin's active QST references active Braddistock zone-13 remains
#1375 at G I lines 80 and 138; its story sidecar records the exact item at
line 94. Preserve that cross-area item ownership and the two consuming
contracts. See the [Braddistock zone-13 audit](BRADDISTOCK_ZONE13_AUDIT_REVIEW.md).

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad story-sidecar giver roles reference Tharnadian Ruin mobiles #5506 and #5528. See [Caertannad source inventory](CAERTANNAD_AUDIT_REVIEW.md#active-source-inventory-and-ownership).

## Checkpoint 840: Current typed-source closure refreshed

A bounded refresh against the unchanged current `areas/` and `src/` sources
reconciles the cross-area typed links recorded in the existing closure:

- **Braddistock proof:** The two local Ruin QST branches require exact OBJ
  #1375, and the local sidecar preserves it as `proof-1375` with
  `disappear=0`. The object and quest material remain Braddistock-owned; the
  Ruin owns the receiving outcomes. ([QST](../../../areas/qst/tharnadian_ruin.qst#L80),
  [second branch](../../../areas/qst/tharnadian_ruin.qst#L138),
  [sidecar](../../../areas/story/tharnadian_ruin.story.json#L90),
  [Checkpoint 775](#checkpoint-775-braddistock-quest-item-reference))
- **Caertannad giver roles:** Its story sidecar refers to local Ruin mobiles
  #5506 and #5528. Preserve those exact MOB identities and the separate
  giver/contact relationship; this is not an external QST item command or a
  duplicate mobile prototype. ([Checkpoint 779](#checkpoint-779-caertannad-typed-dependency))
- **Imported mobiles:** Four Ruin reset roles still import Dungeon MOB #93015
  and four import Plane of Earth MOB #23803. Those reset placements do not
  change prototype ownership. ([Dungeon reset](../../../areas/zon/tharnadian_ruin.zon#L263),
  [Plane of Earth reset](../../../areas/zon/tharnadian_ruin.zon#L299))
- **Heavens key placements:** Four Heavens P resets still place local OBJ
  #5504 in container #1212. This remains an outside reset of a local object,
  separate from the foreign Braddistock proof. ([Heavens reset](../../../areas/zon/heavens.zon#L221))
- **Surface travel items:** Compiled handling of Surface OBJ #500055 retains
  its dynamic arrival into Ruin rooms #5569/#5583; local type-25 OBJ #5539
  retains its return target Surface #568439. Both item-defined routes stay
  distinct from WLD exits and from each other. ([Surface reset](../../../areas/zon/surface.zon#L32),
  [assignment](../../../src/specs/specs.assign.c#L1277),
  [handler](../../../src/specs/specs.zion.c#L37),
  [local portal](../../../areas/obj/tharnadian_ruin.obj#L539))

The complete bounded negative checks and same-type collision review remain as
recorded under Checkpoint 385. No new WLD edge, active shop product, external
QST item consumer, sidecar item input, or type-25 destination into the Ruin is
established by this refresh. Keep active `tharnadian_ruin` **DO NOT MOVE**.
