# Quintaragon Castle repository-wide audit supplement

This supplement reconciles the active Castle WLD, reciprocal Minizones boundary, inactive-map scan, exact imported reset objects, compiled handlers, and bounded title/history evidence with the [source-comprehensive Castle dossier](../zone-stories/CASTLE.md). The frozen source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed.

## Active graph and boundary

The active [Castle WLD](../../../areas/wld/castle.wld#L1) has 130 rooms in the range #2401–#2532, except #2526 and #2531, and 392 exits; all 392 resolve. Of these, 391 are local and one crosses areas. The local exit graph has one weak component of 130 rooms. From entrance #2401, raw directed traversal reaches 123 rooms, while each room has an authored directed route back to #2401. The seven rooms outside root reach are #2472, #2525, #2527, #2528, #2529, #2530, and #2532. The dossier separately documents these editorial/private rooms and the assumptions for gated routes; this raw count does not qualify live access.

Castle has one reciprocal Minizones boundary: entrance #2401 south → #5760, paired with Minizones #5760 north → #2401 ([Castle exit](../../../areas/wld/castle.wld#L13), [Minizones exit](../../../areas/wld/minizones.wld#L1711), [outbound pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L68), [inbound pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L270)). There are no unresolved or negative WLD targets, no active inbound-only edge, and no other active cross-area boundary.

The WLD census records one authored fall-chance field and no currents. The dossier traces F25 in room #2463: because that room has no downward exit, the room loader clears the chance. The sky has 26 no-ground rooms with downward topology, but authored routes do not establish flight, safe arrival, or survival.

## Inactive-map sweep

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parser errors. It found no inactive source-room VNUM overlap with Castle and no inactive exit target into Castle rooms #2401–#2532. No source-only approach is added. The curated source-only total remains 2,796 and the reviewed Surf subset remains 156. This result is static map evidence, not proof that every other transport or runtime route is absent.

## Typed imports and compiled bindings

Castle’s two external ZON imports are carried by Llamanby, mobile #2435: imported object #359 and object #55417 ([reset records](../../../areas/zon/castle.zon#L411)). Object #359 is the Heavens rune-covered epic stone ([definition](../../../areas/obj/heavens.obj#L3725)) and has the explicit `epic_stone` object binding ([assignment](../../../src/specs/specs.assign.c#L1604)). Object #55417 is the Winterhaven memory of Quintaragon Castle ([definition](../../../areas/obj/wh.obj#L4938)); its separate QST exchange remains owned by Winterhaven as documented in the Castle dossier.

Gilman mobile #2428 is an active compiled epic teacher. The teacher table maps mobile VNUM #2428 to `SKILL_INFUSE_LIFE` ([teacher row](../../../src/classes/epic_skills.c#L188)), and epic initialization assigns the `epic_teacher` handler from that table ([initialization](../../../src/classes/epic_skills.c#L1368)). Castle also defines object #2428, but that is a separate object prototype with the same numeric value ([mobile](../../../areas/mob/castle.mob#L373), [object](../../../areas/obj/castle.obj#L409)); the teacher-table key is a mobile VNUM. The dossier covers eligibility, costs, active-accounting refusal, and why a source binding does not prove a successful purchase.

The bounded typed scan found no further outside QST item command targeting a Castle prototype, no external SHP product, no local shop, no outside story-sidecar VNUM dependency, and no external mobile/object definition overlapping Castle prototypes. None of the 869 outside type-25 declarations targets a Castle room. The 44 numeric sidecar matches are all occurrences of scalar/prose token `2500`, including completion-key strings; other QST/story `2500` hits are scalar XP/currency, prose, or completion-key values. None is a typed VNUM dependency. The exact numeric matches are retained as scan output, not converted into ownership claims.

## Title, source history, and disposition

After color removal and whitespace/case normalization, “Quintaragon Castle” has one exact owner among 351 active `.zon` contributors ([title source](../../../areas/zon/castle.zon#L2)). Similar titles such as Apocalypse Castle, Bastine Castle, and Icecrag Castle are different titles. Followed Castle WLD, MOB, OBJ, ZON, and QST paths show the 2009 area-tree relocation and later same-stem maintenance; the tracked ZON/MOB/OBJ quest-item-flag pass is from 2015. These bounded histories show no alternate-stem rename, explicit predecessor, or whole-zone replacement.

`castle` remains a current un-commented member of both [the world list](../../../areas/AREA#L12) and [the QST list](../../../areas/qst/AREA#L12). Keep the active source **DO NOT MOVE**. No source repair, placement change, playable result, or archive clearance is claimed. See the [coverage row](ZONE_COVERAGE_REGISTER.csv#L46), [connectivity checkpoint](WORLD_CONNECTIVITY.md), [history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md), and [attic plan](ATTIC_MIGRATION_PLAN.md).
