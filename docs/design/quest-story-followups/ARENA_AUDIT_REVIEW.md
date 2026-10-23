# Arena repository-wide audit review

This supplement records the authored setting, active WLD graph, compiled
arena routes, typed source dependencies, and bounded title/history review for
active area `arena`. It accompanies the
[Arena source story dossier](../zone-stories/ARENA.md). Findings describe
repository source and compiled configuration; they do not qualify live
availability or successful player travel. No area or gameplay source was
changed.

## Active source and environmental story

At source snapshot `72ba3deff3bb30bde880018cdebe7c6db675b697`, zone 679 is
titled `CTF Tournament` and has 87 rooms (#67900–#67986), zero mobiles, one
object (#67900), and a reset-mode-2 ZON file. The only two local ZON resets
place object #67900 at rooms #67902 and #67904. The object is described as a
large arena portal. The [room source](../../../areas/wld/arena.wld#L1) has no
descriptive prose in any of its 87 records; its environmental story is carried
by titles such as the three squares and treasure vaults, marbled and onyx
hallways, tower balconies, and massive battlefield.

`arena` appears in `areas/qst/AREA` at line 232, but there is no
`areas/qst/arena.qst`, native Q/QA contract, SHP source, or story sidecar. The
MOB source exists but is empty. No local giver, enemy, shop, quest reward, or
authored outcome supplies additional narrative.

There is one active outside placement of the arena object:
`tharnadia.zon` line 518 resets object #67900 in room #132574, The Ogre's Foot
Lounge. Its room prose describes an inn lounge where warriors wait for work.
This is a useful setting clue, but the files do not say that it is a quest
gathering or formal tournament registration point
([reset](../../../areas/zon/tharnadia.zon#L518),
[room](../../../areas/wld/tharnadia.wld#L1939)).

## Active WLD graph and runtime reachability

The source parse found 214 authored exits; all 214 resolve to rooms in
`arena`, and none crosses the active-area boundary. The weak components have
sizes 81, 2, 2, and 2:

| Static component | Rooms | Directed reach and return from representative |
| --- | ---: | ---: |
| Goodie staging pair | 2 (#67904–#67905) | 2/87 from #67904 |
| Evil staging pair | 2 (#67902–#67903) | 2/87 from #67902 |
| Undead staging pair | 2 (#67900–#67901) | 2/87 from #67900 |
| Battle map | 81 (#67906–#67986) | 81/87 from #67906 |

No authored exit carries the scanner's `EX_BLOCKED` bit. All six rooms in the
three staging components carry arena classification, as does the battle map.
The all-95 inactive-WLD sweep parsed without errors, found no overlapping room
VNUMs and no inactive-WLD exits into `arena`; no non-overlapping source-only
WLD approach was found. Thus WLD-only graph traversal cannot explain entry to
the battle map. That separation is bridged by the compiled object callback,
not by a hidden authored exit.

`map_spec` configures #67906–#67986 as the map range, with team deathmatch
spawns at #67907 and #67983 and no Undead spawn. The separate `arena_start_room`
table assigns Goodie to #67904/#67905, Evil to #67902/#67903, and Undead to
#67900/#67901 ([compiled tables](../../../src/combat/arena.c#L55)).
`players_to_map` transfers waiting players from those pairs at match start;
`players_from_map` returns them to each team's first staging room at match end
([transfer functions](../../../src/combat/arena.c#L221),
[match cycle](../../../src/combat/arena.c#L605)). With the compiled default
Deathmatch type, entrants are placed randomly within the map range.

## Typed dependencies and compiled routes

The single local object, #67900, is bound to `arenaobj_proc`
([binding](../../../src/specs/specs.assign.c#L1699)). Its `CMD_ENTER` callback
checks the enabled/open state and race-war eligibility, then moves an entrant
directly to the first staging room for their race. This is a non-WLD arrival
route from outside the zone. An active Tharnadia reset of that same local
object supplies the incoming copy at #132574.

### Undead return path

The callback's in-arena branch uses the compiled hometown table. Its comments
label #6075 Goodie and #29700 Evil; the live room titles resolve to “The Swamp
of the Clavikord” in `lizard.wld` and “The Goodie Bitchin Room” in
`cbl_hall.wld`, respectively. Undead's configured value is -1. The branch
selects Evil only for Evil race-war characters and sends every other eligible
race to #6075. Separately, the final-death branch in `fight.c` indexes
`arena_hometown_location[arena_team(ch)]`; for an Undead team member that
index is -1. Check the intended destinations for all three teams; source
review cannot establish the runtime behavior of room -1 or any alternate
spell/command exit ([hometown table and callback](../../../src/combat/arena.c#L50),
[team indexes](../../../src/combat/arena.h#L28),
[Goodie-comment destination](../../../areas/wld/lizard.wld#L1979),
[Evil-comment destination](../../../areas/wld/cbl_hall.wld#L1),
[final-death branch](../../../src/combat/fight.c#L2640)).

| Team | Code comment and configured value | Current WLD owner/title | Portal exit path | Final-death path |
| --- | --- | --- | --- | --- |
| Goodie | `GOODIE`, #6075 | `lizard.wld`, “The Swamp of the Clavikord” | #6075 | #6075 |
| Evil | `EVIL`, #29700 | `cbl_hall.wld`, “The Goodie Bitchin Room” | #29700 | #29700 |
| Undead | `UNDEAD`, -1 | No room configured | Falls through to #6075 | Indexes -1 |

The code comments and destination titles leave the intended Goodie/Evil
mapping unclear; Undead also lacks a valid configured return. These are
source-level discrepancies to resolve with the arena owner, not verified
runtime failures.

The Undead staging rooms #67900/#67901 form a static pair with no reset portal
and no WLD connection to the Goodie or Evil squares where local portal copies
are reset. At match end, the arena code returns participants to #67900. This
means the local portal route provides no authored return from the Undead
staging component. Treat this as a builder/maintainer question, not as a
claim that all other spell, staff, or administrative exits were tested.

The arena subsystem starts disabled (`DEFAULT_ENABLED 0`) and in Deathmatch
mode (`DEFAULT_TYPE 5`); a trusted user can enable it with the arena command.
The arena type table also includes Capture the Flag. A distinct global CTF
flag subsystem defines its primary Good/Evil flags at #132573 and #97628,
outside this WLD map; builds without the MySQL backend explicitly fence that
subsystem off. These are separate source mechanisms, so the `CTF Tournament`
zone title alone does not establish that this area currently hosts a CTF
match. The sole configured arena map is labeled “Opposing Castles,” though the
WLD room titles do not explain a castle story
([arena defaults](../../../src/combat/arena.h#L8),
[map and game labels](../../../src/combat/arena.c#L37),
[admin command](../../../src/cmd/actnew.c#L3629),
[global flag table](../../../src/combat/ctf.c#L31),
[no-MySQL behavior](../../../src/combat/ctf.c#L48)).

Exact typed scans found no active or unregistered QST item command, SHP
product, story-sidecar item reference, external active duplicate MOB/OBJ
prototype, external type-25 portal targeting an arena room, ITEM_SWITCH target,
or local SHP/QST/story outbound item use. The active Tharnadia reset of local
object #67900 is the only outside ZON reset-role reference found. Direct code
references are confined to the arena map/start tables and the object callback
binding. The external portal scan covered 872 parsed type-25 declarations and
found no target into `arena`. A separate all-object scan reports one truncated
terminal block in `end.obj` at #9999999; it has no complete prototype record
or target matching an arena room. No candidate migration edge is inferred
from room-number overlap.

## Title, same-stem history, and replacement review

After color-code normalization, `CTF Tournament` is unique among 442 current
`.zon` titles. Followed `arena.wld`, `arena.obj`, and `arena.zon` history stays
on the same stem. The path begins with the 2009 areas-tree move in commit
`642da5400`; commit `bb0d59425` (2012) records an object schema/read-write
conversion; commit `71d731c5b` (2013) changes the object's trailing numeric
data while WLD/ZON content is unchanged; and commit `f18992b8f` (2015) runs
the quest-item flag pass on ZON reset metadata. Those edits show no
alternate-stem rename, distinct predecessor, or whole-zone replacement. No
duplicate current title or replacement family was established.

## Builder decisions

1. Confirm whether `CTF Tournament` names an intended CTF mode or a legacy
   label for this staff-enabled arena. The compiled startup defaults are
   closed Deathmatch; the global CTF primary flags are configured elsewhere.
2. Verify every team's intended return. The Goodie/Evil code comments do not
   match the current room titles, and Undead has no destination, no portal
   reset in #67900, and falls through the exit branch to the Goodie-comment
   value. Resolve these paths before treating any team return as safe.
3. Confirm that The Ogre's Foot Lounge is the intended public entry point.
   If so, document the event access and eligibility; if not, identify the
   intended arrival point for the active Tharnadia reset.
4. Keep the story classification functional and event-driven unless a builder
   supplies an intended account of the vaults, tournament stakes, and lasting
   outcomes. The current prose does not establish those details.

## Archive disposition

Keep active `arena` **DO NOT MOVE**. Preserve the Tharnadia reset of object
#67900, the compiled #67900 callback binding, all three start-room pairs, the
#67906–#67986 battle-map range, and both configured team return locations.
Resolve the Undead return before claiming all three team routes are safe.
`arena` is an active `areas/AREA` contributor and is not an attic candidate;
add no candidate row. The register now records the source dossier and typed
dependencies in [coverage row 14](ZONE_COVERAGE_REGISTER.csv#L14).
