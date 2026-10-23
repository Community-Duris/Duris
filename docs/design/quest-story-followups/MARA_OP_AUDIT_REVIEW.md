# Mara Outpost source and shared-identity audit

Canonical key: component:mara_op. Physical coverage row 419 / CSV line 420;
candidate dependency row 68 / CSV line 69. Reviewed October 11, 2026,
Checkpoint 506, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static inventory,
WLD-graph, typed-reference, identity, and bounded-history review. It does
not establish live area loading or portal use.

## Inventory and story content

The unlisted component contains `mara_op.wld`, `.mob`, `.obj`, and `.zon`;
there is no same-stem SHP, QST, or story sidecar. Neither
[areas/AREA](../../../areas/AREA) nor [areas/qst/AREA](../../../areas/qst/AREA)
lists `mara_op`, and `areas/RANDOM_AREA` is empty. The zone header is #1,
the `.zon` title is `The Khomani Kan Outpost of The Marauders`, the reset
mode is 2, and its room range is #29610–#29612. It defines three rooms, two
mobile prototypes, and two objects, with no shop or native Q/Q-A contract.

The three rooms depict a wind-cut ravine and a hidden staircase into
Tikitzopl, with a Blazing Vortex below. Descriptions feature golden skulls,
flaming eyes, silver torches, Rednax-Ela, pillars of gore, and visions and
artwork along the stairway. A plaque contains song text, but there is no local
QST or story sidecar that defines a quest giver, progression, or reward.

## WLD graph and Duris3 boundary

The source WLD has five authored directions. Four local directions form two
reciprocal pairs among all three rooms, giving one weak component of size
three; traversal from #29610 reaches all three rooms. The remaining direction
is the up exit from #29610 to Duris3 #124408 (`Grassy Plains`). That target is
defined in the unlisted `Duris3.wld`; no WLD return from Duris3 to a
Mara Outpost room was found. The same #29610 up exit is separately authored in
`mara_gh.wld`, whose source room VNUMs collide with this component.

No active WLD exit targets this component. Against the current full-area
room table, zero of the five target records resolve and all five are positive
unresolved: four local destinations belong to this unlisted map and the
boundary target belongs to unlisted Duris3. There are no negative targets,
fall-chance records, or current records. Keep the one Duris3 boundary out of
active-route and loaded-area approach totals.

## Resets and portal references

The unlisted [mara_op.zon](../../../areas/zon/mara_op.zon#L9) contains one
object placement, three pillar-of-gore M resets, and one Rednax-Ela M reset.
It places portal object #29610 in room #29612 and mobiles #29610/#29611 in
room #29610. These are source-only because the component is absent from both
area lists. No external active ZON reset, QST giver, SHP keeper, or
component-specific compiled assignment targets these prototypes.

Both objects have type ITEM_TELEPORT, and the static handler uses `value[0]`
as the destination room VNUM
([handler](../../../src/magic/spell_travel.c#L991)). Portal #29610 describes
a route to Good but has destination value #29608; no #29608 room exists in
`mara_op.wld`, while that room is defined in `mara_gh.wld`. This is a
source-only cross-component reference if both inputs are intended together;
without the Guildhall room in the loader table the destination is unresolved.
Portal #29611 names Khomani-Kan and targets local room #29612, the Blazing
Vortex. Confirm both `value[0]` fields with the item and map owners rather
than treating the prose as proof of destination.

## Shared VNUMs, history, and disposition

Mara Outpost and Mara Guildhall separately define the same room VNUMs
#29610–#29612, mobile VNUMs #29610/#29611, and object VNUMs #29610/#29611.
Their room titles, descriptions, and exits match, while room-flag values
differ. Mobile prose is shared but combat/stat fields differ. Portal
keywords and fields differ, although the same destination numbers are used.
Both unlisted zone files reset the shared VNUMs. If admitted together these
sources present same-type duplicate definitions; the current area lists load
neither, so this is a source identity conflict, not a proven active collision
or proof of replacement. Mara Outpost's #29610 portal also targets Mara
Guildhall room #29608, which gives the two components a separate typed link.
Preserve each path and numeric identity while both owners confirm whether
the packages are shared inputs, alternate exports, or legacy copies.

Followed Git history reaches the 2009 area-tree import and a later UTF-8
recode. It establishes no direct rename, retirement, or whole-area
replacement. The similar records and one cross-component portal value do not
identify either component as the other's successor.

**Disposition: RETAIN IN PLACE.** Preserve all four source files outside the
area lists. Resolve the shared rooms, mobiles, objects, the #29608 portal
destination, the Duris3 exit, and any editor/deployment copies with the Mara
Guildhall, Mara Outpost, Duris3, and item owners before changing a VNUM, list,
reset, destination, or path. See the
[20 builder questions](MARA_OP_BUILDER_QUESTIONS.md),
[Mara Guildhall audit](MARA_GH_AUDIT_REVIEW.md#shared-vnums-history-and-disposition),
[Duris3 supplement](DURIS3_AUDIT_REVIEW.md#checkpoint-505-supplement-source-only-mara-guildhall-boundaries),
[coverage row 419](ZONE_COVERAGE_REGISTER.csv#L420), and
[candidate dependency row 68](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L69).

## Checkpoint 917: Mara Outpost exact VNUM-role closure

The exact #29610–#29612 scan across WLD, MOB, OBJ, ZON, SHP, QST, story,
and compiled source confirms the CP506 ownership boundary. WLD, MOB, OBJ,
and ZON matches remain in the two unlisted Mara families; no third source
adds a room, mobile, object, reset, or inbound WLD edge. The only matching
SHP data is Mara Guildhall's #29600 shop, which does not key a keeper or room
to the Mara Outpost mobile/room IDs. QST and story scans contain no matching
reference, and compiled source has no Mara Outpost VNUM binding beyond the
separate Mara Guildhall room #29605 inn assignment.

Mara Outpost remains a three-room, five-direction graph with four local
directions and one up exit from #29610 to unlisted Duris3 #124408. The shared
Mara Guildhall exit is a separate source record; neither is active ingress.
Static ITEM_TELEPORT fields remain #29610 to Mara Guildhall #29608 and #29611
to local #29612. The former is a cross-component object value, not a WLD
direction. Preserve the four source files, distinct file identities, and
**RETAIN IN PLACE** disposition. All 20 MOP questions remain; no runtime
loading or portal travel was tested.
