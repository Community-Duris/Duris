# Githzerai Stronghold: repository-wide source audit supplement

This supplement complements the [Githzerai Stronghold
dossier](../zone-stories/GITHZERAI_STRONGHOLD.md). It reconciles the active WLD boundaries, all
unregistered WLD inputs, external typed references, shared code uses, title, and tracked source
history. These findings come from repository source and static registers; they do not establish
live access, stock availability, or player history.

## Active boundaries and local graph

The active area has 381 rooms and 919 authored exits; all 919 targets resolve. Its local
weak-exit graph has five components, with 194 rooms in the largest. Four active exits enter
Githzerai: Realm room #14210 enters rooms #44753 north and #44476 east; Surface room #538611
enters #44400 north; and Winterhaven room #55634 enters #44590 down ([Realm
north](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L257), [Realm
east](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L258), [Winterhaven
down](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L802), [Surface
north](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1410)).

The only active exit from Githzerai to another area is room #44400 south to Surface room
#538611. Those two records form the reciprocal Surface pair ([Githzerai
south](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L620), [Surface
north](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1410)). The dossier's local graph counts and these
active boundaries describe authored WLD topology, not door state, transport, or ordinary
traversal.

## Unregistered WLD approaches

The scan parsed all 95 unregistered WLD files: 527,791 source rooms and 2,080,773 exits, with
zero parse issues and no source-room VNUM overlap with Githzerai.

Exactly three source-only exits target local room #44400:

