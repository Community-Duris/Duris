# Afterlife GC OP2

Canonical key: `component:afterlife_op2`. Physical coverage row 356. The
source groups three disconnected three-room maps under the shared `.zon`
title “Afterlife GC OP”: an icy cavern, a foggy ring of trees, and a salt
circle leading into the Deep Beyond. Its [technical audit](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md)
records the active Torg room collision, source-only Duris3 exits, imported
reset owners, and seven portal target values. The [builder review](AFTERLIFE_GC_OP2_BUILDER_QUESTIONS.md)
keeps destination and source ownership questions open.

## Three separate passages

The first three rooms describe a cavern beneath frozen tundra. Ice formations
and drifting snow reflect a white glow along a slippery tunnel. Room #29120
mentions four stoic frost golems guarding against trespassers; #29122 contains
a portal framed in ice, with an inscription naming Khomani-Khan. Its D4 exit
from #29120 points to #172709, “The Ocean,” in the unlisted `Duris3.wld`.

The second group moves from a fog-covered ring of sturdy oak trees along a
silent stone path to a glowing portal. The portal frame says it leads to
Khomani-Khan. Its D4 exit from #29123 targets #163943, “Rolling Hills,” also
in `Duris3.wld`. No WLD exit connects this group to the icy cavern or the Deep
Beyond rooms.

The last three rooms are a large circle of salt surrounded by skeletons,
undead, and flying foul creatures. The text says only living beings dare enter
the passage. It leads through a damp, gritty tunnel into a dark room where a
softly glowing mass swirls. This group has no outward WLD exit. The editor map
places the cavern at y=0 and both the forest path and salt passage at y=400;
the forest and salt rooms sit side by side without WLD exits between them.

These descriptions establish three source settings and two authored boundary
exits. They do not establish that either Duris3 target is loaded or reachable.
The zone's reset file places mobile VNUMs #29101–#29104 in each group's first
room, but the OP2 MOB file is empty and those numbers have several separate
candidate definitions. The static reset commands therefore do not prove that
the described frost golems, forest inhabitants, or undead appear in play.

## Room map and source exits

| VNUM | Source title | Direction records |
| --- | --- | --- |
| #29120 | A Massive Cavern of Rock and Ice | D0 → #29121; D4 → #172709 |
| #29121 | A Slippery Walkway Through the Tunnel | D2 → #29120; D3 → #29122 |
| #29122 | A Massive Portal Encrusted in Frost | D1 → #29121 |
| #29123 | A Ring of Trees Enshrouded in Fog | D0 → #29124; D4 → #163943 |
| #29124 | A Stone Path Through the Woods | D2 → #29123; D3 → #29125 |
| #29125 | A Stone Portal Amist the Trees | D1 → #29124 |
| #29126 | A Large Circle of Salt | D0 → #29127 |
| #29127 | Passage Into the Deep | D2 → #29126; D3 → #29128 |
| #29128 | Within the Deep Beyond | D1 → #29127 |

The twelve local direction records form three weak components. Directed
traversal from #29120, #29123, and #29126 reaches three rooms within each
component. The two remaining D4 records target `Duris3.wld` rooms #172709 and
#163943; neither target has a direct exit back to its OP2 source room, and
`Duris3` is absent from the full area list.

## Portals and reset ownership

The seven OBJ prototypes are type-25 teleport objects. Each uses
`value[1]=7` (`CMD_ENTER`); their stored target VNUMs are not proof of the
destination named by their text.

| Object | Described destination | Stored `value[0]` target |
| --- | --- | --- |
| #29120 | Tundra | #29122 |
| #29121 | Goodie Continent | #29120 |
| #29122 | Khomani-Kan | #29109 |
| #29123 | Khomani-Kan | #29109 |
| #29124 | Evermeet | #29125 |
| #29125 | Khomani-Kan | #29109 |
| #29126 | Undead Cont. | #29128 |

The ZON resets the local portal objects #29122 in #29122, #29123 in #29125,
and #29125 in #29128. Six other object resets import Player Castles objects
#11053 and #11103 into the first two rooms of each group. In the reverse
direction, `afterlife_gh.zon` imports OP2 objects #29120, #29124, and #29126
into its own room #29109. No current reset placement was found for object
#29121.

OP2 has no QST or SHP file. Its map asset
`areas/map/afterlife_op2.json` stores editor positions for all nine rooms;
that separate asset and the shared `.zon` title do not make this map a loaded
part of the Guildhall or of `afterlife_op`.

The [technical audit](../quest-story-followups/AFTERLIFE_OP2_AUDIT_REVIEW.md#portal-target-values)
documents the exact prototype and reset sources. Keep the three OP2 maps
distinct from active Torg and from the other same-title Afterlife candidate
until room ownership and all portal targets are resolved.
