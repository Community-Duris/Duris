# Dark Outpost: map, identity, and dependency audit

Reviewed October 11, 2026, Checkpoint 469, against source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static review of the unregistered `darkop` component. It does not establish that the map, reset file, or any same-numbered source is loaded at runtime.

## Source scope and story

The candidate consists of [`darkop.wld`](../../../areas/wld/darkop.wld), [`darkop.mob`](../../../areas/mob/darkop.mob), [`darkop.obj`](../../../areas/obj/darkop.obj), and [`darkop.zon`](../../../areas/zon/darkop.zon). It is absent from both the full [`areas/AREA`](../../../areas/AREA) and [`areas/qst/AREA`](../../../areas/qst/AREA) lists and has no QST, SHP, or `areas/story` sidecar. Its raw AREA annotation is blank. The ZON title is “dark outpost,” header #1, last VNUM #29448.

The 19 rooms form five separate pieces: a three-room temple and maze; a four-room cavern of dead elves, teleportation cave, and slave pen; a four-room ice cavern and slave pen; a four-room blood-red horror scene with a Device of Dark Compact; and a four-room card-themed area ending at “A Place to Other Places.” The mobile file contains temple sentries, green and ice golems/demons/devils, an unnamed mob in each cavern, child spectres/wraiths and an Angel of Death, and four playing-card figures. Ten objects define six destination portals, two pools, a Device-generated portal, and a hole in time and space. The child-harm and slavery material needs owner/editorial review before any rewrite or activation decision.

The ZON has 20 M resets and ten O resets. Five O records import Player Castles magic-mouth object #11059 at rooms #29410, #29420, #29430, #29440, and #29445. Five local O records place object #29411 in room #29412, #29420 in #29422, #29430 in #29432, #29440 in #29442, and #29445 in #29448. There are no G resets. The six portal prototypes #29410–#29415 are not placed by this ZON; five of their numbers are referenced by the separate unregistered `dark.zon` file, discussed below.

Every WLD room’s zone field is 1, as is the ZON header, while all room VNUMs lie in the #29400 band. The records fit the ZON ceiling #29448, but preserve the room-field/header discrepancy for the builder rather than inferring loader behavior.

## Static WLD graph and Harrow identity overlap

| Measure | Result |
| --- | --- |
| Rooms / authored directions | 19 / 36 |
| Candidate-local directions / outward directions | 32 / 4 |
| Weak components / largest | 5 / 4 rooms |
| Plain directed reach from #29410 | 3/19 candidate rooms |
| Current active-table resolutions | 33/36: 32 local targets in Harrow and #123112 in Surface Keeps |
| Positive targets not in the active table / negative targets | 3 / 0 |
| Active inbound directions to a Dark Outpost identity | 0 |
| Fall / current fields | 0 / 0 |

