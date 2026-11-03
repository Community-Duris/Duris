# plane_avernus source audit

Canonical key: component:plane_avernus. Physical coverage row 433 / CSV
line 434; candidate dependency row 82 / CSV line 83. Reviewed October 11,
2026, Checkpoint 520, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static narrative,
connectivity, identity, and typed-reference review. It does not establish
runtime loading, editor ownership, or safe migration.

## Package and identity

The source set has [plane_avernus.wld](../../../areas/wld/plane_avernus.wld#L1),
[plane_avernus.zon](../../../areas/zon/plane_avernus.zon#L1), and empty
[plane_avernus.mob](../../../areas/mob/plane_avernus.mob) and
[plane_avernus.obj](../../../areas/obj/plane_avernus.obj) files. There is no
same-stem QST, SHP, map-coordinate file, or story sidecar. The component is
absent from [areas/AREA](../../../areas/AREA) and
[areas/qst/AREA](../../../areas/qst/AREA); the current
[RANDOM_AREA](../../../areas/RANDOM_AREA) is empty.

Zone #135 is titled “The Plane of Avernus” and ends at #136 with reset mode
2. That normalized title exactly matches active zone #320 in
[avernus.zon](../../../areas/zon/avernus.zon#L1); the active area is listed
in [areas/AREA](../../../areas/AREA#L156) and
[qst/AREA](../../../areas/qst/AREA#L145). The title match does not establish
that the two zones are the same map: active Avernus occupies its own
#32000–#32399 WLD range, while this candidate defines only #135 and #136.

## Minimal authored map and reset state

The WLD contains exactly two rooms. Room #135 is titled “The Barbarian
Wastelands of Avernus,” but its room description is empty; room #136 is
“Unnamed,” also with an empty description. The rooms have only two exits:
#135 east to #136 and #136 west to #135. Both directions are reciprocal,
forming one two-room weak component
([room #135](../../../areas/wld/plane_avernus.wld#L1),
[room #136](../../../areas/wld/plane_avernus.wld#L10)). There are no
external targets, doors, fall fields, or current fields.

The ZON contains a header and terminator only; it has no reset commands
despite recording reset mode 2
([plane_avernus.zon](../../../areas/zon/plane_avernus.zon#L1)). The empty
MOB and OBJ files add no populations or object placements. No quest, shop,
or authored narrative content accompanies the two-room map. Static evidence
therefore describes a tiny reciprocal map stub with competing Avernus names,
not a developed route or story.

## Active ownership and duplicate VNUM sources

No current active WLD owns room #135 or #136, and the active-WLD source scan
found zero target rows into either candidate room. Active reset-command
room fields, active QST/SHP numeric records, and active ITEM_TELEPORT
destinations also produced no references to #135 or #136. Item type 25 is
ITEM_TELEPORT ([defines.h line 140](../../../src/core/defines.h#L140)).
No direct plane_avernus literal or matching room-lookup expression was found
in the scanned C/C++ source.

The low room numbers are not unique across source maps. Two other unlisted
WLDs also define both IDs:

| VNUM | plane_avernus | Dalvik | Mavulsk |
| ---: | --- | --- | --- |
| #135 | The Barbarian Wastelands of Avernus | A Valley of Rock Formations ([line 3251](../../../areas/wld/dalvik.wld#L3251)) | A Blocked Tunnel ([line 2791](../../../areas/wld/mavulsk.wld#L2791)) |
| #136 | Unnamed | Dead End in a Valley of Rock Formations ([line 3271](../../../areas/wld/dalvik.wld#L3271)) | A Tunnel in the Mines ([line 2806](../../../areas/wld/mavulsk.wld#L2806)) |

Dalvik and Mavulsk are not in the current full-area list; their room blocks
and exit tuples differ from plane_avernus. These are competing inactive
room identities, not evidence that any one file supersedes the others.
Active Heavens also has OBJ prototypes #135 and #136
([Heavens object #135](../../../areas/obj/heavens.obj#L1812),
[Heavens object #136](../../../areas/obj/heavens.obj#L1820)). Those are
object VNUMs in a separate typed namespace; plane_avernus has no object
prototypes. The existence of same-number objects does not make Heavens the
owner of candidate rooms.

## Typed-reference limits and history

The static scan found no active WLD or ZON approach to #135/#136, no active
shop or QST numeric reference, and no active type-25 teleport destination.
The candidate has no mobile or object prototypes for external ZON/QST/SHP
records to import. This closes the current source-tree references visible to
those scans; it cannot see editor projects, external imports, deployed
aggregates, dynamically computed room numbers, or persisted player/world
identities.

Checkpoint 192 already recorded that the repeated normalized Avernus title
is a title collision rather than a replacement finding. Bounded tracked-path
history for plane_avernus shows no rename, predecessor, or whole-area
replacement; see the prior
[Avernus audit](AVERNUS_AUDIT_REVIEW.md) and
[Checkpoint 192 identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-192-avernus-title-and-path-history).
The competing inactive room definitions in Dalvik and Mavulsk do not resolve
who owns this two-room file.

**Disposition: RETAIN IN PLACE.** Preserve the two-room WLD and its distinct
source identity until the owner confirms the duplicated zone title, intended
relationship to active Avernus, VNUM provenance relative to Dalvik and
Mavulsk, empty reset/population state, and editor/import history. No
activation, merge, renumber, move, or archive is cleared. See
[20 builder questions](PLANE_AVERNUS_BUILDER_QUESTIONS.md),
[coverage row 433](ZONE_COVERAGE_REGISTER.csv#L434),
[candidate dependency row 82](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L83),
[connectivity checkpoint 520](WORLD_CONNECTIVITY.md#checkpoint-520-plane_avernus-two-room-map-with-no-active-ingress),
[identity checkpoint 520](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-520-plane_avernus-title-collision-and-three-source-room-identities),
and [retention checkpoint 520](ATTIC_MIGRATION_PLAN.md#checkpoint-520-plane_avernus-retain-pending-map-identity-and-provenance).
