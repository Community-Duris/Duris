# Hell, Minauros

This dossier covers active contributor hell_three_minauros, zone #518. The [audit supplement](../quest-story-followups/HELL_THREE_MINAUROS_AUDIT_REVIEW.md) reconciles its room graph with the shared room VNUM #51800 in Hell Two.

## Environmental story

All 400 Minauros WLD records use the title The Plane of Minauros on the Nine Hells and the same paragraph. It presents the third plane as a place of endless dirty rain and hail, murky fog, bodies of dead creatures, and the black fortress of Mammon, said to be built upon those bodies. The text names a strong environmental premise, but there are no distinct landmarks or room-by-room scenes. The paragraph and title repeat from [room #51800](../../../areas/wld/hell_three_minauros.wld#L1) through the whole source range.

The room fields add a notable environmental distinction. All 400 rooms use sector type 0 (SECT_INSIDE), so the loader marks them indoors and no-precipitation. Three hundred rooms also have ROOM_DARK; the contiguous #52000–#52099 block does not. The source text still describes rain and hail in both groups and does not label that undark block as the fortress ([room #52000](../../../areas/wld/hell_three_minauros.wld#L5201)). The sources do not establish whether the room flags intentionally represent a lit fortress within an otherwise dark plane.

## Mobiles, objects, and resets

The local mobile #51800 is a placeholder named nothing monster, with a description that says it is not a monster. The only local object #51800 is the placeholder nothing. Those mobile and object VNUMs do not collide with Hell Two's mobile or object prototypes; matching numbers in different prototype roles are separate namespaces ([mobile](../../../areas/mob/hell_three_minauros.mob#L1); [object](../../../areas/obj/hell_three_minauros.obj#L1)).

The ZON title is Hell, Minauros, with zone reset mode 1, but the file has no reset command beyond S. Neither placeholder is placed by the local reset file ([zone header and terminator](../../../areas/zon/hell_three_minauros.zon#L1)). No local mobile population, object reward, or reset-driven encounter is authored.

## Quest and story boundary

hell_three_minauros appears in both the world [area roster](../../../areas/AREA#L223) and the QST [area roster](../../../areas/qst/AREA#L204), but no local .qst or .shp file, story sidecar, or native Q/QA contract is present. The QST roster entry alone does not supply a quest. The checked-in area sources establish environmental prose and placeholder prototypes, with no player objective, acceptance step, completion condition, or reward chain.

## Builder questions and recommended resolutions

| Builder question | Recommended resolution |
| --- | --- |
| Are 400 copies of the same title and paragraph intended? | Confirm whether the repeated environmental copy is deliberate. If Minauros is meant to support navigation, give Mammon's fortress and other intended landmarks distinct titles and descriptions through normal area ownership. |
| Should every room be an indoor, no-precipitation sector even though the prose describes eternal rain and hail? | Confirm the intended climate and room sector fields. The source uses SECT_INSIDE in all rooms, and loader code adds ROOM_NO_PRECIP; do not treat the text as proof of a live precipitation effect. |
| Is the #52000–#52099 room block meant to be Mammon's lit fortress? | Confirm why only those 100 rooms omit ROOM_DARK. If this is the fortress, label its rooms and contrast their descriptions with the rain-soaked outdoor plane; otherwise confirm the darkness pattern is intentional. |
| Which area owns room #51800, the Minauros or Dis version? | Treat the conflict as unresolved. Both active WLD files contain #51800 with different second-plane and third-plane descriptions, and the tracked sources contain both records at the 2009 tree move. Confirm that both areas should remain; if so, assign unique room VNUMs and update all dependent exits under normal source review. Do not infer that either area replaces the other. |
| What are the intended Minauros entrance, route, and return? | The apparent four-in/seven-out boundary counts all touch the colliding #51800. Confirm a unique entry, verify the route and safe return after VNUM ownership is resolved, and record the result in gameplay review before relying on these edges. |
| Are the unused placeholder prototypes and absent quest contract intentional? | Confirm whether the mobile/object VNUMs are reserved or obsolete and whether Minauros is meant to remain environmental-only. Preserve them until every consumer and the room collision have been reviewed. |
| Should the room prose typos be corrected? | Confirm edits for herel and if built upon them. Preserve the authors' intended meaning and keep any text-only correction separate from route or VNUM decisions. |

## Connectivity and retention boundary

The raw Minauros WLD graph has one weak component of 400 rooms and 1,600 authored exits. Its largest directed strongly connected component contains 396 rooms. From analysis seed #51800, directed reach covers 396 rooms and 400 rooms can reach back to that seed; #51911, #51912, #51913, and #52199 are not reached from it. This is a source-graph result only: #51800 is also authored in Hell Two, so it is not a verified Minauros arrival point.

Hell Three remains an active full-area contributor and **DO NOT MOVE**. Its room collision, missing unambiguous WLD arrival, and unresolved route owner rule out an attic disposition. It has no candidate row or candidate-specific migration plan.