- Unregistered `surf.wld` room #242869 exits north (D0) to #44400 ([source
  room](../../../areas/wld/surf.wld#L899489), [exit](../../../areas/wld/surf.wld#L899493),
  [target](../../../areas/wld/surf.wld#L899496)).

- Unregistered `underdark2011.wld` room #255552 exits north (D0) to #44400 ([source
  room](../../../areas/wld/underdark2011.wld#L216028),
  [exit](../../../areas/wld/underdark2011.wld#L216032),
  [target](../../../areas/wld/underdark2011.wld#L216035)).

- Unregistered `underworld2.wld` room #421439 exits east (D1) to #44400 ([source
  room](../../../areas/wld/underworld2.wld#L36660),
  [exit](../../../areas/wld/underworld2.wld#L36668),
  [target](../../../areas/wld/underworld2.wld#L36671)).

These records are outside the current active WLD manifest and are not active ingress. They raise
the curated source-only total from 596 to 599 and the reviewed `surf.wld` subset from 33 to 34.

## External typed and code references

Four active Alatorin QST recipes reference Githzerai objects: one accepts adamantite coins
#44509; three others accept an adamantite shortsword #44404, an adamantite claymore #44407, and
a glowing adamantite broadsword #44410 ([coin recipe](../../../areas/qst/alatorin.qst#L3434),
[shortsword](../../../areas/qst/alatorin.qst#L5307),
[claymore](../../../areas/qst/alatorin.qst#L5312),
[broadsword](../../../areas/qst/alatorin.qst#L5317)). The sidecar stores the matching item_vnums
fields at [coin](../../../areas/story/alatorin.story.json#L1499),
[shortsword](../../../areas/story/alatorin.story.json#L2634),
[claymore](../../../areas/story/alatorin.story.json#L2647), and
[broadsword](../../../areas/story/alatorin.story.json#L2660). All 231 external sidecars parsed
without errors. A broad digit scan also matched 44471 inside the alphanumeric story ID
`exchange-83342-37c60a44471b`; it is not a prototype field.

Active `brass.shp` and unregistered `brass-old-1.shp` each list object #44504, the gold
adamantite lockpick, as shop product ([active stock](../../../areas/shp/brass.shp#L74),
[source-only stock](../../../areas/shp/brass-old-1.shp#L71)). The loader resolves positive shop
production entries with `real_object` and compares them against the stocked object's runtime
index ([load](../../../src/economy/shop.c#L3379), [stock
match](../../../src/economy/shop.c#L1834)); these are object product references.

Typed ZON scanning found repeated object-role reset commands in five sources. `alatorin.zon` has
nine G resets for shortsword #44404 ([first](../../../areas/zon/alatorin.zon#L3406));
`barovia2.zon` has five G resets for dark potion #44464
([first](../../../areas/zon/barovia2.zon#L478)); `ravenloft2.zon` has eight G resets for #44464
([first](../../../areas/zon/ravenloft2.zon#L1499)) and one O reset for blood #44419 in room
#59276 ([reset](../../../areas/zon/ravenloft2.zon#L1013)). Active `brass.zon` and unregistered
`brass-old-1.zon` each have one G reset for #44504 ([active](../../../areas/zon/brass.zon#L561),
[source-only](../../../areas/zon/brass-old-1.zon#L559)). These command-row counts retain their
source-area ownership and should not be confused with the dossier's count of foreign reset
groups.

The scan found no external MOB or OBJ definition whose prototype VNUM belongs to Githzerai. Of
850 external type-25 portal declarations, one targets a Githzerai room: Ravenloft2 object #59079
points to room #44487 ([portal object](../../../areas/obj/ravenloft2.obj#L1203),
[destination](../../../areas/obj/ravenloft2.obj#L1209)). This is separate from the local
mobile/object namespaces.

Shared Chaos conjurer kit tables reference object #44485, the sapphire eye of a beholder
guardian, in both standard and enhanceable profiles
([standard](../../../src/account/chaos_eq_data.h#L151),
[enhanceable](../../../src/account/chaos_eq_data.h#L400)). The local `lucky_weapon` binding
assigns object #44499, the black-bladed dagger, to its special procedure
([binding](../../../src/specs/specs.assign.c#L1320)). The local binding and shared equipment use
are different code roles.

## Title and tracked history

The normalized title scan found one Githzerai Stronghold title among 351 active area titles. The
active key is `githzer` with scope *444–447; its title appears in the manifest and ZON header
([AREA entry](../../../areas/AREA#L210), [zone title](../../../areas/zon/githzer.zon#L2)). The
duplicate/replacement register has no separate Githzerai predecessor or replacement entry
([register](DUPLICATE_AND_REPLACED_ZONES.md)). Tracked WLD, ZON, MOB, OBJ, and QST history stays
on the same `githzer` paths back to the 2009 area-tree move (commit `642da5400`); no distinct
predecessor or replacement path was identified.

## Audit boundary

This is a static source review. It does not verify live access, dynamic reset state, safe
traversal, accounting-enabled stock, or a played quest journey. No native repair is proposed in
this checkpoint.

## Checkpoint 809 current typed-role refresh

The active `githzer` reset file has 481 typed prototype operands: 237 mobile
operands (207 `M`, 30 `F`, all local) and 244 object operands (243 local plus
active Heavens object #360). It has no unresolved prototype operand. The
current source uses 123 of 124 local mobiles and 164 of 182 local objects as
reset operands; mobile #44488 is not reset-used, and local item objects
#44517/#44522/#44529 remain without a current reset or stock producer. The
imported Heavens #360 stays distinct from local and external prototype roles.

The previously mapped outside consumers remain exact: active Alatorin,
Barovia2, Ravenloft2, and Brass reset records consume local prototypes;
unregistered `brass-old-1` retains its source-only reset and shop reference to
#44504. Four active Alatorin QST item fields have matching typed sidecar item
fields. Active Brass and source-only `brass-old-1` both list local #44504 as
shop stock. No outside SHP keeper, external mobile/object definition, outside
typed story-sidecar role beyond the Alatorin items, or WLD key field targets a
local Githzerai object.

The local object source has 22 type-25 room routes and ten type-29 switches;
the nine-switch count in the Stronghold dossier was one short of its own
listed IDs and is corrected there. Among the 850 external type-25 objects,
only active Ravenloft2 #59079 targets a Githzerai room (#44487); no external
type-29 target was found. Forty local WLD door-key fields use local objects.
Shared Chaos equipment data references local object #44485 twice, and the
local `lucky_weapon` assignment binds object #44499. No other direct C++ room
or mobile numeric binding in the Githzerai VNUM ranges was found. The typed
role split also preserves mobile #44488 as distinct from object #44488's
portal role.

The active graph remains 919/919 loader-resolved exits with four inbound and
one reciprocal outbound boundary. The three source-only WLD approaches and
the separate active item portal do not change those WLD totals. No source
area, registration, or candidate disposition changes; runtime availability,
access, and persisted ownership remain unqualified.
