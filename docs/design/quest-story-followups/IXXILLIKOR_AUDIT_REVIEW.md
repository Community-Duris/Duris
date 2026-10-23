# Ixxillikor world, history, and dependency audit

Priority 140, source area `ixxillikor`, zone 42. This checkpoint supplements the
[source-comprehensive Ixxillikor dossier](../zone-stories/IXXILLIKOR.md), which
contains the story map and thirteen builder follow-ups. It reconciles the
active and inactive WLD graph, tracked title history, and bounded typed
dependencies. The findings are source evidence, not played access,
availability, or safe-return results.

## Active graph and reciprocal boundaries

The current [area manifest](../../../areas/AREA#L22) includes Ixxillikor.
The active scan parsed 351 WLD files and 253,262 rooms. Ixxillikor has 199
rooms and 456 exits: 452 local and four outbound. All 456 targets resolve in
the active world, the local undirected graph has one weak component of 199
rooms, and the census records one fall-chance marker and no current markers.
Each boundary has an authored return:

| Boundary | Ixxillikor exit | Reciprocal exit | Cross-area register |
| --- | --- | --- | --- |
| Underdark cavern | #4200 north to #841353 ([local WLD](../../../areas/wld/ixxillikor.wld#L15)) | #841353 south to #4200 ([Underdark WLD](../../../areas/wld/underdark.wld#L253926)) | [#55](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L55), [#1771](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1771) |
| Underdark cavern | #4200 west to #841752 ([local WLD](../../../areas/wld/ixxillikor.wld#L23)) | #841752 east to #4200 ([Underdark WLD](../../../areas/wld/underdark.wld#L254691)) | [#56](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L56), [#1773](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1773) |
| Cave of Thick Moss | #4380 down to #6426 ([local WLD](../../../areas/wld/ixxillikor.wld#L4030)) | #6426 up to #4380 ([Bugger WLD](../../../areas/wld/bugger.wld#L505)) | [#57](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L57), [#116](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L116) |
| Huge Slimy Cavern | #4398 up to #4441 ([local WLD](../../../areas/wld/ixxillikor.wld#L4384)) | #4441 down to #4398 ([Underworld WLD](../../../areas/wld/underworld.wld#L1313)) | [#58](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L58), [#64](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L64) |

These authored reciprocal exits establish static adjacency only. They do not
verify door state, ordinary-player admission, movement success, hazards, or
safe return in a running world.

## Local directed reachability

The local graph has one weak component and five strongly connected
components: one core of 195 rooms and four single-room components (#4332–#4335).
Each of the three unique active inbound arrival rooms (#4200, #4380, and #4398)
reaches the same 195-room core through authored local exits. All 199 rooms,
including the four singleton rooms, have authored paths back to each of those
three arrivals. From the active arrival rooms, #4332–#4335 are outside
forward-reachable WLD topology. The dossier identifies #4332 as the elder
brain/follower load room and #4333–#4335 as adjacent NO_MOB rooms leading to
the lore chamber; their reset and mobile behavior is separate from this graph.

This graph omits door discovery/state, reset execution, actor movement,
compiled or item travel, and live admission. Its static return paths do not
qualify safe return or prove that players can reach the four isolated rooms.

## Inactive WLD sweep

All 95 WLD files outside the active manifest parsed: 527,791 rooms and
2,080,773 exits, with no parse issues or source-room VNUM overlap. None
contains an exit targeting Ixxillikor. There is no newly identified
source-only approach, so the reviewed Surf subset and global curated
source-only totals do not change.

## Typed dependency closure

The bounded exact scans found no outside active or unregistered QST item
command, SHP product, ZON reset-role row, or typed sidecar `item_vnums` entry
consuming an Ixxillikor object. All 231 external story sidecars parsed without
errors. No external MOB or OBJ file duplicates one of the 28 local prototypes,
and none of 872 external type-25 portal declarations targets an Ixxillikor
room. Raw numeric strings in outside sidecars are identifiers or completion-key
text, not typed prototype references. This does not cover editor data or
user-controlled persisted references.

The selected compiled references are effective content bindings:
[epic_skills](../../../src/classes/epic_skills.c#L192) maps elder brain #4208
to Spatial Focus and [line 210](../../../src/classes/epic_skills.c#L210) maps
Ezallixxel #4203 to Epic Power. [epic_initialization](../../../src/world/epic.c#L1368)
applies the table through `epic_teacher`; normal non-mini startup calls it from
[comm](../../../src/net/comm.c#L1035). The existing dossier qualifies the
active-accounting refusal and the limits of this service. Local crystal #4222
also appears in two [chaos equipment rows](../../../src/account/chaos_eq_data.h#L246)
([second row](../../../src/account/chaos_eq_data.h#L490)); that equipment use
does not make the crystal unique evidence of defeating the elder brain.

Ixxillikor's own reset file imports external prototypes: object #3097 from
active Verz1 at [reset line 131](../../../areas/zon/ixxillikor.zon#L131),
defined in [verz1.obj](../../../areas/obj/verz1.obj#L1199), and mobile #96433 in
two M rows at [lines 249–250](../../../areas/zon/ixxillikor.zon#L249), defined
in active [ixarkon.mob](../../../areas/mob/ixarkon.mob#L535). These are authored
reset references, not a claim that a reset ran or that either prototype is
currently loaded in a live instance.

The local [scroll QST record](../../../areas/qst/ixxillikor.qst#L14) and
[story-sidecar material steps](../../../areas/story/ixxillikor.story.json#L132)
preserve the other direction of dependency: the legacy scroll exchange accepts
#32490, #26614, and #402 and returns #408. The two [auction reward rows](../../../areas/qst/ixxillikor.qst#L45)
([second row](../../../areas/qst/ixxillikor.qst#L50)) offer #35708, for which
the active OBJ catalog has no definition. The source dossier covers the
unresolved producer, reward, payment, and availability questions; these source
references do not add outside consumers or authorize a native repair.

## Title and tracked history

The color-stripped active `.ZON` title [“Ixxillikor”](../../../areas/zon/ixxillikor.zon#L2)
has one owner among 351 current non-empty active titles. The followed WLD path
begins with commit `642da5400` (2009-08-12), a repository-wide `areas/` path
move, and includes the 2011-10-15 edit `677a9f877`. The reviewed path history
and manifest identify no different-stem predecessor, explicit replacement, or
superseded source. This is a bounded history result, not a claim about
untracked or external history.

## Builder and disposition boundary

The existing dossier now presents thirteen explicit builder questions and
recommended resolutions covering story, source, availability, services,
loading, access, and ownership. Its four reciprocal exits and one weak local
component do not settle door behavior or prove a safe route. Ixxillikor remains
**DO NOT MOVE** because it is an active, un-commented `areas/AREA` contributor.
No candidate-specific attic plan entry is needed; no zone, area file, or
gameplay behavior changed in this documentation checkpoint.
