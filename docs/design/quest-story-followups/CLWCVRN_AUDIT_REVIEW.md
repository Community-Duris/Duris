# Clawed Caverns typed-dependency audit

Checkpoint 781 closes the static typed-reference review for active
`clwcvrn`. This is a source review only; it does not change gameplay, area
ownership, or the separate installed-gameplay qualification recorded in the
[story dossier](../zone-stories/THE_CLAWED_CAVERNS.md).

## Current source inventory and ownership

`clwcvrn` is listed as an active full-area contributor and in `qst/AREA`.
Its current source defines 89 rooms (#80700–#80788), 40 mobiles
(#80700–#80739), and 55 objects (#80700–#80754), with eight QST giver
records and 20 contracts. There is no local SHP file. These local VNUM sets
have no same-type collisions with another active contributor.

The ZON contains 151 typed reset commands: 67 M, 46 O, 19 G, 13 E, 3 P,
and 3 F. The other 20 authored commands are D commands. All local mobile,
object, room, and door targets resolve to the current Clawed Caverns source,
except for the explicitly foreign object import below; no typed reset target
is unresolved.

## Outbound and inbound references

The 15 consecutive G commands at
[clwcvrn.zon lines 140–154](../../../areas/zon/clwcvrn.zon#L140) request
Heavens object #827 for the preceding guardian mobile #80739. The source
comments note that the object is not defined in `clwcvrn.obj`. Current active
ownership resolves #827 to Heavens object “strange stone”; keep Heavens as
the prototype owner and preserve these 15 source commands. They are repeated
give-object declarations, not active issuance or proof that all 15 can be
created together in a running zone.

The [Heavens object](../../../areas/obj/heavens.obj#L7775) describes a
strange stone/strange stones. No other external active ZON reset, QST item or
giver reference, shop product, story-sidecar VNUM, or unresolved typed
reference into Clawed Caverns was found. No candidate source adds such a
typed reference. In particular, the existing `ud_map2.wld` north exit from
#1124947 to #80700 remains a source-only WLD approach, not active ingress or
a typed dependency; retain it with the existing `ud_map2` candidate review.

## Local typed fields and code bindings

The local [hot crystal #80747](../../../areas/obj/clwcvrn.obj#L694) is an
`ITEM_SWITCH` (type 29): value[0] is push command ID 270, value[1] is local
affected room #80773, value[2] is west, and value[3] is 1. It is not a
teleport destination. The target room is in the local WLD. Clawed Caverns
defines no local `ITEM_TELEPORT` target. A complete active/candidate object
scan found no external `ITEM_TELEPORT` destination or `ITEM_SWITCH` value[1]
room field into Clawed Caverns.

The bounded compiled-source scan finds the local VNUMs in the expected
Clawed Caverns bindings: range-based death-special assignments and local
golem/mage/switch assignments in
[`specs.assign.c`](../../../src/specs/specs.assign.c#L313), the movement
special's room #80700 check and object behavior in
[`specs.clwcvrn.c`](../../../src/specs/specs.clwcvrn.c#L159), and the four
named item constants in [`vnum.obj.h`](../../../src/world/vnum.obj.h#L133).
These are compiled consumers of local prototypes, not additional cross-area
dependencies.

## QST, shops, and structured story data

All eight local QST givers are #80706–#80710, #80724, and #80726–#80727.
The 38 `G`/`R` item commands all target local objects. Active and candidate
QST sources contain no inbound item/giver reference or custom-QC coordinate
that resolves into Clawed Caverns. There are no local shops and no active or
candidate shop product references into its object VNUMs.

The story sidecar has 27 `giver_vnum`, 10 `mob_vnum`, and 8 `item_vnums`
references; each resolves locally. No active or candidate story sidecar
references a Clawed Caverns prototype. The existing 12 owner questions remain
unchanged: the resolved Heavens import does not create an additional story or
ownership ambiguity.

## Disposition

Keep active `clwcvrn` **DO NOT MOVE** and preserve its 15 Heavens #827 G
commands, local switch behavior, and separate source-only `ud_map2` approach.
No candidate row or migration-plan action changes. No WLD exit totals
change. Runtime reset admission, item availability, actual reachability, and
installed behavior remain outside this static closure.
