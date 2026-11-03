# BogenTok repository-wide audit supplement

This supplement reconciles BogenTok's active WLD topology and doors, inbound
and outbound boundaries, typed references, callback bindings, shops, and
bounded title/history evidence with the [source-story dossier](../zone-stories/BOGENTOK.md).
The reviewed source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`.
No area or gameplay source is changed here.

## Active source identity and story boundary

`bogentok` is an un-commented full-world contributor in the active
[world manifest](../../../areas/AREA#L94) and appears in the separate
[quest-source index](../../../areas/qst/AREA#L87). The current source set has
205 WLD rooms (#18700–#18904), 66 mobile prototypes (#18700–#18765), 46 object
prototypes (#18700–#18745), 512 ZON commands, and three SHP records. Its
header title is “BogenTok”; the ZON reset mode is 2
([WLD](../../../areas/wld/bogentok.wld#L1),
[ZON](../../../areas/zon/bogentok.zon#L1)). There is no local `.qst` or story
sidecar and no local native Q/QA contract. Membership in `areas/qst/AREA`
does not establish that a same-stem quest file exists.

The active `.shp` contains shop records #18707, #18715, and #18747, so this
checkpoint corrects the coverage row's former zero-shop inventory to three.
The shops cover beer and whiskey at the shady bar, three provisions at the
general store, and six bloodmail pieces at the factory. Their source records
are at [#18707](../../../areas/shp/bogentok.shp#L1),
[#18715](../../../areas/shp/bogentok.shp#L36), and
[#18747](../../../areas/shp/bogentok.shp#L81). Local resets place the
bartender in room #18730, the fat-orc keeper in #18741, and the stout duergar
keeper in #18757 ([bar](../../../areas/zon/bogentok.zon#L372),
[store](../../../areas/zon/bogentok.zon#L409),
[factory](../../../areas/zon/bogentok.zon#L482)). These declarations do not
qualify live shop stock or current keeper placement.

## Active WLD topology and boundaries

All 440 authored exits resolve: 438 remain within BogenTok and two leave it.
All 205 rooms form one weak component. There are no negative or positive
unresolved targets, no exits from rooms outside the numeric zone range, no
fall markers, and no current records. Raw directed traversal from each active
inbound anchor reaches all 205 local rooms and can return to that anchor when
authored exits are treated as available. That is a graph result; it does not
test closed-door interaction, player access, or a live journey.

| Active inbound source | Entry in BogenTok | Direct return |
| --- | --- | --- |
| [Underdark #812256 east](../../../areas/wld/underdark.wld#L177549) | #18714, west gate | [#18714 west](../../../areas/wld/bogentok.wld#L381) returns to #812256 |
| [Underdark #812663 west](../../../areas/wld/underdark.wld#L179902) | #18700, east gate | [#18700 east](../../../areas/wld/bogentok.wld#L19) returns to #812663 |

These are two reciprocal active Underdark boundaries: two inbound exits and
two outbound exits. Each inbound anchor participates in the full raw directed
local reach-and-return result above.

The WLD contains 72 door exits. Each carries the door flag without a lock or
secret flag and has key VNUM 0. The ZON includes 72 matching `D` actions, all
with state 1 (closed); the action list begins at
[the first door reset](../../../areas/zon/bogentok.zon#L8) and continues
through the end of the door block at
[the last door resets](../../../areas/zon/bogentok.zon#L292). Excluding all
reset-closed door edges from traversal gives a conservative static reach of
152/205 rooms from either inbound anchor. This traversal does not model
opening a door, access permissions, reset timing, or player movement.

The ZON header uses mode 2. Source code schedules nonzero reset modes and
allows mode 2 to reset a zone while it is occupied
([event scheduling](../../../src/world/new_events.c#L2068),
[mode-2 occupancy condition](../../../src/world/events.c#L429)). The header's
lifespan fields are 40–50. These source settings describe reset scheduling;
they do not establish a live reset time, present door state, current
population, or available inventory.

## Inactive WLD approach

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse issues and found no room-VNUM overlap with BogenTok. It found one
source-only approach: inactive `ud_map2.wld` room #1128575 has a west exit to
BogenTok #18700 ([source room](../../../areas/wld/ud_map2.wld#L114774),
[west exit](../../../areas/wld/ud_map2.wld#L114790),
[target](../../../areas/wld/ud_map2.wld#L114793)). This is not active
ingress. It raises the curated source-only approach total to 3,442 overall,
the reviewed Surf subset remains 207, and the reviewed `ud_map2` subset rises
from 15 to 16. Preserve the approach on the existing `ud_map2` candidate row
[114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114); its one active inbound
Shafts reference remains unchanged.

## Typed dependencies and code bindings

The only outside ZON reset-role declarations found in the exact scan are in
active `alatorin.zon`:

| Consumer | Typed local target | Declaration | Source role |
| --- | --- | --- | --- |
| Alatorin | Object #18718 | [line 1659](../../../areas/zon/alatorin.zon#L1659) | Reset into Alatorin room #83546 |
| Alatorin | Mobile #18718 | [lines 2801 and 2803](../../../areas/zon/alatorin.zon#L2801) | Two reset declarations into room #83361, with 45 and 100 chances |

Mobile #18718 is a steeder; object #18718 is ragged clothes. The matching
number is a typed-ID coincidence, not one shared prototype. The reset comments
identify both imported prototypes as absent from Alatorin's local prototype
files. These are active-source dependencies; the declarations do not prove
current placement. Preserve Alatorin as a consumer if either local prototype
is renamed or moved.

Exact scans across active and unregistered QST files, SHP product lists,
story sidecars, ZON reset roles, and C/C++ numeric bindings found no outside
QST room/item/mobile reference, external SHP product reference, or exact
story-sidecar VNUM reference to BogenTok. No external mobile/object prototype
duplicates any of its local VNUMs. Among 872 external type-25 portal
declarations and 380 external type-29/switch declarations, none targets a
BogenTok room, mobile, or object.

There is no local `.qst` and no native Q/QA definition, but the bartender
mobile #18707 is bound to the generic `world_quest` callback
([binding](../../../src/specs/specs.assign.c#L770)). The callback supports
quest management through a bartender command interface
([handler](../../../src/specs/specs.world_quest.c#L191)). This is a separate
code-backed service; it does not prove that BogenTok has local quest content.
Room #18729 is also bound to the generic inn callback at two assignment sites
in `specs.assign.c` ([first](../../../src/specs/specs.assign.c#L2291),
[second](../../../src/specs/specs.assign.c#L2299)); the handler is defined
in [specs.room.c](../../../src/specs/specs.room.c#L243).

Object #18744 is the general's sword named “Betrayal.” A `betrayal` callback
assignment for it exists only as a commented-out line in
[specs.assign.c](../../../src/specs/specs.assign.c#L1149). It is not an active
binding and is recorded as a builder question, not a runtime behavior.

## Resets and source interpretation

The active ZON contains 367 mobile (`M`), 13 follower (`F`), 72 door (`D`),
43 equipment (`E`), 14 carried-object (`G`), and three room-object (`O`)
actions. The 13 `F` rows are following relationships, not fall markers. The
72 closed-door declarations are the only door-state basis for the conservative
traversal above. The 367 mobile declarations describe reset inputs, not
guaranteed numbers of simultaneous or currently present creatures.

## Title, history, and disposition

Color-stripping and normalizing the current 442 `.zon` titles yields one
`BogenTok` match, in [bogentok.zon](../../../areas/zon/bogentok.zon#L2).
Followed WLD, MOB, OBJ, ZON, and SHP histories retain the `bogentok` stem.
The paths include the 2009 move into `areas/` (`642da5400`), the 2011 WLD
spelling edit (`fac285bc7`), the 2013 WLD content edit (`71d731c5b`), and the
2014 shop cleanup (`1c59fb690`). MOB and OBJ include the 2012 field-format
edit (`bb0d59425`); MOB and ZON include the 2015 quest-item flag pass
(`f18992b8f`). These are same-stem edits. The bounded paths show no
alternate-stem rename, distinct predecessor, or whole-zone replacement.

Keep active `bogentok` **DO NOT MOVE**. Preserve its two reciprocal active
Underdark boundaries, all 72 reset-closed doors, the Alatorin imports of
mobile and object #18718, the active bartender and inn callbacks, and the
source-only `ud_map2` approach on that candidate's existing row. Add no
BogenTok attic-candidate row. Shop availability, quest content, access through
closed doors, and live reset state remain unqualified; see the
[source-story dossier](../zone-stories/BOGENTOK.md) for builder questions.
