# Plane of Earth

Reviewed October 10, 2026 for the active `area:plane_earth_one` contributor,
zone 238, VNUMs #23801–#23929. Its source files are
[`plane_earth_one.wld`](../../../areas/wld/plane_earth_one.wld),
[`plane_earth_one.mob`](../../../areas/mob/plane_earth_one.mob),
[`plane_earth_one.obj`](../../../areas/obj/plane_earth_one.obj), and
[`plane_earth_one.zon`](../../../areas/zon/plane_earth_one.zon). The area has
129 room records, eight mobile prototypes, and eleven object prototypes. It
has no local QST file or story sidecar.

This is the first active Earth elemental area. The active Temple of the Earth
(`earth`, zone 435) and Grumbar's Domain (`earthp`, zone 1312) have separate
maps and identities; they import selected Plane of Earth prototypes and share
one reciprocal WLD boundary with it. Their files do not replace or rename this
contributor.

## Environmental story in the rooms

Most rooms repeat the same sensory frame: solid rock in every direction,
darkness, and the feeling of being buried alive. The few distinct scenes break
that pattern deliberately or anomalously:

- Room #23806 is a warm pocket containing a floating sphere of lightning. Its
  description calls it a fragment of the Para-Elemental Plane of Lightning;
  its sector field is Mountain.
- Room #23833 is titled “A Jet Stream” and describes a chromatic stream
  stretching in all directions. Its sector field is Fire Plane.
- Room #23865 marks the dense, dark core of the plane around an ancient
  monolith with undecipherable runes. The room description does not name a
  matching local object or record a quest interaction.

The residents reinforce a material and elemental setting: stone golems,
earth elementals, large earth elementals, dao nobles, xorn, and earth treants.
Ogremoch is described as a huge mountain of living rock and one of four
elemental princes. Entemoch's mobile description says he is Ogremoch's brother
and periodically returns to the plane while searching for him. These are
descriptions and reset definitions, not evidence of current spawn state or a
completed encounter.

## Residents and reset-defined equipment

The local mobile prototypes are #23801 stone golem, #23802 earth elemental,
#23803 large earth elemental, #23804 dao noble, #23805 xorn, #23806 Ogremoch,
#23807 earth treant, and #23808 Entemoch. The zone reset file places the
ordinary residents through the map and names Ogremoch's load room as #23850.
That room has no local WLD inbound edge, although the mobile can have
movement behavior; actual placement and movement were not tested.

The reset file equips Ogremoch with local objects #23807 (dusty boots) and
#23806 (dust gauntlets). It equips Entemoch with #23809 (stone boots) and
#23808 (stone gauntlets), loading him in room #23926. It also gives Ogremoch
three imported objects: #55198 from White Harbor, #359 from Heavens, and
#14123 from Realm. These are static reset references; they do not establish a
live inventory episode. The local object catalog also defines two potions,
mithril ringlets, a black color pool, the Mace of the Earth, a rock waistband,
and a nipple ring. The reset file places the pool in #23822, ringlets on a
resident in #23840, the waistband on a resident in #23801, and the ring in
#23824.

The two stone objects #23808 and #23809 carry `_noquest_` in their object
keywords. Current world-quest reward selection excludes that keyword when
choosing generic random reward sources; this does not disable the Heavens
QST's fixed requirements by VNUM. An August 2009 same-stem edit added the tag
to these objects.

Four equipment objects have a second, explicit story use in the active
Heavens QST. Its final stage calls for reagents in solid and crushed states,
then records item requirements #23809, #23808, #23807, and #23806. Their
local resets associate the stone pair with Entemoch and the dusty pair with
Ogremoch. The owning contract remains Heavens; Plane of Earth has no native
QST or local story receipt. The source establishes the required object IDs
and reset placements, but not whether an adventurer can obtain them in a
particular live state.

The same 2009 edit added several trap exits from #23926 into terminal room
#23927, with the commit message describing a reduced chance for Entemoch to
reach the map. This is source history, not evidence of a current encounter or
live path.

## Routes represented outside ordinary room exits

The black color pool object #23804 is bound to `magic_pool`. Its values name
Ethereal room #12492 and a 200-damage effect; the handler sends non-trusted
users there while leaving them at one hit point if the damage would be lethal.
This code route is separate from the WLD graph and has no paired Earth WLD
return edge. In the other direction, active Negative Plane object #26660 is a
type-25 fissure reset in room #26693 whose destination is Plane of Earth room
#23833. It is an object route, not an authored WLD boundary.

Gate, Nether Gate, and Plane Shift use #23801 as the Earth anchor and call a
helper that samples the loaded room range for that zone, subject to entry and
teleport restrictions. Two other code destination lists, for Mind Travel and
the illithid teleport veil, contain Earth rooms #23805 and #23812. These
references show code-defined destinations; they do not qualify which landing
rooms are eligible, safe, or returnable in live play.

The numeric source bounds need an owner decision. `IS_EARTH` ends at #23925,
while `EARTHP_VNUM_END` extends to #23929. The loaded zone and WLD contain
four further rooms (#23926–#23929): Ogremoch's load room, a terminal room
titled “Trap room,” a two-exit staging room, and an “exit to plane” room. They
are outside the `IS_EARTH` interval but inside the broader Earth-plane range
and zone top. No use of either bound macro was found outside its definition,
so this difference is recorded without assuming a runtime effect. The reset
file places Entemoch in #23926 even though the WLD title calls it “Ogremoch
load room”; ask the source owner whether the title or reset is stale and
whether the trap branches still serve their intended role.

## Story boundary

The source supports a rock-and-darkness environment, a small number of
elemental landmarks, two named princes, and a four-object item dependency owned
by Heavens. It does not define a Plane of Earth dialogue chain, a local quest
completion state, a monolith interaction, or a local narrative outcome. Keep
those as builder decisions rather than inferring them from descriptions,
equipment, or static reset commands.

See the [Checkpoint 322 audit review](../quest-story-followups/PLANE_OF_EARTH_AUDIT_REVIEW.md)
for the graph, cross-area dependencies, history, retention disposition, and
recommended builder questions.
