# Water Plane source audit

Canonical key: `component:waterplane`. Coverage data row 470 / CSV line 471;
existing candidate dependency row 119 / CSV line 120. Reviewed October 12,
2026 (Checkpoint 557) against source snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static source, graph,
identity, followed-history, and typed-reference review. It does not establish
live loading, player access, reset outcome, editor authority, deployment state,
or persisted identity.

## Source package and authored story

The unregistered package contains [waterplane.wld](../../../areas/wld/waterplane.wld#L1),
[waterplane.mob](../../../areas/mob/waterplane.mob#L1),
[waterplane.obj](../../../areas/obj/waterplane.obj), and
[waterplane.zon](../../../areas/zon/waterplane.zon#L1). The WLD has twelve
rooms (#1–#10, #12, and #72), the MOB has three prototypes, and the OBJ file
is zero bytes. There is no same-stem QST, SHP, story sidecar, or listed map
editor source. `waterplane` is absent from [areas/AREA](../../../areas/AREA)
and [areas/qst/AREA](../../../areas/qst/AREA); `areas/RANDOM_AREA` is empty.
The ZON header is #1, title “Water Plane,” top VNUM #72, and reset mode 2.
The source package has a blank AREA annotation.

Rooms #1–#10 repeat the title “An airy gravity well” and nearly identical
prose. The setting is a silvery water mist with no earth or soil, an unseen
force tugging north, a colored curtain to the south marking the Water/Air
boundary, warm air from the south, cool mist from the north, and dense red fog
obscuring sight east and west. Their extra descriptions call the red fog a
natural phenomenon with a possible chemical hue, attribute the tug to a large
mass and gravity, and describe the colored curtain as magical and opaque
([room #1](../../../areas/wld/waterplane.wld#L1),
[room #6](../../../areas/wld/waterplane.wld#L205)). The source declares exits
through the fog and curtain, but the prose alone does not establish movement,
visibility, or gravity mechanics. The room text says north while the direct
Air boundary is D2 south; preserve that wording and direction until the owner
confirms intent.

Rooms #5–#10 each have an extra standalone `~` after the red-fog extra
description. The all-95 inactive-WLD sweep treated these six lines as empty
terminators; they do not add room or exit records.

Room #12 is titled “The Elemental Plane of Water,” has no description or
exits, and is separate from the main ten-room graph. Room #72 is titled “The
Elemental Plane of Water MOBILE GENERATION ROOM,” also has no description or
exits, and contains all three ZON mobile resets. These labels do not establish
why #12 exists or how the isolated generation room is expected to be used.

The three mobile descriptions are a huge water elemental (#1), a sea serpent
(#2), and a wandering merman/lizard-like watcher (#3). The ZON places one of
each at room #72 ([reset block](../../../areas/zon/waterplane.zon#L10)). There
are no local objects, formal quests, or shop records to connect these actors
to an authored objective. The repeated room prose and mobile text are the
available local story; they do not establish an implemented current, gravity
effect, player route, or live spawn.

## Candidate graph, active identities, and route boundaries

The twelve rooms declare 30 directions: 20 target another numeric room in
the candidate file and ten D2 south exits target active Plane of Air rooms.
All targets are positive; every exit door flag and key VNUM is zero. The
candidate-local numeric graph has three weak components and three SCCs, sized
10, 1, and 1. Room #1 reaches all ten rooms #1–#10 in its component both
forward and in reverse. Rooms #12 and #72 are isolated. Every source room is
inside the declared #1–#72 range; there are no fall-chance or current fields.

Those graph results describe the source file's numeric graph, not a loaded
Water Plane. Six room IDs are already owned by active WLD records: #1 is
Limbo, and #3, #4, #5, #10, and #12 are Heavens. None of the paired full WLD
blocks matches Waterplane. IDs #2, #6–#9, and #72 have no active WLD owner.
The other unregistered WLD files also reuse these numbers; the full owner
counts and source-local reference scan are summarized below. A target at #4
or #5 is candidate-local by Waterplane's numeric graph and Heavens-owned in
the current active map, so those facts cannot be collapsed into one runtime
claim.

| Waterplane room ID | Current active WLD owner | Other unregistered WLD files with that ID |
| --- | --- | ---: |
| #1 | Limbo | 10 |
| #2 | — | 7 |
| #3–#4 | Heavens | 7 each |
| #5 | Heavens | 6 |
| #6–#9 | — | 6 each |
| #10, #12 | Heavens | 6 each |
| #72 | — | 4 |

The other owners are `astral`, `dalvik`, `hostel`, `Magetower`, `mavulsk`,
`Mentiri`, `mtncastl`, `Nexus_1`, `theurgist`, and `ud-inside` for at least
one Waterplane room ID; none has a complete WLD block identical to the
Waterplane record at the same number. The broad low-VNUM reuse is a competing
identity problem, not evidence of a copied successor.

Ten D2 exits lead from `waterplane.wld` into active Air. Five begin at
Waterplane-specific source IDs (#2, #6–#9) and are the five source-only Air
approaches already counted in Checkpoint 321. The other five begin at source
IDs #1, #3, #4, #5, and #10, which belong to active Limbo/Heavens; retain them
as identity-overlap records, not as additional Waterplane entrances. Five
more exits from Waterplane-specific source IDs #6–#9 target Heavens IDs #4,
#5, and #10. They look local in Waterplane's numeric graph but their current
target identities are Heavens. Together the five unique-source Air edges and
five unique-source Heavens edges are the ten source-only records previously
curated; none is active ingress and none has a checked-in return to a
Waterplane-specific room. The exact Air exit lines and individual targets are
listed in [the Plane of Air review](PLANE_OF_AIR_AUDIT_REVIEW.md#inactive-water-plane-approaches-and-identity-overlaps).

One additional local-number edge, Waterplane #3 D3 to #1, numerically joins
active Heavens #3 to active Limbo #1 if interpreted through current owners.
Both endpoints are competing source identities; it does not prove a
Waterplane-to-Limbo route. The all-95 inactive-WLD scan also found 87
directions from seven other unregistered WLD files targeting Waterplane's
otherwise active-unowned IDs #2, #6–#9, and #72. Every one of those source
files defines the same target VNUM locally, so they are same-source graph
edges in `dalvik`, `hostel`, `Magetower`, `mavulsk`, `Mentiri`, `Nexus_1`, or
`theurgist`, not Waterplane inbounds. No active WLD exit targets
Waterplane-specific IDs. Active exits to the overlapping #1/#3–#5/#10/#12
resolve to Limbo/Heavens identities and are excluded from candidate-specific
ingress. The active-inbound count therefore remains zero.

The 87 source-local references are grouped as `dalvik` 21, `hostel` 16,
`Magetower` 10, `mavulsk` 18, `Mentiri` 11, `Nexus_1` 10, and `theurgist` 1.
This owner-aware inventory is an overlap census, not a count of usable
Waterplane access.

For the coverage metrics, 21 of the 30 candidate exits have target VNUMs
currently present in active WLD sources; nine target the candidate-only
numbers #2 and #6–#9, which are source-local in Waterplane but absent from the
active manifest. Ten exits leave the candidate's numeric room set for Air;
the five source-only exits to Heavens remain local-number records and are
described separately above. No exit targets an unregistered WLD destination,
and no return edge or active inbound is added. The ten source-only Air/Heavens
records were already included in the current 11,993 overall / 415 from Surf
approach totals; Checkpoint 557 adds no route.

## Typed dependencies and shared prototype numbers

Waterplane mobile IDs #1–#3 each collide with a different active Limbo
prototype and with same-number MOB records from six other unregistered
sources (`dalvik`, `hostel`, `Magetower`, `mavulsk`, `Mentiri`, and
`theurgist`). None of the same-number full MOB blocks is identical. The active
Heavens ZON has one reset for mobile #2 in room #1212
([reset](../../../areas/zon/heavens.zon#L286)); the active owner of mobile #2
is Limbo, not Waterplane. The three Waterplane M commands still point to
Waterplane's own #1–#3 records at #72. Do not transfer these IDs between map
owners by number alone.

Across all current ZON files, 20 other unregistered M commands also name
mobile IDs #1–#3: `dalvik` has one, `hostel` nine, `Magetower` three, and
`mavulsk` four. Each of those source files defines the same-number MOB locally
and none is a Waterplane-specific import. The only active M command in this
low range is the Heavens #2 reset documented above.

There is no local object prototype, QST giver, or SHP record. A static scan of
current `areas/obj/*.obj` type-25 objects found no destination in Waterplane's
#1–#72 room range. The active QST and shop scans for the shared Limbo mobile
numbers #1–#3 found no active giver or keeper dependency; the bounded low-ID
scan is recorded in the [Limbo source review](LIMBO_STORY_REVIEW.md#typed-prototype-and-code-references).
No checked-in code binding names the `waterplane` source or uniquely targets
its #2, #6–#9, or #72 rooms. `ROOM_LIMBO_VNUM` assigns #1 to Limbo
([constant](../../../src/core/utils.h#L637)), and the current `WATERP` helper
range is #23201–#23325 ([constants](../../../src/core/utils.h#L1077)). Code
and editor exports outside this repository, deployed copies, and persisted
player data were not inspectable; low numeric IDs alone cannot identify this
candidate.

## Title, followed history, and replacement assessment

After color-code removal and whitespace normalization, “Water Plane” is
unique among the 442 current `.zon` titles. The ZON header number #1 is shared
by 29 other current `.zon` files and is not an identity key. Git history
follows this source on the `waterplane` stem: WLD changes appear in 2011 and
2013, MOB changes in 2012 and 2013, the ZON has a 2015 quest-item-flag edit,
and the zero-byte OBJ path dates to the 2005 initial revision. All four paths
also appear in the 2009 repository tree move. These are same-stem maintenance
and format edits; no alternate-stem rename, predecessor, retirement, or
whole-zone replacement is established.

The active `plane_water_one` contributor is separately titled “Plane of
Water,” owns #23201–#23325, and has its own MOB/OBJ/ZON package, formal Heavens
item dependencies, and Sea Kingdom boundary. The all-95 inactive-WLD review
found no Waterplane overlap or approach into that active range. Similar names
do not make the two sources a rename or replacement; see the [active Plane of
Water review](PLANE_OF_WATER_AUDIT_REVIEW.md#inactive-wld-approaches-and-attic-candidates).

Keep all four `waterplane` source files **RETAIN IN PLACE** on existing
candidate row 120, outside both active manifests. Preserve its repeated
environmental prose, isolated #12/#72 rooms, all three #72 reset records,
source-only Air/Heavens exits, and competing low-VNUM identities. Do not
activate, merge, renumber, regenerate, move, or archive these files until map
ownership, reset/mobile ownership, exit and return intent, editor authority,
deployment, and persisted identity have an owner-backed resolution. This is
an in-place source retention recommendation, not migration clearance.

See the [Waterplane builder questions](WATERPLANE_BUILDER_QUESTIONS.md),
[world-connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-557-waterplane-candidate-graph-and-active-identities),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-557-water-plane-low-number-identities-and-followed-history),
[attic-retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-557-waterplane-retained-with-identity-conflicts),
[coverage row 470](ZONE_COVERAGE_REGISTER.csv#L471), and
[candidate dependency row 119](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L120).
