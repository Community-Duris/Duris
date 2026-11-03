# Crystalspyre Mountains: static connectivity and dependency audit

Checkpoint 428; physical coverage row 341, `area:vrzawild`. This supplement
reconciles the [source-story review](../zone-stories/CRYSTALSPYRE_MOUNTAINS_STORY_REVIEW.md)
with active and inactive WLD approaches, exact typed references, title and
tracked history, and the active-area disposition. It does not qualify live
access, player safety, item stock, or the temporary encounter's balance.

## Active WLD graph and directed paths

The local WLD has 100 rooms (#37000–#37099) and 281 authored exits. All 281
resolve: 280 are internal and the one outbound boundary is reciprocal with
Surface. The undirected local graph has one component spanning all rooms. Raw
directed traversal across internal exits from Surface-arrival room #37090
reaches all 100 rooms, and every room can return to #37090 in the same
all-edges-open abstraction; the graph has one SCC of size 100. These counts
ignore door/reset state, the fall fields, actor condition, reset timing, and
runtime scripts.

Three rooms on the Barrier Wall have `F 33` fields: #37065–#37067
([#37065](../../../areas/wld/vrzawild.wld#L1761),
[#37066](../../../areas/wld/vrzawild.wld#L1783),
[#37067](../../../areas/wld/vrzawild.wld#L1805)). No current fields were found.
The four hut D1 resets at #37086–#37088 initialize paired directions closed
([first reset](../../../areas/zon/vrzawild.zon#L8)); they are not locked. Raw
reachability therefore does not establish that the route is open, safe, or
returnable for a particular player.

## Active and inactive approaches

The only active boundary is Surface #523536 west ↔ Crystalspyre #37090 east
([outbound register row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L537),
[inbound register row](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1368)). An all-file
target scan across 95 inactive WLD sources found one source-only approach:
`surf.wld` #210777 north to #37090 ([source](../../../areas/wld/surf.wld#L226153)).
No inactive WLD room record overlaps #37000–#37099, and no other inactive target
to the local rooms was found. The old `surf` record is not active ingress and
does not provide a declared return from Crystalspyre to that inactive map.
Preserve it under the existing Surf candidate review. The reviewed Surf
source-only subtotal rises by one, from 122 to 123; the overall curated total
rises from 1,505 to 1,506.

## Typed dependencies and direct source references

The four-file local source inventory contains 17 mobile and 15 object
prototypes, no local QST file, no SHP file, and no story sidecar. The QST area
index lists `vrzawild` ([index row](../../../areas/qst/AREA#L166)) despite the
missing local contract file. Scans across external ZON, QST, and SHP sources
found no exact reference to local room, mobile, or object VNUMs; there is no
external same-type mobile/object prototype definition, no type-25 target to a
local room, and no structured local VNUM among 232 parsed external story
sidecars. The exact C/C++ token scan found no local VNUM reference.

There is one outbound reset dependency: local ZON line 120 gives Zikata
#37012 imported object #358, defined by `heavens.obj` and bound to the shared
`epic_stone` callback ([reset](../../../areas/zon/vrzawild.zon#L120),
[prototype](../../../areas/obj/heavens.obj#L3709),
[assignment](../../../src/specs/specs.assign.c#L1603)). Keep Heavens as the
object owner and preserve the callback binding. This outbound dependency is
different from the empty scan of external consumers of local prototypes.

## Title, history, and relationship to Verzanan

The normalized `.zon` title “Crystalspyre Mountains” is unique among the 351
active contributors. Followed WLD/MOB/OBJ/ZON history remains on the `vrzawild`
stem through the tracked 2009 `areas/` path relocation and later same-stem
maintenance; no alternate-stem predecessor, title rename, or whole-area
replacement was identified. The bounded semantic relation is explicit: local
object #37001 describes Crystalspyre as a future road to Verzanan, and mobile
#37011 is a Bloodguard of Verzanan. Current `verz1` remains a separate
full-area source with two placeholder rooms and no active WLD boundary to
Crystalspyre. This records a planned narrative relationship, not a current
route or replacement. See the [Verzanan review](../zone-stories/VERZANAN_STORY_REVIEW.md#title-history-and-replacement-boundary),
[slab message](../../../areas/obj/vrzawild.obj#L33),
[Bloodguard text](../../../areas/mob/vrzawild.mob#L156), and
[Crystalspyre title](../../../areas/zon/vrzawild.zon#L2).

Keep active `vrzawild` **DO NOT MOVE**. Its WLD boundary, local encounter
sources, and Heavens #358 reset remain under their separate owners. The
inactive Surf approach remains source-only. Builder follow-ups are in the
[twenty-question review](../zone-stories/CRYSTALSPYRE_MOUNTAINS_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).

This checkpoint used checked-in sources only. No test, build, server,
gameplay, database, migration, or operational command was run.
