# Village of Werrun: repository-wide source audit supplement

This supplement reconciles the [source-comprehensive Werrun dossier](../zone-stories/VILLAGE_OF_WERRUN.md)
with active WLD boundaries, the all-source WLD sweep, typed cross-zone
references, title and replacement history, and shared-code use. Findings come
from static repository sources. They do not establish live access, merchant
availability, played receipts, or persisted player history.

## Active boundaries and local graph

Werrun has 84 rooms and 197 authored exits. All 197 targets resolve in the
active WLD set: 196 exits are internal and one leaves Werrun. Its local
WLD-only weak graph has components of 43 and 41 rooms. The only outbound exit
is Werrun #38300 south to Surface #527537
([outbound record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L546)).

Two active exits enter Werrun:

| Incoming exit | Local destination | Relationship | Boundary record |
| --- | --- | --- | --- |
| Surface #527537 north | #38300 | Reciprocal with Werrun #38300 south | [record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1377) |
| Wh #55614 west | #38316 | One-way authored arrival from a dispersal room | [record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L766) |

The Surface boundary is the area's sole reciprocal pair. Wh #55614 has no
reverse WLD exit. These components and edges describe authored exits only;
the Werrun dossier separately reviews the mirror teleports, doors, resets, and
dynamic routes. Static topology does not establish ordinary access, return
safety, or played discovery.

## Unregistered WLD source sweep

The all-95-file scan parsed 527,791 rooms and 2,080,773 exits with zero parse
issues. No unregistered source room shares a VNUM with Werrun. Two unregistered
exits lead north to local room #38300:

| Source-only exit | Local target | Evidence |
| --- | --- | --- |
| `oldmap8.wld` room #181462, “The Vast Grey Drylands” | #38300 | [exit fields](../../../areas/wld/oldmap8.wld#L30695), [target](../../../areas/wld/oldmap8.wld#L30698) |
| `surf.wld` room #215822, “Grassy Meadows of the Wyvernspur Valley” | #38300 | [exit fields](../../../areas/wld/surf.wld#L331954), [target](../../../areas/wld/surf.wld#L331957) |

Both sources are absent from the active area manifest, so these records add no
active WLD entrance. They raise the curated source-only total from 600 to 602.
The records are separate from the active Surface pair, even though they reach
the same Werrun room.

## Typed dependencies and foreign ownership

