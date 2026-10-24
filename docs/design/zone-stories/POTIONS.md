# Potions: The Potion Treasure Vault

The active `potions` contributor is zone 307, titled “The Potion Treasure
Vault.” It owns 15 rooms (#30700–#30714) and two mobile prototypes. Its
checked-in `potions.obj` file is empty, so the contributor defines no local
object prototypes. The source stem is present in [`areas/AREA`](../../../areas/AREA#L149)
and [`areas/qst/AREA`](../../../areas/qst/AREA#L138); the title and reset
header are in the [zone file](../../../areas/zon/potions.zon#L1).

## Story and quest boundary

The rooms are split into two named vault clusters. The first starts at
“Brass Vault Start Room,” continues through numbered potion rooms, and reaches
rooms titled “First Wander Room - Brass” and “2nd 52 Potion Room - Brass.” The
second begins at “Ceothia Guard Start Room” and contains another set of
numbered potion rooms and a “First Wander Room - Ceothia.” Fourteen of the 15
WLD room descriptions are empty; the only non-empty description is the word
“First” in room #30705. Most of the setting is therefore carried by room and
mobile names rather than environmental prose ([Brass rooms](../../../areas/wld/potions.wld#L1),
[Ceothia rooms](../../../areas/wld/potions.wld#L201)).

The two local guards provide the clearest story. The Ceothian vault guard is
reset into the Ceothia start room. A second mobile is titled “a vault
guardian” and described as the Sultan's guardian, aware of the actions that
led to the visitor's arrival. Its ZON reset places it in the Brass start
room, and the reset comment calls it the Brass guardian. The source does not
explain how the Sultan relates to the Brass vault or whether that placement
is deliberate ([mobile descriptions](../../../areas/mob/potions.mob#L1),
[guard resets](../../../areas/zon/potions.zon#L26)).

The two clusters point to different named vault settings. Six Brass-branch
exits target #25087, a room titled “Treasure Room of the Efreeti” in the
unregistered `brass-old-2.wld` file. Those exits do not resolve in the active
world list. Seven exits from the Ceothia branch lead to active Ceothia room
#81094, “The Vault Room of Lord Lenbrea.” No local text explains the
relationship among the Sultan, the efreeti ruling family, Lord Lenbrea, or
the two branches ([Brass exit records](../../../areas/wld/potions.wld#L146),
[Efreeti room](../../../areas/wld/brass-old-2.wld#L2078),
[Ceothia exit records](../../../areas/wld/potions.wld#L271),
[Lord Lenbrea's vault](../../../areas/wld/ceothia.wld#L5926)).

There is no local QST, SHP, or story sidecar, and no native Q/QA contract.
QST `AREA` membership is present, but does not indicate a local quest file.
The ZON has only two active mobile resets and no active object reset. Its
object, equipment, monolith, and Negative Plane stalker examples are
commented out; they do not describe stock available from the current reset
file. The repository-wide [audit review](../quest-story-followups/POTIONS_AUDIT_REVIEW.md)
records those disabled lines, the active routes, and the builder decisions.

## Owner questions

- Is this contributor intended as two connected vault excursions, a stock
  source, or a location that is currently only partially authored?
- Is the guardian of the Sultan meant to be placed at the Brass start room?
- Are the six Brass exits to the old Efreeti treasure room still intended?
- Should the Ceothia vault route have a WLD return path?
- Should the numbered potion rooms have descriptions, reset stock, or an
  explicit quest purpose?
- Which of the commented object and equipment loads, if any, are still
  intended to be active?

## Current working resolution

Keep active `potions` **DO NOT MOVE**. Preserve its room and mobile VNUMs, two
active guard resets, seven resolved one-way Ceothia exits, and six unresolved
Brass exits while their owners clarify the intended vault routes and contents.
Keep `brass-old-2` on its existing **RETAIN IN PLACE** candidate plan with its
six Potions references recorded separately from the one Realm reference. No
gameplay, reset, route, or migration change is proposed.
