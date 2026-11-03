# PortSkythic: The Ruins of Port Skythic

The active `PortSkythic` contributor is zone 1380, titled “The Ruins of Port
Skythic.” It owns 110 rooms (#138000–#138109), 20 mobile prototypes, and 15
object prototypes. Its local zone resets four rumbling rock golems and twelve
object-placement records. The source files are listed in [`areas/AREA`](../../../areas/AREA#L392)
and [`areas/qst/AREA`](../../../areas/qst/AREA#L341); the authored title and
reset mode are in the [zone header](../../../areas/zon/PortSkythic.zon#L1).

## Story and quest boundary

Port Skythic is portrayed after a siege and invasion. The road leading toward
the city changes from broad, well-kept stone and grass to dried blood, scorched
ground, abandoned war machines, destroyed merchant stalls, and the shattered
front gate. The text calls the gate's destruction unnatural and suggests a
magical blast, but does not identify the attackers or establish exactly what
caused it. The bridge between the outer and inner gates is badly damaged; its
description suggests the city's defenders may have destroyed part of it to
slow an advance. The inner gatehouse records the defenders' final resistance,
while the lack of bodies is attributed to the siege having ended some time
earlier ([approach, gates, and bridge](../../../areas/wld/PortSkythic.wld#L1),
[inner defenses](../../../areas/wld/PortSkythic.wld#L175),
[gatehouse](../../../areas/wld/PortSkythic.wld#L1518)).

The city streets preserve signs of an ordinary trading and residential life:
the Kings Walkway once served shops and taverns, and the outer gate approach
held merchant stalls. Their ruins include a destroyed Raging Boar Tavern, a
burned Dancing Sun Inn, a hidden gaming room and lounge, a blood-splattered
brothel, pillaged homes, and bloodied alleys. The current sources do not give a
definitive date for the siege or explain what happened to the residents
([Kings Walkway and tavern](../../../areas/wld/PortSkythic.wld#L206),
[inn and hidden rooms](../../../areas/wld/PortSkythic.wld#L328)).

In the Celestial District, the altars to Bahamut and Pelor are named as
desecrated. The zone places a blood-splattered platinum dragon statue at the
Bahamut altar and a statue of Pelor mounted on a ki-rin at the other altar.
This is religious evidence within the ruin, not a defined quest or explanation
for the desecration ([district](../../../areas/wld/PortSkythic.wld#L887),
[dragon statue reset](../../../areas/zon/PortSkythic.zon#L43),
[Pelor statue reset](../../../areas/zon/PortSkythic.zon#L44)).

Other parts of the contributor stand out from the damage around them. One
street is described as oddly pristine; it leads to a tower of sorcery with a
library, study, classroom, room of portals, and private spellcrafting rooms.
The nearby room titled “Within the Magic Shop” has no matching local shop
source. The harbor includes war-torn and sunken piers beside two recently
rebuilt piers. A six-room sunken pirate ship and a separate room titled “A
Rift in Space and Time” also appear in the WLD, without authored WLD routes
back to the city ([tower and shop](../../../areas/wld/PortSkythic.wld#L1047),
[piers](../../../areas/wld/PortSkythic.wld#L1181),
[ship](../../../areas/wld/PortSkythic.wld#L1443),
[rift](../../../areas/wld/PortSkythic.wld#L1513)).

The mobile prototypes name dark naga warriors, sentinels, sorceresses,
hunters, and enslavers, a slave trader and master, and enslaved two-headed
creatures. Most have the generic short name “unnamed mob” and a generic
description. The one specifically described rumbling rock golem is the only
mobile placed by the local zone resets: two are reset at the front gate and
two at the secondary gate. The objects include broken gate metal, a shattered
oil pot, charred stone, tavern and inn signs, wine casks, a poker table and
chips, a thief's corpse, a tortured prostitute's remains, torn naga scales,
and statue fragments. Three object prototypes—the thief's corpse, torn naga
scales, and giant statue ruins—are not placed by the local reset file
([mobile prototypes](../../../areas/mob/PortSkythic.mob#L1),
[enslaved-creature prototype](../../../areas/mob/PortSkythic.mob#L78),
[golem prototype](../../../areas/mob/PortSkythic.mob#L89),
[object prototypes](../../../areas/obj/PortSkythic.obj#L1),
[object placements](../../../areas/zon/PortSkythic.zon#L33),
[golem placements](../../../areas/zon/PortSkythic.zon#L46)).

There is no local QST, SHP, or story sidecar, and no native Q/QA quest
contract. QST `AREA` membership is present, but does not indicate that a local
quest file exists. This dossier records the authored setting and evidence; it
does not supply an attacker, timeline, or player objective missing from the
sources. The [repository-wide audit review](../quest-story-followups/PORTSKYTHIC_AUDIT_REVIEW.md)
reconciles map access, resets, source-only approaches, dependencies, history,
and builder decisions.

## Owner questions

- Who attacked Port Skythic, and should the source continue to leave the
  attackers and cause of the magical-looking breach unnamed?
- Is the six-room sunken ship meant to be reached from the piers, and what is
  the intended access and return behavior for the isolated rift?
- Which naga and enslaved mobile prototypes are intended to appear here, and
  should their names, descriptions, or reset placements be completed?
- Should the tower's portal room or the room titled “Within the Magic Shop”
  have a service or route binding, or are they environmental set dressing?
- Are the closed and secret tavern, storage, inn, lounge, and brothel passages
  in their intended reset state?
- Should the source define how the desecrated altars and recently rebuilt
  piers relate to the siege, or remain unexplained landmarks?

## Current working resolution

Keep active `PortSkythic` **DO NOT MOVE**. Preserve its room and prototype
VNUMs, source reset records, isolated ship and rift components, and existing
door setup while the area owner resolves the questions. Keep the 224 distinct
approach records from inactive `Duris3.wld` on that existing candidate's
**RETAIN IN PLACE** plan; they do not create an active route into the port.
No gameplay, access, reset, or source migration change is proposed.
