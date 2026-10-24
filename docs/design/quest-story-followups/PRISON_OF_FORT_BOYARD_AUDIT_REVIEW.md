# Prison of Fort Boyard: graph, dependency, history, and retention reconciliation

Checkpoint 102 · Priority 142 · active area [`prisonb`](../../../areas/AREA#L204) · zone 430; title at [`prisonb.zon`](../../../areas/zon/prisonb.zon#L2).

This supplement adds repository-wide graph, inactive-map, typed-dependency,
source-history, and retention evidence to the [comprehensive Prison of Fort
Boyard dossier](../zone-stories/PRISON_OF_FORT_BOYARD.md) and [source audit
index](../../reference/zone-story-audits/prisonb.md). The dossier remains the
detailed story, access, reset, and builder-follow-up account; this file records
the broader cross-area closure. Source references are static repository
declarations. They do not establish played availability or live reachability.

## Active graph and cross-area routes

The active WLD has 52 Prisonb rooms and 108 exits; all 108 targets resolve. The
authored graph contains 106 local exits and two outbound exits into Fort Boyard.
The local undirected projection has three weak components of 23, 22, and 7
rooms. No fall-chance or current markers were found. Both active inbound
boundaries have reciprocal Prisonb exits:

| Inbound source | Prisonb return | Register |
| --- | --- | --- |
| [`fortb.wld`](../../../areas/wld/fortb.wld#L395) #38121 east → #43041 | #43041 west → #38121 | [cross-area rows 544 and 583](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L544) |
| [`fortb.wld`](../../../areas/wld/fortb.wld#L993) #38149 east → #43000 | #43000 west → #38149 | [rows 545 and 582](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L545) |

The complete scan of all 95 WLD files outside the active manifest parsed
527,791 rooms and 2,080,773 exits with no parse issues. It found no source-room
VNUM overlap with active rooms and no approach into Prisonb. The local ship is
a separate dynamic path: active [`fortb.zon`](../../../areas/zon/fortb.zon#L94)
stocks local ship object #43014 at Fort Boyard room #38106, and its type-25
ENTER target is room #43020. The static graph census does not prove that a
player can use the ship, traverse any current door state, or safely return.
See the [world connectivity report](WORLD_CONNECTIVITY.md) and the full
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv).

## Typed inbound and outbound dependencies

The local QST has no exact outside object inputs or outputs. The local story
sidecar has no typed external `item_vnums`; the area has no local SHP file.
Across reviewed external QST, SHP, and story-sidecar references, no typed item
hit targets Prisonb. No external mobile or object prototype duplicates a
local VNUM, and the external type-25 prototype scan found no target into
Prisonb. The active cross-area reset consumer is:

| Source | Target | Effective classification |
| --- | --- | --- |
| [`fortb.zon`](../../../areas/zon/fortb.zon#L94), O reset at room #38106 | Local object #43014, type 25 | Active external reset source for the Prisonb ship; ENTER7 leads to #43020. The reset comment says the prototype is not in `fortb.obj`; the prototype is defined in `prisonb.obj`. This is a real source dependency, not an outside recipe or typed QST item input. |

Prisonb's own reset file imports object #359 for the epic stone at room #43018
([`prisonb.zon`](../../../areas/zon/prisonb.zon#L189)) and object #67230 for
green-dragon loot ([reset row](../../../areas/zon/prisonb.zon#L306)). The two
imports are already explained by the dossier and do not identify an attic
dependency.

The WH journal adds a narrative-only nearby Fort Boyard reference. Its
[`wh.story.json`](../../../areas/story/wh.story.json#L1027) request asks for
memory object #55173, says to recover it from Master Boyard in Fort Boyard,
and deliver it to WH ambassador #55202. The matching native [`wh.qst`](../../../areas/qst/wh.qst#L2800)
exchange accepts #55173 and grants WH rewards. Both prototypes belong to WH,
not Prisonb; this is not a typed Prisonb dependency or a prerequisite for the
scales/cloak exchange. Master Boyard is Fortb mobile #38131
([prototype](../../../areas/mob/fortb.mob#L468)), which makes `fortb` the
explicit geographic/story lead. Builder question: is the WH memory intended
continuity with nearby Fort Boyard, and should its separate WH receipt remain
distinct from the Prisonb story?

Three selected compiled-data references place cloak #43015 in two chaos kit
tables and crown #43013 in one enhanceable summoner table
([`chaos_eq_data.h`](../../../src/account/chaos_eq_data.h#L165),
[#414](../../../src/account/chaos_eq_data.h#L414), and
[#551](../../../src/account/chaos_eq_data.h#L551)). They are source-context
matches, not quest callbacks or additional typed dependencies. The dossier's
existing no-shop finding remains unchanged.

## Title, tracked history, and disposition

The normalized active `.zon` title “Prison of Fort Boyard” has one owner among
351 current non-empty active titles. Followed `prisonb.wld` history begins with
the repository-wide `areas/` move `642da5400` (2009-08-12), then records the
2013-11-13 edit `71d731c5b` and 2015-10-13 quest-item flag edit `f18992b8f`.
Followed `prisonb.zon` history contains the 2009 move and the 2013 edit. No
followed alternate-stem rename, distinct predecessor, or explicit replacement
was found. The story-specific commit `f3ae391a0` (2026-10-06, “Add Fort Boyard
prison journal and combined scale progression”) documents the current scope;
it does not claim to replace another zone.

The active `prison` area is separately titled “The Prisons of Carthapia” and
uses VNUMs 7300–7498; Prisonb uses 43000–43051. Both are active contributors
([`areas/AREA`](../../../areas/AREA#L41),
[`prison.zon`](../../../areas/zon/prison.zon#L2), and
[`prisonb.zon`](../../../areas/zon/prisonb.zon#L2)). Similar subject matter
does not establish replacement, and the current title, ranges, and followed
history show no predecessor relationship.

Prisonb remains **DO NOT MOVE** as an active area contributor. Preserve its two
reciprocal Fortb boundaries and the active Fortb reset source for ship #43014.
No inactive-WLD ingress or other candidate dependency was found, so this
checkpoint adds no attic-plan candidate and changes no area membership. The
dossier now has fourteen builder follow-ups, including the separate WH
narrative-continuity question. No native gameplay or area-source change ships
with this supplement.

## Checkpoint 330: reset-aware arrival reach and builder decisions

The Prisonb source files remain at coverage snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697; no later source diff changes the
52-room WLD, 44 door resets, QST, story sidecar, prototypes, or reset roles.
This checkpoint adds a traversal that applies the current ZON D states to
the authored local exits and an explicit set of builder questions. It does
not claim live state or played access.

The WLD has 52 rooms and 108 exits, with weak components of 23, 22, and 7
rooms. The two active Fortb boundaries enter #43000 from #38149 and #43041
from #38121. Raw directed local WLD reach and strict initial-open reach from
each source entry are:

| Arrival or dynamic landing | Raw local directed reach | Strict initial-open reach |
| --- | ---: | ---: |
| Fortb #38149 east to #43000 | 22/52 | 7/52 |
| Fortb #38121 east to #43041 | 7/52 | 4/52 |
| Ship object #43014 lands at #43020 | 23/52 | 10/52 |

The strict room sets are #43000–#43004 plus #43050–#43051 from the first
entry, #43041–#43044 from the second, and #43020–#43023, #43025–#43026,
#43028–#43029, #43032, and #43035 from the ship landing.

The 44 D resets set 20 exits closed, 20 closed and locked, two open, and two
blocked; no D reset sets a secret flag. The D reset at #43004 east to #43005
closes and locks the prison entrance, with key #43000 reset on the entrance
guard in #43004. The ship component has its own holding-cell key #43020.
Strict reach treats closed, locked, and blocked exits as unavailable; it
does not model the player obtaining or using keys, or later door changes.
The two ITEM_SWITCH levers can clear the authored blocked north exits
#43014-to-#43015 and #43016-to-#43018. Ship object #43014 and the plank at
#43022 provide the separate ENTER and JUMP travel path described in the
dossier. Neither switch state nor travel safety is proved by this static
count. Reset interpretation follows [setup_dir](../../../src/world/db.c#L1545),
the [zone D reset handler](../../../src/world/db.c#L7857), and the
[exit-state flags](../../../src/core/defines.h#L443).

The normalized title remains unique among 442 current zone titles and the
351 active contributors. The earlier bounded comparison found distinct
current titles and no exact normalized room-title or full-description
overlap with Carthapia prison. Followed WLD/ZON history and the current text
do not identify a predecessor or replacement. The full 95-source inactive
WLD scan found no Prisonb source-room overlap or approach. Retain the two
reciprocal Fortb boundaries and Fortb's active reset of local ship #43014;
do not add an attic candidate or move either active contributor from this
review.

## Checkpoint 330 builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PB-01 | Should Gixmo continue to accept exactly one white, blue, green, red, and black scale together for one cloak? | Preserve the five exact item VNUMs and one native receipt. Do not substitute duplicates, worn or nested items, a second cloak, or dialogue. |
| PB-02 | Does the owner want personal dragon kills or first-source recovery to be prerequisites for the scale exchange? | Keep the current item exchange separate from personal victory and recovery history; any new credited event needs committed actor, source, transfer, and participant authority. |
| PB-03 | Should world-cap-one scale and giver stock renew when a character's daily eligibility renews? | Preserve current cap, reset, and daily behavior. A daily rollover does not promise restock; qualify actual availability before making that claim. |
| PB-04 | Is entrance key #43000, held by the guard at #43004, the intended key for the closed/locked prison entrance? | Preserve the guard, key, D2 state, and pickproof WLD door. Confirm the intended discovery route before changing stock or the barrier. |
| PB-05 | Should access remain shared when another player has already opened the guard or cell doors? | Keep shared door state separate from individual history; do not claim the exchange records who obtained a key or opened a door. |
| PB-06 | Are the two levers intended to clear blocked exits #43014 north and #43016 north with PULL 340? | Preserve exact lever, room, command, and target roles. Treat successful mutation as shared access and an already-clear target as a no-op. |
| PB-07 | Should the Fortb ship's ENTER route and Prisonb plank's JUMP return remain unlimited-charge travel? | Preserve the type-25 target #43020 and plank return to #38106; do not describe a ticket, moving voyage, or verified safe journey. |
| PB-08 | Are prison key #43000 and ship holding-cell key #43020 intended to govern distinct groups of doors? | Keep the two key custodians and door groups separate; do not infer key possession or successful passage from room entry. |
| PB-09 | Should any captive be rescuable, teach, or relocate through an authored mechanic? | Keep the five captive stories as prose until a builder names the intended prisoner, successful state change, participants, recipient, and reset behavior. |
| PB-10 | Should imported epic stone #359 and its group TOUCH reward contribute to the cloak story? | Keep committed stone completion and reset authority separate from Gixmo's Q43 receipt and from personal dragon victories. |
| PB-11 | Does the zone's mode-0 header and Gixmo's daily eligibility imply recurring scale restock? | Preserve the current reset mode and daily policy. Do not promise recurring stock; the stone's committed reset request is a separate event. |
| PB-12 | Is Gixmo's paper and magical-spell prose intended as a real prerequisite, given Q43 accepts five scales only? | Keep the native exact recipe authoritative. Confirm flavor versus planned clue; the blank shipping note is not an exchange input. |
| PB-13 | Are the blue/green color statements in the red and black dragon descriptions copy errors? | Confirm intended identities before a separately scoped prose correction; keep dragon VNUMs, drops, stock, and combat unchanged. |
| PB-14 | Are the captive names, pronouns, and escape wishes intended to distinguish the land cells from the ship cells? | Preserve current mobile identities and avoid a rescue claim. Resolve each bounded text correction with the builder before changing prose. |
| PB-15 | Should the four shared shipping crates remain closed and locked with key value zero while food is reset inside them? | Preserve container and reset prototypes until actual placement and intended picking/opening rules are confirmed; track container instance and parentage for any recovery credit. |
| PB-16 | Is room #43051's northern door and water description scenery, or is a north exit missing? | Confirm intended map design before altering topology; preserve the valid south route and report the current source exit count accurately. |
| PB-17 | Is the Winterhaven memory-of-Fort-Boyard request intended to share continuity with Master Boyard in Fortb? | Confirm the lore relationship while keeping WH object #55173, ambassador #55202, and its receipt distinct from Prisonb's Q43. |
| PB-18 | Should Fortb remain the active reset source for Prisonb ship object #43014? | Preserve the O reset at Fortb #38106 and its local prototype/ENTER target; any future path move must account for both areas and the ship return. |
| PB-19 | Are local imports #359 and #67230 and the cloak/crown chaos-kit rows intended non-quest uses? | Retain their current typed and compiled-data classifications; do not treat them as extra QST inputs, alternate receipts, or evidence that the zone is replaceable. |
| PB-20 | Should Prisonb remain an active in-place area with both reciprocal Fortb entrances and the external ship producer? | Keep Prisonb DO NOT MOVE and add no candidate. Any separate migration proposal must first resolve both boundary pairs, the Fortb reset dependency, and source/editor consumers. |

These twenty decisions make the coverage register's builder field explicit;
they retain rather than replace the dossier's fourteen implementation
follow-ups. No native gameplay, area membership, or candidate count changes
in this checkpoint.
