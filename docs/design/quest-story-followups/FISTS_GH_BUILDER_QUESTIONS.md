# fists_gh builder questions

Canonical key: component:fists_gh. Checkpoint 479; coverage row 392 /
CSV line 393; candidate dependency row 42. These are source-backed decisions,
not implementation instructions. Keep this component RETAIN IN PLACE until
the map, Patrols identities, shop, reset, and external-data owners resolve it.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| FGH-01 | Who owns the unlisted WLD/MOB/OBJ/ZON/SHP files, editor map, and deployment path? | Record the source, map, tooling, and deployment owners before changing paths or loader membership. |
| FGH-02 | Is Fists of Fury Guildhall the canonical title, and are there aliases not recorded in AREA? | Preserve the ZON title and blank raw AREA annotation until the owner supplies a documented title/alias crosswalk. |
| FGH-03 | Is the lack of a same-stem QST, story sidecar, or editor-map file intentional? | Identify any external quest and narrative sources before treating the guildhall as complete or disposable. |
| FGH-04 | Are zone header #305, top VNUM #30509, and reset mode 0 intentional with the five M and fourteen G commands present? | Preserve the exact fields and ask the builder to explain the intended reset policy before changing it. |
| FGH-05 | Is the ten-room graph in the source table complete, including the portal room and colored path rooms? | Confirm the complete authored layout and preserve the current records until the map owner approves changes. |
| FGH-06 | Who owns room VNUMs #30500–#30506: the Fists of Fury map or active Patrols? | Keep both complete map records separate and require an owner-approved VNUM crosswalk before any merge or activation. |
| FGH-07 | Should active Patrols continue to own Elven, Barbarian, Tharnadian, Halfling, Gnomish, Dwarven, and Centaur load rooms at #30500–#30506? | Preserve current active Patrols identities unless the active-area owner approves a separately planned reassignment. |
| FGH-08 | Is #30500 up to inactive Duris3 #157556 “Grassy Plains” intentional, and should it remain one-way? | Confirm the boundary and return path with both map owners before changing either WLD. |
| FGH-09 | Are #30503 down to #30509 and #30509 up to #30503 intended reciprocal exits? | Preserve the raw direction fields and have the builder confirm this vertical connection; correct documentation to match D4 up. |
| FGH-10 | Should the source-only approaches #30507 east to #30506 and #30509 up to #30503 remain attached to this candidate? | Preserve both as candidate references, not active ingress, and do not add them again to curated connectivity totals. |
| FGH-11 | Does any active route enter the Fists map identity, given that active numeric matches resolve to Patrols and candidate inbound is zero? | Keep the active Patrols interpretation and candidate identity distinct until source and route owners provide evidence. |
| FGH-12 | Should golems #30500–#30502 and merchant #30503 remain distinct from same-number Patrols mobiles? | Preserve the typed mobile records separately and check every reset, script, shop, and saved-state consumer. |
| FGH-13 | Are the five M commands and their room/max-count fields intended, including two separate #30500 golem resets? | Confirm the reset counts and placements while preserving the raw commands and reset mode. |
| FGH-14 | Why did the former unnamed OBJ prototype #30500 disappear in the 2016 cleanup, and is the zero-byte OBJ file intentional? | Record the object owner’s explanation; do not recreate or renumber a prototype based on the old description alone. |
| FGH-15 | Is SHP/G stock product #6089 intentionally referenced despite having no current OBJ prototype? | Identify its owner or document that it is deliberately unavailable before altering either product list. |
| FGH-16 | Should product IDs #53, #377, #364, #347, #108, #109, #111, #3050, and #357 remain in both reset and shop stock lists? | Verify each product identity and active prototype source; preserve the same-number lists until the shop owner signs off. |
| FGH-17 | Were gathering bags #491–#494 intentionally added to this candidate’s G resets and SHP in 2026? | Preserve the 2026 additions and confirm their intended guild-shop placement with the feature owner. |
| FGH-18 | Should Player Castles altar #11013 continue to be imported into candidate inn #30504? | Preserve the exact foreign O reset until Player Castles and guildhall owners confirm its purpose and placement. |
| FGH-19 | Is keeper #30503 intended to be a greedy merchant and shop room #30502 intended to be the Fists store? | Preserve both SHP fields and resolve them against current Patrols identities before relying on the source record. |
| FGH-20 | If areas/world.shp is absent, should the non-mini fallback aggregate fists_gh.shp with the eight legacy O-format shops in luiren.shp? | The fallback includes the shard by path, but the current parser rejects O records; have the loader/shop/deployment owners supply a valid aggregate or reconcile the format before claiming a successful load. |
| FGH-21 | Is compiled inn assignment #30504 intended for the Fists inn, active Patrols’ Gnomish Guard Load Room, or another owner? | Resolve the numeric callback with the code and active-area owners; preserve both source identities until then. |
| FGH-22 | Who owns the additional compiled inn reference #30511, which has no current WLD definition? | Record its intended room owner or remove/change it only through a separate code-owner decision. |
| FGH-23 | Do followed source histories establish a rename, predecessor, or replacement relationship with Patrols or another guildhall? | Record only explicit lineage evidence; the numeric overlaps, 2016 object deletion, and 2026 stock additions do not prove replacement. |
| FGH-24 | What evidence would permit a future activation, move, rename, merge, or archive proposal? | Require map/story ownership, route and reset review, shop and product closure, editor/deployment closure, and persisted-VNUM review; retain in place until then. |

See the [static audit](FISTS_GH_AUDIT_REVIEW.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-479-fists_gh-guildhall-map-and-patrols-collisions),
and [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-479-fists_gh-title-history-and-source-identity).