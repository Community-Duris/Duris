# Arachdrathos Guilds (`aracguil`) source story dossier

This dossier maps the authored story across the six Drow houses on Arachdra's
web. The [Checkpoint 185 audit supplement](../quest-story-followups/ARACGUIL_AUDIT_REVIEW.md)
records the complete active graph, switch targets, reset and compiled
dependencies, title review, and bounded source history.

## Six houses on Arachdra's web

The guild area depicts six competing houses, each with its own stronghold,
political aim, and violent means. Seven active doorways lead between their
strongholds and the neighboring Arac-Web district; the audit supplement maps
all seven reciprocal pairs.

| House | Authored role and conflict | Room and mobile evidence |
| --- | --- | --- |
| Zarbonesti | The dominant house is built into a blood-streaked arch. Its slavers sell people and vendetta contracts; its assassins prepare raids against the overworld and rival underworld communities. The arch cleric can transform Drow males into Zarbon. | [Business office](../../../areas/wld/aracguil.wld#L230), [Chamber of Abomination](../../../areas/wld/aracguil.wld#L401), [arch cleric](../../../areas/mob/aracguil.mob#L131), [Zarbon](../../../areas/mob/aracguil.mob#L147), [agent](../../../areas/mob/aracguil.mob#L228) |
| Olipth | A mage tower houses apprentices, a testing laboratory, archives, and an eight-member high council. Its scribe has spent a lifetime serving the house. | [Tower entrance](../../../areas/wld/aracguil.wld#L425), [archives](../../../areas/wld/aracguil.wld#L555), [scribe](../../../areas/mob/aracguil.mob#L296) |
| Zochra | A fortified keep centers on warriors and its warmaster. Blood marks the stairs; a chamber beyond the upper platform contains a celestial soldier. | [Keep entrance](../../../areas/wld/aracguil.wld#L621), [upper stairs](../../../areas/wld/aracguil.wld#L761), [warmaster](../../../areas/mob/aracguil.mob#L407), [celestial soldier room](../../../areas/wld/aracguil.wld#L783) |
| Ilith | Its halls are organized around shrines to Lolth. The high cleric says her mission is to raise Ilith to rulership after Zarbonesti falls. | [House entrance](../../../areas/wld/aracguil.wld#L814), [Shrine of Lolth](../../../areas/wld/aracguil.wld#L995), [high cleric](../../../areas/mob/aracguil.mob#L451) |
| Slin | Assassins use the house's recreation room for the slaughter of captured Drow children. Their master keeps trophies, and the observation deck is described as a site for hunting Zarbon. | [Recreation room](../../../areas/wld/aracguil.wld#L1105), [observation deck](../../../areas/wld/aracguil.wld#L1161), [master assassin](../../../areas/mob/aracguil.mob#L564) |
| Sa'Zarn | Necromancers keep failed creations in a dungeon and serve the lich Zargenenon. His throne room contains the area's account of Arachdrathos' history. | [House atrium](../../../areas/wld/aracguil.wld#L1207), [Dungeon of Abominations](../../../areas/wld/aracguil.wld#L1294), [Zargenenon's throne](../../../areas/wld/aracguil.wld#L1372), [Zargenenon](../../../areas/mob/aracguil.mob#L717) |

The writing gives the city's history a clear point of view. Zargenenon's mural
describes an alliance led by the necromancer and the cleric Delhira against
Llzazan, the destruction of House Hirrogaganth, the creation of the Zarbon,
and the conquest of Sa'Zarn. After Delhira dies, the houses turn on one
another. The mural says Lolth placed two mystical switches in each house
except Zarbonesti to keep the web balanced. It says the Jewel of the Spider
Queen was hidden in Zarbonesti's lost shrine so the power to command the
Zarbon would remain there until a Drow united the houses
([mural](../../../areas/wld/aracguil.wld#L1372)).

## The hidden shrine, switches, and the Jewel

Zarbonesti's lost shrine stands behind the linked passage sequence. A
Mentenozan shade guards it; the local reset gives the shade both a
rune-covered mana stone from [Heavens object #359](../../../areas/obj/heavens.obj#L3725)
and the local [Jewel of the Spider Queen, object #36760](../../../areas/obj/aracguil.obj#L792)
([shade and item resets](../../../areas/zon/aracguil.zon#L237)).
The Jewel's description calls it a dark globe attached to a rod. A separate
Lolth pendant records the arch cleric's discovery of the Zarbon transformation
([pendant #36714](../../../areas/obj/aracguil.obj#L206)).

The ten placed switches match the mural's count: two reset in each of the five
lesser houses. They target blocked passages between house chambers and through
a chain of gleaming tunnels to the lost shrine. The [audit review](../quest-story-followups/ARACGUIL_AUDIT_REVIEW.md#the-ten-reset-switches)
lists every switch, reset room, target exit, and initial door state. Their
native `ITEM_SWITCH` definitions and generic handler support a switch-driven
passage network in the source; runtime traversal and puzzle usability have not
been qualified.

The item's compiled callback introduces a question about the artifact's
identity and effect. Object #36760 is named the Jewel, but its callback is
`rod_of_zarbon`; the function can also try to create missing object #36761 as a
curse item ([callback](../../../src/specs/specs.aracguil.c#L16),
[binding](../../../src/specs/specs.assign.c#L1711)). The source scan found no
object prototype #36761. The audit review records the full conditions and
recommended follow-up without assuming a live result.

## Story and builder follow-ups

The active area has no local `.qst` file or native Q/QA contracts, no SHP file,
and no story sidecar. The authored guild rivalries, the Jewel, and the switches
have no local recorded player outcome. These are questions for the area owner:

| Question | Source evidence | Suggested resolution |
| --- | --- | --- |
| Is the city's balance-of-power conflict intended to be player-facing? | Zargenenon's mural describes the wars, the switches, and the Jewel; Ilith's high cleric states her house's ambition. | Decide whether players should choose or alter a house's position before adding a quest or persistent outcome. |
| What is the intended scope of the ten-switch network? | Five paired house placements point to ten blocked local exits, including the route to the lost shrine. | Preserve the pairings and reset states; qualify the intended order and access in a separate authorized gameplay review. |
| Is the lost shrine meant to resolve the Zarbon source story? | The mural says the Jewel preserves Zarbonesti's power until one Drow unites the houses, and a shade carries the Jewel. | Define who may wield the Jewel and what uniting the houses means before adding a completion contract. |
| Is the Heavens mana stone deliberately assigned to the shrine's shade? | `aracguil.zon` gives external object #359 from `heavens.obj` to the same shade that receives the Jewel. | Confirm the cross-area reset is intentional and keep the import visible in future ownership or migration review. |
| Is the `adamantine lever` prototype intended to open Zochra's gates? | Object #36740 targets an already unblocked gate exit and has no local ZON reset placement. | Confirm its intended placement and target before treating it as a working route. |
| Should `aracguil` have any command-target alias beyond its normalized title “Arachdrathos Guilds”? | The title is unique among 442 scanned `.zon` files; Arachdrathos – Drow City, Arachdrathos Wilderness, and Llzazan Ghetto are distinct neighboring titles. The bounded source review does not establish another Guilds alias. | Keep `aracguil` and “Arachdrathos Guilds” as the source key and display identity. Add an alias only after the owner names its exact target and intended distinction. |

Keep `aracguil` in the active area list. Its seven reciprocal city entrances,
switch network, imported reset item, and compiled callbacks remain source
dependencies; the audit supplement records their migration disposition.
