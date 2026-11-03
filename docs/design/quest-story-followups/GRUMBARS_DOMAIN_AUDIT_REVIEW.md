# Grumbar's Domain: repository-wide connectivity and dependency audit

**Checkpoint 81; priority 121 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive Grumbar's Domain dossier](../zone-stories/GRUMBARS_DOMAIN.md)
with the active WLD graph, inactive WLD inputs, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored edges
and references do not establish ordinary reachability, safe traversal, source
availability, or played qualification.

## Active WLD graph

The active manifest contains 351 WLD contributors. Grumbar's Domain has 199
rooms and 794 exits; all 794 targets resolve. The local graph is one weak
component of 199 rooms, with 793 internal exits and one outward boundary. The
outbound edge to the Elemental Plane of Earth has an active reciprocal return.
Two other active inbound edges from that plane enter room #131364 without a
reverse local edge.

| Boundary | Reciprocal edge | Cross-area register rows |
| --- | --- | --- |
| #131200 up to Plane #23908 ([exit](../../../areas/wld/earthp.wld#L17)) | Plane #23908 down to #131200 ([return](../../../areas/wld/plane_earth_one.wld#L4210)) | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1299), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L386) |
| Plane #23926 up to #131364 ([exit](../../../areas/wld/plane_earth_one.wld#L4899)) | Incoming-only; no reverse local edge | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L387) |
| Plane #23928 south to #131364 ([exit](../../../areas/wld/plane_earth_one.wld#L4937)) | Incoming-only; no reverse local edge | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L388) |

There are no unresolved active exits.

## Inactive Duris3 map records

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. Its 796
`Duris3.wld` exit records with targets in Grumbar's Domain include 400 exits
from non-overlapping source rooms. They cover all 199 active target rooms:

| Source-only map band | Direction and target mapping | Evidence |
| --- | --- | --- |
| #130900–#131098 | 199 south (`D2`) exits, one per source room, to #131200–#131398 in order | [first](../../../areas/wld/Duris3.wld#L648913), [last](../../../areas/wld/Duris3.wld#L653071) |
| #131500–#131698 | 199 north (`D0`) exits, one per source room, to #131200–#131398 in order | [first](../../../areas/wld/Duris3.wld#L661505), [last](../../../areas/wld/Duris3.wld#L665663) |
| #131499 | East (`D1`) to #131200 | [exit](../../../areas/wld/Duris3.wld#L661488) |
| #131399 | West (`D3`) to #131398 | [exit](../../../areas/wld/Duris3.wld#L659396) |

