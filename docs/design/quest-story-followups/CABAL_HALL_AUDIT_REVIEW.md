# Cabal Hall Audit Review

Reviewed October 11, 2026, Checkpoint 459. This is a static source, story, WLD graph, followed-history,
shop-loader, and typed-reference review of the unregistered `cabal_hall` component. It does not establish
live loading, player access, shop operation, ownership, or permission to move source files.

## Source scope and identity

The current source family contains [WLD](../../../areas/wld/cabal_hall.wld),
[MOB](../../../areas/mob/cabal_hall.mob), [OBJ](../../../areas/obj/cabal_hall.obj),
[ZON](../../../areas/zon/cabal_hall.zon), and [SHP](../../../areas/shp/cabal_hall.shp).
The OBJ file is zero bytes and defines no local objects. The zone header is #301, titled
“The Cabal Guildhall,” with last VNUM #30109 and reset mode 0. The files define ten rooms
#30100–#30109, two mobiles #30100–#30101, no objects, and one SHP record keyed #30101.

The component is absent from the current [full area manifest](../../../areas/AREA) and
[QST manifest](../../../areas/qst/AREA). Its source-only status does not resolve its intended use.
The full manifest lists active `underworld3` across *300–303
([area row](../../../areas/AREA#L145)); that active WLD defines every room VNUM #30100–#30109.

## Environmental story and content review

The WLD depicts an entrance into a guildhall with a Netherworld nexus, an underground temple,
a drug deposit, a “real world” gate, an energy vortex, a library, a torture room, an oubliette,
and an unfinished void ([entrance](../../../areas/wld/cabal_hall.wld#L1),
[temple](../../../areas/wld/cabal_hall.wld#L34),
[drug deposit](../../../areas/wld/cabal_hall.wld#L58),
[gate](../../../areas/wld/cabal_hall.wld#L69),
[energy vortex](../../../areas/wld/cabal_hall.wld#L94),
[library](../../../areas/wld/cabal_hall.wld#L80),
[torture room](../../../areas/wld/cabal_hall.wld#L117),
[oubliette](../../../areas/wld/cabal_hall.wld#L132),
[unfinished room](../../../areas/wld/cabal_hall.wld#L143)). The narrative includes drug-dealing,
sexualized and pornographic content, a homophobic slur, and explicit anti-LGBTQ threats of violence.
Ask the content owner to decide how those passages should be handled before any path or loader proposal.

The MOB file defines a dark golem and a drug dealer
([mobiles](../../../areas/mob/cabal_hall.mob#L1)). The ZON repeats four golems in room #30100,
places one dealer in #30103, and supplies 13 equipment G commands following the dealer reset
([mobile resets](../../../areas/zon/cabal_hall.zon#L13),
[dealer equipment](../../../areas/zon/cabal_hall.zon#L18)). No native QST or story sidecar
records a task or completion contract for the guildhall.

## Active Underworld3 ownership of room VNUMs

All ten Cabal Hall room VNUMs #30100–#30109 are active Underworld3 room identities. For example,
Cabal Hall calls #30100 its guild entrance and #30104 its gate to the real world; active Underworld3
uses #30100 for a large underground lake and #30104 for an underground river
([Cabal entrance](../../../areas/wld/cabal_hall.wld#L1),
[Cabal gate](../../../areas/wld/cabal_hall.wld#L69),
[Underworld3 #30100](../../../areas/wld/underworld3.wld#L1999),
[Underworld3 #30104](../../../areas/wld/underworld3.wld#L2085)). Every candidate room field uses
zone 301; the current active Underworld3 source uses zone field 300 for these room VNUMs. All ten
candidate rooms are reachable by numeric resolution in the active world as Underworld3 rooms.

Maintained code also binds an inn special at room VNUM #30104
([assignment](../../../src/specs/specs.assign.c#L2451)). In the current active source list, #30104
belongs to Underworld3's river map; Cabal Hall uses that number for its gate room. The code and
map records establish a shared numeric identity that needs an owner decision. They do not show which
builder intended to own the inn behavior.

The active world has no mobile or object prototype with VNUM #30100–#30109. Cabal Hall mobiles
#30100 and #30101 therefore remain source-only prototypes. No active WLD source outside Underworld3
was found targeting the Cabal Hall room numbers; candidate-specific active inbound exits remain zero.
Inactive `nah.wld` reuses #30100–#30112 with a different guildhall map and title, and
`shadow_hall.wld` reuses #30100–#30102 with a separate Shadow Law identity
([NAH map](../../../areas/wld/nah.wld#L1), [NAH title](../../../areas/zon/nah.zon#L1),
[Shadow Law map](../../../areas/wld/shadow_hall.wld#L1)). These source identities are not listed
in the full area manifest. The exact-number overlaps establish an ownership question, while the
source histories do not establish that one map replaced another.

## Static WLD graph and boundary exit

The Cabal Hall WLD has ten rooms and 19 authored directions: 18 local and one outbound cross-zone
exit. The local graph forms one weak component, and directed traversal from #30100 reaches all ten
rooms. Every room zone field is 301, consistent with its VNUM range.

| Cabal Hall source exit | Target | Static classification |
| --- | --- | --- |
| #30100 D4 up | Duris3 #153434, “Grassy Plains” | Target exists only in unregistered Duris3. Its authored exits lead to other Duris3 rooms; no return to #30100–#30109 was found. |

The outbound record does not create active ingress into Cabal Hall. Its target is outside the current
full area list, and no reciprocal Duris3 exit returns to the candidate. The active Underworld3
same-numbered rooms remain a separate map identity. The candidate has no negative target, fall-chance
field, or current field. This review adds no eligible inactive-WLD approach; totals remain 11,987
overall / 415 Surf. See the [Duris3 target](../../../areas/wld/Duris3.wld#L1122115) and the
[Underworld3 audit](UNDERWORLD_AUDIT_REVIEW.md#underworld3-room-overlap-and-the-compiled-inn-binding).

## Reset, shop-stock, and other typed dependencies

The Cabal Hall ZON has three O resets, five M resets, and 13 G equipment resets. Its O commands
import Player Castles objects #11051, #11101, and #11013 into rooms #30100, #30102, and #30104
([imports](../../../areas/zon/cabal_hall.zon#L9)); those external object prototypes remain distinct
from Cabal Hall's empty OBJ file. Current Player Castles object records include
[#11051](../../../areas/obj/player_castles.obj#L129),
[#11013](../../../areas/obj/player_castles.obj#L121), and
[#11101](../../../areas/obj/player_castles.obj#L249). Preserve these three source-only imports.

The G resets and shop stock reference 13 items: #3050, #108, #109, #111, #377, #412, #399, #53,
#6089, and #491–#494. Twelve IDs have current active OBJ prototypes; #6089 has no current active
prototype. The CBL Hall review traces the old #6089 item to its renumbered Tharnadia prototype
([dependency review](CBL_HALL_AUDIT_REVIEW.md#typed-inbound-and-outbound-dependencies)). The
four objects #491–#494 are active gathering-bag prototypes in Heavens
([first bag](../../../areas/obj/heavens.obj#L5055)); #3050 is defined in Verz1
([prototype](../../../areas/obj/verz1.obj#L850)). Tracked commit `ed94862f0` added #491–#494 to both
Cabal Hall's ZON G resets and SHP product list. The source changes align with the gathering-bag
stock contract described in that commit; they do not establish that this unlisted shop is loaded.

The SHP file contains one shop record: keeper VNUM #30101, shop room #30103, and the 13 products
above ([record](../../../areas/shp/cabal_hall.shp#L1),
[keeper and room fields](../../../areas/shp/cabal_hall.shp#L47)). The candidate's own #30101 mobile
is the drug dealer; the active mobile catalog has no #30101 prototype. `SHOP_FILE` is
`areas/world.shp` ([definition](../../../src/world/db.h#L22)); that aggregate is absent from this
checkout. In non-mini mode, `open_shop_stream` falls back to globbing `areas/shp/*.shp`
([loader](../../../src/economy/shop.c#L3267),
[fallback](../../../src/economy/shop.c#L3278)), which includes the candidate SHP if that path is
used. While parsing, the loader translates the keeper VNUM through `real_mobile`
([lookup](../../../src/economy/shop.c#L3493)). Shop use passes its configured room VNUM through
`real_room` ([room lookup](../../../src/economy/shop.c#L122)); in the current active map, #30103
resolves to Underworld3's river room rather than Cabal Hall's drug deposit. This is a static loader
and VNUM conflict. Shop operation and startup behavior were not tested.

No exact Cabal Hall #30100–#30109 C/C++ use was found beyond the #30104 inn assignment above.

## Title history and neighboring guildhall sources

Followed paths show cabal_hall WLD and MOB files at the same stem since the 2009 area-tree move,
with the WLD last changed in 2013; its tracked OBJ file is empty. The ZON and SHP have current
same-stem records from the 2013 source history and were updated in commit `ed94862f0` in September
2026 to add gathering bags #491–#494. No alternate-stem rename or explicit replacement decision
was found.

The ZON title closely matches the separate `cabal` title “Cabal Guildhall.” Cabal uses room and
mobile VNUMs #29000–#29012; Cabal Hall uses #30100–#30109 and its room story is different. Inactive
`nah` and `shadow_hall` also use room VNUMs in the #30100 range with different stories. Active
Underworld3 currently owns #30100–#30109. These similarities and collisions document several
separate source identities; they do not resolve ownership or establish replacement. See the
[Checkpoint 458 Cabal audit](CABAL_GUILDHALL_AUDIT_REVIEW.md#followed-history-and-similarly-titled-sources)
and the related [Underworld3 review](UNDERWORLD_AUDIT_REVIEW.md#underworld3-room-overlap-and-the-compiled-inn-binding).

## Safe disposition

Keep `cabal_hall` **RETAIN IN PLACE**. Resolve active Underworld3 room ownership, the #30104 inn
binding, the Duris3 boundary, the similarly titled `cabal`, the other #301xx source maps, the
unlisted shop and missing keeper prototype, the #6089 stock gap, gathering-bag distribution intent,
content review, editor/tooling, and persisted VNUM identity before proposing a separate path change.
No area, code, shop, gameplay, manifest, or world-list change is authorized by this audit. The full
owner-question list is in the [Cabal Hall builder questions](CABAL_HALL_BUILDER_QUESTIONS.md).

## Checkpoint 514: Nah room and mobile overlaps

The unregistered Nah WLD uses all thirteen VNUMs #30100–#30112 already owned
by active Underworld3, including the final three rooms beyond Cabal Hall’s
#30100–#30109 range. Nah’s own candidate graph has components of ten and
three rooms, with two one-way Duris3 exits. Its room titles and descriptions
are distinct from Underworld3 and Cabal Hall. The mobile VNUMs #30100/#30101
also collide with Nah’s separate candidate mobile records. See the
[NAH audit](NAH_AUDIT_REVIEW.md) and
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-514-nah-and-competing-room-identities).