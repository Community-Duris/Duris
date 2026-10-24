# The Seacaves

**Source scope:** active seacaves contributor, zone header 22. This is a
source-story map, not a gameplay or migration qualification.

The [active area list](../../../areas/AREA#L10) assigns the stem seacaves
to rooms and prototypes numbered 2200–2295. Its [world file](../../../areas/wld/seacaves.wld#L1)
contains 95 rooms, its [mobile file](../../../areas/mob/seacaves.mob#L1)
35 mobiles, its [object file](../../../areas/obj/seacaves.obj#L1) 24
objects, and its [reset file](../../../areas/zon/seacaves.zon#L1) places
them through 157 source reset commands. areas/qst/AREA lists the stem, but
there is no local QST file, shop file, or story sidecar. The prose and source
bindings below do not establish a local quest chain or an installed service.

## Place and story areas

### Coast, plateau, and cliff path — rooms #2200–2230

The approach is a grass-covered plateau between a road and the cliffs above a
rocky beach. Room #2200 calls it a path from the High Road; active Surface
room #558717 connects to it. A [source-only Surf room](../../../areas/wld/surf.wld#L266832)
also points west into #2200, but that inactive map record is not loaded-world
access.

The beach and ledges frame several small environmental stories: a wagon
carrying Jeronis the Verzanan cloth-maker's goods lies abandoned with bodies
in it; hand-cut ledges and stairs connect the cliff to the beach; gulls nest
in the rock; and an osprey pair raises its young in a protected cave
([coast and wagon](../../../areas/wld/seacaves.wld#L1),
[nest](../../../areas/wld/seacaves.wld#L191)). The text calls the narrow ledge
dangerous, but the source audit found no fall field in these rooms.

### Bandit hideout — rooms #2231–2260

A concealed cave network houses a coastal bandit group. Their lookout watches
caravans and merchants traveling between Verzanan and the northern cities;
the abandoned wagon on the plateau links that threat to the approach. Inside
are guard posts, a leader's room, barracks, kitchen, pantry, training and
sleeping areas, and a ventilated vertical shaft with a trapdoor. Secret
entrances and closed passages make the hideout feel deliberately concealed
([lookout](../../../areas/wld/seacaves.wld#L1395),
[barracks](../../../areas/wld/seacaves.wld#L1044),
[shaft and upper hatch](../../../areas/wld/seacaves.wld#L1350)).

The reset file places an iron key on the head guard and a locked wooden chest
in his screened sleeping area; the chest contains a shield. This gives the
camp a small internal security detail, though no larger bandit objective or
resolution is authored in a QST file.

### Temple of the Sea Goddess — rooms #2261–2264

The temple's repeated crashing-wave sound, flashing storm-like light, and
murals show a coastal city destroyed by a tidal wave. Its altar bears the
two-waves symbol. The priest's quarters are sparse, with the local white-robed
priest and a reset for Adryv, a Sahaugin priest of Sekolah, in the same room
([temple](../../../areas/wld/seacaves.wld#L1593),
[altar room](../../../areas/wld/seacaves.wld#L1628),
[priest's quarters](../../../areas/wld/seacaves.wld#L1651)).

The mural object describes the Sea Goddess rising beside the destroyed city
and suggests that one section can move
([mural object](../../../areas/obj/seacaves.obj#L114)). The source does not
identify the city or say whether the depiction is history or prophecy. The
temple's east exit points to the priest's quarters #2264 through a doorway
described as hidden in the mural
([exit](../../../areas/wld/seacaves.wld#L1611)). Object #2208 is an
ITEM_SWITCH configured for PUSH on that east exit. Its generic procedure
clears the blocked state on both directions and sends a message that the
mural moves aside, but leaves the object in place. The reset marks the exit
“open/blocked”
([door reset](../../../areas/zon/seacaves.zon#L40),
[mural placement](../../../areas/zon/seacaves.zon#L116)). This behavior is
source-coded but was not played. The deity is called the Sea Goddess in area
prose, while the compiled altar behavior calls her the Water Goddess.

### Mage's rooms and summoning chamber — rooms #2265–2267

A separate corridor leads to a cramped mage's room filled with reagents and
to a candle-ringed summoning chamber. The chamber's open book is named
“Mezit's Tome of Summoning”; its blue chalk circle, brazier, ornate mirror,
and living-looking shadows establish a ritual scene. A young mage, a glowing
staff, and an imp are reset in the rooms, but the prose does not state what
the mage summoned, what the mirror does, or what follows the encounter
([mage room](../../../areas/wld/seacaves.wld#L1699),
[summoning room](../../../areas/wld/seacaves.wld#L1744)).

### Crabman burrow — rooms #2268–2279

A widened cave network belongs to a scavenging crabman clan. It includes the
chief's room, a shared gathering area, a nursery with an adult watching the
young, and a shaman's space with woven seaweed and totems. The final niche
holds a polished-shell and salvaged-object trove, including a steel helm
([burrow entrance](../../../areas/wld/seacaves.wld#L1788),
[chief](../../../areas/wld/seacaves.wld#L1880),
[shaman](../../../areas/wld/seacaves.wld#L2048),
[trove](../../../areas/wld/seacaves.wld#L2082)).

The mobile descriptions present the clan as communal and protective of its
young. The scavenging and collection details give it a material culture
distinct from the bandits and shipwreck residents.

### The Wind Dancer wreck — rooms #2280–2295

The grounded barque Wind Dancer has a hole below the waterline, a flooded hold,
missing rigging, and fire-blackened rooms. Passenger cabins have been
ransacked; one has claw marks and children's toys among its wreckage. The
captain's room is also searched. In the hold, a steel chest remains locked
despite dents from attempts to open it. The ship is occupied by lacedons and
the captain's spirit ([wreck and deck](../../../areas/wld/seacaves.wld#L2105),
[passenger cabin](../../../areas/wld/seacaves.wld#L2277),
[fire room](../../../areas/wld/seacaves.wld#L2328),
[captain's room](../../../areas/wld/seacaves.wld#L2365),
[hold](../../../areas/wld/seacaves.wld#L2433)).

The reset file gives the captain's spirit a steel key; a steel chest in the
hold contains a gold band. The text says the fire started in a particular
cabin and that someone or something searched the ship, but does not name the
cause, attacker, route, passengers, or fate of the captain. The lacedons and
spirit establish a haunting without resolving it.

## Source-backed mechanical bindings

Two local objects have explicit compiled special procedures. The coral altar object
#2203 is assigned unspec_altar; praying to it with “altar” in the argument
lets an eligible living player character remove their specialization for ten
epic points. On a completed payment, the code clears the specialization,
updates skills, and forgets spells. The coral sacrificial dagger #2204 is
assigned cutting_dagger; picking it up from a room as a living player
character deals four generic damage, then allows the GET command to continue.
The reset file nests dagger #2204 inside altar #2203 in altar room #2263.
Mural object #2208 is a third, data-bound behavior: its values select PUSH,
temple room #2262, east, and the “switch item moves” message mode. The loader
binds the generic switch procedure to that type when no explicit procedure
is assigned. The procedure clears the blocked state on the configured exit
and its reverse, then sends a movement message without removing the object.
The [object values](../../../areas/obj/seacaves.obj#L120) and [reset](../../../areas/zon/seacaves.zon#L116)
nest the switch in room #2262. Source definitions are in
[specs.assign.c](../../../src/specs/specs.assign.c#L1490),
[specs.character_progression.c](../../../src/specs/specs.character_progression.c#L170),
[specialization payment callback](../../../src/classes/specializations.c#L735),
[specializations.c](../../../src/classes/specializations.c#L750), and
[specs.seacaves.c](../../../src/specs/specs.seacaves.c#L18),
[the switch procedure](../../../src/specs/specs.object.c#L302),
[the default binding](../../../src/world/db.c#L5281), and
[ITEM_SWITCH](../../../src/core/defines.h#L144) and
[the PUSH command value](../../../src/cmd/interp.h#L305).

The reset file also imports equipment objects #209, #210, #214, #215, #227,
#228, #239, #245, #246, #283, #288, and #386 from active heavens prototypes,
and mobile #55265 from active wh. Exact placements and owners are listed in
the [Sea Caves audit review](../quest-story-followups/SEACAVES_AUDIT_REVIEW.md#typed-dependencies-and-compiled-bindings).

## Source records

- [WLD rooms and descriptions](../../../areas/wld/seacaves.wld#L1)
- [MOB prototypes](../../../areas/mob/seacaves.mob#L1)
- [OBJ prototypes and extra descriptions](../../../areas/obj/seacaves.obj#L1)
- [ZON placements and door resets](../../../areas/zon/seacaves.zon#L1)
- [Coverage row 261](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L261)
- [Sea Caves audit and builder questions](../quest-story-followups/SEACAVES_AUDIT_REVIEW.md)
