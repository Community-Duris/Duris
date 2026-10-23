# The Haunted Woods: environmental story map

Reviewed October 9, 2026. Zone 108, `hwood`.
**Source review is complete; live access, shop stock and inn qualification remain unverified.**

The local source set has 58 rooms, 23 mobiles, 19 objects, one reset file and
one shop file containing two shops. The area is listed in `areas/AREA` and
`areas/qst/AREA`, but there is no local QST file, quest contract or story
sidecar. The checked-in story is environmental: a shadowed forest maze, an
abandoned Orc scout, a gnoll settlement across a boat crossing, and an elven
inn built into an oak. No ghost, curse, local quest completion or campaign
callback is defined by these sources.

## Forest, island and route story

The south entrance at room #10800 comes from Surface #517668 and sets the
forest's tone: the path leaves a clearing, the woods grow cold and shadowed,
and insects and indistinct creatures replace ordinary wildlife. At #10801, the
path splits between a cobblestone track toward the inn and a rough trail east
into the deeper woods ([entrance](../../../areas/wld/hwood.wld#L1),
[fork](../../../areas/wld/hwood.wld#L21)).

The eastern forest is intentionally hard to navigate in the prose. Rooms
#10807–#10831 repeatedly describe trails that appear and fade, little sunlight,
similar surroundings and scarce landmarks. They talk of dangerous creatures
and surprise encounters, not supernatural manifestations. The source repeats
two closely related descriptions across the maze instead of providing a
distinct route journal ([maze entry](../../../areas/wld/hwood.wld#L154)).

One hidden corner is a spider web with skeletal remains of travellers. The
gigantic spider is reset in that room. A west exit from room #10812 leads to
the web through an exit reset closed and secret; the return exit from the web
is reset closed ([web room](../../../areas/wld/hwood.wld#L916),
[spider](../../../areas/mob/hwood.mob#L1),
[door resets](../../../areas/zon/hwood.zon#L8)).

Farther north, a worn hut belongs to an old Orc scout stranded after a failed
raid on the elf island. His mobile describes him hiding from elven patrols and
trapping his own food. He wears a jagged metal bracelet said to be made from
rare Underdark metal. The two hut exits are reset closed; the room and item
sources define no local quest or receipt for them ([hut rooms](../../../areas/wld/hwood.wld#L931),
[scout](../../../areas/mob/hwood.mob#L17),
[bracelet](../../../areas/obj/hwood.obj#L20),
[door resets](../../../areas/zon/hwood.zon#L16)).

## River and gnoll settlement

The southwest trail leaves the dark woods, descends to the river and reaches
the southern shore at #10839. The text says the current is too fast for
crossing without a boat. A traveling half-Orc sells canoes and rafts there;
the shop file lists both local vehicle prototypes, and the reset loads the
merchant with a capped supply of canoes and one raft. The opposite bank leads
toward the gnoll settlement ([shore](../../../areas/wld/hwood.wld#L1064),
[merchant](../../../areas/mob/hwood.mob#L138),
[boat shop](../../../areas/shp/hwood.shp#L1),
[stock resets](../../../areas/zon/hwood.zon#L114)). This records intended
source stock and narrative, not live purchase or safe crossing behavior.

The gated gnoll village presents a small raiding society: guards patrol the
entry, nurses tend infants in a makeshift nursery, adolescents train with
spears, and warriors sleep in a barracks. The chief's tent contains medicinal
herbs and trophies; the chief is described as a shaman who cares for the sick
and wounded. Text says the gnolls produce raids against elf and centaur
settlements. These are authored social details; the area defines no gnoll
growth timer, raid event, healer service or quest contract
([gates](../../../areas/wld/hwood.wld#L1127),
[nursery](../../../areas/wld/hwood.wld#L1179),
[training field](../../../areas/wld/hwood.wld#L1196),
[chief's tent](../../../areas/wld/hwood.wld#L1339),
[chief](../../../areas/mob/hwood.mob#L263)).

## Oak Tree Inn and local objects

The cobblestone path ends at an inn set inside a large oak tree. Its prose
calls it a safe haven for elven and centaur travelers. Thalindelil and
Athowela have retired from adventuring to run the inn; a receptionist attends
guests upstairs, and the text only speculates that she may be related to the
owners. The inn's foyer has a separate general shop. Compiled code assigns the
native `inn` procedure to room #10857, the reception room; this source binding
does not establish live room availability, rates or checkout behavior
([tree and foyer](../../../areas/wld/hwood.wld#L1355),
[shop](../../../areas/wld/hwood.wld#L1408),
[living quarters](../../../areas/wld/hwood.wld#L1426),
[reception](../../../areas/wld/hwood.wld#L1443),
[inn binding](../../../src/specs/specs.assign.c#L2310)).

The item sources reinforce the settlement's details without creating quest
steps. A tiny diamond earring, described as lost from an ancient elven
kingdom, is loaded to the forest spider. The old scout wears the Underdark
metal bracelet. The chief wears a polished emerald ring; the savage gnoll
warrior carries a blood-stained spear; Thalindelil wears an emerald scimitar;
Athowela wears a jade necklace. The general shop resets local pastry, water
and silk products plus imported medical supplies from active Heavens
prototypes #363, #368 and #369 ([local objects](../../../areas/obj/hwood.obj#L1),
[Heavens supplies](../../../areas/obj/heavens.obj#L3783),
[shopkeeper resets](../../../areas/zon/hwood.zon#L146)).

The dossier preserves the checked-in story rather than adding objectives. All
eleven builder questions and recommended resolutions appear below; they are
source review, not selected world or shop changes.

## Builder questions and source-grounded recommendations

| Builder question | Recommended resolution before a separate change | Evidence |
| --- | --- | --- |
| Is room #10835 meant to remain a mobile staging room that players cannot reach from the Surface entrance? | Confirm that its one-way placement is intentional; keep its mobile resets separate from player-route claims unless an owner defines an accessible route. | [load room](../../../areas/wld/hwood.wld#L969), [mobile resets](../../../areas/zon/hwood.zon#L80) |
| Should the repeated maze prose stay deliberately disorienting, or should landmarks be added? | Preserve the current repeated descriptions until a builder chooses a navigation change with a matching map. | [maze descriptions](../../../areas/wld/hwood.wld#L154) |
| Is the web intended to require finding and opening a closed, secret west exit from #10812, with a closed exit back from the spider's room? | Confirm discovery and return intent before changing either exit flag or its reset state. | [web exits](../../../areas/wld/hwood.wld#L299), [door resets](../../../areas/zon/hwood.zon#L8) |
| Should the old scout's hut remain closed in both directions without a key or quest clue? | Keep it as authored unless the builder identifies a separate access story; the bracelet and scout text do not define one. | [hut doors](../../../areas/wld/hwood.wld#L931), [scout and bracelet](../../../areas/mob/hwood.mob#L17), [resets](../../../areas/zon/hwood.zon#L16) |
| Are the gnoll settlement's closed gate, nursery, barracks, warrior tent and chief's tent meant to be opened through ordinary door commands? | Confirm the intended access pattern before changing the reciprocal door resets; no key item or keyed quest is declared for these doors. | [settlement](../../../areas/wld/hwood.wld#L1127), [door resets](../../../areas/zon/hwood.zon#L24) |
| Is the river crossing meant to depend on canoes or rafts bought from the half-Orc? | Qualify stock limits, vehicle behavior and arrival/return before adjusting the 999 canoe reset cap, one-raft reset or crossing prose. | [river](../../../areas/wld/hwood.wld#L1064), [shop](../../../areas/shp/hwood.shp#L1), [stock](../../../areas/zon/hwood.zon#L114) |
| Is the nursery's promise that infants will train in a year or two flavor text, or intended to imply a future mobile lifecycle? | Treat it as prose until a builder names an actual timed or stateful behavior; no such transition is authored here. | [nursery](../../../areas/wld/hwood.wld#L1179), [infant](../../../areas/mob/hwood.mob#L170) |
| Should the general shop continue to sell the three Heavens medical prototypes and local food, water and silk stock? | Preserve the cross-area prototypes and their resets; review merchant stock and ownership before removing either the shop products or imports. | [shop stock list](../../../areas/shp/hwood.shp#L38), [import resets](../../../areas/zon/hwood.zon#L157), [Heavens objects](../../../areas/obj/heavens.obj#L3783) |
| Is #10857 the intended inn interaction room, with receptionist #10820 reset there? | Keep the compiled `inn` binding and the receptionist location distinct in source documentation; qualify actual check-in, room choice and checkout before describing them as live service. | [reception room](../../../areas/wld/hwood.wld#L1443), [receptionist reset](../../../areas/zon/hwood.zon#L164), [binding](../../../src/specs/specs.assign.c#L2310) |
| Is the area intended to remain an environmental exploration zone without an accepted local quest? | Retain the zero-contract source classification unless a builder supplies an objective, accepted input/output and lifecycle. | [QST roster](../../../areas/qst/AREA#L55); no local `hwood.qst` or story sidecar is present. |
| Does the “safe haven” inn prose apply only to the room and its elven guards, or should it communicate access restrictions elsewhere? | Keep safety and race eligibility tied to their exact room, reset and shop sources; do not infer island-wide protection or universal shop access. | [inn description](../../../areas/wld/hwood.wld#L1355), [guards](../../../areas/mob/hwood.mob#L281), [shop files](../../../areas/shp/hwood.shp#L1) |

## Qualification boundary

The story, room graph, resets, two shop definitions and compiled room binding
were reviewed from checked-in source. No test, build, server, database,
migration, gameplay session or operational script was run. Actual movement,
closed/secret-door operation, boat purchase and use, NPC behavior, shop stock,
inn service and player safety remain unverified.
