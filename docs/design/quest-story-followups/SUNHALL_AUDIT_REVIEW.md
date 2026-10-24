# sunhall source-component audit

Canonical key: `component:sunhall`. Coverage data row 446 / CSV line 447;
candidate dependency data row 95 / CSV line 96. Reviewed October 11, 2026,
Checkpoint 533, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
manifest, graph, typed-reference, identity, shop-loader, and followed-history
review. It does not establish that the unlisted component is loaded or that
its exits, resets, or shop are usable.

## Inventory, title, and manifest status

The five-file component has ten WLD rooms #30200–#30209 in
[sunhall.wld](../../../areas/wld/sunhall.wld#L1), four MOB prototypes
#30200–#30203 in [sunhall.mob](../../../areas/mob/sunhall.mob#L1), five OBJ
prototypes #30200–#30204 in [sunhall.obj](../../../areas/obj/sunhall.obj#L1),
25 ZON reset commands in
[sunhall.zon](../../../areas/zon/sunhall.zon#L1), and one SHP record keyed
#30201 in [sunhall.shp](../../../areas/shp/sunhall.shp#L1). The ZON header is
#302, the raw title is “Dark Sun Guild Hall,” the last room is #30209, and
reset mode is 0. The normalized title occurs only in this file among the
442 current ZON sources. Header #302 is also used by unlisted `bastard.zon`,
whose raw title is “Die hall”; that header collision does not show common
ownership or replacement.

The WLD describes a pyramid entrance, sphinx guardians, a tomb, an
adventurers' shoppe, an inn named “Desert Sand,” a sparring arena, a shrine
of the Sun God, and a sand-veiled hall. The ZON places four local mobile
prototypes, and the shop lists 17 product VNUMs. There is no same-stem QST or
story sidecar. These texts provide setting and encounter details, not proof
of a quest or current story connection. `sunhall` is absent from
[areas/AREA](../../../areas/AREA),
[areas/qst/AREA](../../../areas/qst/AREA), and the empty current
[RANDOM_AREA](../../../areas/RANDOM_AREA).

## Candidate map and current numeric resolution

The ten-room WLD has 19 authored exits. Eighteen target local Sunhall numbers
and form one weak component. All ten rooms are forward-reachable from #30200
and have an authored route back to #30200. The remaining edge is D4 up from
#30200 to #158571.

| Source room | Sunhall room text | Authored exits |
| --- | --- | --- |
| #30200 | Enormous Gates before a Forgotten Pyramid | D0 → #30201; D4 → #158571 |
| #30201 | Damp Triangular Hall | D0 → #30202; D2 → #30200; D3 → #30204 |
| #30202 | Center of the Pyramid | D1 → #30208; D2 → #30201; D5 → #30203 |
| #30203 | Silent Tomb Filled With Swirling Mist | D4 → #30202 |
| #30204 | Small Gathering Area | D0 → #30205; D1 → #30201; D4 → #30206 |
| #30205 | Old Adventurers Shoppe | D2 → #30204 |
| #30206 | Dusty Inn Named “Desert Sand” | D1 → #30207; D5 → #30204 |
| #30207 | Sparring Arena Covered With Blood | D3 → #30206 |
| #30208 | Ancient Chamber, Shrine of the Sun God | D3 → #30202; D5 → #30209 |
| #30209 | Empty Hall Veiled in Smooth Sand | D4 → #30208 |

Current numeric lookup resolves all 18 internal targets to different
active `underworld3.wld` records. Active Underworld3 owns every room number
#30200–#30209 and describes old dwarven mining tunnels there; the WLD exits
and prose are different from Sunhall's pyramid/guildhall map. The external
target #158571 is “Mountains” only in
[unlisted Duris3.wld](../../../areas/wld/Duris3.wld#L1229992). Its authored
exits lead to other Duris3 rooms and none returns to #30200–#30209. There is
no active WLD ingress to a Sunhall identity. Twenty-two active Underworld3
WLD exits also target #30200–#30209, but current numeric ownership makes
them Underworld3-to-Underworld3 links. Four active Underworld3 ZON M rows
place Underworld3 mobiles in numeric rooms #30202, #30205, and #30209. These
are active Underworld3 placements, not Sunhall resets or identity ingress.
There are no negative targets, source rooms outside the numeric zone range,
range-mismatch exits, fall-chance fields, or current fields in the candidate
WLD.

Unlisted `bastard.wld` independently defines #30200–#30204 as Yggdrasil,
Durithalite cave, Ilixctthu's alcove, the Blood and Skull Tavern, and its
barracks. Those five records differ from both Sunhall and active Underworld3.
They are separate map identities, not proof of supersession.

## Same-number prototypes, typed resets, and shop stock

Sunhall MOB #30200–#30203 has no matching active Underworld3 prototype.
Unlisted `bastard.mob` defines different #30200–#30202 records (giant guards
and Ilixctthu); Sunhall's records are three Sphinx Golem variants and Ahash
Bu'bazul. No same-stem QST or exact numeric source-code callback for
#30200–#30209 was found.

Sunhall OBJ #30200–#30204 has no matching active prototype. The unlisted
Bastard OBJ #30200 is a different badge/symbol. Sunhall's local objects are
a black chalice, waterfall statue, sarcophagus, meatballs, and a scorched
pendant badge. The ZON only places/gives #30203 and #30204, and the SHP only
stocks those two local products; #30200–#30202 are defined but not referenced
by the candidate ZON or shop.

The 25 ZON commands comprise three O rows, five M rows, and 17 G rows.

- O rows 9–11 import active Player Castles objects #11058 (magic mouth),
  #11108 (board), and #11013 (bonding altar) into Sunhall rooms #30200,
  #30204, and #30206. Preserve these three typed imports. The prototypes are
  in [player_castles.obj](../../../areas/obj/player_castles.obj#L121),
  [line 185](../../../areas/obj/player_castles.obj#L185), and
  [line 305](../../../areas/obj/player_castles.obj#L305).
- M rows 13–17 reset #30202, #30200, and #30203 twice in room #30200, then
  Ahash mobile #30201 in room #30205. All four prototypes are source-only;
  active Underworld3 has no exact mobile prototype for these numbers.
- G rows 18–34 place local meatballs #30203 and pendant #30204 plus imported
  objects #3050, #377, #398, #6089, #364, #53, #412, #347, #108, #109,
  #111, and #491–#494. The 15 external IDs also appear in the shop's stock
  list. Fourteen have active object prototypes: #3050 in Verz1; #347,
  #377, #398, #364, #412, #108, #109, #111, and #491–#494 in Heavens; and
  #53 in Heavens. #6089 has no OBJ definition in the current source set.
  #53 also has different unlisted Dalvik, Hostel, and Mavulsk objects;
  #108/#109 have different unlisted Mavulsk variants. Current active object
  lookup for #53/#108/#109 selects the Heavens prototypes.

SHP #30201 has the same 17 product VNUMs as the ZON G rows: local #30203
and #30204 plus those 15 external IDs. Its keeper is #30201 (Sunhall MOB
Ahash), and `in_room` is #30205 (Sunhall's Old Adventurers Shoppe). In the
active manifest, keeper #30201 has no exact MOB prototype and room #30205
resolves to an active Underworld3 mining-tunnel room. External shop object
#6089 is missing from the current OBJ source set, and local products
#30203/#30204 are not in the active OBJ table.

The unlisted `bastard.shp` also has a record keyed #30201 and uses keeper
#30201, but its stock and `in_room` #30202 differ; its source mobile #30201
is Ilixctthu rather than Sunhall's Ahash. Because `areas/world.shp` is absent
from this checkout, full-mode `open_shop_stream()` can glob
`areas/shp/*.shp`, including both files. `boot_the_shops()` resolves each
keeper with `real_mobile()`; the current table has no #30201 prototype, and
[real_mobile()](../../../src/world/db.c#L8587) returns -1 when no exact
mobile exists. [assign_the_shopkeepers()](../../../src/economy/shop.c#L3602)
then uses the resolved value at
[unguarded mob_index access](../../../src/economy/shop.c#L3623), and
[specs.assign.c](../../../src/specs/specs.assign.c#L1261) calls that
assignment after shop boot. This is a conditional static loader hazard, not
evidence that either shop booted. The two same-key SHP records have separate
source contents and owners to resolve before any loader or shop claim.

## Followed history, disposition, and limits

Bounded `git log --follow` history finds the WLD last edited in 2013
(`71d731c5b`); MOB, OBJ, and ZON also show the 2012 format-field update
(`bb0d59425`) and the 2009 area-tree move (`642da5400`). SHP history shows
the 2013 edit and 2009 move. ZON and SHP were both edited September 6, 2026
(`ed94862f0`) to add Heavens bag objects #491–#494 to the reset and shop
lists. This is recent source maintenance, not evidence of current manifest
membership, activation, or replacement. No explicit predecessor, successor,
retirement, or archive decision was found in the followed file history.

Retain all five source files IN PLACE. Resolve active Underworld3 and
unlisted Bastard map identities; the Duris3 #158571 outlet; local MOB/OBJ
ownership; three Player Castles imports; missing object #6089; #53/#108/#109
alternate object sources; bag-update provenance; and the duplicate #30201
shop records, missing active keeper, and active numeric room binding with
area, shop, deployment, and tooling owners before any separate move, rename,
merge, activation, or archive decision. Editor, export, persisted-identity,
and deployment references were not available for this static review.

See the [20 builder questions](SUNHALL_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md#checkpoint-533-sunhall-pyramid-map-and-underworld3-number-resolution),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-533-sunhall-underworld3-and-bastard-source-identities),
[retention checkpoint](ATTIC_MIGRATION_PLAN.md#checkpoint-533-sunhall-retain-map-shops-and-imports),
[related Bastard audit](BASTARD_AUDIT_REVIEW.md#typed-resets-shop-stock-and-overlaps),
[candidate dependency row 95](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
and [coverage row 446](ZONE_COVERAGE_REGISTER.csv#L447).
