# Ogre Raiders: connectivity, typed dependencies, history, and builder decisions

**Checkpoint 335; coverage row 246.** This review reconciles the active WLD graph, reset-aware reach, the inactive Surf WLD approach, typed prototype references, title and source history, and the setting dossier. Source declarations do not establish live actor counts, runtime behavior, or played access.

## Active WLD graph and reset-aware entry

Ogre Raiders has 21 active rooms and 59 authored exits. All 59 targets resolve: 58 exits stay within Raiders and one exits to Surface. The local weak graph is one component of 21 rooms.

| Active boundary | Reciprocal boundary | Cross-area register |
| --- | --- | --- |
| Surface #593506 north to Raiders #8900 | Raiders #8900 south to Surface #593506 | [inbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1560), [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L182) |

The sole active inbound root reaches all 21 rooms by directed WLD exits before reset door states are applied. Two D resets close the two fence exits between #8910 and corral #8920. Both doors are source-marked as closed and unlocked. With closed exits excluded, strict post-reset reach is 20/21; only #8920 is missed. No other local D reset, fall marker, or current affects this traversal.

| Active arrival root | Raw directed reach | Strict post-reset reach |
| --- | ---: | ---: |
| Surface #593506 to Raiders #8900 | 21/21 | 20/21 |

This is a static traversal. It does not simulate door commands, reset timing, actors, keys, or whether a player can safely use a live exit.

## Source-only inactive-WLD approach

