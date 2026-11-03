# The River Styx

This dossier covers active zone #907, `river_styx`, and its 100 room prototypes. The [audit supplement](../quest-story-followups/RIVER_STYX_AUDIT_REVIEW.md) records the source graph, reset snapshot, shared prototypes, tracked history, and retention evidence.

## Environmental story

A source-grounded reading is that the River Styx is a dangerous underworld transit route that also opens into several contained demonic lairs. Room #90700 begins at a wooden dock on Avernus, where an unusual pole holds a bone horn whose inscription promises that sounding it will call a boatkeeper. The river then runs through black underground tunnels, side caves, mist banks, and smaller docks. The room prose varies among rushing water, slow bends, cold, vapor, hidden red eyes, wind, red mist, and the river's pull on memory.

Several side branches provide more specific scenes. Rooms #90709–#90711 lead from the river into a small cave where the darkness appears animate. Rooms #90716–#90718 hide a hot, mauve moss cavern and a cess pit behind secret doors; bones, debris, and a black-and-red ring reinforce the foul environment. A second dock at #90732 bears another horn. Its nearby fissure and mist chamber lead to #90747, where a purplish-black stone leaks shadow-stuff and has a missing chip. The ZON nests that chip inside the stone.

The red-mist branch culminates in a blood-stained ledge and a torture room. Room #90760 describes an angelic captive who says it defeated a fiend in the hideout before the hooks and chains caught it; the room source does not identify that event or attach a quest contract to it. A nearby hall has three fountains and pools. Their portal objects link the hall to an opal throne room, a garnet bedroom, and a pearl laboratory. The laboratory continues into a small study.

Other routes add a wind-filled alcove, a melted-rock passage and slime-worm caverns, a windy river branch, a whirlpool, and stinking water whose description says it opens south into a swamp. The last statement is environmental text: room #90796 has only a north WLD exit back to #90795. Three submerged rooms describe the Styx as draining memory and connect upward to river rooms. The prose therefore suggests a broad, hostile underworld journey, but the authored map has a few route and transportation questions recorded below.

Color-stripped room title and description pairs yield 87 distinct text blocks across 100 rooms. Eight groups intentionally or accidentally repeat an exact block, including four copies of one broad cavern passage description and the repeated descriptions of the three submerged rooms. The repeated blocks are not separate evidence of a replacement zone.

## Mobiles, objects, and resets

The 19 mobile prototypes span River Styx guards and inhabitants. The ZON loads 18 of them locally; #90708, Charon the Boatman, is defined but has no local or external active ZON reset. Three pairs use identical descriptive text for different prototypes: black abishai #90702/#90711, green abishai #90703/#90712, and hydrodaemons #90710/#90713. Their prototype fields differ, so the names and prose do not establish that either copy is safe to merge.

| Mobile VNUM | Prototype |
| --- | --- |
| #90700 | Soulrender shade |
| #90701 | Amnizu devil |
| #90702 | Black abishai devil |
| #90703 | Green abishai devil |
| #90704 | Red abishai devil |
| #90705 | Blue abishai devil |
| #90706 | White abishai devil |
| #90707 | Shadow beast |
| #90708 | Charon the Boatman; not loaded by an active ZON reset |
| #90709 | Charonadaemon |
| #90710 | Hydrodaemon |
| #90711 | Black abishai devil, separate prototype |
| #90712 | Green abishai devil, separate prototype |
| #90713 | Hydrodaemon, separate prototype |
| #90714 | Mammoth maelephant guardian |
| #90715 | Erinyes devil |
| #90716 | Gigantic black slime worm |
| #90717 | Enormous green slime worm |
| #90718 | Shadow shade |

