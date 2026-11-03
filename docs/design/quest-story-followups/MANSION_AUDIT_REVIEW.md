# The Forgotten Mansion: repository-wide connectivity and dependency audit

**Checkpoint 75; priority 115 of the original 220-zone queue.** This supplement
reconciles the comprehensive [Forgotten Mansion dossier](../zone-stories/THE_FORGOTTEN_MANSION.md)
with the active WLD manifest, all inactive WLD inputs, typed references,
compiled numeric bindings, active title inventory, and bounded tracked history.
These are source declarations and code references; they do not establish live
availability, safe travel, or gameplay outcomes.

## Active graph and inactive approaches

All 351 active WLD contributors parsed without issues. The Mansion has 176
rooms and 390 authored exits; all 390 targets resolve. Its weak-exit graph has
four components of 158, 16, 1, and 1 rooms. The sole active outbound is room
#3500 west to Surface #588620 and is reciprocal with Surface's east return
([Mansion](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L46),
[Surface](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1548)). Two active inbounds enter:
that Surface return, plus White House room #55634 northwest (`D6`) to Mansion
#3675 ([White House record](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L803)). The
White House edge has no recorded reverse WLD exit.

All 95 inactive WLD files parsed 527,791 rooms and 2,080,773 exits without
issues. No inactive source-room VNUM overlaps a Mansion room. Two source-only
exits target Mansion #3500: `surf.wld` room #275277 east (`D1`), and
`underworld2.wld` room #436465 east (`D1`). They are inactive map declarations,
not current ingress. Together they raise the curated source-only inbound total
from 675 to 677; the reviewed `surf.wld` subset rises from 106 to 107. The
separate source-room scan found only the unrelated active duplicate #51800 in
Hell Two and Hell Three, not a Mansion collision.

## Typed dependencies and low-VNUM collisions

