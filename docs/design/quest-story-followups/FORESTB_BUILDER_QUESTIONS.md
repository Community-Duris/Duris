# forestb builder questions

Canonical key: component:forestb. Checkpoint 481; coverage row 394 / CSV
line 395; candidate dependency row 44. These are source-backed decisions,
not implementation instructions. Keep this source bundle RETAIN IN PLACE until
the quest, shop, manifest, and ownership questions are resolved.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| FB-01 | Who owns the forestb MOB, OBJ, QST, and SHP files, and where are their editor and deployment sources? | Record separate content, quest, shop, editor, and deployment owners before changing files or loader lists. |
| FB-02 | Is `forestb` the intended canonical title when the raw AREA annotation is blank and no ZON title exists? | Preserve the filename and blank annotation until a responsible owner supplies a title and alias crosswalk. |
| FB-03 | Should any forestb source be added to `areas/AREA` or `areas/qst/AREA`? | Have world and quest owners decide each manifest independently; file presence alone does not settle intended load state. |
| FB-04 | Is there a missing WLD/ZON source, or is this meant to be a source-only mobile, object, quest, and shop bundle? | Keep the four files intact and ask the map owner whether the local type-25 portals #38234→#38297 and #38253→#38295 depend on an omitted map; neither target has a current WLD definition. |
| FB-05 | Are the 45 mobiles #38200-#38244 the complete intended cast, including the unreset apprentice shaman/shopkeeper #38235? | Preserve every prototype and obtain a placement/reset plan before treating a mobile as unused. |
| FB-06 | Should the huge bluebird's first contract consume OBJ #38200, the red-feather staff, when its dialogue refers to meat and feathers? | Compare the exact G/R directives with intended player-facing behavior; do not change the item VNUM based on prose alone. |
| FB-07 | Is the first bluebird reward meant to be 500 coins and OBJ #38201, a white worm piece, rather than feathers? | Have the quest owner verify both reward directives and the corresponding object identities. |
| FB-08 | Is the second bluebird contract meant to require nest OBJ #38204 and reward white worm OBJ #38202? | Preserve both typed references pending a quest-owner confirmation of the dialogue and intended exchange. |
| FB-09 | Does the young woman's quest correctly consume comb OBJ #38241 and reward brooch OBJ #38260 plus 500 experience? | Cross-check the QST G/R fields, messages, and both object prototypes as one contract. |
| FB-10 | Does the shaman quest correctly consume green-spotted deerskin OBJ #38244 and reward bearhide amulet OBJ #38224 plus 500 experience? | Preserve the exact item direction and ask the quest owner to confirm the exchange. |
| FB-11 | Are all three QST givers #38200, #38230, and #38234 supposed to have no local ZON placement because no ZON file exists? | Identify the owning reset/map source for each giver; do not invent a same-stem reset file. |
| FB-12 | Should the SHP keeper be apprentice shaman mobile #38235 even though that prototype appears only in this unlisted MOB file? | Confirm that the keeper prototype is loaded before retaining or activating the shop record. |
| FB-13 | Is SHP room #38259 intended to be a room despite matching candidate object #38259, the crude wooden comb? | Require an explicit room-owner crosswalk and preserve room and object identities as separate types. |
| FB-14 | Are the four shop products #38231, #38246, #38248, and #38247 and their prices/stock settings intentional? | Verify all four stock entries against their candidate OBJ records and intended shop catalog. |
| FB-15 | Should the single forestb SHP record be ingested by the non-mini fallback when `areas/world.shp` is absent? | The fallback globs unlisted shards, but this checkout also supplies eight legacy `O`-format Luiren records rejected by the current `N`-only parser; define a valid, supported catalog input before relying on inclusion. |
| FB-16 | How should the shop loader handle a keeper VNUM whose `real_mobile` lookup returns -1? | Preserve the #38235 source identity and require a validity guard before any complete catalog can assign a missing keeper; the current shard fallback is not evidence that this later assignment path completes. |
| FB-17 | Does any external typed record depend on forestb mobiles, objects, quest givers, or shop IDs? | Checkpoint 893 found no direct match in 442 ZON files, 264 other QST files, 232 sidecars, the 711 other parser-valid N shops, or external portal/switch room targets. Keep editor, dynamic, deployment, and persisted-data closure open. |
| FB-18 | Are same-number records such as MOB/OBJ #38200 and OBJ/room-number #38259 intentional cross-type reuse? | Keep #38234's shaman mobile/QST-giver role separate from the blood-pentagram OBJ and keep #38259's shop-room role separate from the OBJ comb; document each type and never merge identities by number alone. |
| FB-19 | Are the 62 objects #38200-#38261 the complete intended item set, including quest items and all stock products? | Preserve the current prototypes and resolve each QST/SHP item role with the object owner. |
| FB-20 | Is the forestb story text still the intended tribal-village, forest-wildlife, and shaman quest narrative? | Ask narrative and area owners to review the source text and assign any separate story-sidecar owner. |
| FB-21 | Do the bluebird food/feather exchanges and the two village item handoffs match the current prose? | Record approved dialogue-to-directive mappings for all four Q contracts before editing their text or item goals. |
| FB-22 | Is there a same-stem predecessor, generator, editor map, or replacement source outside the four current files? | Keep all current files; require affirmative history or owner evidence before calling any source a duplicate or replacement. |
| FB-23 | What runtime or deployment path, if any, is intended to create or expose portal destinations #38295/#38297, place the keeper, and expose this shop? | Record that path and its owner; neither portal target has a current WLD definition, and static source evidence does not establish runtime behavior. |
| FB-24 | What evidence would permit a future activation, move, rename, archive, or merge proposal? | Require map/quest/shop ownership, manifest and editor/deployment review, typed reference closure, and persisted-ID signoff; retain in place until then. |

See the [static audit](FORESTB_AUDIT_REVIEW.md),
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-481-forestb-source-bundle-and-shop-references),
and [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-481-forestb-title-history-and-typed-identities).
