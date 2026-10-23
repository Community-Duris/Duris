# The Maze of Undead Army: connectivity, dependencies and source history

**Checkpoint 105, source review only.** This supplements the [comprehensive
story dossier](../zone-stories/MAZE_OF_UNDEAD_ARMY.md) with a fresh all-area
connectivity census, inactive-WLD sweep, typed dependency scan, title review,
and tracked history check. It does not claim a live play-through, active-account
quest qualification, or safe zone migration.

## Active graph and boundary

The current [area manifest](../../../areas/AREA#L334) lists `maze_are` as an
active contributor. The scanner parsed all 351 active WLD contributors: 253,262
room records. `maze_are.wld` has 177 rooms, 395 exits, and no parse issues; 394
exits target local rooms and one targets another active area. Every exit
resolves in the loaded room set, and no Maze room VNUM duplicates another
active room. The local undirected projection is one 177-room weak component.

The only active boundary is reciprocal with Surface:

| Direction | Source edge | Return edge | Register |
| --- | --- | --- | --- |
| Surface to Maze | [`surface.wld` #571011 east to #94177](../../../areas/wld/surface.wld#L1490683) | [`maze_are.wld` #94177 west to #571011](../../../areas/wld/maze_are.wld#L3704) | [pair summary rows 265/539](WORLD_CROSS_AREA_PAIR_SUMMARY.csv#L265); [exit rows 1131/1490](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1131) |

From Maze entrance #94177, a directed scan over authored exits reaches 173 of
177 rooms. The other four are the carrier staging rooms #94117, #94118, #94119,
and #94121. They are separate from ordinary entrance-graph access, as the
dossier already notes. Door state, roaming, spells, dynamic travel, and a safe
return were not simulated; graph reachability is not a gameplay result.

## Inactive map approach

All 95 WLD files whose stems are absent from the active manifest parsed without
issues: 527,791 rooms and 2,080,773 exits, with no source-room VNUM overlap
against Maze. The scan found one approach from this source-only set:

| Inactive source | Authored edge | Qualification |
| --- | --- | --- |
| [`surf.wld` #281856, “Burnt Plains of a Fallen Realm”](../../../areas/wld/surf.wld#L1717656) | east to active Maze entrance #94177 | Not active ingress; the room does not belong to the active `surface.wld` manifest entry. |

This raises the reviewed source-only `surf.wld` subset from 131 to 132 and the
curated source-only approach total from 1,518 to 1,519. Keep it distinct from
the reciprocal active Surface edge above. It adds no active route and does not
establish ordinary-player access.

## Typed inbound and outbound dependency review

Local targets are 177 rooms, 43 mobiles, and 25 objects. `maze_are` has no
local SHP file. The scan covered active and unregistered QST/SHP/ZON/story
sources, external mobile/object definitions, all external type-25 object
declarations, and numeric references in maintained C/C++ source.

| Source class | Finding | Interpretation |
| --- | --- | --- |
| External QST item commands and SHP products | No exact typed reference to a Maze object. | No active or source-only exchange/shop consumer found in those formats. |
| External ZON reset roles | No active reset references a Maze room, mobile, or object. One inactive record, [`brass-old-1.zon` line 886](../../../areas/zon/brass-old-1.zon#L886), has `G 1 94009`; Maze object [#94009 is “drums of the south wind”](../../../areas/obj/maze_are.obj#L106). | `brass-old-1` is not in `areas/AREA`; this is a source-only reset declaration, not an active stock or quest input. Preserve it in that candidate's dependency record until the old source group receives its own closure. |
| Outside story sidecars | All 231 other sidecars parsed; zero structured numeric targets and zero numeric-token hits into Maze IDs. | No external story-sidecar dependency found. |
| External MOB/OBJ prototype definitions | Zero duplicate definitions of Maze mobiles or objects. | No external prototype collision found. |
| External type-25 portals | 872 declarations scanned; zero targets into Maze rooms. | No incoming typed portal found. |
| Maintained C/C++ numeric references | [`epic_skills.c` line 200](../../../src/classes/epic_skills.c#L200) binds Kalroh #94017 to Spellbind. | This is the existing compiled teaching registration covered by dossier item ZSQ-MAZEARE-14; its effective service remains unavailable under the reviewed handler/configuration. A separate `94159` token is part of a SHA-256 fingerprint in [`runtime_compatibility_contract.h`](../../../src/core/runtime_compatibility_contract.h#L77), not a VNUM binding. |

The local QST item commands, reset-role targets, and story `item_vnums` remain
local. The authored Surface boundary is the only active WLD dependency. The
inactive Brass reset is the one source-only typed reference discovered outside
the active source set; it does not change Maze's seven exchange recipes or
fresh-stock conclusions.

## Title and tracked source history

The active `.zon` title normalizes to “The Maze of Undead Army,” unique among
the 351 active nonempty titles. A scan of all current `.zon` titles found no
other title containing “maze” or “undead army.” The tracked WLD history follows
`maze_are.wld` through commits `642da5400` (2009 path relocation), `5794d8067`
(2011 map edits), and `71d731c5b` (2013 edits). The tracked ZON history follows
`maze_are.zon` through `642da5400` and `f18992b8f` (2015 quest-item flag
updates). The 2026 journal addition is `02f44475a`. The inspected history and
current source-name inventory show no alternate-stem rename, distinct
predecessor, or explicit replacement. The title “Maze of Death” in room prose
does not by itself establish a second zone identity.

## Disposition and builder questions

- **Maze: DO NOT MOVE.** It is an active `areas/AREA` contributor with one
  reciprocal Surface boundary, local quest/reset sources, and the compiled
  Kalroh service binding.
- **Surf: RETAIN.** Keep the inactive #281856 approach recorded separately
  from active connectivity. Confirm whether it is intentionally preserved
  legacy map data; do not activate it as part of any attic operation.
- **`brass-old-1`: RETAIN.** Preserve the source-only `G` reset to Maze object
  #94009 in the candidate ledger. Confirm its historical intent before any
  separate source-path decision; it is not an active reset.
- The Maze dossier adds **ZSQ-MAZEARE-18** for these two source-only inbound
  records and their builder disposition. No native gameplay or map files were
  changed.


## Checkpoint 274: direct builder and owner questions

These questions make the remaining design choices explicit for the seven Maze returns, their multi-item selection, source availability, hidden access, teaching, and legacy map references. The recommendations preserve the authored recipes and encounter placement until the owners choose a specific change.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Is the current first-matching-recipe behavior intended when a character carries more than one complete lich offering? | [native selection order](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L18), [five recipes](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L8) | Explain that players should carry only the intended set; add explicit recipe selection only in a separately designed service with frozen exact inputs. |
| 2 | Should the five lich bargains, Coordinator's finger return, and Krugor's dust return remain seven independent accepted histories? | [five lich contracts](../../../areas/qst/maze_are.qst#L33), [Coordinator contract](../../../areas/qst/maze_are.qst#L96), [Krugor contract](../../../areas/qst/maze_are.qst#L113) | Preserve all seven exact recipes, rewards, givers, and potential daily identities; do not merge them because they share an NPC or ingredient. |
| 3 | Should exact supplied materials remain eligible without personal kills or original recovery of the carrier? | [exact acceptance analysis](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L18), [proof-origin follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L58) | Keep the native item-only returns; record source victory and personal recovery as optional separate results. |
| 4 | Are the one-per-color carrier caps intended even though several accepted recipes need repeated colors? | [color sources and caps](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L20), [cap follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L61) | Preserve the caps and avoid a universal availability claim until supplied, stored, recovered, and forced-reset stock is qualified. |
| 5 | Are carrier starting rooms #94117, #94119, and #94121 meant to remain unreachable from the ordinary player entrance? | [carrier rooms](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L22), [directed graph](#active-graph-and-boundary) | Keep the staging placement until builders confirm intent; do not route players into hidden load rooms as a quest prerequisite. |
| 6 | Should the lich's Q113 response name Krugor rather than Kalroh? | [Q113 response](../../../areas/qst/maze_are.qst#L33), [actual giver binding](../../../areas/zon/maze_are.zon#L101), [wording follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L6) | Confirm the intended assistant's name, then make a bounded dialogue correction without changing the accepted recipe. |
| 7 | Should optional dust credit require the actor's successful SEARCH and recovery from one of the five hidden piles? | [dust placements](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L26), [dust follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L63) | Track search visibility, actor, pile UID, source generation, and custody independently from the five-item return. |
| 8 | Are the finger items intended to remain visible even though the dialogue describes hidden places? | [finger sources](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L28), [item flags and follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L64) | Preserve current item flags until builders confirm the clue; distinguish actual hidden SEARCH from corpse recovery and transfers. |
| 9 | Should the five-finger return require personal victory over the listed carriers, or is exact carried quantity sufficient? | [Coordinator recipe](../../../areas/qst/maze_are.qst#L96), [source distinctions](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L28) | Keep exact quantity sufficient for the native receipt; make each source and kill condition a separate optional objective. |
| 10 | Does the Coordinator's hand-cure dialogue intend a lasting player injury or healing outcome? | [Coordinator dialogue](../../../areas/qst/maze_are.qst#L81), [cure boundary](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L28) | Preserve the current key return; define a real before/after health or status field before crediting a cure. |
| 11 | Are the golden-key zero break chance and black-key 100% break chance intentional? | [key prototypes](../../../areas/obj/maze_are.obj#L190), [unlock handling](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L34) | Keep the separate policies; qualify committed key destruction and shared unlock state before adding access credit. |
| 12 | Should the sarcophagus stay closed, locked, and pickproof when the tomb prose calls it open? | [sarcophagus analysis](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L32), [container object](../../../areas/obj/maze_are.obj#L163) | Confirm which state the description should communicate; keep the current key and container flags until the owner decides. |
| 13 | Is wrist band #94025 intentionally an OTHER item type despite its wrist wear flag? | [wrist band placement](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L32), [object prototype](../../../areas/obj/maze_are.obj#L238) | Confirm intended equipment classification before a bounded prototype correction; it is not a quest input. |
| 14 | Is exitless room #94118 intended as a carrier trap, or should its current resident be recoverable through ordinary movement? | [trap room analysis](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L22), [spawn follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L62) | Preserve the staging room until the carrier's encounter, corpse, and item-recovery policy is confirmed. |
| 15 | What conditions should renew ingredients and givers for seven potential dailies while active accounting blocks fresh reset issuance? | [reset guard](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L38), [renewal follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L74) | Require durable reset-generation ownership, replay-safe stock issuance, and READY accounting before describing fresh daily stock. |
| 16 | Should the pentagram and dust returns continue removing their givers from the zone? | [giver departure rules](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L6), [follower and stock analysis](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L40) | Preserve D0 behavior; report keeper and follower availability separately from stored completion and date rollover. |
| 17 | Is Kalroh's currently unavailable Spellbind service intentional with its reward row commented and Enchant denied? | [compiled teaching analysis](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L42), [service follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L71) | Keep the service unavailable in player-facing claims until an owner approves a complete teaching and payment design. |
| 18 | Is Krugor's encoded warrior class intentional despite his alchemy descriptions? | [class and AI review](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L44), [assistant prototype](../../../areas/mob/maze_are.mob#L570) | Confirm intended role before a separate class or balance change; keep his alchemist AI and combat behavior separately documented. |
| 19 | Should secret tomb and catacomb access credit only an actor's actual reveal, unlock, open, and arrival sequence? | [key route](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L30), [search and shared-state follow-up](../zone-stories/MAZE_OF_UNDEAD_ARMY.md#L69) | Preserve supplied keys and shared access; attribute each successful state transition rather than infer personal access from a completed return. |
| 20 | Should the inactive Surf approach and Brass reset reference remain only in their existing source-owner records? | [inactive approach](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md#L32), [inactive reset](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md#L55), [disposition](MAZE_OF_UNDEAD_ARMY_AUDIT_REVIEW.md#L82) | Keep active Maze, Surf, and brass-old-1 identities separate; do not load or move the inactive records based on these references. |

Checkpoint 274 validation: the twenty questions link to the local QST, ZON, OBJ, and MOB sources, the comprehensive dossier, and the CP105 dependency and graph audit. Existing map and typed-reference counts remain unchanged. No area or gameplay source changed; no tests, build, server, database operation, migration, or operational script was run.
