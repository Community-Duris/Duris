# Halfcut Hills inactive-source audit

Canonical key: component:halfcut_hills. Physical coverage row 401 / CSV line
402; candidate dependency row 51 / CSV line 51. Reviewed October 11, 2026,
Checkpoint 488, against source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697. This is a static source,
connectivity, typed-identity, history, and relationship review. It does not
establish runtime loading, gameplay, editor ownership, persisted world state,
or permission to activate, merge, or remove these files.

## Source inventory and numeric ownership

The unregistered component has [WLD](../../../areas/wld/halfcut_hills.wld#L1),
[MOB](../../../areas/mob/halfcut_hills.mob#L1),
[OBJ](../../../areas/obj/halfcut_hills.obj#L1),
[ZON](../../../areas/zon/halfcut_hills.zon#L1), and
[QST](../../../areas/qst/halfcut_hills.qst#L1), but no SHP file or candidate
story sidecar. ZON header #1 is titled “Halfcut Hills,” ends at #27235, and
uses reset mode 2. Its WLD contains #27001–#27235. A fresh normalized
scan of all 442 current `.zon` title lines finds this candidate’s “Halfcut
Hills” beside active “The Halfcut Hills”. The titles differ only by the
leading article; this similarity does not resolve the separate source
ownership conflict documented below.

Both [areas/AREA](../../../areas/AREA#L135) and
[areas/qst/AREA](../../../areas/qst/AREA#L127) list active halfcut at
#270–#279; neither lists halfcut_hills. Active halfcut has ZON header #270
and rooms #27001–#27470. The inactive candidate’s 235 WLD room numbers all
overlap active halfcut room numbers. Its ZON header #1 and per-room zone
metadata also differ from the active #270 source. This is an exact numeric
identity conflict; it does not establish that the two source paths are a
rename or that either may be removed.

| Source type | Candidate halfcut_hills | Active halfcut | Exact-ID/source comparison |
| --- | --- | --- | --- |
| WLD | 235 rooms, #27001–#27235 | 470 rooms, #27001–#27470 | All 235 IDs overlap. After normalizing only each record’s zone-number field, 194 room blocks match and 41 differ. |
| MOB | 45 prototypes | 83 prototypes | All 45 candidate IDs overlap; none of the full prototype blocks is identical. |
| OBJ | 22 prototypes | 60 prototypes | All 22 IDs overlap; 14 blocks match and eight differ: #27008, #27012–#27015, #27019, #27026, and #27028. |
| QST | Two records, #27001 and #27002; no Q or QA contracts | Twelve giver records | Only #27002 overlaps, and its QST block differs. Candidate #27001 is not in active halfcut.qst. |
| ZON | 184 commands, zone #1, top #27235, mode 2 | 413 commands, zone #270, top #27470, mode 2 | The inactive reset set is not the active halfcut reset set. |

The active halfcut has its own
[story sidecar](../../../areas/story/halfcut.story.json) and
[source-comprehensive dossier](../zone-stories/THE_HALFCUT_HILLS.md). That
dossier explicitly says the similarly named halfcut_hills files are inactive
and do not define the active journal’s current behavior. Keep that active
story review distinct from this candidate-source review.

## Candidate WLD graph and target identities

The candidate WLD has 550 directions: 549 local edges and one outbound edge,
from #27001 D0 to #256031. Its local graph is one weak component of 235
rooms; directed traversal from #27001 reaches 226. There are no negative
targets, WLD fall-chance markers, or current fields.

The active loader resolves 549 of the candidate's 550 targets by number:
the 549 local destinations resolve to the same-number rooms in active
halfcut. The remaining target #256031 is not active. It has two different
definitions in unlisted files:
[surf.wld](../../../areas/wld/surf.wld#L1175747), "A Gradual Incline Covered
with Grass and Shrubs," and
[surface2011.wld](../../../areas/wld/surface2011.wld#L1176652), "The Azure
Waters of a Stormy Sea." Neither file is in areas/AREA. The surf room's D2
south exit returns numerically to #27001
([destination record](../../../areas/wld/surf.wld#L1175762)), opposite the
candidate's D0 north exit; the surface2011 room's D2 instead targets #256331
([destination record](../../../areas/wld/surface2011.wld#L1176667)).
Thus one unregistered source has a reciprocal record, while the competing
definition does not. Neither source is loaded as an active map.

Current active halfcut uses a different boundary: #27001 D0 targets active
surface #629274, whose south exit returns to #27001
([active boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L410),
[surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1649)). That return
lands in active halfcut because its #27001 is the listed source; it does not
activate the unlisted copy. The active Surface exit therefore creates one
numeric inbound match to a candidate vnum, but not a candidate-specific
arrival. The candidate's #256031 edge remains unresolved in the active/candidate
map set; the unregistered surf.wld reciprocal is documented at Checkpoint 900.

## Story, resets, and typed-reference boundary

Candidate QST contains only M dialogue: Old Grizzler’s greetings and rumors
about the hills, Unk, manticores, and a mine, plus a wounded dwarf’s warning
about poison. It has no Q or QA contract. The candidate dialogue is useful
source history, but the active halfcut dossier and sidecar own the reviewed
current journal/story treatment.

The 184 candidate ZON commands comprise 137 M, 20 D, 16 E, six O, three G,
and two P. All 43 distinct reset MOB references and all 20 distinct reset
OBJ references resolve to local candidate prototypes. MOBs #27031 and
#27042 and objects #27006 and #27007 are not referenced by this candidate’s
resets. This source scan establishes closure only for the candidate’s own
reset references.

Compiled code binds a crossbow procedure to mobile #27009
([spec assignment](../../../src/specs/specs.assign.c#L300)). Active halfcut
has its own source history and maps the shared #27009 identity to its loaded
prototype; the source binding does not establish ownership by the unlisted
halfcut_hills MOB. The candidate has no SHP, while active halfcut has a
separate shop record. Do not import or infer shop, journal, or code ownership
from shared vnums.

## Source history and disposition

Followed halfcut_hills file histories remain on the halfcut_hills stem through
the 2009 area-tree move and later edits. Active halfcut files have their own
same-stem history and later gameplay/story changes. No alternate-stem rename
or explicit whole-zone replacement was found. The active dossier’s explicit
inactive/current distinction, complete numeric overlap, changed boundary,
and divergent typed records provide strong evidence that these are parallel
source identities, while Git history does not grant permission to delete or
merge either path.

Keep candidate row 51 **RETAIN IN PLACE** and do not load both definitions.
Preserve the unregistered source and its history while owners resolve its
relation to active halfcut, the duplicate room/mobile/object IDs, the #256031
boundary, unused prototypes, editor/deployment copies, and persisted IDs.
Keep active halfcut’s sidecar, journal, resets, shop, code binding, and live
area ownership separate. See the [24 builder questions](HALFCUT_HILLS_BUILDER_QUESTIONS.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-488-halfcut-hills-inactive-source-graph),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-488-halfcut-hills-candidate-and-active-source-identities),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-488-halfcut-hills-retained-as-a-separate-inactive-source),
[candidate row 51](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L51), and
[coverage row 401](ZONE_COVERAGE_REGISTER.csv#L402). No source-area files,
code, tests, builds, runtime, server, database, or containers were changed
or used.

## Checkpoint 815: active Halfcut typed closure and candidate boundary

Reviewed October 12, 2026. This supplement records active `halfcut` roles
beside the separately unregistered `halfcut_hills` component already audited
above. It changes no area or native gameplay source.

The active [halfcut.zon](../../../areas/zon/halfcut.zon) has 742 typed
operands: 309 mobile, 69 object, and 364 room operands. All references resolve
to active local prototypes/rooms; there are no reset imports. ZON directly
uses 81 of 83 local mobiles and 50 of 60 objects. The ten objects without a
ZON placement are #27007, #27040-#27045, #27056, #27059, and #27060; the two
unused mobiles are #27031 and #27042. Their other story and source roles are
recorded in the [active source-comprehensive dossier](../zone-stories/THE_HALFCUT_HILLS.md).

The 12 active QST giver headers all resolve to local mobiles. Its 30 G/R I
item commands contain 29 locally defined objects and one unresolved item:
`R I 25000` at [halfcut.qst:236](../../../areas/qst/halfcut.qst#L236). No active
or unregistered OBJ file defines #25000. The source dossier identifies this
as an absent item reward, not a coin or experience command; preserve that
finding without guessing a replacement. The local story sidecar has 69 typed
references (49 mobile, 20 object), all resolving locally. The active shop
record is #27072, with local keeper #27072, room #27130, and six locally
defined products #27006, #27010, #27011, #27046, #27047, and #27048
([shop source](../../../areas/shp/halfcut.shp#L1)).

No external active or other source-only ZON reset operand, QST item/giver,
SHP keeper/room/product, typed sidecar reference, WLD key field, or type-25 /
type-29 destination targets an active Halfcut prototype or room. No same-type
foreign MOB/OBJ definition duplicates an active prototype outside the known
candidate `halfcut_hills`. Candidate `halfcut_hills` itself remains a distinct
source-only component: its reset operands resolve within its own definitions,
it has no Q/QA contract, SHP, story sidecar, ITEM_TELEPORT, or ITEM_SWITCH,
and its sole WLD door key field is -1 (no positive key VNUM). The candidate's
numeric room/mobile/object overlap cannot be treated as an external active
consumer. CP488's comparison records the 235 shared room IDs, 45 shared
mobile IDs, 22 shared object IDs, differing records, and shared QST #27002.

The active WLD has two key fields, both using local object #27033
([first](../../../areas/wld/halfcut.wld#L9656),
[second](../../../areas/wld/halfcut.wld#L9862)); no external key field targets
a Halfcut object. Active items #27024, #27025, #27029, and #27035 are
type-25 portals to local rooms #27242, #27380, #27429, and #27453. Type-29
objects #27020 and #27031 target local rooms #27046 and #27431. These item
routes are not WLD exits; no external type-25 or type-29 field enters the
active zone. The candidate's separate #256031 WLD edge remains unresolved
against its inactive source tree as recorded in the candidate graph section.

Maintained C/H numeric matches that bind to active Halfcut roles are:

| Role | Source |
| --- | --- |
| Active crossbow-ambusher mobile #27009 | [special assignment](../../../src/specs/specs.assign.c#L300). Its routine targets active rooms #27139/#27137/#27136 and #27146/#27144/#27131 ([routine](../../../src/specs/specs.halfcut.c#L74), [second routine](../../../src/specs/specs.halfcut.c#L107)). |
| Object #27030 in chaos equipment data | [first item table](../../../src/account/chaos_eq_data.h#L92) and [second table](../../../src/account/chaos_eq_data.h#L344). |
| Epic-teacher mobile #27035 | Active prototype [#27035](../../../areas/mob/halfcut.mob#L480); [`epic_teachers` row](../../../src/classes/epic_skills.c#L179) binds this mobile VNUM to `SKILL_IMPROVED_LISTEN`; the [`epic_teacher_skill.vnum` field](../../../src/classes/epic_skills.h#L17), [lookup](../../../src/classes/epic_skills.c#L395), and [teacher match](../../../src/classes/epic_skills.c#L469) confirm its role. |

The #27035 compiled reference is a mobile VNUM even though the same number
also identifies a local room and object; typed source and the C++ struct
field distinguish those roles. Active connectivity remains 1,388/1,388
resolved exits with one inbound and one reciprocal Surface outbound.
Candidate inbound number #27001 still resolves to the active room; it does not
grant arrival to the inactive candidate. Keep active `halfcut` **DO NOT MOVE**
and candidate `halfcut_hills` **RETAIN IN PLACE**. Runtime, editor/deployment,
and persisted-ID ownership remain outside this static closure.

## Checkpoint 900: source-only reciprocal edge and typed dependency closure

Reviewed October 13, 2026. This continuation reconciles the inactive surf
and surface2011 target records with the candidate boundary above. It does not
change either source map or active gameplay.

Candidate halfcut_hills room #27001 exits north (D0) to #256031. The
unregistered surf.wld room #256031, "A Gradual Incline Covered with Grass and
Shrubs," exits south (D2) to #27001 at
[the destination field](../../../areas/wld/surf.wld#L1175762). These two
source records form a numerically reciprocal pair across two unregistered
files. The competing unregistered surface2011.wld definition of #256031 is
"The Azure Waters of a Stormy Sea"; its D2 exit targets #256331 instead. The
VNUM therefore has conflicting map identities, and the matching surf exit
does not establish that either file is intended to load with the candidate.

The surf direction already qualifies under the Checkpoint 448 source-only
recount rule: active halfcut is the sole listed WLD owner of #27001, and its
map has no #256031 room. It is already included in the 11,987 eligible
source-only directions overall / 415 from surf.wld; do not add it a second
time. The surf source itself is unlisted, so its numeric target resolves to
active halfcut #27001 if interpreted against active WLD ownership. It is not
a second active ingress to halfcut_hills. The active Surface route from
#629274 remains the sole active cross-area inbound to shared #27001, and the
candidate has no candidate-specific active arrival.

Role-aware closure found no external same-type WLD/MOB/OBJ definition beyond
the known active halfcut duplicate set; no external ZON operand, type-25 or
type-29 target, QST numeric role, parser-shaped N-format SHP role, or
unregistered story sidecar matches the candidate IDs after the active source
is separated. halfcut.story.json is the active source-owned sidecar already
covered at Checkpoint 815. The six exact C/C++ matches remain active Halfcut
roles documented there, not candidate bindings. Candidate ZON resets remain
local; its key field is -1 and it has no SHP, story sidecar, type-25 portal, or
type-29 switch.

Candidate graph totals remain 235 rooms / 550 directions: 549 local targets
resolve by number through active halfcut, while #256031 remains unresolved
in the active loader's source set; the candidate-only graph remains one weak
component with 226/235 directed-reachable from #27001. These figures exclude
the unregistered surf edge from the candidate graph. Keep active halfcut
**DO NOT MOVE** and halfcut_hills **RETAIN IN PLACE** until map, source,
editor/deployment, and persisted-ID owners resolve the conflicting identities.
