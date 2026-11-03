# surfacekeeps_objects object-source audit

Canonical key: component:surfacekeeps_objects. Coverage data row 450 / CSV line 451; candidate dependency data row 99 / CSV line 100. Reviewed October 11, 2026, Checkpoint 537, against checkout c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static object-prototype, typed-consumer, code-binding, and followed-history review. It does not establish a current stub-loader invocation or decide which owner maintains the duplicate records.

## Source inventory and active identity

The only tracked file is [surfacekeeps_objects.obj](../../../areas/obj/surfacekeeps_objects.obj#L1). It contains 54 object prototypes, #120000–#120053, all with same-number definitions in the active [surfacekeeps.obj](../../../areas/obj/surfacekeeps.obj#L1), whose range continues through #120074. The candidate is absent from areas/AREA and has no WLD, MOB, ZON, QST, SHP, or story sidecar. It has no zone title or room graph.

Line-by-line comparison of each full object body after its VNUM header finds 39 identical bodies and 15 different ones: #120006, #120010, #120014, #120016, #120029, #120035, #120037, #120038, #120039, #120042, #120048, #120049, #120051, #120052, and #120053. Most differing records keep the same short name but vary in descriptions, costs, object flags, or other fields. Notable examples are:

- Candidate #120016 describes pink-tinted glasses and lists a cost of 150,000; active SurfaceKeeps describes blue-tinted glasses and lists 500,000.
- Candidate #120048 is a small ore deposit; the active same-number record is “rringlor no-show proc 1,” with different type and mechanics.
- Several other same-number objects differ in object flags or wear/stat values even where their display name is unchanged.

These records are separate source identities despite exact duplicates in 39 cases. Under the current manifest, active consumers resolve to the active surfacekeeps.obj definitions; the unlisted candidate does not become a second active owner.

## Typed consumers and code references

The active SurfaceKeeps source uses its own current object prototypes in 135 ZON object reset roles across 74 VNUMs and in its local QST/story references. Active Alatorin QST contains 14 item-command references across six SurfaceKeeps object VNUMs; its story sidecar has eight item_vnums references to local SurfaceKeeps objects. Alatorin ZON also resets active mobiles #120051 and #120037 and objects #120052 and #120053. Those references resolve to active SurfaceKeeps prototypes under the current manifest, not to surfacekeeps_objects.obj.

An unregistered brass-old-1.zon has one source-only reset reference to object #120047 at [line 549](../../../areas/zon/brass-old-1.zon#L549). No outside SHP product row references these local object numbers, and the SurfaceKeeps audit found no outside type-25 destination into SurfaceKeeps rooms.

Maintained code binds active object #120051 to wh_corpse_decay at [specs.assign.c line 2628](../../../src/specs/specs.assign.c#L2628); the same line range also binds mobile #120051 to wh_corpse_to_object. Two Chaos kit records name object #120013 at [chaos_eq_data.h lines 99 and 351](../../../src/account/chaos_eq_data.h#L99). The constant #120000 in [map.h](../../../src/world/map.h#L17) is the Alatorin map-range start, not a prototype ownership decision. These consumers are already covered by the active SurfaceKeeps audit.

## History and disposition

Followed candidate-file history says it was added for “stub loading” in January 2010 and later received object-file cleanup in August 2016. The candidate remains outside areas/AREA while active surfacekeeps.obj defines every same-number prototype. The history explains why the duplicate source should be preserved for owner review; it does not show that a particular stub build currently loads it or that the candidate may be moved or deleted.

Retain surfacekeeps_objects.obj in place. Confirm the stub-loader purpose, owner, current editor/export path, and whether the 15 differing bodies are intentional before any separate movement, merge, activation, or archive proposal. The twenty questions in [SURFACEKEEPS_OBJECTS_BUILDER_QUESTIONS.md](SURFACEKEEPS_OBJECTS_BUILDER_QUESTIONS.md) record the decisions. No migration clearance is implied.
