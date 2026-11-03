# The Troll Hills repository-wide audit supplement

This supplement reconciles active and inactive connectivity, typed dependencies, title ownership, and bounded source history for the [source-comprehensive Troll Hills dossier](../zone-stories/TROLL_HILLS.md). The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation audit; no area or gameplay source is changed.

## Active graph and boundary records

The active `troll_hills` WLD has 148 rooms and 460 authored exits. All 460 resolve: 457 are local and three leave the area. The weak graph is one 148-room component. With every authored local exit assumed available, directed traversal from road room #1801 reaches all 148 rooms and each room has an authored directed return path to #1801. This raw graph omits the dossier's secret-door, water, actor, key, hazard, and availability conditions.

| Active boundary | Record and relationship |
| --- | --- |
| Troll Hills #1801 north → Surface #625367 | [Troll Hills exit](../../../areas/wld/troll_hills.wld#L36) and [Surface return](../../../areas/wld/surface.wld#L2632124), #625367 south → #1801; reciprocal pair. |
| Troll Hills #1820 south → Surface #626167 | [Troll Hills exit](../../../areas/wld/troll_hills.wld#L597) and [Surface return](../../../areas/wld/surface.wld#L2648936), #626167 north → #1820; reciprocal pair. |
| Troll Hills #1909 north → Ghore #11540 | [Troll Hills exit](../../../areas/wld/troll_hills.wld#L3189) and [Ghore return](../../../areas/wld/ghore.wld#L910), #11540 south → #1909; reciprocal pair. |
| Lortower #134000 east → Troll Hills #1883 | [Lortower exit](../../../areas/wld/lortower.wld#L16); no reciprocal Troll Hills exit is authored at #1883. |

There are four active inbound records, three outbound records, and three reverse records. Troll Hills has two fall-chance fields and no current fields; see the dossier for the ladder endpoints and conditional travel routes.

