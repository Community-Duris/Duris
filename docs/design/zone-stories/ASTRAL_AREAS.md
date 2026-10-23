# Astral Plane, Side Areas source story dossier

## What the source supports

`astral_areas` is active zone `198`, titled **Astral Plane, Side Areas**. Its
61 rooms cover #19826–#19886. The room text moves from an astral gate and a
quiet sanctuary, through an isolated Githyanki fortress, to floating spheres
of air and rock. The fortress holds military yards, a tower, barracks, a
library, a prison, a crypt, a chief's rooms, and an alchemy lab. The sphere
cluster contains a phoenix nesting chamber. Two final rooms are explicitly
labeled load and failed-load rooms
([area entry](../../../areas/AREA#L102),
[zone header and resets](../../../areas/zon/astral_areas.zon#L1),
[room source](../../../areas/wld/astral_areas.wld#L1)).

The local `.mob` and `.obj` files are empty. The zone's reset file instead
places mobiles and objects owned by Astral Main, Astral Tiamat, and Heavens.
There is no local QST, shop, or story sidecar; `astral_areas` appears in the
historical `areas/qst/AREA` index, but no same-stem quest file is present
([quest index](../../../areas/qst/AREA#L232)). The reset-imported props and
creatures are part of the authored setting, not local prototype declarations.

## The gate and sanctuary

Room #19826 opens onto a shifting astral rift. A black rune-covered gate above
leads to the peaceful sanctuary at #19827. Its text presents the room as a
resting place for astral travelers, with swirling lights, a statue, and a
fountain. Both gate directions start closed and locked; their WLD records use
key #19870. The ZON file places that Astral Key on an Astral Guardian in the
rift room ([rift and warning](../../../areas/wld/astral_areas.wld#L1),
[sanctuary](../../../areas/wld/astral_areas.wld#L32),
[gate resets](../../../areas/zon/astral_areas.zon#L8),
[guardian and key resets](../../../areas/zon/astral_areas.zon#L99)).

## The Githyanki fortress

The next group begins at #19828 on a smooth black landmass suspended in the
Astral Plane. A guarded stone fortress dominates the plateau. The route enters
through keyed gates into a courtyard centered on a Githyanki statue, then
branches into the tower, military quarters, library, prison, crypt, chief's
rooms, and laboratory. Text about old battle damage, unused beds, bodies,
weapon practice, alchemy, and a well-furnished leader's chamber gives the
stronghold an occupied but battered past
([landmass](../../../areas/wld/astral_areas.wld#L60),
[fortress gates](../../../areas/wld/astral_areas.wld#L189),
[courtyard](../../../areas/wld/astral_areas.wld#L371),
[library](../../../areas/wld/astral_areas.wld#L426),
[barracks and practice rooms](../../../areas/wld/astral_areas.wld#L533),
[leader's rooms](../../../areas/wld/astral_areas.wld#L632),
[crypt and burial chamber](../../../areas/wld/astral_areas.wld#L702),
[laboratory](../../../areas/wld/astral_areas.wld#L795)).

The ZON resets furnish the fortress with Githyanki warriors, warlocks,
knights, a Supreme Leader and Chief Knight, as well as Astral Tiamat's High
Mage. Imported items include practice weapons, armor, keys, books, a coffin,
alchemical supplies, and a rune-covered pool from Heavens. A second active
area, `githyanki_fortress`, now holds a separate Githyanki stronghold. Its
2010 creation commit calls it a replacement for an “old gith fort”; this
39-room fortress segment is a plausible candidate for that phrase, but the
commit does not name `astral_areas`, and both sources remain active. See the
[separate fortress review](../quest-story-followups/GITHYANKI_FORTRESS_STORY_REVIEW.md)
and [replacement history](../quest-story-followups/DUPLICATE_AND_REPLACED_ZONES.md).

## The spheres

Rooms #19867–#19884 describe large floating spheres in the Astral void. The
air sphere is filled with mist, drifting stones, and a flaming bird. A
smoke-filled passage leads into a chamber inside a carved sphere of rock,
where red feathers and eggshells establish a phoenix nest. The zone resets
place phoenixes and their mother there, while several imported rock and key
objects dress the other rooms
([air sphere](../../../areas/wld/astral_areas.wld#L1035),
[sphere rooms and phoenix](../../../areas/wld/astral_areas.wld#L1054),
[rock sphere and chamber](../../../areas/wld/astral_areas.wld#L1355),
[nest](../../../areas/wld/astral_areas.wld#L1458),
[sphere resets](../../../areas/zon/astral_areas.zon#L92)).

## Load rooms and builder decisions

Room #19885 is labeled `XZAZIXXAZZ LOAD ROOM (astral_areas)` and has a
one-way down exit to Astral Main #19806. The zone places the Illithid captain
and two followers there. Room #19886 is labeled `XZAZIXXAZZ FAIL LOAD ROOM`.
These labels and reset placements show source intent for staging; they do
not establish ordinary player access or what causes a failed load
([load room](../../../areas/wld/astral_areas.wld#L1473),
[failure room](../../../areas/wld/astral_areas.wld#L1499),
[staging resets](../../../areas/zon/astral_areas.zon#L149)).

The source leaves five owner decisions open:

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Are rooms #19828–#19866 a retained old fortress, a staging complex for reset-loaded Githyanki, or an intended player destination? | The component has no active WLD inbound. A separate `githyanki_fortress` area exists, and its 2010 creation commit calls it a replacement for an “old gith fort”; that does not establish that this whole component was replaced. | Confirm intended use and arrival before adding an entrance or describing ordinary player access. Keep both areas active and the historical relationship qualified. |
| Are the two fortress pools and their destinations still intended? | The library pool targets active Ethereal room #12479; the second targets #163372 in inactive `Duris3.wld` and fails normal-world room lookup. | Preserve the working active destination. Treat the dormant target as unresolved and do not infer a route or reactivate/move old data from this reference alone. |
| Are the keyed WLD doors meant to begin in different states? | Some key-tagged doors reset closed but unlocked, while four pairs reset closed and locked. | Confirm each intended access/egress state before editing doors or keys; document the route without treating the key flag alone as a puzzle requirement. |
| What role should the load-room/failure-room pair and one-way exits serve? | Room #19885 is a named load room with the Illithid captain and followers, and exits down one-way to Astral Main #19806; #19886 is labeled a fail load room. | Keep these as staging evidence until admission and trigger are established. Do not describe the pair as an ordinary player route. |
| Should `astral_areas` have a command-target alias beyond its normalized title “Astral Plane, Side Areas”? | The normalized title is unique among 442 scanned `.zon` files, and no lexical title candidate was found. | Keep `astral_areas` and “Astral Plane, Side Areas” as the source key and display identity. Add an alias only after the owner names its exact target and intended distinction. |

**Recommended current classification:** keep active `astral_areas` in place
with its Astral Main, Astral Tiamat, and Heavens reset dependencies. Preserve
the separate fortress evidence and record the unresolved entrance and pool
destinations for the area owner. Do not infer a quest outcome or move the
source from its room text alone.
