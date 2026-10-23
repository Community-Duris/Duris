# The Sky City of Ultarium typed-dependency audit

Checkpoint 782 closes the static typed-reference review for active `cosmic`.
The existing [Ultarium source dossier](../zone-stories/ULTARIUM.md) remains the
story and owner-question record; this supplement changes no gameplay, source
area data, or zone ownership.

## Current source inventory

`cosmic` is an active full-area contributor with 268 rooms (#76001–#76268),
82 mobiles (#76001–#76245), 71 objects (#76001–#76244), 12 QST giver
prototypes, and 17 native exchange contracts. `cosmic.shp` contains two shops,
#76010 and #76013, with six listed products, all local Cosmic objects. The
coverage register's previous zero-shop count was incorrect; it now records
shop range #76010–#76013 and two records. No same-type VNUM overlap with
another active source was found.

The ZON has 338 commands: 154 M, 64 D, 56 O, 31 E, 28 G, four F, and one P.
The 270 typed mobile/object reset commands resolve locally except for eleven
object commands importing ten active Heavens prototypes. There are no
unresolved reset targets.

## Outbound active dependencies

Eleven Cosmic ZON rows import Heavens objects: ground resets of #72 (magical
fountain), #359 (rune-covered stone), and #193 (broken earth), plus G commands
for #77, #73, #500, #412, #221, #220 (two rows), and #223. The exact source
rows are [cosmic.zon lines 307–320](../../../areas/zon/cosmic.zon#L307) and
[lines 365–368 and 512–520](../../../areas/zon/cosmic.zon#L365). Preserve
Heavens as the current prototype owner; the repeated reset rows do not prove
runtime admission or stock availability.

Cosmic QST has eight cross-area item references across six active owners:

- Harpyht plans #31115 and tail #31117 ([QST lines 97 and 146](../../../areas/qst/cosmic.qst#L97));
- Divine Home falcon skull #40731 and shade tiger claw #40733 ([lines 136 and 141](../../../areas/qst/cosmic.qst#L136));
- Myrabolus su-monster pelt #82561 ([line 150](../../../areas/qst/cosmic.qst#L150));
- Hunt heart of Abbadon #13366 ([line 155](../../../areas/qst/cosmic.qst#L155));
- Jade hydra scale #76631 ([line 161](../../../areas/qst/cosmic.qst#L161)); and
- White Harbor memory #55174, a QST reward ([line 64](../../../areas/qst/cosmic.qst#L64)).

Each prototype resolves to its current active owner. The local
[story sidecar](../../../areas/story/cosmic.story.json) has typed item and
completion records for these exchanges. The two
[shop records](../../../areas/shp/cosmic.shp) list only local products
(#76018, #76019, #76033, #76034, #76011, and #76028); no active or candidate
shop product references Cosmic objects.

## Active and candidate inbound dependencies

Six active ZON object-reset rows import Cosmic objects: Vehicles has rows for
#76050, #76066, and #76243 at
[vehicles.zon lines 846–848](../../../areas/zon/vehicles.zon#L846); Alatorin
has two rows for #76067 and one for #76060 at
[alatorin.zon lines 4080, 4514, and 4518](../../../areas/zon/alatorin.zon#L4080).
Active QST adds three White Harbor item commands for #76050, #76066, and
#76243 ([wh.qst lines 1179–1181](../../../areas/qst/wh.qst#L1179)) and one
Myrabolus item command for #76069
([mira.qst line 23](../../../areas/qst/mira.qst#L23)). Their structured
sidecars repeat the White Harbor item set
([wh.story.json](../../../areas/story/wh.story.json#L903)) and identify Cosmic
giver #76029 and study objects #76068/#76069 in the Myrabolus delivery history
([mira.story.json](../../../areas/story/mira.story.json#L316)). No
inbound SHP product, active or candidate ITEM_TELEPORT destination, or
ITEM_SWITCH value[1] room field into Cosmic was found.

Candidate `brass-old-1` has two source-only reset references to current Cosmic
prototypes: M #76073 and G #76064 at
[brass-old-1.zon lines 673 and 825](../../../areas/zon/brass-old-1.zon#L673).
Keep the existing candidate **RETAIN IN PLACE** disposition and preserve both
references as inactive source dependencies; they do not make the candidate
active or authorize an area move.

## Local item travel, switches, and code bindings

Cosmic defines four type-25 ITEM_TELEPORT objects in
[cosmic.obj](../../../areas/obj/cosmic.obj#L55): #76005 → #76228,
#76017 → #76002, #76022 → #76218, and #76023 → #76217. Its two type-29
ITEM_SWITCH objects use command ID 270 and value[1] local rooms: #76015 →
#76134 and #76020 → #76185. These targets all resolve to local WLD rooms.
A full active/candidate object scan found no external teleport or switch room
field into Cosmic. These object fields are separate from the authored WLD
exit graph.

The bounded numeric scan across current source files found five direct VNUM
bindings, all to local prototypes: Deathium mobile #76008 in
[`epic_skills.c`](../../../src/classes/epic_skills.c#L204), smith mobile
#76010 in [`tradeskill.c`](../../../src/economy/tradeskill.c#L202), rename
mobile #76013 and whirlwind object #76032 in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L191), and pet-shop room
#76241 ([line 2354](../../../src/specs/specs.assign.c#L2354)). No additional
matching VNUM literal was found in that scan.

## Disposition

Keep active Cosmic **DO NOT MOVE**. Preserve all Heavens reset imports,
foreign QST item identities, active consumer references, the candidate-only
Brass Old 1 imports, and the local teleports/switches. The existing 15 owner
questions remain unchanged. Existing candidate row 16 records the two Brass
Old 1 references; no row was added or disposition changed, and WLD exit
totals remain unchanged. Static source closure does not qualify live resets,
inventory, shop stock, travel, or installed behavior.
