# City of Torrhan audit review

Checkpoint 393 closes the source-level connectivity, identity/history, typed
dependency, retention, and builder-question reviews for active `torrhan`. The
existing [Torrhan story dossier](../zone-stories/CITY_OF_TORRHAN.md) remains
the detailed contract, reset, behavior, and qualification map. This supplement
records the global-map and cross-area evidence.

## Active WLD graph and boundaries

The active [Torrhan WLD](../../../areas/wld/torrhan.wld#L1) contains 289 rooms
and 623 resolving exit records: 619 local edges and four outbound edges. All
rooms belong to one weak component. The five active WLD arrivals share the
same directed reach set of 280/289 rooms; nine rooms are not reachable from any
arrival using authored WLD exits alone.

| Active source to Torrhan | Torrhan reverse edge | Reach from arrival |
| --- | --- | --- |
| Surface #580480 D2 → #66689 ([source](../../../areas/wld/surface.wld#L1689436)) | #66689 D0 → #580480 ([Torrhan](../../../areas/wld/torrhan.wld#L1684)) | 280/289 |
| Surface #580879 D1 → #66689 ([source](../../../areas/wld/surface.wld#L1697811)) | #66689 D3 → #580879 ([Torrhan](../../../areas/wld/torrhan.wld#L1692)) | 280/289 |
| Surface #580881 D3 → #66712 ([source](../../../areas/wld/surface.wld#L1697861)) | #66712 D1 → #580881 ([Torrhan](../../../areas/wld/torrhan.wld#L2164)) | 280/289 |
| Surface #581280 D0 → #66692 ([source](../../../areas/wld/surface.wld#L1706224)) | #66692 D2 → #581280 ([Torrhan](../../../areas/wld/torrhan.wld#L1751)) | 280/289 |
| Winterhaven #55614 D8 → #66600 ([source](../../../areas/wld/wh.wld#L12647)) | None | 280/289 |

The Winterhaven record comes from an administrative Incarnate dispersal room,
not a normal discoverable route. All four outward edges return to Surface;
286/289 rooms can reach at least one outward edge by WLD alone. Rooms #66804,
#66805, and #66869 cannot. #66804 and #66805 have a separate item route:
Torrhan resets type-25 portal #66646 in #66805, targeting local room #66780
([reset](../../../areas/zon/torrhan.zon#L288),
[portal destination](../../../areas/obj/torrhan.obj#L696)). This is separate
from the ordinary exit graph and does not prove live access or safe return.
Room #66869 has no ordinary exit; its source header is at
[Torrhan WLD line 5144](../../../areas/wld/torrhan.wld#L5144).

The nine rooms outside the active arrival reach set are #66864–#66870,
#66900, and #66901. Their records include a one-way dark-room maze and two
rareload rooms; for example, #66900 and #66901 have exits onward to #66822 or
#66772, while no active WLD arrival points into them
([maze](../../../areas/wld/torrhan.wld#L5049),
[dragon rareload room](../../../areas/wld/torrhan.wld#L5622),
[palace rareload room](../../../areas/wld/torrhan.wld#L5652)). Static reach
does not determine NPC wandering, rareload availability, hidden travel, or
player safety.

## Inactive WLD approaches and the Surf candidate

All 95 inactive WLD files parsed without errors: 527,791 room records and
2,080,773 exit records. None defines a Torrhan room VNUM. Three edges from
inactive `surf.wld` originate at source VNUMs with no active WLD owner:

| Inactive Surf source edge | Source record |
| --- | --- |
| #212237 D2 → active Torrhan #66689 | [exit](../../../areas/wld/surf.wld#L256805) |
| #212538 D3 → active Torrhan #66712 | [exit](../../../areas/wld/surf.wld#L263118) |
| #212837 D0 → active Torrhan #66692 | [exit](../../../areas/wld/surf.wld#L269373) |

These are not active ingress. Record them on the existing
[Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97); add no
Torrhan candidate. Curated source-only WLD totals rise from 9,476 to 9,479
overall and from 294 to 297 for Surf. The Surf candidate's 145 active
unresolved references and 17 source-area keys do not change.

## Typed dependencies and source identity

### QST and story-sidecar references

Shipy QST giver #43187 accepts Torrhan object #66723
([QST input](../../../areas/qst/shipy.qst#L895)). The Shipy story sidecar has
one corresponding typed `item_vnums` reference
([sidecar](../../../areas/story/shipy.story.json#L1127)). Among the 265
current QST files, this is the only external item command that references a
Torrhan object; there is no external giver-header collision. Torrhan QST has
no foreign item command references. All 232 current story sidecars parse; the
Shipy sidecar is the only external sidecar with a typed Torrhan VNUM.

### Reset and item-portal references

Six active external ZON reset roles reference Torrhan objects: Alatorin imports
#66723 three times ([first](../../../areas/zon/alatorin.zon#L1766),
[second](../../../areas/zon/alatorin.zon#L3663),
[third](../../../areas/zon/alatorin.zon#L3671)); Shipy loads fishing pole
#66709 ([reset](../../../areas/zon/shipy.zon#L395)); Surfacekeeps nests that
pole in container #120033 ([reset](../../../areas/zon/surfacekeeps.zon#L146));
and Tharnadia loads it ([reset](../../../areas/zon/tharnadia.zon#L637)). No
active outside reset targets a Torrhan room or mobile.

Three inactive reset-role records also match Torrhan objects: `brass-old-1`
loads #66637 ([reset](../../../areas/zon/brass-old-1.zon#L824)), and inactive
`nexus56` loads #66604 and #66602
([resets](../../../areas/zon/nexus56.zon#L1334)). They are retained legacy
records, not active ZON consumers.

Torrhan ZON imports nine outside object VNUMs. The imported #72 at room #66801
has three same-VNUM definitions: active Heavens' magical fountain and
inactive Hostel's dragonscale legplates and Mavulsk's iron prison key
([reset](../../../areas/zon/torrhan.zon#L287),
[Heavens](../../../areas/obj/heavens.obj#L1000),
[Hostel](../../../areas/obj/hostel.obj#L959),
[Mavulsk](../../../areas/obj/mavulsk.obj#L1173)). Under the current active
manifest, the Heavens prototype is the active #72 definition. Imported #41 at
reset line 561 has four same-VNUM definitions in Dalvik, Heavens, Hostel, and
Mavulsk; only Heavens' guild badge is active in the current manifest, while
the other three definitions are inactive. Preserve the current active
identity and keep those inactive source identities separate if their areas
are reactivated. Other imports are Heavens #359, #363, #368, #369, and #384, plus
Winterhaven #55437, a memory of Torrhan
([resets](../../../areas/zon/torrhan.zon#L300),
[Heavens #359](../../../areas/obj/heavens.obj#L3725),
[Heavens #363](../../../areas/obj/heavens.obj#L3783),
[Heavens #368](../../../areas/obj/heavens.obj#L3836),
[Heavens #369](../../../areas/obj/heavens.obj#L3850),
[Heavens #384](../../../areas/obj/heavens.obj#L4020),
[Winterhaven #55437](../../../areas/obj/wh.obj#L5179)). The reset also names
object #6087, for which no object prototype exists in this checkout
([reset](../../../areas/zon/torrhan.zon#L569)).

No external type-25 object targets a Torrhan room. The local portal #66646 to
#66780 remains a separate item route; it does not change WLD component or
arrival counts.

### Shops and load-source caveat

The six records in `torrhan.shp` contain fifteen local product references.
Four cross-area shop-product references come from active area contributors:
Alatorin shops #83352 and #83356 list Torrhan object #66723
([first](../../../areas/shp/alatorin.shp#L351),
[second](../../../areas/shp/alatorin.shp#L399)); Shipy shop #43103 and
Tharnadia shop #132617 list fishing pole #66709
([Shipy](../../../areas/shp/shipy.shp#L47),
[Tharnadia](../../../areas/shp/tharnadia.shp#L622)). Three more product
references are in inactive `nexus56.shp` (#66604 and #66602) and
`tharnadia1.shp` (#66709)
([Nexus56 first product](../../../areas/shp/nexus56.shp#L259),
[Nexus56 second product](../../../areas/shp/nexus56.shp#L260),
[Tharnadia1](../../../areas/shp/tharnadia1.shp#L622)).

This checkout has no `areas/world.shp`. In full mode,
[`open_shop_stream`](../../../src/economy/shop.c#L3267) falls back to
concatenating individual `.shp` files
([fallback](../../../src/economy/shop.c#L3278)); mini mode reads
[`areas_mini/world.shp`](../../../areas_mini/world.shp), which currently
contains only the terminator. The selected startup mode and shop availability
were not tested. The listed inactive shop records are source references, not
proof that their keepers or stock are available in a running world.

Torrok's assistant #66723 is a mobile keeper; OBJ #66723 is a separate
prototype, the sparkling potion
([mobile](../../../areas/mob/torrhan.mob#L1727),
[object](../../../areas/obj/torrhan.obj#L1845)). The assistant's shop lists
#6087, which has no object prototype, and #384, which resolves to Heavens'
water barrel ([shop record](../../../areas/shp/torrhan.shp#L111),
[missing product](../../../areas/shp/torrhan.shp#L116),
[assistant reset](../../../areas/zon/torrhan.zon#L565),
[missing reset object](../../../areas/zon/torrhan.zon#L569)). Do not infer
successful shop loading from the declarations alone.

### Same-type definitions, history, and maintained code

After stripping color codes, terminators, and spacing, “City of Torrhan” is
unique among all 442 current ZON titles
([title](../../../areas/zon/torrhan.zon#L2)). No active or inactive external
WLD, MOB, or OBJ source defines the same-type VNUMs as a Torrhan prototype;
the inactive WLD scan also found no Torrhan room overlaps. The inactive
`torr` source is a 300-room City of Fehlnan map around #79000. Its market
description and unlisted QST share story material with active Torrhan,
including the troubled King Torrhan, dragon-scale clue, secret beach route,
Rhithel, and Zrilxa. This establishes a narrative parallel, not a tracked
rename or replacement; preserve the separate source/VNUM identities while
the builder relationship remains open. See the [Torr audit](TORR_AUDIT_REVIEW.md)
and its [twenty builder questions](TORR_BUILDER_QUESTIONS.md).

Followed `torrhan` WLD, MOB, OBJ, ZON, QST, and SHP histories remain on those
same source stems through the 2009 `areas/` relocation and later edits. The
story sidecar was added in commit `4085a7810` on 2026-10-03. No tracked source
establishes a distinct predecessor, alternate-stem rename, or whole-zone
replacement. Preserve the separate inactive map identities.

Maintained code has typed Torrhan references in several systems: CHAOS starter
kits use local objects #66675, #66636, and #66630
([sorcerer kit](../../../src/account/chaos_eq_data.h#L152),
[illusionist kit](../../../src/account/chaos_eq_data.h#L260),
[paladin kit](../../../src/account/chaos_eq_data.h#L351)); the epic table names
Thurdorf #66671 ([epic teacher](../../../src/classes/epic_skills.c#L215));
the Githzerai room table contains #66651
([innate table](../../../src/classes/innates.c#L2010)); fishing checks rod
#66709 ([fishing lookup](../../../src/economy/tradeskill.c#L573)); ship code
names port #66689 and crew room #66735
([port](../../../src/ships/ship_variables.c#L43),
[crew assignment](../../../src/specs/specs.assign.c#L2400)); ferry data names
room #66688 ([ferry stop](../../../src/world/ferryact.c#L136)); and
specialization code binds local teachers #66736, #66732, and #66731
([assignments](../../../src/specs/specs.assign.c#L462),
[AP](../../../src/specs/specs.assign.c#L469),
[Mercenary](../../../src/specs/specs.assign.c#L472)). These typed references
must retain their room, mobile, or object roles during any future map work.

Keep active `torrhan` **DO NOT MOVE**. Preserve its four reciprocal Surface
pairs, the separate Winterhaven administrative arrival, the Shipy contract
consumer, local imported resets, and the three source-only Surf edges on the
existing candidate row. The twenty builder questions and conservative
recommendations are in the
[Torrhan dossier](../zone-stories/CITY_OF_TORRHAN.md#explicit-builder-questions-and-recommended-resolutions).
Source declarations do not qualify live travel, item availability, shop
stock, actor state, or safe return.

## Checkpoint 848: Current typed ownership and shop records refreshed

The bounded source closure remains the one recorded at Checkpoint 393.
Shipy's active QST and sidecar still use Torrhan OBJ #66723; its active
Alatorin reset/shop references and Shipy/Tharnadia fishing-pole references
retain their existing owners. Torrok MOB #66723 is a separate typed prototype
from the Torrhan sparkling-potion OBJ #66723. Keep these item, mobile, and
contract roles distinct. ([Shipy QST](../../../areas/qst/shipy.qst#L895),
[Shipy sidecar](../../../areas/story/shipy.story.json#L1127),
[mobile](../../../areas/mob/torrhan.mob#L1727),
[object](../../../areas/obj/torrhan.obj#L1845))

Torrhan's imported reset definitions #72 and #41 retain their active Heavens
identities alongside inactive same-number Hostel, Mavulsk, and Dalvik sources.
The other Heavens and Winterhaven object imports remain as recorded, and #6087
still has no object prototype in this checkout. The six active outside ZON
reset roles and four active outside SHP product records are unchanged. Three
more SHP references and three reset records remain on inactive sources. The
full-mode shop-loader fallback and mini aggregate path remain distinct; neither
path establishes that inactive products or current stock are available in a
running world. No external type-25 object targets Torrhan; local OBJ #66646
remains the item route to #66780, separate from WLD exits.
([Torrhan reset imports](../../../areas/zon/torrhan.zon#L287),
[remaining imports](../../../areas/zon/torrhan.zon#L300),
[missing object reset](../../../areas/zon/torrhan.zon#L569),
[local portal](../../../areas/obj/torrhan.obj#L696),
[loader fallback](../../../src/economy/shop.c#L3278))

The four active reciprocal Surface boundaries and separate one-way Winterhaven
administrative arrival remain unchanged. Three inactive Surf approaches to
#66689/#66712/#66692 stay source-only on candidate row 97; the current
cumulative total remains 11,993 overall and 415 Surf, with these edges already
included. Keep active `torrhan` **DO NOT MOVE**. The inactive `torr` story
parallel remains separate from the active map and is not a tracked rename or
replacement. No new candidate, route activation, or migration clearance is
added.
