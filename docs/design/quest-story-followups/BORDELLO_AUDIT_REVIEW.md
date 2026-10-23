# Vella's Bordello repository-wide audit supplement

This supplement reconciles Bordello's active WLD topology and boundaries,
reset declarations, typed references, code callbacks, shop inventory, and
bounded title/history evidence with the
[source-story dossier](../zone-stories/BORDELLO.md). The reviewed source
snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay
source is changed here.

## Active source identity and story boundary

`bordello` is an un-commented contributor in the active
[world manifest](../../../areas/AREA#L331) and is also listed in
[areas/qst/AREA](../../../areas/qst/AREA#L293). Its active source defines 69
rooms (#93600–#93668), 16 mobile prototypes (#93600–#93615), 12 object
prototypes (#93600–#93611), one SHP record, and 116 ZON commands. The ZON
title is “Vella's Bordello”; the reset mode is 2 and its lifespan fields are
15–25 ([WLD](../../../areas/wld/bordello.wld#L1),
[ZON](../../../areas/zon/bordello.zon#L1)). There is no same-stem `.qst`,
local native Q/QA contract, or story sidecar. QST/AREA membership alone does
not establish local quest content.

The active [shop record](../../../areas/shp/bordello.shp#L1) is #93612 and
serves room #93667. Its keeper, mobile #93612 Vegar, is reset there
([reset](../../../areas/zon/bordello.zon#L128)); the record lists object
#93610 and #93611. This corrects the coverage register's previous zero-shop
count to one. It also carries race-based refusal text. Live stock, keeper
placement, and shop access were not checked.

## Active WLD topology, boundaries, and reset doors

All 147 authored exits resolve: 146 are local and one leaves Bordello. The 69
rooms form one weak component. Raw directed traversal from each active inbound
anchor reaches and can return from all 69 rooms when every authored exit is
treated as available. This static graph result does not test door interaction,
player access, reset timing, or live travel.

| Active boundary | Source edge | Reverse WLD edge |
| --- | --- | --- |
| Inbound from Heavens | [Heavens #29 south](../../../areas/wld/heavens.wld#L345) enters Bordello #93641 | None |
| Inbound from White Harbor | [White Harbor #55614 up](../../../areas/wld/wh.wld#L12634) enters Bordello #93610 | None |
| Inbound from Surface | [Surface #563634 north](../../../areas/wld/surface.wld#L1335805) enters Bordello #93600 | [Bordello #93600 south](../../../areas/wld/bordello.wld#L19) returns to Surface #563634 |

These account for three active inbound exits, one outbound exit, and one
reciprocal edge pair. The active exit register records the rows in
[the full exit ledger](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L9) and the
[pair summary](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L51).

Two reciprocal local exits between Main Hallway #93631 and Treasure Room
#93668 have raw flags 5 and key VNUM 0: door plus locked, with no raw closed
bit ([first exit](../../../areas/wld/bordello.wld#L707),
[reverse exit](../../../areas/wld/bordello.wld#L1295)). Matching ZON `D`
commands use state 0 and comments call both exits open
([resets](../../../areas/zon/bordello.zon#L8)). The loader's state-0 branch
clears both locked and closed bits ([door reset handling](../../../src/world/db.c#L7857)).
Thus the source has a hidden-room description but reset declarations that
open the door pair; current boot or live state was not inspected.

Room #93616, “Above The Great Fountain,” carries one WLD fall marker with
value 2 and a down exit to fountain room #93610
([room and marker](../../../areas/wld/bordello.wld#L356)). It is the only fall
marker; there are no current records. The fall routine is in
[falling.c](../../../src/world/falling.c#L113).

## Inactive WLD approaches

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without
parse issues and found no room-VNUM overlap with Bordello. It found two
non-overlapping source-only approaches, both in inactive `surf.wld`:

| Inactive source edge | Bordello target | Status |
| --- | --- | --- |
| [Surf #225923 south](../../../areas/wld/surf.wld#L543779) | #93600 | Source-only; not active ingress |
| [Surf #226224 north](../../../areas/wld/surf.wld#L550100) | #93600 | Source-only; not active ingress |

The curated source-only total rises from 3,442 to 3,444 overall and from 207
to 209 for Surf; the reviewed `ud_map2` subset remains 16. Preserve both
records on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
Its 145 active inbound exits remain unchanged. Add no Bordello candidate row.

## Typed dependencies and code bindings

The four outside object resets in Bordello's own ZON use VNUM #45 in rooms
#93644, #93646, #93656, and #93665
([reset block](../../../areas/zon/bordello.zon#L19)). The comments say this
object type is not defined in the local `.obj`. Among standard `*.obj` files
whose stems occur in the active `areas/AREA` manifest, `heavens.obj` is the
only source defining #45; it is a slot machine
([prototype](../../../areas/obj/heavens.obj#L525)). Other definitions occur
in inactive `dalvik`, `hostel`, and `mavulsk` object files; an additional
archival copy is in `lornecro.obj.caplor`, whose nonstandard extension is not
the active `lornecro` object input. The generated `areas/world.obj` is absent
from this checkout and runtime resolution was not tested, so this is a
source-level target attribution. Confirm whether the slot machine is intended
in the four Bordello bedrooms or whether the resets refer to a retired/shared
object.

Exact scans found no outside ZON reset-role declarations targeting Bordello
rooms, mobiles, or objects. No external mobile or object prototype duplicates
its local VNUMs. The scan of 872 external type-25 portal declarations and 380
external type-29 declarations found no target in Bordello. A story-sidecar
request ID contains the digit string `93617`, but its exact item VNUMs are
#55130, #55360, and #55351; this is a string-ID collision, not a typed
reference ([Wh sidecar](../../../areas/story/wh.story.json#L934)).

Two code-bound routes enter Bordello outside its WLD boundary count:

| Code route | Target | Conditions visible in source |
| --- | --- | --- |
| `do_shift_prime` | Room #93665 is one of 14 Githzerai destinations | The routine requires an Astral-plane location, no active fight, and an available innate cooldown; it chooses a random valid room from the list ([destination list and routine](../../../src/classes/innates.c#L2005)). |
| Chaos portal alias `vella` | Room #93610 | The alias is in `portdata`; the portal command resolves the room through `real_room` ([destination](../../../src/combat/chaos.c#L20), [dispatcher](../../../src/combat/chaos.c#L337)). |

Seven local mobile callback assignments are present for Vella #93600, workers
#93602, #93603, and #93611, bouncer #93605, rogue #93607, and tired young
man #93613 ([bindings](../../../src/specs/specs.assign.c#L352)). The three
worker handlers accept a 500-copper give and then offer follower behavior;
see, for example, the [well-built worker callback](../../../src/specs/specs.grove.c#L645).
These are code-backed interactions, not native Q/QA quest contracts. No
interaction was tested in game.

## Reset declarations and interpretation

The active ZON contains 63 mobile (`M`), 13 object (`O`), 26 equipment
(`E`), 12 carried-object (`G`), and two door (`D`) commands, for 116 total.
The four `O` commands for object #45 are external typed dependencies; the
other local object resets include the two fountain placements and treasure
room equipment. The 63 `M` commands declare reset inputs, not current
occupants. Reset mode 2 may reset an occupied zone according to the source
condition in [events.c](../../../src/world/events.c#L429); nonzero modes are
scheduled by [new_events.c](../../../src/world/new_events.c#L2068). These
source rules do not establish the current reset interval or door state.

## Title, history, and disposition

The normalized-title scan across 442 current `.zon` titles finds
“Vella's Bordello” only on `bordello`. Followed WLD, MOB, OBJ, ZON, and SHP
histories retain the same stem. The bounded history includes the 2009 move
into `areas/` (`642da5400`), a WLD typo edit in 2011 (`49dc41e31`), 2012 MOB
and OBJ format changes (`bb0d59425`), 2013 area edits (`71d731c5b`), 2014
shop cleanup (`1c59fb690`), a 2015 ZON quest-item flag pass (`f18992b8f`),
and 2016 spelling changes to the MOB and ZON (`79e4934f9`). These are
same-stem revisions; the bounded paths show no alternate-stem rename,
distinct predecessor, or whole-zone replacement.

Keep active `bordello` **DO NOT MOVE**. Preserve its three active WLD arrivals,
one reciprocal Surface exit, reset-open hidden-room doors, the external #45
object dependency pending builder confirmation, both source-only Surf
approaches on Surf's existing **RETAIN IN PLACE** row, and the two code-bound
arrival routes. No Bordello attic-candidate row is warranted by these
findings. Live access, current resets, shop operation, and interaction
outcomes remain unqualified; see the
[source-story dossier](../zone-stories/BORDELLO.md) for builder questions.
