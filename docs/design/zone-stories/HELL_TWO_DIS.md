# Hell, Dis

This dossier covers active contributor `hell_two_dis`. Its [audit supplement](../quest-story-followups/HELL_TWO_DIS_AUDIT_REVIEW.md) reconciles the shared room VNUM #51800 with Hell Three, Minauros.

## Environmental story

All 400 room records repeat the title The Plane of Dis on the Nine Hells and the same description. Dis is portrayed as a broad, mostly flat plane with occasional bare hills, a smoky green sky with lightning and faint thunder, and cold black stagnant rivers. The prose offers a clear environmental premise but no distinct room landmarks or local scenes. It contains the apparent typos “amd” and “think” ([first room](../../../areas/wld/hell_two_dis.wld#L1)).

Every room uses sector type 0 (`SECT_INSIDE`), which the world loader marks indoors and no-precipitation. Three hundred rooms have `ROOM_DARK`; the contiguous block #51601–#51700 does not ([first room in the un-darkened block](../../../areas/wld/hell_two_dis.wld#L5201)). The descriptions still place the plane under an open, lightning-lit sky. The source does not explain whether the sector and darkness fields are deliberate room behavior or template residue.

## Dispator, the rod, and resets

The sole local mobile, #51401, is the Arch Devil Dispator. The zone reset script places one in room #51441 with a maximum load of one ([mobile](../../../areas/mob/hell_two_dis.mob#L1); [reset](../../../areas/zon/hell_two_dis.zon#L10)). Its description calls him an arch duke of the Nine Hells and contains the spelling “Diapator.”

The only local object, also #51401, is the rod of Dispator. Hell Two’s ZON file does not reset the rod. Active `4horse.zon` does reset object #51401 in Vault of the Keepers, room #34804, whose description presents the vault as a reward store for challengers who defeat the Horsemen ([reset](../../../areas/zon/4horse.zon#L231); [vault room](../../../areas/wld/4horse.wld#L6025)). The object is bound in code to the `dispator` procedure, which can trigger hellfire spells while the rod is worn ([binding](../../../src/specs/specs.assign.c#L2009); [procedure](../../../src/specs/specs.underworld.c#L562)).

The code also contains a separate `zion_dispator` procedure and a `DISPATOR_VNUM` constant for #51401. The constant has no other source use, and no object assignment to `zion_dispator` appears in the current assignment search. That is not evidence that the separate procedure is active on this object.

## Quest and story boundary

Hell Two appears in the full world roster and QST roster, but there is no same-stem `.qst` or `.shp` file, story sidecar, or native Q/QA contract ([world roster](../../../areas/AREA#L222); [QST roster](../../../areas/qst/AREA#L203)). The checked-in Hell Two files establish an environmental plane, Dispator, and his rod. They do not define a local player objective, completion condition, or reward chain. The Horsemen vault placement is an external reset dependency, not a Hell Two quest contract.

## Builder questions and recommended resolutions

| Builder question | Recommended resolution |
| --- | --- |
| Are 400 copies of the same title and description intended? | Confirm whether this is a navigation template. If Dis is meant to support exploration, give the intended terrain and landmarks distinct descriptions through the normal area process. |
| Should every room be indoors with precipitation disabled despite the open-sky and river prose? | Confirm the intended climate and sector behavior. Align the text and sector fields after the owner decides; the prose alone does not establish live weather. |
| Why are rooms #51601–#51700 not dark while the other 300 are dark? | Confirm whether the block represents an intentionally lit feature. Name and describe it if so; otherwise confirm whether the flag block is accidental. |
| Which authored room #51800 is intended to own the shared VNUM, Dis or Minauros? | Resolve the duplicate without retiring either active contributor. If both remain, assign one record a unique room VNUM and update every dependent exit after a source review. |
| What is the intended Dis entrance, route, and safe return? | The seven registered inbound edges and four registered outbound edges all touch #51800. Confirm an unambiguous entry and return only after the collision is resolved, then record a gameplay route review. |
| Is the rod meant to be awarded in the Horsemen vault and to carry the `dispator` combat procedure? | Confirm the external #51401 reset and code binding together. Preserve the VNUM and its importer until both owners agree on placement and behavior. |
| Is the separate `zion_dispator` procedure still intended? | Check its unused #51401 constant and lack of a current object assignment. Treat it as unbound until an owner confirms the intended procedure; do not change the live `dispator` assignment as part of this audit. |
| Is Dis meant to remain environmental-only despite the Dispator encounter and rod? | Confirm whether a player-facing objective belongs here. If so, author and review a complete quest contract; the roster entry by itself is not one. |
| Should the prose typos be corrected? | Confirm the intended spelling of “amd,” “think,” “hansom,” and “Diapator,” preserving the established proper name Dispator. Keep text edits separate from VNUM and route decisions. |

## Connectivity and retention boundary

The Hell Two WLD source has 400 rooms and 1,600 authored exits. Its raw local graph is one weak component; from analysis seed #51401, 398 rooms are directionally reachable and all 400 can return to the seed. Rooms #51512 and #51513 are outside the seed’s directed reach, and 78 exits lack a matching reverse-direction record. These counts do not prove a player arrival or safe route.

Hell Two is an active full-area contributor and **DO NOT MOVE**. The shared #51800 room, the active Horsemen reset consumer, and the code-bound rod make attic disposition inappropriate. No candidate row or candidate-specific migration plan is added.
