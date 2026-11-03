# Village of the Damned

The active `yuan_ti2` contributor uses zone header #806 and the authored title
“Village of the Damned.” It contains 79 rooms, 37 mobile prototypes, 26 object
prototypes, and one shop. It is listed in both the full area list and the QST
area index; no local `.qst` or story sidecar exists. See the [active area
list](../../../areas/AREA#L286), [zone header](../../../areas/zon/yuan_ti2.zon#L1),
[QST area index](../../../areas/qst/AREA#L255), and [coverage row
352](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L352).

## Environmental story

A raised, narrow path cuts west from the swamp into an unusually quiet jungle.
Snake-like trails and human-sized footprints appear together near a rickety
bridge; later tracks are described as recent. The trail reaches a walled
Yuan-Ti village whose large, heavy gates stand open. See the [entry path and
tracks](../../../areas/wld/yuan_ti2.wld#L1), [bridge approaches](../../../areas/wld/yuan_ti2.wld#L66),
and [village gates](../../../areas/wld/yuan_ti2.wld#L183).

Inside, the square makes the social divide visible: most Yuan-Ti are poor and
live in ramshackle huts, while a few powerful residents occupy stout homes and
wealthy rooms. The mayor is appointed directly by the “living god” rather than
elected. The mayor’s quarters are described as richer than even the wealthiest
true-blood’s home. This political and class structure is explicit in the
[square](../../../areas/wld/yuan_ti2.wld#L358) and [mayor’s
quarters](../../../areas/wld/yuan_ti2.wld#L1468).

Several signs point to violence and fear, without naming who caused it. A
tortured elven corpse is nailed to a tree with “Death to ALL Invaders!” painted
on its chest. A second victim was staked in a hut before the hut was burned
from the victim’s belly outward. Another home is saturated with old and fresh
blood, and a separate campsite has blood across the ground and signs of a
long-term occupant. The source does not identify the attackers, the victims’
allegiances, or whether these scenes belong to one conflict. See the [corpse
and message](../../../areas/obj/yuan_ti2.obj#L17), [burned
hut](../../../areas/wld/yuan_ti2.wld#L325), [blood-splattered
home](../../../areas/wld/yuan_ti2.wld#L1095), and [campsite](../../../areas/wld/yuan_ti2.wld#L1691).

The village also contains a carefully tended garden of rare herbs and vines
that exude caustic sap, an abandoned hut with a muddy hole, and a golden eye
hidden in that hole. A nearby yellow orchid has been torn down and crushed.
The prose supplies these clues but no explanation for their purpose. See the
[garden](../../../areas/wld/yuan_ti2.wld#L509), [orchid
object](../../../areas/obj/yuan_ti2.obj#L37), and [golden eye and
hole](../../../areas/obj/yuan_ti2.obj#L144).

The population is not exclusively Yuan-Ti. A peaceful ancient elven bard
wanders close to the entrance and doubts he will be allowed to leave alive.
An unarmed human merchant is described as out of place and vulnerable. A
separate, **unreset** poor-human mobile says human men without serpentine
features are expected to bring back slaves for breeding and sacrifice. That
prototype’s text is part of the source record, but the reset file does not
place it as a normal encounter. See the [bard](../../../areas/mob/yuan_ti2.mob#L34),
[merchant](../../../areas/mob/yuan_ti2.mob#L184), and [unreset poor
human](../../../areas/mob/yuan_ti2.mob#L309).

A Yuan-Ti noble and a Githyanki noble with a following warrior load in the
mayor’s quarters. The Githyanki noble appears enraged, waits for something or
someone, and looks ready to attack the Yuan-Ti beside him. The source does not
explain the purpose of the visit or whether the arrangement is diplomatic,
coercive, or temporary. See the [Githyanki profiles](../../../areas/mob/yuan_ti2.mob#L470)
and [mayor-room resets](../../../areas/zon/yuan_ti2.zon#L99).

At the village edge, a disguised fence door leads north from room #80671 to
room #80554 in the separate active `yuan_ti` area. That room is titled “An
Overgrown Jungle Path”; the neighboring zone title is “The Temple to
Skrentherlog.” A rotting note recovered at the bloody campsite mentions a
temple guardian, a dragon called “Skren,” a hammer, a destroyed stone, divine
anger, and trapped souls. The note says it is written in elven script and was
not written in the village. The direct boundary is established, but the note’s
connection to Skrentherlog or the village is not. See the [fence
path](../../../areas/wld/yuan_ti2.wld#L1595), [neighboring
room](../../../areas/wld/yuan_ti.wld#L1341), [neighboring zone
title](../../../areas/zon/yuan_ti.zon#L1), and [rotting
note](../../../areas/obj/yuan_ti2.obj#L435).

## Authored interactions and limits

The reset file places the gold key #80612 on the angry human in room #80630.
That same room resets a heavy chest whose object data names #80612 as its key;
the chest contains two bolts of blue silk and a pile of platinum. This is a
source-defined key/container relationship. No authored room-exit key field
uses #80612. See the [chest and key fields](../../../areas/obj/yuan_ti2.obj#L201)
and [resets](../../../areas/zon/yuan_ti2.zon#L69).

The single local shop belongs to mobile #80628 in room #80656. Its `.shp`
record sells imported Heavens prototypes #364 (rations), #347 (a waterskin),
and #353 (venison jerky); the local reset table also gives those items to the
keeper. The shop’s configured refusal line reads “I do not sell to your
people!” The source does not establish which buyer condition triggers it. See
the [shop record](../../../areas/shp/yuan_ti2.shp#L1), [Heavens
objects](../../../areas/obj/heavens.obj#L3600), and [shopkeeper
resets](../../../areas/zon/yuan_ti2.zon#L81).

Local resets reference all 26 object prototypes and 35 of the 37 mobile
prototypes. Poor human #80619 and true-blood #80633 have no local mobile reset
placement. The latter’s appearance text closely resembles placed mobile
#80632. The remaining local reset relationships, including the Githyanki
follower command and the reset mobile in a named load-error room, are recorded
in the [audit supplement](../quest-story-followups/YUAN_TI2_AUDIT_REVIEW.md).

The outer swamp-path rooms use sector 26, `SECT_SWAMP`. The WLD has no authored
fall-chance or current records. These fields and the room descriptions do not
qualify travel hazards or safe return.

## Questions for the area owner

- Who are the “invaders” in the corpse’s message, and are the tortured elf,
  staked victim, blood-splattered home, and bloody campsite parts of one
  conflict?
- Is the unreset poor-human prototype intended to be unused, or is there a
  missing placement or other caller? Is its slavery text meant as a distinct
  story strand?
- What are the role and intended encounter state of the elven bard and the
  unarmed human merchant?
- What do the herb garden, caustic vines, crushed orchid, hidden golden eye,
  and rotting floor indicate?
- Why are the Githyanki waiting in the mayor’s quarters, and what is their
  relationship to the Yuan-Ti noble and guards?
- Are the secret fence route and the note’s references intended to connect
  Village of the Damned to Skrentherlog or another temple story?
- What is the purpose of rooms #80677 and #80678, and why does a true-blood
  carrying the “Snakebite” sword reset into the “SNAKEBITE LOAD ROOM” while
  the exitless #80678 is titled “SNAKBBITE DID NOT LOAD”?
- Is mobile #80633 intentionally unreset despite its near-duplicate text for
  #80632? Should either unplaced mobile be retained?
- Is the shop’s refusal line tied to a specific race or group, and are the
  three Heavens supply objects the intended imported stock?
- Why does the QST area index list `yuan_ti2` without a local `.qst` file?

## Current source resolution

Keep active `yuan_ti2` **DO NOT MOVE** and preserve its current room and
prototype VNUMs, both reciprocal active WLD boundaries, local chest/key and
shop dependencies, and reset placements. Keep the two inactive source-only
approaches with Surf and oldmap8 **RETAIN IN PLACE**; neither is active ingress.
No source move or gameplay change is proposed. The repository-wide audit
records the exact graph, outside references, title history, and candidate
disposition.

## Checkpoint 439: builder question review

Twenty explicit builder questions and recommended resolutions now gather the
open violence, character, clue, door, shop, and ownership decisions in
[Village of the Damned builder questions](YUAN_TI2_BUILDER_QUESTIONS.md#explicit-builder-questions-and-recommended-resolutions).
The earlier static audit remains the source for its graph, title, dependency,
and retained inactive-map evidence.
