# Skulldrach source audit

Checkpoint 363 closes source review for physical coverage row 276, area:skulldrach. The new [source story map](../zone-stories/SKULLDRACH.md) covers the local room and creature prose, six objects, 220 resets, the missing local QST source, and Bastine's exact shield dependency. This supplement records the active and inactive WLD boundaries, title and path history, same-type identity, and bounded typed callers. It makes no runtime or gameplay claim.

## Static connectivity and source-only approaches

The active WLD contains 252 rooms and 691 directional records. All 691 exit flags and keys are zero. Of those records, 686 target local rooms, two are reciprocal active cross-area pairs, and three target -1. The positive cross-area directions are #12796 north to Citadel #13000 and #12859 south to Surface #632637. Citadel #13000 returns south to #12796; Surface #632637 returns north to #12859. The [Citadel boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L221) and [Surface boundary](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L222) have reciprocal return rows #223 and #1658. The three negative exits are documented in the [unresolved exit register](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L24); the loader drops them. There are no positive unresolved targets, door-reset commands, room fall markers, or current markers.

The local weak graph has two components of 225 and 27 rooms. Directed traversal from room #12700 reaches all 225 rooms in the larger component. The 27-room component is a separate trail through a gully, clearing, dark tree cover, and the foothills; its three negative exits do not connect it to another active room. This is an authored graph summary, not a played route or safety assessment.

All 95 current inactive WLD files parsed without errors: 527,791 rooms and 2,080,773 exits. No inactive source room reuses a Skulldrach room VNUM. One non-overlapping inactive Surf direction reaches the area: room #273887 north to #12859. Preserve it on existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) as source-only, not active ingress. Surf's reviewed source-only subset rises from 281 to 282; the curated total rises from 4,532 to 4,533. Surf's 145 active references are unchanged. No other inactive-WLD approach was found.

## Typed dependencies and code references

The local qst/AREA index lists skulldrach, but there is no local QST file, Q or QA contract, or story sidecar. The typed external scan found one exact QST item command: active Bastine line 50 contains an exact item-give request for object 12802. Bastine's story sidecar contains item_vnums [12802] and a hint that the stone golem near Skulldrach Lake carries the blessed shield. Locally, reset order places object 12802 on mobile 12704 at room 12796. Preserve the identity and reset relationship together.

No external active or source-only ZON reset role targets a Skulldrach room, mobile, or object. No external SHP product references a local object. The scan of 713 active and 159 inactive type-25 object prototypes found no fixed destination in the 252 local rooms. A typed direct-number review of maintained C/C++ found two calls to load mobile #12803: wonder_actions.c's summon helper and the Highway special's read_mobile call. The numeric VNUM 12803 also names local object rusty iron helm; those code paths load the mobile, not the object. A broad numeric scan's only other token match was embedded in a hexadecimal SHA constant and is not a prototype reference.

## Title, same-type identity, and tracked history

After color-code removal, terminator trimming, whitespace normalization, and case folding, Skulldrach is unique among all 442 current ZON titles and 351 active contributors. Same-type scans across active and inactive source files found no other WLD, MOB, or OBJ header using a Skulldrach VNUM. Surface's current map includes descriptive cells named for the desolate grasslands near Skulldrach Lake; those labels are map prose, not a room-VNUM overlap or a replacement for the active Skulldrach contributor. The exact active Surface boundary is #632637 north to #12859.

Followed WLD, MOB, OBJ, and ZON paths remain on the skulldrach stem through the 2009 area-tree move; no alternate-stem predecessor or whole-zone replacement was found in those bounded paths. The WLD history records a 2011 change at #12859 from south to Surface #632637 to east toward #215393, then a 2013 restoration of the south exit to #632637. The 2015 mobile-field migration and the 2026 fisherman numeric-field correction do not establish an area rename. Keep the active contributor DO NOT MOVE.

## Retention and review boundary

Preserve the two reciprocal active boundary pairs, all three negative-target records, the separate 27-room trail component, exact reset parentage for the blessed shield, and the Bastine typed consumer. Preserve the single Surf source-only approach on row 97. The [source story map](../zone-stories/SKULLDRACH.md#builder-questions-and-conservative-recommendations) contains twenty conservative builder recommendations.

This documentation-only checkpoint did not run tests, a build, a server or gameplay session, database work, or an operational script. Actual access, door semantics, lake survival, resets, loot persistence, and quest completion remain outside this static review.
