# The Reliquary Nexus of The Dead: comprehensive zone-story mapping

This dossier maps the story and routes expressed by active source stem `nexus_uc`. It records source declarations; it does not certify live resets, access, safety, or player outcomes. The [Nexus UC audit supplement](../quest-story-followups/NEXUS_UC_AUDIT_REVIEW.md) reconciles WLD-only reachability with object teleport routes, external entrances, inactive-map approaches, title history, and typed references.

## Source inventory and present delivery

Active `nexus_uc` owns 85 WLD rooms, #130200–#130284, in zone 1302; 11 mobile prototypes, #130200–#130210; 18 object prototypes, #130200–#130218; and one ZON source. Its area header is in the full-world [areas/AREA list](../../../areas/AREA#L371). The local inventory has no QST, SHP, or story-sidecar file, no local QST giver, no Q/QA contract, and no shop. The separate [QST/AREA index](../../../areas/qst/AREA#L328) nevertheless lists `nexus_uc`; that index entry is not a local contract.

The ZON uses reset mode 0 and declares 22 door resets, 14 object placements, 16 mobile placements, and four carried-key placements. The 14 reset objects are all `ITEM_TELEPORT` prototypes; each has a nonzero, unlimited-use value and a local destination. There are no fall-chance or current markers. Reset declarations do not prove current availability or safe passage.

## Arrival, massacre, and the dead field

The reciprocal Surface crossing reaches room #130200, “A Field of Withered Grass and Shrubs”: `nexus_uc` north to Surface #577404 and Surface south back to #130200 ([outbound row](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1294), [return row](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1505)). Its text describes rotting vegetation, a stench of decay, a stone archway to the south, and “the vast expanse of the dead” to the north ([entry room](../../../areas/wld/nexus_uc.wld#L1)).

