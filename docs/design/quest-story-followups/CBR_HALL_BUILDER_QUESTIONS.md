# BattleRager Hall Builder Questions

Checkpoint 462 source-backed questions for the unregistered `cbr_hall` component. Recommendations preserve the source while active Newbie, area, shop, code, and data owners clarify its identities and intended use.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| BR-01 | Who owns the `cbr_hall` WLD, MOB, OBJ, ZON, and SHP files and their external editor data? | Identify source, shop, code, and tooling owners before changing paths or loader membership. |
| BR-02 | Is “BattleRager Hall” the canonical title, or should it match the room prose's “Clan BattleRage” name? | Confirm the official clan and area spellings with the owner before recording an alias. |
| BR-03 | Should active Newbie continue to own room VNUMs #29300–#29309 with its current room identities? | Preserve current active Newbie room meanings unless both map and persisted-data owners approve a documented change. |
| BR-04 | How should BattleRager Hall's ten room identities relate to active Newbie's rooms at the same VNUMs? | Keep the maps separate until the owners provide an explicit numeric crosswalk or replacement record. |
| BR-05 | Is Newbie's #29305 “Secret Magic Shop” the intended target of the compiled inn binding at #29305? | Confirm the code owner intended identity; do not redirect the binding to the candidate YMCA room by assumption. |
| BR-06 | Should active Newbie continue to own mobile VNUMs #29300–#29303 for its mercenary, assassin, alchemist, and bleeding man? | Preserve active prototype identity until the code and area owners approve any distinct assignment. |
| BR-07 | Are candidate golems #29300–#29302 and Merzec #29303 meant to be distinct from Newbie's mobiles with the same IDs? | Resolve the mobile-VNUM collision before loading or moving candidate files. |
| BR-08 | Are compiled Ailvio constants #29303/#29304 meant to identify Newbie's bleeding man and half-elf woman? | Have the Ailvio and Newbie owners confirm the code-to-prototype mapping before any renumbering. |
| BR-09 | Should Newbie object #29300 remain the painful dagger named “Pain” while candidate #29300 is an Ogre-hide backpack? | Preserve Newbie's active object identity and settle how the candidate backpack receives a distinct VNUM. |
| BR-10 | Should active Newbie continue to own object IDs #29301–#29309 as its signs, mirror, tomes, quill, and totem? | Keep the active object set distinct from the candidate until the object owner documents any shared IDs. |
| BR-11 | How does `cbr_hall` relate to inactive `eskull_hall`, `loc_gh`, and `ravenmis` maps that also use #29300–#29309? | Ask each map owner to identify only documented lineage; do not infer common ownership from repeated numbers. |
| BR-12 | Is the D4/up exit from candidate room #29300 to Duris3 #152260 “Grassy Plains” intentional? | Confirm the route and both map owners; retain the authored source edge while its status is open. |
| BR-13 | What return route should connect Duris3 #152260 to BattleRager Hall, if any? | Document arrival and return ownership with Duris3 and BattleRager owners; do not infer a reciprocal path. |
| BR-14 | Is Duris3 #152260 the intended area boundary, given that it is not in the full area manifest? | Preserve the source-only target and resolve its current source owner before proposing activation. |
| BR-15 | Should the candidate's room and zone fields remain 293 for the #29300–#29309 range? | Confirm the raw zone convention and the intended editor range; keep the current values until then. |
| BR-16 | Should the four repeated M commands for golem #29300 all reset in room #29300? | Confirm the intended count and group with the builder, including the current active Newbie mobile collision. |
| BR-17 | Are the candidate golem prototypes #29301 and #29302 intentionally defined but absent from the ZON placements? | Confirm whether they are unused variants, missing resets, or historical prototypes before editing. |
| BR-18 | Is the single M placement of Merzec #29303 in candidate shop room #29303 intended? | Preserve the source placement and settle its active VNUM ownership and shop role with both owners. |
| BR-19 | Should the two Player Castles object resets #11055 and #11105 remain in rooms #29300 and #29304? | Preserve both source-only imports until the Player Castles and BattleRager owners confirm placement intent. |
| BR-20 | Should the candidate keep only the single Ogre-hide backpack prototype #29300? | Confirm its capacity, stock role, and intended room/mobile placement after its VNUM is reconciled. |
| BR-21 | Should the one SHP record remain configured with keeper #29303 and room #29303? | Resolve the candidate/Newbie keeper and room identities before publishing or changing the shop record. |
| BR-22 | If the non-mini SHP fallback reads `cbr_hall.shp`, should it use Newbie's active mobile and room #29303? | Ask the shop-loader and area owners to confirm the intended aggregate and typed resolution; do not infer a working shop. |
| BR-23 | Is stock product #29300 meant to be the candidate backpack or active Newbie's dagger? | Settle the product identity and update both SHP stock and ZON G placement only under a separate authorized change. |
| BR-24 | Are the remaining shop products #3050, #204, #53, #399, #412, #364, #384, #203, #108, #109, and #111 still intended? | Confirm inventory and prices with the shop owner while preserving current active object prototypes. |
| BR-25 | Were gathering bags #491–#494 intentionally added to both `cbr_hall.zon` and `cbr_hall.shp` in 2026? | Confirm they are intended active Heavens products and document the desired shop-loader path. |
| BR-26 | Should `cbr_hall` remain without a same-stem QST or story sidecar and outside both manifests? | Confirm the intended static story, shop, quest, and loading contract with the area owner. |
| BR-27 | Does tracked history identify `cbr_hall` as a rename, predecessor, or replacement for Newbie or another guildhall? | Record only a documented rename or replacement; current VNUM reuse is not sufficient evidence. |
| BR-28 | Who owns the persisted room/mobile/object references and any future path-only archival decision? | Identify the map, shop, code, editor, and data owners and request separate authorization after closure. |
