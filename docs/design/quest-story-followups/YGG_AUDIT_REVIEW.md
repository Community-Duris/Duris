# Yggdrasil Guild Hall repository-wide audit review

This supplement records the story, graph, title/history, and typed-reference
review for the unregistered Yggdrasil Guild Hall source component. Findings
describe checked-in source; they do not establish live availability, player
traversal, editor authority, or persisted identity. No area or gameplay source
was changed.

## Source inventory and authored story

At source snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697, the current package is
areas/wld/ygg.wld, areas/mob/ygg.mob, areas/obj/ygg.obj, areas/zon/ygg.zon,
and areas/shp/ygg.shp. It is absent from areas/AREA, areas/qst/AREA, and
areas/RANDOM_AREA. There is no ygg.qst or same-stem story sidecar. The ZON
header is #9, the title is “Yggdrasil Guild Hall,” the top room is #30412,
and the reset-mode field is 0. The thirteen room VNUMs #30400–#30412 sit
outside the conventional #900–#999 band for header #9; all 24 authored exits
originate in those out-of-band rooms. Preserve this source mismatch until the
area owner confirms the intended header and VNUM namespace.

The WLD prose presents a mythic ash tree: a root-surrounded quiet camp holds a
loom whose threads form pictures of mankind’s fate; stairs rise from the tree
base through leaf-made rooms and branch paths; a branch honors the faithful
and gods, another displays autumn-colored arms and armor, a squirrel’s nest
holds a jumble of objects, and the apparent crest says it is still only a
branch. The separate forest group opens on watched slopes, giant stone gates,
and a shaded hammock clearing. Ratatoskr sorts the camp’s junk, Urd discusses
the destiny of a new hero, Heidrun grazes, and three leaf-colored forest
spirits silently watch visitors. These images suggest a Norse-myth setting,
but no local quest, objective, authored outcome, or story file connects them
into a player story. Room #30407’s color-coded description is visibly
scrambled and does not establish a readable destination narrative.

