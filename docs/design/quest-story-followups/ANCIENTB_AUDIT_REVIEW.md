# An Ancient Bridge repository-wide audit review

This supplement completes the source-backed narrative, graph, typed-reference,
title, and bounded history review for active area `ancientb`. It accompanies
the [An Ancient Bridge source story dossier](../zone-stories/ANCIENT_BRIDGE.md),
which maps the bridge, guards, underpass, kobold family, troll, and ordinary
reset objects. All findings are static; they do not qualify installed access,
combat safety, or player outcomes. No native gameplay data changed.

## Active source and story inventory

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 856 is titled
`An Ancient Bridge` and contains 37 rooms (#85600–#85636), 22 mobiles
(#85600–#85621), and two objects (#85600–#85601). Its active files are WLD,
MOB, OBJ, and ZON. The key appears in `areas/qst/AREA`, but
`areas/qst/ancientb.qst` is absent; there are zero native Q/QA contracts. No
local `.shp` or story sidecar exists. The ZON file resets the guards,
travelers, horses, kobold family, troll, crystal ball, and boots. The dossier
maps these as environmental story and reset stock, not a recorded quest
outcome ([source dossier](../zone-stories/ANCIENT_BRIDGE.md)).

The 2014 same-stem commit `dc4aa9310` is titled “Made ancient bridge zone no
gate/port/etc. to stop cheese.” It changes WLD room-flag values and mobile
source fields; it does not establish a different zone owner or replace the
bridge with another map. Preserve that access-control history when considering
any route change.

## Active WLD graph and inactive approaches

The active WLD parser resolves all 74/74 authored exits: 72 local and two
outbound. The local graph is one weak component. Raw directed traversal from
#85600 reaches all 37 rooms, and each room has a path back to #85600 when
authored local exits are treated as available. There are no unresolved active
targets. These are graph results, not a player-safety or runtime-access
finding.

Both active boundaries lead one way to Surface: room #85600 west to #572443
and room #85623 east to #572446. Neither Surface room has an active reverse
exit into Ancient Bridge. See active exit-register [row 1082](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1082),
[row 1083](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1083), and the pair summary
[row 14](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L14); the room sources are
[west end](../../../areas/wld/ancientb.wld#L1),
[east end](../../../areas/wld/ancientb.wld#L561), and Surface destinations
[#572443](../../../areas/wld/surface.wld#L1520735) and
[#572446](../../../areas/wld/surface.wld#L1520798).

All 95 WLD files outside the active `areas/AREA` list parsed without errors.
The sweep covered 527,791 rooms and 2,080,773 exits and found no active-room
overlap. Three non-overlapping, source-only approaches point into the bridge:

| Source record | Source-only exit | Evidence |
| --- | --- | --- |
| `aopal.wld` room #43343, “Spawn Room” | north to #85606 | [Source room](../../../areas/wld/aopal.wld#L817) |
| `surf.wld` room #234341, “An Ancient Stone Highway” | east to #85600 | [Source room](../../../areas/wld/surf.wld#L720433) |
| `surf.wld` room #234343, “An Ancient Stone Highway” | west to #85623 | [Source room](../../../areas/wld/surf.wld#L720475) |

These records raise curated source-only totals from 3,434 to 3,437 overall,
from 201 to 203 for the reviewed Surf subset, and leave `ud_map2` at 15. They
are inactive source declarations, not current ingress. Preserve the `aopal`
record with candidate row [10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10)
and the two Surf records with existing candidate row
[97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Typed cross-area references

Ancient Bridge's active reset file loads mobile #445 into rooms #85600 and
#85623 ([reset rows](../../../areas/zon/ancientb.zon#L51)). The prototype is
defined in `heavens.mob` as a Tharnadian highway patrolman
([prototype](../../../areas/mob/heavens.mob#L1679)). Preserve this outgoing
reset dependency and record it on Heavens coverage row
[121](ZONE_COVERAGE_REGISTER.csv#L121).

Conversely, active Alatorin reset row 2817 loads Ancient Bridge mobile #85617,
the white pony, in room #83361
([Alatorin reset](../../../areas/zon/alatorin.zon#L2817),
[pony prototype](../../../areas/mob/ancientb.mob#L236)). Preserve this incoming
consumer and record it on Alatorin coverage row
[6](ZONE_COVERAGE_REGISTER.csv#L6). The reset import is a typed mobile
dependency, not evidence of a quest or a live route between the two areas.

The exact scans found no external QST item command, SHP product, typed story
`item_vnums` field, external duplicate mobile/object prototype, or external
type-25 target into Ancient Bridge. No additional external reset-role
reference into its rooms or objects was found. Its own zone has no QST/SHP/
story item inputs or outputs; the two Heavens mobile resets are its only
external reset imports. The local reset records and prototype sets otherwise
remain within the zone.

All 231 outside story sidecars parsed without errors and had no numeric token
hits into Ancient Bridge prototypes. Across 872 external type-25 declarations,
none targets an Ancient Bridge room. The only direct numeric C/C++ scan hit is
the number 85606 in a comment at
[`specs.room.c:1`](../../../src/specs/specs.room.c#L1); it is not an executable
room, mobile, or object binding. No local special-procedure assignment was
found.

## Title and bounded history

After color-code normalization, `An Ancient Bridge` is unique among 442
scanned current `.zon` titles, with no lexical title candidates. Followed
`ancientb.wld` history reaches the 2009 `areas/` move, 2013 same-stem content,
and the 2014 access-control pass. Followed `ancientb.zon` history reaches the
2009 move, 2013 same-stem content, and the 2015 quest-item-flag pass. The
bounded title and same-stem history review found no alternate-stem rename,
distinct predecessor, or whole-zone replacement. These checks do not settle
every historical map identity; see
[duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep `ancientb` **DO NOT MOVE** because it is a current un-commented
`areas/AREA` contributor. Preserve its two one-way Surface departures, the
three inactive source-only approaches, the two Heavens patrolman resets, and
Alatorin's reset of pony #85617. Keep `aopal` and `surf` **RETAIN IN PLACE**
with their existing candidate rows; add no candidate row for Ancient Bridge.
Neither source-only exits nor a fully connected raw local graph prove live
access, safe passage, return travel, or migration clearance. See Ancient
Bridge coverage row [7](ZONE_COVERAGE_REGISTER.csv#L7),
[Alatorin row 6](ZONE_COVERAGE_REGISTER.csv#L6),
[Heavens row 121](ZONE_COVERAGE_REGISTER.csv#L121), and candidate rows
[aopal 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10) and
[Surf 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
