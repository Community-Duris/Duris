# Troll Slave Compound: captivity stories without a native rescue contract

Status: source-comprehensive map of the active torture area. The active source
set contains 31 rooms, 19 mobile prototypes, 17 object prototypes, and 98 reset
commands. There is no `areas/qst/torture.qst`, `areas/shp/torture.shp`, or
`areas/story/torture.story.json`; `areas/qst/AREA` nevertheless contains a
`torture *954` entry. That manifest entry is not a QST record or contract.
This review records authored text and source behavior; it does not qualify
installed reachability, reset availability, or any live rescue outcome.

## Native story contract

The title is “Troll Slave Compound.” Its rooms describe a prison, shaft labor,
a main walk with a forge, bone workshop, mess and guard post, holding blocks,
torture chambers, a scalping room, corpse storage and crematory, and a deep
pit. The opening sign says all tours are postponed. A second sign threatens
slackers with torture, and the guard-post etching is “useless to the
unpracticed eye.” These are authored environmental clues, but there is no
local quest file, selected special handler, shop, identified wanted target,
or reset-driven consequence that makes either sign a completed activity.
[Room and sign text](../../../areas/wld/torture.wld#L1) ·
[workshop sign](../../../areas/wld/torture.wld#L484) ·
[guard-post etching](../../../areas/wld/torture.wld#L414)

The people are authored as captives and forced workers: beaten duergar in the
labor tunnels; a human slave in the guard post; duergar and drow slaves at
their forge and bone workshop; a troll criminal in solitary treatment; two
distraught trolls with head torturers; and an elven lad with a troll scalper.
Those placements establish source participants and tone. No selected native
record gives a player custody, release, escort, safe destination, or saved
benefit for any of them. Killing a guard, entering a cell, taking a restraint,
or reading a line of prose therefore has no source-backed rescue meaning.
[Mobile prototypes](../../../areas/mob/torture.mob#L13) ·
[human slave](../../../areas/mob/torture.mob#L153) ·
[criminal troll](../../../areas/mob/torture.mob#L140) ·
[scalping-room victims](../../../areas/mob/torture.mob#L192)

## Spatial source and authored access

| Subarea | Source route and content |
| --- | --- |
| Outer gate and greeting | Room 95400 has a north door to Ghore 11529 and a south door to greeting room 95401. The zone file resets the south pair closed. Room 95401 continues south to the main walk and down into the shaft. |
| Labor tunnels | The shaft descends 95401→95402→95403→95404; an east tunnel continues 95404→95405→95406→95407. Room 95407 branches north to a guard post, east to another guard post, and south to a rock heap. |
| Main walk and work rooms | From 95401, the south route starts at 95411 and continues through 95412–95414. Its east branches reach corpse storage, the bone workshop, forge, mess hall, and an interior guard post; west branches reach the holding blocks, torture chambers, scalping room, and pit. |
| Cells and crematory | One holding-block wing branches from main-walk room 95411; the other branches from junction 95414. The torture chamber at 95416 descends through a hatch to room 95421; corpse storage 95422 descends through a grate to fire-plane room 95423. Both down/up pairs are reset closed and locked. |
| Pit | The west route from 95413 reaches 95426, then the bottom through 95428–95430. Each of 95426, 95428, and 95429 has an open down exit and a fall-chance field. The pit fiend is reset at 95430. |

The active boundary is Ghore #11529 south to room #95400 north and has a
reciprocal return; both directions appear in the cross-area exit ledger
([Ghore to Torture](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L210),
[Torture to Ghore](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1146))
and [pair summary](../quest-story-followups/WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L151).
Neither boundary exit is a door: both records have exit flags 0 and key value
0 ([Torture](../../../areas/wld/torture.wld#L12),
[Ghore](../../../areas/wld/ghore.wld#L684)). The separate interior gate from
Torture #95400 south to #95401 is a door with key value 0; both directions are
reset closed ([reset rows](../../../areas/zon/torture.zon#L8)). The source does
not establish live route availability or interaction outcomes. A scan of all
repository WLD files found no other outside exit target in 95400–95430 and no
inactive source-only inbound to a Torture room.

| Door pair | Authored key and reset state |
| --- | --- |
| 95400 south ↔ 95401 north | Key value 0; both reset closed. |
| 95411 east ↔ 95422 west | Key value 0; both reset closed. |
| 95414 west ↔ 95417 east | Key value 0; both reset closed. |
| 95416 down ↔ 95421 up, “hatch” | Key value 0; both reset closed and locked. |
| 95422 down ↔ 95423 up, “grate” | Key value 0; both reset closed and locked. |

The reset commands encode the door states above; they do not prove that a
reset has run or establish live availability. The door handler maps state 1
to closed and state 2 to closed plus locked. A zero key field is not evidence
of a particular successful unlock method. [Door reset rows](../../../areas/zon/torture.zon#L8) ·
[door-state handler](../../../src/world/db.c#L7857)

## Fall and fire-plane behavior

The WLD fall fields are 10% in shaft room 95402, 20% in pit room 95426, 10% in
95428, and 5% in 95429. The room loader stores these fields as
**chance_fall**. The command interpreter checks that field while processing a
command in the room and can start a falling event; the selected falling code
can move a character through an open down exit and apply impact injury. This
is a command-triggered hazard, not an arrival-only marker. A selected fall can
cause impact injury, including fatal damage. Flight, levitation,
staff status, a caught climb, room state, and the fall route affect whether a
descent starts or continues. No live fall, death, or recovery outcome was
tested. [shaft fall field](../../../areas/wld/torture.wld#L65) ·
[first pit fall field](../../../areas/wld/torture.wld#L522) ·
[middle pit fall field](../../../areas/wld/torture.wld#L556) ·
[bottom pit fall field](../../../areas/wld/torture.wld#L577) ·
[fall parser](../../../src/world/db.c#L1395) ·
[command check](../../../src/cmd/interp.c#L1898) ·
[fall start](../../../src/world/falling.c#L113) ·
[impact handling](../../../src/world/falling.c#L356)

Room 95423 is authored as the Plane of Fire and described as an inferno; its
only WLD exit is the reset-locked grate back to corpse storage. The plane name
comes from the sector table and does not by itself establish a particular
damage amount or a player’s ability to enter the room.
[Crematory](../../../areas/wld/torture.wld#L460) ·
[sector names](../../../src/core/common.c#L558)

## Actors, resets, and custody

The 53 mobile reset rows distribute the local cast across the authored
workplaces. The source maxima include four outer compound guards, eight beaten
duergar, fourteen compound guards, four unruly trolls, two young umber hulks,
three hungry trolls, two berserkers, two distraught trolls and two head
torturers, one each of the dark halfling, puny ogre, human slave, criminal
troll, troll scalper, elven lad, pit fiend, and huge troll, plus two forge
duergar and two workshop drow. These are limits and reset rows, not observed
population counts.

One authored mismatch deserves explicit builder attention. WLD rooms 95421
(special treatment) and 95426 (the first pit room) carry **ROOM_NO_MOB**;
95421 also carries DARK, NO_MAGIC, NO_PRECIP and NO_SUMMON, while 95426 carries
DARK and NO_PRECIP. The ZON file
nevertheless has 100% M rows for criminal troll 95411 at 95421 and huge troll
95418 at 95426. The reset M path directly places its selected mobile in the
target room, while **ROOM_NO_MOB** is also consulted by pathfinding checks.
Treat these as authored reset placements, not evidence that ordinary mobiles
may wander into either room or that the flag suppresses those explicit placements.
[Room flags](../../../areas/wld/torture.wld#L428) ·
[pit flags](../../../areas/wld/torture.wld#L502) ·
[placement rows](../../../areas/zon/torture.zon#L119) ·
[flag definition](../../../src/core/defines.h#L396) ·
[pathfinding check](../../../src/world/graph.h#L132) ·
[reset mobile branch](../../../src/world/db.c#L7218) ·
[direct room placement](../../../src/world/db.c#L7305)

Object resets include five troll corpses and a blood object in corpse storage.
A jade earring is nested under the corpse prototype. A mangled corpse is
carried by the pit fiend with steel battle armor nested under that corpse.
Equipment and held objects outfit guards, laborers, the halfling, solitary
prisoner, pit troll, and scalper; they are not local QST inputs. Keep mobile
and object namespaces distinct: mobile 95400 is a compound guard, while
object 95400 is an alligator hide. [Corpse resets](../../../areas/zon/torture.zon#L49) ·
[jade earring nesting](../../../areas/zon/torture.zon#L54) ·
[prisoner equipment](../../../areas/zon/torture.zon#L119) ·
[pit and corpse stock](../../../areas/zon/torture.zon#L130) ·
[local objects](../../../areas/obj/torture.obj#L1)

## Builder questions and safe story boundaries

- If a future story should release captives, identify the intended recipient,
  their agency and eligibility, who controls the relevant lock, the actual
  release and safe destination, and what persistent result proves the rescue.
  Separate arrival, combat, item possession, escort, death, partial completion,
  reset, and replay.
- Decide whether the solitary troll and pit troll are intended fixed NPC
  placements despite their rooms’ **ROOM_NO_MOB** flags, then document the
  desired distinction between direct reset, wandering, and other mobile entry.
- Resolve whether the pit prose (“climbing up without aid ... is impossible”)
  is descriptive or an intended route rule. The WLD contains up exits from
  95430 through 95429 and 95428; these area files declare no key or special
  gate on that route.
- Identify the guard-post “Wanted” etchings’ intended target and action before
  making them a journal objective. The current WLD text supplies no name,
  mutable state, or native outcome.

## Explicit builder questions and recommended resolutions

The questions below identify decisions the current source leaves open. Their
recommendations preserve authored text and reset behavior until an intended
player action or outcome is explicitly defined.

| # | Source-backed question | Conservative recommendation |
| --- | --- | --- |
| 1 | Is the sign at the entrance saying “ALL DA TOURS ARE POSTPOONED” only a setting joke, or should it lead to an intended tour interaction? ([sign](../../../areas/wld/torture.wld#L20)) | Keep it as environmental text unless a specific guide, route, and tour outcome are designed and qualified. |
| 2 | Should `torture *954` remain in `areas/qst/AREA` while no `areas/qst/torture.qst` exists? ([manifest](../../../areas/qst/AREA#L301)) | Treat the row as manifest membership only; verify the list's loader purpose before editing it, and do not claim a local quest contract. |
| 3 | Is any rescue or captive-release story intended for this area at all? ([captive cast](../../../areas/mob/torture.mob#L13)) | Leave the captivity as environmental narrative until a builder defines an actual player action and outcome. |
| 4 | If a rescue is intended, which of the human slave, duergar, drow, chained criminal, or elven lad can be released, and where can each safely go? ([human slave](../../../areas/mob/torture.mob#L153), [duergar](../../../areas/mob/torture.mob#L167), [drow](../../../areas/mob/torture.mob#L180), [criminal](../../../areas/mob/torture.mob#L140), [elven lad](../../../areas/mob/torture.mob#L205)) | Define eligibility, agency, unlock control, destination, persistence, death/escort handling, and reset/replay before adding a rescue claim. |
| 5 | Is the workshop's claim that bone weapons are sold in Ghore meant to imply a player craft or trade service? ([workshop](../../../areas/wld/torture.wld#L484)) | Keep the marketplace line as lore; author recipes, keeper, stock, and ownership separately if a player economy is intended. |
| 6 | Do the guard-post “Wanted” etchings identify a target or hidden action? ([etchings](../../../areas/wld/torture.wld#L414)) | Name the target and observable action before using the etchings as an objective; otherwise retain them as scenery. |
| 7 | Is the whispering rock heap a clue to hidden content or atmosphere only? ([rock heap](../../../areas/wld/torture.wld#L191)) | No hidden route is present in the authored exits; preserve the text as atmosphere unless a destination and return path are specified. |
| 8 | Do the bone-workshop prose about workers being “quickly replaced” and the slacker threat describe an actual punishment or replacement event? ([workshop](../../../areas/wld/torture.wld#L488), [sign](../../../areas/wld/torture.wld#L499)) | Do not infer a scripted consequence from the prose; define any actor or reset state before presenting it as a working mechanic. |
| 9 | Should the south gate from outside room #95400 reset closed after each zone reset? ([gate](../../../areas/wld/torture.wld#L13), [door resets](../../../areas/zon/torture.zon#L8)) | Preserve the paired closed, non-keyed interior-gate resets pending an explicit entry-policy decision; key value 0 is not a particular key. |
| 10 | Should the Ghore boundary remain a reciprocal, no-door connection while the interior gate remains closed at reset? ([Torture exit](../../../areas/wld/torture.wld#L12), [Ghore return](../../../areas/wld/ghore.wld#L684)) | Keep the outside boundary distinct from the inner gate and describe it as an open WLD pair. |
| 11 | Are the 10% shaft fall field and labor-room prose about prisoners being taken back up intended to imply a player recovery route? ([fall field](../../../areas/wld/torture.wld#L65), [labor prose](../../../areas/wld/torture.wld#L149), [fall handling](../../../src/world/falling.c#L356)) | Treat the prose as prisoner narrative; verify player fall, injury, recovery, and return separately before promising them. |
| 12 | Do the 20%, 10%, and 5% pit fall fields intentionally coexist with the “quick and easy death” prose and upward WLD exits? ([pit falls](../../../areas/wld/torture.wld#L522), [middle fall](../../../areas/wld/torture.wld#L556), [bottom fall](../../../areas/wld/torture.wld#L577), [bottom prose and exit](../../../areas/wld/torture.wld#L583)) | Preserve the authored risks and exits; clarify intended climb requirements and actual damage only after separate behavior qualification. |
| 13 | Should the hatch to Special Treatment remain reset closed and locked with key VNUM 0? ([hatch reset](../../../areas/zon/torture.zon#L32), [room exit](../../../areas/wld/torture.wld#L435)) | Choose and document the intended unlock/access method and safe return before advertising this as a usable route. |
| 14 | Should the grate from corpse storage into the Plane of Fire remain reset closed and locked with key VNUM 0? ([grate reset](../../../areas/zon/torture.zon#L40), [crematory](../../../areas/wld/torture.wld#L460)) | Keep the source lock state and do not imply entry, survivability, or return until those outcomes are established. |
| 15 | Are direct reset placements of the criminal and huge troll intentional in rooms marked `ROOM_NO_MOB`? ([room flags](../../../areas/wld/torture.wld#L434), [pit flags](../../../areas/wld/torture.wld#L511), [criminal reset](../../../areas/zon/torture.zon#L119), [troll reset](../../../areas/zon/torture.zon#L130)) | Decide whether the fixed placements are intentional and document them separately from wandering and pathfinding restrictions. |
| 16 | Does the huge troll guard's “TOSSES you!” description promise a player-moving action, or is it combat flavor? ([mobile](../../../areas/mob/torture.mob#L235), [reset](../../../areas/zon/torture.zon#L130)) | Do not promise a toss route or effect from the description alone; identify and qualify the actual behavior before relying on it. |
| 17 | Are the shackles, collar, broken leg irons, and chain intended as escape tools, lock keys, or only equipment and scene props? ([shackles](../../../areas/obj/torture.obj#L76), [collar](../../../areas/obj/torture.obj#L132), [broken irons](../../../areas/obj/torture.obj#L149), [chain](../../../areas/obj/torture.obj#L163)) | Keep item identity and current resets stable; define a specific action and consequence before treating possession as release or door access. |
| 18 | Should the twitching corpse and blood flowing toward the grate remain environmental object descriptions, or do they imply an interactive encounter? ([corpse storage](../../../areas/wld/torture.wld#L443), [blood object](../../../areas/obj/torture.obj#L108), [corpse resets](../../../areas/zon/torture.zon#L49), [blood reset](../../../areas/zon/torture.zon#L55)) | Treat these as set dressing under the current reset source; add an actor or state transition only with a defined story purpose. |
| 19 | Is the scalping-room pairing of the troll scalper and elven lad meant to have a player-resolvable consequence? ([room](../../../areas/wld/torture.wld#L525), [scalper](../../../areas/mob/torture.mob#L192), [victim](../../../areas/mob/torture.mob#L205)) | Do not imply rescue, capture, or reward from their placement alone; define any intended outcome and replay policy first. |
| 20 | Are Special Treatment's DARK, NO_MAGIC, NO_PRECIP, and NO_SUMMON flags intended to constrain a future captive-release approach? ([room flags](../../../areas/wld/torture.wld#L434), [flag definitions](../../../src/core/defines.h#L396)) | Preserve the authored flags and include their actual effects in any reviewed route or rescue design; do not assume magic or summoning access. |

## Bounded typed closure and limits

The active area source set is the four files linked above: WLD, MOB, OBJ, and
ZON. The QST manifest contains `torture *954`, but there is no local QST file;
there is also no SHP file or story sidecar. Exact VNUM scans across
MOB/OBJ/QST/SHP/story sources found only the local mobile and object
definitions; the reset scan found no external ZON consumer, and the source
code scan found no literal binding for rooms or prototypes 95400–95430. The
Torture WLD has one outbound boundary record, reciprocated by Ghore. This is
bounded source closure, not proof against indirect lookup, editor/tooling
references, arbitrary persisted state, or runtime-generated travel.

Reviewed source totals: 31 room headers, 61 WLD exit records (60 internal
directed records and one Ghore boundary), four fall-chance fields, 19
mobile headers, 17 object headers, and 98 reset commands (53 M, 24 E, 10 D,
6 O, 3 G, and 2 P). The review used read-only source scans; no build, test,
runtime, database, migration, or operational command was run.
