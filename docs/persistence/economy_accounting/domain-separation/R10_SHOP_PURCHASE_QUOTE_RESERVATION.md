# R10 complete ordered shop purchase quote reservation — 2026-10-07

R9 is finally reviewed PASS at accounting
`81de2cbb46205fa3182f55ffecd93bb5272fc600`. Its source15d/305 and terminal
handoffde23 retain their original scope. Continuing Goal remains ACTIVE/no budget,
with full primary Plans1-5/applicable original R1-R8 implementation, integrated
supported gameplay/persistence/recovery, resolved required blockers and owner
completion disposition required. Worker R10 naming does not revise that scope.

Current owned branch93662eeaea1ecdba52a66b812c6fb1c03942a876 and primary81de have
raw-byte identical shop.c and shop purchase runner. The post-R9 nine-family
assessment selected the complete quote as a concrete candidate. Private current
fixture refresh and original executable controls now establish its feasibility;
no maintained shop source/test edits are made before exact coordinator review.
Primary continues without an adoption wait; private active shop overlap is unknown.

## Outcome, exact three files and complete quote boundary

Propose `src/economy/shop_purchase_quote.h` with a dedicated synchronous template
`int shop_prepare_purchase_quote(Observations&)`. It owns the COMPLETE original
purchase quote operation: canonical float charisma modifier, conditional race
doubling, float sell-percent computation and floor, conditional barter factor
adjustment, first native int truncation of item cost, int epic-discount subtraction
and final minimum1. Keep original float/double literals, intermediate assignment
conversions, strict comparisons, expression shapes and integer truncation. No
new saturation, finite/error policy, numeric normalization or input-range expansion.
The original caller assumes finite/representable products/subtractions; no promise
is added for undefined legacy casts/overflow outside that range.

In `src/economy/shop.c:shopping_buy`, replace only that calculation and obsolete
cost_factor local with a local synchronous observation provider and returned sale.
Keep every gate, selection, invalid-stock handling, parse/lookup, shop/batch/container
routing, trust, money/gem/payment, frozen trade preparation, lifecycle, publication,
callback/continuation, native outcome and recovery code unchanged. The quote occurs
ONCE before produced/batch routing; the same sale is reused for continuations.
Trusted-zero handling remains downstream. No second price/accounting plan or
frozen/native authority is added.

Provider observations preserve actual original points:

- `charisma_modifier()` supplies explicit float of canonical
  cha_app[STAT_INDEX(MAX(100,GET_C_CHA(actor)))].modifier, before later probes.
- `same_race()` observes the native actor/keeper race comparison at its point.
- `sell_percent()` returns native float at EACH original calculation/comparison/
  floor read; do not preload extra reads or refresh a captured factor after callbacks.
- `barter_enabled()` retains has_innate(actor,INNATE_BARTER), after the factor/floor.
- `barter_succeeds()` retains the WHOLE native expression
  GET_C_CHA(actor)>number(0,125), only when enabled. RNG stays native. C++ does not
  explicitly sequence these operands; do not split them into separately sequenced
  provider calls or earlier snapshots, and do not claim universal read-before/after.
- `item_cost()` reads selected stock cost AFTER the barter observation.
- `epic_bonus()` retains get_epic_bonus(actor,EPIC_BONUS_SHOP), AFTER first int sale
  truncation. Discount truncation/subtraction and minimum1 follow that observation.

Header stores no provider/game pointer, callback, configuration, RNG state,
allocation, receipt or source capability. Native local provider retains real
pointers/lifetimes. Canonical native char/race/stat/bonus identifiers and actual
field types are reused. Stable common include context must apply to bare primary.

The third file is existing `tests/async/test_shop_purchase_usability.py`: retain
EVERY original Python ordering/list/help assertion, runtime parser/quantity/batch/
container/message/decline/refund/wallet/grant/refusal assertion and compiler flag.
Add actual header and complete quote controls. The necessary current fixture
closure changes below are proposed explicitly as part of that same reviewed
actual-caller regression package, not an unrelated shared-driver cleanup. No new
maintained runner or other production/test driver is selected. Source-only and
full package must both apply independently to bare primary, with no R0-R9 prerequisite.

## Exact current fixture closure and original control fidelity

The unchanged maintained runner currently FAILS compilation before any runtime
controls: duplicate refusal default argument and missing current keeper/completion/
publication/accounted bindings. Original failed compiler log, command/result and
source pins remain in ignored domain-post-r9-feasibility-20261007.

Private ignored domain-shop-feasibility-20261007 refresh compiles COMPLETE current
shopping_buy and all22 original selected native functions plus the complete current
shop_trade_find_original_keeper and shop_trade_completion_impl bodies. Declaration
defaults remain; remove defaults only from generated definition signatures to avoid
duplicating them. Native function bodies remain unchanged. Keep native structs,
real headers, canonical helper IDs and field types.

Original PRELUDE world/wallet/grant/publication endpoints remain controlled fixtures.
Add the existing publication header and explicitly typed physical-before-completion
service binder required by current producer callbacks. Fixture success places the
selected object/nesting and modeled recorded keeper cash before completion; failure
leaves modeled publication refused and retains original receipt/message controls.
This models the original test's declared world/publication endpoints, not authentic
native materialization, proof sealing or ACK. All original DRIVER assertions remain;
only its physical-phase binder changes to the current typed callback order.