No external QST item command, SHP product, active external ZON reset-role
target, external mobile/object prototype definition, exact structured story
sidecar VNUM, or external type-25 destination into the Mansion was found. One
raw QST numeric match is `newhaven.qst` `R C 3500`; the `C` reward is currency,
not an item VNUM ([record](../../../areas/qst/newhaven.qst#L187)). The all-source
ZON role scan did find two `G` object-role rows in unregistered
`brass-old-1.zon`: object VNUMs #3532 and #3524. Those prototypes resolve to
the Mansion magma shield and gauntlets, but `brass-old-1` is not in
`areas/AREA`; these are source-only reset references, not active resets
([rows](../../../areas/zon/brass-old-1.zon#L521),
[#522](../../../areas/zon/brass-old-1.zon#L522)). The comprehensive dossier's
foreign-reset statement is narrowed accordingly.

All 231 other story sidecars parsed without issues and contained no exact
structured VNUM matches. The portal scan examined 872 external type-25
declarations and found no Mansion destination. Broad raw text/numeric matches
also include values embedded in IDs, currency amounts, and ordinary scalar
fields; they are not prototype references without a typed lookup context.

## Compiled bindings

The typed CHAOS kit record stores an object VNUM; its role tables directly list
Mansion objects #3536, #3546, #3537, and #3545, which can therefore be supplied
by those kits independent of Mansion source stock
([definition and kit rows](../../../src/account/chaos_eq_data.h#L9),
[#3536/#3546](../../../src/account/chaos_eq_data.h#L90),
[#3537](../../../src/account/chaos_eq_data.h#L134),
[#3545](../../../src/account/chaos_eq_data.h#L245)). The high-drop list contains
Tyrlos mobile #3524, but its bonus loop is commented out, as already recorded
in the dossier ([entry](../../../src/item/randomeq.c#L123)). The
`akh_elamshin` function contains a direct `real_room(3600)` destination to the
Mansion's Sloping Cavern ([callback](../../../src/specs/specs.room.c#L638),
[target room](../../../areas/wld/mansion.wld#L2121)); its symbol has a source
declaration and definition but no active assignment, so it is not an installed
route. The two `3500`/`3600` entries in the dexterity table are scalar
thresholds, not zone bindings. These typed findings are incorporated into the
comprehensive dossier; no new native journal dependency was identified.

## Title, history, and disposition

The normalized active title inventory found one exact `The Forgotten Mansion`
title among 351 contributors. Its color-coded title is in
[`mansion.zon`](../../../areas/zon/mansion.zon#L2); the active source key is in
[`areas/AREA`](../../../areas/AREA#L18). Bounded WLD/ZON history reaches the
2009 area-tree move, with later WLD edits through 2014 and ZON edits through
2015. No distinct predecessor or replacement is identified in the tracked
replacement register. The Mansion remains **DO NOT MOVE** as a current full-
world contributor.

Static Markdown-link/anchor, CSV-structure and queue-count checks passed for
the changed documentation, and `git diff --check` passed. This checkpoint
changes documentation only. It used repository sources and static registers;
no tests, build, server/runtime, gameplay, database, migration, or operational
script was run. Live reachability, safe return, editor/tooling,
persisted-reference, and dynamic-route qualification remain separate.


## Checkpoint 272: direct builder and owner questions

These questions identify the remaining owner choices around the Mansion's exact returns, access, ghost and curse prose, hazards, and cross-zone leads. The recommendations preserve the three native contracts and current route sources until a specific outcome is selected.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should Pianist, Handmaid, and Englehardt remain three independent accepted requests with no mandatory order? | [Pianist request](../../../areas/qst/mansion.qst#L48), [Handmaid request](../../../areas/qst/mansion.qst#L70), [Englehardt request](../../../areas/qst/mansion.qst#L126) | Preserve the three separate giver, offering, reward, and history identities; make any larger campaign explicitly optional. |
| 2 | Should Englehardt accept the exact head and horn without proof that the character personally killed or recovered either demon's drop? | [Englehardt recipe](../../../areas/qst/mansion.qst#L126), [proof distinction](../zone-stories/THE_FORGOTTEN_MANSION.md#L32) | Keep the exact two-item recipe; track personal victory and recovery as separate optional outcomes. |
| 3 | Is the head-and-horn pair intended as one all-or-nothing acceptance, with the key and XP awarded from the same committed return? | [QA result](../../../areas/qst/mansion.qst#L126), [endpoint summary](../zone-stories/THE_FORGOTTEN_MANSION.md#L32) | Keep both items in one bundle and qualify partial inputs, reward settlement, retirement, replay, and recovery before adding history. |
| 4 | Should the three native requests ever become a family-history campaign that requires ALL of them, or should each remain a complete optional endpoint? | [progression model](../zone-stories/THE_FORGOTTEN_MANSION.md#L41), [campaign follow-up](../zone-stories/THE_FORGOTTEN_MANSION.md#L113) | Preserve independent completion; set ALL/ANY, order, spoiler, and personal/shared rules only after selecting an explicit campaign. |
| 5 | Does Handmaid's rattle return permanently quiet the nursery for the world, for the actor, or only in its response text? | [Handmaid response](../../../areas/qst/mansion.qst#L70), [nursery state](../zone-stories/THE_FORGOTTEN_MANSION.md#L53) | Define affected rooms, characters, episode generation, and reset policy before recording a quiet or rescue result. |
| 6 | Does handing in the demon proofs actually resolve Lord Englehardt's house curse, or does the current Q only grant his key and experience? | [Englehardt response](../../../areas/qst/mansion.qst#L126), [curse follow-up](../zone-stories/THE_FORGOTTEN_MANSION.md#L108) | Preserve the accepted key/XP receipt; describe or implement a wider curse transition only after its affected spirits and durable result are chosen. |
| 7 | Should speaking the family name at the library door earn a personal clue or access outcome? | [spoken gate analysis](../zone-stories/THE_FORGOTTEN_MANSION.md#L52), [native clue](../../../areas/qst/mansion.qst#L48) | Track the actor's successful SAY and door state separately from the existing Pianist request. |
| 8 | Should the gardener's wilderness direction be corrected to match the actual west exit from gate room #3500? | [gate source finding](../zone-stories/THE_FORGOTTEN_MANSION.md#L112), [active exit](../../../areas/wld/mansion.wld#L1) | Confirm intended route text, then make a bounded prose change without altering the exit. |
| 9 | Is the White House northwest arrival into treasure room #3675 an intended alternate entrance outside the local key chain? | [White House entry](MANSION_AUDIT_REVIEW.md#L17), [access analysis](../zone-stories/THE_FORGOTTEN_MANSION.md#L59) | Preserve the authored incoming edge and qualify the White House route, arrival, and return before advertising it as ordinary access. |
| 10 | Should the White House entry remain one-way in source, or is a return route intended? | [boundary evidence](MANSION_AUDIT_REVIEW.md#L17), [route qualification](../zone-stories/THE_FORGOTTEN_MANSION.md#L59) | Keep the current edge until its owning map builders agree on a fair, reciprocal route. |
| 11 | Should the Surf and Underworld2 approaches remain source-only references, or does a map owner intend to activate either map? | [inactive approaches](MANSION_AUDIT_REVIEW.md#L22), [candidate records](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L97) | Preserve each inactive source identity and candidate; do not treat either declaration as current access. |
| 12 | Which vault keys and open gates should count as optional exploration when a player reaches a later room through shared or alternate access? | [vault key chain](../zone-stories/THE_FORGOTTEN_MANSION.md#L54), [later doors](../zone-stories/THE_FORGOTTEN_MANSION.md#L58) | Record actual actor-owned item, door, and arrival transitions independently; do not require earlier native quest history for an alternate route. |
| 13 | Are the F50 falls in rooms #3577/#3578 and the F20 treehouse fall at #3672 the intended hazards and survival challenge? | [fall findings](../zone-stories/THE_FORGOTTEN_MANSION.md#L63), [hazard follow-up](../zone-stories/THE_FORGOTTEN_MANSION.md#L118) | Preserve authored values until builders confirm eligible movement states, landing effects, death handling, and recovery. |
| 14 | Should the lava and no-magic/no-teleport room flags remain as authored across the cavern and vault routes? | [room restrictions](../zone-stories/THE_FORGOTTEN_MANSION.md#L64), [environment mapping](../zone-stories/THE_FORGOTTEN_MANSION.md#L15) | Confirm intended spell, travel, and hazard policy; retain current flags until any change is separately reviewed. |
| 15 | Should the epic stone remain a separate shared group touch outcome rather than part of Englehardt's demon request? | [stone settlement](../zone-stories/THE_FORGOTTEN_MANSION.md#L69), [independent process boundary](../zone-stories/THE_FORGOTTEN_MANSION.md#L73) | Keep eligible group touch, payout, and zone completion separate from personal kills, vault gates, and Q completion. |
| 16 | Should Dragon #3541's Winterhaven memory be presented as a cross-zone lead, and what visibility should it have? | [memory reset](../../../areas/zon/mansion.zon#L370), [Winterhaven receipt](../../../areas/qst/wh.qst#L2480), [ownership boundary](../zone-stories/THE_FORGOTTEN_MANSION.md#L77) | Link the foreign receipt only as an explicitly sourced optional lead; preserve Winterhaven as the owner of its accepted return. |
| 17 | Are the vapor and scroll effects intended equipment/use outcomes that should remain separate from resolving the Mansion curse? | [effect analysis](../zone-stories/THE_FORGOTTEN_MANSION.md#L85), [separate outcomes](../zone-stories/THE_FORGOTTEN_MANSION.md#L67) | Keep equipment use and the foreign scroll handler separate from local spirit, curse, and Q history. |
| 18 | What source and giver renewal policy should govern three potential repeatable requests under reset mode 1? | [reset header](../../../areas/zon/mansion.zon#L3), [stock renewal review](../zone-stories/THE_FORGOTTEN_MANSION.md#L114) | Tie daily credit to active READY accounting and actual cap-one supply and giver renewal; do not infer new stock from a date change. |
| 19 | Should the dossier or builder-facing clues mention an extra Tyrlos high-drop bonus when its bonus loop is commented out? | [high-drop source](../../../src/item/randomeq.c#L123), [Tyrlos finding](../zone-stories/THE_FORGOTTEN_MANSION.md#L56) | Keep current reward claims limited to live reset definitions until an active drop rule is confirmed. |
| 20 | Is the roof contraption, chest, and gnomish key meant to form a route into the Mansion? | [foreign roof route finding](../zone-stories/THE_FORGOTTEN_MANSION.md#L94), [binding scan](MANSION_AUDIT_REVIEW.md#L63) | Preserve the actual White House WLD edge; require an owned room, target, and admitted route before adding or describing a special. |

Checkpoint 272 validation: the twenty questions link to the native QST, WLD, ZON, foreign receipt, compiled finding, and source-comprehensive dossier. Existing CP75 graph, typed-reference, binding, title, and history results remain in place. No area or gameplay source changed; no tests, build, server, database operation, migration, or operational script was run.
