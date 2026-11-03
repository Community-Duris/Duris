# Hell One: Avernus

This dossier covers the active hell_one_avernus contributor, zone #510, and its 400 room prototypes. The [audit supplement](../quest-story-followups/HELL_ONE_AVERNUS_AUDIT_REVIEW.md) records the source graph, boundary exits, resets, typed dependencies, title history, and retention evidence.

## Environmental story

Every room from #51001 through #51400 uses the title The Plane of Avernus on the Nine Hells and the same paragraph. It describes a dark land of rocky crags and hillocks, stunted poisonous vegetation, bare rock, concealed stake pits prepared for intruders, and caves and warrens used as devils' homes. The prose gives the area a clear hostile wilderness theme, but the 400 rooms do not distinguish landmarks or stages of a journey. The paragraph is present at the first room and repeats throughout the WLD source ([room #51001](../../../areas/wld/hell_one_avernus.wld#L1)).

The WLD records do not author fall chances or current fields. The hidden pits are therefore established here as room prose, not as a separately verified movement hazard. A different runtime effect was not established by the area source review.

## Mobiles, objects, and resets

The two mobile prototypes, #51002 and #51003, are both named devil swarm and share the same description: a small, annoying devil that runs in packs numbering hundreds to thousands. The two records are separately used as followers and a leader by the local reset script. The source spells annyoing and hundereds ([mobile source](../../../areas/mob/hell_one_avernus.mob#L1)).

The object prototypes give the area several specific artifacts:

| Object VNUM | Prototype | Source-backed role |
| --- | --- | --- |
| #51001 | nothing | Placeholder; no local reset places it. |
| #51002 | Poison stinger | Equipped on the locally reset swarm devils. |
| #51003 | Staff of Hellspawn | Given by the opening reset to a River Styx mobile; the same number is also the local swarm mobile VNUM in the separate mobile namespace. |
| #51004 | Glowing potion | Reset by five active rows across four other active areas and loaded directly by potion code. |
| #51005 | Death Rider, a blood-red unholy sword | Reset inside a Shabo chest and assigned the holy_weapon procedure. |
| #51006 | Tiamat's poison tail stinger | Reset in Tiamat and assigned the tiamat_stinger procedure. |

The [object prototypes](../../../areas/obj/hell_one_avernus.obj#L1) describe the sword as an unholy blade calling to those who seek blood for evil and the tail stinger as taken from Tiamat's corpse. These descriptions connect the area's artifact set to Hellspawn, Death Rider, and Tiamat, but the local room copy does not tell a more specific event or quest involving those objects.

The local ZON has 209 reset commands: 129 M, 39 F, 40 E, and one G. Of the mobile resets, 128 import nine mobile prototypes owned by River Styx (#90701–#90706, #90711, #90712, and #90715), while one loads local mobile #51003 in room #51381. The 39 followers use local mobile #51002. Forty equipment resets place local object #51002, the poison stinger, on the leader and followers. The opening G gives object #51003, the staff, to the mobile loaded near room #51001. The reset commands are visible in the [Hell One zone file](../../../areas/zon/hell_one_avernus.zon#L1); the imported mobile prototypes are in [River Styx](../../../areas/mob/river_styx.mob#L16).

## Quest and story boundary

Hell One appears in both the world [area roster](../../../areas/AREA#L221) and the QST [area roster](../../../areas/qst/AREA#L202), but there is no local .qst or .shp file, native Q/QA contract, or story sidecar. The presence in qst/AREA does not itself supply a quest. The checked-in local sources establish environmental prose, swarm-devil resets, and artifact prototypes, but no player objective, acceptance step, completion condition, or authored reward chain.

## Builder questions and recommended resolutions

| Builder question | Recommended resolution |
| --- | --- |
| Is the same room title and environmental paragraph intended for all 400 rooms? | Confirm whether the uniform copy is deliberate. If Hell One is meant to support navigation or a staged journey, add landmarks and distinct environmental beats through the normal area process; otherwise document the repetition as intentional. |
| Are Prison room #7492's north, east, south, west, and up exits all meant to arrive at #51032? | Confirm that five directions from one room to the same destination are deliberate. Keep each direction only if it reflects the intended entrance design. |
| Are the one-way boundary exits and the lack of direct reverse exits intentional? | Confirm the intended route from Prison, Celestia, and River Styx into Hell One and from Hell One to Tiamat, including how players return safely. Record and exercise any intended return path during normal gameplay review before changing exits. |
| Are the River Styx mobile imports and the swarm-devil, poison-stinger, and staff reset chain still intended? | Preserve the role-specific VNUMs while active. Review importer ownership and reset behavior together before changing prototypes or reset rows. |
| Should other areas continue importing these mobile and object prototypes? | Confirm the Avernus swarm group, Astral Tiamat staff, potion, Shabo sword, and Tiamat stinger consumers listed in the audit supplement. Review every importer and callback before any retirement or replacement decision. |
| Is Hell One intended to remain environmental-only despite its QST roster entry and named artifacts? | Confirm that no quest contract was omitted. If the area remains environmental-only, keep the register explicit that the roster entry does not provide a local quest. |
| Should the prototype typos be corrected? | Confirm corrections to annyoing and hundereds in the mobile description and stringer in the #51006 extra-description keyword. Preserve VNUMs and leave gameplay behavior unchanged. |

## Connectivity and retention boundary

The local directed WLD graph connects all 400 room records to one another. The seven active inbound WLD edges enter three rooms (#51001, #51032, and #51207), and each can reach the entire local graph. Every local room can reach the single outbound room #51100, whose east exit leads to Tiamat #19600. Those external boundary edges have no matching direct reverse edge in the paired zone sources; global route safety has not been evaluated.

Hell One is an active full-area contributor and remains **DO NOT MOVE**. Its active WLD entrances, one-way exit, local reset population, external reset users, and code-bound objects rule out an attic disposition. It has no candidate row or candidate-specific migration plan.
