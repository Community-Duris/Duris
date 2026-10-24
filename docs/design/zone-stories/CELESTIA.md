# Celestia: the Celestial Plane and Titan Palace

## Source identity and authored story

`celestia` is an active full-world contributor listed at [areas/AREA line 214](../../../areas/AREA#L214). Its `.zon` title is “Celestial Plane”; the file header assigns zone number 455 and room limit #45729, with reset mode 0 ([zone header](../../../areas/zon/celestia.zon#L1)). The area contains 230 rooms, 88 mobile prototypes, and 46 object prototypes. The separate quest index lists `celestia` at [areas/qst/AREA line 196](../../../areas/qst/AREA#L196). There is no local `.qst`, shop, or story-sidecar file and no local Q/QA contract.

The opening WLD text says Celestia was created as a refuge for souls, then became an eternal battlefield in a higher-plane racewar ([#45500](../../../areas/wld/celestia.wld#L1)). That complete title and lore block is repeated across 87 of the 230 room records after color codes are removed. The source also repeats a Galactic Ring description in 16 rooms and a Life Stream Current description in nine. The many repetitions make the central battlefield the area’s dominant authored backdrop.

Distinct scenes add cosmology and hazards to that backdrop. The Life Stream is described as the river of living souls; its “variance” is a calmer interval during the war ([#45545](../../../areas/wld/celestia.wld#L1497)). Evermore is a cosmic being composed of souls ([#45626](../../../areas/wld/celestia.wld#L4127)). Other rooms describe the gravity well, its survivable Galactic Ring, inverted gravity, cosmic storms, vortices, and rifts. The Gates of Hell lead north from Celestia into Avernus ([#45637](../../../areas/wld/celestia.wld#L4346)).

The Celestia Palace is presented as a stronghold of Titans and their guardians. The approach and gates are at #45639–#45640 ([#45639](../../../areas/wld/celestia.wld#L4388), [#45640](../../../areas/wld/celestia.wld#L4403)); later scenes include the courtyard, towers, ballroom, throne, and sealed vaults ([courtyard](../../../areas/wld/celestia.wld#L4750), [ballroom](../../../areas/wld/celestia.wld#L5077), [Cronus’s throne](../../../areas/wld/celestia.wld#L5898), [vault](../../../areas/wld/celestia.wld#L5933)). Door keys, closed gates, reset states, Titan and angel guards, and reset-loaded equipment support a layered palace encounter. The chamber of Malevolence is a separate reality-distortion scene at #45688 ([room](../../../areas/wld/celestia.wld#L5365)); its only WLD link from the inbound anchor is the reset-closed, locked, secret exit described in the audit supplement.

Some room descriptions place apparently unrelated settings inside Celestia: the Great Fountain of Tharnadia (#45679), Bahamut’s chamber (#45681), Macom the Shipwright’s office (#45682), and the Inn of the Shady Grove (#45683). The spatial-rupture room says the traveler has crossed Celestia, the Palace, and a gravity well before reaching the distorted region (#45687). The final gap is #45718, while #45719 is a summon/load room and #45729 explicitly describes a no-load punishment room ([#45718](../../../areas/wld/celestia.wld#L5948), [#45719](../../../areas/wld/celestia.wld#L5961), [#45729](../../../areas/wld/celestia.wld#L6201)). These cross-setting scenes and operational rooms should be reviewed with the area builder for intended purpose and presentation.

## Mobiles, objects, and resets

The local mobile file supplies cosmic hazards, angels, demons, Titans, named guardians, Cronus, Metatron, and Malevolence. The ZON resets place the corresponding groups around the wargrounds and Palace. Five ravaged souls load in #45719; the Palace vault and throne rooms load their named guardians and equipment ([reset placements](../../../areas/zon/celestia.zon#L368), [summon-room placements](../../../areas/zon/celestia.zon#L417)).

Three local objects are `ITEM_TELEPORT` prototypes, with the `enter` command encoded in `value[1]`:

| Object | Reset placement | Encoded destination |
| ---: | --- | ---: |
| #45535 | Celestia #45500 | Astral Main #19721 |
| #45536 | Astral Main #19721 and #19796; Ethereal Main #12401, #12406, and #12431 | Celestia #45500 |
| #45538 | Celestia #45718 | Celestia #45500 |

The local portal definitions and reset placements are at [objects #45535–#45538](../../../areas/obj/celestia.obj#L526) and [Celestia’s portal resets](../../../areas/zon/celestia.zon#L153). External return placements are documented in the [Astral Main audit supplement](../quest-story-followups/ASTRAL_MAIN_AUDIT_REVIEW.md#reset-loaded-type-25-portals). Treat these encoded destinations as conditional routes separate from the WLD graph; current player access is unverified.

The OBJ file also contains #45537, a glowing liquid pool (`ITEM_DRINKCON`). The #45718 reset places external object #63 beside portal #45538 and labels it “object type not in this .obj file” ([reset](../../../areas/zon/celestia.zon#L161), [local pool prototype](../../../areas/obj/celestia.obj#L542)). Four active OBJ sources define #63 as unrelated objects, including a Heavens glowing pool, so the intended prototype for the Celestia reset needs source-owner and loader-resolution review.

Other local prototypes have compiled roles or external consumers. Object #45534 is the level-56 potion reset at #45722 and used by the random-zone potion generator; #45530 and #45531 are pieces of the Master equipment set reset on the Malevolence encounter and also used by Vehicles. Malevolence (#45565) appears in the high-drop mob list. The bindings and consumers are summarized with exact source locations in the [typed-reference review](../quest-story-followups/CELESTIA_AUDIT_REVIEW.md#typed-inbound-and-outbound-dependencies).

## Compiled behavior

`Malevolence` has a compiled procedure. While fighting, it can banish a non-trusted player to a random room in #45500–#45520. Below one-third health it creates Malevolence Incarnate (#45571), transfers the boss’s carried and equipped items, moves the original to room #1, then invokes death. The Malevolence Incarnate and pulsar also have combat procedures. These behaviors are code-defined; their live occurrence and safety were not tested.

## Direct area-owner questions and conservative recommendations

These questions preserve Celestia’s active area identity and keep its reset-
dependent portals, disconnected palace, imported objects, and encounter rules
as explicit owner decisions.

| Question for the area owner | Source evidence | Recommendation |
| --- | --- | --- |
| Should `celestia` have a command-target alias beyond “Celestial Plane”? | The normalized `.zon` title is unique among 351 active and 442 current titles; tracked history stays on the `celestia` stem. | Keep the source key and title. Add an alias only for a named target with a clear content owner and distinction. |
| Is the QST index entry intended to signal a story or quest in Celestia? | The index lists `celestia`, but there is no local QST file, Q/QA contract, or story sidecar. | Treat the current area as environmental content unless the owner defines a story, participants, accepted outcomes, and stable contract identities. Do not infer a contract from index membership. |
| Is the 70-room Palace meant to remain a separate WLD component, and what route should reach it? | The Palace is a distinct 70-room component; its authored approach has only two rooms of open, non-secret reach after reset, and no encoded local type-25 destination enters the component. | Confirm intended player access and the exact key/door sequence before adding links or documenting a route. Preserve the static graph and reset states until an owner-selected path is qualified. |
| Should players arriving at #45718 use reset-loaded portal #45538 to leave, and what access/safety rules apply? | Realm and WH exits both arrive at #45718, whose sole WLD exit is reset closed, locked, and secret; #45538 is reset there with an encoded return to #45500. | Confirm command eligibility, reset availability, entry permissions, return route, and safe use before describing this as a working route or changing the doors. |
| Are the #45535/#45536 Celestia–Astral/Ethereal placements intended as a shared return network? | #45535 targets Astral Main #19721 from Celestia; external Astral and Ethereal resets place #45536 back to Celestia. #45536 has a `teleporting_pool` binding, while current presence and access were not verified. | Preserve all external reset consumers and qualify the exact source, use, and return behavior before changing either portal or treating the network as guaranteed access. |
| Are the Tharnadia fountain, Bahamut chamber, Macom’s office, and Shady Grove inn meant to be cross-plane vignettes? | Those room descriptions appear inside Celestia; the spatial-rupture room describes crossing Celestia, the Palace, and a gravity well. | Keep the text pending the owner’s setting and presentation decision. If selected as stale imports, correct only the identified room descriptions in a separate content change. |
| Which object prototype should the #45718 reset load as object #63, and should local pool #45537 be placed there? | Four active object files define #63 as unrelated prototypes; Celestia’s local #45537 is a drink container and is not reset there. | Resolve source ownership and loader selection before changing the reset. Do not replace #63 with #45537 solely because both descriptions mention a pool. |
| Are summon/load room #45719 and no-load room #45729 intended to keep their distinct roles and prose? | #45719 resets five ravaged souls, while #45729 explicitly describes a no-load punishment room; #45718 is also the external arrival anchor. | Confirm each room’s intended audience, behavior, and consequences before changing resets or descriptions. Keep operational and punishment spaces separate from ordinary quest credit. |
| Is Malevolence’s banish, item-transfer, transformation, and death sequence intended as authored? | Its procedure can banish non-trusted players during combat, transfer carried/equipped items to Malevolence Incarnate below one-third health, move the original to room #1, then invoke death. | Confirm intended eligibility, item handling, recovery, and player-safety terms with the combat owner. Do not promise a safe outcome or alter compiled behavior as part of this source review. |

Keep active `celestia` **DO NOT MOVE**. Its static edges, reset doors, portal prototypes, imported reset objects, and compiled behavior all remain active dependencies. The [repository-wide audit supplement](../quest-story-followups/CELESTIA_AUDIT_REVIEW.md) records the graph, external consumers, history, and retention evidence. This dossier records source intent and questions only; it changes no native content.
