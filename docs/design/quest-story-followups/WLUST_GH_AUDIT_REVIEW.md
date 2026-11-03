# Wanderlust Guild Hall source audit

Canonical key: `component:wlust_gh`. Coverage data row 471 / CSV line 472;
candidate dependency row 120 / CSV line 121. Reviewed October 12, 2026
(Checkpoint 558) against source snapshot
`72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a static source,
graph, identity, followed-history, and typed-reference review. It does not
establish live loading, player access, reset outcome, editor authority,
deployment state, or persisted identity.

## Source package and authored story

The unregistered package contains [wlust_gh.wld](../../../areas/wld/wlust_gh.wld#L1),
[wlust_gh.mob](../../../areas/mob/wlust_gh.mob#L1),
the tracked but zero-byte [wlust_gh.obj](../../../areas/obj/wlust_gh.obj),
[wlust_gh.zon](../../../areas/zon/wlust_gh.zon#L1), and
[wlust_gh.shp](../../../areas/shp/wlust_gh.shp#L1). The WLD defines ten
rooms (#29100–#29109); the MOB defines two records, an Archangel and a
Titan. The ZON header is #291, its title is “Wanderlust Guild Hall,” its
last room is #29109, and its reset mode is 1. There is no same-stem QST,
story sidecar, or editor map JSON. The source is absent from [areas/AREA](../../../areas/AREA#L140)
and [areas/qst/AREA](../../../areas/qst/AREA); [areas/RANDOM_AREA](../../../areas/RANDOM_AREA)
is empty.

The rooms present a ten-scene guildhall or grotto. Room #29100 stands before
angel and demon pillars and an archway, with an ominous presence; room #29101
is a marble Grand Entry Hall decorated with opposing-force battle tapestries,
statues, trophies, a rose window, and a guild crest. Room #29102 is a narrow
stair, and #29103 is the Forge of Judgment, with a cauldron of molten iron and
an anvil where Wanderlust weapons and armor are made. Room #29104 is a map
table and planning room; #29105 is a shimmering “Gateway to Reality” whose
rim bears the names of the dead. Room #29106 is a stone passageway, #29107 a
barren wasteland where wounds seem less grievous, and #29108–#29109 are
colored-mist spaces. The source describes a setting and atmosphere; it does
not supply a formal quest or prove that the forge, gateway, injuries, or
colors implement special mechanics.

The Archangel is described as watching passersby; the Titan is molding weapons
at the forge ([mobile records](../../../areas/mob/wlust_gh.mob#L1)). The ZON
resets the Archangel four times in #29100 and the Titan once in #29103. The
shop lists fifteen stock VNUMs, but the empty local OBJ file supplies none.
These sources do not establish a quest contract, live shop, or spawn outcome.

## Candidate graph, active identities, and route boundaries

The ten-room source graph has 19 directions: 18 numeric-local targets and one
D4 from #29100 to #154553. Its local numeric graph is one weak component and
one strongly connected component of ten rooms; #29100 reaches all ten rooms
both forward and in reverse. Room exits are:

| Room | Authored exits |
| --- | --- |
| #29100 | D0 → #29101; D4 → #154553 |
| #29101 | D0 → #29104; D1 → #29106; D2 → #29100; D3 → #29102 |
| #29102 | D1 → #29101; D4 → #29103 |
| #29103 | D5 → #29102 |
| #29104 | D2 → #29101; D4 → #29105 |
| #29105 | D5 → #29104 |
| #29106 | D0 → #29108; D2 → #29107; D3 → #29101; D5 → #29109 |
| #29107 | D0 → #29106 |
| #29108 | D2 → #29106 |
| #29109 | D4 → #29106 |

All 18 local target numbers also have current active rooms in [torg.wld](../../../areas/wld/torg.wld#L4324)
because the full manifest assigns Torg the *289–291 range. If the
unregistered Wlust file is not loaded, these numbers resolve to Torg records,
not to the Wlust prose. A full-block comparison confirms that Wlust’s ten
same-number rooms differ from Torg’s. Torg itself has 13 authored WLD exits
whose targets are #29100–#29109; they target Torg’s own active identities.
Thus the active candidate-specific inbound count is zero.

The remaining D4 exit leads from Wlust #29100 to #154553 in the unregistered
[Duris3 WLD](../../../areas/wld/Duris3.wld#L1145614), titled “Grassy
Plains.” That source room’s four exits lead to Duris3 #154253, #154554,
#154853, and #154552; none returns to #29100–#29109. The same Duris3 target
is named by `void_gh` #29100 D4, another source-only declaration. Neither
route reaches an active WLD identity, and this checkpoint adds no approach
to the curated 11,993 overall / 415 from Surf source-only totals. There are
no negative targets, out-of-range source rooms, range-mismatch exits, fall
chances, or current fields in Wlust’s ten-room source.

Every Wlust room number #29100–#29109 also occurs in active Torg and in the
unregistered `afterlife_gh`, `scrubs_hall`, `solstice_gh`, and `void_gh`
WLD sources. These are different source records sharing a numeric range;
they do not show that one guildhall replaces another.

## Typed dependencies and shared prototype numbers

The candidate ZON contains three O reset imports from Player Castles:
object #11053 (a magic mouth) in #29100, #11103 (a bulletin board) in #29101,
and #11013 (an altar of bonding) in #29105. They are not prototypes in
`wlust_gh.obj` and are not active resets from this absent candidate ZON.
Preserve the imports with their exact rooms and reset limits
([ZON lines 9–11](../../../areas/zon/wlust_gh.zon#L9);
[magic mouth](../../../areas/obj/player_castles.obj#L145),
[bulletin board](../../../areas/obj/player_castles.obj#L265), and
[bonding altar](../../../areas/obj/player_castles.obj#L121)).

Five M records reset mobile #29100 four times in #29100 and mobile #29101
once in #29103. Wlust mobiles #29100/#29101 have different records from
same-number prototypes in `afterlife_gh`, `scrubs_hall`, `solstice_gh`, and
`void_gh`. No active Torg MOB owns either number. The sibling ZON resets are
their own source declarations, not Wlust activity: `afterlife_gh.zon` resets
#29100 and #29101; `afterlife_op.zon` and `afterlife_op2.zon` reset #29101;
`scrubs_hall.zon` resets #29100; and `solstice_gh.zon` and `void_gh.zon`
reset both #29100 and #29101. None of these candidate ZON files is in the
active AREA list. The one current Torg ZON record mentioning room #29101
uses mobile #28911; it is not a reset of Wlust’s Titan.

There are fifteen G reset VNUMs, repeated in the SHP stock list:
#108, #3050, #398, #6089, #364, #53, #412, #347, #109, #111, #377, and
#491–#494. Fourteen have a prototype in the current full object sources:
#3050 is in active `verz1`; #108, #109, #111, #347, #364, #377, #398, #412,
and #491–#494 are in active `heavens`; #53 is also in active `heavens`, with
additional definitions in `dalvik`, `hostel`, and `mavulsk`; #108 and #109
also have unregistered `mavulsk` definitions. No checked-in OBJ prototype
for #6089 was found. Wlust defines none of these locally. The September 6,
2026 source change added Heavens gathering bags #491–#494 to this ZON and
shop inventory; that edit is source maintenance, not evidence that Wlust is
in the full manifest.

Six type-25 object declarations in other unregistered guildhall sources have
value targets inside #29100–#29109: `afterlife_op.obj` #29110 targets #29106;
`afterlife_op2.obj` #29122, #29123, and #29125 target #29109; and
`void_gh.obj` #29101 and #29102 target #29108 and #29107. They are
source-only declarations and do not identify Wlust-specific destinations.
Under current full-world numeric lookup, the target numbers belong to Torg.
No active type-25 target into #29100–#29109 was found.

The only checked-in QST giver at #29100–#29101 is #29101 in unregistered
`afterlife_gh.qst` ([giver](../../../areas/qst/afterlife_gh.qst#L1)); it is
not a Wlust quest. No same-stem Wlust QST exists. No active QST, MOB reset,
or SHP record uses Wlust’s #29100/#29101 mobile identities.

Wlust’s single SHP record is keyed by keeper #29101, sells the same fifteen
items, and sets its shop room to #29103. `areas/world.shp` is absent in this
checkout. In full mode, [open_shop_stream()](../../../src/economy/shop.c#L3267)
falls back to concatenating `areas/shp/*.shp`, which includes both
`wlust_gh.shp` and `void_gh.shp`; both declare keeper #29101. Shop loading
converts that keeper through `real_mobile()`, and shopkeeper assignment later
indexes the resulting mobile. The current active MOB sources have no #29101
prototype. Current room lookup maps shop room #29103 to Torg’s escape tunnel
([record](../../../areas/wld/torg.wld#L4373)), while Wlust calls it the forge.
Compiled code also binds the `inn` procedure to raw room #29103
([assignment](../../../src/specs/specs.assign.c#L2448)); that binding
resolves to Torg in the current manifest. This is a conditional static loader
and identity risk, not a boot result; no shop or game runtime was tested.

The listed area source has no `wlust_gh` editor JSON, QST, or story sidecar.
Editor exports, deployment inputs, live runtime state, and persisted
references remain outside this repository review.

## Title, followed history, and replacement assessment

After normalization, “Wanderlust Guild Hall” is unique among 442 current
`.zon` titles. Zone header #291 is shared only with `scrubs_hall` (whose raw
title is “Ye' Old ScRubS Pirate Galleon”), so the header number does not
identify a shared map. No canonical alias or title crosswalk ties Wlust to
Torg or a sibling guildhall.

The followed WLD history stays on the `wlust_gh` path: the 2011
`friggin typos` commit corrected “cielings” to “ceilings,” while the 2013
`stuff` commit changed it back and changed file mode. The MOB and empty OBJ
paths show the 2009 area-tree move; the empty OBJ has no prototype content.
ZON and SHP also show a 2013 mode-only change. The September 6, 2026
`ed94862f0` commit added Heavens bag references #491–#494 to both ZON and SHP.
This is later same-stem maintenance, not proof of activation, retirement, or
replacement. No followed alternate-stem rename, explicit predecessor or
successor, retirement, or whole-zone replacement was found.

## Disposition

Keep all five source files **RETAIN IN PLACE** outside `areas/AREA` and
`areas/qst/AREA`. Preserve the ten-room prose and exits, both mobile records,
all three Player Castles imports, five M reset records, fifteen inventory
references, and the shop declaration. Do not activate, merge, renumber,
regenerate, move, or archive them until room/mobile ownership, the
Duris3 boundary and return intent, shopkeeper and stock ownership, the
#29103 code binding, editor/export and deployment inputs, runtime behavior,
and persisted identity are resolved. Twenty-eight source-backed owner
questions and recommended resolutions are in
[WLUST_GH_BUILDER_QUESTIONS.md](WLUST_GH_BUILDER_QUESTIONS.md).
