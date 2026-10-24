# Shadowclave: the hidden forest and its druid circle

This dossier maps active area shadowcl from its WLD, MOB, OBJ, ZON, and SHP sources. It contains 53 rooms, eight mobile prototypes, sixteen object prototypes, and one shop. It has no local QST file or story sidecar, despite its qst/AREA listing. The area has no native Q or QA contract; its environmental story comes from room and mobile text, equipment, and reset placement. External QSTs, stores, and compiled routes use several of its VNUMs.

## Environmental story

The outer forest is dense, storm-fed, and unusually fertile. Rough paths are worn through young, fast-growing plants, flowers, vines, and trees. Several trails are deliberately concealed by bushes or thick vegetation. The path network passes a small pond, flowered clearings, a creek, and huge trees; two separate entrances lead from Surface slopes near Mount Ugta.

The title Shadowclave is explained by the hidden underground gathering space at room #93922. Its room text places druids and animals in a cavern and describes weapons of the Shadow Circle along its walls. Elsewhere, matching mobile equipment identifies a dark shadowy circle badge. A back room stores weapons and clothes for the secret order. One training room holds bears and another has similar animal damage. The meeting chamber has a table, stools, papers, and pitchers; a trapped route descends to Iliyad's alchemical workshop, while another route reaches a small dock.

The natural imagery turns increasingly dangerous below ground. A fast underground river runs through tight passages and jagged rocks, with sharks reset along its length. The final river room connects upward to Surface's small watering hole. The WLD contains no current or fall-chance fields, so the hazardous river prose does not by itself establish a forced current, fall, or safe travel result.

The local characters suggest a self-sustaining druid community rather than a complete native quest chain. Belim fishes at the pond and has a dead trout. A second druidic fisherman at the dock says he needs a fish and has a fishing pole. Verra the beast master asks for animal clippers and a harvest bag. Grelim the grand druid wants animal fur for a rug. Iliyad is an alchemist who brews potions and runs the local shop. Those lines suggest possible tasks, but no local QST file or accepted Q/QA contract implements them.

## Authored route and spaces

- Surface room #545307 south and room #545708 west both enter rough-path room #93900; #93900 also exits north and east to those rooms. These are two separate reciprocal pairs.
- The forest trails lead past concealed bush passages, ponds, gardens, and tall trees. Room #93915 is described as a magically enlarged hollow-tree sanctuary. A nearby ladder and door lead to the Grand Druid's chamber.
- The other forest approach reaches the Shadowclave cavern at #93922, then the meeting chamber and adjoining training, workshop, and back rooms. Most of these inner door directions are flagged and reset closed.
- The small dock in #93933 opens to the underground river. The river's final room #93952 exits up to Surface room #544905; Surface #544905 returns down. The river route is reciprocal in authored WLD, but its prose describes rapid flow and jagged rocks.
- Room #93918 has an east exit record with negative target -1. It is not a resolved route to another room.
- Compiled code adds a distinct possible arrival: Githzerai shifting from the Astral can randomly materialize in room #93919, the small pond. This is not a WLD exit and is conditional on the innate's race, location, combat, and timing checks.

Twenty flagged local directions form ten paired passages. ZON resets match all twenty at 100% chance and state value 1; comments describe the passages as closed. With all flagged edges traversable, the authored local graph is one 53-room strongly connected component. From room #93900, raw reachability and returnability are 53/53; suppressing all flagged edges leaves 5/5 from that entrance. These are static graph comparisons.

## Native stock, shop, and item identity

The ZON header records reset mode 2. The local shop is owned by mobile #93906, Iliyad, in room #93929. Its products are local bubbling brown potion #93914, dark misty potion #93915, and fizzing purple potion #93916. Iliyad also receives one of each as reset inventory. The coverage register previously counted zero shops despite the checked-in SHP record; this checkpoint corrects its count to one.

The zone equips its cloaked druids with badge object #93901, dark shadowy circle. Other local equipment includes the wanderer's staff, lunar sickle, bracelet of earth, forest boots, hierophant ring, beast-shaped earring, mask of beastkind, and heartwood cudgel. Belim receives trout #93910; the dock fisherman equips fishing pole #93913. The room hints about fur and clippers correspond to local object prototypes #93911 and #93912, but no local ZON reset, shop entry, or external typed source for either item was found. Their presence as prototypes does not establish available stock.

## External story and prototype uses

Snarg Skagtooth's Malch QST describes the druids as enemies of the horde and asks for three dark shadowy circles. Its completion block then has three G I #93901 commands and no matching R I #93901 command. The text and action ledger therefore disagree about whether the badges are required inputs or issued rewards. Newhaven QSTs also issue #93901 from two equipment-crafting transactions. Shipy's potion QST text names #93914 as a potion to collect from Shadowclave, while its visible completion action removes #43144 and issues #93914. Do not treat these text/action relationships as proof of a successful item turn-in without resolving the source behavior.

Alatorin resets #93901 onto a cloaked visitor and #93914 onto a dealer; Shipy also resets #93914 on a troll shopkeeper. Alatorin and Shipy shop definitions list #93914, while the local shop sells #93914-#93916. The external Malch, Newhaven, and Shipy story sidecars contain four structured item-VNUM references into Shadowclave. Chaos equipment support references #93915, and generic fishing code recognizes fishing pole #93913.

## Evidence and limits

The [rooms](../../../areas/wld/shadowcl.wld), [mobiles](../../../areas/mob/shadowcl.mob), [objects](../../../areas/obj/shadowcl.obj), [resets](../../../areas/zon/shadowcl.zon), and [shop](../../../areas/shp/shadowcl.shp) were reviewed. The [Shadowclave audit supplement](../quest-story-followups/SHADOWCL_AUDIT_REVIEW.md) records active and inactive boundaries, typed references, title/history, and builder questions. It separates static WLD exits, the conditional compiled Githzerai arrival, inactive Surf approaches, and the external item transactions.
