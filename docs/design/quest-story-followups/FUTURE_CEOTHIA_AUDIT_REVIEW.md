# Future Ceothia: repository-wide route, dependency, and history supplement

This bounded static supplement accompanies the [comprehensive Future Ceothia
source dossier](../zone-stories/FUTURE_CEOTHIA.md). It reconciles active and
source-only WLD edges, raw directed reach, selected exact cross-area item and
mobile references, time-portal destinations, and the repeated title across
the three Ceothia phases. It records source relationships; it does not qualify
installed availability, runtime travel, item issuance, or player outcomes.

## Active WLD boundaries and directed reach

The active area is ceofutur, zone 814, with 278 rooms from #81400 through
#81677. All 1,016 authored exits resolve to local rooms. The map has weak
components of 276 and 2 rooms, no local duplicate room VNUM, no unresolved
exit, and no active WLD outbound edge.

Its sole active WLD inbound is Realm room #14210, northeast to Future Ceothia
room #81677 ([source edge](../../../areas/wld/realm.wld#L4776),
[pair row 387](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L387),
[exit row 264](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L264)). There is no reverse
WLD edge from Ceothia to Realm. From #81400, authored directed traversal
reaches 271 of 278 rooms; 275 rooms have a directed path back to the root.
Root-forward traversal misses #81671–#81677, and #81675–#81677 have no
authored return path to #81400. These are raw WLD graph results, not a claim
about door, reset, scripted, or player availability.

The two-room weak component is #81676/#81677. Three type-25 portals form
separate time routes:

| Route | Source evidence |
| --- | --- |
| Past portal #81111, loaded at room #81389, targets Future room #81400. | [Past reset](../../../areas/zon/ceopast.zon#L51), [portal target](../../../areas/obj/ceopast.obj#L179) |
| Future portal #81422, loaded at room #81670, targets local room #81676. | [Future reset](../../../areas/zon/ceofutur.zon#L37), [portal target](../../../areas/obj/ceofutur.obj#L390) |
| Future portal #81413, loaded at room #81677, targets Present Ceothia room #80980. | [Future reset](../../../areas/zon/ceofutur.zon#L41), [portal target](../../../areas/obj/ceofutur.obj#L239) |

These object destinations are transport references, not WLD exits or proof
that a player has personally used a route. The two Future portals are part of
the dossier's already documented local time-device routes.

## Inactive WLD sweep

All 95 current WLD sources outside the active areas/AREA list were parsed:
527,791 rooms and 2,080,773 authored exits, with zero parse errors. The sweep
found no source-room VNUM overlap with Future Ceothia and no inactive exit
targeting one of its rooms. There is no source-only WLD approach to add, so
the curated source-only totals remain 3,333 overall, 164 for the reviewed Surf
subset, and 15 for ud_map2. These records do not establish a migration
candidate or replace the active Ceothia map.

## Exact typed and structured references

External active QST files consume three Future Ceothia objects:

| Consumer | Exact reference | Source evidence |
| --- | --- | --- |
| Alatorin's exchange accepts object #81403, the Future black steel dagger. | G I #81403 | [Alatorin QST](../../../areas/qst/alatorin.qst#L5327), [Future object](../../../areas/obj/ceofutur.obj#L83) |
| Present Ceothia's exchange accepts object #81410, the dragon horn. | G I #81410 | [Present QST](../../../areas/qst/ceothia.qst#L201), [Future object](../../../areas/obj/ceofutur.obj#L196) |
| Present Ceothia's exchange accepts object #81423, the thread of time. | G I #81423 | [Present QST](../../../areas/qst/ceothia.qst#L218), [Future object](../../../areas/obj/ceofutur.obj#L397) |

The active structured story sidecars mirror some of those relationships.
Alatorin's sidecar maps #81403 in a contract key, an optional carried-item
row, and an accepted-exchange row
([sidecar rows](../../../areas/story/alatorin.story.json#L2683)). Present
Ceothia's sidecar names Future mobiles #81429 and #81454 as contacts and
references #81410 and #81423 in carried-item and completion-key fields
([beholder contact](../../../areas/story/ceothia.story.json#L142),
[Chronomancer contact](../../../areas/story/ceothia.story.json#L149),
[horn branch](../../../areas/story/ceothia.story.json#L335),
[thread branch](../../../areas/story/ceothia.story.json#L396)). Surface Mini's
sidecar also names mobile #81454 as a contact
([sidecar contact](../../../areas/story/surfacemini.story.json#L347)).
These fields are authored story references; a contact or sidecar item field
does not by itself establish a live reset or item transfer.

Future Ceothia's own ZON resets import external object prototypes #360 at
room #81676, #1220 for Jerome #81456, and #55238, #16262, and #900 in the
Chronomancer #81454 load group
([first imports](../../../areas/zon/ceofutur.zon#L40),
[Jerome group](../../../areas/zon/ceofutur.zon#L67),
[Chronomancer group](../../../areas/zon/ceofutur.zon#L310)).
The matching prototypes are owned by Heavens, Winterhaven, and TransTower.
No external active reset-role reference to a Future Ceothia prototype and no
SHP product reference were found; Future Ceothia has no local SHP source file.
The local QST has no exact external object command target.

The wider direct-reference scan found no duplicate external mobile or object
prototype definitions. Among 870 external type-25 declarations, the only
target into Future Ceothia is Past portal #81111 to #81400
([portal target](../../../areas/obj/ceopast.obj#L179)). Compiled source
references include Future object #81419, the mask of the future, in two CHAOS
equipment tables ([standard table](../../../src/account/chaos_eq_data.h#L310),
[enhancement table](../../../src/account/chaos_eq_data.h#L554)), and mobile
#81454 in the random-equipment high-drop list
([high-drop list](../../../src/item/randomeq.c#L118)). The numeric match for
object #900 in artifact.c is inside an example/comment block, not a live
binding ([comment context](../../../src/guild/artifact.c#L271)).

## Shared title and bounded history

The normalized .zon title Ceothia is shared by the three distinct active
phase areas: Present ceothia, Past ceopast, and Future ceofutur
([Present title](../../../areas/zon/ceothia.zon#L2),
[Past title](../../../areas/zon/ceopast.zon#L2),
[Future title](../../../areas/zon/ceofutur.zon#L2)). Preserve all three source
keys and their separate time-period contracts. This is a repeated title, not
evidence of duplicate maps or replacement.

Bounded same-stem git history for ceofutur.wld and ceofutur.zon reaches the
2009 areas/ tree move and later same-stem edits, including the 2013 WLD update
and the 2015 quest-item flag pass. The reviewed paths show no alternate-stem
rename, distinct predecessor, or whole-zone replacement. This history check
does not assert that every historical branch or lost external source has been
recovered. See [duplicate and replacement review](DUPLICATE_AND_REPLACED_ZONES.md).

## Migration disposition

Keep active ceofutur DO NOT MOVE. Preserve the Realm inbound, all three
time-portal dependencies, Alatorin and Present Ceothia QST consumers, the
three structured-sidecar relationships, external object imports, and compiled
CHAOS/random-equipment references. No inactive-WLD candidate row is created;
the existing candidate register remains unchanged. Editor and administrative
callers, installed data, persisted player inventories, indirect runtime
routes, and actual item issuance remain outside this bounded scan. No source,
gameplay, migration, or production change is proposed here.
