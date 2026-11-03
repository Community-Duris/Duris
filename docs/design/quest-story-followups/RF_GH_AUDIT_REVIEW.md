# rf_gh source-component audit

Canonical key: component:rf_gh. Physical coverage row 439 / CSV line 440;
candidate dependency data row 88 / CSV line 89. Reviewed October 11, 2026,
Checkpoint 526, against checkout c500ac51b8cd26540943257e7ca4e9807263ef74.
This is a static source, topology, loader-path, and typed-reference review.
It does not claim that the full area tables or shop fallback have been booted.

## Source inventory, title, and membership

The unlisted source set consists of [rf_gh.wld](../../../areas/wld/rf_gh.wld#L1)
(13 rooms, #28900–#28912), [rf_gh.mob](../../../areas/mob/rf_gh.mob#L1)
(7 mobiles, #28900–#28906), [rf_gh.obj](../../../areas/obj/rf_gh.obj#L1)
(5 objects, #28900–#28904), [rf_gh.zon](../../../areas/zon/rf_gh.zon#L1)
(one zone header, reset mode 0, last VNUM #28912; 34 reset commands), and
[rf_gh.shp](../../../areas/shp/rf_gh.shp#L1) (one shop record, #28904).
The raw zone title is “The GuildHall of Circle of Yggdrasil.” There is no
same-stem QST or story sidecar.

rf_gh is absent from [areas/AREA](../../../areas/AREA), [areas/qst/AREA](../../../areas/qst/AREA),
and the empty current areas/RANDOM_AREA. The listed Torg component owns the
same numeric range in [areas/AREA, line 140](../../../areas/AREA#L140) and
[qst/AREA, line 130](../../../areas/qst/AREA#L130). The expected generated
areas/world.wld, world.mob, world.obj, world.zon, and world.shp files are
absent from this checkout. areas/shp/AREA is also absent. Those absences limit
runtime conclusions, but the shop loader has a distinct per-file fallback
described below.

## Room graph and exits

The WLD contains 24 directed exit records: 22 target room numbers in the
source's own #28900–#28912 set, plus two external positive targets. The 22
internal target references form two weak components: #28900–#28909 (10 rooms)
and #28910–#28912 (3 rooms). No internal exit joins those groups.

| Source room | Authored exits |
| --- | --- |
| #28900, The Organ Pit | D0 → #28901; D4 → #111082 |
| #28901 | D2 → #28900; D5 → #28902 |
| #28902 | D4 → #28901; D5 → #28903 |
| #28903 | D2 → #28904; D4 → #28902; D5 → #28905 |
| #28904, The Lonely Coat Hotel | D0 → #28903 |
| #28905 | D0 → #28906; D4 → #28903 |
| #28906 | D0 → #28907; D2 → #28905 |
| #28907 | D2 → #28906; D5 → #28908 |
| #28908 | D0 → #28909; D4 → #28907 |
| #28909, Bone Hut in the Pit | D2 → #28908 |
| #28910, desecrated-cathedral approach | D0 → #28911; D4 → #154153 |
| #28911, cathedral | D2 → #28910; D5 → #28912 |
| #28912, altar | D4 → #28911 |

The #28900 D4 outlet reaches #111082. That room is “The DragonMaw
Mountains” in listed tharnrifts.wld; the same VNUM also occurs as “The Ocean”
in unlisted Duris3.wld. The current areas/AREA entry for tharnrifts is
[line 366](../../../areas/AREA#L366), while Duris3 is not listed. The #28910
D4 outlet reaches #154153, found only as “The Ocean” in unlisted Duris3.wld.
No reverse exits to the rf_gh room numbers were found in either target source.
Thus one external target resolves numerically through the current area list
(to tharnrifts), while #154153 remains unresolved through that list; neither
outlet is reciprocal.

All 22 internal exit targets also resolve numerically to rooms in listed
torg.wld, but those are different room identities, not rf_gh rooms. This is
why the row records 23 loader-resolved target numbers without treating the
candidate map as an active destination. The listed surface.wld has an exit
from #611053 to #28900; listed torg.wld returns from #28900 to #611053.
Those reciprocal rows connect current surface and Torg. #28900 is the
listed Torg “Thin Trail” room, not an inbound route to the absent rf_gh
Organ Pit identity. Unlisted surf.wld has another numeric edge from #277211
to #28900, which does not establish a loaded route. No negative exits,
fall-chance rooms, or current rooms were found.

## Same-type identities and history

Every room VNUM #28900–#28912, mobile VNUM #28900–#28906, and object VNUM
#28900–#28904 in rf_gh has a different same-type prototype in listed Torg
sources. Examples are:

| Type and VNUM | rf_gh identity | Listed Torg identity |
| --- | --- | --- |
| WLD #28900 | The Organ Pit | A Thin Trail Winding Up the Mountain |
| MOB #28904 | Wretched vampire shopkeeper | Duergar miner |
| OBJ #28902 | Rotting Flesh sigil | Hand axe |

This is a 25-ID same-type overlap across the component's WLD, MOB, and OBJ
files, not evidence that Torg replaces rf_gh. The two WLD paths were both
changed by the same November 13, 2013 “stuff” commit; the followed history
has no explicit predecessor, successor, or retirement decision. Mini-area
scans found no matching VNUMs. The only exact candidate-number occurrence in
C/C++ source is #28911 in the generated Chaos equipment VNUM table
([chaos_eq_data.h](../../../src/account/chaos_eq_data.h#L82)); it is an item
VNUM matching Torg OBJ #28911, not a room route. Torg's QST also uses #28906
and #28911 in typed item rows, separate from rf_gh's room identities.

## Reset imports and shop-loader path

rf_gh.zon has 9 O, 9 M, and 16 G commands. The 9 O commands include four
local object placements and five source-only imports of Player Castles
objects: #11058 twice, #11013 once, and #11108 twice. The 9 M commands use
local same-stem mobile IDs. The 16 G commands include the local #28902 sigil
and 15 imported objects: #364, #384, #377, #3050, #398, #53, #108, #109,
#111, #203, #204, and #491–#494. These imports are source reset rows, not
active ZON resets for rf_gh. Preserve the exact Player Castles references
already called out in the dependency register. The remaining imported item
definitions are in Heavens or Verz1; #53 additionally has an unlisted Dalvik
same-type definition (“dazzling sapphire bracelet”) beside active Heavens
OBJ #53 (“rope coil hemp”). Resolve which identity the reset and shop stock
intend before changing either source.

The one SHP record has shop header #28904, keeper VNUM #28904, and
in_room #28909; its product list includes #28902 and 15 imported object
VNUMs. The source MOB #28904 is the wretched vampire placed in candidate
room #28909, but current Torg uses MOB #28904 for a duergar miner, WLD #28909
for a corridor, and OBJ #28902 for a hand axe.

In full mode, [shop.c](../../../src/economy/shop.c#L3267) first opens
SHOP_FILE, defined as areas/world.shp in
[db.h](../../../src/world/db.h#L22), then falls back to concatenating every
areas/shp/*.shp file when that aggregate cannot be opened. The fallback
includes rf_gh.shp; areas/world.shp is absent here. The parser resolves
products with real_object at [shop.c, line 3380](../../../src/economy/shop.c#L3380)
and keeper VNUMs with real_mobile at [line 3493](../../../src/economy/shop.c#L3493);
assign_the_shopkeepers is later called from
[specs.assign.c](../../../src/specs/specs.assign.c#L1261). If this conditional
full-mode path receives the current active Torg prototype tables, rf_gh's
shop could bind #28904 to the Torg duergar miner, #28909 to the Torg
corridor, and product #28902 to the Torg axe. That is a static loader and
same-type-resolution risk, not an observed boot result or proof that the
shop is currently available.

## Disposition and limits

The CP527 comparison with [rf_op](RF_OP_AUDIT_REVIEW.md) found that its
three-room cathedral segment reuses this component's closing #28910–#28912
scene at #29510–#29512. The matching rooms, mobiles, and portal-object text
support fragment reuse, while changed portal destinations and separate
unlisted source identities leave the component relationship unresolved.
This does not establish that rf_op replaces rf_gh or vice versa.

Keep all five rf_gh files RETAIN IN PLACE. Resolve the shared VNUM ownership,
both disconnected map groups and external exits, Player Castles imports, the
#53 duplicate item, and the conditional SHP fallback mapping with the area,
shop, and deployment owners before any separate activation, rename, move,
merge, or archive proposal. No migration or activation clearance is implied.
See the [21 builder questions](RF_GH_BUILDER_QUESTIONS.md), [connectivity
checkpoint](WORLD_CONNECTIVITY.md), [identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
and [retention checkpoint](ATTIC_MIGRATION_PLAN.md).

## Checkpoint 930: reciprocal portal objects cross the two WLD groups

`rf_gh.obj` defines two type-25 `ITEM_TELEPORT` objects and no ITEM_SWITCH. Their `value[0]` fields are room VNUMs: object #28903's type/value rows at [rf_gh.obj](../../../areas/obj/rf_gh.obj#L46) target room #28912, and object #28904's rows at [rf_gh.obj](../../../areas/obj/rf_gh.obj#L65) target room #28905. The travel code reads `value[0]` and resolves it with `real_room` ([spell_travel.c](../../../src/magic/spell_travel.c#L991)); type 25 is `ITEM_TELEPORT` ([defines.h](../../../src/core/defines.h#L140)). The ZON source places #28903 in room #28905 and #28904 in room #28912 ([reset #28903](../../../areas/zon/rf_gh.zon#L13), [reset #28904](../../../areas/zon/rf_gh.zon#L17)); those candidate rooms are [WLD #28905](../../../areas/wld/rf_gh.wld#L93) and [WLD #28912](../../../areas/wld/rf_gh.wld#L236). These are reciprocal authored destinations between the WLD-only component #28900–#28909 and the WLD-only component #28910–#28912.

The reciprocal object pair is separate from the 24 authored WLD directions and does not change the two-component WLD graph or active inbound count. The candidate remains unlisted, its ZON reset mode is 0, and generated world tables are absent; this source data does not prove either portal loads or can be used.

Each portal object number and destination room number also resolves to a different listed Torg prototype in the current full-area source set: Torg OBJ #28903 is iron cuffs and OBJ #28904 is a bent longsword ([OBJ #28903](../../../areas/obj/torg.obj#L40), [OBJ #28904](../../../areas/obj/torg.obj#L56)), while Torg WLD #28905 and #28912 are corridors ([WLD #28905](../../../areas/wld/torg.wld#L102), [WLD #28912](../../../areas/wld/torg.wld#L248)). The candidate's inter-component teleport pair therefore depends on four source identities that collide by type and VNUM with Torg; same-number Torg records do not provide the RfGh portal objects or rooms.

Torg's QST and story records use item VNUMs #28906 and #28911 ([QST #28906](../../../areas/qst/torg.qst#L52), [QST #28911](../../../areas/qst/torg.qst#L128), [story item refs](../../../areas/story/torg.story.json#L51)), matching current Torg OBJ #28906 (severed tentacle) and OBJ #28911 (dracolich-hide bracelet) ([OBJ #28906](../../../areas/obj/torg.obj#L89), [OBJ #28911](../../../areas/obj/torg.obj#L160)), not the rf_gh mobile or room records with those numbers. The Chaos equipment data also carries #28911 in an item slot ([chaos_eq_data.h](../../../src/account/chaos_eq_data.h#L82)). These are typed item references, not additional inbound to the candidate map. Keep all five files RETAIN IN PLACE and resolve portal, room, reset, and shared-table ownership before any activation or migration proposal. The follow-up adds question 21 to [RF_GH_BUILDER_QUESTIONS.md](RF_GH_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
