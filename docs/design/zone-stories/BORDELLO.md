# Vella's Bordello (`bordello`) source story dossier

This dossier reads the active room, mobile, object, shop, and reset text as a
static story. The [Checkpoint 196 audit supplement](../quest-story-followups/BORDELLO_AUDIT_REVIEW.md)
reconciles the map, boundaries, reset doors, typed dependencies, callbacks,
and bounded title/history evidence. It does not qualify live occupants, shop
stock, quest availability, player access, or safe travel.

## A desert road and a popular mansion

Two stone-path rooms cross a desert toward an old mansion, with broken wagon
pieces, frequent tracks, and customer carts outside
([approach](../../../areas/wld/bordello.wld#L1),
[mansion exterior](../../../areas/wld/bordello.wld#L41)). The entry hall uses
velvet seating, potted plants, a mesh carpet, and hooded lanterns. Inside,
hallways and private rooms repeat the same visual language: velvet, pillows,
mirrors, exotic pictures, drapes, towels, and a pair of fountains
([entry](../../../areas/wld/bordello.wld#L60),
[great fountain](../../../areas/wld/bordello.wld#L219),
[upper fountain room](../../../areas/wld/bordello.wld#L356)). The upper room
has a hole in its floor through which the fountain rises; its room data also
declares a fall chance of 2 toward the fountain below.

Vella's room is the most ornate private space: worn pillows, mirrors, erotic
art, toys, and food make it a personal suite rather than an ordinary bedroom
([room](../../../areas/wld/bordello.wld#L909)). A beauty parlor shows workers
preparing with mirrors, makeup, and garments
([room](../../../areas/wld/bordello.wld#L1260)). The zone's recurring prose
frames the house around paid adult services, but does not establish how the
workers define their own terms or how visitors are expected to treat them.

## A hidden account of killings

The Main Hallway leads through a door to a room titled “The Treasure Room.”
Its description says the treasure belongs to men killed by the people of the
Bordello and that the room is hidden among the other rooms
([hallway exit](../../../areas/wld/bordello.wld#L690),
[treasure room](../../../areas/wld/bordello.wld#L1287)). It never identifies
who committed the killings, why they happened, or whether Vella or any named
worker is involved. The hoard therefore introduces a murder story that the
rest of the zone does not resolve.

The hallway and treasure room have reciprocal door exits whose raw flags are
door plus locked. Their ZON reset commands set state 0 and call both exits
“open” ([door resets](../../../areas/zon/bordello.zon#L8)). The text calls the
room well hidden, so builders should confirm that the open reset state and
available clues tell the intended story. This source reading does not prove
the current in-game door state.

## Residents, services, and trade

Sixteen mobile prototypes describe Vella, workers, a bouncer, merchants,
travelers, and customers; ZON records place Vella in her room and Vegar in the
shop room ([mobile prototypes](../../../areas/mob/bordello.mob#L1),
[reset declarations](../../../areas/zon/bordello.zon#L31)). Three worker
callbacks handle a 500-copper payment and then offer to follow the customer.
That is code-backed behavior, separate from the zone's lack of a local quest
contract; the audit supplement records the callback assignments and source
locations.

One customer prototype is described as “probably not even in his twenties,”
then as enjoying an appointment with the house's workers
([mobile text](../../../areas/mob/bordello.mob#L190)). In this adult-coded
setting, the intended adult age of this character needs clarification.

Vegar runs one shop in room #93667. Its record lists two products, local
objects #93610 and #93611; the room describes shelves of toys, and the shop
record contains a race-based refusal message
([shop](../../../areas/shp/bordello.shp#L1),
[shop room](../../../areas/wld/bordello.wld#L1273)). The source record does
not establish live stock or whether the refusal rule remains intended.

## Builder questions and follow-up

- **What is the house's intended story and tone?** Confirm how its workers,
  customers, and proprietor relate to the implied paid services, the concealed
  treasure, and the statement that men were killed. Add clues or context where
  needed, and keep the story consistent across room and mobile text.
- **Is the customer meant to be an adult?** The mobile text explicitly makes
  his age uncertain in a sexualized setting. Establish adult intent in the
  character description or revise the placement and text to remove that
  ambiguity.
- **How should the hidden room be found?** Decide whether the reciprocal
  passage should reset open, what clue reveals it, and whether the treasure's
  murder story is meant to implicate any named character.
- **What object should the four bedroom resets create?** Bordello resets
  object #45 into four rooms, while the only definition in the active object
  manifest is Heavens' slot machine. Confirm the intended prototype before
  changing either source.
- **Are the worker interactions and shop rules intended?** Confirm the
  500-copper follow behavior, the shop's two listed products, and its
  race-based refusal message; make the text explain the offered service and
  outcome clearly.
- **Should `bordello` have a command-target alias beyond “Vella's Bordello”?**
  The normalized title is unique in the current scan, and the bounded area
  sources do not establish another command alias. Keep the source key and
  display title; add an alias only after the owner names its exact target and
  intended distinction.

The active Bordello source remains in place. No gameplay text, reset, shop,
placement, or code behavior was changed during this source review.