Accounted production availability is false in this fixture. Both unavailable
accounted start/continuation endpoints assert(false) if entered; no native token,
capture/checkpoint, source, SQL/coordinator, publication/ACK or recovery authority
is fabricated. Existing active-unavailable refusal cases remain and pass. New quote
controls exercise inactive SQL/flat COMPATIBILITY paths through the complete caller;
these runtime mode fixtures do not qualify actual backend services or accounted
successful trades. This limitation remains explicit in final proof.

The preliminary original refresh passes every original runtime assertion with
unchanged C++20 -Wall/-Wextra/-Werror, -g, ASan/UBSan, frame-pointer, function/data
sections, real src/libxml/mysql include paths and linker GC flags. Root independently
authenticated all22 original bodies plus two current bodies and preserved DRIVER/
PRELUDE changes; its preliminary review did not rerun that baseline. Private AST
execution also PASSes ALL original main() Python assertions before generation,
including epic rejection order, buy parse-before-route, listing and help content.
The maintained final package must still execute those assertions normally.

## Complete original extended quote proof and preserved expectation corrections

Complete ORIGINAL producer plus all original controls and **23 quote scenarios per
controlled mode (46 total)** compile/run PASS with the same flags and a30-second
external runtime guard. No original assertion or range control is removed/weakened.
Coordinator independently compiled/executed the exact final CPP with the original
recipe, exits0, and authenticated all24 native bodies and all56 original DRIVER
assert statements retained verbatim. Its independent review also confirms23 new
scenarios per controlled profile. Root evidence remains private coordinator-shop
independent-original-RESULT.json/original-controls-integrity-RESULT.json; those
checks do not replace the separately executed original Python outer assertions.
Generated final CPP SHA256
`d095f5c95b23e41a3bdfcb2590793cec57df506f9779e88977a89fe897825bbc`.

The18-value matrix covers canonical signed-byte modifier and min100 stat-index,
same/different race, floor, strict/equal/failed barter, float percent, first int
truncation, odd positive epic discount, zero/greater-than-sale discount, negative
controlled bonus, small/zero/negative resulting factor and final minimum1. Additional
cases cover state changes during RNG and epic observation, one quote reused by a
two-copy continuation, parse refusal before any quote and unavailable-accounted
refusal before any quote. All prices are finite/representable; controlled unusual
bonus/percent observations are policy cases, not genuine native bonus distributions.

Exact complete-caller traces include later legitimate CAN_CARRY_N STAT_INDEX200
queries, rather than truncating/filtering them away. Flat initial purchase has one
later carry query; SQL produced initial purchase has two. Complete two-copy delivery
has two/five carry queries respectively, with exactly one quote's stat/innate/RNG/
epic sequence. All wallet/submission/delivery assertions remain.

The original RNG-mutation case establishes the supported GCC13/default-optimization
component observation: native float factor1.01 remains captured, the original
comparison observes charisma100 before number() changes it to0, successful factor
becomes float.76, later stock cost120 yields int91, current epic.5 discounts45,
final fee46. This is actual compiler/profile evidence, not a universal language
operand rule. Keep the native comparison expression together in the provider.
The epic-mutation case keeps previously truncated sale101 while bonus observation
changes stock/config; odd half discount yields51. Native later reads/captured earlier
factors are protected without broad snapshots or recapture.

Retain initial original experiment failures: first omitted a later carry query;
then an added expected67 assumed newly sequenced post-RNG charisma. Corrected from
actual complete original source/compiler behavior to46 BEFORE extraction. Later
SQL trace revealed its additional carry queries. Initial/diagnostic/corrected CPP,
logs and RESULTS remain; no baseline failure is mislabeled as a production defect.
The final generated controls preserve exact traced events and native output laws.

## Original preimages and qualification plan

| Input | SHA256 |
|---|---|
| src/economy/shop.c | `de8bdbe95cfa626c6ca869495ccc34afc0097af4c964d021ad2d66cb99d34393` |
| existing shop purchase runner | `d6d54260c7de565deb996d6834c5cde964a698c5e3ba024e714df8b01f184368` |
| complete shopping_buy body | `f976962ace6fdc04228f1f2adebb12fcf056d6e1afb05b8e358984e78abdc5ef` |
| complete native quote block | `bfda1477d3788d3374fb664fee0ff59f730b6b13a81d02011491d92848a77ba2` |

Private proof-index.json authenticates **26 feasibility files** with byte counts,
including actual original binaries/generated CPP, preserved failed experiments,
generators, Python check result and source pins. Original maintained fixture failure
is separately retained. None is committed. Current source/test remain unchanged.

After boundary approval, implement exactly these three files and run identical
complete original/extracted controls and every original main() assertion through
legitimate current fixture closure. Keep the declared observation/profile limits.
Run relevant existing shop parser/sequence/live-route/purchase/list/source contracts,
preserve original unrelated failures, both strict maintained SQL/flat builds,
formatting and whitespace, exact bare-primary production/full import and actual
bare package component plus full shop.c SQL/flat module checks. Pin tested/committed
source, binaries/logs/terminal, actual tree/archive bodies and dependency disposition.
Existing740-object maintained graphs remain separate from current754 links and
unrun genuine native player/keeper/source/publication/ACK/recovery/backend journeys.

Ablation removes generic multi-action pricing, new service layers, early snapshots,
separate RNG/charisma operand calls, quote repetition, new clamp/error policies,
accounted capability mocks, shared native owner/manifest changes and extra maintained
test files. Remaining changes own one complete existing quote and its necessary
actual-caller regression package. The exact three-file boundary awaits coordinator
review before maintained edits; all native/shared primary ownership is preserved.
