# Ant Forest repository-wide audit review

This supplement completes the source-backed narrative, graph, typed-reference,
title, and bounded history review for active area `ants`. It accompanies the
[Ant Forest source story dossier](../zone-stories/ANT_FOREST.md), which maps
the snow-forest approach, ant colony, two switch-opened passages, lower rift,
and unresolved `Void` reset location. Findings about switch behavior are
static source/data analysis, not a played interaction or proof of installed
reachability. No native gameplay data changed.

## Active source and story inventory

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 109 is
titled `Ant Forest` and contains 41 rooms (#10900–#10940), nine mobiles
(#10900–#10908), and four objects (#10900–#10903). Active WLD, MOB, OBJ, ZON,
and SHP files exist. The SHP file is empty, the key appears in
`areas/qst/AREA` but `areas/qst/ants.qst` is absent, and there are zero native
Q/QA contracts or story sidecar. The reset file places the forest wildlife,
ant colony and caretakers, two switch objects, a keeper, and an ethereal frost
crown. These are environmental scenes and object-driven passage interactions,
not a recorded colony quest outcome.

## Active WLD graph and inactive approaches

The active WLD parser resolves all 122/122 authored exits: 121 local and one
outbound. The weak local graph is one component. From root #10900, directed
traversal over all authored local exits reaches 39/41 rooms; all but #10940
have a directed path back to #10900. Rooms #10939 and #10940 are outside root
forward reach, and #10940 has no return path. Excluding the two internal
exits carrying the blocked flag reduces root reach to 15/41. These counts
describe authored direction and flags; the matching switch objects and zone
reset commands affect passage state, and no live access or safety result is
claimed.

The sole active boundary is reciprocal with Surface: Ant Forest #10900 west
to #526267 and Surface #526267 east back to #10900. See active exit-register
[Ant Forest row 206](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L206), [Surface row
1374](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1374), and reciprocal pair summaries
[rows 15](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L15) and
[461](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L461). Sources: [Ant Forest
entrance](../../../areas/wld/ants.wld#L1) and [Surface
room](../../../areas/wld/surface.wld#L551498).

All 95 WLD files outside the active `areas/AREA` list parsed without errors:
527,791 rooms and 2,080,773 exits, with no room-VNUM overlap against active
Ant Forest. One non-overlapping source-only approach points into Ant Forest:
`surf.wld` room #211552 east (D1) to #10900
([source room](../../../areas/wld/surf.wld#L242424)). This inactive source
record is not current ingress. It raises curated source-only totals from 3,437
to 3,438 overall, from 203 to 204 for reviewed Surf, and leaves `ud_map2` at
15. Preserve it in existing Surf candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Data-driven switches and typed dependencies

Two local `ITEM_SWITCH` objects encode passage puzzles:

| Object and reset placement | Trigger and target | Static native effect | Evidence |
| --- | --- | --- | --- |
| #10901 butchered ant corpse in room #10904 | `CMD_CRY` (53) targets #10904 D5 down to #10908 | The generic handler clears `EX_BLOCKED` on that exit. The corpse's extra text supplies the “cry” clue. | [Reset](../../../areas/zon/ants.zon#L57), [object](../../../areas/obj/ants.obj#L14), [room/exit](../../../areas/wld/ants.wld#L83), [`CMD_CRY`](../../../src/cmd/interp.h#L88) |
| #10902 handprint in room #10916 | `CMD_TOUCH` (320) targets #10915 D0 north to #10917 | The generic handler clears `EX_BLOCKED` on that secret exit; since the object is in another room, its use can signal a distant rumble. | [Reset](../../../areas/zon/ants.zon#L59), [object](../../../areas/obj/ants.obj#L31), [enclave](../../../areas/wld/ants.wld#L310), [target room](../../../areas/wld/ants.wld#L291), [`CMD_TOUCH`](../../../src/cmd/interp.h#L355) |

Both objects have type `ITEM_SWITCH` (#29). The standard `read_object()` path
automatically binds the generic `item_switch` function when no object-specific
function is installed ([type](../../../src/core/defines.h#L144),
[automatic binding](../../../src/world/db.c#L5281),
[handler](../../../src/specs/specs.object.c#L302)). The handler clears only
`EX_BLOCKED`; the secret bit and any zone-authored door state remain separate.
The zone reset explicitly marks #10904 D5 blocked/secret and assigns the
#10915 D0 door state
([reset rows](../../../areas/zon/ants.zon#L24),
[second-tunnel reset](../../../areas/zon/ants.zon#L40)). The passages are
interactions in the native environment, but source inspection does not prove
their live room messages, search requirements, timing, or post-reset state.

A typed scan of 21,682 active and unregistered object prototypes found exactly
these two switches targeting Ant Forest rooms; both are local. No external
`ITEM_SWITCH` target into an Ant Forest room was found. The only object-source
parse exception was the `end.obj` `#9999999` terminator record.

Repository-wide exact scans found no external QST item command, SHP product,
typed story `item_vnums` field, ZON reset-role reference, duplicate mobile or
object prototype, or external type-25 target into Ant Forest. The local SHP
source has zero bytes, and no local QST or story item inputs/outputs exist.
All 231 outside story sidecars parsed without errors and contained no numeric
token hits into Ant Forest prototypes. No direct compiled C/C++ prototype
number assignment was found; this does not exclude the generic `ITEM_SWITCH`
dispatch documented above.

## Title and bounded history

After color-code normalization, `Ant Forest` is unique among 442 scanned
current `.zon` titles, with no lexical title candidates. Followed
`ants.wld`/`ants.zon` history stays on the same stem through the 2009
`areas/` tree move, the 2013 same-stem content commit, and the 2015 quest-item
flag pass. The 2015 change to `ants.zon` is only the zone-editor version
comment; it does not show a map replacement. The bounded title and same-stem
history review found no alternate-stem rename, distinct predecessor, or
whole-zone replacement. See
[duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep `ants` **DO NOT MOVE** because it is a current un-commented
`areas/AREA` contributor. Preserve the reciprocal Surface boundary, both
data-driven switch interactions, reset states, and the separate source-only
Surf approach. Keep `surf` **RETAIN IN PLACE** with existing candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); add no candidate row for
Ant Forest. The inactive approach is not active ingress, and the static
graph/handler analysis does not qualify runtime access, switch usability, safe
travel, or migration clearance. See Ant Forest coverage row
[8](ZONE_COVERAGE_REGISTER.csv#L8) and the
[source story dossier](../zone-stories/ANT_FOREST.md).
