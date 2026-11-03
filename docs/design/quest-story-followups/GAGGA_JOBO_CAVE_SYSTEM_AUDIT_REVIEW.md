# The Gagga'Jobo Cave System: repository-wide connectivity and dependency audit

**Checkpoint 88; priority 128 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/GAGGA_JOBO_CAVE_SYSTEM.md)
with the active WLD graph, inactive WLD sources, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored edges
and static code destinations do not establish ordinary availability, safe
travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. The Gagga'Jobo Cave System has
23 rooms and 45 exits; all 45 resolve. Its internal-exit graph has one weak
component of 23 rooms. Its only active boundary is reciprocal:

| Cave edge | Active return | Cross-area register rows |
| --- | --- | --- |
| #19000 west to Underdark #842047 | Underdark #842047 east to #19000 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L309), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1774) |

There are no negative or positive unresolved exits, fall-chance fields, or
current fields. The one global active-WLD duplicate room VNUM, #51800 in Hell
Two and Hell Three, is unrelated. This authored pair does not qualify live
doors, access, reset state, route safety, or other dynamic travel.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. It found no
inactive source-room VNUM overlap with the cave and no inactive exit into an
active cave room. The curated source-only reference total remains 1,092, and
the reviewed Surf subset remains 121. There are no source-only approaches to
add.

## Typed references and compiled numeric matches

External typed scans found no QST item command, SHP product, or ZON reset
matching a Gagga'Jobo object or mobile. They also found no external mobile or
object prototype definition, no destination into the cave among 872 external
type-25 declarations (713 active, 159 source-only), and no exact `item_vnums`
hit across 231 parsed external story sidecars.

A context-reviewed C/C++ scan found one dynamic room reference:

| Local target | Static reference | What the source establishes |
| --- | --- | --- |
| Room #19022 | [Ixarkon veil handler assignment](../../../src/specs/specs.assign.c#L1874), [random destination list](../../../src/specs/specs.ixarkon.c#L19) | Ixarkon object #96402 is bound to `illithid_teleport_veil`, whose random destination list includes room #19022. The handler selects a valid room and relocates the actor when the supported ENTER command is invoked; object access, selection of this destination, and a successful played arrival are not established by the literal. This dynamic code route is not an ordinary WLD edge. |

Remaining raw numeric matches are constants, decimal values, comments, or
namespace collisions. Room, mobile, object, reset, and travel values were
classified by source context rather than treated as interchangeable.

## Title, history, and disposition

The color-stripped title in [goblincave.zon](../../../areas/zon/goblincave.zon#L2)
normalizes to “The Gagga'Jobo Cave System” and is unique among 351 active
contributors. The followed WLD and ZON histories include older source edits
and the 2009 area-tree path move; the curated
[replacement register](DUPLICATE_AND_REPLACED_ZONES.md) identifies no distinct
predecessor or replacement. The active, un-commented
[areas/AREA row](../../../areas/AREA#L95) remains **DO NOT MOVE**.

The existing dossier covers the six local contracts, four paid services,
shared recipes, actor bindings, resets, and selected outside-source closure.
This repository-wide scan adds the active Underdark boundary, confirms the
absence of inactive-WLD approaches and typed external producers, and classifies
the Ixarkon veil destination. Editor/tooling closure, persisted identity,
dynamic route availability, door/reset state, safe travel, and played
qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 812 current typed dependency closure

The local zone has 61 typed prototype operands: 23 mobile operands and 38
object operands (two `O`, ten `P`, four `E`, and 22 `G`). All resolve locally;
all ten mobiles and 14 of 15 objects are reset-used. Object #19012 is the
exception in the ZON inventory but is a local QST reward. The six local QST
contracts contain 20 typed item fields across rewards and inputs, all resolving
to local objects; the local sidecar's typed giver/item references also resolve
locally. No outside active or source-only ZON reset/room role, QST giver/item,
SHP keeper/product, typed story-sidecar role, or duplicate external MOB/OBJ
definition targets a cave prototype.

There is no local type-25/type-29 object and no external type-25/type-29 target
into the cave. No local or external WLD key field targets a cave object. The
exact compiled scan still finds only the Ixarkon veil's dynamic room
destination #19022; it is not an ordinary WLD edge. The active graph remains
45/45 resolved with its reciprocal Underdark boundary, and the inactive-WLD
scan still finds no source-only approach. No source, registration, or
candidate disposition changes. Editor/tooling, persisted identity, dynamic
route availability, reset state, and live access remain pending.
