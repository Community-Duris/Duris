# Guildhall of the Dark Builder Questions

Checkpoint 467 source-backed questions for the unregistered Dark temple and shop files. Recommendations preserve current Harrow, Player Castles, Duris3, and shop-source identities until their owners confirm intent.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| DK-01 | Who owns the five tracked `dark` files and their editor/deployment state? | Identify the map, shop, content, and tool owners before a path or content decision. |
| DK-02 | Was the component’s absence from `areas/AREA` intentional? | Preserve its unregistered status until its owner confirms the intended loader contract. |
| DK-03 | What is the relationship, if any, between `dark` (“guildhall of the dark”) and `dark_gh` (“the Dark Guildhall”)? | Keep the separate source stems and #29400–#29409 versus #29200–#29242 identities until the builders document lineage. |
| DK-04 | Is “Guildhall of the Dark” the intended title and full scope for this temple-and-maze map? | Record an owner-confirmed title and alias; do not infer it from filename similarity. |
| DK-05 | Who owns each room #29400–#29409 when all ten VNUMs also name active Harrow rooms? | Preserve Harrow as the active owner and require explicit map-identity resolution before any VNUM or path change. |
| DK-06 | Are the ten different Dark room records meant to remain separate from Harrow’s active records? | Retain both map sources and resolve them with their builders; do not treat numeric overlap as a copy or replacement. |
| DK-07 | Do the 36 Dark local exits intentionally reuse target numbers that resolve to Harrow rooms in the current loader? | Keep these as source-local, identity-ambiguous edges; do not label them active Harrow or Dark routes. |
| DK-08 | Should Harrow’s two active Surface connections at #29400 remain wholly owned by Harrow? | Preserve the active boundary records and do not attribute them to the Dark map. |
| DK-09 | Is Dark temple room #29400’s up exit to unregistered Duris3 #156726 intended? | Confirm the target and one-way design with both source owners; preserve it as an unresolved inactive-source edge meanwhile. |
| DK-10 | Is the Grassy Plains destination expected to provide a return to Dark #29400? | Ask the builders to document the intended return; the current Duris3 room has no reverse exit to #29400. |
| DK-11 | Are the maze’s repeated room descriptions and one-component 10-room graph deliberate? | Review the repeated templates and each exit with the map owner; static reach does not establish travel safety. |
| DK-12 | Are the raw #294 header, #29400–#29409 room band, and #29409 zone ceiling correct? | Preserve the current range until the owner confirms reset and numbering bounds. |
| DK-13 | Are three repeated mobile #29400 resets and one #29402 reset at room #29400 intentional? | Confirm the intended sentry placements and identity before changing reset counts. |
| DK-14 | Is priest #29401 intended to reset in maze room #29408? | Confirm the keeper’s placement, which differs from active Harrow mobile #29401 and room #29408 identities. |
| DK-15 | Are all nine O resets complete, including eight references outside `dark.obj`? | Preserve the five current Harrow mappings (#29410, #29412–#29415) and three Player Castles imports; those five VNUMs also define portals in unregistered `darkop.obj`, so resolve the candidate relationship before changing ownership. |
| DK-16 | Is local waterfall object #29400 the only intended object reset from `dark.obj`? | Confirm local object placement and the source role of the unreset pamphlet #29401. |
| DK-17 | Should twelve G resets equip the #29401 priest with the same twelve SHP products? | Compare the reset inventory and shop stock with the builder’s original inventory record. |
| DK-18 | What is the intended owner and content for product #6089, which has no active object prototype? | Keep it unresolved until an object owner supplies the intended definition or confirms the missing entry. |
| DK-19 | Is the `dark.shp` record headed #29401 intended to coexist with `infernal_gh.shp` record #29401? | Ask both shop owners and the shop-loader owner to resolve the repeated record key before packaging. |
| DK-20 | Should the non-mini shop fallback include `dark.shp` when `areas/world.shp` cannot be opened? | Confirm the fallback contract and deployment source; do not infer that manifest exclusion prevents the wildcard. |
| DK-21 | Is shop keeper #29401 meant to resolve to the Dark elderly priest or active Harrow’s same-number gardener mobile? | Keep the shopkeeper identity unresolved until the shop and map owners agree on a collision-safe VNUM. |
| DK-22 | Is shop room #29408 meant to be the Dark maze or active Harrow Fountain Square? | Confirm intended room ownership before enabling or changing the shop record. |
| DK-23 | Are all twelve product VNUMs correct when #29401 resolves to Harrow’s bag and #6089 has no active definition? | Verify every product with the shop owner, including its exact active or unresolved object owner. |
| DK-24 | Are Heavens objects #53, #108, #109, #111, #364, #377, #385, #398, and #412 plus Verz1 #3050 intended stock for this shop? | Preserve the named foreign owners and confirm each import rather than redefining shared products. |
| DK-25 | Should the sexualized no-money response in `dark.shp` remain in the shop’s player-facing text? | Obtain owner/editorial review and any requested rewrite through a separate content change. |
| DK-26 | Does the `dark.shp` shop file need a documented relationship to the separate `infernal_gh` record? | Ask the shop owners to record the relationship without merging source files by key alone. |
| DK-27 | Is the lack of a QST file or sidecar intentional for this temple and guildhall? | Confirm there is no separate quest or narrative contract expected outside tracked files. |
| DK-28 | Do external editors, packaging scripts, or persisted records refer to these paths, IDs, or shop messages? | Complete toolchain and persisted-data closure before any path, content, or activation proposal. |
| DK-29 | Does the tracked history before the visible 2009 area-tree move identify an earlier map owner? | Request upstream history from the source owner; the current checkout does not establish earlier lineage. |
| DK-30 | Who can approve a later archival, migration, or activation proposal after closure? | Identify the accountable owners and make any such proposal separately from this audit. |

[coverage row 381](ZONE_COVERAGE_REGISTER.csv#L381) · [Dark map and shop audit](DARK_AUDIT_REVIEW.md).