All 19 candidate room VNUMs also belong to active Harrow; every complete `darkop.wld` room block differs from the same-number Harrow block. All 32 source-local directions therefore resolve numerically to Harrow rooms under the current active WLD set. Harrow has 44 authored directions targeting VNUMs in Dark Outpost’s room set, but those active directions belong to Harrow’s own room blocks. The existing [Harrow audit](HARROW_AUDIT_REVIEW.md#unregistered-wld-target-matches) likewise records Dark Outpost’s 32 target-number matches and 19 overlapping source rooms. These numeric matches do not make Dark Outpost active or establish an active approach to its map identity.

All four source-external directions are one-way in the tracked target room records:

| Source direction | Tracked target definition | Current active resolution |
| --- | --- | --- |
| #29410 up → #123112 | Inactive Duris3 “The Ocean” and active Surface Keeps “The Walls of Alatorin”; neither has a reverse to #29410 | Surface Keeps #123112 |
| #29420 up → #163278 | Inactive Duris3 “The Ocean”; no reverse to #29420 | None |
| #29430 up → #177358 | Inactive Duris3 “The Ocean”; no reverse to #29430 | None |
| #29445 up → #182631 | Inactive Duris3 “The Ocean” and inactive Oldmap8 “The Vast Grey Drylands”; neither has a reverse to #29445 | None |

The source has three positive targets absent from the active table: #163278, #177358, and #182631. All four external VNUMs occur in inactive Duris3, and #182631 also occurs in Oldmap8; #123112 additionally resolves to active Surface Keeps. Since every Dark Outpost room number is already owned by active Harrow, its #29410 direction to Surface Keeps is not a confirmed non-overlapping inactive-WLD approach. No new eligible approach is added to the curated inactive-WLD total. The map has no negative targets, range mismatch, fall field, or current field; static traversal ignores door state and runtime conditions.

## Candidate map and cross-area identity

All 18 candidate mobile VNUMs and all ten candidate object VNUMs also occur in active Harrow. Every full mobile and object block differs from its Harrow counterpart. For example, Dark Outpost mobile #29410 is a temple sentry versus Harrow’s sunflower; mobile #29411 is another temple sentry versus Harrow’s kitten. Dark Outpost object #29440 is a portal to the Living while active Harrow #29440 is a near-finished painting. Treat Harrow’s complete active map and prototype blocks as the current numeric owners; retain Dark Outpost’s different records as unregistered source data.

The unregistered `dark.zon` file has five object reset references outside `dark.obj`: #29410 and #29412–#29415. Those numbers are defined as destination portals in unregistered `darkop.obj`, but the current active object table assigns them to Harrow. This is a source-backed candidate-to-candidate numeric overlap, not proof that `dark` intended to import `darkop` portals; the [Dark audit](DARK_AUDIT_REVIEW.md#conditional-shp-fallback-and-exact-numeric-consumers) records the current Harrow resolution. Confirm the relationship among `dark`, `darkop`, and Harrow before changing these objects or their reset references.

There is also tracked content reuse with unregistered BSGHOP. Dark Outpost’s room text and most exits for #29440–#29443 reproduce BSGHOP’s compact horror map; #29440 shares its description and north exit, while BSGHOP adds a negative up target. Dark Outpost object #29440 has the same portal name and prose as BSGHOP’s portal but different numeric fields. This establishes shared source content, not canonical ownership or replacement. See the [BSGHOP audit](BSGHOP_AUDIT_REVIEW.md#conflicting-numeric-ownership-and-copied-room-text).

Active Harrow’s own ZON and QST records use many of these VNUMs, which remain Harrow-owned. Harrow’s separate audit identifies its active Tundra story-sidecar use of object #29440 and compiled item slots; those are current Harrow consumers, not evidence that Dark Outpost is registered. Other unregistered resets also reuse Harrow/Dark Outpost numbers: `dark.zon`, `bsghop.zon`, and `infernal_gh.zon`. Keep their individual source identities separate until builders resolve the relationships.

## Title, history, and retention

After color-token, tilde, and whitespace normalization, “dark outpost” is unique among the 442 tracked ZON titles. The neighboring unregistered `dark` title is “guildhall of the dark”; `dark_gh` is “the Dark Guildhall.” Their related wording, the adjacent #29400–#29448 bands, temple themes, and `dark.zon`’s portal reset numbers suggest questions for the builders, but do not establish an alias or planned connection.

Followed histories for the four `darkop` paths reach the 2009 area-tree move. Later visible edits are same-stem changes through 2013; no alternate-stem rename, predecessor, or explicit whole-zone replacement appears in those tracked paths. This bounded history does not establish earlier upstream lineage or settle the shared BSGHOP content.

Keep candidate row 32 **RETAIN IN PLACE**. Resolve the 19 Harrow room, 18 mobile, and ten object VNUM collisions; the 44 active Harrow numeric target matches; the four one-way boundaries; the WLD zone-field discrepancy; Player Castles resets; Dark-to-Darkop portal references; BSGHOP content reuse; active Harrow QST/Tundra/code owners; and story/editor/tool/deployment/persisted-data ownership before any separate move or activation proposal. See the [32 builder questions](DARKOP_BUILDER_QUESTIONS.md), [coverage row 383](ZONE_COVERAGE_REGISTER.csv#L383), [candidate row 32](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L32), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-469-dark-outpost), [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-469-dark-outpost-map-identity-and-history), and [migration plan](ATTIC_MIGRATION_PLAN.md#checkpoint-469-dark-outpost-retained-pending-owner-review).
