# The Ancient Halls of Ironstar (lornecro): world and owner review

**Scope:** checkpoint 267 adds directed connectivity, active and source-only
dependencies, title/history, attic, and builder-owner reconciliation to the
existing [source-comprehensive Ironstar dossier](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md)
and [native source index](../../reference/zone-story-audits/lornecro.md). This
is a source review; player access, reset admission, hazards, paid settlement,
and gameplay outcomes remain unqualified.

## Story coverage and authored inventory

The active contributor is zone 1389, The Ancient Halls of Ironstar. It has 100
rooms #138900–#138999, 34 mobiles, 66 objects, three QST givers, seven native
Q/QA contracts, no shop records, and a revision-1 story sidecar. The sidecar
maps four stories, three equipment services, fifteen contacts, and fourteen
optional item or producer checks. The existing dossier reviews all seven
exchanges, all dialogue, room and item ownership, resets, locks, river and fall
hazards, shared procedures, prior direction repair, and active qualification
limits. It remains the detailed source of record for exact quest behavior.

The current dossier distinguishes Larra's ring from Robert's Soulcatcher and
Haldron's crown from Dralor's separate vault-key route. It also treats the
dragonbone, demonic dagger, and demonic hammer as equipment services, not
additional story credit. Clan rebuilding, rescue/reunion, school completion,
letter delivery, and other lore have no reviewed accepted terminal event.

## Directed WLD graph and access boundary

The 100 rooms have 207/207 resolving exits: 206 local and one outbound. The
local undirected graph is one weak component. Its directed graph has one
98-room strongly connected component and two singletons, #138991 and #138992.
Surface room #512296 enters Ironstar at #138978, and Ironstar exits north from
#138978 back to Surface #512296, forming one reciprocal pair
([Ironstar exit, cross-area row 1312](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1312),
[Surface return, row 1324](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L1324)).

From either Wherston Road #138900 or the Surface arrival #138978, raw traversal
reaches 98 rooms; #138991 and #138992 have no inbound exit from the main graph.
Room #138991 has five exits to #138992 and one to #138900. Room #138992 has no
exit, so 99 rooms can route back to #138900 but #138992 cannot. The existing
dossier identifies these two as random-loading infrastructure without active
M or F placements. The all-95 inactive-WLD scan finds no non-overlapping
source-only approach into Ironstar.

The source has 52 door-marked exits, ten current-marked rooms, and three
fall-chance rooms. Effective access depends on reset commands: 50 D commands
set halls, locks, secret doors, and the paid vault route to different states.
Room #138911 retains a southward current direction while its visible route
curves east and has no south exit. Raw traversal alone does not prove the
effective door state, secret discovery, fall or current survival, underwater
breathing, arrival, or safe return. Keep those separate from story credit.

