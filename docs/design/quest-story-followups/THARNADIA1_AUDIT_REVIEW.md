# `tharnadia1` source-only shop audit

Canonical key: `component:tharnadia1`. Coverage data row 453 / CSV line 454;
candidate dependency data row 102 / CSV line 103. Reviewed October 12, 2026,
Checkpoint 540, against checkout
`c500ac51b8cd26540943257e7ca4e9807263ef74`. This is a static source,
identity, and dependency review. It does not establish that an individual
shop is loaded or usable in a running game.

## Membership and relationship to active Tharnadia

The only candidate file is
[areas/shp/tharnadia1.shp](../../../areas/shp/tharnadia1.shp#L1). It contains
18 shop records and has no WLD, MOB, OBJ, ZON, QST, or story-sidecar
companion. The current
[areas/AREA list](../../../areas/AREA#L382) includes active `tharnadia`
(#1325–#1330), not `tharnadia1`. There is no candidate zone title or local
map from which to infer a public route or a separate place name.

Active [tharnadia.shp](../../../areas/shp/tharnadia.shp#L1) has 19 current
records in a different VNUM band. Sixteen `tharnadia1` records (#6000–#6117)
have clearly related active records: their shop and keeper VNUMs are remapped
to #132500–#132617, their room locations use the active Tharnadia map, and
their local stock VNUMs are remapped. Fourteen of those sixteen stock lists
match after remapping. Current #132585 adds product #102; current #132617 adds
#400223, #400224, and #55555. Candidate #6500 and active #132650 have
different stock, and candidate #6519 has no matching current Tharnadia shop.
The active Tharnadia file therefore supplies strong related-source evidence,
but it does not prove a retirement instruction for the candidate.

The old shop numbers also occur in separately inventoried
[`tharold.shp`](../../../areas/shp/tharold.shp#L1) and
[`twinkeep.shp`](../../../areas/shp/twinkeep.shp#L1). Twelve candidate
headers are also in `tharold`; two (#6500 and #6519) are in `twinkeep`. In
each comparison the keeper and room fields agree, but the complete shop
record differs. Preserve these source identities and their checked-in
differences; do not co-load or infer replacement from the numbers alone. The
[Twinkeep audit](TWINKEEP_AUDIT_REVIEW.md) documents the two shared keys,
typed products and locations, and the conditional fallback shop binding.

The candidate file history begins with its 2010 addition and has a 2013 edit.
Current Tharnadia has a long, later same-stem history, including explicit
shop updates. Neither history states that `tharnadia1` was retired or
authorizes its removal. The current Tharnadia source and the old shop-only
source remain separate until an owner confirms their editor and export roles.

## Shop records and typed references

The following is the complete inventory parsed from the 18 source records.
`in_room` is a shop-location field, not a WLD exit. “Current counterpart”
names the related active Tharnadia record where one is established.

| Candidate shop | Source line | Keeper | `in_room` | Product VNUMs | Current Tharnadia counterpart |
| ---: | ---: | ---: | ---: | --- | --- |
| #6000 | 1 | #6000 | #6058 | #6007, #6008, #6009 | #132500 |
| #6001 | 37 | #6001 | #6055 | #6015, #6016, #6017, #6054, #835, #6132 | #132501 |
| #6002 | 76 | #6002 | #6130 | #6021, #6022, #6023, #6029, #6030, #6031, #6065, #6073, #6074, #6076, #6079 | #132502 |
| #6025 | 123 | #6025 | #6242 | #6041, #6043, #6037, #6038, #6039, #6040, #6042 | #132525 |
| #6030 | 165 | #6030 | #6113 | #6052, #6053 | #132530 |
| #6051 | 200 | #6051 | #6267 | #6100, #6101, #6045, #6046, #6047, #6048, #6049 | #132551 |
| #6058 | 240 | #6058 | #6057 | #6088 | #132558 |
| #6061 | 274 | #6061 | #6053 | #6104, #6105, #6106, #6070, #6109, #6110, #835, #336 | #132561 |
| #6066 | 315 | #6066 | #6243 | #6050, #6051, #6055, #6056, #6058, #6069, #6086 | #132566 |
| #6070 | 356 | #6070 | #6262 | #6019, #6059, #6072, #6067, #6068, #6087, #53, #11010, #11011, #1211, #191 | #132570 |
| #6085 | 402 | #6085 | #6031 | #6089, #6092, #6093, #6094, #6095, #500, #73, #77 | #132585 |
| #6086 | 445 | #6086 | #6044 | #6020, #6022, #6064 | #132586 |
| #6111 | 484 | #6111 | #6609 | #6111, #6112, #6113, #6114, #6115, #54 | #132611 |
| #6113 | 526 | #6113 | #6358 | #6116, #6117, #6118, #6119, #6120, #6122, #6123, #6130 | #132613 |
| #6115 | 577 | #6115 | #6054 | #820, #821, #823, #822, #824, #825, #826, #835, #827, #828 | #132615 |
| #6117 | 620 | #6117 | #6001 | #66709, #253, #252, #431, #74203, #191, #363, #368, #369 | #132617 |
| #6500 | 666 | #6500 | #0 | #6500 | #132650 has different stock |
| #6519 | 703 | #6519 | #6656 | #203, #204, #3050, #6537 | no matching record found |

Across the file, 108 distinct product VNUMs occur. Thirty-five resolve to
current active OBJ prototypes; 73 have no current OBJ definition in the
checked-in `areas/obj` tree. The candidate does not define its own stock
prototypes. Shop #6000 lists active Lizard objects #6007–#6009. Shop #6070
lists active Player Castles objects #11010/#11011 at
[lines 365–366](../../../areas/shp/tharnadia1.shp#L365); active Tharnadia
#132570 also lists those same external objects at
[line 365](../../../areas/shp/tharnadia.shp#L365). Preserve that source-only
dependency on this candidate row.

One prior cross-namespace hit needs a precise classification: source line 561
is shop #6113's `in_room` value #6358, not a product entry. That number names
an active Mist room; no current Mist OBJ #6358 exists, and #6358 is not in the
candidate stock lists. The earlier Shadow Forest false-positive finding is
therefore retained as a shop-room/object-namespace collision, not a typed
product dependency.

Four keeper VNUMs resolve to current active mobiles (#6000–#6002 in Lizard
and #6500 in Shaughin); the other fourteen have no active MOB match. The
record locations #6242/#6243 are active Nomads rooms, but their keeper
VNUMs #6025/#6066 have no active MOB match. Three positive `in_room` values
(#6130, #6262, #6267) have no active WLD room; most other values resolve to
rooms in Lizard, Nomads, Mist, Shaughin2, or Limbo. These are typed shop
dependencies, not routes into a `tharnadia1` map.

## Loader, code, and runtime boundary

The normal [shop-file generator](../../../areas/src/shp/make_shp.c#L17) reads
the main area list and opens `shp/<area>.shp`; that selects active
`tharnadia.shp` and excludes `tharnadia1.shp`. The server's
[`open_shop_stream`](../../../src/economy/shop.c#L3267) first opens the
combined `areas/world.shp`. In non-mini mode, if it cannot open that file, it
falls back to globbing every `areas/shp/*.shp` file, so this candidate and
the separate `tharold`/`twinkeep` sources are within that conditional input
set. The combined file is absent from this checkout; this source inspection
does not establish which artifact a deployed server uses.

If both `tharnadia1` and `tharold` records enter that fallback, their twelve
shared keeper-and-room pairs can produce duplicate non-roaming room matches.
[`singleton_shop_id`](../../../src/world/world_singletons.c#L75) uses a valid
explicit shop binding when one exists; without one, a second matching room
record makes the lookup ambiguous. This is a concrete loader/identity issue
for the owner to resolve, not evidence that the fallback ran or a shop failed
in a live server. A bounded C/C++ scan found no `tharnadia1` string or direct
typed literal binding to the candidate's keeper and room IDs.

The shop prose describes merchants and opening/closing messages. There is no
local WLD, QST, or story sidecar from which to claim a zone narrative or quest
behavior. Keep the source **RETAIN IN PLACE** pending the twenty owner
questions and recommendations in
[THARNADIA1_BUILDER_QUESTIONS.md](THARNADIA1_BUILDER_QUESTIONS.md). No
activation, co-loading, move, merge, renumbering, or archive clearance is
implied.

## Bloodstone Checkpoint 778: candidate-only shop product

Candidate Tharnadia1 shop #6117 lists Bloodstone object #74203 at line 626. Preserve this source-only product under RETAIN IN PLACE; it is not active shop stock. See the [Bloodstone typed-dependency audit](BS_AUDIT_REVIEW.md#registered-candidate-only-dependencies).
