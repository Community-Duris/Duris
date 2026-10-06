# Gagga'Jobo: livestock materials, goblin crafts and recipe selection

Reviewed October 6, 2026. Source area `goblincave`, zone190, schema3/revision1.
Six independent cards cover every native return:two item-only requests and four
paid supporting services. Four contacts preserve all seven M blocks/eleven aliases;
eight optional material rows and six terminal rows explain exact quantities and
costs. Two achievement/two potential daily candidates remain. Four paid recipes
leave achievement credit through explicit service classification, without losing
their native identities or journal history. Discovery is separate. No native
repair ships in this checkpoint.

## Evidence boundary

Reviewed complete [Q/M source](../../../areas/qst/goblincave.qst),
[rooms](../../../areas/wld/goblincave.wld), [mobiles](../../../areas/mob/goblincave.mob),
[objects](../../../areas/obj/goblincave.obj) and [resets](../../../areas/zon/goblincave.zon).
The [audit](../../reference/zone-story-audits/goblincave.md) records exact bindings.
Read all81 QST lines/finalS and88 raw reset lines/finalS. No local shop file exists.
Registry18905–19022 differs from23 actual rooms19000–19022.

All23 rooms were read through23 full prose,2 headers,1 non-exit metadata,
21 relative graph and4 complete exit-text families, covering45 exact edges.
All10 mobiles19000–19009 include full prose/numeric records. All15 complete
objects19000–19014 include E/apply/value records. Read44 expanded reset families;
65 commands are D4/O2/P10/M23/E4/G22. Caps are shared declarations, without
guaranteed admitted quantities. No imported objects, foreign reset actors,
selected foreign reuse/recipe/shop candidates or missing reset prototypes were
found. Full boundary842047 and reciprocal19000W↔842047E were read. No active
incoming type25 portal was found; selected outside review is not full foreign-zone
qualification.

Full selected local assignment/creation search found no custom handler. Computed
assignment/creation remains a manual discovery limit. Reused completed reviews
of unchanged generic door/container, ASK/accepted settlement/credit and accounting
gates. Native quest_completion, loader Q/G prepending and full durable offering
selector were reread for exact recipe precedence and mixed-fee evidence.

## Six accepted recipes and exact supplies

| Native recipe / recipient | Complete offering | Reward / classification |
| --- | --- | --- |
| Q26 leatherworker19005@19008 |1×I19006 hide +C1000(1 platinum) |I19007 shoes / paid service |
| Q32 leatherworker19005 |2×I19006 hide +C2000(2 platinum) |I19008 shirt / paid service |
| Q39 leatherworker19005 |3×I19006 hide |I19009 backpack / item-only request |
| Q46 leatherworker19005 |4×I19006 hide +C10000(10 platinum) |I19010 gloves / paid service |
| Q67 bone-worker19006@19011 |I19011 bone +I19013 pootata flesh +I19006 hide +C100000(100 platinum) |I19012 sword / paid service |
| Q75 bone-worker19006 |2×I19011 bone |I19014 earring / item-only request |

All have D0; both recipients remain. Monetary macros establish C copper and
1000 copper/platinum. Four mixed-fee definitions already lack daily eligibility
as unsupported durable offerings. Classifying them as services additionally
removes four achievement units, while all six bindings/history remain. Backpack
and earring retain two achievement/daily candidates. Candidate shape does not
certify successful dispatch or payout.

Exact repeated quantities need different roots; sword bone/flesh/hide are three
distinct kinds. Matching supplied loose goods fit native recipes, without a
personal kill, learned-topic or previous-commission prerequisite. Current
material readiness, paid funds and accepted history differ. Schema currently
has no coin-balance/reservation row, so eight optional checks show materials
without claiming a complete paid offering. Rendering stays read-only.

## A source-reproduced backpack dispatch blocker

The loader prepends Q entries, so Q46 gloves precedes Q39 backpack, Q32 shirt and
Q26 shoes. It also prepends G entries, so Q46's last C10000 becomes its first
goal. submit_durable_quest_offering first scans all goals and matches a hide;
then the non-item coin goal marks the recipe unsupported before selecting roots.
The unsupported branch **breaks the outer completion scan**, so standard durable
GIVE never reaches the otherwise item-only three-hide backpack recipe.

This is a concrete source blocker, beyond daily eligibility or available stock.
It is not a played failure claim. The journal explicitly explains it and does
not promise payout from three carried hides. At the bone-worker Q75 earring
precedes Q67 sword:two distinct eligible loose bones can reach item-only
selection. One bone plus other sword materials does not bypass the fee guard.
Actual settlement, reward authority and persistence remain to be played.

The four mixed material-and-coin recipes are unsupported by current durable
item-only submission. Numeric coin GIVE and legacy fallback are also refused
under active accounting. Keeping guards intact prevents unsafe fees, but does
not make later supported choices reachable. Proposed repair should add explicit
recipe selection or a clearly specified supported-candidate policy, with complete
terms and refusal behavior. Atomic mixed fees require coin reservation/debit,
item roots, recipient/recipe identity, rollback, replay and frozen recovery.
Any selected engine repair belongs in a separate named fix/news commit with
original-fails/repaired-passes tests; no native dispatch change ships here.

