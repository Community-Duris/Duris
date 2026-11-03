# The Tower of the Adamantine Mage inactive-source audit

Canonical key: component:Magetower. Physical coverage row 413 / CSV line 414;
candidate dependency row 62 / CSV line 63. Reviewed October 11, 2026,
Checkpoint 500, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source,
connectivity, numeric-identity, reset, code-reference, editor-map, and
bounded-history review. It does not establish live loading, gameplay,
editor-project ownership beyond tracked files, deployed map input, or
permission to activate, rename, move, or archive any source.

## Source inventory and active low-VNUM ownership

The unlisted component has [WLD](../../../areas/wld/Magetower.wld#L1),
[MOB](../../../areas/mob/Magetower.mob#L1),
[OBJ](../../../areas/obj/Magetower.obj#L1), and
[ZON](../../../areas/zon/Magetower.zon#L1) compiler sources. It also has a
tracked [editor coordinate file](../../../areas/map/Magetower.json#L1) whose
zoneId is Magetower and whose 71 positions cover all 71 candidate room IDs;
its lastModified field is November 30, 2025. It has no same-stem QST, SHP,
or story sidecar. Neither [areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists Magetower.

The ZON header is #1000, titled “The Tower of the Adamantine Mage,” with top
#71 and reset mode 2. The sources define 71 rooms (#1–#71), 23 mobiles
(#1–#23), and 20 objects (#1–#20). The normalized title is unique among
scanned ZON titles, but header #1000 is also used by cbl_outpost2,
monsteri, and random. Every room VNUM is outside the numeric #1000 room band;
the tracker records 71 out-of-range rooms and 152 exits from those rooms.
This is an identity and range-mismatch finding, not a runtime compiler test.

The active room table already has different room identities at 43 of the
candidate VNUMs. Limbo owns room #1; active Heavens owns #3–#5, #10–#46,
#66, and #69. For example, Magetower #1 is a carved stone tunnel while
active Limbo #1 is the formless Void; Magetower #44 is a tunnel through the
Valley of Tears while active Heavens #44 is deep within Io's Brain. See the
[Limbo review](../../../areas/wld/limbo.wld#L8) and
[Heavens review](../../../areas/wld/heavens.wld#L539).

Typed prototypes have broad active collisions too. Candidate mobiles #1–#6
overlap Limbo; #7–#17 and #19–#23 overlap Heavens; #18 has no same-number
active mobile. All 20 candidate objects overlap active prototypes: #1–#9 in
Limbo and #10–#20 in Heavens. These are competing same-number definitions,
not evidence that either active owner is Magetower. The separate
[areas_mini fixture](../../../areas_mini/mini.wld#L1) also shares 27 candidate
room IDs, 17 mobile IDs, and 20 object IDs; it is distinct from the normal
areas/AREA input.

## Candidate graph and active-table resolution

Magetower's 71 rooms form one weakly connected component with 152 authored
direction records, all targeting room VNUMs defined in this candidate WLD.
Directed traversal from #1 reaches all 71 rooms. There are no negative
targets, fall chances, or current fields; there are no candidate WLD exits
outside its #1–#71 numeric set.

The current active room table resolves 93 of those 152 target records by
number to Limbo or Heavens rooms with different identities. The other 59
target numbers have no current active room record. Active WLD sources
separately contain 70 direction records targeting numbers in the Magetower
band: 60 from Heavens, nine from Desert to current Limbo #1, and one from
Thetis to current Heavens #38. These are routes to the listed Limbo/Heavens
identities, not active ingress to the unlisted Magetower map. Candidate
inbound remains zero.

Two Magetower source edges, room #2 north and room #64 south, target candidate
room #1 locally. In the current active table the same number belongs to
Limbo, so they are source-only numeric references to Limbo #1, not live
Magetower entrances or a replacement boundary. The existing
[Limbo review](LIMBO_STORY_REVIEW.md#L63) records these sources separately.
The candidate graph has zero cross-zone exits in or out, zero reverse
cross-zone exits, and zero unregistered target VNUMs. Keep all 152 WLD edges
separate from the four ZON D door-state records.

## Reset commands, compiled code, and editor map

The candidate ZON has four D door-state commands and five M mobile resets,
with no O, E, P, or G commands. Three M commands reset candidate mobile #1
as a shaped-adamantium golem; the other two reset #20 Hezipa and #15 an
emissary of Vecna. These reset IDs collide with the current active table's
Limbo #1 mobile and Heavens #20 eagle / #15 witch doctor. The D comments mark
the north/south passage between #1 and #64 closed and secret and the passage
between #7 and #8 as open/secret. No live door state or mob availability was
tested.

Compiled code also uses low VNUMs as keys for current Limbo and Heavens
identities. `necro_dracolich` is assigned to mobiles #3–#10; Limbo currently
owns #3–#6 and Heavens #7–#10. Other assignments include Heavens #11–#13
and #15. Magetower defines different mobile content at those same IDs,
including its emissary at #15. Code assigns object #4 to blood-stains
behavior, #14 and #15 to banana behavior, #17 in multiple source assignments,
and #18–#19 to a special weapon function; current Limbo/Heavens objects own
those VNUMs. Room-number code also uses Heavens #17, #42, and #44 for
multiclass, cage, and Heavens-specific behavior. See the
[compiled assignment sites](../../../src/specs/specs.assign.c#L383),
[room assignments](../../../src/specs/specs.assign.c#L2269),
[Heavens room code](../../../src/specs/specs.heavens.c#L4950), and the
[Limbo code/dependency review](LIMBO_STORY_REVIEW.md#typed-prototype-and-code-references).
These are static numeric bindings to the current active table; no behavior
was executed.

The coordinate JSON places every candidate room and was added in commit
b57048496 (“pfile migration tool”). This is a tracked editor artifact; it
does not place Magetower in either area list or prove which editor project or
deployed copy is authoritative. Preserve it with the four compiler sources
until those owners reconcile the source and coordinate copies.

## Story, bounded history, and disposition

The map describes an adamantine tower, its golems and wardens, halls of
conjuration and illumination, a library, and a secret briar route. Its
prototypes include named residents, magical equipment, and relics, so the
source is narrative-bearing even without a QST or separate story sidecar.
The candidate has no shop source and its ZON does not reset its OBJ
prototypes; no reward contract is established by these files.

Followed histories show older tracked WLD/MOB/OBJ/ZON sources and generic
format, flag, or permission edits, without a direct rename, retirement, or
whole-area replacement decision. The coordinate JSON is a later tracked
addition from 2025. Those histories do not resolve the active Limbo/Heavens
collisions, zone-header reuse, low VNUM range, code bindings, or editor
ownership.

**Disposition: RETAIN IN PLACE.** Keep the four compiler files out of both
area lists and preserve areas/map/Magetower.json alongside them. Preserve
current Limbo and Heavens room/mobile/object ownership, reset consumers, and
compiled code bindings. Resolve the #1000 header reuse, all 43 room overlaps,
mobile/object ownership, 70 active numeric route references, candidate
#1-to-Limbo ambiguity, door and mob resets, mini-fixture overlaps, and
editor/export/deployment/persisted state before any activation, renumbering,
path move, merge, or archive proposal. See the
[25 builder questions](MAGETOWER_BUILDER_QUESTIONS.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-500-magetower-low-vnum-source-identities-and-active-limbo-and-heavens-ownership),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-500-magetower-retained-pending-low-vnum-map-code-and-editor-ownership),
[candidate dependency row 62](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L63), and
[coverage row 413](ZONE_COVERAGE_REGISTER.csv#L414).

## Checkpoint 911: role-aware low-VNUM source closure

The CP500 candidate graph and **RETAIN IN PLACE** disposition are unchanged.
The exact active WLD scan reproduces 70 direction records whose destinations
fall in #1–#71: 60 are Heavens-to-Heavens targets, nine are Desert exits from
#49790 to active Limbo #1, and one is a Thetis target to active Heavens #38.
None targets a Magetower map identity. A wider scan found 869 unlisted WLD
direction records into the same numeric band. Of these, 868 target rooms
defined in their own source WLDs: Dalvik (262), Hostel (176), Mavulsk (153),
Mentiri (159), Nexus_1 (92), Theurgist (6), and Waterplane (20). The remaining
record is unlisted Surf #201670 to #36, which resolves to active Heavens.
[Surf target](../../../areas/wld/surf.wld#L35061) is not an active route or
Magetower ingress.

Same-number room definitions extend beyond active Limbo and Heavens. Active
room ownership remains Limbo #1 and Heavens #3–#5, #10–#46, #66, and #69.
Separate unlisted WLDs define all 71 candidate room numbers in Dalvik, Hostel,
Mavulsk, and Mentiri; Nexus_1 defines #1–#40, Theurgist #1–#4, and Waterplane
#1–#10 and #12. Astral, Mtncastl, and ud-inside each also define #1. These
source-file overlaps do not establish a shared area identity. The separate
areas_mini overlap remains 27 rooms, 17 mobiles, and 20 objects.

The prototype scan confirms active same-type mobile owners Limbo #1–#6 and
Heavens #7–#17 and #19–#23; active mobile #18 is absent. Unlisted Dalvik
defines mobiles #1–#3; Hostel, Mavulsk, and Mentiri each define #1–#23;
Theurgist defines #1–#14; and Waterplane defines #1–#3. Active object owners
remain Limbo #1–#9 and Heavens #10–#20. Unlisted Dalvik, Hostel, Mavulsk, and
Mentiri each define objects #1–#20, while Theurgist defines #1–#3. These
competing source records remain distinct pending owner review.

An exact typed ZON sweep found 94 active reset fields with values in the
candidate room/mobile/object sets: 74 match a prototype or room defined by
the same active source, and 20 resolve to current Limbo or Heavens prototypes.
Another 445 matching fields in unlisted ZONs are source-local to Dalvik,
Hostel, Mavulsk, Mentiri, Nexus_1, Theurgist, or Waterplane. No scanned reset
field names a Magetower-only identity. The candidate's four D door-state and
five M mobile-reset commands remain unchanged; none loads its objects.

Active QST matches are item fields in Alatorin (#5, #18, #19), SurfaceKeeps
(#8), and Tharnadia (#5), plus Heavens giver #17; the items and giver resolve
to active Limbo or Heavens prototypes. Unlisted Dalvik, Hostel, and Mavulsk
QSTs use their own same-number mobile and object definitions. Three active
story sidecars, Alatorin, SurfaceKeeps, and Tharnadia, likewise reference
active Limbo/Heavens item IDs #5, #8, #18, and #19. No Magetower QST or story
sidecar exists. See [Alatorin QST items](../../../areas/qst/alatorin.qst#L212),
[Heavens giver](../../../areas/qst/heavens.qst#L1), and the existing
[Limbo typed-reference review](LIMBO_STORY_REVIEW.md#typed-prototype-and-code-references).

The parser-valid N-format shop scan found active Alatorin, Harrow, and Player
Castles product references to Limbo object #5, and Heavens shop #101 located in
Heavens room #23. No external N-format shop keeper or product selects a
Magetower-only prototype, and Magetower has no SHP source. No external type-25
teleport or type-29 switch destination targets rooms #1–#71; all 20 candidate
objects are type 13, so the candidate itself has no portal or switch object.
No portal-derived route is added.

A direct compiled-source scan found 45 literal `real_room`, `real_mobile`, or
`real_object` lookup occurrences in the candidate's numeric sets. Their values
resolve through the current active table to Limbo or Heavens identities. The
existing bindings include Limbo room #1 and mobiles #3–#6, Heavens rooms
#11/#17/#40/#42/#44 and mobiles #7–#17/#19–#23, Limbo objects #2–#4, and
Heavens objects #14/#15/#17–#19. No source-path or Magetower-name binding was
found. These are static references; no code behavior or runtime loading was
tested.

Keep all four compiler files and the 71-position coordinate JSON together at
their current unlisted paths. Preserve the CP500 room graph, range mismatch,
active Limbo/Heavens code ownership, the additional low-VNUM source identities,
and the exact QST/reset/shop roles above until map, editor, export, deployment,
and persisted-state owners resolve them. The existing 25 builder questions and
all owner approvals remain open.
