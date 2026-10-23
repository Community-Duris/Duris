# Jungle of Khazan inactive-source audit

Canonical key: component:jungle. Physical coverage row 405 / CSV line 406; candidate dependency row 55 / CSV line 55. Reviewed October 13, 2026, Checkpoint 904 (baseline review Checkpoint 492), against source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697.

This is a static source, manifest, connectivity, identity, reset, and typed-reference review. It does not establish live loading, gameplay, deployed quest inputs, saved-world ownership, editor copies, or permission to activate, move, rename, or archive any source.

## Source inventory and manifest state

The unlisted Jungle component contains [WLD](../../../areas/wld/jungle.wld#L1), [MOB](../../../areas/mob/jungle.mob#L1), [OBJ](../../../areas/obj/jungle.obj#L1), and [ZON](../../../areas/zon/jungle.zon#L1) files. The ZON header is #670, titled “Jungle of Khazan,” with top VNUM #67092 and reset mode 1. The WLD has 92 rooms (#67001–#67092), the MOB has 14 prototypes (#67000–#67013), and the OBJ has five prototypes (#67000–#67004). No same-stem QST, SHP, or structured story sidecar is present.

The full [areas/AREA](../../../areas/AREA#L252) row is commented. Commit c0cf55b85e00b7e01411848a03d6f7d47b32b1f0 did that on April 16, 2026; it changed the full-area list entry but left the source files in place. By contrast, [areas/qst/AREA](../../../areas/qst/AREA#L226) still has an un-commented Jungle row for zone 670, last attributed by blame to the July 2012 list update. No current areas/qst/jungle.qst file or followed file history for that path is present.

That QST list is not the production quest input. The Makefile names areas/world.qst as generated output; [make_qst.c](../../../areas/src/qst/make_qst.c#L6) reads AREA in its working directory, skips lines beginning with an asterisk, and looks for qst/<area>.qst for each remaining name. The area-story catalog also documents areas/qst/AREA as a narrower historical list, not the production input. Runtime [quest.c](../../../src/world/quest.c#L79) reads the generated areas/world.qst aggregate and explicitly does not glob areas/qst/*.qst. Since Jungle is commented out of the full list and has no individual QST source, the tracked standard quest-generation path does not include a Jungle quest. The aggregate is absent from this checkout because it is generated; deployed or separately generated inputs were not inspected. Treat the lingering QST/AREA row as a stale-looking manifest discrepancy whose owner intent still needs confirmation, not as proof of a loaded quest.

The source rooms provide a setting rather than a tracked quest arc: dense jungle, faint trails, riverbanks and swift water, rocks, massive trees, a rickety bridge, and “Under the Bridge.” The prototype names add apes, baboons, jaguars, leopards, tigers, piranhas, snakes, a viper vine, spiders, frogs, harriers, and a dark naga, with eggs, gems, and a bronze circlet. Whether a separate story or quest was intended is not recorded.

## Candidate WLD graph, active targets, and source-only approaches

The candidate WLD has 288 authored exits. Of these, 282 target another room in the same candidate set. The local undirected graph is one weak component of 92 rooms, and raw directed traversal from #67001 reaches all 92. This is authored graph reachability only; it says nothing about loaded access, current movement, resets, or safe return.

| Jungle exit | Authored target | Current tracked identity |
| --- | --- | --- |
| #67001 west | #158558 | Duris3.wld “Mountains”; unlisted and no direct inverse return |
| #67005 north | #616072 | Active Surface “The Rapid Waters of a Tropical River” |
| #67037 east | #158560 | Duris3.wld “Rolling Hills”; unlisted and no direct inverse return |
| #67087 south | #616872 | Active Surface “The Rapid Waters of a Tropical River” |
| #67091 south | #158659 | Duris3.wld “Grassy Plains”; unlisted and no direct inverse return |
| #67074 west | -1 | One authored negative target, not a positive destination |

Only the two Surface target records resolve in the current active WLD table. The three Duris3 targets exist in Duris3.wld but that source is not in the full area list and the target rooms do not link directly back to Jungle. The 282 local targets are present in Jungle.wld but do not resolve through the current active WLD table. Thus 285 of 287 positive target records are unresolved in the active table; the remaining record is the single negative target.

There is no active WLD exit into Jungle’s #67001–#67092 range. A separate full-source scan found four incoming WLD records, all in files absent from areas/AREA:

| Source-only WLD record | Destination |
| --- | --- |
| surf.wld #252412 south | Jungle #67005 |
| surf.wld #253012 north | Jungle #67087 |
| underdark2011.wld #253961 south | Jungle #67005 |
| underdark2011.wld #255161 north | Jungle #67087 |

These are historical source references, not live approaches. They have no direct reverse exit from the named Jungle room. The two Jungle-to-Surface exits above were already recorded at Checkpoint 373 as inactive-source approaches into active Surface rooms. Keep those references with Jungle, and do not add them again to the curated Surface approach total. The active Surface target rooms continue to #616472 rather than back to the Jungle source endpoints. Other unlisted files also define #616072 and #616872 differently, but the current full list resolves both numeric targets to Surface.

## Resets, currents, and fall marker

The Jungle ZON contains 76 commands: 68 M mobile loads, four O object placements, three G inventory loads, and one E equipment load. All typed prototype and room arguments in the candidate ZON are local. Every one of the 14 candidate mobiles and five candidate objects is referenced by a reset. The four O commands place harrier eggs twice in #67027 and twice in #67055; the G and E commands distribute the three gem prototypes and bronze circlet. Reset commands directly use 53 rooms; the other 39 rooms have no direct room argument. No external prototype reset target was found in any other tracked ZON.

Eleven rooms carry C 5 2 fields: #67005, #67015, #67025, #67033, #67042, #67051, #67060, #67070, #67079, #67087, and #67092. The source declares speed 5 in direction 2 (south). Room #67051, “A Bridge Over the River,” also has F 5 and a down exit to #67092. The WLD reader loads these current and fall fields; the static source does not qualify their actual in-game effects.

## Typed dependency closure

The bounded source scan found no external typed ZON reset reference to Jungle room, mobile, or object VNUMs; no QST giver header or item goal for its prototypes; no SHP product entry for its objects; no typed story-sidecar reference; and no exact Jungle room/mobile/object integer in tracked C/C++ source. Jungle itself has no QST or SHP file. These findings cover the checked-in source only. They do not close external scripts, editor state, exports, deployed generated files, persisted character/world data, or dynamically computed references.

## Identity, history, and disposition

After stripping the color codes, the current ZON title is unique among tracked ZON sources. The candidate WLD, MOB, OBJ, and ZON histories remain on the Jungle stem to the 2009 areas-tree move; later bounded history includes 2011/2013 WLD work and 2012–2015 MOB/OBJ/ZON edits. No competing same-type definition for any Jungle room, mobile, or object VNUM was found in the current WLD/MOB/OBJ source tree, and no tracked whole-zone replacement or alternate-stem predecessor was identified. The April 2026 full-list comment is a list-state change, not a source deletion or replacement decision.

Keep the WLD, MOB, OBJ, and ZON **RETAIN IN PLACE**. Preserve the April 2026 unlisted state and the VNUMs while the owner resolves the lingering QST/AREA row, the four source-only inbound references, three Duris3 outbound references, two already-counted Surface routes, mode-1 reset intent, southern currents, bridge fall marker, editor/deployment copies, and persisted identity. Do not infer archive or activation permission from unique VNUMs or list exclusion. See the [identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-492-jungle-source-identities-and-manifest-history), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-492-jungle-retained-pending-source-route-and-movement-ownership), [candidate row 55](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L55), and [coverage row 405](ZONE_COVERAGE_REGISTER.csv#L406).

## Review limits

No tests, builds, server, database, migration, or container work was run. This checkpoint is based on tracked source and static loader/compiler inspection; it does not certify runtime route behavior, fall/current outcomes, quest deployment, or player safety.

## Checkpoint 904: external typed-role refresh

The CP492 inventory, authored graph, four source-only inbound records, and
**RETAIN IN PLACE** recommendation are unchanged. A role-aware recheck found
no additional outside references into Jungle's room, mobile, or object sets.

No outside ZON typed reset field targets a Jungle room/mobile/object. No
external type-25 teleport destination or type-29 switch room target enters the
map; Jungle's own five objects are types 8, 9, 11 (two), and 27. External QST
giver/item commands, structured story-sidecar numeric roles, parser-shaped
N-format shop fields, and direct `real_room`/`real_mobile`/`real_object` calls
also have no Jungle-range match. No outside WLD room-definition record reuses
a same-type Jungle VNUM.

Keep the known routes distinct: the four WLD approaches from unlisted `surf`
and `underdark2011` remain source-only; two Jungle exits to active Surface
remain the CP373-counted approaches; three D4 exits remain one-way references
to unlisted Duris3. These counts do not change. Keep the lingering
`qst/AREA` listing separate from standard production input: no
`areas/qst/jungle.qst` exists, and the compiler reads the full `areas/AREA`
list. The role refresh adds no evidence of an active Jungle quest or travel
path.

Retain all four files in place. Preserve mode-1 resets, eleven southbound
current fields, bridge F 5, and the external manifest, editor/deployment,
story-owner, and persisted-ID questions. See the [updated 27 builder
questions](JUNGLE_BUILDER_QUESTIONS.md),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-904-jungle-has-no-new-external-typed-route),
[identity/history supplement](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-904-jungle-retains-its-unlisted-identity),
[retention note](ATTIC_MIGRATION_PLAN.md#checkpoint-904-jungle-retains-existing-route-and-movement-boundaries),
[candidate dependency row 55](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L55), and
[coverage row 405](ZONE_COVERAGE_REGISTER.csv#L406).
