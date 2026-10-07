# R12 staged shop sale and valuation quotation reservation — 2026-10-07

This proposes exactly three maintained implementation files for review. No
maintained implementation has been edited. R11's final declared-scope review is
closed at accounting0fd; the continuing Goal remains ACTIVE/no budget through
the original primary finish line. This proposal is not implementation approval.

Outcome: move the complete sale/value base-price calculations and complete
trophy-price adjustment into a domain calculation, with the actual native callers
invoking each stage at its existing point. Both callers retain their exact int
prices, float/double rounding, callback/property observations and native gates.
There are two real callers, with materially different policies and timing; the
proposal does not treat their quotations as identical.

Non-goals: parser/admission/selection, money, keeper cash, custody, active native
authority, capabilities, payloads, publication/ACK, proof sealing, persistence,
recovery, shop config/metadata ownership, native completion bodies, repair, smith,
purchase quotes and shared test drivers. No public signature, schema, storage,
wire format, normalization, saturation, widening, finite check or malformed-input
policy changes. No R0-R11 dependency, new service, dependency or test framework.

Files:

- New `src/economy/shop_sale_quote.h`, owning the connected ordered base quotations
  and trophy adjustment. It stores no native pointers, provider, state, config,
  RNG or authority. No allocation or suspension is introduced.
- `src/economy/shop.c`, limited to one include in stable common standard-include
  context, one local synchronous borrowed-observation provider shared by the two
  callers, their two base-calculation replacements and their trophy-calculation
  replacements. The obsolete float locals are removed. Original messages, guards,
  integer sale variable and every native effect/continuation remain at their points.
- New `tests/async/test_shop_sale_quote.py`, compiling the complete actual
  shopping_sell/shopping_value/refusal and parser bodies with the pinned canonical
  fixtures/86 original controls. This is their first direct valuation/sale quote
  execution coverage; existing purchase, command, runtime and source-contract
  drivers remain unchanged. Its closure includes no optional R0-R11 header/body.

Proof: final original-body baseline/Og controls first, then identical extracted
actual-body controls, smallest adjacent shop checks, changed-line formatting and
both strict maintained builds. Exact full three-file and production two-file
patches must apply to the then-current bare primary without optional predecessor
ancestry. Qualify the complete native shop module under SQL and flat types, pin
actual current compiler inputs, and run the actual new runner in the full export.
Retain all failures, source/CPP/ELF/import/tree/archive pins and terminal states.
No current754/native journey or primary adoption follows from an owned740 build.

## Exact staged boundary

The shared native provider borrows P_char actor/keeper, selected P_obj and native
shop index for the duration of the synchronous calls. It owns only native reads
and callbacks: canonical signed `::byte` charisma modifier cast to float through
the existing STAT_INDEX(MAX(100, GET_C_CHA(ch))); current GET_RACE comparison;
float buy_percent; int selected cost; sh_int condition; bool has_innate;
the whole native `GET_C_CHA(ch) > number(0, 125)` comparison; int sql_shop_trophy;
and float get_property results with the original double defaults0.05 and0.10.
It introduces no decision about pricing coefficients, caps, floors, trophy
eligibility or route admission. Literal decision policy belongs to the header.

The header has explicit sale-base and valuation-base entry points. Sale preserves
its native `cost_factor / 2.` for different race and makes no barter observation.
Valuation preserves its native `cost_factor / 2`, then conditional barter enablement
and the whole comparison. Native comparison operands are not specified to execute
left to right. Preserve that expression in one provider method; do not split RNG
and charisma into sequenced calls or replace either with an early snapshot.
The executed GCC profiles' mutation result15 is evidence for those profiles,
not a universal C++ operand-order guarantee.

Both base entry points own the full ordered float factor calculation: signed
charisma modifier, caller-specific race division, buy_percent multiplication using
the original double literals/division, repeated current buy_percent cap comparison
and cap assignment, caller-specific barter policy, then
`(int)(cost * cost_factor * MIN(100, condition) / 100)` and minimum1. The original
macro expression/read multiplicity is retained; neither std::min nor an early
condition snapshot replaces it. Selected cost/condition are read after valuation
barter, while the earlier computed factor remains captured. Undefined overflow
or out-of-range float-to-int conversion remains outside qualified inputs.

Sale calls its base entry point at old2374-2393, before the existing guild/roaming
keeper-cash and NODROP gates. Those gates remain in the native caller and observe
the BASE sale, not a prematurely adjusted final price. Only after they pass does
sale invoke the complete trophy adjustment at old2411-2421. Valuation invokes its
base at old2552-2582 and its adjustment at old2584-2594, after its own earlier
NODROP/artifact/encrust admission. Its message and barter differences remain.
Original line numbers refer to the pinned primary0fd shop.c below.

