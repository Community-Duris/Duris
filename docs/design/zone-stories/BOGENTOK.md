# BogenTok (`bogentok`) source story dossier

This dossier reads BogenTok's active room, mobile, object, shop, and reset
text as a static story. The [Checkpoint 195 audit supplement](../quest-story-followups/BOGENTOK_AUDIT_REVIEW.md)
reconciles the map, boundaries, reset doors, typed dependencies, code bindings,
and bounded title/history evidence. It does not qualify live shop stock,
occupants, quest availability, player access, or safe travel.

## A shared Underdark city with a martial center

Both gate descriptions call BogenTok a common Underdark city governed
primarily by figureheads of its key races. They describe adamantite gates made
in Khildarak and give the city a deliberately hostile boast about destroying
the “good” races ([east gate](../../../areas/wld/bogentok.wld#L1),
[west gate](../../../areas/wld/bogentok.wld#L355)). That public image sits
beside the welcome offered to travelers through plentiful beer and commerce.
The text does not explain whether the shared government represents civic
cooperation, military necessity, or a fragile alliance.

The street names carry a second founding story. Titanor is described as a
fabled ogre hero who is never seen, may live among the noble houses, and may
help conduct army training. At the main intersection, troop movements and
training lie to the south, while upper- and middle-class homes and commerce
lie to the north ([Titanor Street](../../../areas/wld/bogentok.wld#L29),
[intersection](../../../areas/wld/bogentok.wld#L175)). Tvlar Way is the
north-south route; Titanor Street runs east-west. These repeated landmarks
make the city's class and military division legible, although the uncertain
wording around Titanor leaves the founder's role intentionally—or
accidentally—mythic.

The central fountain is described as a citywide water source. Its room text
connects the crowded lower-class apartments to the inn and execution block,
placing ordinary daily use next to state violence
([fountain](../../../areas/wld/bogentok.wld#L407)). Quiet barracks,
small homes, back alleys, outer-wall streets, markets, and the northern
residential blocks fill out that public geography. The prose repeatedly
distinguishes poorer residents and muddy alleys from well-kept streets,
nobles, and upper-class viewing areas.

## Hospitality, coercion, and social status

BogenTok's central public spaces are not neutral. The Community WhoreHouse
has a common room, private rooms, and an office; its description presents the
business as a draw for Underdark travelers
([house](../../../areas/wld/bogentok.wld#L582)). The zone also contains the
Strangled Paladin Inn. Its name comes from a stated practice of recovering
mortally wounded paladins from battle and torturing them on display; its bar
serves imported Khildarak stout and Drow wine, while a separate lounge caters
to shady visitors ([inn](../../../areas/wld/bogentok.wld#L671),
[bar](../../../areas/wld/bogentok.wld#L690)). The public execution block is
for city traitors, with lower-status residents watching from the muddy ground
and Drow nobles watching from balconies and windows
([execution block](../../../areas/wld/bogentok.wld#L747)).

The contrast between a multi-race public welcome and conspicuous torture,
executions, and sexual commerce is a major part of the zone's tone. The
source does not frame these practices as contested, explain who can be
targeted, or say how ordinary residents regard them. The dossier records the
adult and coercive themes without treating them as an endorsement or adding
detail beyond the source.

## Army, city defenses, and bloodmail

The southern district is an army training ground for the city's evil races.
Its entrance text says that goblins, ogres, and other races train together
without discrimination on that basis
([training entrance](../../../areas/wld/bogentok.wld#L994)). The general's
quarters present a celebrated duergar war hero who has fought the “good” races
and collected weapons and body parts as trophies
([quarters](../../../areas/wld/bogentok.wld#L1686)). The outer wall is carved
from durithalite and is said to protect the city from purple worms and
unwelcome rogues ([wall](../../../areas/wld/bogentok.wld#L1730)).

The Bloodmail Factory joins military supply to the city's industrial story.
Room text describes molten adamantite mixed with blood from Drow, duergar,
trolls, dragons, and other magical beings; the resulting armor is said to
retain properties of its source material. Drow workers handle blood while
duergar handle the molten metal ([factory](../../../areas/wld/bogentok.wld#L1241),
[processing room](../../../areas/wld/bogentok.wld#L1458),
[platform](../../../areas/wld/bogentok.wld#L1482)). A duergar taskmaster and
Drow priestess oversee the process. The source does not say whether the
blood is obtained from willing donors, slain enemies, or some other supply.

## Residents, trade, and source-declared populations

The active source defines 66 mobile prototypes and 46 object prototypes.
The roster includes city guards, soldiers, sergeants, nobles, children,
workers, merchants, travelers, inn and shop staff, steeders, and the army's
general. ZON placement records include a general and following priestess, a
public executioner, guards, barracks residents, and factory workers
([mobile prototypes](../../../areas/mob/bogentok.mob#L1),
[reset declarations](../../../areas/zon/bogentok.zon#L298)). These are
source-declared reset roles, not proof of current occupants. The `F` records
are followers, not fall markers.

Three local shops are present. The Tvlar Way bartender's shop stocks beer and
whiskey; the general store carries dwarf flesh, a waterskin, and a gray
potion; the Bloodmail Factory duergar shopkeeper sells bloodmail pieces
([bar shop](../../../areas/shp/bogentok.shp#L1),
[general store](../../../areas/shp/bogentok.shp#L36),
[factory shop](../../../areas/shp/bogentok.shp#L81)). Their keepers are
placed by local resets in the bar, store, and factory
([bar reset](../../../areas/zon/bogentok.zon#L372),
[store reset](../../../areas/zon/bogentok.zon#L409),
[factory reset](../../../areas/zon/bogentok.zon#L482)). The store's room
description says its items vary, while its `.shp` record names a fixed set of
source products; live inventory was not checked.

## Builder questions and follow-up

- **What social order should the city communicate?** Clarify how the
  figurehead government, Titanor's founder myth, Drow noble status, lower-class
  neighborhoods, shared army training, and racial slogans fit together.
  Align the street, market, home, and barracks descriptions with that model.
- **What audience and tone are intended?** Confirm how the zone should present
  prostitution, torture of captured paladins, public executions, and the
  treatment of the factory's blood supply. These are prominent adult and
  coercive themes in public-facing descriptions.
- **What is the Bloodmail Factory's intended practice?** Specify whether its
  blood is taken from willing donors, victims, defeated enemies, or another
  source, and make the factory's labor and supply story consistent with that
  answer.
- **What does the bartender's quest interaction do here?** Mobile #18707 is
  bound to the generic `world_quest` handler, which accepts quest-management
  commands. BogenTok has no same-stem `.qst` or native Q/QA contract, so
  establish whether the generic service is intentional and where its quest
  content is defined before describing a local quest.
- **Is the Betrayal sword meant to have special behavior?** Object #18744 is
  [the general's unholy sword](../../../areas/obj/bogentok.obj#L507), while its
  `betrayal` callback assignment is commented out in current code. Confirm
  whether that callback was retired or whether behavior is missing.
- **How should the closed doors shape navigation?** All 72 WLD door exits have
  reset declarations that close them. Confirm that the resulting district
  access is intended and that room clues explain how visitors open ordinary
  doors. Static graph results do not establish actual access or a live route.
- **Do the shop descriptions match their configured products?** Reconcile the
  general store's variable-stock prose with its three fixed products and
  confirm the intended bartender and factory inventories at runtime.
- **Should `bogentok` have a command-target alias beyond “BogenTok”?** The
  normalized title is unique in the current scan, and the bounded area sources
  do not establish another command alias. Keep the source key and display
  title; add an alias only after the owner names its exact target and intended
  distinction.

The zone is an active Underdark city and should remain in its current source
location. No gameplay, text, reset, shop, or code behavior was changed during
this source review.
