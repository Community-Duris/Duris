# The Deep Ravine of Passage: repository-wide connectivity and dependency audit

**Checkpoint 79; priority 119 of the original 220-zone queue.** This supplement
reconciles the [source-comprehensive Deep Ravine dossier](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md)
with the active WLD graph, inactive WLD inputs, typed references, compiled
numeric matches, normalized title, and bounded source history. Authored edges
and references do not establish ordinary reachability, safe traversal, source
availability, or played qualification.

## Active WLD graph

The active manifest contains 351 WLD contributors. The Deep Ravine has 178
rooms and 404 exits; all 404 targets resolve. The local graph has five weak
components of 81, 27, 27, 23, and 20 rooms. Its six outbound boundaries each
have a reciprocal active return: four Surface pairs and two Underdark pairs.

| Deep Ravine edge | Reciprocal edge | Cross-area register rows |
| --- | --- | --- |
| #94700 south to Surface #606867 ([exit](../../../areas/wld/minopass.wld#L13)) | Surface #606867 north to #94700 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1135), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1591) |
| #94721 north to Surface #604472 ([exit](../../../areas/wld/minopass.wld#L466)) | Surface #604472 south to #94721 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1136), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1583) |
| #94721 east to Surface #604873 ([exit](../../../areas/wld/minopass.wld#L470)) | Surface #604873 west to #94721 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1137), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1585) |
| #94818 east to Underdark #832822 ([exit](../../../areas/wld/minopass.wld#L2390)) | Underdark #832822 west to #94818 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1138), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1760) |
| #94818 south to Underdark #833221 ([exit](../../../areas/wld/minopass.wld#L2394)) | Underdark #833221 north to #94818 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1139), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1762) |
| #94878 south to Surface #584060 ([exit](../../../areas/wld/minopass.wld#L3659)) | Surface #584060 north to #94878 | [outbound](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1140), [return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1530) |

There are no unresolved active exits. The all-95 inactive WLD scan parsed
2,080,773 exits without issues and found no source-room overlap. It found four
non-overlapping source-only exits from `surf.wld` into three Ravine rooms:

| Source-only exit | Source record |
| --- | --- |
| `surf.wld` #209185 north to #94878 | [exit](../../../areas/wld/surf.wld#L192777) |
| `surf.wld` #250895 south to #94721 | [exit](../../../areas/wld/surf.wld#L1067959) |
| `surf.wld` #251196 west to #94721 | [exit](../../../areas/wld/surf.wld#L1074276) |
| `surf.wld` #252396 north to #94700 | [exit](../../../areas/wld/surf.wld#L1099440) |