See [WORLD_CONNECTIVITY.md](WORLD_CONNECTIVITY.md#checkpoint-267-lornecro-directed-reachability-and-surface-pair),
[coverage row 165](ZONE_COVERAGE_REGISTER.csv#L165), and the
[Ironstar dossier's access and hazard review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#access-reset-state-travel-and-hazards).

## Typed dependencies and source identity

The 198 local reset commands resolve their local mobile, object, and room
targets. They also import four object identities owned elsewhere: tradeskill
objects #400280, #400274, and #400277 are reset in the treasure chamber, and
Heavens object #359 is carried by a reset mobile. Object #359 has the compiled
epic-stone special and enters shared epic-stone behavior
([reset imports](../../../areas/zon/lornecro.zon#L242),
[Heavens assignment](../../../src/specs/specs.assign.c#L1604),
[epic procedure](../../../src/world/epic.c#L863)). Keep those identities with
their existing owners; neither ore stock nor the epic-node outcome is an
additional Ironstar QST input.

No active foreign ZON reset, QST giver or item command, SHP product, or story
sidecar typed field points to an Ironstar mobile, object, or room. The source
code's static Chaos equipment data includes Soulcatcher object #138950
([prototype](../../../areas/obj/lornecro.obj#L844),
[equipment data](../../../src/account/chaos_eq_data.h#L114)); those equipment
templates do not add an Ironstar quest or another local procedure. There are
no local shop or literal special assignments.

Inactive Duris3.wld contains 90,000 ocean rooms and reuses all 100 Ironstar
room VNUMs #138900–#138999 with different room names and descriptions. For
example, #138900 is Wherston Road in lornecro and The Ocean in Duris3. The
matching number is a source-owner collision, not a duplicate room or evidence
of replacement. Duris3 is already retained on candidate row 36; preserve that
source-only map separately and do not co-load or reassign these room identities
([Duris3 room](../../../areas/wld/Duris3.wld#L816901),
[candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36)).

The tracked .caplor files beside the lornecro source files are also preserved.
They are not listed as active contributors and are not a separate area or an
attic candidate. No inactive WLD approach or new candidate is recorded for
Ironstar.

## Title, history, and attic disposition

After color stripping and case/whitespace normalization, The Ancient Halls of
Ironstar has one owner among 351 active .zon contributors. Tracked lornecro
history stays on the same source stem; the 2012 map-marker work points toward
and away from Ironstar, later changes revise local content, and the 2026
direction fix adjusts two WLD exit descriptions. The reviewed history shows no
title rename, separate predecessor, or whole-area replacement. The inactive
Duris3 Ocean records and same-stem .caplor files do not establish a replacement
family. Keep active lornecro DO NOT MOVE. Preserve the reciprocal Surface pair,
the Duris3 VNUM ownership boundary, external ore and Heavens prototypes, and
the shared epic-stone binding. See
[DUPLICATE_AND_REPLACED_ZONES.md](DUPLICATE_AND_REPLACED_ZONES.md#checkpoint-267-ironstar-title-and-tracked-history)
and [ATTIC_MIGRATION_PLAN.md](ATTIC_MIGRATION_PLAN.md#checkpoint-267-ironstar-retention-and-duris3-room-identity).

## Builder and owner questions

These direct questions record decisions to resolve before changing content or
credit. The recommendations do not authorize a native gameplay change.

| # | Question | Source evidence | Recommended resolution |
| ---: | --- | --- | --- |
| 1 | Should the inactive Duris3 rooms #138900–#138999 remain The Ocean records while active Ironstar owns those VNUMs? | [Lornecro entrance](../../../areas/wld/lornecro.wld#L1), [Duris3 overlap](../../../areas/wld/Duris3.wld#L816901), [candidate row 36](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L36) | Keep the source families separate; require an owner-approved identity map before any loader, move, or renumbering change. |
| 2 | Are #138991 and #138992 intentional inaccessible reset staging rooms? | [staging records](../../../areas/wld/lornecro.wld#L2074), [zone reset file](../../../areas/zon/lornecro.zon#L1) | Confirm they are inert staging or provide a concrete route and reset use; do not treat them as reachable story content while they remain isolated. |
| 3 | Is the southward current metadata in room #138911 intentional when its described route turns east and no south exit exists? | [river bend](../../../areas/wld/lornecro.wld#L258), [hazard analysis](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#access-reset-state-travel-and-hazards) | Confirm the intended current direction and topology before an isolated source fix; keep the current effect unqualified until exercised. |
| 4 | Which current, fall, breathing, and trap routes are safe for the intended character range? | [river hazard source](../../../areas/wld/lornecro.wld#L258), [reset and movement review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#access-reset-state-travel-and-hazards) | Review each route with effective reset and movement behavior before adding safe-arrival or survival credit. |
| 5 | Should the Black Hall ring remain behind the black skull key and closed desk while the entry door is merely closed? | [ring exchange](../../../areas/qst/lornecro.qst#L16), [desk and key review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#sources-actors-and-material-ownership) | Preserve the key, parent container, and ring identities; qualify the actual unlock/open/GET path before recording item-source history. |
| 6 | Is the King's tomb meant to require dagger mold #138944 when prototype and reset door states differ? | [tomb access analysis](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#access-reset-state-travel-and-hazards) | Confirm the effective post-reset door and intended route; do not infer a mandatory vault prerequisite from the prototype key field. |
| 7 | Should each of the three vault locks keep its distinct exact key and route? | [vault-key exchange](../../../areas/qst/lornecro.qst#L114), [door and key analysis](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#access-reset-state-travel-and-hazards) | Preserve silver key #138949, container-held key #138955, and paid key #138958 as distinct identities unless an explicit design replaces the sequence. |
| 8 | Is the silver mithril key intended to come only from the third baby dragon? | [dragon reset chain](../../../areas/zon/lornecro.zon#L335), [key source review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#sources-actors-and-material-ownership) | Confirm its child placement, cap, and eligible reset path before promising key availability or crediting a dragon kill. |
| 9 | Should large mithril key #138955 remain inside gravel-container #138956 rather than on Daxion? | [container source analysis](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#sources-actors-and-material-ownership) | Preserve the parent and exact item ownership; qualify container recovery before describing it as carried loot. |
| 10 | Is the axe plus 500,000-copper vault-key price intended when native accounting currently refuses mixed durable offerings? | [vault-key exchange](../../../areas/qst/lornecro.qst#L114), [guarded fee review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#blockers-plan-expansions-and-balanced-repair-proposals) | Keep the fee guarded until atomic item/currency settlement, rollback, replay, and recovery are supported. |
| 11 | Is the Datherlion password meant to unlock the already reset-open grate, and should it clear secret flags without opening it? | [magic-door source analysis](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#access-reset-state-travel-and-hazards) | Keep the clue as optional until locked-state, reveal, OPEN, and arrival events are separately qualified. |
| 12 | Is the reunion with Larra intended to have an endpoint after Robert accepts her ring and retires? | [ring and Robert exchanges](../../../areas/qst/lornecro.qst#L16), [outcome review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#progression-stories-and-exact-acceptance) | Credit the current exchanges only; add reunion or soul-rest credit only after a concrete accepted terminal event exists. |
| 13 | Is Robert's disappearance after Soulcatcher delivery intended, and how should replay or renewal behave? | [Robert exchange](../../../areas/qst/lornecro.qst#L228), [recipient qualification](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#sources-actors-and-material-ownership) | Confirm recipient episode, retry, and renewal behavior before exposing daily eligibility or persistent completion. |
| 14 | Does returning the king's crown intend only the axe exchange, or also a persistent Ironstar clan-rebuilding outcome? | [crown exchange](../../../areas/qst/lornecro.qst#L92), [story interpretation](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#progression-stories-and-exact-acceptance) | Preserve the current exact return and axe reward; keep clan reconstruction out of credit until its world/party outcome is defined. |
| 15 | Is Maltheas meant to hand out broken Fury #138965 separately from intact Fury #138908 equipped on him? | [broken weapon exchange](../../../areas/qst/lornecro.qst#L124), [Maltheas reset](../../../areas/zon/lornecro.zon#L343) | Keep the exact broken and intact VNUMs distinct; add a breaking operation only with approved ownership and replay rules. |
| 16 | Should Urian's apprentice letter and the school story remain environmental lore without delivery or graduation events? | [letter placement](../../../areas/zon/lornecro.zon#L252), [missing endpoint review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#blockers-plan-expansions-and-balanced-repair-proposals) | Keep the letter and school references outside completion credit until an accepted recipient or result exists. |
| 17 | Are Marius's book, diary, portrait, and office globes intended as lore props, or should any trigger a real travel or knowledge action? | [prop and travel review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#access-reset-state-travel-and-hazards) | Keep descriptions separate from learned-topic or teleport credit unless an installed interaction confirms the effect. |
| 18 | How should Dralor's three paid equipment services settle item and coin offerings under durable accounting? | [mail contract](../../../areas/qst/lornecro.qst#L107), [dagger and hammer contracts](../../../areas/qst/lornecro.qst#L124), [service repair proposal](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#blockers-plan-expansions-and-balanced-repair-proposals) | Preserve exact recipes and guard refusals until atomic owned-item/currency settlement and recovery are supported. |
| 19 | How should the cap-one demonhide scroll and broken Fury be allocated between the dagger and hammer recipes? | [dagger recipe](../../../areas/qst/lornecro.qst#L124), [hammer recipe](../../../areas/qst/lornecro.qst#L134), [shared supply review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#sources-actors-and-material-ownership) | Confirm whether services compete or stock renews; do not reuse one physical item or lower the recipe silently. |
| 20 | Should rune stone #359 continue invoking shared epic-stone behavior in Ironstar's treasure room? | [reset import](../../../areas/zon/lornecro.zon#L329), [compiled assignment](../../../src/specs/specs.assign.c#L1604), [shared procedure review](../zone-stories/THE_ANCIENT_HALLS_OF_IRONSTAR.md#blockers-plan-expansions-and-balanced-repair-proposals) | Keep Heavens and shared epic ownership intact; qualify the actual procedure separately before attributing a local story result or safety claim. |

