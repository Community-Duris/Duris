# The Shadow Forest: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Shadow Forest
source dossier](../zone-stories/SHADOW_FOREST.md). It reconciles active and
source-only WLD edges, typed QST/story/reset dependencies, compiled prototype
references, title ownership, and bounded same-stem history. Static source
review does not establish runtime access or gameplay outcomes.

## Active WLD boundaries and directed reach

Active mist has 73 rooms from #6300 through #6372 and 199/199 resolving
exits: 197 local and two across area boundaries. Its local graph is one weak
component. Raw directed traversal from #6300 reaches 67/73 rooms; 72/73 have
a path back to #6300. Rooms #6367–#6372 are outside root-forward reach, and
#6371 has no authored return path to #6300. No internal exit has flag 8. These
figures describe authored WLD edges, not door, reset, script, or runtime
admission.

Four active inbound records come from three active areas:

| Source | Active edge | Reverse record |
| --- | --- | --- |
| Minizones | #5804 north to #6368 | No reverse exit from mist; [pair row 271](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L271) |
| NewFor | #53300 south to #6366 | No reverse exit from mist; [pair row 316](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L316) |
| Surface | #628063 west to #6300 | Reciprocal with mist #6300 east; [pair rows 281 and 545](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L281) |
| Surface | #628462 north to #6300 | Reciprocal with mist #6300 south; [pair rows 281 and 545](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L545) |

