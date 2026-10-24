# Temple of the Earth: repository-wide source audit supplement

This checkpoint supplements the [Temple of the Earth dossier](../zone-stories/TEMPLE_OF_THE_EARTH.md). It reconciles active WLD boundaries, all unregistered WLD inputs, typed external references, shared code uses, the normalized area title, and tracked path history. These are static source findings; they do not establish live access or player history.

## Active boundaries and local exits

The active Earth map has 635/636 resolving exits across 198 rooms and two weak
components; its largest component contains 197 rooms. The active Underdark pair
is reciprocal: Earth room #43500 exits west to Underdark #804038, whose east
exit returns to #43500 ([Earth exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L616),
[Underdark return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1720)).

A separate active exit enters Earth from Realm: room #14209 exits down to Earth
#43568, and the cross-area register records no reverse-direction WLD exit
([Realm approach](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L255)). Earth room #43571
also retains its east exit with destination #-1; setup drops this negative
target, as recorded in the unresolved-exit register
([unresolved record](WORLD_UNRESOLVED_EXIT_REGISTER.csv#L215)).

## Unregistered WLD approaches

The scan parsed all 95 unregistered WLD files: 527,791 rooms and 2,080,773
exits, with zero parse issues and no source-room VNUM overlap with Earth.
Exactly two source-only exits target Earth room #43500:

- Unregistered `surf.wld` room #229658 exits south (`D2`) to #43500
  ([source room](../../../areas/wld/surf.wld#L622146),
  [exit](../../../areas/wld/surf.wld#L622158),
  [target](../../../areas/wld/surf.wld#L622161)).
- Unregistered `underworld2.wld` room #437928 exits south (`D2`) to #43500
  ([source room](../../../areas/wld/underworld2.wld#L157522),
  [exit](../../../areas/wld/underworld2.wld#L157534),
  [target](../../../areas/wld/underworld2.wld#L157537)).

No other unregistered WLD exit targets an Earth room. These routes raise the
curated source-only total from 594 to 596 and the reviewed `surf.wld` subset
from 32 to 33. They remain source records, not active-world ingress.

## External typed and code references

No external native QST or SHP line matched an Earth room, mobile, or object
VNUM. All 231 external story sidecars parsed without error and had no exact
structured numeric reference to an Earth prototype. No external MOB/OBJ record
defines one of Earth's local prototypes. The scan of 866 external type-25
portal declarations found no destination in an Earth room.

Typed reset-role scanning found five active `M` reset commands in `brass.zon`
that use Earth mobile #43540, the magma elemental ([active entries](../../../areas/zon/brass.zon#L765)).
Five additional `M` commands in unregistered `brass-old-1.zon` use the same
mobile in source-only reset data ([unregistered entries](../../../areas/zon/brass-old-1.zon#L761)).
This is a mobile reference; the same number also names a separate Earth object
prototype.

The shared Chaos conjurer equipment tables use Earth object #43563, the stone
ring of elemental might, in both standard and enhanceable kits
([standard kit](../../../src/account/chaos_eq_data.h#L165),
[enhanceable kit](../../../src/account/chaos_eq_data.h#L414)). The global
high-drop mobile roster includes Earth mobile #43576, Eligoth ([roster entry](../../../src/item/randomeq.c#L142));
the roster checks the current mobile VNUM while processing high-drop gear
([check](../../../src/item/randomeq.c#L650)). Earth's local bindings attach
Eligoth's death procedure to #43576 and the chamber switch procedure to object
#43584 ([bindings](../../../src/specs/specs.assign.c#L2188)). The local death
object constant #43580, switch room #43697, and switch mobile list are in
[specs.earth.c](../../../src/specs/specs.earth.c#L15). These code references are
documented separately from QST goals and source-only resets.

## Title and tracked history

The normalized title scan found one “Temple of the Earth” among 351 active area
titles. The active key is `earth`, scope `*435-437`, and the matching title
appears in the ZON ([AREA entry](../../../areas/AREA#L207),
[zone title](../../../areas/zon/earth.zon#L2)). The duplicate/replacement
register has no separate retired or replacement entry for this area
([register](DUPLICATE_AND_REPLACED_ZONES.md)). Tracked WLD/ZON/MOB/OBJ/QST
history remains on the same `earth` paths back to the 2009 area-tree move
(`642da5400`), with no distinct predecessor or path replacement identified.

## Audit boundary

This review used repository source and static registers only. No tests, build,
server/runtime, gameplay, database, migration, or operational script was run.
The source-comprehensive queue is now **101/220 complete, 119 pending**;
priority 102, Githzerai Stronghold (`githzer`), is next. Source mapping remains
separate from played source custody, reset issuance, room access, and
return-safety qualification.

## Checkpoint 792: current typed census refresh

### Local resets, quests, and story sidecar

The active earth.zon source has 278 prototype-reset commands: 165 M, 51 O,
43 E, 12 G, and seven P. Each P command names both a child and a container,
so the commands carry 285 typed prototype VNUM operands. Of these, 282
resolve to local Earth mobiles or objects. G rows 502 and 503 import Heavens
object #360 and Plane of Earth object #23805, respectively
([Heavens import](../../../areas/zon/earth.zon#L502),
[Plane of Earth import](../../../areas/zon/earth.zon#L503)). The corresponding
object definitions remain owned by [Heavens](../../../areas/obj/heavens.obj#L3741)
and [Plane of Earth](../../../areas/obj/plane_earth_one.obj#L56).

O row 248 targets object #43595, for which no Earth OBJ prototype exists
([reset](../../../areas/zon/earth.zon#L248),
[object source](../../../areas/obj/earth.obj)). The same number identifies an
Earth mobile, room, and QST giver, but those are separate typed namespaces
([mobile](../../../areas/mob/earth.mob#L1378),
[room](../../../areas/wld/earth.wld#L2418),
[QST header](../../../areas/qst/earth.qst#L198)). The object reset remains an
unresolved source reference; the other definitions do not fill its OBJ slot.

The local QST has four R/G item fields, #43561, #43525, #43562, and #43539;
all resolve locally ([QST](../../../areas/qst/earth.qst#L32)). The current
sidecar has 18 typed values: 12 mobile, four giver, and two item values, all
local ([story sidecar](../../../areas/story/earth.story.json#L23)). All 231
external story sidecars parsed without error and none references an Earth
mobile or object. No external native QST item or giver header, SHP product,
or external MOB/OBJ definition targets an Earth prototype. Earth has no local
shop.

### Inbound reset references and compiled bindings

Active Brass has five M reset rows for Earth mobile #43540
([active resets](../../../areas/zon/brass.zon#L765)). Unregistered
brass-old-1 has five matching source-only M rows
([candidate source](../../../areas/zon/brass-old-1.zon#L761)); keep that
source on existing candidate row 17. Earth also defines object #43540, which
is a distinct object prototype and is not the Brass reset target.

The shared Chaos equipment tables reference Earth object #43563 in standard
and enhanceable kits ([standard](../../../src/account/chaos_eq_data.h#L165),
[enhanceable](../../../src/account/chaos_eq_data.h#L414)). The high-drop
mobile roster includes Earth mobile #43576, Eligoth
([roster](../../../src/item/randomeq.c#L142)); its VNUM check is at
[line 650](../../../src/item/randomeq.c#L650). Local assignments bind
Eligoth's mobile procedure to #43576 and the chamber-switch procedure to
object #43584 ([bindings](../../../src/specs/specs.assign.c#L2188)).
The local rift object, switch room, and mobile list are declared in
[specs.earth.c](../../../src/specs/specs.earth.c#L15).

### Typed item travel

Earth has six type-25 destinations: #43506 to #43594, #43515 to #43652,
#43528 to #43536, #43580 and #43582 to #43502, and #43583 to #43611.
Its eleven type-29 room values are #43502 to #43594, #43503 to #43601,
#43509 to #43610, #43530 to #43532, #43534 to #43518, and #43584 through
#43589 to #43550. Every destination resolves to one of the 198 local Earth
rooms. No external type-25 destination or type-29 switch room targets an
Earth room. These item values do not add WLD exits.

### Audit boundary

This checkpoint closes the static typed-source census only. It does not
qualify reset admission, travel behavior, a deployed shop/runtime state,
editor tooling, or persisted player references. No area source, candidate
disposition, gameplay, or WLD total changed; runtime and played qualification
remain unestablished.
