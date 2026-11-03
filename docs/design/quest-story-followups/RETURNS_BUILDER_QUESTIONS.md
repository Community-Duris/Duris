# returns builder questions

Canonical key: component:returns. Checkpoints 525 and 929; coverage row 438 / CSV line 439; candidate dependency data row 87 / CSV line 88. These are source-backed owner decisions. Keep both mobile records RETAIN IN PLACE until their identity, intended placement, and source path are resolved.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| RT-01 | Who owns returns.mob and the two mobile prototypes? | Record the mobile, area, editor, and deployment owners before changing the records or their source path. |
| RT-02 | Is returns the intended canonical source-group name, or only a historical filename? | Preserve the filename and request an owner-provided title and alias crosswalk before renaming it. |
| RT-03 | Which tower does the silent golem description call “this tower”? | Keep the description as written and identify the intended tower from an owner or editor source before linking or moving the mobile. |
| RT-04 | Where should silent golem #93400 stand, given there is no matching WLD or ZON source? | Confirm the intended room and reset source; do not add a placement based only on the description. |
| RT-05 | Where should diminutive cleric #93401 be placed, and whose window is described? | Obtain the map and story owner’s placement decision before associating the cleric with a room or area. |
| RT-06 | Are the golem and cleric part of one scene or unrelated mobile prototypes grouped by filename? | Preserve both records separately until their owner confirms a shared or independent story relationship. |
| RT-07 | Are #93400 and #93401 durable identities used by editor, export, or saved data? | Keep both VNUMs stable until external editor and persisted-state references are checked. |
| RT-08 | Are the current mobile flags, stats, class, and descriptions intentional for both prototypes? | Have the mobile owner review the complete records before editing flags or combat fields. |
| RT-09 | What reset source or dynamic spawn path is intended for either mobile? | Locate an owner-approved ZON or dynamic creation path; this checkout contains no same-stem ZON or external exact reset reference. |
| RT-10 | Should returns remain absent from the full areas/AREA manifest? | Make registration a separate owner decision and verify the generated world.mob input before changing membership. |
| RT-11 | How is areas/world.mob generated for deployment, and does any external build include these records? | Trace the aggregate and deployment source list; the per-file MOB source does not prove runtime loading. |
| RT-12 | Should either record be included in a mini-area input? | Preserve the current source and verify the mini deployment contract; no matching prototype appears in the checked-in mini-area sources. |
| RT-13 | Are #93400 and #93401 intentionally absent from current ZON M resets, QST giver/mobile fields, SHP keeper/room/stock fields, ITEM_TELEPORT or ITEM_SWITCH destinations, and compiled numeric references? | Confirm intended ownership and spawn semantics without inventing a quest, room, or object connection from the prose; the CP929 source scan found no direct checked-in consumer in those roles. |
| RT-14 | Is there a story sidecar, map export, or editor file that supplies the missing context? | Search the owner’s source and export workspace before treating the two descriptions as the complete story. |
| RT-15 | Does another area or source group reuse the names “silent golem” or “diminutive cleric”? | Keep the current typed identities distinct unless a documented lineage or source match is found. |
| RT-16 | Was the pair moved from another stem or area before the recorded repository move? | Request the external history or editor record; the followed Git path alone records no predecessor. |
| RT-17 | Should the sentinel and cleric descriptions remain separate if they share a tower? | Confirm the scene’s intended chronology and placement with the narrative owner before making prose edits. |
| RT-18 | Do scripts, persisted player state, or external tools refer to either VNUM? | Require script, editor, and state-owner review before renumbering, moving, or removing either prototype. |
| RT-19 | What disposition is intended while tower and placement ownership remain unknown? | Retain both source records unchanged while collecting map, spawn, editor, and deployment evidence. |
| RT-20 | What evidence would permit activation, migration, merge, rename, or archive? | Require a named owner, intended placement/spawn path, generated-input confirmation, and external-state signoff; retain in place meanwhile. |

See the [static audit](RETURNS_AUDIT_REVIEW.md#checkpoint-929-both-mobile-vnums-remain-isolated-in-current-sources), [connectivity note](WORLD_CONNECTIVITY.md#checkpoint-525-returns-two-mobile-prototypes-without-a-map), [identity/history note](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-525-returns-isolated-mobile-source-identity), and [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-525-returns-retain-mobile-source-pending-tower-and-placement-owners).
