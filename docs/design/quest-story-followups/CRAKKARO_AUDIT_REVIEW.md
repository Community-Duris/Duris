# Crakkaros' Liar typed-dependency audit

Checkpoint 784 closes the static typed-reference review for active
`crakkaro`. This supplement does not change the area, native contracts, or
gameplay.

## Current source inventory

Crakkaros' Liar is an active full-area contributor with 372 rooms
(#87001–#87470), 37 mobiles (#87000–#87036), and 68 objects
(#87000–#87082). Its three local QST givers define eleven native contracts;
the local story sidecar has 188 structured VNUM/item/completion references,
all resolving to local prototypes. There is no local SHP file. No same-type
room, mobile, or object VNUM overlap with another active contributor or
candidate source was found.

The ZON contains 501 commands: 191 M, 94 D, 82 E, 64 G, 51 O, 12 F, six P,
and one R. The 394 mobile/object prototype reset commands (M/G/E/O/P) all
resolve locally; there are no unresolved reset targets or outbound typed
dependencies.

## Active and candidate inbound consumers

The only active external reset consumer found is one SurfaceKeeps G command
for local object #87074, the blue potion, at
[surfacekeeps.zon line 561](../../../areas/zon/surfacekeeps.zon#L561).
Preserve Crakkaro as the current prototype owner; the reset declaration does
not prove stock or successful admission.

Candidate `brass-old-1` has one source-only G reset for local object #87011,
the stone collar, at
[brass-old-1.zon line 542](../../../areas/zon/brass-old-1.zon#L542). Keep the
existing candidate **RETAIN IN PLACE** disposition and preserve this inactive
reference. It does not make Brass Old 1 an active consumer or authorize a
move.

No other active or candidate ZON, QST, SHP, or structured story reference
targets a Crakkaro mobile, object, or room. No external ITEM_TELEPORT
destination or ITEM_SWITCH value[1] room field into Crakkaro was found.

## Local switch and compiled references

Object #87062 is a type-29 ITEM_SWITCH. Its value[0] is command ID 346,
value[1] targets local room #87418, value[2] is direction 5 (down), and
value[3] is 1. It is not a portal or an external room edge. Crakkaro defines
no local type-25 ITEM_TELEPORT destination.

The bounded numeric scan found no compiled C/C++ VNUM literal matching
Crakkaro rooms, mobiles, or objects, and the source index has no literal
local special assignments. All non-local source references in this closure
are accounted for above.

## Disposition

Keep active Crakkaros' Liar **DO NOT MOVE**. Preserve the active SurfaceKeeps
reset and the Brass Old 1 candidate-only reset with their exact source
identities. Existing 16 owner questions remain unchanged. No candidate row was added or disposition changed; existing candidate row 16 now records the source-only reset. WLD exit totals are unchanged. Static source closure does not qualify current stock,
reset admission, item custody, or installed behavior.