These 400 edges are authored in `Duris3.wld`, which is absent from
[`areas/AREA`](../../../areas/AREA); it is tracked as a source-only component
([coverage row](ZONE_COVERAGE_REGISTER.csv#L387)). The edges are source-only
map records, not active ingress. The same inactive file contains 199 other room records whose
VNUMs overlap the active #131200–#131398 range. Their generic “The Ocean”
titles and map-grid exits differ from the active earth-room records (compare
[`Duris3` #131200](../../../areas/wld/Duris3.wld#L655201) with
[`earthp` #131200](../../../areas/wld/earthp.wld#L1)). The non-overlapping
400-record set raises the curated source-only total from 685 to 1085.
Keep those source-map declarations separate from active routes and from the
199 same-numbered room identities.

## Typed references and compiled numeric matches

The external QST scan found no command that consumes a Grumbar's Domain
prototype. No external SHP product, story-sidecar numeric reference, external
ZON reset-role reference, or foreign mobile/object prototype definition
matched. The area has no local SHP file. All 231 other parsed story sidecars
had no exact structured reference to its room, mobile, or object VNUMs.

The external type-25 scan found no destination in a Grumbar's Domain room
among 871 declarations from other object files: 712 active and 159
source-only. The local crevice object #131223 targets room #131396 as already
described in the zone dossier.

The only direct C/C++ numeric match is mobile #131232 in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L581), where it binds the
purple-worm special. This is the imported prototype's existing code binding,
not another quest hook. No direct local room or object number match was found.

## Title, history, and disposition

The normalized title from [`earthp.zon`](../../../areas/zon/earthp.zon#L2) is
unique among the 351 active contributors. The global active-WLD duplicate scan
found only room #51800 in Hell Two and Hell Three; Grumbar's Domain has no
active duplicate room VNUM. The 199 same-numbered `Duris3.wld` Ocean rooms are
inactive map records with different titles and topology, not active duplicates
or evidence of a replacement.

The followed WLD and ZON histories begin with the 2010 Grumbar renumbering and
upload sequence; they identify no distinct predecessor or replacement in the
curated register. The current WLD and ZON were both updated in the 2026-09-22
builder sync commit `e91618fa4`. The WLD history includes the room #131376
edit described in commit `ed63223d9` as allowing mobs to wander from that
dispersal room, then the builder sync adjusts several room coordinate fields.
The ZON sync changes four dust-mephit placements to follow their room's pillar
and adds earthen bracers to two dao-guardian reset groups. These tracked edits
remain part of the current source.

Grumbar's Domain remains a current, un-commented
[`areas/AREA` contributor](../../../areas/AREA#L376) and is **DO NOT MOVE**.
The existing dossier covers the active boundary records, imported reset
groups, the local type-25 crevice, and the direct purple-worm binding. Active
and source-only exits do not qualify door/reset state, ordinary access,
successful use, safe travel, or return behavior. Tooling/editor closure,
persisted references, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 793: current typed census refresh

### Active ZON prototype operands

The active earthp.zon has 543 reset commands: 14 D door rows and 529 typed
prototype commands (160 M, 104 F, seven O, 96 G, and 162 E). F loads a
follower mobile after an M command, as documented by the reset handler
([db.c](../../../src/world/db.c#L7747)). The typed fields comprise 231 local
mobile refs, 186 local object refs, 33 imported mobile refs, and 79 imported
object refs. All 112 imported refs resolve to same-type current prototypes;
no ZON prototype operand is unresolved.

Five active owners account for those imports:
- Heavens: 25 F mobile refs (#1121 thirteen times, #1122 twelve) and 15 object
  refs (#193 seven, #360 one, #434 six, #749 one).
- Nyneth: 12 G resets of object #22936
  ([first](../../../areas/zon/earthp.zon#L93)).
- Ravenloft: six G resets of object #58388
  ([reset](../../../areas/zon/earthp.zon#L118)).
- Tradeskills: 46 object reset refs: #400104 eighteen times; #400164,
  #400169, #400174, and #400194 six times each; #400272 and #400275 twice
  each ([first shard reset](../../../areas/zon/earthp.zon#L108)).
- Plane of Earth: seven M resets of mobile #23802 and one of #23801
  ([#23802](../../../areas/zon/earthp.zon#L510),
  [#23801](../../../areas/zon/earthp.zon#L574)).

The Heavens refs include follower mobiles #1121/#1122 and objects #193, #360,
#434, and #749 ([Heavens mobiles](../../../areas/mob/heavens.mob#L2299),
[objects](../../../areas/obj/heavens.obj#L2294)). Nyneth object #22936,
Ravenloft #58388, and Plane mobiles #23801/#23802 resolve in their current
same-type sources. Tradeskills' active object file defines the seven imported
items ([#400104](../../../areas/obj/tradeskills.obj#L837)). The unregistered
tradeskills.old.obj component duplicates five of those IDs
(#400104/#400164/#400169/#400174/#400194); that overlap does not establish
that the old path is loaded or selected
([old-source review](TRADESKILLS_OLD_AUDIT_REVIEW.md)).

### QST and story references

The local QST has 13 R/G item values: #131211, #131232, and #131227 each occur
once and are local; ten G fields use external Tradeskills object #400104
([QST](../../../areas/qst/earthp.qst#L85)). The structured story sidecar has
12 typed VNUM values: six mobile and four giver values, all local, plus item
values #131227 (local) and #400104 (Tradeskills)
([sidecar](../../../areas/story/earthp.story.json#L23)). The local story's
#400104 reference is the same active object used by the QST and reset imports.

None of the 231 other current story sidecars contains a typed reference to an
Earthp room, mobile, or object. No external QST item/giver header, active ZON
reset role, SHP product, or external MOB/OBJ definition targets a local Earthp
prototype. Earthp has no local SHP/shop, and no same-type external mobile or
object prototype duplicates a local ID.

### Item travel and compiled reference

The local crevice object #131223 is type 25 and targets Earthp room #131396.
The fractured-wall object #131210 is type 29 and targets room #131362; both
destinations resolve locally. The scan of 871 external type-25 and 379
external type-29 declarations found no room target in Earthp
([crevice](../../../areas/obj/earthp.obj#L390),
[wall switch](../../../areas/obj/earthp.obj#L172)). These item routes add no
authored WLD exit.

The only direct C/C++ numeric reference to a local Earthp prototype is the
purple-worm binding for mobile #131232
([assignment](../../../src/specs/specs.assign.c#L581)). The typed reset
inventory does not establish live reset admission, follower behavior,
current stock, loader/editor selection, or persisted player references.
No source area, candidate disposition, gameplay, or WLD total changed.
