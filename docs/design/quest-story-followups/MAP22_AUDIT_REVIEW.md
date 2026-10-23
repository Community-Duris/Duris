# map22 labyrinth grid and random-lab dependency audit

Canonical key: component:map22. Physical coverage row 415 / CSV line 416;
candidate dependency row 64 / CSV line 65. Reviewed October 11, 2026,
Checkpoint 502, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static WLD, list,
numeric-reference, code-binding, and bounded-history review. It does not
establish current loading, generator execution, editor ownership, or live
travel.

## Source and template inventory

The only tracked same-stem source is [areas/wld/map22.wld](../../../areas/wld/map22.wld).
It has no same-stem MOB, OBJ, ZON, QST, SHP, story sidecar, or map-editor
file. Neither [areas/AREA](../../../areas/AREA#L1) nor
[areas/qst/AREA](../../../areas/qst/AREA#L1) lists `map22`; the separate
[areas/RANDOM_AREA](../../../areas/RANDOM_AREA) file is zero bytes in this
checkout.

The WLD defines every room from #900000 through #909999 exactly once. All
10,000 room names are “A labyrinth in the grave,” all room descriptions are
empty, and all room header fields are identical (`9000 134217728 24 0`). The
file has no zone header or reset commands because no ZON source exists for
this stem. The repeated labels and uniform fields are evidence of a template
or generated layout, not a distinct authored story for each room.

## Source graph and current table resolution

The 10,000 rooms form a complete 100-by-100 orthogonal grid: D0, D1, D2, and
D3 each occur 9,900 times, for 39,600 direction records. Every target is a
local room VNUM, every record has a reciprocal record, and directed traversal
from #900000 reaches all 10,000 rooms in one weak component. No negative or
external target, fall chance, or current field was found.

No other WLD defines one of these 10,000 room VNUMs or has a direction record
targeting this range. Consequently, none of the 39,600 candidate target
records resolves in the current active room table. The source-only inbound
WLD count is zero. Keep the active-table metrics distinct from the dynamic
code path below: that path does not add a static WLD edge or an active
candidate-inbound record.

## Random-lab code and Duris3 entry

Compiled code gives this otherwise unlisted grid a direct, conditional
consumer. [`reset_lab(1)`](../../../src/world/random.zone.c#L1355) selects
start VNUM #900000, uses Duris3 room #145443 as its entrance, and checks each
of 10,000 possible room lookups before indexing the room table.
[`create_lab(1)`](../../../src/world/random.zone.c#L1449) selects start VNUM
#905050, the same entrance #145443, theme 5, and relic object #68. It aborts
if either the start room or entrance is absent; after carving its random
maze, it dynamically adds a down exit from the entrance to #905050. The
candidate start room is present in the WLD at
[#905050](../../../areas/wld/map22.wld#L105247); the dynamic link is created
at [the generator's connection step](../../../src/world/random.zone.c#L1605)
and removed by [reset_lab](../../../src/world/random.zone.c#L1443). The
trusted [`randobj remove`](../../../src/item/randobj.c#L165) and
[`randobj map`](../../../src/item/randobj.c#L175) paths invoke the
create/reset routines, and relic handling also calls `reset_lab(1)` for
VNUM #68. None of these paths was executed.

Room #145443 is “The Ocean” in the separate
[Duris3 WLD](../../../areas/wld/Duris3.wld#L954304), which is also absent from
`areas/AREA`. `map22` and `Duris3` are therefore separate unlisted WLD
identities connected by a code-defined entrance only if both required rooms
are available and the creation path runs. The code-created link is not an
authored WLD edge or evidence of current travel. See the
[Duris3 audit](DURIS3_AUDIT_REVIEW.md).

The type-1 code selects object VNUM #68. The current active Heavens object
#68 is the Relic of the Spider Queen ([prototype](../../../areas/obj/heavens.obj#L952));
unlisted `hostel.obj` and `mavulsk.obj` also define object #68. `map22` has
no object source of its own. Preserve that separate object-identity question
with the generator owner; it does not change the room-graph counts.

## History and disposition

Followed history reaches the 2009 area-tree import commit `642da5400` and
shows no earlier tracked same-stem rename, retirement, or replacement
decision. The checked-in source itself does not identify its owner, editor,
generator inputs, or deployment copy.

**Disposition: RETAIN IN PLACE.** Preserve the 10,000-room WLD with the
random-lab code that references #900000–#909999 and #905050, and preserve
Duris3 room #145443 as a distinct code-selected entrance. Keep all sources
out of the current area lists until the map, random-zone, Duris3, object, and
deployment owners reconcile the intended input set and identities. Do not
interpret the dynamic code binding as a reason to activate or merge either
WLD. See the [20 builder questions](MAP22_BUILDER_QUESTIONS.md),
[identity/history review](DUPLICATE_AND_REPLACED_ZONES.md),
[retention plan](ATTIC_MIGRATION_PLAN.md),
[coverage row 415](ZONE_COVERAGE_REGISTER.csv#L416), and
[candidate dependency row 64](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L65).

## Checkpoint 913: role-aware random-lab dependency closure

The CP502 100-by-100 WLD graph remains unchanged. Exact scans found no
external WLD room definition or direction target in #900000–#909999, no ZON
reset room operand, no N-format shop room, no QST giver/item value, no story
`room_vnum`, no OBJ type-25/type-29 room destination, and no literal compiled
`real_room`/`real_room0` call in the candidate range. The candidate has no
MOB, OBJ, ZON, QST, SHP, or story sidecar of its own.

A raw numeric scan did find four #900000 tokens in object-data rows: Avernus
object #32020 (type 10), Dragonnia #6829 (type 20), Tradeskills #400283
(type 8), and Voluntown #142429 (type 8). These are ordinary object data
fields, not type-25/type-29 room destinations. They do not add a WLD target
or map22 identity.

The type-1 random-lab code remains the only direct map dependency. Its
`reset_lab(1)` start is #900000 and entrance is Duris3 #145443; `create_lab(1)`
uses start #905050, the same entrance, and relic object #68. The created down
link is still conditional code behavior and was not run. Object #68 is defined
by active Heavens and also by unlisted Hostel and Mavulsk. Hostel has three G
resets and Mavulsk one G reset for their own object #68 records; no QST/story
item or parser-valid N-format shop-stock reference to #68 was found. The
current active object identity remains Heavens' Relic of the Spider Queen,
with the competing unlisted definitions unresolved.

Keep the WLD, `reset_lab`/`create_lab`, Duris3 #145443, and relic #68 as
separate source roles. Preserve **RETAIN IN PLACE**, all 10,000 room VNUMs,
zero active WLD ingress, and the existing 20 builder questions. No static
route, loading conclusion, generation result, or object-owner decision follows
from these scans.