The 23 object prototypes provide set dressing, gates, and possible rewards. The two bone horns (#90700 and #90701) are both bard-instrument type objects; #90700 carries the inscription about calling the boatkeeper. The key, pools, portals, stone, corpse, and nested items establish most of the area-specific object chain.

| Object VNUMs | Source role |
| --- | --- |
| #90700–#90701 | Bone horns placed on the two docks; the first has the inscription |
| #90702 | Rune-covered adamantium key placed beneath the black waters |
| #90703–#90706 | Emerald, black-opal, garnet, and pearl fountains |
| #90707–#90712 | Three pools and their return portals between the fountain hall and three chambers |
| #90713, #90722 | Shadowed purplish-black stone and nested chip |
| #90714 | Black, slime-covered eggs |
| #90715–#90716 | Decaying devil corpse with the slime-covered pitchfork nested inside |
| #90717–#90721 | Greaves, bone-sewn gloves, ring, ruby rod, and wavy dagger |

The local ZON has 104 reset actions: 75 M, 17 O, two P, one G, three E, and six D. Its six chance-100 D commands close the two secret-door pairs at #90716/#90717 and #90717/#90718, and the locked adamantium door between #90759 and #90760. The last door uses key object #90702, which the ZON places at #90799. Objects #90707/#90709/#90711 lead from pool rooms in #90761 to #90762/#90763/#90764; matching objects #90708/#90710/#90712 return to #90761. These are item teleports activated with `enter`, so they are outside the ordinary WLD graph.

## Quest and story boundary

The area appears in both the full-world `areas/AREA` list and `areas/qst/AREA`, but has no local QST or SHP file, story sidecar, or native Q/QA contract. QST roster membership alone does not define a quest. The local sources establish an environmental dungeon, its locked chamber, its teleport objects, and possible hooks around the captive, stone, and boatkeeper; they do not establish an accepted objective, completion condition, or reward chain.

## Builder questions and recommended resolutions

| Builder question | Recommended resolution |
| --- | --- |
| Is the dock horn meant to summon Charon as its inscription says? | Confirm the intended player action and destination. The local horn objects are bard instruments, while the compiled spectral-ferry setup uses different prototypes and a different route; do not treat the prose alone as proof of a working crossing. |
| Are the identical horns #90700 and #90701 intentional copies for the two docks? | Keep both VNUMs while confirming the placement. Preserve #90700's inscription and decide whether the second horn should carry the same text. |
| Is the unused local Charon prototype #90708 intended to remain in River Styx? | Confirm whether it is an authored but dormant encounter or legacy data. Keep its identity separate from the ferry's #60008 mobile until builders resolve the relationship. |
| Is the fixed spectral-galleon route beginning at #600586 a separate travel system from the River Styx docks? | Confirm the intended connection between the two systems, including boarding action, itinerary, fare, and return. Current source code places the ferry on the Surface route and does not list a #907xx stop. |
| Is the absence of an ordinary active WLD entrance into River Styx intentional? | Confirm how players are intended to arrive. The all-source WLD census found no inbound edge, and the checked-in type-25 objects in other areas do not target this zone. |
| Is the one-way south exit from dock #90700 to Hell One #51001 intentional? | Confirm the intended route and safe return, because Hell One has no reciprocal WLD edge to the Styx dock. Keep the global route question open until its owner confirms it. |
| Should #90796 open south into the swamp described in its text? | Confirm the missing route. Either align the prose with a terminal river bend or have the area owner review a swamp continuation through the normal mapping process. |
| Is the north exit from #90740 back to #90740 intentional? | Confirm whether the self-target is a deliberate looping current or a WLD destination error. Preserve the source finding until an owner resolves it. |
| Are the submerged rooms #90797–#90799 meant to be reachable and escapable by their upward exits? | Verify intended movement, water, air, and current behavior in normal gameplay. The descriptions say the water erases memory, but static exits do not establish safe surfacing. |
| Are the 71 room-current fields aligned with the prose's changing river directions? | Confirm the directions and strengths across the bends and tributaries. In particular, review the strength-50 northward current at #90796 and the wind-driven east/west branch. |
| Are both closed secret-door pairs around the mauve moss and cess pit deliberate? | Confirm which exits should be discoverable and whether they are intended to be opened without a key. Keep the current closed reset state documented until that decision is made. |
| Is the #90702 key meant to open the locked door between #90759 and #90760? | Confirm the key-and-door chain and ensure the submerged key room is intentionally accessible before the bloodflow encounter. The WLD records name #90702 on both sides of the door and the ZON resets it at #90799. |
| Is the angelic captive in #90760 a finished story encounter or environmental background? | Confirm the captive's identity, the defeated fiend, and whether the missing memory cycle is deliberate. The room has no local quest contract that resolves those clues. |
| Are the three pool-to-chamber teleports the intended only entries into #90762–#90764? | Confirm that each fountain's pool leads to the intended destination and that its paired portal is the intended return. Preserve the separate object VNUMs and mapping. |
| Is #90780 intended as an optional continuation of the portal-accessed laboratory? | Confirm that the study's contents and return route are complete. Its only WLD link is to #90764, whose portal returns to the fountain hall. |
| Does the stone-and-chip pair (#90713/#90722) represent an unfinished objective? | Confirm whether the stone, contained chip, room text, and mobile resets are ambience or belong to a missing objective/reward sequence. No QST contract currently binds them. |
| Are the identical-text black/green abishai and hydrodaemon prototypes intended as different strength variants? | Keep the separate VNUMs and all external reset imports while the owner confirms the variants. Do not collapse them based on repeated description text. |
| Are the eight repeated room-description groups intentional navigation texture? | Confirm whether repeated paragraphs are deliberate. If the river should provide stronger route cues, add differentiated landmarks through normal area review rather than changing the content in this source audit. |
| Is River Styx meant to remain environmental-only despite its QST roster entry and named props? | Confirm that no quest file or sidecar was omitted. If no objective is intended, record that the QST roster line is membership only. |
| Should the source typos and inconsistent punctuation in the room/mobile/object prose be corrected? | Confirm editorial fixes with the area owner while preserving VNUMs and the intended distinctions among mobile variants. |

## Connectivity and retention boundary

The 100-room WLD has 194 resolved exits: 193 local and one outbound, with four weak components sized 96, 2, 1, and 1. Starting the static walk at dock #90700 reaches 96 rooms, and all 96 can return to that dock by directed WLD edges. The chance-100 reset snapshot closes four additional local rooms, leaving 92 reachable; the strict walk also excludes the four portal-only/lab rooms from the ordinary graph. The pool/portal objects can link those rooms when the player reaches #90761 and the objects are available. This is static source analysis, not a live access or safety test.

The single cross-zone exit is River Styx #90700 south to Hell One #51001, with no direct reverse edge. No active or non-active source WLD file among the other 445 files approaches a River Styx room or duplicates one of its room VNUMs. The area declares no fall chance and 71 river-current fields; native movement code can resist upstream movement and sweep characters with the current in water rooms. Neither the exit graph nor the descriptions certify safe passage.

Keep active `river_styx` **DO NOT MOVE**. Its active outbound edge, local travel objects, and 205 active external ZON mobile references (193 M loads and 12 F follow rows) are source dependencies. No candidate-specific attic row or migration plan is appropriate for this active contributor.
