# `tharold` source-only shop audit

Canonical key: `component:tharold`. Coverage data row 454 / CSV line 455;
candidate dependency data row 103 / CSV line 104. Reviewed October 12, 2026,
Checkpoint 541, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
identity, and dependency review. It does not establish runtime shop loading or
operation.

## Membership and shop identity

The candidate consists only of
[areas/shp/tharold.shp](../../../areas/shp/tharold.shp#L1), with 12 shop
records and no WLD, MOB, OBJ, ZON, QST, or story-sidecar companion. The current
[areas/AREA list](../../../areas/AREA#L382) includes active `tharnadia`, not
`tharold`. There is no `tharold` map or zone title from which to infer a
separate place or route.

Active [tharnadia.shp](../../../areas/shp/tharnadia.shp#L1) has a separate
shop catalog in the #132xxx VNUM band. All twelve `tharold` shop records
have related active Tharnadia records with remapped shop/keeper/local-stock
VNUMs and locations in the active Tharnadia map. Eight stock lists match
after local VNUM remapping. The other four differ: old #6001 omits active
stock additions, old #6061 has a larger list, old #6070 omits later active
stock, and old #6085 has a shorter list. This is related source evidence,
not a source-retirement instruction.

All twelve candidate shop numbers also occur in the separately inventoried
[`tharnadia1.shp`](../../../areas/shp/tharnadia1.shp#L1). Each pair has the
same keeper and `in_room` values, but none has the same complete record; eight
product lists happen to match, while #6001, #6061, #6070, and #6085 differ.
Every paired record is non-roaming. Preserve the two candidate files
separately. Their histories reach the 2009 area-tree move and a 2013 edit,
but do not document retirement, an exact rename, or permission to merge the
records.

## Complete source inventory and typed references

`in_room` is a shop location field, not a WLD exit. The full twelve-record
inventory follows:

| Candidate shop | Source line | Keeper | `in_room` | Product VNUMs | Active Tharnadia counterpart |
| ---: | ---: | ---: | ---: | --- | --- |
| #6000 | 1 | #6000 | #6058 | #6007, #6008, #6009 | #132500 |
| #6001 | 37 | #6001 | #6055 | #6015, #6016, #6017, #6054 | #132501 |
| #6002 | 74 | #6002 | #6130 | #6021, #6022, #6023, #6029, #6030, #6031, #6065, #6073, #6074, #6076, #6079 | #132502 |
| #6025 | 120 | #6025 | #6242 | #6041, #6043, #6037, #6038, #6039, #6040, #6042 | #132525 |
| #6030 | 162 | #6030 | #6113 | #6052, #6053 | #132530 |
| #6051 | 197 | #6051 | #6267 | #6100, #6101, #6045, #6046, #6047, #6048, #6049 | #132551 |
| #6058 | 237 | #6058 | #6057 | #6088 | #132558 |
| #6061 | 271 | #6061 | #6053 | #6104, #6105, #6106 | #132561 |
| #6066 | 307 | #6066 | #6243 | #6050, #6051, #6055, #6056, #6058, #6069, #6086 | #132566 |
| #6070 | 348 | #6070 | #6262 | #6019, #6059, #6072, #6067, #6068, #6087, #53 | #132570 |
| #6085 | 390 | #6085 | #6031 | #6089, #6092, #6093, #6094, #6095 | #132585 |
| #6086 | 430 | #6086 | #6044 | #6020, #6022, #6064 | #132586 |

The 59 distinct stock VNUMs resolve to four current active OBJ prototypes:
#53 and Lizard #6007–#6009. The other 55 have no current OBJ definition in
the checked-in `areas/obj` tree. Three keeper VNUMs (#6000–#6002) resolve to
active Lizard mobiles; the other nine have no active MOB match. Nine
`in_room` values resolve to active Lizard or Nomads rooms, while #6130,
#6262, and #6267 have no active WLD room. The Nomads references are
shop #6025 at #6242 and #6066 at #6243; their keeper VNUMs have no active
MOB definition. These cross-area numeric matches are typed shop dependencies,
not routes or proof of active stock.

## Loader and source-history boundary

The normal [shop-file generator](../../../areas/src/shp/make_shp.c#L17) reads
the main area list and selects `shp/<area>.shp`; this selects active
`tharnadia.shp` and excludes `tharold.shp`. The server's
[`open_shop_stream`](../../../src/economy/shop.c#L3267) opens the combined
`areas/world.shp` first. In non-mini mode, if that file cannot be opened, it
falls back to globbing every `areas/shp/*.shp`, which includes `tharold` and
`tharnadia1`. That condition is source code evidence only; the combined
artifact is absent from this checkout, and the deployed input is unknown.

If both candidates enter that fallback, their twelve shared
keeper/room pairs can cause an ambiguous non-roaming shop lookup.
[`singleton_shop_id`](../../../src/world/world_singletons.c#L75) accepts a
valid explicit binding; without one, a second non-roaming record matching the
same keeper and room makes the lookup ambiguous. A bounded C/C++ scan found
no `tharold` source-name hit or direct typed literal binding to its keeper
and room VNUMs.

The file contains merchant trade text but has no local map, quest, or story
sidecar. Do not infer a zone narrative or active commerce from the dialogue.
Keep `tharold.shp` **RETAIN IN PLACE** pending the twenty owner questions and
recommendations in
[THAROLD_BUILDER_QUESTIONS.md](THAROLD_BUILDER_QUESTIONS.md). No activation,
co-loading, merge, move, or archive clearance is implied.
