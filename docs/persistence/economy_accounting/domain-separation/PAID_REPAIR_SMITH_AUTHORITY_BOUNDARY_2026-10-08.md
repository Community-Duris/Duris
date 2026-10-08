# Paid repair and legacy smith authority boundary — 2026-10-08

This is a source/design delivery for the compound paid services beyond the
already-qualified price/material calculations. It establishes no active repair
or smith admission, native atomicity, publication, ACK or recovery qualification.
The sole changed file is this document. No production, test, registry, schema,
shared driver or existing handoff changes, builds, DB/server runs or native
journeys accompany it. The original continuing Goal remains **BLOCKED and
unfinished**; this independent delivery does not resume it.

## Frozen source and reuse

Actual public `experimental-accounting` is frozen at
`f679ee312baccbe077267aedd36fceaa2f97b14f`, verified from the remote head before
export. The reported private primary `87c253f4` is source-report context only;
its bodies and execution are unavailable here and are not adopted. The public
source and async-test trees remain `833d3085815b396861ad18a77635412212381e4b`
and `790f367adf805a69d53aac6460938f5c921f9136`. A D: archive freezes `src`,
`tests/async`, `docs/persistence`, `migrations` and `scripts`: 3,575 files,
58,951,680 archive bytes, SHA256
`c8eb0bd4829926db5690a982f6f9362348fff1d20a0eedbf7084c6d781980d2b`.
Private evidence separately authenticates the owned reference documents below;
they are not silently substituted for public source.

Reuse these exact prior sections; their historical Goal/adoption wording does
not supersede the current blocked disposition:

