# Alterian Wilderness

Canonical key: `component:alterian_wilderness`. Physical coverage row 359.
This is a source-only Alterian map, not a zone listed in the active area
manifest. The [technical audit](../quest-story-followups/ALTERIAN_WILDERNESS_AUDIT_REVIEW.md)
records its room graph, conflicting VNUM identities, and typed dependencies.
The [builder questions](ALTERIAN_WILDERNESS_BUILDER_QUESTIONS.md) capture
decisions needed before any source or loader change.

## The authored journey

The map begins underground, where wet footing and rising steam make a narrow
cavern route feel unstable. Small humid rooms lead through iron grates toward
a smoke-filled cavern and boiling pool. The prose places a keyhole and a heavy
iron key in the same story, but the zone reset data marks both grate exits
open. That combination suggests a puzzle or gated route whose present source
state needs confirmation.

Beyond the steam caves, rooms #4759–#4760 describe a dark interior lair with
crude furniture and carvings. Two magical portal objects store the other
room's VNUM, and source code assigns them the `magic_pool` callback. The code
provides a travel handler, while the Wilderness files' unlisted status leaves
runtime availability unresolved. The lair therefore reads as a distinct
destination connected at the object layer, even though the WLD-only graph
places it in its own two-room component.

Above ground, a gravel road passes through forest and open prairie toward a
working farmstead. The rooms describe a yard, well, cornfields, a shed, a
porch, and several family rooms. Birds, small mammals, predators, livestock,
and rural encounters make the place feel inhabited. Farther along, the road
and foothills become a mountain trail, while bogs, meadows, and ponds provide
smaller landscape changes. The local source set contains no quest giver,
shop, or story sidecar; its story comes from place descriptions, wildlife,
resets, and the two portal objects.

## Separate map identities

The source contains 130 rooms and 366 exits, but its local WLD graph has three
weak components. The nine-room steam-cave cluster (#4750–#4758) and the
two-room lair (#4759–#4760) are separated in WLD data; the forest and farm
network forms the 119-room component (#4801–#4919). The room numbers #4801–
#4809 also exist in active Tchan with different records. Numbers #4900–#4919
also exist in active Mining, again with different records. Keep these records
as distinct map identities.

Several authored approaches point outward: five exits reach Mining-numbered
rooms, two lead to Shadow Forest #6368, and four distinct-origin routes reach
Tchan-numbered rooms. Sixteen other Wilderness exits match Tchan room numbers
from the overlapping #4801–#4809 source range. A trail at #4919 points to
Mountain #4920; the Mountain link and a Surf link at #4915 are reciprocal in
their source files. Those paths remain source-only evidence. They do not show
that Wilderness is active, that an overlapping room is shared, or that a
traveler can return safely.

## Keep its relationships visible

Wilderness zone number 47 and its title “Alterian Region - Wilderness” are
unique in current ZON sources. Its own resets use Mountain mobile prototypes;
Mountain resets use Wilderness mobiles and object #4806. Active `bs.zon` also
has two object #4814 reset commands, while Wilderness defines #4814 as an ear
of corn. Code assigns `poison` to mobile #4812 and `magic_pool` to objects
#4801/#4802. Mobile and object #4800 also collide with different active
Tchan prototypes, including a compiled guard callback on the active #4800
identity. Preserve each source relationship without assuming that a same
number means the same object, mobile, or room at runtime.

Keep `alterian_wilderness` **RETAIN IN PLACE** until the loader owner resolves
area membership and the overlapping map/prototype identities. The [audit's
connectivity and dependency findings](../quest-story-followups/ALTERIAN_WILDERNESS_AUDIT_REVIEW.md#world-connectivity-and-map-identity),
[candidate row 8](../quest-story-followups/ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8),
and [coverage row 359](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L359)
preserve these source boundaries.