Legacy quest_completion checks recipient inventory/money, unlike the durable
actor-owned batch. It must not be used as an accounting-on workaround. Rendering
material rows cannot select a recipe, pay a fee or remove this blocker.

## Livestock sources, ordinary food and shared passages

Six large chothe19000 are declared@19018/19019×2/19020/19021/19022, cap6, each
with G19006 hide/G19011 bone cap6. Five pootata19001@19012–19016 cap5 each declare
G19013 flesh cap5. These are ordinary livestock inventory sources, not selected
skinning or slaughter callbacks. Personal combat, corpse/source recovery,
transfer, loose custody and accepted crafting need separate evidence.

Chained chothe19003@19007 declares no quest materials. Butcher19004, apprentice
19007 and workers supply context without another native recipe or selected
slaughter/feed/transport handler. O19001 butcher table/P19003/P19004, O19003 meat
and worker19002's E19002 meat pack/P contents are food/container stock; they do
not replace the exact hide, bone or pootata flesh. Room scraps and bone piles
are descriptions, not extra declared quest supply.

Trapdoor19010DOWN↔19012UP and iron gate19017E↔19018W are WLD1/D1 closed, key0.
No special guardian/control was found. Ordinary shared opening and actual
arrival differ from personal material collection; no guard-kill prerequisite
is authored. Described winches, prods, troughs, farming and historical
conservation are not implemented quest milestones.

## Learned recipes, products and equipment claims

Leatherworker's five M blocks have chothe/hide/strip, backpack/bp, shoes, shirt
and gloves aliases. Bone-worker's two M blocks have bone/sword and earring.
Eleven aliases explain complete recipes without a durable learned-response
milestone. The sword asks for two flesh kinds in prose but exact terms use
pootata flesh plus chothe hide. The journal supplies exact terms; builders can
clarify the intended wording in separate editorial fix/news work.

The craftsperson's native name list contains **leatherworker&n**, including a
color suffix. The loader stores it as player.name, and strict isname requires
word termination, so the plain leatherworker token does not match that alias.
LEATHER is valid, but apprentice19007 in the same room also has it. The journal
uses LEATHER and explains selecting the actual craftsperson with a numbered
target when necessary. A separate proposed native alias fix should remove the
suffix and test precise recipient selection; no prototype change ships here.

Leatherworker G19007/19008/19009/19010 and bone-worker G19014 declare finished
goods already in their inventories. Recovering/transferring these products is
different from commissioning and cannot create the native accepted receipt.
Shoes19007 have APPLY_MOVE14/+20, a finite movement modifier; the promise that
the player can walk forever is not an infinite-walking mechanic. Wielding the
bone sword, wearing products and their actual effects need separate evidence.
Typed mob19006 bone-worker/item19006 hide and mob19005 leatherworker/item19005
badge must stay distinct.

## Accounting and qualification

Mode2/lifespan40–50/header1 and capped reset rows describe intent. Active
accounting refuses fresh O/P/E/G issuance; discovery/daily clocks do not produce
ingredients or rewards. New discovery, encounter, journal, achievement and daily
progress requires active, ready accounting. Frozen recovery remains separate.

Full production catalog/inventory/audit regression, all143 compiled journal
journeys, source/schema assertions, changed/staged formatting and exact previous
map/native/roadmap/PR preservation passed with the local tools described below. Local tools
use MSYS2 UCRT64 C++20, pinned cJSON and canonical clang-format14.0.6. Last
maintained Linux server build passed with unchanged src; fresh Linux build/
journal runs remain unavailable after WSL launch failures. No played active-
accounting source, exact recipe selection, mixed fee, accepted craft/reward,
shared access, equipment or recovery journey is claimed. No native repair ships;
selected fixes require separate named commits and prominent PR/news reporting.
## Builder and universal capability follow-ups

