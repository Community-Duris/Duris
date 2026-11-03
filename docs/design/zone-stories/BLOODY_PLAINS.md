# The Bloody Plains (`bloody_plains`) source story dossier

This dossier reviews the active area's room prose, creatures, objects, and route
clues. The [Checkpoint 194 audit supplement](../quest-story-followups/BLOODY_PLAINS_AUDIT_REVIEW.md)
records the WLD graph, reset and fall markers, cross-area boundaries, typed
dependencies, and bounded title/history review. The sources describe a static
story; this review does not qualify live room contents, actual access, combat,
fall resolution, or safe return.

## A battlefield with no settled aftermath

The WLD repeatedly presents the plains as the remains of a battle between good
and evil. Its common description gives skeletal remains across a devastated
plain, craters large enough to swallow groups, melted or blasted boulders, and
thick fog with eerie silver mist. The same text says that no plant or animal
life can be seen ([representative room](../../../areas/wld/bloody_plains.wld#L2988)).
The repeated wording makes the battlefield itself the area's dominant story,
but it gives little detail about who fought, what magic caused the devastation,
or how long ago it happened.

Road rooms interrupt the open wasteland. The old road is made from gigantic
stones joined by wooden beams; other routes are worn trails, an old road, and
an intersection where several roads meet ([road](../../../areas/wld/bloody_plains.wld#L3907),
[intersection](../../../areas/wld/bloody_plains.wld#L4648),
[abrupt end](../../../areas/wld/bloody_plains.wld#L518)). Room descriptions
place these routes amid fog and craters. The road network offers recognizable
navigation landmarks, but the source does not establish that every road avoids
the fall hazards or is safe from creatures.

## Craters, falls, and buried spaces

Several crater descriptions show smooth walls, broken stone, openings into
darkness, midair rooms, and bare rocky bottoms. The descriptions attribute at
least one crater to an ancient meteorite. A deadfall of hollowed timber and
tangled brush contains animal and humanoid bones, a strong stench, and a
leucrotta family lair ([crater](../../../areas/wld/bloody_plains.wld#L3865),
[leucrotta lair](../../../areas/wld/bloody_plains.wld#L3423)). These places
turn the battle scar into present-day terrain and animal habitat.

The map also has twelve chance-fall records from 78% to 100%. Some continue
down through additional midair rooms before reaching a crater floor. Their
exact chances and destinations are listed in the audit supplement. The old
sign warns that travelers have died in the craters, but no room prose promises
a safe descent, recovery route, or safe return. Treat the warning as a hazard
clue, not as proof that the adjacent roads bypass every fall.

## A shack, a hag, and the rusty key

A small broken-down shack is the area's separate enclosed scene. It has a
window, door, shelf, potions, a black elixir, and a torn scroll. An old hag is
reset inside with a red brooch containing a hag's eye and a frayed black robe
with red lining ([shack](../../../areas/wld/bloody_plains.wld#L1),
[hag prototype](../../../areas/mob/bloody_plains.mob#L134),
[resets](../../../areas/zon/bloody_plains.zon#L17)). The source gives the
occupant a potion-making and chanting activity but does not explain her
relationship to the battle or to the creatures outside.

The shack entrance is reset closed from both sides; the inward-facing exit is
also locked with key VNUM 33513. The old rusty key is reset at the bottom of a
crater in room #33695 ([door resets](../../../areas/zon/bloody_plains.zon#L8),
[key placement](../../../areas/zon/bloody_plains.zon#L30),
[key prototype](../../../areas/obj/bloody_plains.obj#L124)). This is a clear
source connection between the crater route and the shack, but the source alone
does not show whether finding the key, opening the door, and returning from the
crater are intended to be fair or practical.

## Creatures and surviving life

The local mobile set contains nine prototypes: mature male, protective female,
and immature leucrottas; a giant blood worm; a red chromatic dragon; an old
annis hag; and small, large, and leader wild dogs. The female says she is
protecting her young, and the young are reset with her in the two-room deadfall
lair. The red dragon and its scales occupy a crater-side scene; dog packs and
mature male leucrottas are spread through other rooms. A blood worm is placed
near the far crater depths ([mobile prototypes](../../../areas/mob/bloody_plains.mob#L1),
[local resets](../../../areas/zon/bloody_plains.zon#L32)).

The eighteen objects include troll and ogre remains, coins and equipment,
potions and a torn scroll, the rusty key, the warning sign, dragon scales, and
a small pool. The pool's extra description explicitly says that small insects
swim at the surface; crater-bottom prose says green moss covers a slippery
wall ([pool](../../../areas/obj/bloody_plains.obj#L153),
[crater bottom](../../../areas/wld/bloody_plains.wld#L5940)). These are concrete
exceptions to the repeated claim that the area cannot support organic life.
The discrepancy may be deliberate evidence of resilient life or a difference
between the battlefield's broad appearance and specific pockets. The sources
do not explain which reading is intended.

## Builder questions and proportionate follow-up

- **What does “barren” mean here?** Reconcile the repeated no-life room text and
  sign with leucrottas, wild dogs, the worm, dragon, hag, pool insects, and
  crater moss. If “barren” means sparse or unable to support ordinary growth,
  qualify the broad claims; otherwise explain why these specific inhabitants
  endure. Do not change reset populations without builder direction.
- **What is the intended crater experience?** Decide how characters are meant
  to discover the twelve fall zones, whether flight, levitation, climbing, or
  another ability should change that experience, and how a visitor can recover
  and return. Verify the actual routes before describing any road as safe.
- **Should the key be in the deep crater?** Confirm that the key at #33695 is
  deliberately required for the shack, and whether its placement, the locked
  inward door, and return path form a fair route. Adjust prose or placement only
  after the intended access is clear.
- **Is the shack closed to ordinary traversal?** Confirm whether the paired
  reset door states are meant to isolate the hag and whether a player is meant
  to open the door with the rusty key. Static graph reachability does not show
  live door state or player access.
- **Is the Killing Fields boundary deliberately one-way?** Five other
  Killing Fields boundaries and the Surface route have direct returns; the
  east exit from Killing Fields #33422 to #33597 does not. Preserve accurate
  navigation if deliberate, or authorize a separate route repair if not.
- **Should `bloody_plains` have a command-target alias beyond “The Bloody
  Plains”?** The normalized title is unique in the current scan, and the
  bounded area sources do not establish a separate command alias. Keep the
  source key and display title; add an alias only after the owner names its
  exact target and intended distinction.

The area has no local `.qst`, `.shp`, or story sidecar and no native Q/QA
contract. Its story comes from room, mobile, object, and reset prose. The zone
header uses reset mode 0, so runtime periodic reset scheduling is a separate
availability question; static reset declarations do not establish current
occupants or item availability.
