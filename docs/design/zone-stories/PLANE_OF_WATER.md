# Plane of Water

Reviewed October 10, 2026 for active `area:plane_water_one`, zone 232,
VNUMs #23201–#23325. Its source files are
[`plane_water_one.wld`](../../../areas/wld/plane_water_one.wld),
[`plane_water_one.mob`](../../../areas/mob/plane_water_one.mob),
[`plane_water_one.obj`](../../../areas/obj/plane_water_one.obj), and
[`plane_water_one.zon`](../../../areas/zon/plane_water_one.zon). The area has
125 room records, seven mobile prototypes, and six object prototypes. It has
no local QST file or story sidecar.

The current Plane of Water map is separate from the Sea Kingdom (`seakngdm`);
the two have a reciprocal WLD boundary. The older unregistered `waterplane`
map remains on its existing attic candidate record and is not established as
a rename or replacement of this contributor. Its separate low-VNUM source,
story, and route review is in the
[Waterplane candidate audit](../quest-story-followups/WATERPLANE_AUDIT_REVIEW.md).

## Environmental story in the rooms

123 rooms share a description of shifting water currents that make travel
cumbersome and attract native creatures. The source contains no explicit
room-current fields, so this prose does not establish a movement mechanic.
Two rooms break from the repeated setting:

- Room #23206 contains a bright lightning sphere, described as a fragment of
  the Para-Elemental Plane of Lightning.
- Room #23233 is “A Jet Stream,” where the water currents form a fast-moving
  column. Negative Plane object #26658 names it as a destination.

The mobile catalog defines eyes of the deep, water elementals, large water
elementals, marid nobles, tritons, Olhydra the Princess of Water, and an
elemental whale. The zone resets elemental residents through the map and loads
Olhydra in #23250 with the cloak of the manta ray, Orb of the Sea, and black
pearl earring. These descriptions and resets define the local cast and
placement; they do not prove current spawn state or player access.

## Quest ownership and items

The area has no local QST or story-sidecar contract. The active Heavens QST
owns the external fixed item requirements #23202 and #23203: the black pearl
earring and cloak of the manta ray. Those items are associated with Olhydra
by her zone reset. Nearby Heavens prose asks for a “relic with regenerative
powers” and “the necklace of the one who sits on a throne of water,” then
describes handing over Poseidon's prized necklace. The source object names
for the two fixed requirements are an earring and a cloak. Keep the item IDs
and story-stage relationship as an owner question; the source does not explain
that naming gap or confirm how a player obtains each item.

The `surfacemini.story.json` contact record calls Olhydra a water-planar
essence source but defines no topic or local quest outcome. Compiled
`olhydra_shout` behavior can call water helpers to attack the target. Ship
quest initialization also searches for Olhydra as a possible holder of a
moonstone fragment; those cross-system uses are detailed in the
[audit review](../quest-story-followups/PLANE_OF_WATER_AUDIT_REVIEW.md#typed-dependencies-and-external-consumers).

## Title, history, and retention

After color-code removal and whitespace normalization, “Plane of Water” is
unique among all 442 current `.zon` titles and among the 351 active area
contributors. Followed WLD, MOB, OBJ, and ZON histories remain on the
`plane_water_one` stem through the 2009 move into the current `areas/` tree
and later same-stem edits. The Orb of the Sea received a proc/flag update in
2015 and a gear-stat adjustment in 2026; neither changed its owner or area
identity. No alternate-stem rename, distinct current predecessor, or whole
zone replacement is established by these paths. See the [audit review](../quest-story-followups/PLANE_OF_WATER_AUDIT_REVIEW.md#title-followed-history-and-retention).

Keep active `plane_water_one` **DO NOT MOVE**. Preserve its 125-room map,
the reciprocal Sea Kingdom route, Heavens' fixed item VNUMs, Olhydra's
callback and ship-quest consumer, the Orb of the Sea effect, the damaging
Ethereal pool, and all external destinations. No route, reset, gameplay, or
migration change is proposed.
