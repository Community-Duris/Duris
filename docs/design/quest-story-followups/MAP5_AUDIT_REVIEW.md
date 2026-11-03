# map5 mobile source and typed-dependency audit

Canonical key: component:map5. Physical coverage row 416 / CSV line 417;
candidate dependency row 65 / CSV line 66. Reviewed October 11, 2026,
Checkpoint 503, against source snapshot
c500ac51b8cd26540943257e7ca4e9807263ef74. This is a static MOB,
source-list, reset, prototype-text, code-binding, and bounded-history review.
It does not establish a live reset, mobile-special invocation, or external
editor/deployment copy.

## Inventory and character content

The only tracked same-stem file is [areas/mob/map5.mob](../../../areas/mob/map5.mob).
It defines 40 unique mobile VNUMs between #150000 and #150141, with gaps. It
has no same-stem WLD, OBJ, ZON, QST, SHP, story sidecar, or editor-map file.
Neither [areas/AREA](../../../areas/AREA) nor
[areas/qst/AREA](../../../areas/qst/AREA) lists `map5`. The file has no zone
title or room graph, and its mobile IDs do not collide with definitions in
other tracked MOB files.

The prototypes include faction patrols and soldiers, a dryad, a forest
ranger, an arcane merchant, and named characters Maelron, Hellaranin, and
Graelma. They provide character prose but no local quest, shop, map, or
reward contract. The lack of a companion file does not establish that the
named or code-bound mobiles are disposable.

## Numeric identities, text copies, and code bindings

No other tracked MOB source defines any of map5's 40 VNUMs. Several records
share descriptive content with prototypes under different IDs. Eight
map5 records have an exact full-record copy in unlisted `surf.mob` after the
VNUM header is removed; further map5 text matches occur in that file.
Map5 #150000 and #150001 share their four text fields with active Minizones
#5700 and #5714. Map5 Maelron #150002 and arcane merchant #150003 share the
same four text fields as active Killing Fields #33300 and #33308. These are
different numeric prototypes, not proof of one zone identity or replacement.
Killing Fields QST #33300 remains keyed to its own active Maelron, and its
shop/reset source uses merchant #33308. Do not transfer those contracts to
map5 #150002 or #150003.

Compiled specials directly reference map5 prototypes. The
[assignment table](../../../src/specs/specs.assign.c#L688) binds
`outpost_captain` across #150115–#150140, and the
[Tharnadia helper table](../../../src/specs/specs.tharnadia.c#L838) lists
22 map5 prototypes from that range. Map5 defines 22 of those 26 IDs; #150120,
#150121, #150127, and #150128 are absent from every tracked MOB source.
Separate [patrol assignments](../../../src/specs/specs.assign.c#L1213) bind
map5 #150100 and #150101. These static assignments do not prove the
unlisted prototypes are present in a live mobile table. A raw `150000` in
[`specs.olympus.c`](../../../src/specs/specs.olympus.c#L43) is a room-range
base passed to `real_room0`, not a mobile reference to map5 #150000.

## Source-only Surf placements and current dependencies

The unlisted [surf.zon](../../../areas/zon/surf.zon#L69) contains 45 `M`
commands referencing nine map5 mobile VNUMs. They place those prototypes in
40 distinct room IDs, all defined in the separate unlisted
[surf.wld](../../../areas/wld/surf.wld#L27268). None of the 45 commands is
from a current `areas/AREA` contributor: `surf` is not listed, while the
similarly named `surface` and `surfacemini` entries are separate inputs.
Treat these as source-only reset relationships, not live placements.

A bounded scan found no QST giver or SHP keeper keyed to a map5 VNUM and no
active ZON `M` reset to any map5 prototype. The story-sidecar numeric matches
to #150000 were currency/experience fields, not mobile-giver references.
There is no WLD source to which connectivity metrics apply; leave the
candidate graph fields blank and keep candidate inbound active exits at
zero. Preserve the typed code and unlisted Surf reset findings separately
from WLD route counts.

## History and disposition

Followed history reaches the 2009 area-tree import commit `642da5400`, which
introduced the current 574-line path in the tracked history. No earlier
same-stem source, direct rename, retirement, or replacement decision is
established. Prototype-text reuse across Surf, Minizones, and Killing Fields
does not establish that map5 was replaced by those sources.

**Disposition: RETAIN IN PLACE.** Preserve all 40 mobile records outside the
area lists while the map5, Surf, Tharnadian, Minizones, and Killing Fields
owners reconcile the code bindings, source-only resets, text copies, gaps,
and editor/deployment copies. Keep the 45 `surf.zon` placements source-only;
do not attach Killing Fields QST/shop identities to map5 VNUMs or activate
map5 from the code references alone. See the
[20 builder questions](MAP5_BUILDER_QUESTIONS.md),
[coverage row 416](ZONE_COVERAGE_REGISTER.csv#L417),
[candidate dependency row 65](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L66),
and [Surf dependency row](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97).

## Checkpoint 914: map5 cross-namespace role closure

The exact set of 40 map5 mobile VNUMs has no same-number definition in any
other tracked MOB file. An all-WLD exact-number scan finds those same 40
numbers only in unlisted `Duris3.wld`, where each is defined once as a room
VNUM. These are separate typed identities; they do not create a map5 room,
mobile duplicate, or active map5 route. Map5 still has no WLD and its graph
metrics remain blank.

A typed reset scan found 45 `M` placement commands for nine map5 prototypes
and nine `F` follow commands for map5 #150102, all in unlisted [surf.zon](../../../areas/zon/surf.zon#L69).
[src/world/db.c](../../../src/world/db.c#L7747) defines `F` as following the last mobile loaded by `M`.
`surf` is absent from `areas/AREA`, and no other ZON source names a map5 mobile
in an `M` or `F` role, so these 54 rows remain source-only rather than active
placements. The 45 `M` rows use 40 room IDs in the separate unlisted
`surf.wld` source.

The literal #150000 matches in QST and story data are reward currency or
experience amounts, not mobile givers; structured story mobile/giver fields
have no map5 VNUM. No parser-valid N-format SHP keeper uses a map5 ID. Static
compiled mobile assignments remain the patrol specials for #150100/#150101
and `outpost_captain` for the 26 IDs in [specs.assign.c](../../../src/specs/specs.assign.c#L688) #150115–#150140. Map5 defines 22 of those
26; #150120, #150121, #150127, and #150128 remain absent from tracked MOB
sources. The Tharnadian [`tower_data`](../../../src/specs/specs.tharnadia.c#L838) lists 22 map5 prototypes. The separate
[specs.olympus.c](../../../src/specs/specs.olympus.c#L43) value #150000 is a room base passed to `real_room0`, not a
mobile lookup; it numerically matches the Duris3 room identity above.

No active reset or WLD graph count changes. Preserve the 40 mobile records,
the 45 source-only `M` rows, nine `F` followers, and existing static code
bindings under **RETAIN IN PLACE**. Keep all 20 builder questions and the
map5, Surf, Tharnadia, content, and deployment owner decisions open. No
runtime or live prototype availability was tested.
