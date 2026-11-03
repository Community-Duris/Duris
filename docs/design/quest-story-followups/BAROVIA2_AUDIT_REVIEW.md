# The Realm of Barovia Continued: repository-wide source audit supplement

This supplement reconciles the [source-comprehensive Barovia Continued dossier](../zone-stories/THE_REALM_OF_BAROVIA_CONTINUED.md)
with the active world graph, the all-source WLD sweep, typed cross-zone references,
the normalized title, and tracked source history. These are static repository
findings; they do not establish live access, reset availability, a played receipt,
or persisted player history.

## Active boundaries and local graph

Barovia Continued has 196 rooms and 494 authored exits. All 494 targets resolve
in the active WLD set: 492 exits remain within the area and two leave it. The
local WLD-only weak graph has five components of 163, 30, 1, 1, and 1 rooms.
The two outbound exits are:

| Local exit | Destination | Boundary record |
| --- | --- | --- |
| Barovia Continued #58800 south | Barovia #91162 | [outbound record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L814) |
| Barovia Continued #58958 south | Castle Ravenloft #58567 | [outbound record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L815) |

Five active exits enter Barovia Continued:

| Incoming exit | Local destination | Relationship | Boundary record |
| --- | --- | --- | --- |
| Barovia #91162 north | #58800 | Reciprocal with the first outbound exit | [record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1119) |
| Castle Ravenloft #58567 north | #58958 | Reciprocal with the second outbound exit | [record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L813) |
| Catacombs #59356 down | #58958 | One-way authored arrival | [record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L819) |
| Catacombs #59398 north | #58811 | One-way authored arrival | [record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L822) |
| The Realm #14210 down | #58984 | One-way authored arrival | [record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L261) |

Thus the two Barovia and Castle boundaries are reciprocal pairs, while three
Catacombs/Realm exits arrive without a reverse WLD exit. The five components
describe authored room exits only. The zone dossier separately maps type-25
portals, control objects, reset behavior, and other travel mechanics; the static
graph does not prove actual access, safe traversal, or a return route.

## Unregistered WLD source sweep

The scan parsed all 95 WLD files absent from the active area manifest: 527,791
rooms and 2,080,773 exits, with zero parse issues. None of those source rooms
shares a VNUM with Barovia Continued, and none of their exits targets a local
room. This adds no source-only ingress; the curated source-only WLD total remains
600. The unregistered files are not active contributors through the current
manifest.

## Typed dependencies and foreign ownership

Three active foreign QST item commands reference two Barovia Continued object
prototypes:

| Foreign owner | Exact command | Local prototype and context |
| --- | --- | --- |
| Catacombs / Rahadin | `G I 58803`, paired with removal of five fortune items #59202 | Crystal ball #58803 is a local Barovia Continued object. The Catacombs QST and its sidecar describe Rahadin's separate exchange; it is not a Barovia Continued receipt. ([QST input](../../../areas/qst/ravenloft2.qst#L1505), [QST output](../../../areas/qst/ravenloft2.qst#L1506), [object](../../../areas/obj/barovia2.obj#L46), [Catacombs sidecar](../../../areas/story/ravenloft2.story.json#L774)) |
| Surface Keeps / hunter-killer | `R I 58814` twice | Smoke bomb potion #58814 is local to Barovia Continued. The foreign QST text describes a body-part exchange, while its completion key also lists two #58814 items; retain the exact cross-zone dependency and ask the builder whether this input is intended. ([body-part text](../../../areas/qst/surfacekeeps.qst#L560), [commands](../../../areas/qst/surfacekeeps.qst#L566), [sidecar key](../../../areas/story/surfacekeeps.story.json#L741), [object](../../../areas/obj/barovia2.obj#L245)) |

