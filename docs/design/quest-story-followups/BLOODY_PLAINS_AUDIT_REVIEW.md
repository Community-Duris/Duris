# Bloody Plains repository-wide audit supplement

This supplement reconciles the active Bloody Plains map, its inbound and
outbound WLD boundaries, fall and door reset declarations, typed consumers,
and bounded title/history evidence with the [source-story dossier](../zone-stories/BLOODY_PLAINS.md).
The reviewed source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`.
No gameplay or area source is changed here.

## Active source identity and story boundary

`bloody_plains` is an un-commented full-world contributor in both the active
[world manifest](../../../areas/AREA#L162) and the separate
[quest source index](../../../areas/qst/AREA#L150). It has 199 WLD rooms
(#33500–#33699, with #33687 absent), nine mobile prototypes, eighteen object
prototypes, and 100 ZON reset commands. The zone header names it “The Bloody
Plains” and sets reset mode 0 ([WLD](../../../areas/wld/bloody_plains.wld#L1),
[ZON](../../../areas/zon/bloody_plains.zon#L1)). There is no local `.qst`,
`.shp`, or story sidecar, and no local native Q/QA contract. Membership in
`areas/qst/AREA` does not mean a same-stem quest file exists.

The environmental story is the devastated aftermath of an ancient battle,
with fog, roads, craters, old remains, surviving dangerous creatures, and a
locked shack occupied by a hag. The unresolved prose question is the contrast
between “incapable of supporting organic life” and the source's mobile resets,
pool insects, and crater moss; see the [dossier](../zone-stories/BLOODY_PLAINS.md#creatures-and-surviving-life).

## Active WLD topology and boundaries

The active WLD scan parses 351 manifest files and all 199 Bloody Plains rooms.
Its 704 authored exits all resolve: 698 stay within the area and six leave it.
The local graph is one weak component of 199 rooms, with no negative or
positive unresolved targets. There are no current records.

Seven active WLD exits enter the area:

| Source edge | Entry room | Direct return |
| --- | --- | --- |
| [Killing Fields #33422 east](../../../areas/wld/killing_fields.wld#L3835) | [#33597](../../../areas/wld/bloody_plains.wld#L2988) | No direct west exit from #33597 to #33422 |
| [Killing Fields #33437 east](../../../areas/wld/killing_fields.wld#L4323) | [#33612](../../../areas/wld/bloody_plains.wld#L3443) | [#33612 west](../../../areas/wld/bloody_plains.wld#L3470) to #33437 |
| [Killing Fields #33452 east](../../../areas/wld/killing_fields.wld#L4801) | [#33627](../../../areas/wld/bloody_plains.wld#L3907) | [#33627 west](../../../areas/wld/bloody_plains.wld#L3931) to #33452 |
| [Killing Fields #33467 east](../../../areas/wld/killing_fields.wld#L5291) | [#33642](../../../areas/wld/bloody_plains.wld#L4367) | [#33642 west](../../../areas/wld/bloody_plains.wld#L4394) to #33467 |
| [Killing Fields #33482 east](../../../areas/wld/killing_fields.wld#L5781) | [#33657](../../../areas/wld/bloody_plains.wld#L4839) | [#33657 west](../../../areas/wld/bloody_plains.wld#L4866) to #33482 |
| [Killing Fields #33497 east](../../../areas/wld/killing_fields.wld#L6201) | [#33672](../../../areas/wld/bloody_plains.wld#L5325) | [#33672 west](../../../areas/wld/bloody_plains.wld#L5347) to #33497 |
| [Surface #586575 north](../../../areas/wld/surface.wld#L1817423) | [#33681](../../../areas/wld/bloody_plains.wld#L5577) | [#33681 south](../../../areas/wld/bloody_plains.wld#L5596) to #586575 |

The six outbound edges are those five west exits to Killing Fields plus the
south exit at #33681 to Surface. From each of the seven inbound anchors, raw
directed traversal through authored local exits reaches all 199 rooms, and all
199 can return to the respective anchor. This is a static graph result that
treats authored exits as usable; it does not test doors, player access, or a
live journey. The missing direct west return from the #33422 boundary is
explicit, though alternate local routes and the other boundary anchors exist.
See the [Killing Fields boundary inventory](KILLING_FIELDS_AUDIT_REVIEW.md#active-world-and-entry-boundaries)
and the [Bloody Plains WLD](../../../areas/wld/bloody_plains.wld#L2988).

## Reset doors and falls

The only reset-closed local passage is the pair serving shack room #33500.
ZON closes #33500 east and closes plus locks #33501 west with key #33513
([first reset](../../../areas/zon/bloody_plains.zon#L8),
[second reset](../../../areas/zon/bloody_plains.zon#L12)). The rusty key
prototype resets in #33695 ([placement](../../../areas/zon/bloody_plains.zon#L30)).
Excluding those two closed door edges from the raw graph leaves 198 of 199
rooms reachable and returnable from each inbound anchor; #33500 is the
excluded shack room. This static estimate does not simulate opening, picking,
key custody, or live reset timing.

The WLD contains twelve chance-fall markers, each with a downward exit:

| Room | Chance | Downward destination | WLD marker |
| ---: | ---: | ---: | --- |
| #33514 | 87% | #33698 | [416](../../../areas/wld/bloody_plains.wld#L416) |
| #33531 | 85% | #33695 | [959](../../../areas/wld/bloody_plains.wld#L959) |
| #33536 | 100% | #33691 | [1112](../../../areas/wld/bloody_plains.wld#L1112) |
| #33557 | 90% | #33690 | [1777](../../../areas/wld/bloody_plains.wld#L1777) |
| #33569 | 93% | #33696 | [2157](../../../areas/wld/bloody_plains.wld#L2157) |
| #33625 | 98% | #33688 | [3891](../../../areas/wld/bloody_plains.wld#L3891) |
| #33654 | 78% | #33697 | [4775](../../../areas/wld/bloody_plains.wld#L4775) |
| #33688 | 100% | #33689 | [5760](../../../areas/wld/bloody_plains.wld#L5760) |
| #33691 | 100% | #33692 | [5814](../../../areas/wld/bloody_plains.wld#L5814) |
| #33692 | 100% | #33693 | [5835](../../../areas/wld/bloody_plains.wld#L5835) |
| #33693 | 100% | #33694 | [5856](../../../areas/wld/bloody_plains.wld#L5856) |
| #33698 | 100% | #33699 | [5937](../../../areas/wld/bloody_plains.wld#L5937) |

The room loader reads `F` into `chance_fall` and clears it only when a room
has no downward exit ([loader](../../../src/world/db.c#L1395)). Movement checks
that chance before calling `falling_start` ([movement](../../../src/cmd/interp.c#L1898));
the fall routine has trust, levitation, flight, riding, and climbing checks
([fall handling](../../../src/world/falling.c#L113)). The two 100% crater
sequences continue downward through more fall-marked rooms. Their practical
escape, character-specific behavior, recovery, and return were not played or
otherwise runtime-tested.

The reset mode is 0. The boot-time scheduler adds periodic zone reset events
only when `reset_mode` is nonzero ([scheduler](../../../src/world/new_events.c#L2068)).
The 100 source reset commands therefore do not establish periodic replenishment
of the key, objects, or creatures. This review does not infer boot contents or
current live state.

## Inactive WLD approach and typed reset dependencies

The all-95 unregistered-WLD scan parsed 527,791 rooms and 2,080,773 exits with
no parse errors and no room-VNUM overlap with Bloody Plains. It found one
source-only approach: inactive `surf.wld` room #280345 north to active room
#33681 ([source room](../../../areas/wld/surf.wld#L1685945),
[exit and target](../../../areas/wld/surf.wld#L1685949)). This is not active
ingress. It raises the curated source-only total to 3,441, the reviewed Surf
subset to 207, and leaves the `ud_map2` subset at 15. Preserve this record with
the existing `surf` candidate row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

There are two typed reset relationships for mature leucrotta mobile #33500
outside its home source:

- Active `mobs_surf.zon` lines 10–29 declare twenty M resets into active
  `mobs_surf` room #31412 ([reset source](../../../areas/zon/mobs_surf.zon#L10),
  [room](../../../areas/wld/mobs_surf.wld#L180)). This is an active source
  consumer and must be preserved with the prototype owner. It does not prove
  the mobile is currently placed there.
- Unregistered `surf.zon` lines 469–482 declare fourteen source-only M resets:
  four into #269647, three into #274438, three into #270534, and one each into
  #267215, #264827, #263026, and #263332 ([reset source](../../../areas/zon/surf.zon#L469)).
  These are not active resets. Keep them with candidate row 97.

Typed/exact scans found no external QST or sidecar item reference, SHP product
reference, duplicate external mobile/object prototype, or C/C++ numeric
callback binding for the local VNUMs. Across 872 external type-25 declarations
and 380 external type-29 declarations, none targets a Bloody Plains room,
mobile, or object. The only outside ZON reset-role declarations found are the
active `mobs_surf` and source-only `surf` imports above. The area has no local
SHP; its own reset file declares seven root object resets, seven contained
object resets, three equipment resets, one carried object, eighty mobile
resets, and the two door resets.

## Title, history, and disposition

Color-stripping and normalizing the 442 current `.zon` titles yields one
`The Bloody Plains` match, in [bloody_plains.zon](../../../areas/zon/bloody_plains.zon#L2).
Followed WLD, MOB, OBJ, and ZON histories retain the `bloody_plains` stem.
The WLD path includes the 2009 area-tree move (`642da5400`) and 2013 content
edit (`71d731c5b`). MOB and ZON also include the 2015 quest-item flag pass
(`f18992b8f`); MOB and OBJ include the 2012 field-format edit (`bb0d59425`).
These are same-stem changes. The bounded history shows no alternate-stem
rename, distinct predecessor, or whole-zone replacement.

Keep active `bloody_plains` **DO NOT MOVE**. Preserve its seven active inbound
boundaries, six outbound routes, shack key/door relationship, fall warnings,
and the active `mobs_surf` reset import. Do not create an attic candidate row
for an active contributor. Keep the existing `surf` candidate **RETAIN IN
PLACE** under row [97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97), including
the new source-only WLD approach and unregistered reset imports. Neither the
inactive route nor the reset references establish live access or migration
clearance.