The active edge records are also listed in the [connectivity register](WORLD_CONNECTIVITY.md#priority-204--checkpoint-164--the-shadow-forest).

## Inactive WLD sweep

All 95 WLD files outside the active areas/AREA list parsed cleanly: 527,791
rooms and 2,080,773 exits. No inactive source room overlaps a mist room. Three
non-overlapping source-only exits target mist room #6368:

| Source record | Source-only exit | Evidence |
| --- | --- | --- |
| alterian_wilderness.wld #4801 | south to #6368 | [Source room](../../../areas/wld/alterian_wilderness.wld#L215) |
| alterian_wilderness.wld #4918 | south to #6368 | [Source room](../../../areas/wld/alterian_wilderness.wld#L3262) |
| aopal.wld #43301 | west to #6368 | [Source room](../../../areas/wld/aopal.wld#L1) |

These records raise curated source-only totals to 3,349 overall; Surf remains
169 and ud_map2 remains 15. They are source-only, not active ingress. Keep
alterian_wilderness and aopal RETAIN IN PLACE with their existing candidate
rows [8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8) and
[10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10).

## Exact typed, structured, and reset dependencies

The manifest-active external QST scan finds no item command targeting any of
mist's seven object prototypes. The active and unregistered SHP product scan
also finds no product entry to a mist object; no external ZON reset-role
reference targets a mist room, mobile, or object. No typed item_vnums field in
the 231 other story sidecars points to a mist object.

Three raw story-token matches are string IDs, not typed prototype fields:
Alatorin story ID 6346 at [line 368](../../../areas/story/alatorin.story.json#L368),
and White Harbor IDs 6349 and 6329 at [lines
1021](../../../areas/story/wh.story.json#L1021) and
[1085](../../../areas/story/wh.story.json#L1085). All 231 scanned story
sidecars parsed without errors.

A broad cross-namespace number scan also reports tharnadia1.shp line 561,
product value #6358. The product field is an object reference, but #6358 is a
mist room VNUM, not a mist object VNUM: mist's object prototypes are #6300–#6306
([object list](../../../areas/obj/mist.obj#L1); [room #6358](../../../areas/wld/mist.wld#L1537)).
The typed SHP product scan confirms no dependency on a mist object. Preserve
the unregistered tharnadia1 candidate's other recorded dependencies without
counting this namespace collision as a mist item reference.

Mist's local QST item commands, ZON reset roles, and structured story
item_vnums contain no reference outside its local object set. Mist has no
local SHP source.

## Compiled references and external prototype checks

The compiled assignment binds mist mobile #6307 to mist_protect at
[specs.assign.c line 2222](../../../src/specs/specs.assign.c#L2222). The
procedure references room #6300 through real_room and checks the northward
command at [specs.mist.c line 37](../../../src/specs/specs.mist.c#L37). It
returns early for a trusted guardian; otherwise, when the guardian is at
#6300, it blocks northward movement for a non-trusted player above level 36.
This describes the compiled source condition, not a claim that a player met
it in a running game.

Other raw compiled-number hits are not prototype references: #6310, #6302,
and #6308 occur inside floating-point values in [memorize.c lines
78–82](../../../src/classes/memorize.c#L78), while #6351 is part of a SHA
round constant in [player_save_journal.c line
234](../../../src/player/player_save_journal.c#L234). The bounded scan found
no other direct compiled prototype reference to mist.

No external duplicate mobile or object prototype was found. Across 872
external type-25 declarations, none targets mist.

## Title and bounded history

The normalized title “The Shadow Forest” belongs only to active stem mist
among 442 scanned .zon files ([title source](../../../areas/zon/mist.zon#L2)).
Followed mist.wld history includes the 2009 areas/ move and same-stem 2011
and 2013 edits; mist.zon history includes the 2009 move and 2015 quest-item
flag update. The reviewed paths establish no alternate-stem rename, distinct
predecessor, or whole-zone replacement. See the [duplicate and replacement
review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active mist DO NOT MOVE. Preserve the two reciprocal Surface paths, the
one-way active Minizones and NewFor inbounds, and all three source-only
approaches. Preserve the existing Alterian Wilderness and Aopal candidate
rows. No new candidate row is added; the tharnadia1 SHP number match is a
room/object namespace collision. Editor, administrative, installed-data,
persisted-player, indirect-runtime, and actual traversal closure remain
outside this bounded scan; no gameplay, migration, or source change is
proposed.


## Checkpoint 282: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should Palon continue accepting exact loose head #6305 for crown #6306 and departure without a remembered kill, carve, first recovery, or greeting? | [Native Q6](../../../areas/qst/mist.qst#L6), [journal contract](../../../areas/story/mist.story.json#L86) | Preserve the supplied-item contract; track personal source history only as an optional separate story. |
| 2 | Should Darnac's carried head remain valid while he is alive, distinct from a corpse, generated death trophy, or player-carved item? | [Darnac reset and head](../../../areas/zon/mist.zon#L66), [head prototype](../../../areas/obj/mist.obj#L57), [source boundary](../zone-stories/SHADOW_FOREST.md#native-acceptance-source-and-departure) | Keep the native exact item identity and do not make death or carving a new prerequisite. |
| 3 | Should the one cap-one Darnac source and single native daily candidate remain separate from guaranteed daily supply or renewal? | [Zone header and resets](../../../areas/zon/mist.zon#L3), [native contract](../../../areas/qst/mist.qst#L6) | Keep daily policy disabled until active READY source creation, encounter access, reward/save/replay, and renewal are qualified. |
| 4 | Should `darnac` remain the only addressed Palon topic, with repeated dialogue kept separate from source discovery and completion? | [M2 topic](../../../areas/qst/mist.qst#L2), [dialogue review](../../reference/zone-story-audits/mist.md#dialogue-responses) | Preserve the single native alias unless an intentional compatibility change is selected; don't create extra achievements from repeated speech. |
| 5 | Should Darnac's current source placement at #6370 stay optional provenance rather than a fixed personal route requirement? | [Darnac placement](../../../areas/zon/mist.zon#L66), [ordinary reach and source review](../zone-stories/SHADOW_FOREST.md#installed-guardian-and-actual-movement) | Qualify actual roaming and admitted encounters; keep supplied heads eligible and distinguish current stock room from guaranteed access. |
| 6 | Should Palon's `D` clause continue to mean acknowledged departure without claiming mounted travel or relocation to another room? | [Native completion](../../../areas/qst/mist.qst#L6), [Palon placement](../../../areas/zon/mist.zon#L69), [mount boundary](../zone-stories/SHADOW_FOREST.md#native-acceptance-source-and-departure) | Record actor removal and reward as separate actual results; add a destination or mount story only with an authored movement source and endpoint. |
| 7 | Should crown #6306 issuance remain the native reward, separate from later possession, wear, or any claimed light effect? | [Crown prototype](../../../areas/obj/mist.obj#L65), [reward contract](../../../areas/story/mist.story.json#L86) | Preserve the exact reward and restrictions; don't infer an ability from its name or create extra credit from equipment use. |
| 8 | Should `mist_protect` remain a north-only blocker at room #6300 for a present guardian and non-trusted player above level 36? | [Procedure](../../../src/specs/specs.mist.c#L37), [literal binding](../../../src/specs/specs.assign.c#L2222) | Preserve the exact room, direction, actor, and trust conditions; don't apply a zone-wide level gate. |
| 9 | Should the Surface, Minizones, and NewFor approaches remain distinct routes with only the two Surface exits reciprocal? | [Current route inventory](SHADOW_FOREST_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach), [world source](../../../areas/wld/mist.wld) | Track actual entry and return independently; keep the one-way Minizones/NewFor edges explicit without inventing a reverse exit. |
| 10 | Should #6371 remain a no-exit staging room even though its title says “Trap Room”? | [Staging-room review](../zone-stories/SHADOW_FOREST.md#installed-guardian-and-actual-movement), [room source](../../../areas/wld/mist.wld) | Treat the title as prose until an actual trap, damage, or movement controller is authored; do not award trap or death outcomes from a visit. |
| 11 | Should unreachable-from-root Palon/Darnac staging and roaming remain as authored, or should their ordinary access paths be changed? | [Directed reach and stock placement](../zone-stories/SHADOW_FOREST.md#installed-guardian-and-actual-movement), [local graph](SHADOW_FOREST_AUDIT_REVIEW.md#active-wld-boundaries-and-directed-reach) | Qualify all ordinary and dynamic entry paths, population, and return before proposing a balanced route change. |
| 12 | Should Grolen's and the treant's forest-protection lore remain separate from any accepted stewardship or rescue achievement? | [Progression story map](../zone-stories/SHADOW_FOREST.md#progression-stories), [native placements](../../../areas/zon/mist.zon) | Keep lore and carried/equipped stock as context; define a real contribution, beneficiary, and saved woodland result before adding credit. |
| 13 | Should existing spider, swarm, and treant stock remain environmental population rather than player ecology contribution? | [Reset groups](../../../areas/zon/mist.zon#L14), [ecology boundary](../zone-stories/SHADOW_FOREST.md#progression-stories) | Credit player intervention only from a defined admitted action and lasting state change; a kill, visit, or reset count is insufficient. |
| 14 | Should traveler dialogue about a rose, clothing, or home remain background until a real delivery, guidance, or survival endpoint is selected? | [Traveler mobile](../../../areas/mob/mist.mob#L116), [story limits](../zone-stories/SHADOW_FOREST.md#progression-stories) | Do not invent an item stock, escort, or rescue contract from prose; define exact actor, consent, destination, and persistence first. |
| 15 | Should Grolen's necklace/mask, the treant's boots, and Darnac's stiletto remain equipment sources without new proc or forest-resolution credit? | [Local mobiles and equipment](../../../areas/mob/mist.mob), [local object records](../../../areas/obj/mist.obj), [equipment boundary](../zone-stories/SHADOW_FOREST.md#installed-guardian-and-actual-movement) | Preserve actual reset/equipment roles; qualify any intended use effect and recipient before adding an objective. |
| 16 | Should `NO_TELEPORT` and `NO_SUMMON` remain room controls even where prose or ordinary exits suggest other travel? | [Control and travel review](../zone-stories/SHADOW_FOREST.md#exhaustive-local-and-bounded-source-review), [world controls](../../../areas/wld/mist.wld) | Use the actual flags and successful movement result; don't infer travel, safety, or a pet arrival from text alone. |
| 17 | Should sector-zero cottage and fort rooms remain without an added `INDOORS` flag, and should the forest prose remain separate from `DARK` state? | [Room-flag findings](../zone-stories/SHADOW_FOREST.md#exhaustive-local-and-bounded-source-review), [world controls](../../../areas/wld/mist.wld) | Preserve current fields pending builder intent; name, shelter description, or forest mood does not establish a room-control flag. |
| 18 | Should the two Alterian Wilderness and one Aopal approaches remain source-only on candidate rows 8 and 10? | [Three inactive-WLD approaches](SHADOW_FOREST_AUDIT_REVIEW.md#inactive-wld-sweep), [candidate row 8](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L8), [row 10](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L10) | Retain the records with their current owners and never describe them as active ingress. |
| 19 | Should the normalized title, active stem, and existing no-move disposition remain “The Shadow Forest” / `mist` pending any separate authored rename? | [Title and bounded history](SHADOW_FOREST_AUDIT_REVIEW.md#title-and-bounded-history), [active manifest](../../../areas/AREA#L63) | Keep active `mist` DO NOT MOVE; no predecessor or replacement is established by the bounded history. |
| 20 | Should wider guardian, investigation, stewardship, guidance, and equipment stories require explicit active READY events without adding hidden gates to Palon's native exchange? | [Story integration boundary](../zone-stories/SHADOW_FOREST.md#implementation-and-validation-boundary), [native contract](../../../areas/qst/mist.qst#L6) | Keep the native supplied exchange independent and disabled daily policy unchanged; give each new story its own typed source, beneficiary, persistence, replay, and blocked-state rules. |
