# Village of Refugees: repository-wide connectivity and dependency audit

**Checkpoint 77; priority 117 of the original 220-zone queue.** This supplement
reconciles the source-comprehensive [Village of Refugees dossier](../zone-stories/VILLAGE_OF_REFUGEES.md)
with the active WLD graph, inactive WLD inputs, typed references, compiled
numeric matches, title, and bounded source history. It records source evidence;
normal-play access, service availability, safety, and gameplay qualification
remain separate.

## Active WLD graph

The 351 active WLD contributors parsed without issues. Village of Refugees has
67 rooms and 142 exits; all 142 targets resolve in one weak component of 67
rooms. Its only active outbound exits are from room #98600 north to Surface
#627239 and east to #627640. Both have reciprocal Surface returns, so the
active boundary has two incoming and two outgoing records forming two pairs.
The unresolved-exit register has no Village of Refugees source row.

| Boundary | Evidence |
| --- | --- |
| Village of Refugees #98600 north to Surface #627239 | [Area exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1178); [Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1640) |
| Village of Refugees #98600 east to Surface #627640 | [Area exit](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1179); [Surface return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1643) |

## Inactive WLD approach

All 95 inactive WLD files parsed without errors: 527,791 rooms and 2,080,773
exits. No inactive source room overlaps an active room VNUM. One source-only
exit from `surf.wld` room #264438 west (direction 3) targets Village of
Refugees #98600 at [`surf.wld`](../../../areas/wld/surf.wld#L1352162). It is
not active ingress and raises the curated source-only inbound total from 679
to 680 and the individually reviewed Surf subset from 109 to 110.

## Typed external references

