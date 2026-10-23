# Sylvandawn audit review

Canonical key: `component:sylvandawn`. Coverage data row 451 / CSV line 452;
candidate dependency data row 100 / CSV line 101. Reviewed October 11, 2026,
Checkpoint 538, against checkout `c500ac51b8cd26540943257e7ca4e9807263ef74`.
This is a static source, identity, graph, dependency, code-binding, and history
review. It does not establish which shop-loading path ran, whether the numeric
bindings work at runtime, or what players can currently reach.

## Source inventory and narrative identity

The current unregistered component has five tracked source files:
[WLD](../../../areas/wld/sylvandawn.wld#L1),
[MOB](../../../areas/mob/sylvandawn.mob#L1),
[OBJ](../../../areas/obj/sylvandawn.obj#L1),
[ZON](../../../areas/zon/sylvandawn.zon#L1), and
[SHP](../../../areas/shp/sylvandawn.shp#L1). The `.zon` header is #80, title
`Sylvandawn`, last VNUM #8327, reset mode 2. Current `areas/AREA` does not list
the stem; `areas/qst/AREA` does not list it either. There is no local QST file
or story sidecar, and no membership in the current random-area list.

The source defines 327 rooms (#8001–#8327), 91 mobile prototypes (#8000–#8336),
one object prototype (#8000), 305 `M`, 142 `G`, five `O`, and 58 `D` zone
commands. The 58 door commands and the room arguments of its `M` and `O`
commands target local Sylvandawn room records. The lone object is the holy
Avenger sword; it is not referenced by a current active QST item command, SHP
product, or same-number active object.

Room names and mobile descriptions portray an elven settlement with a city
center, inn and shops, guilds, palace, magic tower and library, druid grove,
prison, port, and Grovetree theatre. That material supports a coherent place
identity, but the source contains no native quest contract. Active
[Mazzolin QST](../../../areas/qst/mazzolin.qst#L5) and
[Icecrag QST](../../../areas/qst/icecrag.qst#L108) mention Sylvandawn in
prose; these textual references do not establish a numeric route, local
quest, or current loaded owner.

## WLD graph and Temple of Laduguer identity collision

The map has 693 authored directions: 691 target records fall within this
WLD's own room VNUM set, and two target records point outside it. Its local
numeric graph has three weak components of 325, 1, and 1 rooms. A directed
walk from #8001 visits 320 of 327 rooms. No fall-chance or current directives
were found. These static counts do not qualify doors, reset-dependent access,
safe travel, or return routes.

Two external directions point to rooms in the unregistered [Duris3 WLD](../../../areas/wld/Duris3.wld):
#8087 west to #163980 and #8325 east to #164082. Both targets are outside the
current active WLD table; preserve the source references without calling them
active routes.

Room VNUMs #8100–#8202 also belong to active `temple_laduguer.wld`. A full
record comparison finds all 103 Sylvandawn/Temple room pairs different, with
no exact full-room match; all 103 titles differ. The Temple audit already
separated 223 raw Sylvandawn directions targeting that numeric band into 17
approaches from Sylvandawn-only source-room IDs and 206 transitions whose
source IDs also have active Temple owners. Those 206 are ambiguous same-number
records, not 206 established routes. The 17 source-only approaches are not
active ingress and are already documented in the
[Temple identity/connectivity review](TEMPLE_LADUGUER_AUDIT_REVIEW.md#inactive-sylvandawn-approaches-and-room-identity).

Current active WLD data has 215 direction records whose targets numerically
fall in Sylvandawn's #8100–#8202 set. They are edges in the active Temple
graph and resolve to Temple's own room records; none identifies a
Sylvandawn-owned destination. Candidate-specific active WLD ingress therefore
remains zero. Keep the candidate's two Duris3 targets separate from its 17
already-reviewed Temple approaches and do not add either group again to the
global inactive-WLD route census.

## Typed resets, shops, and external consumers

Of 305 Sylvandawn `M` resets, 302 reference local mobiles and three reference
other mobile VNUMs. The local resets use 83 distinct candidate mobiles; eight
local prototypes have no local `M` placement: #8036, #8039, #8041, #8042,
#8311, #8312, #8321, and #8322. The external resets are #1253 at ZON lines
452 and 626 and #666 at line 627; both currently resolve to active Heavens
mobiles (#1253 is an elven archer, #666 a seaman). Maintained code also binds
#1253 to `archer`. Keep these source-only reset records distinct from the
active prototype definitions. Five `O` reset rows reference active object
prototypes.

The 142 `G` reset rows contain 104 rows referencing current active OBJ
prototypes and 38 rows across 31 VNUMs with no current `.obj` definition in
the source inventory: #3032, #3051, #3902, #4240, #5214, #5541, #7202,
#8004, #8006–#8009, #8022–#8023, #8331, #8335–#8339, #8342–#8345,
#11030–#11031, #12116, #12531, #12581, #12584, and #18280. This is a
source-file lookup result, not proof that every record is unusable. Preserve
the earlier typed findings: ZON line 517 resets active Lizard warning sign
#6002; lines 480 and 643 reset active Orrak mask #9017; line 636 resets active
Kobold bread #1403, also listed by the SHP at line 593; line 243 resets active
Realm sword #14001; the SHP lists active Dragonnia emerald #6834 at line 432
and Plane of Fire potions #25400/#25401 at lines 120–121; and eight products
refer to active Temple objects. These are source-only references. The
candidate dependency row retains the prior checkpoint citations.

The SHP file has 26 shop records, with keeper VNUMs #8002–#8310. All 26
keepers are local Sylvandawn mobiles. Its 106 product rows resolve to 77
current active OBJ prototypes; 29 rows reference VNUMs without a current
`.obj` definition. Eight products refer to active Temple objects at SHP lines
22, 61, 101, 140, 178, 528, 771, and 812; the Temple audit lists their
VNUMs. The candidate's products are source records, not proof of a loaded
keeper or live stock.

There is a static shop-loader path worth preserving with this candidate:
outside mini mode, [shop.c](../../../src/economy/shop.c#L3267) first tries
`areas/world.shp`, then falls back to globbing `areas/shp/*.shp` and appending
the files without checking `areas/AREA`. `areas/world.shp` is absent in this
checkout. This code makes the candidate SHP file relevant to the fallback
path if that path is invoked; no runtime invocation or usable shop outcome
was tested.

The bounded active-source scan found no active ZON reset, SHP keeper/product,
QST item command, or story-sidecar VNUM reference into Sylvandawn's own MOB
or OBJ prototypes. No active MOB/OBJ source has a same-type VNUM collision
with the 91 candidate mobiles or #8000 object. That result does not erase
the separate room-number collision with active Temple.

Maintained code still carries Sylvandawn-number bindings. [specs.assign.c]
(../../../src/specs/specs.assign.c#L375) assigns cityguard, money-changer,
guild-guard, janitor, and world-quest handlers to 18 candidate mobile VNUMs;
lines 2428–2432 assign room handlers to #8010, #8211, #8323, #8003, and
#8287. `src/core/constant.c` retains the Sylvandawn hometown name and
destination arrays at lines 1183, 1724, 1803, and 1902–1903. The
[guard specification](../../../src/specs/specs.guards.c#L157) also branches
on room #8200 and #8113, numeric IDs whose Sylvandawn and Temple room
identities differ. These source-level bindings are not evidence that the
unregistered records currently load or that a binding resolves to the
intended owner.

## History, disposition, and builder questions

Followed WLD/MOB/OBJ/SHP paths show old source maintenance through 2013,
including the 2009 `areas/` path move; the WLD has a later ANSI correction.
The current active manifest omits Sylvandawn. The inspected history does not
establish a Sylvandawn-specific retirement decision, whole-zone replacement,
or authorized path move. Its 103 distinct same-number rooms do not make active
Temple a proven replacement. Preserve the component RETAIN IN PLACE while an
area owner confirms its map, code, reset, item, and editor/build roles.

The twenty source-backed questions and conservative recommendations are in
[SYLVANDAWN_BUILDER_QUESTIONS.md](SYLVANDAWN_BUILDER_QUESTIONS.md). No
activation, archive, merge, or migration clearance is implied.
