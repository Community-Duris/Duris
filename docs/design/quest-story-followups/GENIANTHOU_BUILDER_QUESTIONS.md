# Gimluk’s Cottage builder questions

Canonical key: component:gianthou. Baseline checkpoint 486; role-aware
supplement checkpoint 898; coverage row 399 / CSV line 400; candidate
dependency row 49 / CSV line 49. These are source-backed owner decisions,
not implementation instructions. Keep gianthou RETAIN IN PLACE until its
area range, map, story, reset, and shop owners resolve them.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| GI-01 | Who owns the six gianthou files and any external editor, staging, or deployment copies? | Record map, story, quest, reset, shop, editor, and deployment owners before a path or activation decision. [Source inventory](GENIANTHOU_AUDIT_REVIEW.md#source-inventory-and-range-ownership) |
| GI-02 | Should gianthou be registered in areas/AREA, areas/qst/AREA, both, or neither? | Have world and quest owners decide independently; it is absent from both current lists. [areas/AREA](../../../areas/AREA#L201), [qst/AREA](../../../areas/qst/AREA#L185) |
| GI-03 | Who owns #426–#428 when Pworm is listed through #426 and #427–#428 are marked open? | Resolve the numeric band with the area-list owner before adding or activating gianthou. [Pworm and open bands](../../../areas/AREA#L201) |
| GI-04 | Is “Gimluk’s Cottage” the canonical title, and what raw AREA annotation or aliases should be recorded? | Preserve the uniquely found ZON title and blank annotation until the area owner records a canonical alias. [ZON](../../../areas/zon/gianthou.zon#L1) |
| GI-05 | Are the 220 blank WLD room descriptions intentional, or are map descriptions unfinished? | Preserve current text and have the map owner identify any rooms requiring authored prose. [WLD](../../../areas/wld/gianthou.wld#L1) |
| GI-06 | Who owns the cottage, captive-dwarf, and Gimluk story continuity, given that there is no story sidecar? | Keep the source dialogue intact and record a story owner before archive or narrative edits. [MOB](../../../areas/mob/gianthou.mob#L1), [QST](../../../areas/qst/gianthou.qst#L1) |
| GI-07 | Are the atrium bug and snake errands meant to be the opening quest path and key progression? | Preserve the QST conditions and rewards until the quest owner confirms the intended order. [QST](../../../areas/qst/gianthou.qst#L36) |
| GI-08 | Is Gimluk’s origin and his son’s “pet” dwarf captivity the intended reason for the revenge request? | Keep the source’s language and ask the story/quest owner to confirm its role and tone. [QST](../../../areas/qst/gianthou.qst#L69) |
| GI-09 | How should Gamdin, Djarill, and the key’s escape history connect to the wider dwarf story? | Preserve these named references and document their continuity owner before changing the quest text. [QST](../../../areas/qst/gianthou.qst#L23) |
| GI-10 | Is killing Gimluk and returning his redwood stick the intended completion proof and reward path? | Keep the current request and completion condition until the quest owner confirms the contract. [QST](../../../areas/qst/gianthou.qst#L71) |
| GI-11 | Should the reward dialogue send the player to Renthis’la for weapon enchanting? | Confirm that the named NPC and handoff remain a supported follow-up. [QST](../../../areas/qst/gianthou.qst#L82) |
| GI-12 | Is Ugala’s refusal meant to preserve a no-theft/no-slaughter branch involving Gimluk’s children? | Keep the refusal and its item handling intact; confirm how it ends or redirects the quest. [Ugala QST](../../../areas/qst/gianthou.qst#L173) |
| GI-13 | Is #42600 the intended entry point for all 271 rooms in the one connected local graph? | Preserve the map and confirm the intended starting point and progression with the map owner. [WLD](../../../areas/wld/gianthou.wld#L1) |
| GI-14 | What should happen to #42811 D0, whose authored target is #-1? | Keep the source exit intact until its intentional sentinel/blocking behavior is confirmed. [WLD exit](../../../areas/wld/gianthou.wld#L3208) |
| GI-15 | Are the 20 positive WLD F fall chances and absence of current fields intentional, and should the separate 38 ZON F commands remain follower resets? | Preserve room hazards and reset followers as distinct authored fields and confirm both sets with their owners. [WLD fall example](../../../areas/wld/gianthou.wld#L401), [WLD parser](../../../src/world/db.c#L1395), [ZON followers](../../../areas/zon/gianthou.zon#L6813), [ZON parser](../../../src/world/db.c#L7747) |
| GI-16 | Which unlisted map definition is intended for #274141, surf’s grassy plains or surface2011’s stormy sea? | Obtain a target identity and manifest decision from both map owners before describing the exit as usable travel. [surf](../../../areas/wld/surf.wld#L1555793), [surface2011](../../../areas/wld/surface2011.wld#L1556962) |
| GI-17 | Is the one-way #42870 D3 edge intentional, and should the selected target map add a return route? | Preserve the source-only edge and confirm both directions, safety, and map ownership. [gianthou exit](../../../areas/wld/gianthou.wld#L4563), [connectivity review](WORLD_CONNECTIVITY.md#checkpoint-486-gimluks-cottage-map-and-boundaries) |
| GI-18 | Is reset mode 2 and the full 7,136-command ZON set intentional? | Preserve its mode and typed command inventory until the reset owner reviews it. [ZON header](../../../areas/zon/gianthou.zon#L1) |
| GI-19 | How should maintainers review the 6,634 P object-in-object placements as one closed set? | Keep every parent, child, limit, and local prototype relationship until a scoped reset review confirms them. [Reset example](../../../areas/zon/gianthou.zon#L141) |
| GI-20 | Is MOB #42618 intentionally unused by M/F resets and QST giver records? | Keep the prototype and ask the mobile owner whether it is reserved, omitted from resets, or obsolete. [MOB prototype](../../../areas/mob/gianthou.mob#L206) |
| GI-21 | Are all four shop keeper/room pairs and their 28 product relationships intended? | Preserve all four records and confirm each keeper, room, product list, and local object identity with the shop owner. [SHP](../../../areas/shp/gianthou.shp#L1) |
| GI-22 | What eligibility rule is expressed by “I do not sell to your people!” in the #42637 shop? | Preserve the refusal text and have the shop owner document its intended customer rule. [SHP](../../../areas/shp/gianthou.shp#L1) |
| GI-23 | Can the wildcard shop-file fallback read gianthou.shp while its keeper mobiles remain unlisted? | Preserve the four source shop records and resolve all keeper indices before including this SHP. The external 708 parser-shaped N-format shops have no exact keeper, room, or product role to gianthou, but the source fallback and candidate keeper availability remain conditional. [Checkpoint 898 audit](GENIANTHOU_AUDIT_REVIEW.md#checkpoint-898-complete-typed-cross-source-closure), [fallback](../../../src/economy/shop.c#L3276), [keeper lookup](../../../src/economy/shop.c#L3492), [assignment](../../../src/economy/shop.c#L3602) |
| GI-24 | What signoffs are required before activation, move, rename, archive, or merge? | Require Pworm/open-band, map and #274141 target/return-path, story/quest, reset/shop, manifest/editor, cross-source typed-closure, and persisted-ID signoff; retain in place until then. [Checkpoint 898 audit](GENIANTHOU_AUDIT_REVIEW.md#checkpoint-898-complete-typed-cross-source-closure), [retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-898-gianthou-retains-source-only-boundaries-and-typed-identities) |

See the [static audit](GENIANTHOU_AUDIT_REVIEW.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-486-gimluks-cottage-map-and-boundaries),
and [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-486-gimluks-cottage-identities-and-history).