These declarations are not active ingress. They raise the curated source-only
inbound total from 680 to 684 and the individually reviewed Surf subset from
110 to 114. The exact source-only approaches and scope limits are also recorded
in the [Surf component row](ZONE_COVERAGE_REGISTER.csv#L448).

## Typed references and compiled numeric matches

The external QST scan found no command that consumes a local Ravine prototype.
It did find Alatorin's Q7425 reward command `R I 94726`, which returns the
alternate spectral symbol after the First Mountain shard exchange
([QST](../../../areas/qst/alatorin.qst#L7428)). The Alatorin sidecar carries
the same `receive=I:94726` contract in its accepted-exchange payload and
rendered completion ([contract](../../../areas/story/alatorin.story.json#L3942),
[completion](../../../areas/story/alatorin.story.json#L3946)). No explicit
`item_vnums` field in the 231 other parsed sidecars matched a local prototype.

External reset groups import local prototypes into active areas. The table
summarizes the 37 matching active reset commands; the last row is five
additional source-only commands outside `areas/AREA`.

| Source reset | Local prototype and role | Evidence |
| --- | --- | --- |
| `caves_skelenak.zon` | 12 `M` placements of mobile #94739 in rooms #4140, #4141, and #4144 | [reset](../../../areas/zon/caves_skelenak.zon#L203) |
| `brass.zon` | 5 `M` placements of mobile #94740 | [reset](../../../areas/zon/brass.zon#L766) |
| `caertannad.zon` | 5 `O` placements of object #94720 | [reset](../../../areas/zon/caertannad.zon#L406) |
| `tharnrifts.zon` | 8 `O` placements of object #94720 | [reset](../../../areas/zon/tharnrifts.zon#L23) |
| `alatorin.zon` | 1 `G` placement of object #94726 and 4 `G` placements of coin pouch #94730 | [symbol](../../../areas/zon/alatorin.zon#L3237), [pouch](../../../areas/zon/alatorin.zon#L3743) |
| `surfacekeeps.zon` | 2 `G` placements of axes #94733 | [reset](../../../areas/zon/surfacekeeps.zon#L106) |
| `brass-old-1.zon` | 5 `M` placements of mobile #94740; source-only because the stem is absent from `areas/AREA` | [reset](../../../areas/zon/brass-old-1.zon#L762) |

No external SHP product or external MOB/OBJ prototype definition matched, and
this area has no local SHP file. The foreign type-25 scan found no target in a
Ravine room. Relative to the local prototypes, it covered 705 active external
declarations plus 159 source-only declarations, 864 total. The eight local
type-25 objects bring the full active count to 713, matching the existing
dossier's active-only scan.

Six direct C/C++ numeric matches all point to object #94726 in the chaos
equipment table, including [`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L219).
This is an alternate static equipment source for the symbol, not a local
special-procedure binding or proof of player issuance. No other direct local
room, mobile, or object number match was found.

## Title, history, and disposition

The normalized title from [`minopass.zon`](../../../areas/zon/minopass.zon#L2)
is unique among the 351 active contributors. The global active-WLD duplicate
scan found only room #51800 in Hell Two and Hell Three; The Deep Ravine has no
duplicate room VNUM.

Bounded WLD and ZON histories remain on the `minopass` paths back to the
2009-08-12 area-tree move. The latest path entries are the 2026-08-01
line-ending cleanup; ignoring line endings, that commit has no content delta
from its parent. Current WLD room #94772 has the title shown in source and exits
south to #94773; #94773 exits north back to #94772. This is a reciprocal
connection, not a self-exit ([#94772 south exit](../../../areas/wld/minopass.wld#L1446),
[#94773 north return](../../../areas/wld/minopass.wld#L1461)). The June 2026
update/revert sequence also leaves a south return from #94822 to #94821, and a
closed-only rather than closed-and-locked state on the south exit from #94820,
reflected in its WLD flags and ZON reset/comment ([south gate](../../../areas/wld/minopass.wld#L2436),
[reset](../../../areas/zon/minopass.zon#L84), [return](../../../areas/wld/minopass.wld#L2470)).
Room #94821's coordinate field also moves by four units. These retained edits
are part of the current source; the bounded history and curated
duplicate/replacement register identify no separate predecessor or replacement.

The Deep Ravine remains a current, un-commented [`areas/AREA` contributor](../../../areas/AREA#L337)
and is **DO NOT MOVE**. The existing dossier covers the imported reset groups,
Alatorin's alternate reward, local portals, and source behavior. Active and
source-only exits do not qualify door/reset state, ordinary access, hazard
safety, successful use, or return travel. Tooling/editor closure, persisted
references, and played qualification remain separate.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 279: direct builder and owner questions

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should Oesh'lyn's exchange continue to accept the exact spore #94723 and return key #94724 without requiring a prior conversation sequence? | [native contract](../../../areas/qst/minopass.qst#L38), [source index](../../reference/zone-story-audits/minopass.md#native-exchanges) | Preserve the exact item contract; dialogue and earlier arrival remain guidance unless a sequence is explicitly designed. |
| 2 | Should a supplied spore remain valid even when the player did not personally find or take it from a hidden reset container? | [spore reset and container](../../../areas/zon/minopass.zon), [endpoint review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#exact-accepted-endpoints) | Keep supplied-item acceptance; track actual source parent, reset generation, actor, and custody separately for optional provenance. |
| 3 | Should the gates, hidden spore, key use, opening, and passage through the door remain separate access outcomes? | [room and reset sources](../../../areas/wld/minopass.wld), [key and access review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#shared-access-return-routes-and-recovery) | Record successful door-state change and arrival only when each occurs; key possession or text does not prove passage. |
| 4 | Should the native keyword `contracter` remain accepted as authored, or should a corrected alias be added? | [contractor response](../../../areas/qst/minopass.qst#L53), [source index](../../reference/zone-story-audits/minopass.md#dialogue-responses) | Preserve the existing alias and add compatibility only as a deliberate, documented change. |
| 5 | Should Contractor's exchange remain one exact signet-for-key-and-three-potions result with the current giver departure? | [native contract](../../../areas/qst/minopass.qst#L67), [endpoint review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#exact-accepted-endpoints) | Keep the reward objects and NPC departure as separate effects; do not use any one of them as proof of another. |
| 6 | Should the ivory gate and East Wing keep their distinct key checks and door states, even when the player already has a supplied key? | [gate and key records](../../../areas/wld/minopass.wld), [door review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#shared-access-return-routes-and-recovery) | Preserve supplied-key eligibility while requiring the actual addressed door state and successful crossing for access credit. |
| 7 | Should the dwarf spirit continue to require both holy #4402 and unholy #88807 as distinct exact objects? | [spirit contract](../../../areas/qst/minopass.qst#L130), [paired-relic review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#exact-accepted-endpoints) | Keep the native AND condition; duplicates of one relic must not replace the other. |
| 8 | Should the spirit's departure continue to mean a possible spiritual release, without claiming a lasting resurrection? | [native completion](../../../areas/qst/minopass.qst#L130), [spirit outcome review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#capability-and-builder-follow-ups) | Preserve the bounded interpretation until a durable living replacement or other endpoint is authored. |
| 9 | How should the provenance of symbol #94726 distinguish the local paired-relic reward from Alatorin's alternate reward and chaos-equipment references? | [Alatorin exchange](../../../areas/qst/alatorin.qst#L7428), [source review](DEEP_RAVINE_AUDIT_REVIEW.md#typed-references-and-compiled-numeric-matches), [chaos table](../../../src/account/chaos_eq_data.h#L219) | Keep exact item delivery valid; use source lineage only for optional history or attribution. |
| 10 | Should the spore story, signet story, and paired-relic story remain independently completable, or should any earlier receipt gate a later one? | [all four contracts](../../reference/zone-story-audits/minopass.md#native-exchanges), [story sequence review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#capability-and-builder-follow-ups) | Keep native endpoints independent until a complete campaign order, supplied-item route, and recovery policy are approved. |
| 11 | Should Nahasp's service remain exactly 10,000 copper for grate key #94729, without story or achievement credit? | [paid service](../../../areas/qst/minopass.qst#L174), [service review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#exact-accepted-endpoints) | Preserve its economic-service classification and exact fee; do not invent a quest reward. |
| 12 | Should the paid service stay unavailable until debit, key issuance, replay, and recovery work as one recoverable transaction under active accounting? | [service contract](../../../areas/qst/minopass.qst#L174), [payment limits](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#capability-and-builder-follow-ups) | Keep current guards until the full settlement path is qualified; do not record a wallet-ready row as successful payment. |
| 13 | Should grate credit require successful opening and movement after receipt of #94729, rather than the key receipt alone? | [grate source](../../../areas/wld/minopass.wld), [key-state review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#shared-access-return-routes-and-recovery) | Track item receipt, door change, and player arrival as distinct events. |
| 14 | Which remote switch and lever outcomes should count as successful access when the addressed exit is secret or already open? | [switch sources](../../../areas/obj/minopass.obj), [shared switch behavior](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#shared-access-return-routes-and-recovery) | Record before/after exit state and confirmed traversal; an attempted command is not a completed access result. |
| 15 | Are all four declared portal pairs intended to remain usable in both directions despite the blazing-portal warning? | [portal inventory](../../../areas/obj/minopass.obj), [portal review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#shared-access-return-routes-and-recovery) | Preserve the authored return edges until builders choose an explicit restriction or confirm the warning is narrative. |
| 16 | Should the Moonwell's “last chance” warning remain prose, or should it record an irreversible decision? | [Moonwell and sidecar](../../../areas/wld/minopass.wld), [journal source](../../../areas/story/minopass.story.json) | Keep it as prose unless a durable decision state and recovery behavior are designed. |
| 17 | Should healing credit require actual recovery at either healing shrine rather than hearing timed spirit or holy-man text? | [healing flags](../../../areas/wld/minopass.wld), [recovery review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#shared-access-return-routes-and-recovery) | Measure the real healed subject and amount; do not infer healing from an ambient message or room entry. |
| 18 | Should Borthur's timed congratulations remain separate from any rescue, release, escort, or safe-arrival outcome? | [Borthur response](../../../areas/qst/minopass.qst#L87), [rescue review](../zone-stories/DEEP_RAVINE_OF_PASSAGE.md#capability-and-builder-follow-ups) | Do not add rescue credit until its actor, destination, state change, and failure/recovery path are selected. |
| 19 | Should the four `surf.wld` approaches remain source-only on Surf candidate row 97, with no implication of active ingress? | [four approach sources](DEEP_RAVINE_AUDIT_REVIEW.md#active-wld-graph), [Surf candidate](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) | Preserve their existing candidate disposition and keep them separate from the six reciprocal active boundaries. |
| 20 | Should the south exit from #94820 remain closed-only, with #94822 south returning to #94821 as the current source records specify? | [WLD gate and return](../../../areas/wld/minopass.wld#L2436), [ZON door reset](../../../areas/zon/minopass.zon#L84), [return exit](../../../areas/wld/minopass.wld#L2470) | Preserve the current authored flags and return edge unless the area owner selects and validates a replacement state. |

## Checkpoint 779: Caertannad typed dependency

Active Caer Tannad has five object reset rows for Deep Ravine of Passage object #94720. See [Caertannad typed audit](CAERTANNAD_AUDIT_REVIEW.md#outbound-active-reset-references).
