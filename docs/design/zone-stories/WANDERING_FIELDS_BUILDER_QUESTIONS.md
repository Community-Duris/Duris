# The Wandering Fields: builder questions

## Explicit builder questions and recommended resolutions

These questions are tied to the checked-in WLD, MOB, OBJ, ZON, and shared NPC
movement sources. Keep active ownership and typed references intact until the
area owner resolves the design questions.

| # | Source-backed builder question | Recommended resolution |
| --- | --- | --- |
| 1 | Is #36600 still intended as a mobile-loading room used for Llzazan, as its description says? See [the root room](../../../areas/wld/wander.wld#L1). | Preserve its staging role and area owner. Do not present it as an authored player hub without an intended entrance. |
| 2 | Are the six selector rooms #36601–#36606 still the intended destinations for Rothe, mushrooms, the slave camp, fungus farms, and the lower and upper ghetto? See [the destination descriptions](../../../areas/wld/wander.wld#L31). | Keep the six labels and their Llzazan exit groups together until the owner confirms a deliberate map change. |
| 3 | Is it intentional for both south and down from Mushroom Grove #36602 to target Llzazan room #36267? See [the two exits](../../../areas/wld/wander.wld#L61). | Confirm whether the duplicate is a weighted choice or an accidental duplicate. Preserve it until that choice is explicit. |
| 4 | Are the 36 one-way exits from Wander to Llzazan intended for wandering mobiles, ordinary players, or both? See [exit rows 493–528](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L493). | Preserve the authored exits. Review player entry and return independently from the NPC movement path. |
| 5 | Should any route back from Llzazan into the Wander staging rooms exist? No active WLD source currently targets #36600–#36613. | Confirm the intended boundary and entry mechanism before adding a reverse edge or describing player access. |
| 6 | Is the branch from #36607 intended to give a 50% load chance through the loader #36600 or the terminal room #36608? See [the branch](../../../areas/wld/wander.wld#L211). | Keep the two exits and label together while the reset and mobile-movement owner confirms the intended chance. |
| 7 | Are the 25% and 12.5% labels on #36609 and #36610 still correct for their respective branches? See [the chain](../../../areas/wld/wander.wld#L231). | Preserve the chain and labels until the owner verifies the intended odds against reset placement and mobile movement. |
| 8 | Should #36611 keep its 6.25% title when its room text says 6.5%? See [#36611](../../../areas/wld/wander.wld#L259). | Resolve the disagreement with the area owner, then update the title and prose together. Do not choose a value from inference alone. |
| 9 | Should #36612, titled 3.125%, have explanatory chance text like the other probability rooms? See [#36612](../../../areas/wld/wander.wld#L273). | Confirm whether the missing text is intentional and retain the current numeric title until then. |
| 10 | Does #36613 intentionally say 1.0625%, even though halving 3.125% would yield 1.5625%? See [#36613](../../../areas/wld/wander.wld#L287). | Ask the owner to verify the intended value and update only after confirmation. Do not silently substitute the mathematically expected value. |
| 11 | Is #36608 meant to be a permanent exitless sink for mobiles? Its description says mobs there will not make it into the game that boot. | Keep the room and its terminal graph role until the owner confirms whether a reset or runtime cleanup also belongs there. |
| 12 | Are the current direct M resets to #36607, #36609, #36610, and #36612 intended to work with the whole probability chain? See [the reset groups](../../../areas/zon/wander.zon#L103). | Preserve reset mode 2 and all current placements. Qualify the full reset/movement cycle before altering probability rooms. |
| 13 | Should the two Arachdrathos chasm demons #36011 remain reset at Wander #36600? See [the two M records](../../../areas/zon/wander.zon#L26) and [prototype owner](../../../areas/mob/arac_wild.mob#L168). | Preserve Arachdrathos as the mobile owner and keep both active cross-area reset references until both area owners confirm otherwise. |
| 14 | Are the 12 G resets of Llzazan objects #36204 and #36205 intended to give rothe meat to Wander's rothe group? See [the reset group](../../../areas/zon/wander.zon#L28), [#36204](../../../areas/obj/llzazan.obj#L57), and [#36205](../../../areas/obj/llzazan.obj#L70). | Preserve Llzazan as the object owner and verify whether both meat prototypes and their current recipients are intended. |
| 15 | Should the zone continue to equip the local barbed scimitar #36600 on the Zarbon group? See [the item](../../../areas/obj/wander.obj#L1) and [equipment resets](../../../areas/zon/wander.zon#L107). | Keep the item and E/G reset assignments attached to Wander until the owner confirms custody and replenishment behavior. |
| 16 | Are the slave, rebel, Drow commoner, exiled-cleric, acrola, and Zarbon descriptions intended to serve only as Llzazan environmental setting? See [representative mobile text](../../../areas/mob/wander.mob#L99). | Preserve the source themes and avoid inventing a quest, rescue outcome, or guaranteed item reward from the descriptions alone. |
| 17 | Is the wander entry in [areas/qst/AREA](../../../areas/qst/AREA#L163) intentional while wander.qst is absent? | Keep index membership distinct from a local quest definition. Do not add or remove a QST file or index entry without confirming the loader and owner intent. |
| 18 | Should Wander remain without a local shop and story sidecar despite its mobile and probability-room text? | Preserve the current file inventory. Add a contract or sidecar only with explicit goals, proof, reset semantics, and an owner-approved design. |
| 19 | Should the six unregistered ud_map0.zon M resets continue to reference Wander mobiles #36601, #36605, and #36626? See [source-only reset records](../../../areas/zon/ud_map0.zon#L10). | Preserve all six references under the existing ud_map0 candidate review. They are source-only dependencies and do not demonstrate current placements. |
| 20 | Should the six selector-room exits remain unflagged and without key VNUMs, given their shared mobile and possible player route role? | Keep the current exit flags and keys. Ask the owner to decide whether the links are deliberately public before changing access or adding a return route. |

This review records questions only. It does not authorize source edits, movement
changes, probability corrections, or an attic migration.
