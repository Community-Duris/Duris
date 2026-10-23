# Mavulsk source and identity audit

Canonical key: component:mavulsk. Physical coverage row 420 / CSV line 421;
candidate dependency row 69 / CSV line 70. Reviewed October 11, 2026,
Checkpoint 507, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static source inventory,
story, WLD graph, typed-reference, identity, and bounded-history review. It
does not establish live availability, travel, combat, or contract outcomes.

## Inventory and authored content

The unlisted source component consists of
[mavulsk.wld](../../../areas/wld/mavulsk.wld),
[mavulsk.mob](../../../areas/mob/mavulsk.mob),
[mavulsk.obj](../../../areas/obj/mavulsk.obj),
[mavulsk.zon](../../../areas/zon/mavulsk.zon), and
[mavulsk.qst](../../../areas/qst/mavulsk.qst). There is no same-stem SHP file
or story sidecar. Neither [areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists the stem, and
areas/RANDOM_AREA is empty. The zone header is #2, its color-coded title
decodes to “Mavulsk — the Minotaur Settlement,” reset mode is 1, and its
room band is #1–#233. It defines 233 rooms, 98 mobiles, 110 objects, and no
native shop.

The rooms describe a settled minotaur town around a stone mansion, with
streets, homes, workrooms, a restaurant, palisade, mountain paths, mines,
cavetroll territory, and deeper demon and sea-creature areas. Thirteen QST
giver blocks contain 18 Q contract clauses. Their topics include a missing
child and returned letter, a three-stage hide-working chain, One-Eye’s lost
eye and a troll shaman, an investigation of a fire and troll threat, the
Baron’s missing miners, and smaller exchanges and conversations. The QST
also includes an emissary giver with dialogue but no Q clause. Its 18 R and
21 G item references all fall within Mavulsk’s local object definitions
#1–#110. No QST/AREA entry indexes these contracts.

## WLD graph and unresolved map boundaries

The WLD contains 518 authored directions with no malformed target records,
duplicate direction numbers, zero targets, or negative targets. The target
of 517 directions lies within Mavulsk’s own #1–#233 room set. The remaining
direction is room #188 east to #228202. The room graph has four weak
components of 228, 3, 1, and 1 rooms; raw directed traversal from #1 reaches
200 of 233 rooms. Rooms #127 and #128 each carry an F fall value of 50; no
current fields were found. These counts describe WLD directions only.

Current full-area room lookup finds an active same-number destination for 94
of the 518 target records; the other 424 are positive unresolved records in
that active table. The 94 numeric hits do not establish Mavulsk travel:
Mavulsk itself is unlisted, and its same-number room records differ from the
active owners. For example, Mavulsk #2 and #3 both target its own room #1,
“Before the Stone Mansion”; the active room #1 belongs to Limbo. Preserve
these as local Mavulsk references when describing the candidate graph.

The sole source target outside Mavulsk’s room band is #188 east to #228202
([source exit](../../../areas/wld/mavulsk.wld#L3856)). That number is
defined in both unlisted surf.wld and surface2011.wld. Their #228202 titles
differ (“The Great Sea of Silvanos” and a stormy sea), though the four
authored directions in those room blocks target the same neighboring
numbers. Neither variant is in areas/AREA, and neither #228202 block has a
WLD direction back to Mavulsk #188. Keep the target owner and any return
route unresolved; this is not an active Surface boundary.

Six local ITEM_TELEPORT objects provide additional source-level destinations
that the WLD-only graph does not represent. Their value[0] targets are #154,
#153, #176, #153, #226, and #225 for objects #79, #82, #83, #88, #92, and
#95 respectively. The ZON places those objects in rooms #153, #167, #179,
#178, #225, and #226. All six destinations exist in Mavulsk.wld. In
particular, #83 points into the three-room #176–#178 WLD component, and
#92/#95 connect rooms #225 and #226. The source-only fields do not establish
that a player can obtain or use these objects.

### Thetis target #104

One active Thetis exit, #38095 north → #104, remains unresolved in the
current loader and numerically targets this candidate. Mavulsk #104 is “A
Quiet Library”; inactive Dalvik #104 is the Great Temple of Ubtao. Neither
source owns a loaded destination. Keep Thetis #38095 unresolved until its
author or source history identifies the intended map.

## Resets, prototype references, and code note

The 861-line, source-only [mavulsk.zon](../../../areas/zon/mavulsk.zon)
contains 484 command records: 236 M, 32 O, 69 G, 16 E, 7 F, 2 P, and 122 D.
The room, mobile, and object VNUM slots examined in those records stay within
the Mavulsk WLD/MOB/OBJ ranges. This reset program is not included in the
current full-area list.

The six ITEM_TELEPORT value[0] destinations above are local room VNUMs.
Object records #79, #82, #83, #88, #92, and #95 are each referenced by the
source-only ZON. The existing
[teleport handler](../../../src/magic/spell_travel.c#L991) treats
ITEM_TELEPORT value[0] as its room destination; this inventory does not test
the handler or the objects in play.

Maintained source
[plushit.c](../../../src/item/plushit.c#L40) explicitly names
mavulsk.mob #48, the Captain, among the five source mobiles carrying a
PLUSONE-through-PLUSFIVE requirement. That comment identifies prototype
data; Mavulsk remains outside areas/AREA and the comment does not establish
an active Captain instance or a separate executable VNUM binding.

## Same-type identities, history, and disposition

Every Mavulsk room VNUM #1–#233, mobile VNUM #1–#98, and object VNUM #1–#110
also appears in at least one other same-type source file. Current active
same-number owners exist for 48 room IDs, 97 mobile IDs, and all 110 object
IDs. Each complete Mavulsk prototype block at those active-overlap IDs
differs from the current active definition. Low numeric matches must not be
used to assign Mavulsk’s rooms, mobs, objects, or internal reset targets to
Limbo, Heavens, or another map.

The Thetis #104 question is one example of that wider identity issue:
Mavulsk’s quiet library and Dalvik’s temple are distinct inactive room
records at the same number. Existing object reviews also record Mavulsk’s
same-number items, including #53, #108, and #109, alongside other sources.
Those relationships do not establish a shared canonical owner or a
replacement.

Followed WLD, MOB, OBJ, ZON, and QST histories show same-stem edits: a 2012
content update, 2026 line-ending and ANSI cleanup, and a 2015 object quest
item-flag pass. The reviewed history shows no alternate-stem rename,
predecessor, retirement, or whole-area replacement decision. It does not
identify the original builder or any external editor/deployment copy.

**Disposition: RETAIN IN PLACE.** Preserve all five files outside both area
lists. Resolve the low-VNUM owner conflicts, Thetis #104, the #228202 Surf
target, the six teleport fields, QST index/ownership, reset policy, shop
intent, and editor/deployment copies with the relevant builders before any
activation, renumbering, archive decision, merge, or path change. No
migration clearance is implied. See the
[25 builder questions](MAVULSK_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-507-mavulsk-local-identities-and-the-unresolved-surf-target),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-507-mavulsk-vnum-overlap-and-boundary-identity),
[retention supplement](ATTIC_MIGRATION_PLAN.md#checkpoint-507-mavulsk-retained-pending-vnum-and-map-owner-decisions),
[candidate dependency row 69](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L70), and
[coverage row 420](ZONE_COVERAGE_REGISTER.csv#L421).

### Checkpoint 918: Mavulsk typed cross-source role closure

The CP507 WLD inventory and graph are unchanged: 233 rooms, 518 directions,
one Mavulsk source exit to unlisted Surf #228202, and one active WLD inbound
record, Thetis #38095 north → #104. The following reset, quest, shop, story,
and object-value roles are separate from those WLD edge counts.

The active ZON pass found ten reset mobile arguments in the Mavulsk #1–#98
number band. Nine are local to their own same-stem MOB source. The exception
is [Heavens ZON line 286](../../../areas/zon/heavens.zon#L286),
`M 0 2 1 1212 100 0 0 0`: Heavens has no mobile #2, the current active same-number
definition is Limbo #2, while unlisted Mavulsk #2 and Mentiri #2 are separate
prototypes. No
active F or R command targets this mobile range. Eighty-three active ZON room
arguments fall in Mavulsk’s #1–#233 band, and each has a same-stem WLD room;
they do not reset into the unlisted Mavulsk map.

Active ZON object commands contain 265 primary object arguments in #1–#110
(121 O, 117 G, and 27 P rows). In 208 rows, the zone stem has no same-stem
OBJ definition for that number. Twelve additional P container arguments in
the candidate band have same-stem OBJ definitions. The active source tree has
same-number OBJ definitions for all 110 candidate values, while Mavulsk’s
objects remain outside the active lists. These low-number reset slots do not
identify the Mavulsk prototypes. The command parser treats R as a mobile mount
role and P’s third argument as its container object; the counts above separate
primary object arguments from the 12 container arguments.

Across QST sources, 23 giver-key matches fall in Mavulsk’s mobile range: 13
Mavulsk, four Hostel, two Dalvik, and four active Heavens. The four active
Heavens givers (#17, #46, #60, and #78) each have same-stem Heavens MOB
definitions. Active QST files also contain 41 R/G item-reference lines in
Mavulsk’s object-number band; unlisted sources contain 52 such lines. These
are numeric item references and do not select Mavulsk’s unlisted objects.
The parser-valid N-format SHP scan found 204 stock-number matches, no keeper
VNUM match in #1–#98, and one Heavens shop-room value of #23. Mavulsk has no
same-stem SHP. The area-story JSON scan found 16 `item_vnums` matches
(Alatorin 12, Shipy 2, SurfaceKeeps 1, and Tharnadia 1), with no candidate
room, giver, mobile, or object-owner field match.

The maintained
[ITEM_TELEPORT handler](../../../src/magic/spell_travel.c#L991) reads
`value[0]` as a room VNUM and resolves it with `real_room()`. Seventeen
active-source type-25 object fields point to six VNUMs that have no active WLD
definition, though each is also defined by Mavulsk and at least one other
unlisted WLD:

| Target | Active source object fields |
| --- | --- |
| #2 | [githzer.obj](../../../areas/obj/githzer.obj) #44488, #44493, #44573 |
| #9 | [crypt.obj](../../../areas/obj/crypt.obj) #14547 |
| #50 | [fields_between.obj](../../../areas/obj/fields_between.obj) #71002; [halfcut.obj](../../../areas/obj/halfcut.obj) #27035 |
| #100 | [eth2.obj](../../../areas/obj/eth2.obj) #32622; [fortb.obj](../../../areas/obj/fortb.obj) #38143; [negplane.obj](../../../areas/obj/negplane.obj) #26636, #26640, #26641; [seveno.obj](../../../areas/obj/seveno.obj) #38618; [teka2.obj](../../../areas/obj/teka2.obj) #75556 |
| #150 | [jin.obj](../../../areas/obj/jin.obj) #82019 |
| #200 | [arcium.obj](../../../areas/obj/arcium.obj) #85700; [mir.obj](../../../areas/obj/mir.obj) #41925, #41926 |

These source fields do not establish a usable route or identify Mavulsk as
their intended owner. They remain outside the WLD graph and active-inbound
metric. A bounded literal-call scan found 130 `real_room`, `real_mobile`, or
`real_object` calls in these low-number bands, but the calls do not name a
Mavulsk source identity. The only map-specific code mention remains the
`plushit.c` comment for mavulsk.mob #48; it is prototype commentary, not
evidence of a loaded mobile or a VNUM binding.

No rename, predecessor, or whole-area replacement evidence was added.
**Disposition remains RETAIN IN PLACE.** Preserve the five source files,
active Limbo #2 identity, unresolved Thetis #104 and Surf #228202 boundaries,
all six local Mavulsk ITEM_TELEPORT fields, and the external low-number role
matches until their owners resolve intended identities. See [MV-14](MAVULSK_BUILDER_QUESTIONS.md),
[connectivity](WORLD_CONNECTIVITY.md#checkpoint-918-mavulsk-object-destinations-do-not-change-wld-metrics),
[identity/history](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-918-mavulsk-role-matches-do-not-establish-identity-or-replacement),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-918-mavulsk-typed-cross-source-references-remain-under-retention),
[coverage row 420](ZONE_COVERAGE_REGISTER.csv#L421), and
[dependency row 69](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L70).