| Prior authority or accepted preparation | Reused scope and limit |
| --- | --- |
| [R1 published implementation and terminal evidence](HANDOFF.md#r1-published-implementation-and-terminal-evidence), [terminal flatfile runtime](HANDOFF.md#r1-terminal-flatfile-runtime-qualification), [terminal R1 SQL and R2 caller qualification](HANDOFF.md#terminal-r1-sql-and-r2-caller-qualification) | Existing recipe plan, original input/output/pouch/progression continuation and its exact historical recipe journeys. No reopening/requalification or inference that legacy smith uses that transaction. |
| [R2 reservation](R2_RESERVATION.md#exact-preimage-and-interface), [currency native entry points](CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md#native-entry-points-and-captured-state), [publication/ACK](CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md#native-publication-ack-and-persistence-boundaries) | Existing denomination vectors, payment evaluation, submission versus completion, retained receipt and original native owner responsibilities. No duplicate arithmetic or generic paid-service framework. |
| [R4 terminal handoff](R4_SUPERIOR_TRIBUTE_HANDOFF.md), paragraph beginning "Forge preview and smith" | The existing five-entry forge price table and ore detachment timing were already identified. No thin price-table extraction. |
| [R11 calculation and native observations](R11_ITEM_VALUE_QUOTE_HANDOFF.md#calculation-and-native-observations) | Complete item-value calculation is already qualified at its declared scope; item selection, effects, payment and admission remain native. R3–R9 enhancement math/policy closures likewise remain closed. |
| [R13 complete policy and native ownership](R13_SHOP_ITEM_ACCEPTANCE_HANDOFF.md#complete-policy-and-native-ownership), [R14 complete decision and native feedback](R14_SHOP_CUSTOMER_ACCESS_HANDOFF.md#complete-decision-and-native-feedback) | Reuse actual selector/classifier/access behavior, including legacy evaluator-false acceptance and observation order. These decisions grant no repair/payment authority. The frozen public source still contains the original bodies; optional extracted headers are not assumed present. |
| [Item custody: complete subtree](ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md#complete-subtree-not-a-roots-only-inventory), [save and recovery obligations](ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md#recovery-and-save-nonoverwrite-are-separate-obligations) | Existing full literal/UID/topology and save nonoverwrite requirements; no second item custody, recovery, publisher or ACK owner. |

Quest preparation separately owns reward completion and its tests/maps. Plans 5,
shared authority, source issuance, native wallet/keeper/item/save and cold drivers
stay with their owners. Closed Collector and genuine Hand evidence is preserved.

## Command and procedure reachability

| Entry / source anchor | Actual dispatch and refusal boundary |
| --- | --- |
| `src/cmd/interp.c:3486` and `:3133` | REPAIR is a trigger command; FORGE also has the ordinary `do_forge` command handler. A callable C++ function alone is not proof of a player route. |
| `src/cmd/interp.c:2405`, `:2771`, `:2825` | Specials run before the normal handler. Room/equipment/inventory specials can consume input before an awake eligible room mobile's procedure receives `(keeper, customer, cmd, arg)`. `no_specials` can bypass this dispatch. No claim that every configured smith receives every forge input. |
| `src/economy/shop.c:2990`, `:2998`, `:3003` | `shop_keeper(keeper,ch,...)` refuses active REPAIR/FORGE before smith dispatch, shop-ID lookup or secondary proc. The money-trade exception is passed only for BUY/SELL; it does not open services. FORGE then calls `smith(keeper,ch,...)`, before singleton shop lookup and normal shop access checks. |
| `src/economy/shop.c:3036`, `:3126` | After a valid singleton shop and any secondary proc, REPAIR dispatch requires the customer's configured shop room or roaming flag. The direct repair body repeats the service refusal and `is_ok` access checks. |
| `src/net/comm.c:959`; `src/economy/tradeskill.c:951`, `:770` | Non-mini boot initializes tradeskills, assigning `smith` only when a configured mobile has no existing proc. Mini mode skips that initializer. Direct smith FORGE has its own active refusal at `:793`. SET_PERIODIC succeeds, and PERIODIC may draw RNG/play a cosmetic hum before the FORGE-specific guard. These are not paid completion callbacks. |
| `src/economy/shop.c:3602`; `src/specs/specs.mobile.c:1048`, `:1063` | Shop boot binding/secondary procedure handling and existing mobile wrappers are additional reachability prerequisites. Actual boot world/procedure identity is required, not just the smith's template VNUM. |
| `src/world/db.c:682`; `src/specs/specs.assign.c:1261` | Normal mobile assignment, including shop boot/binding, is conditional on specials being enabled. Later tradeskill initialization does not overwrite existing mobile procs. Neither a mini-mode fixture nor direct function invocation proves a normal-world service route. |
| `src/economy/tradeskill.c:409`; `src/economy/crafting.c:604` | Unconsumed ordinary FORGE enters the modern crafting handler. The old forge block following `tradeskill.c:414` is commented out. Its apparent calls are not live evidence. |
| `src/cmd/interp.c:1414`, `:1455`, `:1461`, `:1500`, `:1520`, `:1530`, `:1648`, `:1657` | REPAIR and FORGE are item- and currency-dependent commands for existing pending-input fences. Those fences protect unpublished state; they do not create a compound operation or bind an original service invocation. |

`refuse_unported_shop_mutation`, `src/economy/shop.c:69`, returns false when
accounting is inactive. When active, its BUY/SELL-only preparation exception is
irrelevant to repair, smith and direct `transact` (`:3673`). Preserve these refusals
and direct smith's guard; no RAM enable switch or generic authority bypass is
proposed. Actual world dispatch and policy remain distinct from source reachability.

## Repair: original observations and ordered effects

`shopping_repair`, `src/economy/shop.c:2876`, has one synchronous legacy body:

1. Refusal, current access (`is_ok`, `:1602`), argument parsing and visible carried
   item selection precede effects. `get_selling_obj`, `:1839`, reads the customer's
   actual inventory and invokes `trade_with(..., repairing=TRUE)`. Messages say
   the item is handed to the keeper, but there is no `obj_from_char`/`obj_to_char`
   transfer in repair. The selected item stays in its existing native custody.
2. NOREPAIR/artifact rejection (`:2902`) precedes the weapon probe. A weapon calls
   `read_object` using its current prototype index (`:2915`); the returned object
   is dereferenced without a null check. Its `value[3]` overwrites the original
   weapon's attack type, or `MSG_SLASH` replaces it if the probe is not a weapon
   (`:2919`, `:2926`). The probe is then extracted (`:2928`). This happens before
   condition checks, wallet checks, payment and RNG. A full-condition, ruined or
   unpaid item can therefore retain this attack-type change. Do not move it behind
   payment or reinterpret it as a successful paid repair without an explicit
   reviewed behavior decision.
3. Only condition strictly between 0 and 100 is priced: native integer
   `(obj->cost / obj->condition) * 5`, bounded 100..100000 (`:2937`). The old
   difference-to-100 expression is commented out. A scalar price result cannot
   represent the preceding prototype construction/mutation or later outcome.
4. Insufficient untrusted cash calls `accept_gem_for_debt` (`:2946`), but the actual
   helper unconditionally returns null at `:3638`. Independently `transact`
   forces `merchandise=0` at `:3681`. Gem selection, transfer, valuation at 3/4 and
   change code below those guards is unreachable through this route. Do not
   enable it by freezing a hypothetical selected gem. Trusted status does not
   waive `transact`'s actual cash requirement.
5. `transact` requires the same native room and sufficient wallet value, then
   calls `SUB_MONEY(from,value,0)` and `ADD_MONEY(to,value)` (`:3682`, `:3720`).
   Persistent-PC debit success can mean accepted asynchronous submission; NPC
   keeper credit follows a separate native helper path. The return is not a
   receipt proving both wallet legs and the repair effect committed together.
6. After that return, keeper level >35 draws `number(-9,10)` to compute a bounded
   average, then all paid cases draw `number(ave,100)` (`:2964`, `:2970`). The final
   condition is floored at 100 (`:2972`), not capped there. `src/core/random.c:127`
   supports descending ranges, so a high average can yield condition >100.
   Lower-level paid repair ends at 100; its second call draws only when the
   average is below 100 (original condition 1..49). A degenerate `(100,100)` call
   returns without advancing RNG. Freeze actual results and draw order, not an
   assumed always-100 outcome or a fixed number of random-state advances.
7. Full-condition items receive a no-work message; nonpositive condition refuses.
   Earlier attack-type mutation still stands. No repair-specific durable business
   continuation, save completion, refund handler or guarded ACK exists in this
   body. Messages are not custody changes or completion receipts.

The probe itself is not a pure immutable template read. `read_object`,
`src/world/db.c:6823`, instantiates via `:5196`; the adapter allocates UID, marks a
creation candidate, increments counts, enrolls the global list (`:5225`) and may
install procedures/schedule events (`:5265`, `:5287`). `extract_obj`,
`src/world/handler.c:3445`, recursively handles descendants, clears events at
`:3505`, unenrolls/decrements and frees at `:3522`. Preserve/capture those actual
effects or obtain an explicit owner-reviewed replacement; do not substitute a
new pure accessor and call it unchanged behavior.

## Smith: identity, ore custody, output and failure

`smith(P_char ch,P_char pl,...)`, `src/economy/tradeskill.c:770`, uses **ch as
keeper and pl as customer** at the actual shop/mobile callsites. Its order is:

| Stage and anchor | Actual observation/effect and implication |
| --- | --- |
| Keeper menu lookup, `:799`; configuration, `:196`, `:202` | Keeper VNUM selects `smith_array`. Empty argument lists its menu. Numeric choice uses `atoi` then `choice > i` at `:832`, where `i` is still the smith-array lookup index, not a computed menu length. First smith index 0 admits no positive choice. Preserve/report this source behavior; correcting menu validation is a separate review. |
| Selection/detachment, `:844`, `:846`, `:854` | For each configured ore slot, scans **ch->carrying**, chooses first matching VNUM and immediately detaches it. Comments say pull from `pl`; the code does not. Detached roots cannot be found twice. Selection may change list/count/weight/dirty state before price or payment. |
| Missing ore, `:859` | Describes recipe, then returns previously detached objects to **pl**, in reverse selected order. If keeper and customer differ, this is a cross-owner return, not restoration to the original keeper. No intended-customer assumption can erase that difference. |
| Count/funds/debit refusal, `:873`, `:880`, `:891` | Existing five-tier table is indexed by selected ore count. Each refusal similarly returns detached ore to pl. Debit happens only after those removals. `SUB_MONEY` uses the customer, not keeper. |
| Creation, `:904` | `forge_create(choice,pl,needed_ore[0]->material)` uses the first selected ore's material; no equality check on other ores is present. Output naming uses the customer. A missing output refunds via `ADD_MONEY(pl,price)` and returns ore to pl (`:907`, `:913`). No atomic refund receipt is established. |
| Grant, `:919`; wrapper, `:61` | `grant_tradeskill_item` submits an existing detached output to the item creation owner with `economic_source_kind::crafting`. If refused synchronously, it extracts the output with FALSE, then smith requests the same money refund and ore return (`:921`, `:924`). |
| Grant accepted, `:930` | Smith immediately `extract_obj(...,TRUE)` for each selected ore, then emits completion messages using the output pointer. It supplies no business completion callback to the grant owner. True is not proof that output publication or durable completion succeeded. A later grant failure has no smith-specific ore/payment compensation in this body. |

`forge_create`, `src/economy/tradeskill.c:312`, constructs actual template 1255
(`:320`) before applying actual menu strings and customer-name substitution
(`:326`), selected material, two `number` calls for affect modifiers (`:336`), wear and four
affect-word bits, class flags, and optional replacement of `extra_flags` with
ITEM_ALLOWED_CLASSES (`:344`, `:354`). Quiver keywords additionally draw capacity
20..80 and selector 0..1, set `value[2]=1`, `value[3]=0`, and type QUIVER;
otherwise type ARMOR (`:357`). Degenerate modifier ranges return without an RNG
state advance. Other prototype literal fields remain inherited.
The real configuration is `src/economy/tradeskill.h:33` and
`src/item/forge_items.c:20`; do not replace it with a price-only outcome.

`obj_from_char`, `src/world/handler.c:2036`, changes the original linked inventory,
weight/count, action/world/light state, dirty flags and location. The void
`obj_to_char` wrapper (`:2027`) ignores the checked placement result; source-level
"return ore" calls alone do not prove physical restoration. Material retirement
must cover the original selected roots and complete descendants, not only
configured ore VNUMs or root counts.

## Submission, completion, save and authority ownership

| Existing owner / anchor | Exact distinction for the compound service |
| --- | --- |
| `src/core/utility.c:3301`, `:3313`, `:3116`, `:3143` | Persistent PCs submit generic wallet spend/reward operations; local/NPC fallbacks mutate native denominations. Active unsupported calls refuse. `ADD_MONEY` is void and can defer failed credit through pending-money handling. A requested smith refund is not an acknowledged refund. Reuse R2 math and the currency owner's real receipt/replay contracts. |
| `src/item/item_movement_transaction.c:3862`, `:1573`, `:1643` | Creation grant checks actual PID/UID/nowhere/recipient and queues the existing output. True may mean queued, transiently blocked, started or attached work, not final success. The smith wrapper supplies neither completion callback nor source_id argument; the existing default source ID is zero. |
| `src/item/item_movement_transaction.c:1556`, `:1338`, `:1270` | The item owner submits its existing transfer, later rejects/discards unpublished output or publishes to an authenticated live recipient and retains publication failure. It can notify a supplied business callback; smith supplies none. Do not invent a callback context or durable format here. |
| `src/player/player_save_pipeline.c:1476`, `:1489` | Existing saves refuse while creation/publication is pending; other original holds retain their owners. This does not retroactively bind pre-detached ore or prepayment repair mutation to that grant. Repair's direct assignments contain no explicit player dirty/save call. Dirtying by another money/item path cannot be assumed to prove a complete service checkpoint. |
| `docs/persistence/economy_accounting/writers.json` entries `shop.repair`, `shop.legacy_transact`, `crafting.smith` | Registry explicitly describes legacy/unverified separate effects and absent atomic envelopes. Classification is not an admission capability. `service_cost` and `crafting_cost` must retain their own source/reason identities; helper wallet_spend/reward reasons cannot silently stand in for a service root. |
| `src/economy/economic_accounting_plan.c:123`, `:130`; `src/economy/economic_gameplay_authority.h:58` | Existing source-kind constraints require service versus crafting. Generic currency/item preparation is not an installed repair/smith native authority. No new reason, schema, command version, format, borrowed DB transaction or participant is created by this map. |

Ordinary recipe Forge is a different producer: `src/economy/crafting.c:74` selects
without inventory mutation; `:141` constructs the existing recipe continuation
and `:150` submits input forest, output, pouch and progression to the shared native
craft owner. `:40` handles business completion after its progression publisher.
Legacy smith does not call this path. Reuse R1's exact historical evidence and
existing owned inputs; do not route smith into it merely because both use FORGE
or call a grant crafting.

## Implementation order and required owned observations

1. Retain all active/direct guards and command fences. Review the observed
   prepayment repair mutation, smith keeper-ore/customer-return discrepancy and
   menu bound with the original owner first. Compatibility capture versus a
   behavioral fix must be explicit; moving selection/probe/RNG to simplify a
   plan is not neutral. Do not automatically activate either outcome.
2. Bind the **original invocation**: actual actor PID/account/runtime generation,
   actual keeper retained native identity/procedure/world binding, room/shop and
   source issuer. Template VNUM, later lookup by same name, object pointer or
   a newly synthesized command ID is insufficient for replay. No shared keeper
   or quest source capability is transferred by this document.
3. At original observation points, capture complete existing item literal and
   custody forests, expected owner/item clocks, config/template bodies, price,
   selected material and actual RNG draws/outcomes. `src/core/structs.h:491` and
   `src/player/player_snapshot.h:261` show the required UID/root/parent, generated
   key, VNUM/type, text, eight values, timers, flags, weight/material/cost/condition,
   craftsmanship, five affect words, fixed/dynamic affects and extra descriptions.
   Those are existing representations, not a proposed new wire format. Include
   untouched fields and unrelated roots needed by the original owner cut.
4. Let the original currency/item/native/save authorities review one compound
   service boundary: exact cost disposition, payer/keeper or permitted sink leg,
   repair before/after literal, original ore retirement or exact restoration,
   admitted output UID/body, and rejection/refund obligations. Reuse current
   transaction/source/receipt machinery only where its actual owner supports
   this producer. Smith currently has no keeper credit; do not invent one.
5. Publish only the retained committed result against original current-world
   custody and generations; hold conflicting saves and commands through the
   original save owner. A dirty bit, sent message, accepted submission or later
   template regeneration is not native completion. Associate business completion
   and guarded ACK with the original operation/receipt; failed publication keeps
   the original obligation/fences. No replacement authority/context is reserved.
6. Add owner-reviewed warm/replay/cold journeys only after authentic original
   setup, frozen outcomes, publisher/ACK and backend inputs exist. Verify exact
   original-ID replay, one charge/disposition, unchanged UID/literals, no new RNG
   draws, no second ore consumption/output/refund, and save nonoverwrite. The
   missing native inputs block this execution, not this source/design map.

## Manual baseline observations and dependent fault cuts

These are design oracles for future original fixtures, not executed cases or a
proposal to stub the unavailable owners. Use the real command/proc path with
distinct original customer and keeper, real configured shop and original carried
forests, real prototype 1255/weapon prototype and recorded native RNG outcomes.

| Setup / schedule | Manual original expectation and required comparison |
| --- | --- |
| Active REPAIR/FORGE, direct repair/smith/transact, service exception excluded | No paid selection/probe/detachment/debit/outcome/grant. Periodic smith hum is a distinct cosmetic route. Prove gate placement and real dispatch separately. |
| Repair accepted weapon, cost 1000/condition 50, changed attack type, payer below 100 copper | Attack type changes to probe result before failed payment; condition stays 50; gem selection is null. Freeze complete before/after literal and actual probe lifecycle, not an all-fields-unchanged refusal oracle. |
| Same cost/condition, funded, keeper level 35 | Integer price 100; average 100, degenerate second range, no repair RNG state advance, final condition 100. Payment receipt/keeper credit and effect ordering still need independent native evidence. |
| Funded repair, keeper level 36, recorded first roll +10, recorded descending-range result 111 | Average 111, second range `(111,100)`, final condition 111. Do not re-roll during replay or clamp maximum to 100. |
| Repair weapon condition 100 or 0 | No priced payment/RNG; prepayment attack-type adjustment can still occur. NOREPAIR/artifact refusal precedes the probe and therefore avoids it. |
| Smith keeper VNUM 82518 (array index 1), input `1`, original menu entry 1 | Configured medium+small iron ores, two-tier price 5000; scan keeper inventory, detach each, use first selected material. Entry's two fixed modifier ranges are 1..1, output ARMOR. Customer's ore-only inventory is not selected. |
| Same smith with missing second ore, insufficient funds or synchronous debit refusal | Already detached roots are sent to customer in reverse order, not restored to keeper; compare exact UIDs/graphs and real placement result, no duplicate allocation. |
| Output creation or synchronous grant refusal after accepted debit | Requested 5000 refund and ore return occur; grant refusal discards output candidate. Verify actual durable refund completion rather than treating the call as a receipt. |
| Grant accepted, then original durable grant rejection/offline publication | Legacy smith has already extracted ore and sent success text. It has no supplied business callback to compensate; retain this failing native gap rather than manufacture atomic success. |

Required dependent cuts: before/after original item selection/probe; between each
ore detachment and before price/debit; after debit submission but before durable
completion; between repair RNG draws and before final literal; after candidate
UID creation but before grant; after accepted grant but before ore retirement;
at each ore extraction; after durable result but before native publication; after
native publish but before guarded ACK; and save/copyover/cold restart at those
retained stages. Include original participant absence, reused keeper generation,
changed config/prototype, stale owner/revision, foreign/missing descendant,
publication failure and duplicate original-ID delivery. Do not run legacy native
gap cases and label them passing compound semantics; first agree the corrected
business contract and preserve original comparison evidence/failures.

## Smallest available nonduplicate acceptance proposal

The available public provider supports a **source-contract truth correction**,
not an honest independent compound runtime acceptance. Proposed future owned
paths are only `tests/async/test_smith_tradeskill_contract.py` and
`docs/persistence/economy_accounting/domain-separation/PAID_REPAIR_SMITH_SOURCE_CONTRACT_HANDOFF_2026-10-08.md`.
Neither is edited/reserved for implementation by this delivery. No new price
helper, native capture type, producer, driver or authority file is proposed.

The current test's docstring/method says "atomic compound transaction", but it
reads a 4,500-character smith substring and checks debit < creation < grant <
`extract_obj`, plus the presence of ADD_MONEY and a writer reason. It does not
execute C++, distinguish earlier `obj_from_char` detachment, test either refund,
prove accepted versus committed grant, or observe native publication/save/ACK.
The exact available input is the frozen complete public functions, configuration
and writer entries pinned below, with no world fixture needed for a text check.

Extend that existing test after source/design review: label it source-order only;
bound complete functions rather than a character window; retain its true ordering
and writer checks; add manual assertions for keeper scan/customer return, original
choice bound, service/direct active refusals, attack-type mutation before payment
and condition tests, disabled gem paths, and absence of a smith-supplied business
completion callback. Pair exact positive fragments/order with deliberate private
source mutations that must fail each assertion; those mutations are test strings,
not alternate providers or acceptance of modified production. Repeated parse
must not write the input. This protects concrete documented hazards against a
misleading "atomic" regression claim without duplicating accepted math controls.

Qualification required for that proposal: independent complete-function/source
review, original full-file Git/SHA pins, focused Python runner with bounded child
execution and preserved stdout/stderr/nonzero/timeout evidence, exact negative
mutation controls and a two-path diff review. It has no native setup, scheduler,
COMMIT/crash/ACK cuts or sanitizer claim: those belong to the dependent table
above. Its result must explicitly report native compound qualification false.
If a genuine executable component is requested instead, first supply the original
retained service participants, template/UID/native lifecycle, frozen outcomes,
currency/item receipts, save hold, authenticated publisher/ACK and cold SQL/flat
fixture. Do not link accepting stubs or promote text order to that evidence.

Existing `test_crafting_enhancement_regressions.py` checks forge price/type text;
`test_crafting_recipe_persistence_contract.py` checks recipe provider text;
`test_recipe_craft_transaction.py` and accepted R1 journey evidence target the
ordinary recipe owner. R13/R14 already qualify their policy scopes. None proves
the distinct repair/smith compound operation, and none is rerun here.

## Source/design verification and private evidence

`D:\Dev\Temp\paid-repair-smith-authority-20261008` retains the archive, complete
public path/byte/SHA/Git-blob manifest, separately pinned owned references,
review script/results/commands, final document copy, publication proof and hashed
evidence index. The bounded review compares every original archive body and
Git tree/blob with exact public primary, checks each appendix body pin, every
explicit source anchor and relative link/section, the concrete ordering/guard/
disabled-path/design assertions, and the sole staged path with whitespace checks.
The branch parent is the closed Hand commit
`c20288ff9f96c30f96c1721de8b8a8935d415ba8`; no primary adoption wait or closed
bundle rerun is required. This proof is source/design only.

The final bounded review passes: 3,575 original public files/blobs, eight owned
reference documents, 33 body pins, 110 source-anchor occurrences, 12 relative
links/sections and 32 design/scope checks. The Python review driver retains
combined output, exit status and a 120-second deadline; Git child reads have
30-second limits. Earlier private review/encoding/line-ending corrections remain
in separate attempt logs. No source-provider or native execution pass is implied.

## Frozen body authentication

The following public and owned-reference Git blobs authenticate the actual bodies
used above. The private manifest also retains SHA256 and byte counts.

| Scope | Path | Git blob |
| --- | --- | --- |
| public | `src/cmd/interp.c` | `74281b6f20ccd88a588d63cdda143da275258e85` |
| public | `src/core/random.c` | `16c9baf3985d46704794c1dc8eb55b362c6bc8bc` |
| public | `src/core/structs.h` | `5ba85524e8ceee7ae085fc6637b501d528a41fe0` |
| public | `src/core/utility.c` | `92bdffd57f8979d9531251d1b049887505d7715d` |
| public | `src/economy/crafting.c` | `14b5db3633c939434fbaa323ead1b8bac6e94d64` |
| public | `src/economy/economic_accounting_plan.c` | `8204617fcf1ea5f322f06897f9c5c8848f9aa38b` |
| public | `src/economy/economic_gameplay_authority.h` | `43891ba700f319de8313e9b0d2dd341b9b8a8be5` |
| public | `src/economy/shop.c` | `0e63a8b3a72c705bce5030caf6b828b57c8ccae5` |
| public | `src/economy/tradeskill.c` | `194a393c174c4067557777a88e36f82dcfc7de12` |
| public | `src/economy/tradeskill.h` | `b4c53e6c71cda17843760c37e142391b53fd61af` |
| public | `src/item/forge_items.c` | `0b0dfbc72842fef15556aafd055ba8ab7462956f` |
| public | `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` |
| public | `src/net/comm.c` | `8bcbc9ab95073b0d5fa71dc3f3516db183982eb1` |
| public | `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| public | `src/player/player_snapshot.h` | `bd9f5ca1810aa64fbc3251bb413ad870d923e1e3` |
| public | `src/specs/specs.assign.c` | `a0f1724469d4dd4c72d276ace11e03127fd3738d` |
| public | `src/specs/specs.mobile.c` | `bfa42787efcdd33d3deee7b668bf7793f273596d` |
| public | `src/world/db.c` | `da996337d17da9a0f81016e3f4c077405fd4fdee` |
| public | `src/world/handler.c` | `0054c7db2f7cb990dfde047e6de09250d54670e3` |
| public | `tests/async/test_smith_tradeskill_contract.py` | `88c958e7acfd082fcf856acd05c254a0c578a6e4` |
| public | `tests/async/test_crafting_enhancement_regressions.py` | `ba78d933de6687519134c9e65e9675b12618ee68` |
| public | `tests/async/test_crafting_recipe_persistence_contract.py` | `b66992a5077b00d8f91abe5ab30fb3a18192cde4` |
| public | `tests/async/test_recipe_craft_transaction.py` | `12ed4f36c7bc24dbb8e9e3907f783baf98f02441` |
| public | `docs/persistence/economy_accounting/writers.json` | `71fab7022992e198764b3732dde4936605ed7db1` |
| public | `docs/persistence/economy_accounting/registry.json` | `f3797ec71cd79c7f24fb0f3c015af6ef5ad39754` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/CURRENCY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `d3d1caab73fe706bdd7fc0e912fd7a2f83ede65e` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/HANDOFF.md` | `493abc0e785f339acf687b0bb347f79454bac373` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/ITEM_CUSTODY_DOMAIN_AUTHORITY_BOUNDARY_2026-10-08.md` | `70e8cc751e01bb283629582b7572365059725ccc` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/R11_ITEM_VALUE_QUOTE_HANDOFF.md` | `84dfbe7d1e7a76851c324d5ee8df7f58e364a5f4` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/R13_SHOP_ITEM_ACCEPTANCE_HANDOFF.md` | `55358d0ee2915a2d454256bdfc3b63e3bc47e549` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/R14_SHOP_CUSTOMER_ACCESS_HANDOFF.md` | `c10a93d30fda5450436f6721d4be30a979c60350` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/R2_RESERVATION.md` | `57ffc720ee8ade963017284ece3c0709daa04f82` |
| owned reference | `docs/persistence/economy_accounting/domain-separation/R4_SUPERIOR_TRIBUTE_HANDOFF.md` | `1c7613a8912c263f0d1653fe9e0c73adc1431b66` |
