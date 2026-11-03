# The Wandering Fields repository-wide audit review

Checkpoint 429; physical coverage row 342, area:wander. This supplement
completes the source-backed story, directed graph, typed-reference, title,
bounded-history, and active-area disposition review. It accompanies the
[source-story dossier](../zone-stories/WANDERING_FIELDS_STORY_REVIEW.md) and
[builder questions](../zone-stories/WANDERING_FIELDS_BUILDER_QUESTIONS.md).
No source area, native gameplay file, or runtime state was changed.

## Active source and authored purpose

The active contributor is listed in [areas/AREA](../../../areas/AREA#L176).
Zone 366 is titled “The Wandering Fields” and contains 14 rooms (#36600–#36613),
27 mobiles (#36600–#36626), and one object (#36600). WLD, MOB, OBJ, and ZON
files exist. The [QST index](../../../areas/qst/AREA#L163) lists wander but
there is no local QST file. There is no local SHP file or story sidecar. The
zone uses reset mode 2 and has 109 commands: 85 M, 10 E, and 14 G.

The root-room prose describes a somewhat random mobile-loading section for
Llzazan. Six local destination-selector rooms each have six outbound exits to
Llzazan; the probability chain and “Cosmic Toilet” are separate loader
branches. The local mobile text describes Rothe, mushroom spores, enslaved and
rebellious svirfneblin, Drow ghetto residents, exiled clerics, acrolas, and
Zarbons. The source defines environmental setting, not a native player
contract.

## Active WLD graph, directed reach, and approaches

The parser resolves all 54 authored exits: 18 are local and 36 leave for active
Llzazan. There are no negative or unresolved exits, no range-mismatch rooms,
and no reverse WLD pair. The local undirected graph is one weak component of
14 rooms.

Under local directed open-edge traversal, #36600 reaches only itself and the
six selector rooms #36601–#36606. Each of those six has only outbound
cross-zone WLD exits and no local return edge. In the other direction, #36607
and #36609–#36613 can each reach #36600, but no edge from #36600 enters that
probability chain. Room #36608 has no exits. This graph separates the reset
staging topology from the city routes; it does not declare ordinary player
access or a safe return.

Every cross-zone edge targets Llzazan. The 36 edges cover 35 distinct target
rooms because #36602 points to #36267 both south and down
([first target](../../../areas/wld/wander.wld#L77),
[second target](../../../areas/wld/wander.wld#L89),
[cross-exit register](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L493)). A scan of
all active WLD files found no incoming exit to #36600–#36613. The 95 inactive
WLD files contain no target to a Wander room and no room-VNUM overlap. Curated
source-only WLD totals therefore remain 1,506 overall and 123 for Surf.

No fall or current fields and no D reset commands were found. The source/code
movement boundary still matters: the random NPC wandering branch uses
CAN_GO and excludes NO_MOB and no-ground destinations; ACT_STAY_ZONE limits
cross-zone movement. None of the 27 local mobile act fields sets SENTINEL or
STAY_ZONE, all 36 exits have zero exit flags and key VNUMs, and the Llzazan
targets are not marked NO_MOB. This supports the authored NPC-distribution
role without measuring actual spawn frequency, movement outcomes, or route
safety. See [wandering logic](../../../src/mob/mobact.c#L8018),
[movement gate](../../../src/cmd/actmove.c#L1339), and
[flag definitions](../../../src/core/defines.h#L1165).

## Typed dependencies and external consumers

The local ZON imports the Arachdrathos Wilderness mobile #36011 twice at room
#36600 ([reset](../../../areas/zon/wander.zon#L26),
[owner prototype](../../../areas/mob/arac_wild.mob#L168)). It also uses
Llzazan objects #36204 ten times and #36205 twice in G resets associated with
the rothe placement group ([first reset](../../../areas/zon/wander.zon#L29),
[second object reset](../../../areas/zon/wander.zon#L39),
[Llzazan objects #36204](../../../areas/obj/llzazan.obj#L57) and
[#36205](../../../areas/obj/llzazan.obj#L70)). Preserve those prototype
owners and reset contexts.

The exact external typed scan found six M reset records in unregistered
ud_map0.zon that consume active Wander mobiles: #36605 at rooms #1102380 and
#1105256; #36626 at #1104623, #1105159, and #1105240; and #36601 at #1105256
([first](../../../areas/zon/ud_map0.zon#L10),
[later records](../../../areas/zon/ud_map0.zon#L20), and
[remaining records](../../../areas/zon/ud_map0.zon#L27)). They are source-only
references and do not prove current placement. Preserve them under the
existing [ud_map0 candidate row 112](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L112).

No other active external ZON reset refers to a Wander prototype. Exact scans
found no external QST item command, SHP product entry, story-sidecar numeric
reference, same-type mobile or object definition, type-25 portal target, or
C/C++ direct VNUM reference into the local room/mobile/object sets. No active
external WLD approaches a local room. The local inbound WLD and compiled-code
scans are empty; the typed ud_map0 resets remain separately classified as
source-only dependencies. The existing
[Llzazan review](LLZAZAN_STORY_REVIEW.md#directed-wld-reachability) independently
records 36 Wander arrivals and leaves their player/mobile role open. The
[Arachdrathos review](ARAC_WILD_AUDIT_REVIEW.md#typed-dependencies-and-compiled-travel-endpoints)
records the same two imports of mobile #36011.

## Title, history, and owner boundary

After color-code removal, “The Wandering Fields” is unique among the active
.zon titles. The followed WLD/MOB/OBJ/ZON history stays on the wander stem
from the tracked 2009 areas-tree relocation through later same-stem edits.
No alternate-stem predecessor, title rename, or whole-zone replacement was
identified.

The WLD states that Wander is used for Llzazan. The cross-zone targets and
imported Arachdrathos mobile #36011 show functional ties between three
contributors; they do not transfer ownership. Keep Wander, Llzazan, and
Arachdrathos as separate active owners. The unresolved command-target alias
field remains separate from title and replacement review.

## Safe disposition

Keep active wander **DO NOT MOVE** as a current full-area contributor.
Preserve all 36 one-way exits into Llzazan, both Arachdrathos #36011 resets,
the Llzazan #36204/#36205 imports, and the active mobile prototypes referenced
by ud_map0. Keep ud_map0 **RETAIN IN PLACE** under its existing candidate row
until its map owner and tool/editor closure are resolved. No new attic
candidate row or migration clearance is warranted.

The normalized active title review, bounded source history, and typed
dependency closure are complete. Live reset odds, mob placement, ordinary
player routes, and safe return remain unqualified. No test, build, server,
gameplay, database, migration, or operational command was run.
