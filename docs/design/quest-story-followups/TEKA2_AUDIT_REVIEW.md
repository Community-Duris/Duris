# The Dark Stone Tower of the Northern Realms repository-wide audit supplement

This checkpoint supplements the [source-comprehensive Teka2 dossier](../zone-stories/DARK_STONE_TOWER.md) with active and inactive connectivity, typed cross-area references, normalized title and bounded source history. The source snapshot is `72ba3deff3bb30bde880018cdebe7c6db675b697`. This is a documentation audit; it changes no area or gameplay source.

## Active graph and North Lakes boundary

The active `teka2` WLD has 98 rooms and 198 authored exits. All 198 resolve: 197 are local and one leaves for North Lakes. The weak undirected projection has components of 85 and 13 rooms. A plain directed local-exit traversal from cathedral room #75546 reaches 11/98 rooms; reversing those authored edges shows paths back to #75546 from 85/98 rooms. These raw WLD traversals do not include key access or the four typed item-teleport commands. The dossier separately maps those conditional routes and finds larger eligible paths when keys, objects and secret-door discovery are included.

| Active boundary | Evidence and relationship |
| --- | --- |
| Teka2 #75546 south → North Lakes #75264 | [Teka2 exit](../../../areas/wld/teka2.wld#L1071). |
| North Lakes #75264 north → Teka2 #75546 | [North Lakes exit](../../../areas/wld/nlakes.wld#L1422). This is the one active reciprocal pair. |

Teka2 has one fall-chance field at #75581 ([field](../../../areas/wld/teka2.wld#L1730)) and three current fields at #75596–#75598 ([#75596](../../../areas/wld/teka2.wld#L2054), [#75597](../../../areas/wld/teka2.wld#L2081), [#75598](../../../areas/wld/teka2.wld#L2103)). Keep those actual source hazards distinct from the maze of keys, object teleports and eligible water travel recorded in the dossier; authored routes do not establish safety or successful arrival.

## Inactive-WLD and attic checks

The all-95 inactive-WLD sweep parsed 527,791 rooms and 2,080,773 exits without parse issues. It found no inactive source-room VNUM overlap and no exit target into any of the 98 Teka2 rooms. No source-only map approach is added. The curated source-only total remains 1,997 overall, and the reviewed Surf subset remains 144. There is no Teka2-specific attic candidate or migration clearance arising from this pass.

## Typed dependencies and scoped negative checks