| Source and role | Classification |
| --- | --- |
| [`newhaven.qst`](../../../areas/qst/newhaven.qst#L340), `G I 98606` | Active Newhaven Q331 item requirement for the Village of Refugees' frayed collar #98606. This foreign receipt remains Newhaven's. |
| [`newhaven.story.json`](../../../areas/story/newhaven.story.json#L488), item VNUM #98606 | One exact structured sidecar reference across 231 parsed sidecars; no parse issues. The item belongs to Newhaven's collar recipe. |
| [`newbie.shp`](../../../areas/shp/newbie.shp#L8), shop #29201 product #98621; [`newbie.zon`](../../../areas/zon/newbie.zon#L222), `G` object #98621 | Active shop and reset references to the local torch prototype. |
| [`alatorin.zon`](../../../areas/zon/alatorin.zon#L2813), `M` mobile #98632 | Active Alatorin reset placement for the Village of Refugees' skeletal draft horse. |
| [`nexus56.shp`](../../../areas/shp/nexus56.shp#L549), shop #83039 product #98618; [`nexus56.zon`](../../../areas/zon/nexus56.zon#L1389), `G` object #98618 | Source-only references to the local waterskin from an unregistered area. The reset is outside the normal `areas/AREA` source list. The shop file has a conditional loader fallback described below. |

No external mobile/object prototype definition matched Village of Refugees.
None of 871 scanned external type-25 declarations targets it. All 231 other
story sidecars parsed without issues and had one exact structured VNUM match,
the Newhaven collar reference above.

`nexus56` is absent from the current [`areas/AREA` manifest](../../../areas/AREA).
In non-mini mode, [`open_shop_stream()`](../../../src/economy/shop.c#L3272)
tries `areas/world.shp`, then its fallback globs every `areas/shp/*.shp`
([fallback branch](../../../src/economy/shop.c#L3278)). This checkout has no
`areas/world.shp`, so the unregistered shop record has a possible fallback
reader path. That source path does not prove an operating shop in a deployed
world; no runtime was started. The fallback applies to SHP input, not the
unregistered `nexus56.zon` reset.

## Direct compiled references

The repository-wide numeric scan found no direct C/C++ numeric match for the
67 room, 44 mobile, or 68 object VNUMs. The existing dossier records no literal
local special assignment and keeps the shared teleport, item-switch, quest,
and source-owner behavior distinct from scenery and foreign receipts.

## Title, history, and disposition

The active title is [`Village of Refugees`](../../../areas/zon/ruins.zon#L2).
Its normalized title is unique among 351 active contributors. The global WLD
duplicate-room scan found only room #51800 shared by Hell Three and Hell Two;
Village of Refugees has no duplicate room ID. The curated duplicate/replacement
register has no distinct predecessor or successor entry for this area.

Bounded WLD/ZON `git log --follow` histories keep the files on their existing
`ruins` paths back to the 2009-08-12 area-tree move. The latest tracked WLD edit
is 2013-11-13 and the latest tracked ZON edit is 2016-08-29. This path history
does not identify a separate predecessor or replacement area.

Village of Refugees remains a current, un-commented [`areas/AREA` contributor](../../../areas/AREA#L356)
and is **DO NOT MOVE**. The source-only Surf approach and typed references do
not establish ordinary reachability, safe traversal, permanent rescue, or an
operating fallback shop. Live door behavior, dynamic travel, editor/tooling
closure, persisted references, and played qualification remain pending.

This checkpoint used repository sources and generated documentation only. No
tests, build, server, gameplay session, database operation, migration, or
operational script was run.

## Checkpoint 344: reset-aware reach, refreshed source closure, and builder decisions

The active WLD file has 67 rooms and 142 exits: 140 local and two outbound Surface exits, all resolving in one weak component. Both Surface approaches arrive at room 98600 and have reciprocal exits from 98600 north to 627239 and east to 627640 ([cross-area exit records](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1178)). In the raw directed local graph, all 67 rooms are reachable from 98600 and have a path back to it.

The zone resets declare six paired door states at 100% ([reset block](../../../areas/zon/ruins.zon#L8), [remaining pairs](../../../areas/zon/ruins.zon#L24)). They are: 98607/98610 closed and blocked (state 9); 98609/98629 closed and secret (state 5); 98614/98615 open and secret (state 4); 98624/98628 closed (state 1); 98626/98627 closed and locked (state 2); and 98665/98666 closed (state 1). With state 1, 2, 5, and 9 edges filtered, 45 of 67 rooms remain mutually reachable from 98600. The filtered set excludes 98610, 98627–98646, and 98666. This is a conservative reset-state graph; it does not test door commands, key ownership, player access, or safe traversal.

Eight rooms declare 10% falls: 98600, 98601, 98602, 98613, and 98629–98632 ([entry trail](../../../areas/wld/ruins.wld#L1), [lower rooms](../../../areas/wld/ruins.wld#L547)). No local room declares a current field. The 98665/98666 closed barn door also leaves the horse yard outside the reset-filtered route from the entrance.

The all-95-file inactive-WLD sweep still has one non-overlapping source-only approach: surf.wld #264438 west to #98600 ([source](../../../areas/wld/surf.wld#L1352162)). It was already counted at Checkpoint 77 and is retained on existing Surf candidate row 97. It is not active ingress; current curated source-only totals remain 4,495 overall, Surf 259, ud_map1 two, ud_map2 20, and Underworld2 eight. Surf active references remain 145.

The bounded typed closure remains exact: active Newhaven QST and story sidecar each reference local object #98606; active Newbie SHP/ZON refer to local torch #98621; active Alatorin ZON resets local mobile #98632; and source-only nexus56 SHP/ZON refer to local waterskin #98618. The possible SHP fallback path is described above and does not establish an operating shop. Across current object files, 712 active and 159 source-only type-25 declarations were checked; none targets a Village room. No outside same-type mobile/object definition, direct C/C++ number match, or additional typed QST/SHP/story/ZON reference matched the local prototypes. The external sidecar scan parsed 231 files and found the one Newhaven reference already recorded.

The normalized title Village of Refugees is unique among 442 current ZON titles and 351 active contributors. Followed history stays on the same ruins stem: WLD last changed in 2013, MOB in the 2026 warning cleanup, OBJ and ZON in 2016, QST in 2013, SHP in 2014, and the story sidecar in 2026. No alternate-stem predecessor or replacement was found. The global duplicate-room result remains one unrelated collision at #51800 in Hell Two/Hell Three; no Village room or prototype number is duplicated by another same-type source.

### Builder questions and recommended resolutions

1. **Does the sidecar's complete status describe the intended scope?** It maps the farmer's proof, Anguinel's paid craft, three contacts, and 17 ASK aliases. **Recommendation:** preserve the two distinct contract outcomes and keep discovery, service, and rescue claims separate.
2. **Which Anguinel fee is intended?** Q16 lists 1,000 copper while his dialogue asks for 10 platinum. **Recommendation:** retain the currently gated service and ask the owner to choose one fee before a separately reviewed repair.
3. **Which white-feather bird name is intended?** Anguinel says Snowy Owl, but the source mobile is a screech owl. **Recommendation:** confirm the name with the builder before changing either mobile or authored clue.
4. **Is the raven-feather placement intentional?** The pitch-black feather is placed inside the broken gate rather than carried by the raven. **Recommendation:** decide whether the gate is the intended source and retain exact item identity until then.
5. **Should personal feather gathering be part of the service story?** Matching supplied feathers also satisfy the recipe; loose item possession does not prove who gathered them. **Recommendation:** keep current supplied-item semantics unless a source-tracked gathering outcome is explicitly designed.
6. **Does the craft remain a service without achievement or daily credit?** The dossier classifies Anguinel's paid exchange separately from the farmer's proof. **Recommendation:** preserve that separation and the active-accounting gate pending service qualification.
7. **Are the farmer's many topics one dialogue chain?** The eight dialogue blocks are aliases within one native proof request, not eight completions. **Recommendation:** retain one completion and do not assign credit to topic count.
8. **Should the blocked table passage record any separate story progress?** PULL TABLE opens a shared switch-controlled exit. **Recommendation:** record a personal access step only if a successful actor-specific action can be observed; opening by another character is not personal proof.
9. **Is ENTER WELL the intended route, and how should its hazard be described?** The local type-25 well teleports to the underground route, where 10% fall fields apply. **Recommendation:** keep the route and fall semantics together in guidance; do not call the trip safe from static source.
10. **Are the six reset door pairs and their state flags intended?** The source contains blocked, secret, open-secret, closed, locked, and barn-door states. **Recommendation:** confirm each with the area owner before changing reset values or treating a door as traversable.
11. **Should the attic clue route remain open-secret?** The pair 98614/98615 leads into the hidden wedding area and resets open/secret. **Recommendation:** preserve its secret flag and verify ordinary discovery separately from raw graph reach.
12. **Is the lower chamber's strict isolation behind the well door intended?** Closing state-5 pair 98609/98629 makes rooms 98627–98646 unreachable from 98600. **Recommendation:** verify door/key route and reset timing in a live review before revising its journal description.
13. **Are all eight 10% fall fields intentional?** They are on trail rooms and lower rooms 98629–98632. **Recommendation:** confirm the hazards and their messages together; do not remove fall fields as part of a story-only edit.
14. **What should the moaning spirit's wedding clues count as?** A ghost key, veil, and note point to one hidden chest. **Recommendation:** keep these as clues unless the builder defines an examined/read outcome with exact text and ownership.
15. **What should the mourning spirit's cedar-chest clues count as?** Its topic leads to a cummerbund and faded letter in a separate house. **Recommendation:** preserve this as a distinct clue chain and avoid inferring acceptance or reconciliation.
16. **Is freeing the captive a separate desired story?** The farmer accepts an eyestalk; the prisoner and cages are separate source entities. **Recommendation:** require an actual release and durable rescue outcome before writing a rescue completion.
17. **What reset outcome is intended for the farmer and his household?** The farmer leaves after accepting proof; the spouse, animals, follower chain, and barn have independent resets and caps. **Recommendation:** preserve those reset identities and confirm renewal/household state with the reset owner.
18. **Should Newhaven continue to own the collar exchange?** Its QST and sidecar consume local collar 98606 with an external fee and reward. **Recommendation:** preserve Newhaven's foreign receipt and do not create a duplicate Village completion for carrying the collar.
19. **Are the Newbie torch and Alatorin horse consumers intended?** Active files reset local torch 98621 and skeletal horse 98632. **Recommendation:** preserve these consumers and coordinate any prototype change with their area owners.
20. **What is the disposition of the Nexus56 and Surf source-only records?** nexus56 references local waterskin 98618 and may be exposed to the conditional shop-file fallback; inactive surf.wld #264438 approaches 98600. **Recommendation:** retain both on their existing source owners, label them source-only, and do not add an attic candidate or claim an operating Nexus56 shop.

### Safe disposition

Village of Refugees remains active and DO NOT MOVE. Preserve both active reciprocal Surface pairs, local story and service identities, foreign Newhaven/Newbie/Alatorin consumers, and the source-only Nexus56 and Surf records. Static reset-aware reach is now documented; ordinary access, played safety, service operation, source ownership by a character, editor/tooling references, and persisted state remain outside this source-only qualification. No gameplay, area, or migration change is proposed.
