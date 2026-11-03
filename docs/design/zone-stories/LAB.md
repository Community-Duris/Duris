# The Swamp Laboratory of Khul'Lor

The active `lab` contributor is zone #823, titled The Swamp Laboratory of
Khul'Lor. It has 51 rooms, 20 mobile prototypes, and 24 object prototypes.
It appears in both `areas/AREA` and `areas/qst/AREA`, but there is no current
`lab.qst`, shop, story sidecar, or native Q/QA contract. The active
`labyrinth` contributor is a separate zone (#50), titled Labyrinth of No
Return; its story sidecar belongs to that area. See the [area
list](../../../areas/AREA#L295), [zone header](../../../areas/zon/lab.zon#L1),
[QST area index](../../../areas/qst/AREA#L263), and [coverage row
153](../quest-story-followups/ZONE_COVERAGE_REGISTER.csv#L153).

## A swamp under permanent darkness

The entrance is a tall, rusted iron gate in a slime-covered wall. Green mist
flows from within, while the sky stays dark and no living beings are visible
at the gate. The local rooms describe a wide swamp, dead woods with sap
running down the trees, underground tunnels, an ancient mansion, a rope bridge
over a ravine, and a mountain keep. The text offers competing explanations:
one corridor suggests a curse or magical darkness caused by a wizard, while
the pond room says the mist comes from the pond and is not magical. These are
environmental statements, not an established cure or quest objective. See the
[gate, corridor, pond, and woods](../../../areas/wld/lab.wld#L1).

Reset-placed creatures give each part a resident population. Slime-covered
frogs, reptiles, crocodiles, treants, and a swampthing occupy the swamp and
mansion. A note on a frog's necklace says an unnamed writer has turned “prince
Uklor” into a frog and calls him the reader's “beloved champion.” A werewolf
guards the entry. The underground torture chamber holds an evil dragon; the
treasure room has a werewolf; the abandoned mansion has a swampthing, a
guardian ward, and a green-feathered raven. Wolfen occupy the forest and
bridge. Stone, porcelain, and marble golems guard the keep and castle. See the
[local resets](../../../areas/zon/lab.zon#L66), [frog and necklace
text](../../../areas/obj/lab.obj#L1), and [mobile
profiles](../../../areas/mob/lab.mob#L1).

The main stronghold contains a garden, tower, magical library, and the
dwelling of Khul'Lor. The mobile profile describes him as an archmage in
formal robes who built a kingdom; the room around him has warm marble walls,
bookshelves, runes, a fireplace, and a lounge chair. His reset inventory
includes an arcane spellbook, a sphere of negative energy, and the large
stone key for the keep's northern gate. The mob text, rooms, and key fields
place him as the area's central figure. See the [Khul'Lor
profile](../../../areas/mob/lab.mob#L197), [library and
dwelling](../../../areas/wld/lab.wld#L983), and [key
resets](../../../areas/zon/lab.zon#L111).

The name also appears in active `desert` source. Room #49572 describes the
Abandoned Mines of Khul'Lor: it says the mage used the mine to fund a dwarven
kingdom's tower and citadel, which was his residence, and that his spirit
still lingers at the abandoned site. A desert hunter says the mines were
infested by thri-kreen, and token #49135 is named for those mines. The files
do not provide an exact VNUM link to the `lab` prototypes or establish the
relationship between the living archmage here and the lingering spirit
there. See the [mine room](../../../areas/wld/desert.wld#L14358), [hunter
profile](../../../areas/mob/desert.mob#L3061), and [lost
token](../../../areas/obj/desert.obj#L1972).

## Enterable places and keyed doors

Seven locally reset `ITEM_TELEPORT` objects encode the `enter` command and a
local destination. They provide source-authored entrances to the Tree of
Wisdom, a mid-air tunnel, an ancient mansion, and the depths of the pond.
Three pairs of tree and mansion objects point back to their exterior rooms.
These item links join room groups that are disconnected in the ordinary WLD
exit graph; they should be kept distinct from the WLD-only connectivity
counts. The [teleporter objects](../../../areas/obj/lab.obj#L16), [their
reset placements](../../../areas/zon/lab.zon#L49), the `ITEM_TELEPORT`
[destination handler](../../../src/magic/spell_travel.c#L745), and the
`CMD_ENTER` [command value](../../../src/cmd/interp.h#L42) describe the
authored behavior.

The zone resets five paired doors closed. The outer gate at #82300/#82301
starts closed. Other pairs connect the torture chamber to the swamp, a
treasure room, the keep's rune-covered circular chamber, and the porcelain
castle's royal garden. The circular chamber pair starts locked and both WLD
sides name key #82320, which is carried by Khul'Lor. The dragon carries key
#82310 for the dungeon-to-swamp door, and a green golem carries key #82305 for
the treasure room. The gate fields and reset states are recorded in the
[WLD exits](../../../areas/wld/lab.wld#L115) and [door
resets](../../../areas/zon/lab.zon#L8).

The circular chamber reset includes an imported Heavens object #358, a small
rune-covered stone whose description discusses a world-wide network of mana
nodes. The chamber itself describes extensive runes but no other visible
exit. In another local object chain, the positive-energy sphere #82318 is a
closed, locked container keyed to negative-energy sphere #82319; the reset
places marble cube #82322 inside it, while Khul'Lor carries #82319. The
sources establish those item relationships but do not say what the cube or
rune stone is meant to unlock or teach. See the [container and
cube](../../../areas/obj/lab.obj#L249), [chamber and item
resets](../../../areas/zon/lab.zon#L60), and imported [Heavens
stone](../../../areas/obj/heavens.obj#L3709).

## Builder follow-up

- Is the living Khul'Lor in this laboratory intended to be the same mage
  described in the Desert mines, where his spirit is said to linger? If so,
  what is the intended chronology between the two locations?
- Who is prince Uklor, and is the frog's note meant to resolve through a
  quest, another character, or an outside story? The local area has no QST
  file or native Q/QA contract.
- Should the entry gate start closed, and what authored path is intended to
  open it? Active WLD arrivals lead to #82350; the reset-open local exit graph
  reaches only the path and gate rooms.
- Confirm the intended key progression for the three key-bearing door pairs,
  including the locked keep gate keyed to Khul'Lor's stone key.
- Is Heavens object #358 intended as an interactive mana node here? What
  purpose should the locked positive-energy sphere and its marble cube serve?

The [Lab audit supplement](../quest-story-followups/LAB_AUDIT_REVIEW.md#builder-questions-and-recommended-resolutions)
turns these open points into direct builder questions with source-backed
recommended resolutions, including the reset-closed door pairs, disconnected
WLD rooms, and inactive-map approaches.

