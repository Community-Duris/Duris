# Sea (Graendiae): source audit reconciliation

**Checkpoint:** 348, coverage row 260. **Disposition:** DO NOT MOVE. No gameplay, area-membership, or migration change is proposed.

This supplement reconciles the existing [Sea story review](SEA_STORY_REVIEW.md) with reset-aware static reach, the all-inactive-WLD sweep, exact typed dependencies, normalized title and history, and builder decisions. It uses checked-in source only. No build, test, live travel, reset, shop, door, quest, or fall journey was run.

## Source story and authored scope

The active [AREA entry](../../../areas/AREA#L255) lists `sea` for headers 673–675. The source defines 270 rooms, 55 mobiles, 62 objects, 415 reset commands (284 M, 54 G, 38 D, 13 E, 25 O, and one P), and 11 shops. `areas/qst/AREA` lists Sea, but there is no current `areas/qst/sea.qst` and no native Q/QA contract is counted in [coverage row 260](ZONE_COVERAGE_REGISTER.csv#L260). Sea's [story sidecar](../../../areas/story/sea.story.json#L6) says coverage is complete and invites local requests, while its `contacts`, `stories`, and `exclusions` arrays are empty.

The existing narrative review maps Graendiae's coral settlement, engineered currents, Pjworn chowder fields, the abandoned recycling district, temples and guilds, hidden crime, museum, and domestic life. It also records the generic bartender `world_quest` assignment and the two generic inn assignments. Those source findings establish described content and bindings; they do not qualify runtime availability or prove a local quest chain.

## Active connectivity and reset-aware reach

The active-WLD scan parsed all 351 manifest contributors and found 270 Sea rooms with 719 exits and no Sea parse issues. There are zero active WLD arrivals from other areas. Of the Sea exits, 716 resolve within the active world and three leave the active map from room #67568 toward VNUMs found in unregistered `surf.wld` and `surface2011.wld`:

| Sea source | Direction | Unresolved target | Active resolution |
| --- | --- | ---: | --- |
| #67568 | East (`D1`) | #219820 | None |
| #67568 | South (`D2`) | #220119 | None |
| #67568 | West (`D3`) | #219818 | None |

These targets are not active cross-area exits in the normal manifest build. The frozen active census records zero cross-area exits in either direction and three positive unresolved targets; normal loading therefore does not establish a route through them. See the [unresolved-exit rows](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L254).

The full sweep of all 95 unregistered WLD files parsed without errors, found no source-room VNUM overlap with Sea, and found exactly three exits into Sea. All three are in `surf.wld`:

| Unregistered source | Direction to active Sea #67568 | Source |
| --- | --- | --- |
| #219818, “Rolling Foothills Covered With Grass and Plantlife” | East (`D1`) | [`surf.wld` room and return](../../../areas/wld/surf.wld#L415766) |
| #219820, “Rolling Foothills Covered With Grass and Plantlife” | West (`D3`) | [`surf.wld` room and return](../../../areas/wld/surf.wld#L415808) |
| #220119, “Beautiful Cascading Waterfall” | North (`D0`) | [`surf.wld` room and return](../../../areas/wld/surf.wld#L422075) |

Each is a source-only return paired by direction with one of Sea's three unresolved exits. They were already enumerated in the checked-in [Surf retention baseline](ATTIC_MIGRATION_PLAN.md#L27) and on existing [candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Do not add them again: Surf remains at 263 reviewed source-only records and the curated global total remains 4,499.

The same three numeric VNUMs also appear in inactive `surface2011.wld`, but those records are titled “The Azure Waters of a Storm Sea” and do not exit to Sea #67568:

| VNUM | `surf.wld` source identity | `surface2011.wld` source identity |
| ---: | --- | --- |
| #219818 | “Rolling Foothills Covered With Grass and Plantlife”; east to Sea #67568 ([record](../../../areas/wld/surf.wld#L415766)) | “The Azure Waters of a Storm Sea”; its exits do not target Sea #67568 ([record](../../../areas/wld/surface2011.wld#L416179)) |
| #219820 | “Rolling Foothills Covered With Grass and Plantlife”; west to Sea #67568 ([record](../../../areas/wld/surf.wld#L415808)) | “The Azure Waters of a Storm Sea”; its exits do not target Sea #67568 ([record](../../../areas/wld/surface2011.wld#L416221)) |
| #220119 | “Beautiful Cascading Waterfall”; north to Sea #67568 ([record](../../../areas/wld/surf.wld#L422075)) | “The Azure Waters of a Storm Sea”; its exits do not target Sea #67568 ([record](../../../areas/wld/surface2011.wld#L422500)) |

The matching VNUMs do not make the two inactive maps interchangeable or identify which legacy identity, if either, should own Sea's unresolved destinations. Preserve both source records and their separate candidate-specific evidence. The `surface2011` row remains at 145 active references; neither candidate's active-reference count changes.

Sea's local weak-exit graph has two components: 269 rooms and isolated stable room #67540. Directed source-graph reach from three local reference points is:

| Local reference point | Reach with all authored local exits | Reach with every reset-controlled door direction suppressed |
| --- | ---: | ---: |
| #67301, Seaweed Covered Grate | 243 / 270 | 1 / 270 |
| #67305, Central Square | 243 / 270 | 210 / 270 |
| #67568, coral-reef entrance | 243 / 270 | 1 / 270 |

These are source-graph comparisons, not confirmed player arrivals: Sea has no active WLD inbound route. The filtered column suppresses each direction with a matching, 100%-chance `D` reset. All 38 nonzero WLD door flags have a matching reset by room and direction. Reset comments and states describe 27 closed doors (state 1), nine closed/secret doors (state 5), and two closed/locked/secret doors (state 6). A key, special, or runtime state could change actual access; no such journey was tested.

Room #67540, “Stables of Graendiae,” has no authored exit even though the ZON resets a seahorse and hippocampus there. Creature Rental #67539 exits to Waterways #67525, not to the stable, and neither room has a shop record. The WLD also records currents in 58 rooms and one fall-chance field: Waterway Latrines #67501 stores `F 5` with current values `10 5`. These are authored markers, not proof of usable service, hazard behavior, or safe return.

## Typed dependencies and compiled bindings

| Source relationship | Exact source evidence | Audit classification |
| --- | --- | --- |
| Sea reset places external object #3097 in room #67523 | [Sea reset](../../../areas/zon/sea.zon#L182), [Verz1 object](../../../areas/obj/verz1.obj#L1199), [active owner](../../../areas/AREA#L15) | A typed import from active Verz1. Preserve the object namespace and owner if either area changes. |
| Sea object #67359 is used by Brass | [Brass reset](../../../areas/zon/brass.zon#L822), [Brass shop stock](../../../areas/shp/brass.shp#L585), [Sea object](../../../areas/obj/sea.obj#L616), [active Brass entry](../../../areas/AREA#L397) | Active external reset and shop consumers of a Sea object. |
| Sea object #67359 is also used by `brass-old-1` | [legacy reset](../../../areas/zon/brass-old-1.zon#L832), [legacy shop stock](../../../areas/shp/brass-old-1.shp#L571) | Source-only candidate dependency; keep separate from active Brass. |
| Object #67322 is referenced locally but has no current object prototype | [Karlaen shop stock](../../../areas/shp/sea.shp#L115), [Sea reset](../../../areas/zon/sea.zon#L368); mobile #67322 is Sanriff | Unresolved typed local reference. Mobile and object VNUMs are separate namespaces; the mobile definition does not satisfy the object reference. |

The direct C/C++ range scan found only mobile #67324 assigned the generic `world_quest` special and rooms #67493 and #67503 assigned the generic `inn` special ([assignments](../../../src/specs/specs.assign.c#L757)). The wider typed scan found no external QST or story-sidecar references, no same-type external mobile/object prototype definitions, no inactive room-number overlap with Sea, and no target into Sea among 871 external type-25 object declarations. These are source-reference results; persisted player state, editor paths, dynamic dispatch, and installed runtime behavior are not inferred.

## Title, duplicates, and followed history

After color-code removal, trimming, whitespace normalization, and case-folding, “Graendiae” occurs once among all 442 current `.zon` titles and once among the 351 active contributors. Same-type scans found no duplicate Sea room, mobile, or object VNUM in another active contributor; the all-95 inactive-WLD scan found no Sea room-number overlap. The equal-numbered `surface2011` records described above are distinct source identities, not evidence of an active duplicate or replacement.

Sea and `seakngdm` (“Sea Kingdom”) remain distinct active stems and titles. Followed WLD/MOB/OBJ/ZON/SHP history stays on the Sea stem from the 2009 move into `areas/` through same-stem edits; latest checked content commits are WLD 2013, MOB 2014, OBJ 2016, ZON 2015, and SHP 2014. The Sea story sidecar was added on 2026-10-02. No current Sea QST file, alternate-stem predecessor, or explicit whole-zone replacement was found. Keep active Sea DO NOT MOVE.

## Builder questions and recommended resolutions

The earlier narrative review has eight story-focused questions. This consolidated cross-discipline list expands that agenda to twenty and keeps each recommendation tied to checked source.

1. **Does the sidecar's `coverage: complete` and “local requests” wording mean Sea is intended to have no mapped local quest, despite empty story arrays and no current QST file?** **Recommendation:** either map the existing environmental requests and generic interactions accurately, or revise the completeness/requests wording; do not imply a native local contract that is not present.
2. **Should the bartender's generic `world_quest` feature be represented as a general world-quest service rather than a Graendiae story?** **Recommendation:** label its scope clearly in the sidecar and keep it separate from local QST coverage.
3. **Are the Broken Promise Inn and Flowering Anemone Inn intended to provide only the generic `inn` behavior assigned in code?** **Recommendation:** record the service boundary in the story map and qualify actual registration separately.
4. **Is the unemployed father's explicit job request in Laughter's Edge only environmental color, or should an employment story be designed?** **Recommendation:** keep it as authored setting detail until a supported arc and owner are specified.
5. **Can the two mobile descriptions that identify teenagers and then describe explicit sexual activity be rewritten as age-appropriate, nonsexual text?** **Recommendation:** resolve this player-facing text before presenting those descriptions; no text was changed in this audit.
6. **What object should VNUM #67322 identify?** It appears in shop stock and a reset but has no current object prototype. **Recommendation:** confirm the intended object, then update the reference or restore an owned prototype in a separate content change.
7. **Are the Verz1 counter import #3097 and the Brass consumer of Sea ration #67359 intentional cross-area dependencies?** **Recommendation:** preserve each typed owner and coordinate any later object or candidate decision with Verz1, Brass, and the separate `brass-old-1` source-only reference.
8. **Which active-world route should lead to Sea from room #67568, if any?** Its east, south, and west exits resolve only to VNUMs in inactive candidates. **Recommendation:** decide an active entrance and reciprocal return as a separate map proposal before changing the source edges.
9. **Do the three reciprocal `surf.wld` returns still express a desired former-Surface route to Sea?** **Recommendation:** keep them source-only on Surf row 97, already counted, until map ownership and an active route decision are explicit.
10. **Which map identity, if any, should be associated with the equal VNUMs in `surface2011.wld`?** Their Storm Sea prose and exit topology differ from `surf.wld`. **Recommendation:** retain both candidate identities; do not infer a route owner or merge by VNUM.
11. **Should the underwater city Sea remain separate from `seakngdm` / Sea Kingdom, and is the absence of an active boundary intentional?** **Recommendation:** preserve the two active identities and require an explicit reciprocal route proposal before linking them.
12. **Is stable room #67540 meant to be reachable through a special or omitted route?** Mounts reset there, but the room has no WLD exits. **Recommendation:** identify the intended access mechanism and verify it before treating stable stock as player-accessible.
13. **Should Creature Rental #67539 lead to the isolated stable or operate independently from Waterways #67525?** No shop record ties either room to a rental service. **Recommendation:** confirm the room relationship and service binding before documenting rental as operational.
14. **Are the grate and tunnel doors intended to start closed, secret, or locked as the 100%-chance resets describe?** **Recommendation:** preserve the paired source flags and resets until builders qualify keys, discovery, and return from both sides.
15. **Do the current fields and authored swimming warnings describe intended player travel difficulty across all 58 marked rooms?** **Recommendation:** map currents to routes and qualify movement/return behavior before describing any stream as safe or traversable.
16. **What should happen when the `F 5` field in #67501 triggers, and where should the character land?** **Recommendation:** verify the configured fall route and a usable return before presenting the room as safe.
17. **Is Pjworn farming and chowder export meant to be a major civic livelihood or background texture?** **Recommendation:** retain the authored production chain in the setting map and avoid inferring economic mechanics from prose alone.
18. **Which account of Janinae's contained lava is intended to remain contested: divine gift or a gnome scientist's bargain?** **Recommendation:** preserve the disagreement as authored unless the area owner chooses an authoritative explanation.
19. **Is the gang name “Ghil'ha” or “Ghil'ra”?** The WLD uses both spellings around Sslin and the black market. **Recommendation:** choose one canonical label or intentionally assign distinct groups before standardizing prose.
20. **Should the museum's limited surface perspective and imperfect restoration remain the voice of Graendiae's child-facing exhibits?** **Recommendation:** clarify what the city values or misunderstands about surface artifacts while preserving the source's deliberate aquatic perspective.

## Disposition

Keep active `sea` DO NOT MOVE. Preserve the three Surf source-only returns on existing candidate row 97 without changing the pre-existing Surf subset or global total. Retain `surface2011` row 98 and its distinct Storm Sea records. Any source prose, QST, route, object, shop, or reset changes require separate builder decisions documented above; this supplement changes documentation only.