The all-95 inactive-WLD scan found exactly one non-overlapping source-only approach into Raiders: [inactive surf.wld room #277145](../../../areas/wld/surf.wld#L1618817) has a north exit to active room #8900. The source room is not an active Surface room and the edge is not active ingress.

Preserve the finding on the existing [Surf candidate row 97](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97). Surf's 145 active references do not change. The reviewed Surf source-only subset rises from 252 to 253, and the curated source-only total rises from 4,488 to 4,489. No candidate is added.

## Typed dependencies and bounded code scans

### Local content and cross-area consumers

The coverage inventory has no local QST, native Q/QA contract, shop, or story sidecar for Raiders. The local ZON equips the Ogre Leader with sword #8900 and the two half-orc followers with local armor objects.

One active external typed consumer was found: [surfacekeeps.zon line 233](../../../areas/zon/surfacekeeps.zon#L233) uses P to place Raiders object #8900 inside parent object #120028. The parent is the enormous stitched hide sack defined in [surfacekeeps.obj](../../../areas/obj/surfacekeeps.obj#L446). Preserve this consumer and the local equipment reset together. The source row does not prove the sword is currently present in a live sack.

Bounded scans found no external QST item command, SHP product, ZON placement of a Raiders room or mobile, story-sidecar VNUM reference, duplicate current mobile/object definition for a Raiders VNUM, or direct maintained C/C++ numeric reference. The type-25 object scan found no external target in Raiders among 872 current declarations. These results cover exact source references; they do not qualify editor tooling or runtime behavior.

## Title, identity, and followed history

After color removal, whitespace normalization, and case folding, "Ogre Raiders" occurs once among 442 current zone titles and once among 351 active area contributors. The six local mobile and three local object VNUMs have no duplicate current definitions in other active prototype files.

Followed history for the current raiders WLD, MOB, OBJ, and ZON paths remains on the same source stem. It records the 2009 area-tree move and later content or format updates; the MOB history also includes the 2015 quest-item-flag update commit. The bounded path/title review found no alternate-stem predecessor or explicit whole-zone replacement. No replacement is inferred from the shared "raider" subject matter elsewhere.

Keep active `raiders` **DO NOT MOVE**. Preserve its reciprocal Surface boundary, SurfaceKeeps' typed placement of sword #8900, and the separate inactive Surf approach on candidate row 97.

## Checkpoint 335 builder questions and recommended resolutions

These questions give the area owner concrete choices grounded in current source. Recommendations preserve authored content until an intended change is selected and qualified.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| OR01 | Does "Ogre Raiders" describe the camp and band shown in the source, or is another raid event intended? | Preserve the current title, room descriptions, and reset records; do not add an encounter or raid behavior without an authored design. |
| OR02 | Was the entrance fissure caused by a lightning strike, a small avalanche, or is the source meant to leave both possibilities open? | Keep both alternatives as written until the owner selects a more definite history. |
| OR03 | Does "the plains" south of #8900 refer to the active Surface connection at road room #593506? | Preserve the reciprocal boundary and leave the geographic wording unchanged until the area owner confirms the relationship. |
| OR04 | Should inactive Surf room #277145 remain a map-only northern approach to #8900? | Keep it source-only on Surf candidate row 97; do not treat it as active ingress or enable it through a move. |
| OR05 | Are the repeated descriptions across camp rooms #8903-#8918 intentional uniformity or unfinished variation? | Preserve all 16 room records and current prose until a builder supplies the desired room-by-room differences. |
| OR06 | Should the crevice's loose footing and mounted-travel restriction have any behavior beyond room description? | Treat the wording as prose; do not infer movement restrictions or hazards beyond source flags and exits. |
| OR07 | Is the camp's straw, smoke, animal, dung, and unwashed-body detail the intended environmental tone? | Retain the sensory text across the grid unless the owner requests a specific tone change. |
| OR08 | Is the leader's slow survey meant to signal ordinary authority, suspicion, or a deeper loyalty test? | Keep it as descriptive text; no current quest or special handler defines a test. |
| OR09 | Are the hut's cart floors, broken crates, bear-hide carpet, and smoky torch the intended account of the band's resources? | Preserve the scavenged-material details and the room's current single south exit. |
| OR10 | Should the priest's Sarmiz'Duul origin remain unexplained in this area? | Retain the origin and mystery language; no local contract supplies a travel history. |
| OR11 | Does the priest's search for assassins describe tension in the camp or an intended active threat? | Keep it as mobile description; do not infer a hidden assassin encounter or response. |
| OR12 | Is the shaman's BeastLord clan scarring meant as background, and is his presence intentionally unexplained? | Preserve both details until the owner identifies an authored relationship or story purpose. |
| OR13 | Are the two `F` reset rows meant to make the half-orcs follow the Ogre Leader in the hut? | Retain the source follower rows and equipment placements; qualify live following behavior separately before describing it as guaranteed. |
| OR14 | Are the orc raider and dirty bugbear roles complete as written, including the bugbear's unknown origin? | Keep both mobile descriptions and reset roles; do not invent a recruitment or origin story. |
| OR15 | Are the six repeated Ratou rows in corral #8920 intentional source declarations? | Preserve all six rows and their source caps; reset rows do not establish a live population. |
| OR16 | Is the fence meant to keep players out of the Ratou corral as well as animals out of camp? | Preserve the two closed, unlocked door resets; do not assume live access or safety from the static graph. |
| OR17 | Is the sword's local leader-equipment placement meant to coexist with SurfaceKeeps placing #8900 in sack #120028? | Preserve both typed records and ask the owners of the two areas to confirm the cross-area story relationship before changing either. |
| OR18 | Is the robe's residual magic important to its history, or only descriptive flavor? | Keep the current object description and placement; no source contract identifies a spell or quest use. |
| OR19 | Is the gold leggings' torn-set description an intentional clue to another item or just equipment flavor? | Preserve its text and the shaman's equipment reset; do not infer a missing set or collection objective. |
| OR20 | Is the absence of a Raiders QST, shop, and sidecar intentional for this environmental story? | Keep the area source-only in its current form unless the owner provides a quest or service design; no progression state is present to preserve. |

This was a documentation-only checkpoint. No WLD/MOB/OBJ/ZON source, code, area membership, candidate membership, or gameplay behavior changed. No build, test, server, database, migration, or gameplay session was run.