The adjustment entry point receives the captured int base sale and the borrowed
provider. It owns exactly one current trophy-count call, `count > 1`, int orig_sale
capture, trophy-modifier multiplication and int deduction, native MAX expression
with current minimum-percent observations, and final minimum1. It returns the
final int price plus the exact int trophy count. The native caller retains the
count for its existing common-item message and the sale caller's later `temp <= 1`
message branch. Both messages remain after calculation and before their original
route/effect work. The native MAX can call get_property twice when the floor wins;
that repeated evaluation remains, including the later returned value if a callback
changes it. No early property snapshot or result memoization is substituted.

This two-stage boundary is necessary: folding sale into one pre-gate final quote
would hoist history/properties across keeper/NODROP refusals. A coefficient-only
wrapper would leave the connected rounding, floor and history policy in the
caller. The proposed header owns both complete calculation stages while native
control decides whether the second is reached. Plan ablation removes generic mode
configuration, snapshots, a pricing service, new grant/receipt plumbing and shared
driver refresh. The two caller-specific base entry points, one shared adjustment,
borrowed provider and first direct regression runner are the sufficient scope.

## Executed original feasibility

Immutable source is primary
`0fd938ce2fbae7b7e675346d60b30b3867bea0f7`; complete shop.c SHA256
`de8bdbe95cfa626c6ca869495ccc34afc0097af4c964d021ad2d66cb99d34393`.
Complete shopping_sell body SHA256
`f473fcc3a1ffcc9c09a2ab3d31ac947ae9018f519c8ef8531ebf42fe1a337ba5`;
complete shopping_value body SHA256
`a1492a470c97c7476063a1fced14e07a6c99a58d2662f4c02aaa5ed029e3e9e5`.
The private harness also compiles exact native refusal, one_argument and lohrr_chop
bodies. The complete interp.c source authenticates to primary0fd, SHA256
`4f7dcf15adf0217193c9398c70c99d90ac59090f85f043b1b34e6d9be0df3dba`.

Final original-v5 generated CPP SHA256
`7798c1dc2ccf8ee3518ef5362c3cdc2418560aac7c3a1ddc4523d9b26fb8d95a`
passes86 controls per baseline and -Og, all four compile/runtime exits0 with
C++20/Wall/Wextra/Werror/debug/ASan/UBSan/frame-pointer/sections/GC flags and
compile120/runtime30 bounds. Only the supplement adds -Og. Actual compiler
dependency capture pins446 inputs; all70 tracked header inputs authenticate
against primary0fd. The independent production export has read-only source,
isolated writable bin and network none; the controlled harness uses no optional
R11 source. No database, server or player state is accessed.

Thirty-six matrix controls cover the two callers with positive/negative/min/max
signed-byte charisma, below-min/current stat indices, same/different races,
fractional buy percent/int truncation, zero/negative costs/percent and native
condition below/equal/above100. Remaining controls prove trophy counts0/1/2/20,
integer deduction/floor/final1, callback property changes, repeated MAX property
evaluation, trophy mutation after base capture, and STAT_INDEX mutation before
current percent/condition reads. Sale has no barter even when enabled; valuation
tests strict success/equality/failure, RNG bounds and mutation of later cost/
condition/config/charisma. Under both executed profiles that mutation gives15.

Actual caller controls also cover is_ok, parse-empty and failed lookup, artifact,
encrust, NODROP, potion and nonempty-container sale refusal; sale's pre-trophy
roaming-cash guard, equality and guild exception; SQL price/message/money handoff;
flat pending/refused price handoff; busy refusal; active sale refusal and active
read-only valuation. Canonical structs/macros and real native gate/body order are
compiled; SQL/money/world lookup/movement and flat payload/submission are controlled
fixtures. Active production availability is false; accounted admission, physical
publication, completion and unsupported retirement endpoints assert if entered.
No real receipt, proof, capability, publication/ACK, recovery or native lifetime
authority is manufactured. Gameplay/backend durability is unqualified.

Private evidence remains in `bin/tests/domain-sale-feasibility-20261007`.
The49-file SHA256/byte-length index is
`b55f629c4f4467eb969df450d785b4b287ada688bdf8b7c9b63cb7b29fdd1194`.
All supervisors/children are terminal. Retained closure failures: v0 fixture name
collision and canonical SQL return type, v1 missing actual salchemist constant
header, v2 missing parser log binding, and v4 newly added fixture name typo.
They were fixed only in the private harness with canonical headers/signatures;
original maintained bodies/flags/assertions remain unchanged. Earlier successful
v3 retains75 controls; the coordinator independently authenticated that complete
CPP/native bodies and ran baseline75 in a distinct namespace. Final strengthened
v5 and Og remain separately labeled worker-executed evidence pending independent
reservation review. No implementation or extracted compatibility is claimed yet.

The [post-R11 nine-family assessment](POST_R11_OWNER_DEPENDENCIES_2026-10-07.md)
compares this concrete two-caller calculation with remaining native owner gaps.
The primary continues independently; this proposal and its optional future import
never become a primary wait or a substitute finish milestone.
