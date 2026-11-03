# Lair of the Purple Worm: graph, dependency, history, and retention reconciliation

Checkpoint 103 · Priority 143 · active area [`pworm`](../../../areas/AREA#L201) · zone 425; title at [`pworm.zon`](../../../areas/zon/pworm.zon#L2).

This supplement adds repository-wide graph, inactive-map, typed-dependency,
source-history, and retention evidence to the [comprehensive Lair of the
Purple Worm dossier](../zone-stories/LAIR_OF_THE_PURPLE_WORM.md) and [source
audit index](../../reference/zone-story-audits/pworm.md). The dossier remains
the detailed story, access, reset, and builder-follow-up account; this file
records the broader cross-area closure. Source references are static repository
declarations. They do not establish played availability or live reachability.

## Active graph and reciprocal boundaries

The active WLD has 98 `pworm` rooms and 216 exits: 212 local and four outbound.
All 216 targets resolve in the 351-file active WLD manifest. The local
undirected projection is one 98-room weak component. All four active inbound
boundaries have reciprocal local exits:

| Inbound source | `pworm` return | Register |
| --- | --- | --- |
| [`underworld.wld`](../../../areas/wld/underworld.wld#L6168) #4618 west → #42588 | #42588 east → #4618 | [rows 81 and 578](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L81) |
| [`underdark.wld`](../../../areas/wld/underdark.wld#L184966) #814263 east → #42583 | #42583 west → #814263 | [rows 1733 and 577](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L577) |
| `underdark.wld` #832712 west → #42500 | #42500 east → #832712 | [rows 1759 and 575](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L575) |
| `underdark.wld` #833111 north → #42500 | #42500 south → #833111 | [rows 1761 and 576](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L576) |

These eight authored boundary records are static map declarations. They do not
prove ordinary-player access, loaded door state, encounter availability, or
safe return. The complete local graph and its shared-door/body qualifications
remain in the [dossier](../zone-stories/LAIR_OF_THE_PURPLE_WORM.md); the
[cross-area register](WORLD_CROSS_AREA_EXIT_REGISTER.csv) records exact rows.

## Inactive WLD sweep and source-only approaches

The all-95 WLD files outside the active manifest parsed 527,791 rooms and
2,080,773 exits with no parse issues. No source-room VNUM overlapped an active
room. Three non-overlapping source-only exits lead into `pworm` from inactive
[`ud_map2.wld`](../../../areas/wld/ud_map2.wld):

| Inactive source | Active target | Evidence |
| --- | --- | --- |
| #1123784 west → #42525 | `pworm.wld` #42525 | [`ud_map2.wld` source/exit/target](../../../areas/wld/ud_map2.wld#L41908); active room at [`pworm.wld`](../../../areas/wld/pworm.wld#L586) |
| #1123975 east → #42542 | `pworm.wld` #42542 | [source/exit/target](../../../areas/wld/ud_map2.wld#L44824); active room at [`pworm.wld`](../../../areas/wld/pworm.wld#L1004) |
| #1124682 north → #42500 | `pworm.wld` #42500 | [source/exit/target](../../../areas/wld/ud_map2.wld#L55296); active room at [`pworm.wld`](../../../areas/wld/pworm.wld#L1) |

These declarations are not active ingress and do not add active graph edges.
They raise the curated source-only approach total from 1,514 to 1,517; the
reviewed Surf subset remains 130. `ud_map2` also has a separate active Shafts
exit whose target is absent from the active world, plus previously recorded
source-only approaches into Ixarkon, The Caverns of the Worms, and Khildarak.
Preserve all seven source-only approaches in the candidate-specific plan; do
not restore or move `ud_map2` on this evidence alone. See the
[`ud_map2` retention plan](ATTIC_MIGRATION_PLAN.md#L35) and
[candidate dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv).

## Typed inbound and outbound dependencies

The local QST item commands use local prototypes. No typed external item
reference appears in the local story sidecar, the area has no SHP source, and
the local reset file has no role reference outside the local prototype/range
sets. Across the reviewed external QST, SHP, and story-sidecar files there is
no typed item hit into `pworm`; all 231 outside sidecars parsed without error.
No external mobile or object prototype duplicates a local VNUM. The external
type-25 scan covered 872 declarations and found no target into `pworm`.

One active external reset-role dependency does point to a local mobile:

| Source | Target | Effective classification |
| --- | --- | --- |
| [`alatorin.zon`](../../../areas/zon/alatorin.zon#L4410) M rows 4410–4412, 4423–4425, and 4427–4429 | [`pworm.mob`](../../../areas/mob/pworm.mob#L17) mobile #42501 | Nine active reset declarations place the infant purple worm in Alatorin rooms #83972, #83974, and #83975; each row declares cap 9 and chance 100. Their comments say the mob type is not in the Alatorin mob file. This is a cross-area reset-role dependency, not a quest item input or proof that the reset executed. |

Builder question: are Alatorin's nine reset rows an intended use of the shared
`pworm` mobile prototype? If so, document that ownership and preserve the
cross-area reset contract. If not, decide the owner and prepare a separate
reset/source correction with availability and balance review; do not silently
duplicate or remove the prototype.

## Title, tracked history, and disposition

The normalized active `.zon` title “Lair of the Purple Worm” has one owner
among 351 current non-empty active titles. Followed `pworm.wld` history starts
with the repository-wide `areas/` move `642da5400` (2009-08-12) and includes
`71d731c5b` (2013-11-13). Followed `pworm.zon` history includes those changes
and `f18992b8f` (2015-10-13), which updated quest-item flags. The current story
commit `f591e1900` (2026-10-06, “Add Purple Worm journal and seven-hide family
progression”) documents the present recipe. No alternate-stem rename,
distinct predecessor, or explicit replacement was found.

The active `worms` area is separately titled “The Caverns of the Worms” and
uses VNUMs around #6900; `pworm` uses #42500–#42598. Both are active
contributors, and the similar theme does not establish one as a copy or
replacement ([`areas/AREA`](../../../areas/AREA#L38),
[`worms.zon`](../../../areas/zon/worms.zon#L2), and
[`pworm.zon`](../../../areas/zon/pworm.zon#L2)). Preserve both source
identities.

`pworm` remains **DO NOT MOVE** as an active area contributor. Keep the four
reciprocal current boundaries and the active Alatorin reset-role dependency
visible. The three `ud_map2` approaches remain source-only; the candidate plan
retains that source because its identity, active Shafts reference, and other
source-only routes still require owner review. No source files were moved and
no native gameplay or area-source change ships with this supplement.

## Checkpoint 331: reset-aware arrival reach and builder decisions

The pworm source files remain at coverage snapshot
72ba3deff3bb30bde880018cdebe7c6db675b697; no later source diff changes the
98-room WLD, six door resets, QST, story sidecar, prototypes, or reset roles.
This section applies the current ZON D states to the authored directed WLD
exits. Each traversal starts inside pworm after arrival; it does not prove
that a player can reach that entry in the live world.

The four reciprocal active boundaries have three distinct pworm arrival
rooms: #42588 from Underworld #4618, #42583 from Underdark #814263, and
#42500 from Underdark #832712 and #833111. Raw directed WLD reach from each
room is 95/98. Strict initial-open reach is:

| Arrival room | Raw directed reach | Strict initial-open reach | Rooms absent from strict reach |
| --- | ---: | ---: | --- |
| #42588 | 95/98 | 92/98 | #42500, #42511, #42564, #42594–#42596 |
| #42583 | 95/98 | 92/98 | #42500, #42511, #42564, #42594–#42596 |
| #42500 | 95/98 | 1/98 | Every local room except #42500 |

All six [ZON D rows](../../../areas/zon/pworm.zon#L8) set a closed/unlocked
state; none sets secret or blocked. The four reciprocal boundary records are
listed in the [active graph section](#active-graph-and-reciprocal-boundaries).
The entrance north of #42500 is closed/unlocked, so strict initial-open
reach from that room stops at the arrival room. From #42588 and #42583, the
closed reset directions at #42511 and #42564 remove those two rooms from the
initial-open traversal. All three directed traversals miss loading rooms
#42594, #42595, and #42596 even though the local undirected graph has one
98-room component. This is a static WLD result, not proof that reset loading,
wandering, or another runtime mechanic cannot place a character there. The
loader's reset-state handling is in [db.c](../../../src/world/db.c#L7857).
The three ud_map2 approaches stay source-only on its existing candidate row
114; they do not add active ingress or alter its one active Shafts
reference.

The title remains unique among 442 current zone titles and 351 active
contributors. Followed WLD/ZON history has no predecessor or replacement;
the active worms area is separately titled and numbered. The all-95 inactive
WLD scan found no pworm room overlap; its three ud_map2 approaches are already
recorded and no additional candidate or approach is introduced here. Preserve
all four active reciprocal boundaries and Alatorin's nine active reset rows
for mobile #42501 when reviewing any separate path migration.

## Checkpoint 331 builder questions and recommended resolutions

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PW-01 | Should Draknahov continue to accept one amulet #42502 and exactly seven hides #42500 together for leggings #42504? | Preserve the exact recipe multiset and one accepted native exchange; extra armor, worn/nested proof, dialogue, or reward possession does not substitute. |
| PW-02 | Are seven supplied hides intended to prove seven personal kills or first recoveries? | Keep current inventory readiness separate from personal victory and source history. Any new credit needs committed actor, source, transfer, container, and participant authority. |
| PW-03 | Is amulet #42502 in corpse-container #42503 meant to be extracted loose before it qualifies? | Preserve the current loose-item contract and source reset; separate finding, opening, extraction, first-source recovery, and accepted turn-in. |
| PW-04 | Should the recovered amulet establish that Draknahov's brother was rescued or relocated? | Keep the emotional recognition text separate from a rescue outcome; no local Q records a living brother, recipient, or relocation. |
| PW-05 | Should the amulet dialogue response announce the ending even when the seven-hide exchange is incomplete? | Keep dialogue and native acceptance distinct; add a separate clue-text proposal that names all seven hides without changing the recipe. |
| PW-06 | Are the mouth and body descriptions meant to match the east and diagonal directions actually authored? | Confirm intended wording, then propose a bounded text fix. Do not add a north exit, swallow gate, or teleport merely to match prose. |
| PW-07 | Should the body route remain one-way from #42589 toward the mouth-side room #42554? | Preserve the authored route pending builder decision; establish intended traversal and return before changing topology or publishing safe-travel advice. |
| PW-08 | Are the boulder and breeding-door passages meant to be ordinary shared closed doors or secret exits? | Preserve current D1 closed/unlocked resets and WLD descriptions until their intended hidden-door state is confirmed; no D reset currently sets EX_SECRET. |
| PW-09 | Should a successful open/search/pass through a shared door ever count as a personal story step? | Keep current Q31 independent of door history. A future step needs an authoritative successful mutation and reset epoch, not room entry or another player's open state. |
| PW-10 | Should the local race-combat behaviors qualify swallowed, killed, or recovered victims for this story? | Keep generic PW shriek/salivation/corrosive behavior distinct from the separate purple_worm swallow callback; add no local receipt without an exact source and committed actor outcome. |
| PW-11 | Should the 65% M resets for the giver and two other mobiles be guaranteed to load at #42594? | Preserve current chance and cap. Qualify initial, forced, and ordinary reset behavior before a separately scoped global loader fix; do not make these mobiles guaranteed by documentation. |
| PW-12 | Does daily eligibility promise fresh giver and hide stock after a character's daily window renews? | Preserve mode-1, cap, and ordinary reset rules. Daily rollover does not reset doors, restock stock, or guarantee a new exchange. |
| PW-13 | Are RandomLoadRoom #42594 and UltraRareLoadRoom #42596 special spawn or trap mechanisms? | Treat names as room text. Confirm actual desired placement and loader behavior before adding a special, dispersal rule, or route claim. |
| PW-14 | Should exitless loading room #42595 or unused VNUM #42597 be connected or created? | Preserve the source inventory and current numbering; add a room or edge only after confirming intended map and loader ownership. |
| PW-15 | Are the two reset door sides and WLD state values intended to stay synchronized as currently authored? | Preserve current ZON records, key values, and actual source flag behavior; test state synchronization only in a separately authorized gameplay repair. |
| PW-16 | Does Alatorin intentionally reset infant purple worm #42501 from the pworm prototype? | Preserve its nine active reset-role rows unless the owner reassigns them; do not duplicate or delete the mobile based on comments alone. |
| PW-17 | Should the three ud_map2 exits to #42525, #42542, and #42500 be restored as live approaches? | Keep them source-only on candidate row 114, distinct from active entry edges; resolve map identity and generation ownership before any future source move. |
| PW-18 | Is Lair of the Purple Worm related by ownership or replacement to the separately active Caverns of the Worms? | Preserve both named active contributors; shared worm subject matter and the bounded title/history scan establish no replacement. |
| PW-19 | Should bracer, boots, earring, ring, and mobile spelling mismatches change item identity or recipe inputs? | Confirm each intended label, then use a text-only fix/news scope; keep prototypes, stock, equipment, and Q31 unchanged. |
| PW-20 | Should pworm remain DO NOT MOVE with its four reciprocal boundaries, Alatorin reset dependency, and existing ud_map2 candidate evidence? | Keep the active area in place and create no candidate. Any future review must preserve those dependencies and separately qualify access, source ownership, and tool/editor consumers. |

These twenty decisions make the coverage register's builder field explicit
while retaining the dossier's fifteen implementation follow-ups. No native
gameplay, area membership, or candidate count changes in this checkpoint.
