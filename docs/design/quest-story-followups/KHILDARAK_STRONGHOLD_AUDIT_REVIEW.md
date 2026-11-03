# Khildarak Stronghold: repository-wide connectivity and dependency audit

**Checkpoint 86; priority 126 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive dossier](../zone-stories/KHILDARAK_STRONGHOLD.md)
with the active WLD graph, inactive WLD sources, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored edges
and static code destinations do not establish ordinary availability, safe
travel, or played qualification.

## Active WLD graph

The active manifest has 351 WLD contributors. Khildarak Stronghold has 751
rooms and 1,582 exits; all 1,582 resolve. The internal-exit graph has three
weak components of 749, 1, and 1 rooms. The singleton rooms are #17303, “Pet
Room for the duergar animal masters,” and #17592, “Slave Storage Room.” There
are no negative or positive unresolved exits, 51 rooms with fall-chance values,
and no rooms with current fields. The one global active-WLD duplicate room
VNUM, #51800 in Hell Two and Hell Three, is unrelated.

Khildarak has three reciprocal active boundaries and one additional active
inbound edge without a local reverse:

| Khildarak edge | Active return | Cross-area register rows |
| --- | --- | --- |
| #17000 down to Mushroom Caverns #24188 | Mushroom Caverns #24188 up to #17000 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L302), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L394) |
| #17377 west to Black Tunnels #10721 | Black Tunnels #10721 east to #17377 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L303), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L204) |
| #17746 south to Mushroom Caverns #24108 | Mushroom Caverns #24108 north to #17746 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L304), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L392) |
| Heavens #32 south to Khildarak #17087 | No local reverse edge | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L12) |

These authored edges do not qualify live doors, access, reset state, route
safety, or other dynamic travel.

## Inactive WLD sources

The all-95 inactive WLD scan parsed 2,080,773 exits without issues. No inactive
source-room VNUM overlaps Khildarak. It found one source-only approach:
`ud_map2.wld` room #1124854 has an up exit to Khildarak #17000. It is not active
ingress. This raises the curated source-only reference total to 1,091; the
reviewed Surf subset remains 120. The active Shafts exit to a VNUM in
`ud_map2.wld` and the earlier source-only Ixarkon and Worms references remain
separate dependencies. See the [ud_map2 coverage row](ZONE_COVERAGE_REGISTER.csv#L465)
and [candidate dependency row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114).

## Typed references and compiled numeric matches

The external typed scan found no QST item-command consumer of Khildarak
objects. Numeric QST collisions in `forgotten_forest.qst` and
`surfacekeeps.qst` are experience or coin rewards, not item or route references.
Five active Alatorin M reset records load selected Khildarak mobiles at rooms
#83511 and #83605; these are existing foreign reset uses, not additional local
completion units.

Khildarak also consumes Faang beer bottle #15242: its G reset loads the object
into keeper stock and its SHP lists the same object as a product
([reset](../../../areas/zon/khildarak.zon#L2389),
[product](../../../areas/shp/khildarak.shp#L167),
[Faang prototype](../../../areas/obj/faang.obj#L394)). This typed reference
does not establish current shop availability. See the
[Faang typed audit](FAANG_STORY_REVIEW.md#checkpoint-796-current-typed-census-refresh).

The matching external shop products occur only in unregistered source files:

| Source | Typed matches | Classification |
| --- | --- | --- |
| `nexus56.shp`, shop #83034 | Objects #17059 and #17060 | Source-only product rows for dried fish filet and fish eyes; `nexus56` is absent from the active area manifest. Its unregistered ZON also has source-only G records for these objects. |
| `player_guilds.shp`, shop #35199 | Object #17070 | Source-only product row for rolled parchment; `player_guilds` is absent from the active area manifest. |

The current checkout has no `areas/world.shp`. The
[static shop-loader fallback](../../../src/economy/shop.c#L3267) globs
`areas/shp/*.shp` when the aggregate file cannot be opened, so these matches
indicate possible parser exposure under that fallback, not a claim that either
shop is loaded, operating, or available. The scan found no
external mobile or object prototype definition for Khildarak VNUMs, no external
type-25 portal destination into Khildarak among 871 declarations (712 active,
159 source-only), and no exact `item_vnums` hit across 231 parsed external
story sidecars.

A context-reviewed C/C++ scan found these additional typed references:

| Local target | Static reference | What the source establishes |
| --- | --- | --- |
| Room #17021 | [Chaos destination alias](../../../src/combat/chaos.c#L41) | The destination table associates this room with “khild” and “khildarak”; static code checks govern portal resolution. The room destination is distinct from the same-numbered warhammer object. |
| Room #17607 | [Innate shift destination list](../../../src/classes/innates.c#L2009), [psionic travel list](../../../src/classes/psionics.c#L615) | Random destination arrays include this room; membership alone does not prove a selected or successful relocation. |
| Room #17001 | [Hometown mapping](../../../src/core/constant.c#L1726), [guild-location table](../../../src/core/constant.c#L1912) | The hometown and guild-location tables name Khildarak rooms. |
| Room #17736 and #17087 | [Auction-room registry](../../../src/economy/auction_room_registry.c#L9), [inn constant](../../../src/guild/guildhall.h#L40), [generated portal destination](../../../src/guild/guildhall_rooms.c#L438) | Khildarak has a registered auction room and generated guildhall portals can target its inn. |
| Room #17643 and local mobile/object handlers | [Guard room check](../../../src/specs/specs.guards.c#L60), [special assignments](../../../src/specs/specs.assign.c#L1003), [archer attack table](../../../src/specs/specs.monster_attacks.c#L235) | The source dossier maps the active local guard, combat, pet-shop, inn, tradeskill, founder, and other bindings. The [duergar_guild helper](../../../src/specs/specs.room.c#L1417) can select mobile #17260, but its [room assignments](../../../src/specs/specs.assign.c#L2362) are commented out and are not active bindings. |

Remaining raw numeric matches are constants, decimal values, comments, or
namespace collisions. Room, mobile, object, reward, and shop-product values
were classified by source context rather than treated as interchangeable.

## Title, history, and disposition

The color-stripped title in [khildarak.zon](../../../areas/zon/khildarak.zon#L2)
normalizes to “Khildarak Stronghold” and is unique among 351 active
contributors. The active WLD/ZON history traces source edits and path changes,
including a 2009 path move, but identifies no distinct predecessor or
replacement in the curated [replacement register](DUPLICATE_AND_REPLACED_ZONES.md).
The active, un-commented [areas/AREA row](../../../areas/AREA#L91) remains
**DO NOT MOVE**.

The existing dossier covers local quests, reset behavior, actor bindings,
shared controls, shops, and selected foreign ownership. This repository-wide
scan adds the active boundary census and classifies inactive-WLD, typed, and
compiled references. Editor/tooling closure, persisted identity, dynamic route
availability, door/reset state, safe travel, and played qualification remain
separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.
