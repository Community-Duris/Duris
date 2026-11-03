# Bastard source audit

Canonical key: `component:bastard`. Physical coverage row 364; attic
candidate row 13. The five source files have no `areas/AREA`, `qst/AREA`,
random-area, map JSON, or story-sidecar registration. The mixed status is
especially important here: active `underworld3` already owns all five room
numbers in the candidate, while unregistered `sunhall` defines the same five
rooms and overlapping mobile/object identities.

## Source identity and loader boundaries

The source consists of `areas/wld/bastard.wld`, `areas/mob/bastard.mob`,
`areas/obj/bastard.obj`, `areas/zon/bastard.zon`, and `areas/shp/bastard.shp`.
No `bastard` entry appears in the full-area or QST area lists, and no
`areas/map/bastard.json` or `areas/story/bastard.story.json` exists. There is
no local QST source or native quest contract. The source title is raw ZON
header #302, “Die hall” ([header](../../../areas/zon/bastard.zon#L1)).

The standalone shop record is keyed to keeper mobile #30201, specifies shop
room #30202, and lists twelve sale items ([record](../../../areas/shp/bastard.shp#L1),
[stock](../../../areas/shp/bastard.shp#L3), [keeper field](../../../areas/shp/bastard.shp#L44),
[room field](../../../areas/shp/bastard.shp#L46)). `areas/world.shp` is absent in
this checkout. The shop loader's `open_shop_stream()` falls back to globbing
`areas/shp/*.shp` if the aggregate is missing
([loader](../../../src/economy/shop.c#L3267)); this source path does not prove
that mobile #30201 or any stock item resolves in a live world. The fallback
also sees `sunhall.shp`, a separate source record with the same shop key
#30201; preserve the two assignments as distinct until shop ownership and
aggregate generation are established.

## Story and area structure

The entrance is an exposed Yggdrasil root guarded by frost giants. A short
passage enters a durithalite cave described as a duergar excavation, with a
carved Clavados mark, preserved remains, dark torches, and a stairway to a
Blood and Skull tavern. The west alcove is described as Ilixctthu's guild
shop. The tavern contains separate lodging and barracks spaces, different
sleeping places for its races, and the room text of a raiding guild.

Two source identity details remain distinct: the room mark names Clavados,
while the one local badge object says it is the symbol of Carnage. Its text
features a blood-red skull and crossed bones and the phrase “HAS MADE PEOPLE
DIE!” The tomb/raiding imagery should not be turned into an inferred
organization relationship without a builder's confirmation. See the
[story dossier](../zone-stories/BASTARD.md).

## Static graph and numeric room ownership

| Measure | Source result |
| --- | ---: |
| WLD rooms | 5 (#30200–#30204) |
| Authored directions | 9: 8 within the five-number set, one outbound |
| Local weak components | 1, containing all 5 rooms |
| Forward reach from #30200 within the source graph | 5 / 5 |
| Source rooms with an authored local return to #30200 | 5 / 5 |
| Fall-chance rooms / current rooms | 0 / 0 |

The north exit from #30200 enters #30201; the connected local chain reaches
the shop, tavern, and barracks. The up exit from #30200 instead targets
#1120721 in unregistered `ud_map2` ([exit](../../../areas/wld/bastard.wld#L18),
[target room](../../../areas/wld/ud_map2.wld#L2213)); that room is titled “The
Endless Rock Filled Cavern Ithilienienzuth.” No `ud_map2` exit targets #30200,
so that source-only boundary has no authored return edge to this candidate.

All five candidate WLD VNUMs also have definitions in active
[`underworld3.wld`](../../../areas/wld/underworld3.wld#L4103), where
#30200–#30204 are old dwarven mining-tunnel records. All five room descriptions
and exits differ from the `bastard` definitions. Unregistered
[`sunhall.wld`](../../../areas/wld/sunhall.wld#L1) independently defines the
same five VNUMs with pyramid rooms. Sunhall's separate prototype sets are
visible in its [MOB](../../../areas/mob/sunhall.mob#L1),
[OBJ](../../../areas/obj/sunhall.obj#L1), and
[SHP](../../../areas/shp/sunhall.shp#L1) files. Thus the active target owner for those
five numbers is `underworld3`; active numeric exits among them are not
candidate-specific ingress to `bastard`. The `bastard` and `sunhall` room
records remain separate source identities, not proof of replacement or
co-loading. Candidate-specific active inbound remains zero.

The title “Die hall” is unique among the current 442 ZON files, but header
#302 appears in both `bastard.zon` and `sunhall.zon`. `underworld3` is listed
at [`areas/AREA` line 145](../../../areas/AREA#L145) with a wider *300–303
range and header #300. All five candidate rooms are inside its active WLD
definitions even though the candidate's own area contributor is unlisted.
The current inactive-WLD approach recount is unchanged: the one outbound
direction targets unregistered `ud_map2`, not an active WLD owner.

## Typed resets, shop stock, and overlaps

The ZON has 22 commands: five mobile resets, three object-in-room resets, and
fourteen give resets. Its five M commands reference the three local mobile
prototypes (#30200–#30202); its one local G reset references badge object
#30200. The three O resets import Player Castles objects #11060, #11110, and
#11013 into local rooms #30200, #30203, and #30204
([first reset](../../../areas/zon/bastard.zon#L9),
[other resets](../../../areas/zon/bastard.zon#L10),
[third reset](../../../areas/zon/bastard.zon#L11),
[altar prototype](../../../areas/obj/player_castles.obj#L121),
[magic-mouth prototype](../../../areas/obj/player_castles.obj#L201),
[board prototype](../../../areas/obj/player_castles.obj#L321)).
Those are source-only imports because `bastard.zon` is not in the full-area
list.

The remaining thirteen G resets reference object IDs #53, #6094, #347, #377,
#364, #6084, #6089, #412, #3050, #398, #108, #109, and #111. Exact current
object-source lookup finds no prototype for #6084, #6089, or #6094. The shop
also offers #6089 despite that missing current prototype. The other IDs have
current definitions: #347/#377/#364/#412/#398/#108/#109/#111 in active
Heavens, #3050 in active Verz1, and #53 in active Heavens plus source-only
Dalvik, Hostel, and Mavulsk. #108 and #109 also have different source-only
Mavulsk definitions. These competing and absent source identities are not
resolved by the candidate's source text ([give resets](../../../areas/zon/bastard.zon#L18),
[shop inventory](../../../areas/shp/bastard.shp#L3),
[Heavens prototypes](../../../areas/obj/heavens.obj#L788),
[Verz1 prototype](../../../areas/obj/verz1.obj#L850),
[Mavulsk prototypes](../../../areas/obj/mavulsk.obj#L1798)).

Source-only `sunhall` also uses ZON header #302 and rooms #30200–#30204. It
has distinct room text, mobiles #30200–#30203, objects #30200–#30204, and an
SHP record keyed to its own Ahash mobile #30201. The exact active room overlap
for this candidate is five (active owner `underworld3`); there is no active
same-type mobile or object owner for the candidate's #30200–#30202/#30200
prototypes. Keep the Sunhall prototype and shop references separate from
Ilixctthu and the local badge.

The Player Castles object imports are already tracked in
[candidate row 13](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L13). A repository
scan found no outside ZON reset that resolves specifically to the candidate's
local mobile/object prototypes and no compiled source callback keyed to those
VNUMs. Numeric room exits from active `underworld3` resolve to its own
same-number rooms. No QST or structured story-sidecar consumer was found.

## Title, history, and safe disposition

The normalized ZON title “Die hall” occurs once in the current 442-file scan.
Header #302 occurs twice: `bastard` and `sunhall`. Followed WLD/MOB/OBJ/ZON/SHP
histories reach the 2009 area-tree move; MOB/OBJ/ZON also record the 2012
format-field update, and the current same-stem records share a 2013 “stuff”
commit. No tracked path shows an explicit alternate-stem predecessor or
whole-zone replacement. The active `underworld3` VNUM ownership and unlisted
`sunhall` overlap are independent current source evidence, not history-based
replacement proof.

Keep candidate row 13 **RETAIN IN PLACE**. Preserve the three-way room-number
collision, active `underworld3` ownership, separate `sunhall` records, the
`ud_map2` outbound reference, Player Castles object imports, unresolved object
IDs, and the same-key shop sources. Do not move or archive the files until
the owner resolves area identity, VNUM ownership, the two guild/shop sources,
missing stock/reset prototypes, and editor/persisted/live state. No native
area or gameplay source changed. See the [story dossier](../zone-stories/BASTARD.md),
[builder questions](../zone-stories/BASTARD_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions),
[identity review](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-451-bastard-die-hall-room-ownership-and-history),
[retention plan](ATTIC_MIGRATION_PLAN.md#checkpoint-451-bastard-retained-with-room-and-shop-collisions),
[candidate row 13](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L13),
[Sunhall candidate row 96](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L96),
[ud_map2 candidate row 114](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L114), and
[coverage row 364](ZONE_COVERAGE_REGISTER.csv#L364).
