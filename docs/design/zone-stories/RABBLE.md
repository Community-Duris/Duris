# Rabble: source story and setting dossier

**Checkpoint 334; coverage row 245; source snapshot 72ba3deff3bb30bde880018cdebe7c6db675b697.** This dossier maps the current authored setting, room topology, shop, actors, and environmental text. Rabble has no native QST file or structured story sidecar, so its signage and scene details remain setting evidence rather than quest or progression records.

## Source scope

Rabble is the active full-area contributor at [areas/AREA line 245](../../../areas/AREA#L245). Its current sources define six WLD rooms (#65100–#65105), two mobiles (#65100–#65101), seven objects (#65100–#65106), one ZON, and one SHP. There is no Rabble QST file or story sidecar. The current source list has no separate special binding for its mobiles or rooms.

The area has one active WLD boundary: Heavens room #1234, Fo's Place, descends to Rabble room #65100, and Rabble returns upward to #1234. The reciprocal pair is recorded in the [world exit register](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L20) and [outbound register row](../quest-story-followups/WORLD_CROSS_AREA_EXIT_REGISTER.csv#L825).

## Rooms and authored route

| Room | Authored setting | WLD exits |
| --- | --- | --- |
| #65100, Within an Onyx Building | Black marble with red veins surrounds a shimmering dimensional portal. A north passage reaches the star-filled expanse; up leads to Heavens #1234. | North to #65101; up to Heavens #1234. |
| #65101, Within a Huge Expanse of Nothingness | Stars and emptiness surround a small stone platform and stair. | South to #65100; down to #65102. |
| #65102, The Room of Twilight | The text calls this the center or nexus of the space. It identifies an east bar, a south “familiar place,” and a west “private retreat.” | East to #65103; south to #65105; west to #65104; up to #65101. |
| #65103, Frothe's Bar and Firepit | The welcome describes people eating, drinking, and leaving their worries behind. Newspaper clippings and trophy heads decorate the room. | West to #65102. |
| #65104, Keira's Playroom | The description calls this Keira's space and says she decides whether others may play. Its only WLD exit is open in the source. | East to #65102. |
| #65105, Room #13 | A small furnished room with a bed and large chest in the prose. The local reset file does not place a matching room object here. | North to #65102. |

The authored traversal is a six-room connected scene with a reciprocal Heavens entrance. It does not define a multi-destination portal network: the other rooms connect by ordinary WLD exits, and the area has no door resets or other WLD arrivals.

## Environmental story

The entry room's main description frames the portal as a dimensional rift in an onyx building. Its extra description under the keyword “portal” describes a blue-lit crypt with troll and orc bones and says the portal's destination is unknown. The active WLD destination is Heavens #1234, whose title is Fo's Place. That text leaves a builder-facing question about whether the crypt text is meant to describe the far side or whether the portal narrative needs alignment.

The Room of Twilight presents itself as a nexus “where all ends meet,” but the local WLD declares just one cross-area boundary. The “familiar place” is the south-connected Room #13. The “private retreat” is west-connected Keira's Playroom; its prose asks for an invitation, but its WLD passage has no door or reset gate. Keep the authored social meaning separate from enforced access.

Three extra descriptions in the WLD use the keyword “portal”: the onyx entry and both side rooms, Keira's Playroom and Room #13. The same crypt illustration and text appear in all three. The two side rooms do not describe a portal in their main text and have no cross-area exit. This is a repeated source feature that needs owner review before it is treated as intentional scenery.

Frothe's Bar combines tavern setting with community references. Its wall clippings tell a humorous “five years ago and now” story about Mc'Malok's spread across Duris, Parka's Flamestrike cooking method in Tharnadia, and Malok settling down after adventuring in the Underdark and elemental planes. A second clipping jokes about Cython returning to the NBA after an ankle injury involving Tripod and a triple-purge-ban slam dunk, and mentions Tolm. The room also names trophy subjects Ithaquua, Taranno, and Zupov, and its dartboard extra description jokes about “Pin the Dart on the Thugan.” These are authored references and jokes; the source does not connect them to a QST, kill counter, or completion flag.

## Actors, shop, and placed objects

The ZON resets four identical Obsidian Demon mobile rows at the onyx entrance, each with a source cap of four, then one Frothe row in the bar with a cap of one. The Demon description uses threatening, infernal imagery. Frothe is described as a dwarf Brewmeister and has the speech string “Me Dorf, Me frothe, no picture yet.” The source-only reset declarations do not establish how many actors are present after a live reset or what encounter outcome a player experiences.

The single shop record uses keeper #65101, Frothe, and lists local objects #65102 Hot Wings, #65103 Guinness (spelled “Guiness” in its prototype), and #65104 Rabble Insignia Patch. The ZON also gives those three local objects to Frothe. No local QST consumes the patch or treats the trophies, darts, drinks, or demon as mission progress.

Local resets place the entrance sign #65100 in #65100, the Obsidian Fountain #65106 in the Twilight room #65102, the second sign #65101 in the bar, and Punch Bowl #65105 in the bar. The punch bowl and fountain are type-17 object records; this dossier records the data without assuming live drinking, refill, or fountain behavior.

The source also gives Frothe external objects #377, #347, #399, #53, #6094, and #6098. Four have an active Heavens object definition; the #53 definition is a hemp rope coil. The current object-source scan found no definition for #6094 or #6098. Those VNUMs also identify rooms in the Lizard WLD source, which does not make them object prototypes. See the [typed-dependency review](../quest-story-followups/RABBLE_AUDIT_REVIEW.md#typed-dependencies-and-bounded-code-scans) before changing the local reset list.

## Story and qualification boundary

Rabble has zero native quest contracts and no sidecar state model. It contains environmental history, one tavern shop, two mobile prototypes, props, and a single reciprocal Heavens boundary. The presence of a shop item, trophy name, room description, or reset declaration does not establish an authored mission, a persistent relationship, a successful transaction, or a completed encounter.

Twenty source-backed questions with recommended resolutions are in the [Rabble audit review](../quest-story-followups/RABBLE_AUDIT_REVIEW.md#checkpoint-334-builder-questions-and-recommended-resolutions). No gameplay repair is selected in this dossier.
