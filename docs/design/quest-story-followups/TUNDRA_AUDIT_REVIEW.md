# Tundra: source-boundary and typed-reference supplement

Status: repository-wide static supplement to the existing
[Tundra dossier](../zone-stories/TUNDRA.md). It reconciles active boundaries,
current unregistered WLD inputs, exact external prototype references, and the
active-title/history boundary. No area data, story sidecar, or gameplay code
was changed or qualified.

## Active and source-only WLD boundaries

The active area entry is `tundra *137-139`; the canonical zone header is
`#137`. The existing dossier records 175 active rooms, 401 authored exits,
the 19 directed boundary records, and the disabled or unusable outgoing
destinations. The linked exit-target lines show the eight reciprocal pairs:

| Active boundary | Evidence |
| --- | --- |
| Tundra 13703 west ↔ Surface 538336 east | [Tundra exit](../../../areas/wld/tundra.wld#L94), [Surface exit](../../../areas/wld/surface.wld#L804790) |
| Tundra 13712 south ↔ Surface 514280 north | [Tundra exit](../../../areas/wld/tundra.wld#L268), [Surface exit](../../../areas/wld/surface.wld#L299856) |
| Tundra 13733 south ↔ Underworld3 30264 north | [Tundra exit](../../../areas/wld/tundra.wld#L706), [Underworld3 exit](../../../areas/wld/underworld3.wld#L5477) |
| Tundra 13778 east ↔ Surface 538746 west | [Tundra exit](../../../areas/wld/tundra.wld#L1475), [Surface exit](../../../areas/wld/surface.wld#L813388) |
| Tundra 13796 west ↔ Surface 542729 east | [Tundra exit](../../../areas/wld/tundra.wld#L1827), [Surface exit](../../../areas/wld/surface.wld#L896995) |
| Tundra 13870 west ↔ Surface 523898 east | [Tundra exit](../../../areas/wld/tundra.wld#L2786), [Surface exit](../../../areas/wld/surface.wld#L501772) |
| Tundra 13895 east ↔ Surface 543120 west | [Tundra exit](../../../areas/wld/tundra.wld#L2913), [Surface exit](../../../areas/wld/surface.wld#L905198) |
| Tundra 13936 west ↔ Surface 543909 east | [Tundra exit](../../../areas/wld/tundra.wld#L3447), [Surface exit](../../../areas/wld/surface.wld#L921784) |

The one-way incoming staff-room edge is Heavens 1221 northeast to Tundra
reception 13714 ([Heavens exit](../../../areas/wld/heavens.wld#L1170)). The
outgoing south target from 13869 is `-1`; the east target from 13893 is
inactive legacy room 110038, present only in `Duris3.wld`
([Tundra exits](../../../areas/wld/tundra.wld#L2772),
[legacy-only target](../../../areas/wld/tundra.wld#L2861)). The existing
dossier treats these separately from ordinary access.

A stateful parse of all 95 current unregistered WLD files read 2,080,773
authored D0–D9 exits with zero parse issues. Seven source-only exits in
`surf.wld` target active Tundra rooms:

| Unregistered source room | Direction and active target | Evidence |
| --- | --- | --- |
| 213929 | East (`D1`) to Tundra 13796 | [exit target](../../../areas/wld/surf.wld#L292308) |
| 217352 | North (`D0`) to Tundra 13712 | [exit target](../../../areas/wld/surf.wld#L364059) |
| 219160 | East (`D1`) to Tundra 13870 | [exit target](../../../areas/wld/surf.wld#L401991) |
| 219780 | East (`D1`) to Tundra 13703 | [exit target](../../../areas/wld/surf.wld#L414987) |
| 219787 | West (`D3`) to Tundra 13778 | [exit target](../../../areas/wld/surf.wld#L415134) |
| 224279 | East (`D1`) to Tundra 13936 | [exit target](../../../areas/wld/surf.wld#L509354) |
| 224287 | West (`D3`) to Tundra 13895 | [exit target](../../../areas/wld/surf.wld#L509522) |

These records are in unregistered WLD source, not the active area input. They
are not active-world entrances and do not change the 401-exit active census or
establish ordinary access, safe arrival, or a return path. They increase the
curated source-only record count from 375 to 382.

## Direct external typed references

The existing dossier already traces Cairme's Ship Yards ration exchange and
the broader foreign source dependencies. The bounded exact-prototype scan
confirms that consumer and records these active reset and shop references to
local Tundra objects:

| Current source | Tundra prototype and source role | Evidence |
| --- | --- | --- |
| Active Ship Yards quest | Consumes local iron rations object 13723 as one of Cairme's inputs | [QST input](../../../areas/qst/shipy.qst#L736), [story mapping](../../../areas/story/shipy.story.json#L943), [rations](../../../areas/obj/tundra.obj#L324) |
| Active Alatorin reset and shop | Gives and stocks local iron rations object 13723 | [reset](../../../areas/zon/alatorin.zon#L2210), [shop stock](../../../areas/shp/alatorin.shp#L98), [rations](../../../areas/obj/tundra.obj#L324) |
| Active Surface Keeps reset | Places three iron rations 13723 and one waterskin 13724 inside backpack 120033 | [first ration reset](../../../areas/zon/surfacekeeps.zon#L147), [third ration reset](../../../areas/zon/surfacekeeps.zon#L149), [waterskin reset](../../../areas/zon/surfacekeeps.zon#L152), [rations](../../../areas/obj/tundra.obj#L324), [waterskin](../../../areas/obj/tundra.obj#L332) |
| Compiled room assignment | Binds room 13714 to the inn procedure; this is a room reference, not a mobile-prototype reference | [assignment](../../../src/specs/specs.assign.c#L2459) |

The exact active-source scan found no other direct Tundra mobile or object
prototype links in native QST, ZON, SHP, story-sidecar, or C/H sources. Numeric
fields in unrelated MOB/OBJ records were excluded where they are not typed
prototype references.

Two additional source references are in current but unregistered components;
they are not active behavior:

| Unregistered source | Tundra prototype and source role | Evidence |
| --- | --- | --- |
| `surf.zon` | Six `M` resets reference local snow buffalo mobile 13714, and two reference caribou mobile 13718 | [first buffalo reset](../../../areas/zon/surf.zon#L140), [last buffalo reset](../../../areas/zon/surf.zon#L145), [first caribou reset](../../../areas/zon/surf.zon#L146), [last caribou reset](../../../areas/zon/surf.zon#L147), [buffalo](../../../areas/mob/tundra.mob#L214), [caribou](../../../areas/mob/tundra.mob#L277) |
| `player_guilds.shp` | Four source-only shop stock lists reference local iron rations object 13723 | [first](../../../areas/shp/player_guilds.shp#L240), [second](../../../areas/shp/player_guilds.shp#L363), [third](../../../areas/shp/player_guilds.shp#L442), [fourth](../../../areas/shp/player_guilds.shp#L746), [rations](../../../areas/obj/tundra.obj#L324) |

## Active title and tracked history boundary

The normalized active title “Tundra” has one owner: area `tundra`, zone 137.
The tracked WLD/ZON paths show edits to these current files and name no
distinct predecessor or replacement. Checkpoint 760 adds a bounded comparison
of current cold-area titles and source references that use “Tundra” regionally;
that comparison finds no alternate Tundra map owner or replacement. Runtime
command-target alias ownership remains outside the static title/source scan.

[Active title](../../../areas/zon/tundra.zon#L2) ·
[active-area entry](../../../areas/AREA#L77) ·
[duplicate/replacement register](DUPLICATE_AND_REPLACED_ZONES.md)

## Checkpoint 760: regional wording and adjacent cold-area names

The normalized scan of all 442 current `.zon` titles still finds exactly one
“Tundra” title: `tundra`, zone 137. Nearby cold-themed titles belong to
separate contributors, including Towering Glacier of Dayedenvale, IceCrag
Castle, Neverwinter Woods, Valley of the Snow Ogres, the Royal Mausoleum of
Castle IceCrag, and the City of Winterhaven ([Glacier](../../../areas/zon/glacier.zon#L2),
[IceCrag](../../../areas/zon/icecrag.zon#L2), [Neverwinter Woods](../../../areas/zon/moria.zon#L2),
[Snow Ogres](../../../areas/zon/snogres.zon#L2), [Royal Mausoleum](../../../areas/zon/Voluntown.zon#L2),
[Winterhaven](../../../areas/zon/wh.zon#L2)). These titles describe other
areas and do not identify a second Tundra map.

Surface's active WLD contains 1,065 room descriptions using “Snow Capped Hills
of the Tundra”; for example, [Surface room text](../../../areas/wld/surface.wld#L392401).
Surface QST/story source names Grey Fang and describes a Tundra-hunter paw and
fang exchange ([QST](../../../areas/qst/surface.qst#L595),
[story mapping](../../../areas/story/surface.story.json#L219)). The Surface
reset places Grey Fang mobile #500095 in Surface room #543147
([reset](../../../areas/zon/surface.zon#L378),
[mobile](../../../areas/mob/surface.mob#L1404)). This is Surface map geography
and story content, distinct from the [Tundra WLD](../../../areas/wld/tundra.wld#L2)
and its 175 active rooms.

Black Pearl QST text calls the destination the frozen continent of the Tundra
and locates Verspin there ([QST](../../../areas/qst/blackpearl.qst#L103)). Its
story sidecar says Derimous is intended for Verspin, while the current Black
Pearl reset still places Derimous mobile #142226 in Black Pearl room #142200
([story lead](../../../areas/story/blackpearl.story.json#L9),
[current reset](../../../areas/zon/blackpearl.zon#L343)). Verspin is its own
rostered area, zones 281–282 ([area entry](../../../areas/AREA#L137),
[title](../../../areas/zon/verspin.zon#L2)); Black Pearl is separately
rostered at zone 1422 ([area entry](../../../areas/AREA#L402)). These are a
regional description and an intended cross-area story destination, not a
second Tundra map. The same frozen-continent sentence also occurs in the
unlisted [fooquest](../../../areas/qst/fooquest.qst#L103) and
[raxquest](../../../areas/qst/raxquest.qst#L103) files; their different file
hashes do not support classifying them as exact copies.

The existing typed-reference findings and the seven `surf.wld` source-only
inbounds remain as previously recorded. Neither regional wording nor these
prototype/exit references changes map ownership or the existing candidate
dispositions: retain Surf row 97 and Player Guilds row 84 in place pending
their separate owner decisions. This checkpoint completes the bounded
semantic/source-name comparison; gameplay and runtime name resolution remain
outside its evidence.

## Audit boundary

These are static source findings. The `surf` and `player_guilds` references are
in unregistered source groups, and active reset/shop declarations do not prove
that an item is generated, sold, carried, accepted, or received. Boundary
records and the Ship Yards consumer do not qualify player access, custody, or
completion. Gameplay, editor/tooling, persisted-player, and indirect or
dynamic consumers remain outside this bounded closure.

## Checkpoint 405: explicit builder questions

Physical coverage row 318 now has exactly 20 direct-source builder questions
with conservative recommendations in the [Tundra dossier](../zone-stories/TUNDRA.md#explicit-builder-questions-and-recommended-resolutions).
They distinguish the six story outcomes from the paid armor service and cover
exact material bundles, optional supplied inputs, Eleadora's retirement,
fishing eligibility, shared access, route gaps, moving actors, foreign item
ownership, unsupported paid purchases, and seven inactive-WLD inbounds. Existing
boundary, dependency, and title/history limits remain in force. No area or
gameplay source changed.
## Checkpoint 860: current boundaries and typed closure carried forward

CP860 carries the existing Tundra source-boundary and typed-reference findings forward against the unchanged pinned area/code snapshot. The active WLD census remains 175 rooms and 401 authored exits, with 399 loader-resolved exits, eight reciprocal boundary pairs, one one-way Heavens staff-room inbound, and the unusable `-1` and inactive Duris3 destinations described above. The seven `surf.wld` approaches still target active Tundra rooms only in unregistered source; they do not create active ingress. They are already represented in the current source-only aggregate of 11,993 records overall / 415 from Surf; the 9,480 / 298 aggregate recorded at Checkpoint 405 is historical.

The bounded typed closure remains unchanged: Ship Yards consumes local iron rations #13723; Alatorin resets and stocks that object; Surface Keeps resets rations #13723 and waterskin #13724 inside backpack #120033; compiled code binds room #13714 to an inn procedure; unregistered `surf.zon` references local mobiles #13714/#13718; and unregistered `player_guilds.shp` stocks #13723 four times. The exact active-source scan found no additional direct Tundra mobile/object prototype links. Keep the seven Surf approaches on existing attic candidate row 97 and the Player Guilds shop references on row 84. The Checkpoint 760 title/history result remains bounded to static titles, source names, and tracked WLD/ZON history.

Keep active Tundra **DO NOT MOVE**. This carry-forward grants no candidate, merge, activation, or migration clearance. Actual access, dynamic consumers, persisted-player state, and runtime name resolution remain outside this static review.
