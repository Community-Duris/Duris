# Pharr Valley: repository-wide connectivity and dependency supplement

**Checkpoint 71.** This supplement adds the repository-wide WLD, typed-reference,
title, and bounded-history review for the comprehensive
[Pharr Valley dossier](../zone-stories/PHARR_VALLEY.md). The findings describe
source declarations and loader paths; they do not establish played access,
stock, custody, or completion.

## Active connectivity and inactive WLD scan

The active scan parsed all 351 active WLD files without issues. Pharr Valley
has 189 rooms and 408 authored exits: 406 are internal and two leave the area;
all 408 resolve to loaded rooms in one 189-room weak component. Both active
boundaries are reciprocal:

| Pair | Source records |
| --- | --- |
| Valley Crushk | [Valley Crushk #2006 east to Pharr Valley #40300](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L33); [Pharr Valley #40300 west return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L551) |
| Pharr Valley Swamp (`pods`) | [Pods #28500 west to Pharr Valley #40389](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L412); [Pharr Valley #40389 east return](WORLD_CROSS_AREA_EXIT_REGISTER.csv#L552) |

The all-95 inactive WLD scan parsed 527,791 rooms and 2,080,773 exits with no
issues. It found no source-room VNUM overlap and no inactive exit target into
Pharr Valley. This checkpoint adds no source-only WLD ingress, so the curated
source-only inbound total remains 603.

## External typed references

No external QST item command, mobile/object prototype definition, exact
story-sidecar VNUM, or type-25 portal destination targets Pharr Valley. All
231 other story sidecars parsed without issues and had zero exact structured
VNUM matches. The scan found 872 external type-25 declarations and no target
into this area.

Two unregistered Nexus56 declarations name local apple #40208. In
[`nexus56.shp`](../../../areas/shp/nexus56.shp#L256), shop #83032 lists the
apple as a product. Nexus56 is omitted from the production area list and the
normal shop compiler reads its `AREA` list
([compiler](../../../areas/src/shp/make_shp.c#L26)). The runtime shop loader
first opens `areas/world.shp`, then falls back to globbing every
`areas/shp/*.shp` file if that catalog cannot be opened
([catalog path](../../../src/world/db.h#L22),
[loader](../../../src/economy/shop.c#L3272),
[fallback glob](../../../src/economy/shop.c#L3276)). The Nexus56 product is
therefore a conditional fallback input, not a guaranteed inactive shop record.

The unregistered [`nexus56.zon`](../../../areas/zon/nexus56.zon#L1331) also
contains a `G` reset for object #40208 at room #999. Nexus56 is absent from the
production `areas/AREA` list, and the normal zone compiler reads that list
([Pharr Valley manifest row](../../../areas/AREA#L192),
[zone compiler](../../../areas/src/zon/make_zon.c#L26)). This reset remains a
source-only declaration; its source comment notes that #40208 is not defined in
the Nexus56 object file. The matching object is Pharr Valley's local apple
([prototype](../../../areas/obj/pharrvly.obj#L85)).

Direct C/C++ matches are the already-documented Pharr Valley ship and pet
bindings: compiled ship tables use the 55 local crew mobiles #40220–#40274
and ship chest/key objects #40215–#40225; the ship-capacity check classifies
the broad #40201–#40299 NPC range, and the Soldon-hat procedure creates mobile
#40248
([ship crew](../../../src/ships/ship_npc.c#L1573),
[capacity check](../../../src/ships/ship_utils.c#L692),
[hat procedure](../../../src/specs/specs.lohrr.c#L368)). These are code-owned
behaviors in the comprehensive dossier, not additional native QST receipts.

## Title, history, and disposition

The normalized active title scan found one exact “Pharr Valley” title among
351 active contributors. `pharrvly` is distinct from `pods`, titled “Pharr
Valley Swamp”; neither is a duplicate or replacement declaration. The active
title and list entry are recorded in
[`pharrvly.zon`](../../../areas/zon/pharrvly.zon#L2) and
[`areas/AREA`](../../../areas/AREA#L192). The global active-room scan found no
Pharr Valley VNUM collision; the only duplicate remains #51800 in the two Hell
areas.

Bounded tracked WLD history reaches the 2009 area-tree move and the 2010
pirate-chest load-room additions, with later same-path edits. It identifies no
distinct predecessor or replacement area. Keep Pharr Valley **DO NOT MOVE**.
The Nexus56 shop fallback and reset references remain for source-owner review
([migration plan](ATTIC_MIGRATION_PLAN.md#L40),
[candidate dependency register](ATTIC_CANDIDATE_DEPENDENCY_REGISTER.csv#L79));
this audit makes no route, shop, reset, or migration change.

This checkpoint used repository source and static registers only. No tests,
build, server/runtime, gameplay, database, migration, or operational script
was run.

## Checkpoint 319: builder questions and recommended resolutions

The active world, quest, story, and relevant implementation inputs match the source snapshot used for Checkpoint 71. The earlier dossier and repository-wide scans therefore remain applicable to the current tree. These recommendations clarify builder intent; they do not claim normal-play access, stock, persistence, or runtime outcomes.

| ID | Builder question | Recommended resolution |
| --- | --- | --- |
| PV-01 | Must Farmer #40201's apple/orange/nest request (Q87) be completed before the carapace request (Q100), or vice versa? | Keep the two exact receipts independent. The source imposes no order, Q87 retires its farmer, and Q100 leaves its farmer present; do not invent a campaign sequence. |
| PV-02 | Should Q87's dialogue about a book, carapace, and scroll remain when its contract accepts apple #40208, orange #40209, and nest #40210 for E2500? | Align the prose with the actual three-item contract, or design a separate explicit scroll result with its own identity and durable receipt. Do not imply a parchment reward that the QST does not declare. |
| PV-03 | Should the carapace clue identify Gartham leader #40215 when ordinary soldiers #40216/#40217 do not carry shell #40213? | Keep the single declared leader drop and align the hint with it. Do not add copies to ordinary soldiers unless a separate stock design is approved. |
| PV-04 | Must the player personally defeat the leader to offer carapace #40213? | Do not add a personal-kill requirement from flavor text. Q100 checks the exact object; any encounter-history requirement needs an explicitly designed and settled event. |
| PV-05 | Can a book, shell, parchment, or other item substitute for any of Q87's three declared roots? | Preserve the exact roots #40208, #40209, and #40210 as one bundle. The current source names no substitute. |
| PV-06 | Can nest #40210 be offered while it remains inside scarecrow #40201 or still contains child objects? | Keep parent withdrawal and child custody explicit. Qualify how a filled container is removed and consumed before claiming that its complete contents are safely destroyed; do not infer recursive ownership behavior from the recipe. |
| PV-07 | What should supply replacement quest materials while active accounting blocks legacy O/P/G/E reset issuance? | Preserve the accounting guard. Define a supported durable issuance path before promising that fresh active-epoch stock is available; keep the declared reset placements as source evidence. |
| PV-08 | Should the two receipts, achievements, and potential daily units remain distinct from each other and from discovery? | Preserve two separate native receipt identities and keep discovery separate. Qualify replay and recovery against those identities before changing journal mappings. |
| PV-09 | How should the second request remain available after Q87 removes Farmer #40201? | Document the existing disappearance and recipient lifecycle. Keep Q100 independent and require an available recipient only when the player attempts it; do not impose a false order. |
| PV-10 | Is one reset instance of Farmer #40201 (M cap 1) intended to serve competing players? | Preserve the cap and qualify contention, restock, and disappearance behavior before changing population. Do not multiply givers to mask a stock or concurrency issue. |
| PV-11 | Is Curator #40200 intended to be a permanent quest contact or teacher? | Keep the curator as an optional clue. The prototype has neither SENTINEL nor ACT_TEACHER, so do not make a fixed room presence or teaching behavior a prerequisite. |
| PV-12 | Are case #40204, key #40203, chest #40202, and their clues required for either farmer contract? | Keep them optional. The exact QST contracts do not gate acceptance on opening the case/chest, obtaining the key, or reading the paper clue. |
| PV-13 | Should paperwad #40211 become the promised history scroll or award a learned-topic receipt? | Keep its ordinary trash identity unless a separately specified object and outcome are authored. Its description alone does not create a produced scroll or receipt. |
| PV-14 | Should the lost-wife, Skexis, peaceful-peak, cave-war, or lost-scroll prose grant rescue, cure, history, or completion credit? | Treat these as environmental lore until a builder defines an objective, evidence, and settled outcome. Do not add inferred quest credit. |
| PV-15 | Should pushing flat rock #40214 count as a personal puzzle completion or daily? | Preserve the existing shared PUSH 270 passage. The item-switch handler clears the blocked forward/reciprocal exits for everyone; add personal credit only with an explicit per-player event design. |
| PV-16 | Are the blocked east tunnel, reset door states, falls, and no-heal cave room intended as hazards or as a rescue progression? | Preserve the current door and hazard records, including falls in #40373/#40374 and NO_HEAL in #40382. Document any intended rescue or recovery objective before changing access or reset behavior. |
| PV-17 | Are the reciprocal boundaries to Valley Crushk #2006 and Pharr Valley Swamp #28500 intended to remain the valley's two active cross-area routes? | Preserve both reciprocal pairs. They are active world connections; they do not by themselves establish a quest route or authorize map rewiring. |
| PV-18 | Should the five ship-crew tiers and generated treasure be treated as part of the local valley quest? | Keep runtime crew, ship locations, zone 600, and generated loot separate from local resets and QST receipts. A future sea campaign needs explicit world location, attempt, source, and custody scope. |
| PV-19 | Are Nexus56 shop #83032's apple #40208 product and room #999 G reset intended legacy dependencies? | Keep both declarations on existing Nexus56 candidate row 79 as source-only references. The shop fallback is conditional and the reset is outside the production zone list; resolve owner and loader-path intent before any candidate move. |
| PV-20 | Do Soldon's hat procedure and the broad ship-capacity mobile range belong to Pharr Valley's intended builder-facing activity? | Treat them as separate compiled cross-system bindings. Keep hat #435's pet creation and the #40201–#40299 capacity classification documented for their code owners; do not bundle a balance or classification change into this map/story review. |