The active Shipyards QST contains one exact item command referencing Werrun
cigar box #38314. Its sidecar describes Frull's separate foreign-owned
commission for the cigar box and 66,666 copper; it records that receipt under
Shipyards, not Werrun
([QST payment](../../../areas/qst/shipy.qst#L825),
[QST item command](../../../areas/qst/shipy.qst#L826),
[cigar object](../../../areas/obj/werrun.obj#L237),
[sidecar completion](../../../areas/story/shipy.story.json#L1037),
[typed item field](../../../areas/story/shipy.story.json#L1046)). All 231
external story sidecars parsed without issues; this is the one exact structured
`item_vnums` match to a Werrun prototype.

Foreign reset sources reuse Werrun prototypes. Active Alatorin resets place the
cigar box #38314 in room #83380, reset the Werrun muscular horse #38303 in
room #83361, and equip the separate Werrun object #38303, a slab of beef, on
starving warriors ([cigar reset](../../../areas/zon/alatorin.zon#L1584),
[horse reset](../../../areas/zon/alatorin.zon#L2811),
[beef reset](../../../areas/zon/alatorin.zon#L3282),
[local prototypes](../../../areas/mob/werrun.mob#L49) and
[objects](../../../areas/obj/werrun.obj#L46),
[cigar](../../../areas/obj/werrun.obj#L237)). The comprehensive dossier maps
five complete Alatorin reset groups. A separate unregistered `nexus56.zon`
row resets Werrun book #38326 ([reset](../../../areas/zon/nexus56.zon#L1375),
[book](../../../areas/obj/werrun.obj#L412)). These declarations record source
reuse and reset ownership; they do not prove current population or stock.

An unregistered `nexus56.shp` shop record #83038 lists Werrun book #38326 as a
product ([record](../../../areas/shp/nexus56.shp#L493),
[product](../../../areas/shp/nexus56.shp#L498)). The `nexus56` stem is absent
from `areas/AREA`; its keeper and room #83038 are defined only in unregistered
Nexus56 source files ([mobile](../../../areas/mob/nexus56.mob#L569),
[room](../../../areas/wld/nexus56.wld#L792)). This checkout has no generated
`areas/world.shp`. When that aggregate is missing, `open_shop_stream` falls
back to every `areas/shp/*.shp` file and appends `end.shp`
([aggregate path](../../../src/world/db.h#L22),
[fallback](../../../src/economy/shop.c#L3272),
[source glob](../../../src/economy/shop.c#L3278),
[terminator handling](../../../src/economy/shop.c#L3304)). The source record
can therefore enter that fallback stream, while a live keeper binding and
player access remain unqualified.

The bounded reset-role scan found no external reset room field targeting a
Werrun room. It found no external mobile or object prototype definition that
reuses a Werrun VNUM. The scan of 870 foreign type-25 portal declarations found
no destination into Werrun. These negative findings are bounded to the exact
typed records scanned.

Shared C/C++ uses are classified by context. The active `world_quest` binding
on bartender #38309 starts generated missions
([assignment](../../../src/specs/specs.assign.c#L792)). Mobile #38317, Lady
Death, occurs in the random-equipment high-drop list, whose bonus lookup is
commented out ([list](../../../src/item/randomeq.c#L120),
[disabled lookup](../../../src/item/randomeq.c#L649)). Neither code reference
creates another Werrun QST receipt.

## Title, tracked history, and disposition

The active manifest lists Werrun as `*383`
([AREA entry](../../../areas/AREA#L185)); its ZON title is “Village of Werrun”
([header](../../../areas/zon/werrun.zon#L2)). The normalized title scan found
one match among 351 active titles. The curated
[duplicate and replacement register](DUPLICATE_AND_REPLACED_ZONES.md) has no
Werrun entry. The active WLD duplicate scan found no Werrun room collision; its
only global duplicate is #51800 in the two Hell areas
([Hell Three](../../../areas/wld/hell_three_minauros.wld#L1),
[Hell Two](../../../areas/wld/hell_two_dis.wld#L10375)).

The tracked WLD, MOB, OBJ, ZON, and QST paths reach the 2009 area-tree move
`642da5400`. Later history includes the 2013 Werrun area edits, the 2014
shop-file cleanup, and 2017 price/quest-item changes. The story sidecar was
added at `ce6948d16` on 2026-10-05. This bounded path/register review identifies
no distinct replacement source. Werrun remains a full active area contributor,
so its coverage disposition stays **DO NOT MOVE**.

## Recommended reconciliation

- Keep the four Werrun QST receipts and their local ownership as documented in
  the zone dossier. Keep Frull's Shipyards commission and Alatorin reset groups
  attributed to their declaring areas.
- Ask the builder whether the `nexus56.shp` #83038 listing of Vewon's book
  #38326 is intended for the shop-loader fallback. If it is, document its
  keeper and access ownership; if it is not, address the fallback source in a
  separately reviewed content/tooling change. The listing and reset should stay
  under review while their load and keeper context remains unresolved.
- Preserve the two source-only WLD inbounds as unregistered evidence. Qualify
  the active Wh arrival and return route separately before making ordinary
  player-access claims.
- Keep Werrun in the active manifest. This audit supports no attic move or
  native gameplay repair.

This checkpoint is a bounded static source audit. Indirect code paths,
editor/tooling consumers, persisted player data, live shop binding, and played
discovery, access, combat, and return journeys remain outside its scope.

## Checkpoint 431: Builder question review

The existing source-comprehensive dossier and this bounded audit already
record Werrun’s four native QSTs, the generated bartender missions, mirror and
bush travel, the traveller’s trapped branch, foreign prototype consumers, and
the conditional Nexus56 shop source. Twenty direct builder questions with
recommended resolutions now collect those owner decisions in one place:
[Village of Werrun builder questions](../zone-stories/VILLAGE_OF_WERRUN_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
This adds question coverage without changing any world source or converting
unqualified access, payment, recurrence, shop, or reward behavior into a
completed claim.