The source does not explain how the disconnected tree and forest groups are
meant to relate. Preserve their prose and current grouping until the source
owner confirms the story, the intended name, and whether a quest belongs here.
See [the WLD source](../../../areas/wld/ygg.wld#L1), [mobile source](../../../areas/mob/ygg.mob#L1),
[ZON source](../../../areas/zon/ygg.zon#L1), and the
[builder questions](YGG_BUILDER_QUESTIONS.md).

## Authored graph and active-world identity

The 13 rooms have 24 authored directions: 22 target local room numbers, one
D4 from #30400 to active SurfaceKeeps room #123522, and one D4 from #30410 to
unregistered Duris3 room #157535. The numeric local graph has two weak and
strongly connected components of ten and three rooms. From #30400, forward
traversal reaches its ten-room component; from #30410, it reaches the
three-room component. All 24 directions originate in rooms outside the
conventional numeric band for header #9. There are no negative targets,
fall-chance records, or current records.

All twelve non-camp room VNUMs #30401–#30412 also belong to active
ardgral.wld, and the paired room records differ. Forty-eight active Ard’gral
directions target those numbers; all remain Ard’gral-to-Ard’gral edges. No
active WLD exit resolves to a Yggdrasil-specific room identity, so candidate
active ingress remains zero. The unregistered ygg D0 from #30400 to #30401 is
an owner-ambiguous target: it is both Yggdrasil’s own room number and active
Ard’gral room #30401.

The inactive-WLD recount table reports two ygg source-only records under its
destination-local numeric rule. One is #30400 D4 to SurfaceKeeps #123522,
already recorded in Checkpoint 374; the SurfaceKeeps room has no exit back to
Yggdrasil. The other is #30400 D0 to #30401. That record remains an
identity-ambiguous numeric match, not confirmed Yggdrasil access. These two
records are already included in the current 11,993 overall / 415 from Surf
table; they are not new increments or confirmed player routes. The #30410 D4
to Duris3 #157535 target is unregistered, and that exact Duris3 room has no
return to #30401–#30412. It is not an approach into the active-world recount.
See [the existing source-only recount](WORLD_CONNECTIVITY.md#full-inactive-wld-source-only-approach-recount),
[SurfaceKeeps room #123522](../../../areas/wld/surfacekeeps.wld#L49693), and
[Duris3 room #157535](../../../areas/wld/Duris3.wld#L1208236).

## Duplicate title, VNUMs, and followed history

After color-code removal and whitespace normalization, “Yggdrasil Guild
Hall” occurs twice among 442 current ZON titles: ygg and circlegh. Both also
use ZON header #9. Their room ranges differ: Yggdrasil is #30400–#30412 and
circlegh is in the #45400 band. The separate rf_gh source is in the #28900
band. These title and header matches do not establish a rename, canonical
alias, or replacement relationship.

Yggdrasil WLD rooms #30401–#30412 overlap active Ard’gral room identities;
Yggdrasil #30400 has no second current WLD owner. Mobile #30401 overlaps
Ard’gral’s nightmare essence, while Yggdrasil mobiles #30400, #30402, and
#30410–#30412 have no active Ard’gral prototype. Object #30401 overlaps
Ard’gral’s mask of nightmares; object #30400 has no active duplicate. The
paired mobile and object prototypes differ. Treat these as competing
numeric identities, not evidence that one zone supersedes the other.

The title and header sources are [ygg.zon](../../../areas/zon/ygg.zon#L1)
and [circlegh.zon](../../../areas/zon/circlegh.zon#L1). The same-number
active room, mobile, and object records are in
[ardgral.wld](../../../areas/wld/ardgral.wld#L1),
[ardgral.mob](../../../areas/mob/ardgral.mob#L1), and
[ardgral.obj](../../../areas/obj/ardgral.obj#L1).

Followed WLD/MOB/OBJ/ZON/SHP histories stay on the ygg stem. The 2009 change
is the areas-tree move; the 2012 and 2013 MOB/OBJ/ZON changes are field
layout/read-write edits. The 2013 WLD and SHP changes alter file mode without
changing text. No alternate-stem rename, distinct predecessor, or whole-zone
replacement appears in the followed paths. Similar-title history for
circlegh and rf_gh is documented in
[DUPLICATE_AND_REPLACED_ZONES.md](DUPLICATE_AND_REPLACED_ZONES.md).

## Typed dependencies and bounded code references

The Yggdrasil ZON contains seven O commands: five external Player Castles
object resets (#11055 twice, #11105 twice, and #11013 once) plus local portal
objects #30400 and #30401. It contains nine M commands, including three
repeated Norn rows and two repeated green-spirit rows. Its eleven G commands
give the same eleven VNUMs listed as products in the squirrel’s SHP record.
The five Player Castles imports are source-only reset references and are not
active resets. See
[the exact reset rows](../../../areas/zon/ygg.zon#L9) and the
[Player Castles dependency review](PLAYER_CASTLES_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers).

The two local type-25 portal objects store destinations #30412 and #30407.
Those values numerically match Yggdrasil rooms and active Ard’gral rooms, so
they do not settle which room identity or route the original builder intended.
The portal descriptions name the Lush Jungles and the realm of Light,
respectively. A scan of current object prototypes found no other type-25
destination into #30400–#30412.

The eleven VNUMs repeated in the ZON G commands and SHP product list are #377,
#3050, #398, #6089, #364, #53,
#412, #347, #108, #109, and #111. Current active OBJ owners exist for ten:
#377, #398, #364, #412, #347, and #111 are in Heavens; #3050 is in Verz1;
#108 and #109 are in Heavens and Mavulsk; #53 is in Heavens, Dalvik, Hostel,
and Mavulsk. No current OBJ prototype for #6089 was found.

SHP defines two records. The #30400 keeper record has those eleven products
and names room #30408; the #30412 keeper record has no products and names
room #30410. The SHP keeper VNUMs match Yggdrasil mobiles #30400 and #30412,
neither of which is an active mobile prototype. The rooms #30408 and #30410
also overlap active Ard’gral room numbers. The checkout has no
areas/world.shp. In full mode, shop loading first opens areas/world.shp and,
if unavailable, concatenates areas/shp/*.shp; that fallback would include
ygg.shp. The loader resolves the keeper field through real_mobile, so the
current source does not establish an active Yggdrasil shopkeeper. This is a
conditional source-path finding, not a live-load result. See
[the shop input path](../../../src/world/db.h#L22),
[fallback loader](../../../src/economy/shop.c#L3267),
[keeper lookup](../../../src/economy/shop.c#L3489), and
[the SHP records](../../../areas/shp/ygg.shp#L1).

Exact current QST/story-sidecar, ZON, SHP, and type-25 scans found no active
consumer that identifies a Yggdrasil-only mobile, object, or room. Active
Ard’gral’s #30401–#30412 reset references resolve to its own WLD/MOB/OBJ
records. Compiled code does directly select random destination rooms
#30401–#30464 for cast_ardgral and assigns pool callbacks to Ard’gral
objects #30402–#30412; these bindings preserve Ard’gral identities and are
not Yggdrasil bindings. See
[the spell range](../../../src/classes/sillusionist.c#L2209) and
[object assignments](../../../src/specs/specs.assign.c#L1931).

## Disposition

Keep all five Yggdrasil files **RETAIN IN PLACE** outside areas/AREA and
areas/qst/AREA. Preserve the thirteen rooms, both graph components, all
authored exits, six mobile prototypes, two portals, seven O / nine M / eleven
G reset commands, both SHP records, and the earlier Player Castles and
SurfaceKeeps findings. Do not activate, merge, renumber, regenerate, move, or
archive these files until a map/editor owner resolves the title and source
identity, Ard’gral VNUM collisions and compiled range, the two outbound
boundaries and return intent, portal targets, imported resets, shop/keeper
and fallback behavior, deployment, runtime, and persisted identity.

No change to the active world or to the curated 11,993 overall / 415 from
Surf source-only totals is proposed. See the [28 builder questions](YGG_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-559-yggdrasil-graph-and-active-identities),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-559-yggdrasil-title-history-and-ardgral-collisions),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-559-ygg-retained-with-room-prototype-and-shop-conflicts),
[coverage row 472](ZONE_COVERAGE_REGISTER.csv#L473), and
[candidate dependency row 121](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L122).