| Follow-up | Evidence and implementation proposal | Qualification / repair boundary |
| --- | --- | --- |
| ZSQ-GOBLINCAVE-CRAFT-CLASSIFICATION | Four Q recipes accept materials plus C fees; two accept only exact items. All six remain journal cards; four paid services have no achievement/daily credit. | Preserve every native binding and history. Two item-only requests retain two candidates; the four-unit achievement reduction is explicit classification. Product custody is not commissioning evidence. |
| ZSQ-GOBLINCAVE-EXACT-QUANTITIES | Shoes1 hide/C1000; shirt2/C2000; backpack3/no fee; gloves4/C10000; swordbone+flesh+hide/C100000; earring2 bones. | Bind exact kinds, repeated root counts and complete terms; alternatives cannot replace repeated quantity or distinct materials. Inputs are loose current inventory; supported completion must use accepted transaction history. |
| ZSQ-GOBLINCAVE-MIXED-FEE-AUTHORITY | Four paid recipes have C inputs; current durable item-only path and active numeric coin handover refuse them. C is copper,1000/platinum. | Extend selected atomic item+coin fee manifests, recipient/recipe identity, balance reservation, refusal/rollback/replay/recovery before enabling. Do not relax current guards or mark a material count as paid service readiness. Selected engine repair needs separate named fix/news work. |
| ZSQ-GOBLINCAVE-RECIPE-PRECEDENCE | Loader prepends Q46 gloves beforeQ39 backpack and G C10000 before its hide goals. Durable match finds the hide, flags the non-item goal unsupported and breaks the outer recipe scan. | Current standard durable GIVE cannot reach the backpack despite item-only daily shape. Add explicit recipe selection or a carefully specified supported-candidate policy, preserving full terms and unsupported refusal. Separate fix/news tests must reproduce original blocker, revised backpack acceptance, safe paid refusal and ambiguity/partial/supplied/replay/cold behavior. |
| ZSQ-GOBLINCAVE-COIN-READINESS | Sidecar schema supports carried/equipped item and completion rows, without a coin balance/reservation row. Eight optional material rows cannot check four fees. | Add read-only selected fee readiness with authoritative balance/reservation context and clear current-versus-accepted display. Funds, banked money, material stock and settlement are separate; never deduct or award from journal rendering. |
| ZSQ-GOBLINCAVE-SOURCE-OR-TRANSFER | Six large chothe19000 declare G19006hide/G19011bone; five pootata19001 declare G19013flesh. Matching supplied roots fit recipes. | First recovery needs committed item UID/source owner/room/container/actor/generation and transfer/party policy. Livestock kill, corpse recovery, loose custody and commissioned acceptance are different. No automatic skinning/slaughter milestone. |
| ZSQ-GOBLINCAVE-ADMITTED-SUPPLY | Mode2/lifespan40–50, capped materials, recipient product stocks and P/O meat are reset intent. Fresh O/P/E/G issuance is refused under active accounting. | Qualify admitted sources, cap exhaustion, ownership, renewal/retirement/refusal and cold recovery. Six declarations do not guarantee six usable hides; discovery/daily clocks cannot create supplies or craft rewards. |
| ZSQ-GOBLINCAVE-PRODUCT-VERSUS-RECEIPT | Leatherworker G19007/19008/19009/19010 and bone-worker G19014 declare ready products in ordinary inventories. Shoes APPLY_MOVE14/+20 is finite; prose says walk forever. | Source loot or transfer of a finished item cannot complete commissioning. Wearing and actual effects need their own selected authority. Confirm intended boast or descriptive correction before separate editorial fix/news work; no infinite movement guarantee. |
| ZSQ-GOBLINCAVE-LIVESTOCK-AND-SHARED-ACCESS | Closed trapdoor19010DOWN/19012UP and gate19017E/19018W have D1/key0. No selected guard/slaughter/feed/transport/conservation special was found. | Add only builder-selected committed door/arrival and subject/episode outcomes. Described prods, troughs, chains, hide scraps and historical conservation do not implement interactions or require a guard kill. |
| ZSQ-GOBLINCAVE-TYPED-GOODS | Item19006 is hide while mob19006 is bone-worker; item19005 is badge while mob19005 is leatherworker. Meat19003/19004, chained mob19003 and pack/table contents are not exact craft proofs. | Preserve type and recipient/source authority. No extra achievement from the apprentice, butcher, ground meat or matching names. Boundary842047 arrival differs from material collection and accepted recipe. |
| ZSQ-GOBLINCAVE-LEARNED-CRAFT-LORE | Seven M blocks/eleven aliases explain quantities and materials. The sword description calls for two flesh kinds, while exact recipe uses pootata flesh and chothe hide. | Record selected successfully delivered responses if builders make learned recipes objectives. Keep exact recipe terms in the journal; builder may clarify prose in separate editorial fix/news work after confirming material intent. |
| ZSQ-GOBLINCAVE-RECIPIENT-ALIAS | Native mob19005 keyword is leatherworker&n; strict isname requires word termination, so plain leatherworker differs. Valid LEATHER is also on apprentice19007 in the same room. | Use the valid alias and actual numbered recipient in the journal. Proposed separate fix/news work should remove the color suffix from the unique intended alias, then cover original mismatch/repaired exact targeting, awake/visible/race/shared-room selection and no accidental apprentice offering. No prototype change ships here. |
| ZSQ-GOBLINCAVE-ACCOUNTING-AND-PLAYED | Full source mapping closes six Q, four service cards and two request candidates; backpack dispatch, mixed fees, stock, reward and equipment remain different qualifications. | Run active-accounting supplied/exact-count/held/nested/partial/dispatch/refusal/return/reward/replay/cold journeys before live claims. Frozen recovery remains separate. Any selected native repair requires a separate named commit and prominent PR/news reporting. |
