# Drifting Realm repository-wide audit supplement

This supplement reconciles the active Dream WLD and cross-area boundary, all-file inactive-map scan, typed quest/reset/shop references, compiled prototype references, and bounded title/history evidence with the [source-comprehensive Drifting Realm dossier](../zone-stories/DRIFTING_REALM.md). The frozen source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. No area or gameplay source is changed by this audit.

## Active graph, boundary, and non-WLD travel

The active [Dream WLD](../../../areas/wld/dream.wld#L1) has 27 rooms (#31300–#31326) and 46 authored exits; all 46 resolve. There are 45 local exits and one outbound cross-area exit. The local graph has weak components of 24 and 3 rooms. Raw directed traversal from #31300 reaches and returns from only 3/27 rooms. The authored WLD boundary is reciprocal with Ethereal: Dream #31301 up → #12442 and Ethereal #12442 down → #31301 ([Dream exit](../../../areas/wld/dream.wld#L29), [Ethereal return](../../../areas/wld/ethereal_main.wld#L1759), [outbound pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L121), [inbound pair row](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L133)). There are no unresolved or negative exits, falls, or current fields. No active room-number duplicate overlaps Dream #31300–#31326.

The WLD graph does not capture the authored item travel between its components. Active Arcium reset #85725 places a type-25 fluffy bed at room #85805; its `ENTER` target is Dream room #31300 ([Arcium reset](../../../areas/zon/arcium.zon#L109), [bed definition and target](../../../areas/obj/arcium.obj#L348)). In Dream, the flickering light #31300 uses `LOOK` to target #31302, while solid light #31301 and cliff #31303 target Arcium #85805 with their own commands ([flickering light](../../../areas/obj/dream.obj#L1), [solid light](../../../areas/obj/dream.obj#L13), [cliff](../../../areas/obj/dream.obj#L40)). One of 869 outside type-25 declarations targets a Dream room: this Arcium bed. The dossier details object selection, command matching, dispatch, charges, and why configured travel is not proof of arrival or a safe return. The whirlpool #31302 is a type-17 drink container, not a second travel control.

## Inactive-map sweep

The all-95 inactive-WLD scan parsed 527,791 rooms and 2,080,773 exits without parser errors. It found no source-room VNUM overlap with Dream and no inactive exit target into rooms #31300–#31326. No source-only approach is added. Curated source-only totals remain 2,798 overall and 156 for the reviewed Surf subset. This static map result does not qualify the non-WLD Arcium bed/light/cliff controls or their return safety.

## Typed quest, reset, shop, and story references

Dream's own Q27 requests four ethereal soul shards #31311; Q46 requests one each of five exact skulls #31316–#31320. The local QST rows preserve those independent exact recipes ([Q27 shard rows](../../../areas/qst/dream.qst#L42), [Q46 skull rows](../../../areas/qst/dream.qst#L52)). Dream resets local skull #31316 beside the inner voice. The other four local skull prototypes are stocked by active outside reset sources: #31317 in `mril.zon`, #31318 in `lava.zon`, #31319 in `arcium.zon`, and #31320 in `fishermans_wharf.zon` ([Miaeril reset](../../../areas/zon/mril.zon#L92), [Lava reset](../../../areas/zon/lava.zon#L180), [Arcium reset](../../../areas/zon/arcium.zon#L272), [Wharf reset](../../../areas/zon/fishermans_wharf.zon#L138); [Dream skull definitions](../../../areas/obj/dream.obj#L225)). The complete selected holders, source rooms, reset parents, and native item restrictions are in the dossier; source placement alone does not prove encounter or hand-in availability.

Dream also G-resets Alatorin dreamfoil #83283 from its inner voice ([Dream reset](../../../areas/zon/dream.zon#L139), [Alatorin definition](../../../areas/obj/alatorin.obj#L2864), [Alatorin stock](../../../areas/zon/alatorin.zon#L2581)). This is a foreign-owned ingredient used in the separate Alatorin flower return, as described in the [Alatorin dossier](../zone-stories/ALATORIN.md); it does not transfer that quest's receipt to Dream.

No active outside QST item command or foreign SHP product targets a Dream prototype. Dream's local [shop source](../../../areas/shp/dream.shp#L1) lists its tarot pack #31314. An outside story-sidecar scan found one occurrence of #31320 in a Fishermans Wharf contact description ([prose reference](../../../areas/story/fishermans_wharf.story.json#L146)); it is a string-token explanation of the Dream quest, not a typed item-VNUM field. The remaining 230 outside story sidecars have no exact numeric token hit, and all 231 parsed without errors. No external mobile or object prototype overlaps Dream's local 18 mobiles or 21 objects.

The outside type-25 scan found no second target into Dream. All 869 outside declarations were checked; only Arcium bed #85725 targets room #31300. Dream's external ZON reset #83283 is separately owned by Alatorin. No migration inference follows from either relationship.

## Compiled references

Fifty-nine C/C++ source lines containing exact Dream VNUMs are all in `chaos_eq_data.h` table data. The arrays reference inner-vision spectacles #31306, necromantic shroud #31312, ethereal soul-shard suit #31313, the tarot pack #31314 twice in one shared support-consumable entry, and swirling-force badge #31315 ([spectacles](../../../src/account/chaos_eq_data.h#L74), [shroud](../../../src/account/chaos_eq_data.h#L109), [suit](../../../src/account/chaos_eq_data.h#L243), [shared tarot entries](../../../src/account/chaos_eq_data.h#L581), [badge](../../../src/account/chaos_eq_data.h#L75)). The CHAOS new-character kit path reads the configured profile VNUMs with `read_object()` ([item loader](../../../src/account/nanny.c#L634), [profile application](../../../src/account/nanny.c#L709)); this is a source-level item-issuance dependency, not proof that an individual character received an item. No exact compiled reference to the soul shard #31311 or skulls #31316–#31320 was found, and no direct numeric special or epic-teacher assignment keyed to a Dream prototype was found. These tables do not construct Qin's receipts.

## Title, history, and disposition

After stripping color tokens and normalizing whitespace and case, a fresh scan of all 442 current `.zon` title lines finds one exact “Drifting Realm” owner: `dream` ([full-area entry](../../../areas/AREA#L152), [QST entry](../../../areas/qst/AREA#L141), [title source](../../../areas/zon/dream.zon#L2)). The only other current titles containing “dream” are “Ard'gral, plane of dreams” in `ardgral` and “The Fortress of Dreams” in `eth2` ([Ard'gral title](../../../areas/zon/ardgral.zon#L2), [Fortress title](../../../areas/zon/eth2.zon#L2)); these are distinct titles and owners.

Followed WLD, MOB, OBJ, ZON, QST, and SHP histories for `dream` remain on the same stem from the 2009 area-tree move through the 2025–2026 source edits. The story sidecar was added on 2026-10-06. No alternate-stem rename, distinct predecessor, retirement, or whole-zone replacement was established in the reviewed history.

`dream` remains an un-commented contributor in both [the world manifest](../../../areas/AREA#L152) and [QST manifest](../../../areas/qst/AREA#L141); keep it **DO NOT MOVE**. See [coverage row 80](ZONE_COVERAGE_REGISTER.csv#L80), active boundary [pair rows 121 and 133](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L121), [connectivity checkpoint](WORLD_CONNECTIVITY.md), and [history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md). Static source review does not claim played shard/skull recovery, accepted hand-in, shop purchase, successful travel, water survival, or safe return.

## Checkpoint 789: shop and sidecar closure refresh

The current source confirms 90 local ZON commands and 72 M/G/E/O prototype fields: 71
resolve locally and one Alatorin dreamfoil import, #83283 at G row 139
([reset](../../../areas/zon/dream.zon#L139),
[Alatorin object](../../../areas/obj/alatorin.obj#L2864)). All local M/O placement and
D room fields resolve locally. The eleven local QST item fields resolve to Dream objects.

The local SHP contains one record, shop #31310, with the local tarot pack #31314 as its
product ([source](../../../areas/shp/dream.shp#L1)). The coverage register's former
zero-shop count missed the `#31310~` header; it now records one shop.

The current story sidecar has 25 typed VNUM fields: 14 `mob_vnum`, five `giver_vnum`,
and six `item_vnums`. All typed targets resolve locally. Its five `completion_key`
strings duplicate the two QST receipts across contract and step records; the repeated
signatures add no contracts. The separate Fisherman's Wharf prose mention of #31320
remains a string explanation, not an item-VNUM field.

The four active outside reset consumers remain Mriel #31317, Lava #31318, Arcium #31319,
and Fisherman's Wharf #31320. Active Arcium bed #85725 is still the only external
type-25 destination into Dream, targeting #31300 across the 869 external declarations.
The scan of 380 external type-29 declarations found no switch target into Dream. The
three local type-25 objects target #31302, Arcium #85805, and #85805; no local type-29
switch exists. No active outside QST item/giver, SHP product, typed sidecar reference, or
duplicate mobile/object definition targets Dream. The 59 exact C/C++ lines remain CHAOS
table entries; no native special/teacher or receipt-constructor binding was found. No
WLD total, candidate, or owner-question set changed. See
[coverage row 79](ZONE_COVERAGE_REGISTER.csv#L80).

