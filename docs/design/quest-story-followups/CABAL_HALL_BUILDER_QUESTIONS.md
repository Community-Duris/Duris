# Cabal Hall Builder Questions

Checkpoint 459 source-backed questions for the unregistered `cabal_hall` component. Each recommendation
preserves the existing source while its area, content, shop, and code owners clarify intent.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| CH-01 | Who owns the cabal_hall WLD, MOB, OBJ, ZON, and SHP files? | Identify the area and shop owners before changing source, loader membership, or paths. |
| CH-02 | Is “The Cabal Guildhall” the intended title, and what group does Cabal represent? | Confirm the canonical title and identity with the original builder. |
| CH-03 | Is cabal_hall the same guildhall as `cabal`, a replacement, or a separate source? | Compare owner intent and preserve both identities until it is documented. |
| CH-04 | How does cabal_hall relate to the unregistered Nihilists Anarchists Horde (`nah`) map using rooms #30100–#30112? | Ask both source owners to identify map lineage and VNUM ownership. |
| CH-05 | How does cabal_hall relate to the separate `shadow_hall` source using rooms #30100–#30102? | Keep the maps distinct unless their owners confirm a relationship. |
| CH-06 | Why do cabal_hall rooms #30100–#30109 reuse active Underworld3 room VNUMs? | Resolve numeric ownership before any world-list addition or room renumbering. |
| CH-07 | Should Underworld3 remain the owner of the currently active #30100–#30109 room identities? | Have the active area owner confirm the current map and any persisted room references. |
| CH-08 | Should Cabal Hall room fields be zone 301 when active Underworld3 uses zone field 300 for overlapping #301xx rooms? | Confirm the intended editor and zone-numbering convention for each source. |
| CH-09 | Is the compiled inn binding at #30104 intended for Underworld3's river room, Cabal Hall's gate, or another room identity? | Ask the code and area owners to settle the bound room VNUM before either source changes. |
| CH-10 | Should the unregistered component be added to the full area manifest? | Confirm runtime and gameplay intent after resolving the Underworld3 collision, shop keeper, content, and reset dependencies. |
| CH-11 | Is Cabal Hall #30100's up exit to Duris3 #153434 “Grassy Plains” intentional? | Ask both map owners to confirm the boundary destination and source ownership. |
| CH-12 | What is the intended return route from #153434 to Cabal Hall, if any? | Record the arrival and return contract with both map owners; preserve the one-way source record until then. |
| CH-13 | Room #30104 is called the “Gate to the Real World,” but its WLD exit returns to #30102 while #30100 contains the Duris3 boundary. Which route should players use? | Clarify the intended player path in source text and authored exits without inferring travel from prose. |
| CH-14 | Should the guildhall remain environmental story without a native QST or story sidecar? | Confirm the desired player-facing contract and add a story task only through a separate authorized change. |
| CH-15 | Who owns the content review for the drug-dealing, sexualized, pornographic, and violent material? | Ask the responsible content owner to review the rooms before any path or activation proposal. |
| CH-16 | Should the pornographic-library material and homophobic slur be retained, rewritten, or removed? | Record the content decision without copying slurs into new source or documentation. |
| CH-17 | Is the explicit anti-LGBTQ violence in the torture-room text appropriate for this guildhall? | Obtain a content-owner decision and revise the passage only under a separate authorized content change. |
| CH-18 | What player-facing role is intended for the Dope Deposit and its drug-dealer mobile? | Confirm the setting and shop behavior with the area and moderation owners. |
| CH-19 | Are four repeated dark golem resets in room #30100 intentional? | Confirm reset group and population intent before editing the ZON. |
| CH-20 | Are all 13 G commands intended as the drug dealer's equipment? | Confirm command grouping, mobile ownership, and the expected inventory. |
| CH-21 | Should the tracked zero-byte cabal_hall OBJ file remain empty? | Confirm that the component owns no local objects and that all object references are intentional imports. |
| CH-22 | Should the three O resets for Player Castles objects #11051, #11101, and #11013 remain? | Preserve them as source-only imports until the Player Castles owner confirms placement intent. |
| CH-23 | Are shop products #3050, #108, #109, #111, #377, #412, #399, #53, and #491–#494 still the intended inventory? | Confirm product choice and price with the shop owner; preserve their current active OBJ prototypes. |
| CH-24 | Is #6089 still intended in both the dealer equipment and shop stock despite having no current active OBJ prototype? | Reconcile it with the CBL Hall review and current Tharnadia replacement before selecting an item. |
| CH-25 | Were gathering bags #491–#494 intentionally added to both cabal_hall.zon and cabal_hall.shp in 2026? | Confirm the shop is meant to distribute the active Heavens bag prototypes and document its intended loader path. |
| CH-26 | Should the non-mini shop fallback read cabal_hall.shp when `areas/world.shp` is absent? | Ask the shop-loader and area owners to confirm the expected aggregate and fallback behavior. |
| CH-27 | Which active keeper should Cabal Hall's shop use when its configured mobile #30101 is not in the active mobile catalog? | Identify the intended keeper and prototype ownership before changing the shop key or loader membership. |
| CH-28 | Should the shop's configured room #30103 resolve to Underworld3's river room or Cabal Hall's Dope Deposit? | Confirm the intended active room identity before editing the SHP location. |
| CH-29 | Should shop #30101 remain a separate SHP record, and which file-generation process should publish it? | Confirm the source of the runtime aggregate and whether this record should be included there. |
| CH-30 | Who owns the editor data, map-generation paths, and persisted identity for VNUMs #30100–#30109? | Record the tooling and identity owners before a separate path proposal or numeric reassignment. |
