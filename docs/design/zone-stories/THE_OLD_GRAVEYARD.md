# The Old Graveyard

This source dossier covers active `grave`: 204 rooms (#8400–#8603), 28 mobile prototypes, and 15 object prototypes. The authored exits, resets, compiled references, title/history review, and inactive-WLD approaches are in the [Grave audit supplement](../quest-story-followups/GRAVE_AUDIT_REVIEW.md).

## A cemetery where the graves open from within

The approach follows an old pebble path to a rusting iron gate. Inside, fog and darkness blur the paths; room text describes graves disturbed from below, with bones and torn clothing scattered around them. A nearby crypt door has been pried open by grave robbers, while other crypt descriptions stress their locked doors and keyholes ([approach](../../../areas/wld/grave.wld#L1), [disturbed graves](../../../areas/wld/grave.wld#L1694), [robbed crypt](../../../areas/wld/grave.wld#L1526), [keyed crypt](../../../areas/wld/grave.wld#L1284)).

Several scenes make the undead disturbance personal. One coffin is nailed shut while someone beats from inside; another is open, with an inscription asking for release from unwanted vigilance ([beating coffin](../../../areas/wld/grave.wld#L2716), [open casket](../../../areas/wld/grave.wld#L2735)). The caretaker’s shed stands nearby, but its keeper is described as crazed, and a duergar grave robber is trapped inside a tomb among the risen dead ([shed](../../../areas/wld/grave.wld#L1820), [caretaker](../../../areas/mob/grave.mob#L343), [trapped robber](../../../areas/mob/grave.mob#L358)).

## The Fryntok family and Ogrebane

A marble mausoleum preserves the Fryntok family’s epitaphs. Kalyb, Alzanir, and Erilya are remembered as Raydyrn’s children; Emily is remembered as a generous wife and neighbor. Raydyrn’s own inscription says an ogre troop slaughtered his family east of the Tharnadian gates. He pursued revenge in the Faang caverns, killed many ogres, then died to a veteran ogre guard during the siege and was brought back for burial ([family epitaphs](../../../areas/wld/grave.wld#L2552), [Raydyrn’s account](../../../areas/wld/grave.wld#L2637)).

The resets give those names an undead presence: young zombies occupy the children’s graves, Emily returns as a zombie, and Raydyrn as a vampire ([family resets](../../../areas/zon/grave.zon#L177), [Emily](../../../areas/mob/grave.mob#L167), [Raydyrn](../../../areas/mob/grave.mob#L182)). Raydyrn is equipped with object #8406, the gold-tipped sword Ogrebane. Its inscription dedicates the blade to his family’s vengeance; its bound special procedure has a one-in-fifteen chance to cast Bigby’s Clenched Fist when Raydyrn’s weapon hits an ogre ([Ogrebane](../../../areas/obj/grave.obj#L83), [reset](../../../areas/zon/grave.zon#L181), [procedure binding](../../../src/specs/specs.assign.c#L1288), [ogre-only proc](../../../src/specs/specs.jot.c#L382)).

## A petrified oak and a necromancer’s laboratory

A dead oak dominates one corner of the graveyard. Its trunk and branches are described as petrified by an unnatural incantation, and gargoyles watch from its high limbs ([tree base](../../../areas/wld/grave.wld#L1656), [petrified trunk](../../../areas/wld/grave.wld#L3697)). A small garden remains alive nearby, standing out as a pocket of flowers untouched by the surrounding necromancy ([garden](../../../areas/wld/grave.wld#L1836)).

Farther inside, the path to the largest tomb is described as the likely center of the disturbance. Magical auras grow stronger on the tomb’s second floor. At #8601, an ancient Drow necromancer works in a laboratory of rituals, books, and reagents; the resets place the necromancer, a wight, and a wraith there ([tomb approach](../../../areas/wld/grave.wld#L3223), [upper crypt](../../../areas/wld/grave.wld#L3542), [laboratory](../../../areas/wld/grave.wld#L4143), [laboratory resets](../../../areas/zon/grave.zon#L246), [necromancer](../../../areas/mob/grave.mob#L420)).

## Builder questions and recommended resolutions

| Question for the area owner | Source evidence | Recommended resolution |
| --- | --- | --- |
| Is a release or rescue story intended for the trapped dead? | One coffin is nailed shut while someone beats from inside, and another inscription asks for rest; no local QST source records an objective or outcome ([coffins](../../../areas/wld/grave.wld#L2716), [QST roster and source boundary](../quest-story-followups/GRAVE_AUDIT_REVIEW.md#source-inventory)). | Keep these as environmental horror until an owner specifies a player action and outcome. Do not add a rescue reward or ending based on the room text alone. |
| Is the necromancer confirmed as the cause of the disturbance? | The largest tomb is described as the likely center; the auras grow stronger and the necromancer is placed in a ritual laboratory ([tomb approach](../../../areas/wld/grave.wld#L3223), [laboratory](../../../areas/wld/grave.wld#L4143)). | Preserve the source's uncertainty. Describe the necromancer as a suspected center of activity unless the area owner confirms a direct causal link. |
| Is Ogrebane meant to be a player-facing reward? | Raydyrn is reset wielding Ogrebane, whose proc checks for ogre hits; the local QST source is absent ([reset](../../../areas/zon/grave.zon#L181), [item procedure](../../../src/specs/specs.jot.c#L382)). | Keep the sword tied to Raydyrn's recorded revenge. Add acquisition or reward expectations only if a separate authored objective supports them. |
| Is the living garden meant to contrast with the petrified oak? | The oak is petrified by an unnatural incantation while the nearby garden is described as untouched by the necromancy ([oak](../../../areas/wld/grave.wld#L1656), [garden](../../../areas/wld/grave.wld#L1836)). | Preserve the contrast and let the area owner decide whether the garden has a separate cause. Do not infer a cleansing, protection, or restoration quest. |

These recommendations retain the documented cemetery story while leaving unrecorded objectives and causal links for the area owner to decide.

## Source boundary

Both `areas/AREA` and `areas/qst/AREA` list Grave. The local QST source, SHP file, story sidecar, and native Q/QA contract are absent. The dossier records the checked-in WLD, MOB, OBJ, and ZON story; it does not establish a local player objective, a way to release the dead, or the result of confronting the necromancer.