| Dependency | Source-backed result and boundary |
| --- | --- |
| Local imported reset objects | Teka2 resets imported rune-covered stone #358 and White Hills memory #55454 at #75519 ([reset imports](../../../areas/zon/teka2.zon#L113), [second import](../../../areas/zon/teka2.zon#L114)). Their definitions remain in `heavens.obj` and `wh.obj` ([#358](../../../areas/obj/heavens.obj#L3709), [#55454](../../../areas/obj/wh.obj#L5349)). The compiled object binding assigns #358 to `epic_stone` ([assignment](../../../src/specs/specs.assign.c#L1603)); the imported objects are not rhinestone #75559 or the native receipt. |
| Outside reset use of a local object | Active Alatorin reset groups equip local strange vial of blood #75529 on flesh golems in two separate groups ([first](../../../areas/zon/alatorin.zon#L2068), [second](../../../areas/zon/alatorin.zon#L2070)). This is a shared item-stock dependency, not a native rhinestone consumer or accepted Teka2 quest outcome. |
| DLSC numeric match | The only outside QST token match is `R E 75500` in DLSC ([QST](../../../areas/qst/dlsc.qst#L26)). Its paired typed story result is 75,500 experience (`E:75500`) plus outside item #36859 ([contract](../../../areas/story/dlsc.story.json#L156), [completion row](../../../areas/story/dlsc.story.json#L189)); `E` is the experience reward, not a Teka2 VNUM reference. No outside exact QST item command consumes a local Teka2 prototype. |
| Neighbor orientation text | North Lakes' sidecar mentions its vestibule connection to Teka2 and explicitly keeps the areas' discovery and quest credit separate ([orientation](../../../areas/story/nlakes.story.json#L13)). This is narrative route context, not an item or typed completion dependency. |
| Compiled references | Normal-boot epic skill rows bind Aerlyn #75500 to chant mastery and Osmirim #75523 to sneaky strike ([epic skills](../../../src/classes/epic_skills.c#L173), [second skill](../../../src/classes/epic_skills.c#L177)). The old #75561 flaming-mace special remains commented out ([assignment](../../../src/specs/specs.assign.c#L2019), [legacy procedure comment](../../../src/specs/specs.teka2.c#L13)); it is not an enabled binding. The raw numeric hit in `mail_sender.c` is a `LIBCURL_VERSION_NUM >= 0x075500` version guard, not a VNUM ([guard](../../../src/net/mail_sender.c#L655)). |
| Bounded negative checks | No outside exact story item VNUM, outside SHP product, external duplicate mobile/object prototype definition or external type-25 portal destination into Teka2 was found. Teka2 has no local SHP source. The typed outside matches are the two Alatorin resets above; the DLSC experience constant and North Lakes orientation sentence are classified separately. |

The Teka2 dossier remains the detailed authority for the #75559 dragon-egg source, hidden-object and actor admission, four shared teleport commands, key/door states, training and epic-node bindings, current/fall checks and 22 builder follow-ups. Static source records do not establish current stock, successful search/get, actor eligibility, training settlement or played arrival.

## Title, history, and disposition

After removing color tokens and normalizing whitespace/case, “The Dark Stone Tower of the Northern Realms” has one exact owner among 351 active `.zon` titles: `teka2` ([title](../../../areas/zon/teka2.zon#L2), [active area membership](../../../areas/AREA#L271)). The lexical title scan found no second active candidate.

The followed WLD, ZON, MOB, OBJ and QST source histories remain on the `teka2` stem in the available tracked history from the 2009 area-source commit `642da5400`, with subsequent same-stem edits including the 2013 area update `71d731c5b`, 2015 quest-item flag updates `f18992b8f`, and 2026 builder/equipment changes `c8b1fa45f`. No alternate-stem rename, explicit predecessor or whole-zone replacement claim appears in these bounded paths. `teka2.story.json` was added on 2026-10-06 in commit `92fab5e67`. Keep active `teka2` **DO NOT MOVE**; keep its North Lakes boundary as the authored reciprocal pair.

## Builder recommendations

1. Preserve the active North Lakes pair at #75546↔#75264. Keep raw directed WLD traversal separate from the dossier's conditional key, object-teleport, secret-door and water routes.
2. Preserve the local imported object identities #358/#55454 and Alatorin's two reset uses of local blood #75529. Neither imported stock nor outside item stock substitutes for accepted rhinestone #75559.
3. Keep DLSC's 75,500 XP constant and North Lakes' route orientation as their own typed/narrative records. Do not interpret same-numbered experience values as local prototypes.
4. Retain active `teka2` **DO NOT MOVE**. This checkpoint finds no inactive-WLD approach and introduces no gameplay repair or area placement change.
## Checkpoint 379: Twenty-two builder questions and recommended resolutions

The questions below make the existing dossier's builder follow-ups explicit as decisions. They preserve source ownership and current behavior pending a builder choice; they do not authorize native repairs.

1. **Should Aerlyn's Q32 remain one return with two rewards and one departure?** The native contract consumes rhinestone #75559, issues mask #75560 and mace #75561, then removes Aerlyn. **Recommendation:** Keep one accepted receipt and one completion outcome for the complete native transaction; do not create separate progress for either reward or departure.

2. **Which dialogue should award an addressed-response outcome?** The three selected ASK families have exact aliases, while three qc_action80 records are random CMD_NONE room messages. **Recommendation:** If builders want dialogue credit, bind it to successful addressed dispatch and response state; aliases stay within one family and ambient lines remain orientation only.

3. **Should the dragon source prove personal acquisition?** The silver dragon reset carries egg #75542; the nested stone #75559 and vial of blood #75529 have separate P declarations, with cap-one/chance-100 reset fields. **Recommendation:** Keep first-source lineage optional and distinct from native turn-in. Exact supplied stones remain eligible; do not add personal kill or mining-report prerequisites.

4. **What should count as revealing and recovering the hidden rhinestone?** The egg is closed, closeable and unlocked; the stone is SECRET, and searching, revelation, ownership recovery and a visible loose offering are distinct. **Recommendation:** Preserve the actual OPEN/SEARCH/GET sequence and actor attribution. Do not award reveal or recovery from a shared message or another character's action.

5. **Can the current preparation display claim that an item is ready to offer?** Runtime snapshot counts prototype presence without CAN_SEE or UID ownership checks. **Recommendation:** Describe current readiness as raw carried-prototype presence until status is admission-aware; a future status change must represent visibility, ownership and loose-root eligibility without silently changing native Q32 rules.

6. **Should the one potential daily renew on a timer alone?** The zone uses reset mode 1; the dragon and egg declarations are cap-one, and Aerlyn leaves after the accepted return. **Recommendation:** Keep renewal disabled until fresh source generations, actual teacher availability, daily identity and durable accepted history are qualified. A reset interval does not prove a new offering or restore Aerlyn.

7. **Does Aerlyn actually raise the long-dead mage of Ruzdo?** Her completion text promises this result, but the selected native outcome issues two objects and removes Aerlyn. **Recommendation:** Let builders choose clarified prose or a separately specified resurrection state, actor, destination and durable result. Do not record the promise as an achieved event.

8. **Does Ilbe's request stop mining operations?** Ilbe asks to be told if the rhinestone is found so operations may stop; no selected report or stop-work state change exists. **Recommendation:** Keep the line as dialogue until builders select a report recipient and actual operation-state outcome. A response or player possession alone is not a completed report.

9. **What evidence should establish one of the four object-teleport journeys?** Altar, mirror, orb and board use generic type-25 dispatch with TOUCH, ENTER, STARE and PULL. **Recommendation:** Keep their separate object, command, destination, actor-admission and actual-arrival checks; a handled command or travel message does not prove placement.

10. **Are the paired door reset states and available paths intentional?** Keyed doors have asymmetric reset flags, two mirror pairs rely on secret/search state, and raw WLD secret fields without matching D resets are masked by the loader. **Recommendation:** Preserve actual D-reset state and eligible alternate routes. Resolve prose-versus-reset mismatches with the builder before changing a door or requiring a hidden search.

11. **Is the cathedral the intended orientation point for the wider tower?** Raw directed traversal from #75546 reaches 11 of 98 rooms; keys, secret discovery and the four typed teleports change conditional reach, while the weak graph has components of 85 and 13. **Recommendation:** Present raw topology and conditional routes separately. Keep current directions until builders select a route change and its balanced access.

12. **Should rooms #75577 and #75578 remain without a selected inbound route?** They have outgoing paths but no selected incoming edge or portal; unused #75579 has no dangling reference. **Recommendation:** Ask whether an inbound connection is intended. Preserve present room identities and directions meanwhile; do not renumber a harmless gap.

13. **Should the six holder/key/door routes remain separate optional exploration?** The golden, ivory, priest, prison, iron and small keys belong to different holders and gates, with reset flags and alternate paths. **Recommendation:** Keep each key's source and matching door explicit. Do not turn a key into an extra rhinestone requirement or treat a shared unlock as personal acquisition.

14. **How should fall and current fields inform safe-travel guidance?** Room #75581 has a fall chance; rooms #75596-#75598 have current fields and water-sector movement. **Recommendation:** Keep hazard outcomes conditional on actor eligibility and actual travel. A room description or arrival message does not establish safety, survival or successful return.

15. **When should Aerlyn's chant teaching receive progress?** Normal boot configures Aerlyn as an epic teacher; the maintained service checks its level/class/cap/prerequisite/funds rules and refuses purchases while accounting is active. **Recommendation:** Count only an admitted learned-skill change with its fee and durable save; encounters, displayed prices and refusals are not learning.

16. **How should Aerlyn's departure affect teaching and source availability?** Accepted Q32 removes the same mobile that serves as the teacher and quest giver. **Recommendation:** Keep the availability consequence visible and distinct from return acceptance. Do not imply a future daily source until builders define and qualify replenishment or a replacement teacher.

17. **Should Osmirim's sneaky-strike teaching remain independent of Aerlyn's request?** Normal boot binds Osmirim #75523 to that skill. **Recommendation:** Preserve its own actual teacher admission and durable learning result; meeting Osmirim, skill display or Aerlyn completion cannot stand in for a successful purchase.

18. **What counts as an imported rune/node outcome?** Rune #358 is an imported object bound to epic_stone; memory #55454 has a _noquest_ keyword and distinct metadata. **Recommendation:** Keep node touch, participant and zone ownership under their existing subsystem, and do not award Teka2 quest completion from visiting a node or carrying the memory.

19. **Should the prison, demon, Lashan-soul and scene-following leads become a campaign?** The selected 12F dialogue and reset following do not provide a rescue, escort or soul-release completion endpoint. **Recommendation:** Keep these as story orientation until builders define player contribution, recipient, durable state and safe arrival for any campaign.

20. **Should Alatorin continue to reset local vial #75529 onto its flesh golems?** Two active Alatorin groups use the local blood object under Alatorin's reset owner. **Recommendation:** Preserve that foreign stock relationship and type identity; it is not the rhinestone, an Aerlyn turn-in, or proof of a player's source history.

21. **Do the negative-weight containers and trapped-soul staff need repair?** Egg and robe weights are legacy container data; staff #75555 is ITEM_OTHER with trap metadata. **Recommendation:** Ask for separate balance/type intent before repair. Names alone do not authorize changing item type, weight, trap behavior or native access.

22. **Should this area advertise shops, inns or additional generic services?** The reviewed source has no local SHP or selected inn, and sector/mobile numeric fields have defined loader meanings. **Recommendation:** Keep service claims tied to an actual callback and admission path; do not infer shops, cold gates, direction blockers or travel from names or misread numeric fields.

These are source-backed review questions and recommendations. Existing source facts, contract ownership, reset declarations, generic dispatch and service gates remain unchanged pending builder decisions.

## Checkpoint 834: Typed source closure rechecked

An exact-source refresh against the unchanged pinned snapshot confirms the typed routes and their boundaries:

- **Imported objects:** Teka2's [ZON](../../../areas/zon/teka2.zon#L113) places Heavens object #358 and White Harbor object #55454 in room #75519. Heavens remains the #358 prototype owner and its maintained binding is `epic_stone`; the White Hills memory remains distinct from Teka2's local rhinestone #75559 and native receipt.
- **Shared local stock:** Alatorin has two active `G` reset groups for Teka2 object #75529, the strange vial of blood ([first](../../../areas/zon/alatorin.zon#L2068), [second](../../../areas/zon/alatorin.zon#L2070)). This is a separate source of local item stock, not a rhinestone turn-in or proof of a Teka2 quest outcome.
- **Typed experience and orientation references:** DLSC's `R E 75500` is 75,500 experience, not an item VNUM; its sidecar preserves that as `E:75500` and separately returns DLSC item #36859 ([QST](../../../areas/qst/dlsc.qst#L26), [story](../../../areas/story/dlsc.story.json#L156)). North Lakes' mention of Teka2 is route orientation, with discovery and quest credit kept separate ([story](../../../areas/story/nlakes.story.json#L13)). Neither creates an outside item consumer of Teka2 prototypes.
- **Compiled roles:** Normal-boot skill bindings still map Aerlyn #75500 to chant mastery and Osmirim #75523 to sneaky strike. The #75561 flaming-mace assignment remains commented out, and the `0x075500` mail-sender check is a library-version constant rather than a prototype ID.

No new QST item command, sidecar prototype role, SHP product, duplicate external prototype, or type-25 destination into Teka2 was found. The existing four item-teleport routes, door/key conditions, room graph, and source ownership remain as documented in the [Dark Stone Tower dossier](../zone-stories/DARK_STONE_TOWER.md). Keep active Teka2 **DO NOT MOVE**; this refresh adds no candidate path or gameplay/migration clearance.

