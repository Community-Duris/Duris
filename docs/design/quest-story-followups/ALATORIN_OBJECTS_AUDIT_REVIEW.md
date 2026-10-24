# Alatorin object bank audit review

Checkpoint 444 is a static audit of the unregistered object-only source key
component:alatorin_objects. It covers its item lore, all 408 object VNUMs,
the former areas/AREA stub, type-25 target values, active and source-only
reset/quest/shop consumers, and the competing Alatorin and Nexus56 prototypes.
It does not inspect editor databases, persisted player objects, a live object
index, or runtime travel. The [story dossier](../zone-stories/ALATORIN_OBJECTS.md)
summarizes the authored artifacts; the [builder questions](../zone-stories/ALATORIN_OBJECTS_BUILDER_QUESTIONS.md)
record owner decisions.

## Inventory and current membership

| Source | Current evidence |
| --- | --- |
| [Object bank](../../../areas/obj/alatorin_objects.obj#L1) | 408 OBJ prototypes spanning #83100–#83507. |
| WLD / MOB / ZON / QST / SHP | No same-stem file exists for alatorin_objects. |
| Map / story sidecar | No alatorin_objects editor JSON or story JSON exists. |
| Current full area list | Does not list alatorin_objects; active alatorin is listed at [line 300](../../../areas/AREA#L300). |
| Current QST area list | Does not list alatorin_objects. |

The component has no rooms, reset file, local quest source, shop, map, or zone
title. The object VNUMs are not unique to this file: active alatorin.obj
defines every one of the same 408 IDs, with active Alatorin spanning 602
objects total. The unlisted nexus56.obj also defines 168 of those IDs.

## Historical loader status and object identity

Commit 60f9f14 (2010-01-26) added alatorin_objects.obj and a one-line
areas/AREA stub saying the objects were being loaded while the zone was
reviewed, because existing players' Alatorin objects were causing crashes.
Commit 30db49c (2010-06-08) removed that stub and restored active Alatorin's
AREA entry. The object file remained in the repository and continued to
receive same-path edits through 2026. The current loader list therefore does
not establish that historical saved-object references were removed.

A normalized prototype comparison finds:

- All 408 candidate VNUMs overlap active alatorin.obj; there are no
  candidate-only IDs within the range.
- 204 records match after line-ending normalization. The other 204 differ in
  some combination of object type, flags, values, stats, affects, or text.
  Text fields differ in 49 of those records.
- Nexus56.obj overlaps 168 candidate VNUMs, and all 168 Nexus56 records differ
  from the alatorin_objects records. Nexus56 is absent from areas/AREA and
  has a separate candidate entry at [row 79](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79).

Do not infer that either object bank is a whole-zone replacement. They are
separate source sets with shared numeric IDs and distinct versions. The
active full list currently names alatorin, not alatorin_objects or nexus56.

## Story content and room targets

The OBJ prose identifies Taark Shanat as Alatorin's ruler and hero, records
the Wyrmskull Throne's four blue-dragon skulls, names Breuldin, Dumathoin,
Shanat military artifacts, and Jedd Burningbraids, and describes portals
into shadow, elemental, prison, and plane settings. There is no local WLD
graph. The 24 type-25 prototypes and all command/target pairs are tabulated
in the [story dossier](../zone-stories/ALATORIN_OBJECTS.md#type-25-targets).

The generic type-25 handler reads value[0] and resolves it with real_room;
value[1] selects the command ([ITEM_TELEPORT](../../../src/core/defines.h#L140),
[command IDs](../../../src/cmd/interp.h#L36),
[target handling](../../../src/magic/spell_travel.c#L991)). Twenty-three of
the 24 targets have the same value in active alatorin.obj; #83434 is the
exception. The object-bank copy targets #83831, the Ancient Lava Tube. The
active copy targets #83188, Atop the Mystic's Tower, and active
alatorin.zon resets #83434 in room #83823 ([reset](../../../areas/zon/alatorin.zon#L1751)).
Both destination rooms exist in active Alatorin. Object #83301 is an exact
matching type-25 copy that the active ZON resets in #83487; its target is
Astral Main #19721, Great Worm Hole ([reset](../../../areas/zon/alatorin.zon#L1622)).
The target's WLD exits lead to other Astral rooms and do not directly return
to Alatorin #83487. This is an authored object destination, not an Alatorin
WLD edge.

The full-area-listed target VNUMs resolve to current source definitions in
Alatorin, Ard'gral (#30453), Astral Main (#19721), and Ethereal Main
(#12511). Several Alatorin target IDs also have different definitions in
nexus56.wld, but Nexus56 is unlisted. Static source resolution does not
prove a running destination or a safe travel loop.

## Typed incoming uses

The object IDs have extensive dependencies through the active Alatorin
source set. These uses refer to IDs also defined in active alatorin.obj; they
do not prove that alatorin_objects.obj is currently loaded.

| Consumer source | Exact typed reference census |
| --- | --- |
| alatorin.zon | 1,021 O/G/E/P object commands covering 347 distinct IDs in this bank's range. |
| Other active ZON files | 42 O/G/E/P commands across dream (1), dreggan_wood (4), ravenloft (1), ravenloft2 (2), surfacekeeps (14), and vehicles (20). |
| alatorin.qst | 602 R-I/G-I item records covering 118 distinct IDs. |
| Other active QST files | cloister (1 record / 1 ID), surfacekeeps (10 / 9), vehicles (15 / 5). Total QST records across the four sources: 628. |
| alatorin.shp | 35 shop-product entries covering 30 IDs. |
| surfacekeeps.shp | 4 shop-product entries covering 4 IDs. |
| alatorin.story.json | Structured item_vnums/completion_key fields cover 125 distinct IDs. |
| Other active story JSON | cloister.story.json covers #83374; surfacekeeps.story.json covers 9 distinct IDs. |
| Compiled source | specs.assign.c binds object #83457 to the miners_helmet callback at [line 1297](../../../src/specs/specs.assign.c#L1297). |

The matching string #83301 in alatorin.qst is also a mobile quest-giver VNUM;
it is not an object-typed quest item reference. Keep item, mobile, room, and
shop namespaces separate when reconciling the shared integers.

The source-only nexus56.zon has 344 O/G/E/P commands using 167 VNUMs in the
overlapping range. Those IDs also have Nexus56 object definitions, all
different from this bank's records. Its reset comments and object prototypes
are source evidence for a competing object set, not proof that the
alatorin_objects prototypes are loaded. The source-only brass-old-1.zon has
two further object reset commands using two VNUMs in the range.

## Connectivity, identity, and disposition

There are no WLD rooms or direction records under this component, so the
world graph and candidate inbound-active-exit count are both empty/zero.
Do not convert the 24 type-25 targets or the other command-triggered object
targets into WLD edges. The target rooms have their own WLD records, but
their object availability, access, return routes, and runtime resolution
remain unqualified.

Keep candidate row 6 alatorin_objects **RETAIN IN PLACE**. Preserve all
408 definitions, the exact content variants, the #83434 target divergence,
the active Alatorin consumers, the source-only Nexus56 collision, and the
2010 saved-object loader history. Resolve stored-player object references
and intended prototype ownership before any separate removal, consolidation,
or migration decision. See [candidate row 6](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L6),
[coverage row 357](ZONE_COVERAGE_REGISTER.csv#L357),
[duplicate/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-444-alatorin-object-bank-history-and-overlap),
[connectivity review](WORLD_CONNECTIVITY.md#checkpoint-444-alatorin-object-bank-and-typed-travel-targets),
and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-444-alatorin-object-bank-retains-its-source).
