# Kzkrkeep source-only QST/SHP audit

Checkpoint 906 refreshes the prior CP494 source, typed-role, and loader-path
review against the currently tracked source tree.

This checkpoint covers the source files named `areas/qst/kzkrkeep.qst` and
`areas/shp/kzkrkeep.shp`. The name does not correspond to an active area in
either area list, and this checkout has no Kzkrkeep WLD, MOB, OBJ, ZON, or
story sidecar. These are static source and loader-path findings; they do not
qualify live shop loading or player access.

## Source inventory and loading paths

`kzkrkeep.qst` defines one giver header, #6917, and one native Q contract.
`kzkrkeep.shp` contains two shop records, headed #6917 and #6924. Neither file
is named by `areas/AREA` or `areas/qst/AREA`; neither list has a Kzkrkeep row.
No matching WLD, MOB, OBJ, ZON, or `areas/story/` file exists. There is no
candidate AREA annotation or zone-title source to establish a display name.

The standard quest generator reads the top-level `AREA` list
([compiler list](../../../areas/src/qst/make_qst.c#L6)), and quest runtime
opens the generated `areas/world.qst` file without globbing orphan QST files
([runtime path](../../../src/world/quest.c#L79),
[no-glob behavior](../../../src/world/quest.c#L2086)). The standard shop
generator also reads the top-level `AREA` list
([compiler list](../../../areas/src/shp/make_shp.c#L6)). However, shop runtime
has a distinct fallback: if `areas/world.shp` cannot be opened, it globs
`areas/shp/*.shp` and appends `end.shp`
([stream selection](../../../src/economy/shop.c#L3267),
[fallback glob](../../../src/economy/shop.c#L3278)). `areas/world.shp` is
absent in this checkout. In that condition the fallback input includes
`kzkrkeep.shp` despite its omission from the standard area list. This code
path makes SHP handling conditional on the generated/deployed file state;
no server or live shop load was run.

## Quest story and typed references

The dialogue names Lirthandarn. It offers healing potions, says troll guards
barred the speaker from entering a keep, and requests the troll leader's
amulet. The authored item commands instead remove item #6900 and award #6912:

| QST field | Current active typed target | Finding |
| --- | --- | --- |
| Giver header #6917 | No active MOB prototype #6917; the same VNUM is an active Worms room and OBJ, which are different types. | No Lirthandarn mobile definition exists in the checked-in area sources. |
| `R I 6900` ([QST line](../../../areas/qst/kzkrkeep.qst#L20)) | Worms OBJ #6900 is a strip of grey wormskin ([object](../../../areas/obj/worms.obj#L1)). | This is not an amulet by its active object name or description. |
| `G I 6912` ([QST line](../../../areas/qst/kzkrkeep.qst#L21)) | Worms OBJ #6912 is a strip of purple wormskin ([object](../../../areas/obj/worms.obj#L145)). | This is also not an amulet or a uniquely identified treasure in the source description. |

The authored request/reward story therefore conflicts with the current OBJ
identities selected by its numeric item commands. The bounded exact-number
search found no C/C++ reference that supplies the missing Kzkrkeep keeper or
giver.

## Shop records and dependencies

The two records each name a keeper and a room. The loader resolves product
VNUMs through `real_object()` and keepers through `real_mobile()`
([product resolution](../../../src/economy/shop.c#L3380),
[keeper resolution](../../../src/economy/shop.c#L3489)).

| SHP header | Keeper VNUM | `in_room` VNUM | Products |
| --- | ---: | ---: | --- |
| #6917 ([record](../../../areas/shp/kzkrkeep.shp#L1)) | #6917 ([field](../../../areas/shp/kzkrkeep.shp#L26)) | #6915 ([field](../../../areas/shp/kzkrkeep.shp#L28)) | #6913-#6920 and #6958 |
| #6924 ([record](../../../areas/shp/kzkrkeep.shp#L44)) | #6924 ([field](../../../areas/shp/kzkrkeep.shp#L80)) | #6919 ([field](../../../areas/shp/kzkrkeep.shp#L82)) | #6924, #189, #184, #185, #163, #167, #170, #435, #434, #433, #432, #395, #267, #287, #6968, #6969 |

Neither keeper VNUM has an active MOB prototype. The room values resolve to
active Worms rooms: #6915 is a smooth tunnel
([room](../../../areas/wld/worms.wld#L306)) and #6919 is a crumbly tunnel
([room](../../../areas/wld/worms.wld#L386)); they are not rooms in a Kzkrkeep
map.

There are 25 positive product entries. Twenty-two resolve to active OBJ
prototypes: eight Worms objects #6913-#6920, Worms object #6924, and thirteen
Heavens objects #163, #167, #170, #184, #185, #189, #267, #287, #395, and
#432-#435. The remaining #6958, #6968, and #6969 have no OBJ prototype; each
is instead a room VNUM in the Worms WLD (#6958, #6968, #6969). This is a
typed-reference mismatch, not an item definition. Product and keeper
resolution at runtime was not qualified.

The existing [Worms source supplement](WORMS_AUDIT_REVIEW.md#external-typed-and-code-references)
already records the QST item overlap and the nine Worms-OBJ shop products.
This checkpoint adds the missing keeper, shop-location, remaining stock, and
quest-story reconciliation.

## History, naming, and disposition

Followed history for both source paths contains the 2009 `areas/` tree move
(`642da5400`) and a 2013 same-path change labeled `stuff` (`71d731c5b`). The
reviewed paths show no direct rename, retirement, or replacement commit. The
QST dialogue refers to “the keep,” but there is no Kzkrkeep map, area
annotation, ZON title, or display-name record here. The filename and shared
Worms numbers do not establish whether this is an old Worms shop/quest, a
separate lost area, or stale copied data.

Keep both files **RETAIN IN PLACE**. Do not add them to either area list,
move them, or delete them based on the current checkout. The QST is outside
the normal generated quest input, while the SHP can enter the shop fallback
when `areas/world.shp` is missing. Resolve the intended area, mobile
definitions, amulet/object identities, stock VNUMs, deployed `world.shp`
state, editor/export path, and persisted quest identity with their owners
before proposing a separate change.

## Checkpoint 906: cross-source typed-role refresh

The same-number scan confirms that the source-only files refer to active
Worms and Heavens identities across separate type namespaces; those numbers
do not identify Kzkrkeep rooms, keepers, or items. Worms defines the relevant
room VNUMs, object prototypes, resets, and QST/story item roles. Its QST has
six `G I` commands for objects #6900 and #6912, and its ZON resets those
wormskin items. The source-only Kzkrkeep commands `R I 6900` and `G I 6912`
therefore bind to active Worms OBJ prototypes: grey wormskin and purple
wormskin, respectively. This confirms the typed item mismatch with the
amulet story; it does not establish a Kzkrkeep quest deployment.

Kzkrkeep's #6917 QST giver and first SHP keeper, and #6917/#6924 shop
keepers, still have no active MOB prototypes. Worms has same-number rooms and
objects for #6917/#6924, but those types cannot satisfy `real_mobile()`.
The two SHP `in_room` values #6915/#6919 resolve active Worms rooms. Three
shop product VNUMs (#6958, #6968, #6969) also resolve only as Worms rooms and
have no OBJ prototypes, so their product-role mismatch remains.

The cross-source SHP scan found 13 other parser-shaped `N`-format stock rows
across six active shards (`alatorin`, `ashrumite`, `goblinht`, `khildarak`,
`orogs`, and `surfacekeeps`) that reuse seven of Kzkrkeep's Heavens object
products: #163, #167, #184, #185, #189, #267, and #287. These are shared
Heavens OBJ identities, not Kzkrkeep keepers, rooms, or replacement evidence.
No other `N`-format shop header, keeper, or location matched #6917/#6924 or
#6915/#6919, and no other `N`-format shop stocks Kzkrkeep's Worms objects
#6913–#6920/#6924. `areas/world.shp` remains absent in this checkout; the
runtime fallback would glob `areas/shp/*.shp` if that generated file were not
available. The shop's appearance in that path and the three invalid products
remain unqualified because no shop or game runtime was run.

One compiled code consumer reuses #6900 in a different type role:
[`illithid_teleport_veil`](../../../src/specs/specs.ixarkon.c#L18) includes it
in a room-destination table passed to `real_room()`. #6900 is also active
Worms room #6900 ([WLD](../../../areas/wld/worms.wld#L1)) and Worms OBJ #6900
(grey wormskin); the code uses the room, not the Kzkrkeep quest item. No
compiled lookup supplies the missing #6917/#6924 keeper or #6917 giver. No
type-25/type-29 destination can be assigned to a Kzkrkeep map because this
component has no WLD room set. These role matches add no candidate route and
do not change the CP494 history or retention decision.

K-11 now records the separate #6900 room-code role. Keep both source files
**RETAIN IN PLACE** outside the area lists while the quest/item, keeper,
shop-location, invalid-stock, `world.shp` deployment, editor/export, and
persisted-identity questions remain open. See the [20 builder questions](KZKRKEEP_BUILDER_QUESTIONS.md),
[Worms typed-reference findings](WORMS_AUDIT_REVIEW.md#external-typed-and-code-references),
[connectivity supplement](WORLD_CONNECTIVITY.md#checkpoint-906-kzkrkeep-roles-remain-in-active-worms),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-906-kzkrkeep-numeric-overlaps-remain-worms-identities),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-906-kzkrkeep-retains-unlisted-qst-and-shop-sources),
[candidate dependency row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57), and
[coverage CSV line 408](ZONE_COVERAGE_REGISTER.csv#L408).

## Review limits

This is a bounded static review of tracked source, area lists, current
prototypes, relevant loader/compiler code, and followed path history. No files
under `areas/` or `src/` were changed. No build, test, server, database, or
container was run.

See the [20 builder questions](KZKRKEEP_BUILDER_QUESTIONS.md),
[connectivity checkpoint](WORLD_CONNECTIVITY.md),
[identity/history checkpoint](DUPLICATE_AND_REPLACED_ZONES.md),
[retention plan](ATTIC_MIGRATION_PLAN.md),
[candidate row 57](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L57), and
[coverage row 407](ZONE_COVERAGE_REGISTER.csv#L408).
