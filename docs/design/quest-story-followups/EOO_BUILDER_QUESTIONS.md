# EOO (Valhalla) builder questions

Canonical key: `component:eoo`. Physical coverage row 387 / CSV line 388;
candidate dependency row 37. The [technical audit](EOO_AUDIT_REVIEW.md)
records a ten-room Valhalla map, 29 reused room-VNUM pairs, five colliding
mobile records, two food objects, seven imported object resets, one shop, and
an unresolved Duris3 boundary.

## Explicit builder questions and recommended resolutions

| ID | Question | Recommended resolution |
| --- | --- | --- |
| EOO-01 | Who owns the Valhalla/EOO map, its editor project, source exports, and disposition? | Identify the world, area, and tool owners before changing source paths, registration, or data. |
| EOO-02 | Why is EOO absent from both full area manifests despite its WLD, MOB, OBJ, ZON, and SHP files? | Preserve the current manifest state until the loader and area owners confirm the intended deployment contract. |
| EOO-03 | Is the raw ZON title “Valhalla” the intended area name and alias? | Keep the unique title and blank AREA annotation intact until the owner records the authoritative map name. |
| EOO-04 | Is zone header #1, reset ceiling #29209, and the #29200–#29209 room band intentional together? | Verify the zone header and all reset ranges against the editor source before any renumbering. |
| EOO-05 | How should EOO coexist with dark_gh and genocide_gh, which also define every #29200–#29209 room number? | Keep all three map identities separate; none of the compared complete room blocks is identical. |
| EOO-06 | Are EOO rooms #29201–#29209 intentionally assigned the same VNUMs as active Newbie rooms? | Preserve Newbie’s active ownership and obtain an owner-approved identity mapping before any registration change. |
| EOO-07 | Why do all 22 active numeric arrivals to #29201–#29209 come from Newbie’s own WLD? | Treat them as Newbie routes, not EOO ingress; identify any intended outside EOO access separately. |
| EOO-08 | Is EOO #29200 north to Newbie #29201 an intended approach? | Preserve it as the previously reviewed source-only Ailvio approach and do not count it again. |
| EOO-09 | Is EOO #29200 up to Duris3 #179748 (“The Ocean”) intentional and one-way? | Preserve the endpoint and request an owner-approved return or destination decision; do not claim current access. |
| EOO-10 | Is EOO supposed to connect with the separate eoo_op portal maps through objects #29211–#29213? | Confirm the intended import source and keep eoo_op prototypes distinct from active Newbie objects with those numbers. |
| EOO-11 | Are Hugin #29200, Munin #29201, the serpent #29202, Fenris #29203, and Gunnar #29204 meant to coexist with same-numbered mobiles? | Preserve every typed mobile record and have each source owner reconcile the shared IDs. |
| EOO-12 | Is EOO object #29201 meant to be an apple pastry while active Newbie #29201 is a map? | Keep the two object identities separate and verify any shop/reset mapping against the intended source owner. |
| EOO-13 | Why does eoo.zon place seven objects that are not defined in eoo.obj? | Resolve each typed reset import before activation; keep all seven original reset rows and their target rooms. |
| EOO-14 | Are #11051, #11101, #11005, and #11013 the intended Player Castles objects for EOO rooms? | Preserve the Checkpoint 325 imports and ask the Player Castles and EOO owners to confirm their source roles. |
| EOO-15 | Are eoo_op portal objects #29211–#29213 intended at EOO rooms #29203 and #29206? | Confirm the cross-component portal identity, target, and return contract before changing either source. |
| EOO-16 | Should the current active Newbie prototypes #29211–#29213 be used if EOO or eoo_op stays outside the manifest? | Do not infer that the same numbers identify the intended portals; obtain an explicit object-owner decision. |
| EOO-17 | Is Gunnar’s Brunhilde story meant to be lore only, or should it have a quest record and completion outcome? | Preserve the current MOB text and do not invent a QST or reward until the content owner defines one. |
| EOO-18 | Does the Norse Valhalla text intentionally relate to Jotunheim, Storm Giants, or other mythology sources? | Treat thematic references as lore only; have the story owners document any intended chronology or shared canon. |
| EOO-19 | Does the five-M, seven-O, fourteen-G reset plan place the statues, imported props, and shop goods in the intended rooms? | Reconcile reset placement and imported item ownership with the ZON editor source before activation. |
| EOO-20 | Is the single SHP record keyed to #29204 intended to belong to Gunnar in EOO room #29204? | Preserve the current source record and resolve its keeper and room identities before loading it. |
| EOO-21 | If areas/world.shp is absent, should the non-mini fallback concatenate eoo.shp together with the eight legacy O-format shops in luiren.shp? | The fallback includes the shard by source path, but the current parser rejects O records; have shop/deployment owners provide a valid aggregate or reconcile the legacy format before claiming a successful load. |
| EOO-22 | Are all fourteen shop products intended, including #29200, #29201, and #6089? | Verify each stock prototype; #29201 is active Newbie’s map, #29200 lacks an active owner, and #6089 has no tracked OBJ definition. |
| EOO-23 | If a valid catalog loads EOO's record, should keeper #29204 and shop room #29204 resolve to active Newbie's Lady Laurel and walkway? | Under the current active tables both numbers select Newbie identities; preserve EOO's keeper and room identities separately and have both shop owners resolve the collision. |
| EOO-24 | Who can approve activation, archival, a shop-source change, or any path/identity migration? | Record the map, shop, story, tooling, deployment, and persistence owners while retaining every source in place. |

[Coverage row 387](ZONE_COVERAGE_REGISTER.csv#L387) |
[EOO audit](EOO_AUDIT_REVIEW.md) |
[candidate row 37](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L37)