The south route reaches a blood-smeared antechamber and a ruined living quarter. The room descriptions mention rotting flesh, shattered furniture, body parts, and blood-stained floors ([antechamber](../../../areas/wld/nexus_uc.wld#L18)). Thotenthomanp, named the Lord of Decay, is reset in that chamber beside a claw-foot onyx tub filled with blood. The tub’s description invites bathing. The source leaves the massacre’s victims, cause, and relationship to the demon unresolved.

From the massacre area, the tub and an onyx mirror form a command-triggered pair: bathe in the tub to travel from #130201 to #130202, then stare into the mirror to return from #130202 to #130201. The script assigns the tub a maximum level of 30; higher-level non-trusted users are refused by the object procedure. The object data’s value 30 was added in the 2016 level adjustment.

The broader field is black, cracked earth filled with echoing cries from tortured souls. Its description says choking, acrid dust makes navigation nearly impossible. Wraith, vampire, and lich projections are explicitly tied to the Negative Material Plane. Their mob text distinguishes dangerous life-draining undead from liches, who retain memory and agency and may aid fellow necromancers. Two masses of negative energy connect separate field pockets when entered, and granite pillars provide touch-activated travel between other pockets. These room-to-room teleport routes are separate from WLD exits.

## Limestone wall, courtyards, and tower

The dead field gives way to an imposing white-limestone wall and a chain of gates around an unexpectedly lush courtyard. The courtyard prose places plants, birds, and jungle wildlife inside the stone structure. A spired tower’s base contains paintings of lost artifacts, legendary animals, and unknown people; higher levels are covered with unreadable glyphs and accounts of ancient wars and warlords ([wall and courtyard](../../../areas/wld/nexus_uc.wld#L309), [tower base](../../../areas/wld/nexus_uc.wld#L699), [upper tower](../../../areas/wld/nexus_uc.wld#L1441)).

Four named keys appear in the local object file: the “rite of passage” key #130214, white limestone gate key #130215, Nexus Tower key #130216, and key to the Reliquary Nexus of the Dead #130217 ([key prototypes](../../../areas/obj/nexus_uc.obj#L174)). The ZON gives #130214 to Thotenthomanp, #130215 to an Anubian soldier in room #130219, #130216 to another soldier in #130238, and #130217 to Osiris in the throne chamber. The reset source closes the paired arch, secret rite door, white gates, courtyard/tower passages, and final Nexus doorway. Osiris is titled Keeper of the Land of the Dead; his mob description only says that he wears white robes, clutches an ankh, and chants.

The tower’s throne chamber centers on a white-marble throne and the powers of undeath. The last room, #130284, is titled “The Reliquary Nexus of The Dead.” Its prose describes power bolts, deafening and waking a visitor, and magic that taints the soul. The description does not specify a durable effect, an accepted quest result, or the route’s intended return state ([throne and Nexus rooms](../../../areas/wld/nexus_uc.wld#L1475)).

## Elemental pockets and the seven object-route pairs

The same WLD range also holds isolated fire and earth pockets. A “Blazing Inferno” has near-melting heat, fire spouts, and a walkable molten floor; flame salamanders and a magman inhabit it. A rocky mass describes floating stone and the sensation of being buried alive; xorn hunt for gems, dao nobles attack intruders, and an efreeti scout searches the fire plane for slaves for the City of Brass. Statues name the earth lord Ogremoch, while two phoenixes are frozen in time ([fire pocket](../../../areas/wld/nexus_uc.wld#L1129), [rock pocket](../../../areas/wld/nexus_uc.wld#L1251)). The source connects these distinct pockets to the map through seven paired, reset `ITEM_TELEPORT` objects:

| Object-route pair | Trigger and source direction | Paired destination / return | Source-backed cue |
| --- | --- | --- | --- |
| Blood-filled tub #130200 / mirror #130218 | Bathe: #130201 → #130202 | Stare: #130202 → #130201 | Tub invites bathing; mirror is placed in the dead field. |
| Granite pillars #130201 / #130204 | Touch: #130215 → #130208 | Touch: #130208 → #130215 | Palm-print slots invite contact. |
| Negative-energy masses #130202 / #130203 | Enter: #130217 → #130226 | Enter: #130226 → #130217 | The masses draw at the soul and invite entry. |
| Earthen-hulk statues #130205 / #130206 | Stare: #130261 → #130270 | Stare: #130270 → #130261 | The statues’ eyes pulsate and stare back. |
| Ogremoch statues #130207 / #130208 | Worship: #130280 → #130269 | Worship: #130269 → #130280 | The statue description says its power draws worship. |
| Frozen phoenixes #130211 / #130212 | Scream: #130263 → #130254 | Scream: #130254 → #130263 | The object emits an echoing piercing scream. |
| Fireballs #130209 / #130210 | Touch: #130274 → #130273 | Touch: #130273 → #130274 | Their extra description draws attention and invites touch. |

The source configures these command IDs in each object’s second value and destinations in its first value. The core `ITEM_TELEPORT` handler checks the command, resolves the destination, and applies its uses ([handler](../../../src/magic/spell_travel.c#L932)); the command names are declared in [interp.h](../../../src/cmd/interp.h#L42). Counting these destination edges with the WLD graph makes all 85 rooms reachable from, and returnable to, the Surface entry in a source-level topology. That conclusion assumes the objects are reset, present, and usable; it is not a live path or safety guarantee.

## Additional level-gated entrances

The Surface exit is not the only declared entrance. A shared swirling-rift object #468 is reset in Heavens room #23 and in four active rooms in Tharnadia, Kimordril, Khildarak, and Shady ([Heavens reset](../../../areas/zon/heavens.zon#L145), [Tharnadia](../../../areas/zon/tharnadia.zon#L515), [Kimordril](../../../areas/zon/kimordril.zon#L170), [Khildarak](../../../areas/zon/khildarak.zon#L2127), [Shady](../../../areas/zon/shady.zon#L369)). The Heavens object’s extra description shows the tainted grounds of the Undead continent and invites entry ([object #468](../../../areas/obj/heavens.obj#L4865)).

The object callback accepts `enter`, denies non-trusted characters below level 10 or above level 30, and sends entrants directly to #130200 ([portal code](../../../src/specs/specs.heavens.c#L1519), [object-proc assignment](../../../src/specs/specs.assign.c#L1652)). The separate proc on local tub #130200 denies non-trusted characters above its value-3 limit of 30 ([level-gate code](../../../src/specs/specs.nexus_uc.c#L15), [local object-proc assignment](../../../src/specs/specs.assign.c#L1696)). These code paths explain why the area’s entry is absent from the WLD cross-area exit register; source code does not prove a portal is currently reset or usable.

## Story boundaries and retention

Source supports an environmental sequence through a massacre chamber, a hostile dead field, planar undead, a hidden limestone route, guarded courtyards, a keyed tower, and a final Nexus that taints visitors. Paired object routes branch into elemental pockets and provide source-declared returns. It does not provide a local QST, SHP, or story sidecar that assigns a quest giver, contract, reward, canonical massacre history, accepted allegiance, or saved result. The QST/AREA index alone does not supply those missing outcomes.

The normalized title “The Reliquary Nexus of The Dead” is unique among all 442 current `.zon` titles. Its same-stem WLD/MOB/OBJ/ZON history identifies no alternate-stem predecessor, tracked rename, or explicit whole-zone replacement. `nexus_uc` is distinct from the separately titled Nexus areas: “The Reliquary Nexus of Aggression,” “The Reliquary Nexus of the Corsairs,” and “The Reliquary Nexus of the Roper Den.” Keep active `nexus_uc` **DO NOT MOVE**. Keep the unregistered Duris3 source **RETAIN IN PLACE**; its source-only references are not active approaches or replacement evidence.
