# Wildland Trails bounded source audit

Canonical key: `area:wildland_trails`. Physical coverage row 346. This
checkpoint supplements the [environmental story review](../zone-stories/WILDLAND_TRAILS_STORY_REVIEW.md)
with directed-map, typed-reference, history, and candidate-identity evidence.

## Checkpoint 433: source inventory and environment

The active source set is `wildland_trails.wld`, `.mob`, `.obj`, and `.zon`:
99 rooms, 25 mobiles, 13 objects, and 142 reset commands. The reset command
counts are 99 M, 18 D, 16 E, six G, one O, and two P. All parsed local mobile,
object, and room command targets resolve to the same-stem prototypes. There is
no local QST, SHP, or story sidecar. The [story review](../zone-stories/WILDLAND_TRAILS_STORY_REVIEW.md)
records the landscape, Welgurd camp, outpost, cave, key, and special-procedure
context.

## Active and source-only WLD graph

The active map has 198 exits: 196 resolve to local rooms and two point to
positive room VNUMs outside the active `areas/AREA` world list. They are already
preserved as [unresolved-exit register rows 40 and 41](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L40):
Wildland room #14601 south to #217245, and room #14677 north to #215745. The
normal build omits the candidate sources and `renum_world` prunes these exits.
Both VNUMs are present in inactive `surf.wld` and `surface2011.wld`, but the
room identities differ: Surf #217245 is a forested crevice and #215745 is a
Quistery valley floor; both Surface2011 records are Azure Waters of a Storm
Sea. Do not infer the active target owner from the numeric VNUM alone.

Inactive `surf.wld` also contains the reverse source records: #217245 north to
Wildland room #14601 and #215745 south to #14677. They are source-only map
approaches, not active ingress. The corresponding inactive map evidence stays
on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97)
and [Surface2011 candidate row 98](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L98).
No active-area WLD source enters Wildland Trails; the only outward exits point
to inactive target candidates, so no cross-area route is active between loaded
areas. Its local open-edge graph has one weak component containing all 99
rooms. Starting at either source-only arrival room (#14601 or #14677) reaches
70/99 local rooms; every local room has an open-edge path back to each arrival
room. These graph results do not resolve the two candidate targets, the
closed/secret/locked resets, actual player entry, or safe return.

## Typed dependencies and local special assignments

Every role-parsed reset reference in local `wildland_trails.zon` is same-stem:
all M mobiles, E/G/O/P objects, and D rooms resolve locally. The source set has
no same-stem quest, shop, or story sidecar. Exact external scans found no
reference to its room/mobile/object VNUM sets in active or unregistered QST
item commands, ZON reset roles, SHP product entries, structured story-sidecar
fields, or same-type MOB/OBJ definitions. An active-object scan found no
type-25 portal target into the 99 local rooms among 872 portal declarations.

A raw C/C++ numeric scan found four meaningful `real_mobile0` assignments in
`specs.assign.c`, all local: the three named vine effects and the Spiritist
procedure. Its fifth numeric match is a Unicode range endpoint, not a zone
dependency. The paralysis procedure's `CMD_DEATH` cleanup is unreachable after
the nonzero-command return; the Spiritist has a branch that drops a weapon
after setting `ITEM_SECRET`. These are source-control-flow observations, not
runtime qualification. See the [environmental review](../zone-stories/WILDLAND_TRAILS_STORY_REVIEW.md#special-procedures-and-behavior-boundary).

The numeric overlap of room, mobile, and object VNUMs is intentional
type-separated source data. In particular, mobile corpse #14600 and key object
#14600 are distinct prototypes. Preserve type identity; no exact external
typed consumer or same-type prototype owner was found in the bounded scan.

## Title, followed history, and owner boundary

After color-code removal, “The Wildland Trails” is unique among current active
ZON titles. Followed WLD, MOB, OBJ, and ZON histories reach the August 2009
areas-tree move and remain on the `wildland_trails` stem. The latest followed
WLD/OBJ commits in the checked history are 2013 edits; MOB continues to an
August 2026 edit, ZON to October 2015. The special-procedure source has a
September 2026 integration commit. No distinct alternate-stem predecessor or
whole-area replacement was identified. The “WD bypass road” wording is a
comment in the code assignment block, not evidence of a renamed zone.

## Safe disposition

Keep active `wildland_trails` **DO NOT MOVE**. Preserve its current source files,
all locked/secret edge records, and the existing source-only Surf and
Surface2011 evidence on candidate rows 97 and 98. The current map contains two
unresolved exits to room numbers shared by candidates with different room
identity; neither candidate is active. Do not create a new candidate or claim
that either path is usable. No world-source file was changed for this audit.
