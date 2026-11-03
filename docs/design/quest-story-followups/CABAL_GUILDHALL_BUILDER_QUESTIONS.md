# Cabal Guildhall Builder Questions

Checkpoint 458 source-backed questions for the unregistered cabal component. Each recommendation
preserves the current files until their responsible builder, area owner, or shop owner confirms
intent.

| ID | Question | Recommended resolution |
| --- | --- | --- |
| CG-01 | Who owns the cabal WLD, MOB, OBJ, ZON, and SHP files? | Identify the area and shop owners before changing source, loader membership, or paths. |
| CG-02 | Is “Cabal Guildhall” the intended title, and what is the intended expansion or identity of Cabal? | Confirm the canonical title and group identity with the original area owner. |
| CG-03 | Why does the unlisted map use all 13 room VNUMs #29000–#29012 already owned by active Torg? | Resolve the active map owner and decide which room identity is authoritative before re-registering or renumbering either source. |
| CG-04 | Should Cabal mobile keys #29000–#29002 remain shared with Torg's different mobiles? | Confirm the prototype owner and any persisted references before changing or reactivating those keys. |
| CG-05 | Does the Cabal shopkeeper VNUM #29001 intentionally collide with Torg's drunk hobgoblin guard? | Identify the intended active keeper and do not infer Odif is loaded from cabal.mob. |
| CG-06 | Should cabal.shp be included by the non-mini all-file fallback when cabal is absent from areas/AREA? | Ask the shop-loader and area owners to state the intended aggregate behavior for this source record. |
| CG-07 | The Cabal shop's location is room #29003, which is an active Torg inner-compound room. Where should the shop be located? | Confirm the intended room identity before changing the SHP location or active area list. |
| CG-08 | Should one shop record keyed #29001 exist for Cabal? | Confirm whether it is current, historical, or meant to be generated through a different loader path. |
| CG-09 | Which products should the shop stock when #29000, #29001, and #6089 have no current active object prototypes? | Ask the shop owner to resolve these three products and keep the ten currently defined stock items distinct. |
| CG-10 | Is object #6089 still intended in both Cabal's stock and its G reset list? | Reconcile it with the CBL Hall review, which identifies the old Tharnadia item as renumbered, before any replacement. |
| CG-11 | Are the 10 other stock IDs (#364, #385, #377, #3050, #398, #53, #108, #109, #111, and #412) still the intended inventory? | Confirm the current stock contract and source owners for the active Heavens and Verz1 object prototypes. |
| CG-12 | Should the Cabal item resets for Player Castles objects #11005, #11062, #11013, and #11112 remain? | Confirm those external decorations with the Player Castles owner; retain them as typed references until then. |
| CG-13 | Are the 13 G equipment commands intended for Odif or another mobile? | Confirm the command grouping and equipment owner, especially because the preceding M resets name multiple Cabal guardians and Odif. |
| CG-14 | Do the type-25 objects #29002 and #29003 intentionally target Cabal rooms #29010 and #29007? | Verify the object values and intended initial destinations with the travel-code and item owners. |
| CG-15 | Why do the portal descriptions name Goodie Continent and Ice Crag while their encoded targets are local Cabal rooms? | Document the full expected route from each object, including the final destination and return path. |
| CG-16 | Are the two type-25 objects intended to connect the separate #29000–#29009 and #29010–#29012 WLD components? | Confirm that the item routes are intentional and keep them separate from authored WLD exits in documentation. |
| CG-17 | Is Cabal #29001's up exit to #111079 intended to reach active Tharn Rifts, Duris3, or neither? | Resolve the same-numbered room identities and preserve the Tharn Rifts audit's source-room overlap classification. |
| CG-18 | Is the up exit from Cabal #29012 to inactive Duris3 room #156242 intentional, and where is the return path? | Ask both map owners to confirm the one-way inactive-to-inactive boundary. |
| CG-19 | Should the zone header and all room fields be 1 when the room VNUMs belong to zone 290? | Confirm the intended editor and loader zone numbering without changing manifest membership in this audit. |
| CG-20 | What reset behavior is intended by ZON reset mode 0? | Ask the zone owner to interpret the field and document the desired reset lifecycle. |
| CG-21 | Is cabal_hall the same guildhall, a later alternative, or a separate component? | Compare both owners' intent; the near-identical title, shared product IDs, different VNUMs, and 2026 edits do not prove replacement. |
| CG-22 | Is active cbl_hall related to Cabal, or is its Clan BloodLust identity separate? | Keep the registered CBL source distinct unless its owner confirms a connection. |
| CG-23 | Is roleplay-only environmental storytelling sufficient for the meditation room, message board, gear room, and portal nexus? | Confirm the intended player-facing function; no Cabal QST or story sidecar currently records a task contract. |
| CG-24 | Who owns the editor data, map-generation paths, and persisted identity for these VNUMs? | Record the tooling and identity owners before any separate path move or numeric reassignment. |
