# Torg audit review

Checkpoint 392 closes the source-level connectivity, identity/history, typed
dependency, retention, and builder-question reviews for active `torg`. The
existing [Torg story dossier](../zone-stories/TORG.md) remains the detailed
quest, reset, and behavior map. This review records cross-area and global-map
findings that the dossier did not previously close.

## Active WLD graph and boundary

The active [Torg WLD](../../../areas/wld/torg.wld#L1) contains 282 rooms and
624 resolving exit records: 623 local edges and one outbound edge. Its ordinary
WLD graph has two weak components of 265 and 17 rooms. With all authored exits
treated as open, entry at #28900 or #29093 reaches 253/282 rooms; entry at
#29071 or #29073 reaches 17/282. A total of 264/282 rooms can reach Torg's
active outbound at #28900; #29063 and #29065–#29081 cannot. These counts do
not model locks, hidden doors, key possession, item portals, reset stock,
mobile movement, or safe travel.

| Active source to Torg | Reverse WLD edge | Directed reach from target |
| --- | --- | --- |
| Surface #611053 east → #28900 | Torg #28900 west → Surface #611053 | 253/282 |
| Scorch Valley #71330 down → #29071 | None | 17/282 |
| Winterhaven #55633 northwest → #29093 | None | 253/282 |
| Juiblex #87672 down → #29073 | None | 17/282 |

The only active Torg WLD outbound is #28900 west to Surface #611053. Direction
6 is northwest in the maintained direction constants
([DIR_NORTHWEST](../../../src/core/defines.h#L435)); the Winterhaven source
record itself stores D6 at
[line 13065](../../../areas/wld/wh.wld#L13065). The four source records are
at [Surface](../../../areas/wld/surface.wld#L2331518),
[Scorch Valley](../../../areas/wld/scorchvalley.wld#L2868),
[Winterhaven](../../../areas/wld/wh.wld#L13065), and
[Juiblex](../../../areas/wld/juiblex.wld#L3619). The reverse boundary is
authored in [Torg WLD](../../../areas/wld/torg.wld#L15).

Three local type-25 objects add separate item routes: #28933 at #29064 targets
#29065, #28934 at #29065 targets #29064, and #28952 at #29081 targets #29117.
The first two connect the 265- and 17-room WLD components through objects; they
do not change the ordinary WLD component count. See the
[object declarations](../../../areas/obj/torg.obj#L514),
[second pool](../../../areas/obj/torg.obj#L528),
[third pool](../../../areas/obj/torg.obj#L804), and their
[reset placements at lines 467–469](../../../areas/zon/torg.zon#L467). No active external
type-25 object targets a Torg room.

Room #29063 has no ordinary exit. The dossier records that cap-one legendary
mobiles and Lanella can wander into this holding room, and that other mobile
paths can strand them beyond the active outbound. Keep those availability
questions separate from the static graph and from the active arrival count.

## Inactive WLD sources and the Surf candidate

All 95 inactive WLD files parsed without errors: 527,791 room records and
2,080,773 exit records. They contain 97 room VNUMs in Torg's room set, across
`afterlife_gh`, `afterlife_op`, `afterlife_op2`, `cabal`, `rf_gh`,
`scrubs_hall`, `solstice_gh`, `void_gh`, and `wlust_gh`. All 97 normalized
room titles differ from Torg's same-numbered rooms. The inactive sources also
contain 163 exit records targeting Torg room VNUMs. Of those, 162 begin at
VNUMs owned by active Torg rooms and are excluded as source-only approaches;
their inactive map identities remain separate.

One exit begins at a VNUM with no active WLD owner:
[inactive Surf room #277211](../../../areas/wld/surf.wld#L1620203) goes east
(D1) to active Torg #28900 ([exit](../../../areas/wld/surf.wld#L1620211),
[target](../../../areas/wld/surf.wld#L1620214)). It is not active ingress.
Record it on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97);
do not create a Torg candidate. This raises the curated source-only WLD total
from 9,475 to 9,476 overall and Surf from 293 to 294.

## Typed dependencies and source identity

### Active QST and story-sidecar references

Three active foreign QST inputs consume Torg objects: Scorch Valley giver
#71236 accepts Zuzon's head #28980, and Winterhaven giver #55125 accepts either
heart #28997 or #28998 ([Scorch Valley QST](../../../areas/qst/scorchvalley.qst#L128),
[Winterhaven QST](../../../areas/qst/wh.qst#L2108),
[second heart exchange](../../../areas/qst/wh.qst#L2127)). Those remain
foreign contract owners. Torg's Lore Keeper QST #28964 returns Winterhaven
object #55363 ([Torg QST](../../../areas/qst/torg.qst#L199),
[Winterhaven object](../../../areas/obj/wh.obj#L4424)); it is a foreign reward
prototype, not Torg-owned stock.

All 232 current story sidecars parsed. Three external sidecars contain five
typed values referencing Torg: Juiblex names Lore Keeper #28964 as a `mob_vnum` and giver,
Scorch Valley lists #28980 as an `item_vnums` input, and Winterhaven lists
#28997/#28998 ([Juiblex](../../../areas/story/juiblex.story.json#L514),
[Scorch Valley](../../../areas/story/scorchvalley.story.json#L398),
[Winterhaven](../../../areas/story/wh.story.json#L991)). The Torg sidecar's
local contacts and item references remain with Torg. No other typed outside
QST item command or QST giver VNUM collision was found among the 265 current
QST source files.

### Reset, shop, and item-portal references

No active foreign ZON reset-role command targets a Torg room, mobile, or
object. The active Torg ZON imports three outside objects at room #29093:
object #72, Heavens object #359, and Winterhaven object #55442
([reset lines 470–472](../../../areas/zon/torg.zon#L470)). Object #72 has
three different active definitions: a Heavens magical fountain, Hostel
dragonscale legplates, and a Mavulsk prison key. The reset VNUM alone does not
identify which source was intended. Heavens #359 is a rune-covered stone;
Winterhaven #55442 is a memory of the Kingdom of Torg. Keep all three reset
rows and preserve the unresolved identity decision. See the three conflicting
[#72 definitions](../../../areas/obj/heavens.obj#L1000),
[Hostel](../../../areas/obj/hostel.obj#L959), and
[Mavulsk](../../../areas/obj/mavulsk.obj#L1173), plus the distinct
[Heavens #359](../../../areas/obj/heavens.obj#L3725) and
[Winterhaven #55442](../../../areas/obj/wh.obj#L5229).

Across inactive ZON sources, 153 of 155 reset-role number matches resolve to a
same-stem source prototype. The remaining two are `brass-old-1.zon` G resets
for Torg objects #28983 and #28984. `brass-old-1` is outside the active area
list, so these are retained legacy references, not active consumers
([reset records](../../../areas/zon/brass-old-1.zon#L821)).

Inactive item portals also contain four explicit numeric destinations into
Torg: `afterlife_op` object #29110 targets Torg room #29106, while
`afterlife_op2` objects #29122, #29123, and #29125 target #29109. Their reset
and object sources are inactive and do not establish current access. Thirteen
other inactive type-25 matches target rooms in their own inactive WLD source.
None belongs in the active WLD edge count. The portal objects are
[afterlife_op #29110](../../../areas/obj/afterlife_op.obj#L1),
[afterlife_op2 #29122](../../../areas/obj/afterlife_op2.obj#L17),
[#29123](../../../areas/obj/afterlife_op2.obj#L25), and
[#29125](../../../areas/obj/afterlife_op2.obj#L41); their inactive reset
placements are at [afterlife_op](../../../areas/zon/afterlife_op.zon#L9) and
[afterlife_op2](../../../areas/zon/afterlife_op2.zon#L11),
[second afterlife_op2 reset](../../../areas/zon/afterlife_op2.zon#L14), and
[third afterlife_op2 reset](../../../areas/zon/afterlife_op2.zon#L17).

The two products in `torg.shp` resolve to local object prototypes. One separate
record in inactive `rf_gh.shp` lists object #28902 as a product and keeper
#28904 in room #28909 ([record start](../../../areas/shp/rf_gh.shp#L1),
[keeper field](../../../areas/shp/rf_gh.shp#L67), room field at line 69). `rf_gh` is
not an active area source. This checkout has no `areas/world.shp`. In full
mode, the [shop loader](../../../src/economy/shop.c#L3272) falls back to
concatenating individual `.shp` files
([fallback](../../../src/economy/shop.c#L3278)); mini mode reads
`areas_mini/world.shp` at [the mode branch](../../../src/economy/shop.c#L3269),
which currently contains only the file terminator. The selected startup mode
was not tested. Where the individual-file fallback is selected, the loader
resolves product and keeper VNUMs. The same VNUMs
currently identify Torg's rusty hand-axe #28902 and duergar miner #28904
([object](../../../areas/obj/torg.obj#L28),
[mobile](../../../areas/mob/torg.mob#L63)). Torg resets that mobile in rooms
#28944–#28947, not #28909
([first mobile reset](../../../areas/zon/torg.zon#L524),
[last mobile reset](../../../areas/zon/torg.zon#L538)). Shopkeeper assignment
uses the resolved mobile prototype ([assignment](../../../src/economy/shop.c#L3602)).
This is a retained cross-area collision; actual shop availability and startup
configuration were not tested. Do not count `rf_gh` as a third Torg-owned
shop.

### Same-type definitions and compiled references

No active external WLD, MOB, or OBJ file defines a duplicate Torg prototype
VNUM. Inactive sources have 97 WLD room overlaps, ten same-type mobile
definitions (`cabal` #29000–#29002 and `rf_gh` #28900–#28906), and five
same-type object definitions (`rf_gh` #28900–#28904). The matching names and
room titles differ from the active Torg prototypes; keep those inactive map
identities distinct.

The Cabal-specific room/mobile overlap and full-mode shop-loader collision are detailed in the
[Checkpoint 458 Cabal Guildhall audit](CABAL_GUILDHALL_AUDIT_REVIEW.md#active-torg-ownership-of-the-same-room-and-mobile-vnums).

Maintained code contains these typed Torg references:

- Mobile #28961 binds `timoro_die`, and mobile #29025 binds `lanella_heart`
  ([assignments](../../../src/specs/specs.assign.c#L1144) and
  [heart assignment](../../../src/specs/specs.assign.c#L282)). Room #29103
  binds the inn ([assignment](../../../src/specs/specs.assign.c#L2448)); room
  #29181's berserker binding is commented out
  ([inactive altar binding](../../../src/specs/specs.assign.c#L2279)).
- The invasion uses room #29116 and leader #28975 in
  [Torg-specific code](../../../src/specs/specs.torg.c#L15). Mobile #28975
  also appears in the epic teacher table
  ([epic skill table](../../../src/classes/epic_skills.c#L205)).
- The smith table names Kordor #28917
  ([tradeskill table](../../../src/economy/tradeskill.c#L218)); the existing
  dossier explains why the earlier shop assignment prevents this table from
  attaching through the normal initialization order.
- Winterhaven's custom heart handler creates object #28998 or #28997
  ([broken heart](../../../src/specs/specs.winterhaven.c#L4867),
  [intact heart](../../../src/specs/specs.winterhaven.c#L4870)).
- CHAOS starter-kit arrays reference seven Torg objects: #28911, #28913,
  #28921, #28954, #28971, #28981, and #28988
  ([representative entries](../../../src/account/chaos_eq_data.h#L82)).

The complete Torg reset census and gameplay caveats remain in the
[dossier](../zone-stories/TORG.md). Source declarations do not qualify runtime
stock, travel, shop operation, persistent credit, or live gameplay.

## Title, followed history, and retention

After removing color tokens and terminators and normalizing case and spacing,
“The Kingdom of Torg” is unique among all 442 current ZON titles
([title](../../../areas/zon/torg.zon#L2)). Followed WLD, MOB, OBJ, ZON, QST,
and SHP histories stay on same-stem `torg` paths through the 2009 area-tree
move and later edits. The story sidecar is tracked from commit `b61918be4` on
2026-10-02. No tracked `kolut.*` source stem, explicit Torg rename, distinct
predecessor, or whole-zone replacement was found. The shared `(kolut)` AREA
annotation is not enough to infer any of those; see the earlier
[Scorched Valley annotation review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-347-scorched-valley-title-duplicates-and-followed-history).

Keep active `torg` **DO NOT MOVE**. Preserve its Surface pair, three one-way
active arrivals, typed outside QST and story references, local imported reset
objects, the source-only Surf edge on candidate row 97, and the inactive map
identity evidence. No candidate or zone move is added. The twenty direct
builder questions and recommendations are in the
[Torg dossier](../zone-stories/TORG.md#explicit-builder-questions-and-recommended-resolutions).

## Checkpoint 847: Current typed owners and shop-loader boundary refreshed

The active Torg typed closure remains as recorded at Checkpoint 392. Scorch
Valley consumes Torg OBJ #28980; Winterhaven accepts either #28997 or #28998;
Juiblex names Lore Keeper MOB #28964 as an outside story contact; and Torg's
Lore Keeper QST returns Winterhaven OBJ #55363. Preserve each receiving quest,
contact, and foreign reward with its current owner. Torg's own ZON still imports
OBJ #72, #359, and #55442 at #29093. The #72 prototype identity remains
ambiguous across Heavens, Hostel, and Mavulsk; keep the source reset without
choosing one owner from its number alone.
([Scorch Valley QST](../../../areas/qst/scorchvalley.qst#L128),
[Winterhaven QST inputs](../../../areas/qst/wh.qst#L2108),
[second Winterhaven input](../../../areas/qst/wh.qst#L2127),
[Torg reward QST](../../../areas/qst/torg.qst#L199),
[Torg imported objects](../../../areas/zon/torg.zon#L470),
[Juiblex contact](../../../areas/story/juiblex.story.json#L514))

The inactive references remain separate from the active closure: `brass-old-1`
resets Torg objects #28983/#28984, and inactive `afterlife_op`/`afterlife_op2`
objects declare four Torg room targets. Torg has two local shop product rows.
The separate `rf_gh.shp` record lists product #28902, keeper #28904, and room
#28909, which currently resolve to Torg's rusty hand-axe, duergar miner, and
corridor if the full-mode individual-file fallback is selected. The fallback
path is documented, but startup mode and actual shop availability remain
unqualified; do not count this as a third active Torg shop. Preserve the
separate [rf_gh component review](RF_GH_AUDIT_REVIEW.md#reset-imports-and-shop-loader-path)
and its RETAIN IN PLACE disposition.
([inactive shop record](../../../areas/shp/rf_gh.shp#L1),
[loader fallback](../../../src/economy/shop.c#L3278),
[Torg product](../../../areas/obj/torg.obj#L28),
[Torg keeper](../../../areas/mob/torg.mob#L63),
[Torg room](../../../areas/wld/torg.wld#L1))

Keep the Surface WLD pair and the three one-way arrivals from Scorch Valley,
Winterhaven, and Juiblex. Inactive Surf #277211 east to Torg #28900 stays
source-only on candidate row 97; its approach is already included in current
curated totals of 11,993 overall and 415 Surf. Keep active `torg` **DO NOT
MOVE**; this refresh adds no Torg candidate, route activation, or migration
clearance.