All 231 external story sidecars parsed without issues. Two contain typed
`item_vnums` references to local objects. Barovia's sidecar names the optional
outer Ravenloft key #58826 and explains its sources in Barovia Continued; the
Catacombs sidecar identifies Madam Eva's crystal ball #58803 in Rahadin's story.
These are foreign-context item guidance, not additional local completions. The
fields are object-typed even where the number also occurs in another prototype
namespace.
([Barovia sidecar](../../../areas/story/barovia.story.json#L733),
[key object](../../../areas/obj/barovia2.obj#L462),
[Catacombs sidecar](../../../areas/story/ravenloft2.story.json#L782))

Foreign reset sources also reuse local prototypes. Castle Ravenloft gives local
potion #58808 to Vistani secret police and places local mobile #58846, outer
Megosh, in room #58567 ([Castle resets](../../../areas/zon/ravenloft.zon#L1257),
[Megosh placement](../../../areas/zon/ravenloft.zon#L1276),
[local prototypes](../../../areas/obj/barovia2.obj#L133) and
[mobile](../../../areas/mob/barovia2.mob#L762)). The Catacombs place Barovia
Continued mobs #58840, #58842, and #58843 in room #59182 and equip its vine proc
#58837 ([mob resets](../../../areas/zon/ravenloft2.zon#L1323),
[proc resets](../../../areas/zon/ravenloft2.zon#L1466),
[local prototypes](../../../areas/mob/barovia2.mob#L663) and
[object](../../../areas/obj/barovia2.obj#L619)). The comprehensive dossier's
full foreign-reset review records 13 complete groups. These reset declarations
identify cross-zone source ownership; they do not establish live population or
guaranteed stock.

The scan covered 867 foreign type-25 portal declarations and found one targeting
local room #58964, Jagg-ed Pinnacle: Catacombs object #59091 declares destination
#58964 and is reset at Catacombs room #59169
([portal](../../../areas/obj/ravenloft2.obj#L1347),
[destination](../../../areas/obj/ravenloft2.obj#L1353),
[placement](../../../areas/zon/ravenloft2.zon#L930),
[local room](../../../areas/wld/barovia2.wld#L3773)). This is a foreign travel
control, not a quest receipt. The bounded scans found no external mobile or
object prototype definitions reusing Barovia Continued VNUMs and no external
SHP product reference to a local object.

Shared code uses also remain typed by context. Chaos equipment tables contain
object #58815 in two optional slots ([first table](../../../src/account/chaos_eq_data.h#L567),
[second table](../../../src/account/chaos_eq_data.h#L572)).
Object #58825 is bound to `barovia_undead_necklace`
([assignment](../../../src/specs/specs.assign.c#L2175)). Mobile #58835, Chenovog,
appears in the random-equipment high-drop list and has the named mobile macro;
the bonus lookup loop is commented out ([list](../../../src/item/randomeq.c#L144),
[disabled lookup](../../../src/item/randomeq.c#L644),
[mobile macro](../../../src/world/vnum.mob.h#L21)). These references do not add
QST ownership or prove an active drop bonus.

## Title, tracked history, and disposition

The active manifest lists `barovia2` as `*588-589`
([AREA entry](../../../areas/AREA#L241)); the ZON title is “The Realm of Barovia
Continued” ([header](../../../areas/zon/barovia2.zon#L2)). The normalized title
scan found one match among 351 active zone titles. The curated
[duplicate and replacement register](DUPLICATE_AND_REPLACED_ZONES.md) has no
Barovia Continued / `barovia2` entry.

The tracked WLD, MOB, OBJ, ZON, and QST paths reach the 2009 area-tree move
`642da5400`; later history includes the 2012–2014 Barovia2 edits, including the
port-room/key changes (`55c62c54e`, `83cc67c42`). The story sidecar first appears
at `9f5512449` on 2026-10-05. This path history does not identify a distinct
replacement source. Barovia Continued remains a full active `areas/AREA`
contributor, so its coverage-register disposition stays **DO NOT MOVE**.

## Recommended reconciliation

- Keep the five Barovia Continued QST receipts and their local ownership as
  recorded in the source dossier. Cross-zone QST, sidecar, reset, code, and
  portal references remain attributed to their declaring areas and systems.
- Ask the Surface Keeps builder to confirm the two `R I 58814` references in
  the hunter-killer recipe, since its body-part text and completion key both
  need to be read together. If intentional, retain the documented dependency;
  if not, resolve it as a separately reviewed recipe/content change.
- Keep Castle/Catacombs reset declarations and the Catacombs portal mapped as
  foreign-owned references. Qualify live availability and travel separately
  before making player-facing access or renewal claims.
- Keep the zone in the active manifest. The inventory supports no attic move
  or native gameplay repair.

This review is a bounded static source audit. Indirect code paths, editor and
tooling consumers, persisted player data, runtime stock, and played discovery,
access, combat, and return journeys remain outside this checkpoint.