The [Lortower orientation note](../../../areas/story/lortower.story.json#L19) describes an outward route to Troll Hills #1883 and the absence of ordinary incoming exits into Lortower's own room range. The active Lortower #134000 → #1883 edge agrees with that direction. This text reference is narrative context, not a typed game dependency.

## Inactive-WLD approaches and attic evidence

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. No inactive source room overlaps the 148 active Troll Hills room VNUMs. It found two source-only approaches, both in unregistered `surf.wld`:

| Inactive source-only approach | Evidence |
| --- | --- |
| #278929 south → Troll Hills #1852 | [Surf exit record](../../../areas/wld/surf.wld#L1656261). |
| #279529 north → Troll Hills #1871 | [Surf exit record](../../../areas/wld/surf.wld#L1668829). |

These are not active ingress because `surf.wld` is not an active world contributor. At Checkpoint 126, the two records raised the curated source-only approach total from 1,997 to 1,999 and the reviewed Surf subset from 144 to 146; those counts are historical, and both approaches are already included in the current 11,993 overall / 415 Surf total. Preserve the source-map identity and keep `surf` / `surface2011` **RETAIN** pending map ownership and full dependency/tool closure. This finding grants no migration clearance; active `troll_hills` remains **DO NOT MOVE**.

## Typed dependencies and bounded negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Imported reset equipment | Five Troll Hills `E` resets import external object #283 at [reset lines 28, 30, 43, 57, and 76](../../../areas/zon/troll_hills.zon#L28); its prototype is defined in [Heavens](../../../areas/obj/heavens.obj#L3036). This is ordinary source equipment, not a QST acceptance or a local object prototype. The local [Q23](../../../areas/qst/troll_hills.qst#L23) separately consumes idol #1804 and issues goo #1807 plus 5,000 experience. |
| Outside QST, SHP, and sidecar references | No exact outside QST item command or SHP product references a local Troll Hills object. Troll Hills has no local SHP source. Its own story sidecar has no out-of-area typed item-VNUM dependency. White Hills sidecar story IDs 1937 and 1918 are JSON story identifiers, not Troll Hills VNUM dependencies. |
| Duplicate prototypes and portal arrivals | The bounded active/unregistered prototype scan found no outside duplicate mobile or object definition for the local Troll Hills prototypes. One external type-25 declaration targets room #1823: Airp cloud portal #131641 at #131652 ([object record](../../../areas/obj/airp.obj#L470)). It has command 0 and zero charges; generic dispatch requires the declared command to match the actual command, so this record is not a qualified incoming route. |
| Compiled references | The notable literal compiled binding is [specs.assign.c](../../../src/specs/specs.assign.c#L417), which assigns `bridge_troll` through missing mobile #1919. The actual half-breed prototype is #1819, but its stocked room and the handler's selected endpoints do not establish a coherent crossing. The dossier records the failed binding and handler in detail; changing the VNUM alone would not repair the service. |
| Narrative and numeric matches | Lortower's #1883 sentence is route prose, while the White Hills `id` values are sidecar keys. Other direct numeric search hits include constants and time values. They are not typed dependencies. The active WLD exit from Lortower is recorded separately above. |

The dossier remains the detailed authority for the idol exchange, reset stock, optional treasure, SEARCH/OPEN access, fall, device and potion behavior, bridge handler, malformed #1820 header, and builder follow-ups. Static source and graph results do not establish live stock, successful travel, bridge service, ownership, or persistence.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, “The Troll Hills” has one exact owner among 351 active `.zon` titles: `troll_hills` ([title source](../../../areas/zon/troll_hills.zon#L2), [active area membership](../../../areas/AREA#L7)). The lexical title scan found “The Dark Stone Tower of the Northern Realms” as a nearby phrase, but it is the distinct `teka2` title.

Followed WLD, ZON, MOB, OBJ, and QST histories remain on the `troll_hills` stem through the 2009 `areas/` relocation and same-stem edits through 2013. The story sidecar was added in 2026 in commit `3c13be6d6`. These bounded paths show no alternate-stem rename, explicit predecessor, or whole-zone replacement claim; they cannot establish the absence of untracked history. Keep active `troll_hills` **DO NOT MOVE** and keep `surf` / `surface2011` **RETAIN**. No source placement changed.

## Builder recommendations

1. Preserve the three reciprocal Surface/Ghore pairs and the separate Lortower inbound record. Keep the one-way Lortower-to-Troll Hills route distinct from inbound access to Lortower.
2. Preserve the two inactive Surf approaches as source-only map evidence. Do not present them as current player access or migration clearance.
3. Keep the missing #1919 binding, mismatched bridge destinations, and active-accounting NPC-payment incompatibility as one coherent builder review. A binding-only edit does not establish a usable bridge.
4. Treat the Airp #1823 portal as a candidate dependency until builders select its intended command and travel behavior and an admitted route is qualified.
5. Keep the #1820 metadata correction separate from journal work; confirm intended room fields and use loader-level before/after evidence in a separately named repair.

## Checkpoint 402: explicit builder questions

The [Troll Hills dossier](../zone-stories/TROLL_HILLS.md#explicit-builder-questions-and-recommended-resolutions)
now expresses the existing twenty findings as direct, source-linked builder
questions with conservative recommendations. CP402 completes the builder
question field for physical coverage row 315. It does not implement the
bridge, room-header, travel, journal, or item-action proposals above.

## Checkpoint 857: Current boundaries and source-only approaches carried forward

The reviewed `areas/` and `src/` content remains identical to pinned source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`; this checkpoint carries the Checkpoint 126 connectivity/dependency review and Checkpoint 402 builder recommendations forward without claiming a second independent parse. Troll Hills remains 148 rooms with 460/460 resolving exits (457 local, three outbound), in one weak component. Its three reciprocal pairs remain Surface twice and Ghore once; the separate inbound from Lortower #134000 east to #1883 remains one-way. The two inactive Surf approaches (#278929 south to #1852 and #279529 north to #1871) stay source-only on existing candidate row 97 and are already included in the current 11,993 overall / 415 Surf total. The older Checkpoint 126 count of 1,999 / 146 is historical.

The typed closure remains unchanged: five reset commands import Heavens OBJ #283; local Q23 still consumes idol #1804 and declares goo #1807 plus experience; Airp portal #131641 still declares room #1823 with command zero and zero charges; and the `bridge_troll` assignment still refers to missing mobile #1919 while the local half-breed is #1819. No outside exact QST item or SHP product, external story-item reference, or duplicate mobile/object prototype was established. The failed bridge binding and Airp declaration remain unresolved source evidence, not working service or qualified arrival. The twenty builder recommendations from Checkpoint 402 remain source review, not gameplay qualification.

Keep active `troll_hills` **DO NOT MOVE**. Preserve both Surf approaches on existing candidate row 97 and keep `surf` / `surface2011` **RETAIN IN PLACE** pending ownership and tooling closure. No source activation, relocation, route repair, or migration clearance follows. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L315), [connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-857-troll-hills-boundaries-and-surf-approaches-remain-unchanged), and [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).
