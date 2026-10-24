# Black Tunnels (`black_tunnels`) source story dossier

This dossier reviews the active area's authored environment, inhabitants,
objects, and route clues. The [Checkpoint 193 audit supplement](../quest-story-followups/BLACK_TUNNELS_AUDIT_REVIEW.md)
records its WLD graph, door resets, outside routes, typed references, and
bounded source history. The review is static; it does not qualify a live visit,
reset state, player access, or safe return.

## Black-stone passages and a drowned cavern

The 82-room map begins at Black Tunnel #10700 and runs through wet black-stone
passages whose descriptions mention dripping water, stalagmites, stalactites,
and small rivulets. Several passages branch around pools and converge on an
underwater route: a small pool descends to a deep pool and submerged tunnel,
then opens into a nine-room cavern with green-tinged water and algae. Its center
contains a nest of sticky strands and a humanoid skeletal hand
([entrance](../../../areas/wld/black_tunnels.wld#L1), [pool](../../../areas/wld/black_tunnels.wld#L402),
[underwater cavern](../../../areas/wld/black_tunnels.wld#L461), [center](../../../areas/wld/black_tunnels.wld#L542)).

An air pocket above the submerged tunnel offers a humid chamber with mushrooms.
The adjoining purple-rock passages are described as rarely traveled and
well-hidden. They connect to the dracolisk's lair, overhead ledges, and a rear
passage whose text carries the lair's smell. The adult aboleth and its young
occupy the underwater cavern; the prose describes them as predators close to
their nest ([air pocket](../../../areas/wld/black_tunnels.wld#L668),
[secret passage](../../../areas/wld/black_tunnels.wld#L686),
[lair](../../../areas/wld/black_tunnels.wld#L831),
[rear passage](../../../areas/wld/black_tunnels.wld#L953),
[aboleth prototypes](../../../areas/mob/black_tunnels.mob#L16)).

The route descriptions include submerged rooms and heights above the lair.
WLD prose also describes vertical movement, water flow, and the fiery cavern's
heat. Those clues describe the environment; movement and hazard behavior are
addressed separately in the audit supplement.

## A fire cavern, a stone pillar, and the bone halls

Three Pit of Fire rooms connect to a much larger red-stone cavern. Its prose
describes extreme heat, a ceiling roughly one hundred feet above the floor, and
a one-person opening in the floor; one shaft description mentions a strange
lightness. Seven nearby rooms approach a stone pillar with spiral slits. The
pillar's base contains fire-themed reliefs, a spiral stair, and an eternal
flame. The source gives the stair no upward WLD exit. An adjoining hot alcove
describes a solid diamond door to the east, while its only authored exit is to
the west ([fire pit](../../../areas/wld/black_tunnels.wld#L90),
[shaft](../../../areas/wld/black_tunnels.wld#L127),
[pillar entrance](../../../areas/wld/black_tunnels.wld#L1335),
[pillar base](../../../areas/wld/black_tunnels.wld#L1463),
[hot alcove](../../../areas/wld/black_tunnels.wld#L1447)).

The fire-side creatures include a salamander guarding the alcove and holding a
diamond key, a fire elemental, and a gaund family. The adult male is territorial;
the female is described as protecting her brood. The area places four gaund
eggs in their nest ([mobiles](../../../areas/mob/black_tunnels.mob#L63),
[gaund family](../../../areas/mob/black_tunnels.mob#L78),
[zone placements](../../../areas/zon/black_tunnels.zon#L100)).

The eastern branch ends in a Room of Bones, an expansive white hall, and a
small tomb. The halls describe animal and humanoid bones set into the rock and
a trail of black bones. The tomb contains a carved stone coffin with ruby eyes
and dusty niches; the source places an angry spectre and a sparkling diamond
necklace there. A hulking skeletal guard and skeletons occupy the white hall.
At #10781 the description names a solid white door to the west, with no west
exit authored ([Room of Bones](../../../areas/wld/black_tunnels.wld#L1492),
[small tomb](../../../areas/wld/black_tunnels.wld#L1532),
[white-door room](../../../areas/wld/black_tunnels.wld#L1598),
[skeletal guard](../../../areas/mob/black_tunnels.mob#L207),
[necklace](../../../areas/obj/black_tunnels.obj#L34)).

## Orc patrols, predators, and found objects

The mobile prototypes establish several different pockets: osquips in the
entry passages; adult and young aboleths in the water; a trapped, angry
dracolisk; fire natives; gaunds; an orc leader with hunters and warriors; and
undead in the tomb branch. The prose gives the orcs a territorial patrol role
and describes the dracolisk as imprisoned in rock
([osquip](../../../areas/mob/black_tunnels.mob#L1),
[dracolisk](../../../areas/mob/black_tunnels.mob#L48),
[orc leader](../../../areas/mob/black_tunnels.mob#L165),
[undead](../../../areas/mob/black_tunnels.mob#L192)).

Objects support those scenes. A skeletal arm in the aboleth nest contains a
shiny ring; the dracolisk lair has coins, a scale, and discarded equipment.
The fire and gaund branches have weapons, a cracked lyre, a red gem, eggs, and
the diamond key. The bone hall has a separate set of bone armor and weapons.
These are reset-defined objects, not a local QST reward sequence. The
[skeletal arm and nested ring](../../../areas/obj/black_tunnels.obj#L1),
[diamond key](../../../areas/obj/black_tunnels.obj#L143), and
[zone reset placements](../../../areas/zon/black_tunnels.zon#L49) provide
representative evidence.

## Story boundary and builder questions

The active contributor has no local `.qst` file, no native Q/QA contracts, no
story sidecar, and no shop record. Its authored narrative is in room and
prototype prose, plus reset placement. The source leaves these owner decisions
open; recommended resolutions keep the environment and route declarations
consistent:

- **Diamond door and key:** Does #10712 open the east door described at #10772?
  The salamander receives the key, but #10772 has only a west exit and no WLD
  key field names #10712. If the door is intended as a route, define its exact
  destination, reciprocal path, key behavior, and safe return. If it is
  scenery, align the room text and the key's intended role.
- **White door and pillar stair:** Are the west-facing white door at #10781 and
  the stair ending in the ceiling at #10773 deliberate dead ends? If a route is
  intended, identify its destination and return path; otherwise keep the text
  explicit that these features are sealed or decorative.
- **Secret passages:** Should the closed/secret links at #10711/#10740 and the
  closed/locked/secret links at #10735/#10736 be discoverable by search? Their
  WLD keyword and exit-description strings are empty, and their key fields are
  zero. Confirm which search, pick, bash, or other interaction is intended, and
  set discoverability language to match that owner decision.
- **Khildarak gate:** Is `open gate` at Khildarak #17377 the intended arrival
  action into #10721, and should the ordinary reciprocal WLD exits also work?
  Khildarak's exit text names a gate; #10721 calls the east wall blank, while
  its WLD exit is named `gate`. Align both room descriptions with the chosen
  route and return behavior.
- **Hazard expectations:** Should the underwater branch and the hot vertical
  shafts require breathing, special movement, or fall protection? The prose
  describes those conditions, while the WLD contains no fall-chance or numeric
  current records. Confirm survivability and return expectations for the
  intended access path.
- **Name and command identity:** Should `black_tunnels` have any command-target
  alias beyond its normalized title “Black Tunnels”? The title is unique in the
  current scan, and no separate alias is established by the bounded area
  sources. Keep the source key and display title; add an alias only after the
  owner names its exact target and intended distinction.
