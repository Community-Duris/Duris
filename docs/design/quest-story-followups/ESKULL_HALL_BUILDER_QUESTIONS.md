# eskull_hall builder questions

Canonical key: component:eskull_hall. Checkpoint 477; coverage row 390 /
CSV line 391; candidate dependency row 40. These are source-backed decisions,
not implementation instructions. Keep this component RETAIN IN PLACE until
the map, shop, code, reset, and external-data owners resolve its identities.

## Explicit builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| ESKULL-01 | Who owns the unlisted WLD/MOB/OBJ/ZON/SHP files and their external editor data? | Record the map, shop, deployment, and source-of-truth owners before any loader or path decision. |
| ESKULL-02 | Is Ebon Skull Guildhall the canonical area title, and what aliases are recognized by builders or players? | Preserve the raw ZON title and blank AREA annotation until the owner documents the canonical name and aliases. |
| ESKULL-03 | Which story version owns Azrel, Lacha, Khayne, the Hall of Lost Souls, Archane Elders, and the lich's summoning-scroll lore? | Preserve current prose and spelling; request a content decision before connecting it to another guildhall story. |
| ESKULL-04 | Is the absence of a same-stem QST and story sidecar intentional for this story-bearing map? | Identify any external quest, editor, or narrative source before calling the story incomplete or disposable. |
| ESKULL-05 | Are zone header #293, top VNUM #29312, and room IDs #29300–#29312 the intended identity and range? | Keep the raw fields and resolve their overlap with active Newbie's zone 293 before changing numbers. |
| ESKULL-06 | How should this map relate to inactive cbr_hall, loc_gh, ravenmis, and ethdawn_op, which reuse some or all of its room range? | Ask each map owner for documented lineage; do not infer a shared map or replacement from VNUM overlap. |
| ESKULL-07 | Should active Newbie continue to own every room VNUM #29300–#29312 with its current records? | Preserve the active Newbie map identity and require an explicit owner-approved crosswalk before any merge or activation. |
| ESKULL-08 | Are the #29300–#29309 and #29310–#29312 groups intentionally separate WLD components? | Preserve both components and their authored reset groups until the map owner confirms the gap or provides a source route. |
| ESKULL-09 | Is #29300 up to inactive Duris3 #156221 The Ocean intentional, and should that boundary remain one-way? | Confirm the target identity, arrival, and any return route with both map owners. |
| ESKULL-10 | Is #29310 up to inactive Duris3 #162341 The Ocean intentional, and should it have a return path? | Preserve the authored edge while Duris3 and guildhall owners document the intended boundary. |
| ESKULL-11 | Are Azrel, Lacha, the lich, and Khayne (#29300–#29303) meant to remain distinct from active Newbie's same-number mobiles? | Keep both complete prototype sets distinct and resolve every reset and consumer by typed owner. |
| ESKULL-12 | Should candidate Khayne #29303 share a number with Newbie's bleeding man and the compiled Ailvio mobile constant? | Confirm the Ailvio and map owners' intended mobile identity before changing a VNUM or loading this MOB file. |
| ESKULL-13 | Should the compiled inn binding at room #29305 continue to target Newbie's A Secret Magic Shop, or was the candidate Falling Star Inn intended? | Have the code and area owners identify the intended room; do not redirect the callback based on this candidate alone. |
| ESKULL-14 | Should portal objects #29300/#29301 coexist with Newbie's same-number Pain dagger and sign? | Resolve the OBJ identities and all typed item consumers before either prototype can be loaded or renumbered. |
| ESKULL-15 | Do the two gateways to oblivion intentionally target #29312 and #29309 as a reciprocal pair? | Confirm the exact destination map identities; the current active table points those numbers at Newbie rooms. |
| ESKULL-16 | Are CMD_ENTER (7) and charge value -1 intended for both portals? | Confirm the stored command and negative-charge convention with the object owner before editing either prototype. |
| ESKULL-17 | Are the two Duris3 exits and the portal pair intended to connect both separate WLD components? | Document the full arrival and return design; do not bridge the components speculatively. |
| ESKULL-18 | Should the four Player Castles imports (#11056, #11106 twice, and #11013) remain in the candidate resets? | Preserve all four exact O references until the Player Castles and guildhall owners confirm their item identity and placement. |
| ESKULL-19 | Should the non-mini fallback aggregate eskull_hall.shp when areas/world.shp is absent and the shard set also contains eight legacy O-format records? | Ask loader, shop, and deployment owners how a valid catalog will be supplied or reconciled; source inclusion is not proof that the current N-only parser can load the complete stream. |
| ESKULL-20 | If a valid shop catalog loads the candidate record, should keeper #29303 and room #29302 remain distinct from the cbr_hall #29303 shop and the active Newbie keeper/room identities? | The loader supports room-scoped shop records that share a keeper prototype; preserve each source record and resolve its intended owner, keeper, and room before relying on it. |
| ESKULL-21 | Are the fifteen SHP products and matching G-reset IDs still the intended shop inventory? | Preserve the shared list until the shop owner confirms each product and its reset role. |
| ESKULL-22 | What should happen to #6089, which appears in both the SHP product list and G resets but has no current OBJ prototype? | Identify the intended item or a documented replacement before changing either reference. |
| ESKULL-23 | Were gathering bags #491–#494 intentionally added to this unlisted shop and its resets in 2026? | Confirm the 2026 guild-shop stock update applies to this source and document its loader path and keeper owner. |
| ESKULL-24 | What evidence would permit a future loader, move, rename, merge, or archive decision? | Require map/story/shop/code owners, typed reset and stock closure, external-tool ownership, and persisted-identity review; retain in place until then. |

See the [static audit](ESKULL_HALL_AUDIT_REVIEW.md), its
[connectivity findings](WORLD_CONNECTIVITY.md#checkpoint-477-eskull_hall-ebon-skull-guildhall),
and [identity/history review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-477-eskull_hall-identity-and-bounded-history).