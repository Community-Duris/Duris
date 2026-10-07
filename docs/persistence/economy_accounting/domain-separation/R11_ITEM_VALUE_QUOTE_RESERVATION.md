# R11 complete item valuation reservation — 2026-10-07

R10 final declared-scope review PASS is published at accounting
`dc1489782eceb38ed51971bd2858db4a4d24b212`. Its immutable e6/740-object evidence
remains unchanged; see [R10 handoff](R10_SHOP_PURCHASE_QUOTE_HANDOFF.md).
This is a NEW exact reservation for coordinator review before maintained edits.
Private original feasibility is executed; no valuation implementation is made.
Current owned tip is0edd9867ad68257bc6990c5757bcf6e08bf6d96c, with R10 review-link
updates accompanying this reservation. Current primary isdc1489782, including
88f745c85 atomic flat coin source claims and3010d250b typed borrowed-lock room-pile
reader. Those native changes and their owner-reported combined754 builds are
separate from R10's independently verified740 links and unrun native journeys.
Whole tradeskill.c remains byte-identical between owned tip and current primary.
The continuing Goal remains ACTIVE/no budget with its full unchanged finish line.

## Complete operation, outcomes and exact scope

Extract the COMPLETE existing529-line `itemvalue(P_obj)` calculation into one
synchronous dedicated operation, retaining its existing native wrapper and all
callers. The result is the SAME native int valuation, with original null0, minimum1,
forced-one special types, double arithmetic and conversion laws. This is a whole
operation; do not create individual wear/stat/proc coefficient wrappers, duplicate
an existing price or accounting plan, or extract the two existing metadata helpers.

Exactly THREE maintained implementation files are proposed:

1. New `src/economy/item_value_quote.h`: `template<typename Observations> int
   item_prepare_value(Observations&)` owns original null decision, all wear/affect
   contributions and multiplier changes, packed proc decoding/cast mode/rounding,
   native prototype-proc contribution, every ordered affect-slot rule and seven-way
   racial stat switch, weapon/backstab/armor contributions, two-hand adjustment,
   final multiplier, minimum and forced-one decision/notice sequencing.
2. `src/economy/tradeskill.c`: replace ONLY the itemvalue calculation with its local
   synchronous native provider and one operation call; add a stable common include.
   Existing int itemvalue(P_obj) declaration/prototype and every caller remain.
   Both get_mincircle and get_ival_from_proc bodies/table behavior remain untouched.
3. New `tests/async/test_item_value_quote.py`: the first direct complete actual
   itemvalue regression runner, compiling actual native wrapper/body with canonical
   structs/macros and the pinned original controls. Existing tests currently stub
   itemvalue for their separate producer tests; leave those tests and all their
   assertions/fixtures/flags untouched. No maintained shared driver refresh.

There is no direct dedicated original itemvalue runner in tests/async. A new focused
runner is justified by this whole-operation extraction rather than editing unrelated
producer fixtures to impersonate native valuation. The original/extracted package
must retain every pinned ORIGINAL executable assertion below. This reservation
adds no production interface, service, manifest, schema, state owner or authority.

## Native observation boundary and arithmetic laws

The header retains no P_obj, prototype proc pointer, provider, callback/config/RNG,
native world state or authority. It does not allocate or suspend. The local provider
retains the real object pointer synchronously and supplies native scalar observations
at the ORIGINAL points. Early whole-object/table/flag snapshots are not proposed.
No new saturation, finite/error checks, input normalization, clamp policy or broad
range qualification. Existing undefined integer products/conversions on malformed
or unrepresentable inputs remain unqualified and are not silently rewritten.

Keep double workingvalue0/multiplier1/mod, original coefficients/literals/order,
compound assignments and int spellcirclesum/numspells. Retain original fixed int
slot loop and MAX_OBJ_AFFECT. Preserve canonical signed `::byte` object type/affect
location, signed sbyte modifier, int value slots, unsigned-int wear/extra flags,
unsigned-long affect words and sh_int racial table attributes. Native high bits
must survive. Word5 is deliberately not valued by this original function.

The proposed provider has native presence, wear/extra flag and word1-4 affect tests,
int object-value reads, signed native type/affect-location/modifier reads, current
prototype-proc presence/value lookups, minimum-circle calls, seven native racial
attribute reads, native IS_BACKSTABBER and CAN_WEAR(ITEM_TAKE), and exact original
invalid-race/default/forced-value diagnostics. A small concrete method inventory
may name those native observations; do not add a new property registry or generic
pricing API. The header owns the seven explicit race switch cases; the provider
returns the corresponding current native stat_factor attribute, without duplicating
that switch/rule system. Canonical constants come from existing headers.

All initial wear multipliers and affect-word contributions occur before packed-proc
metadata calls. Preserve the special greater-spirit-ward ADDITIVE multiplier1.20.
For packed procs, all three spell IDs and level/chance double mod are captured before
the three ordered get_mincircle calls, including zero spells. The single/all mode
reads current value[5] AFTER those calls. Single mode counts nonzero captured spells,
uses integer `spellcirclesum / numspells` BEFORE multiplication and truncates the
product to int. All mode adds its double product without that early int truncation.
Do not preload later proc selection or object facts.

Prototype presence is checked AFTER circle calls, and its contribution uses current
obj_index[object->R_num].func.obj through existing get_ival_from_proc. That helper is
a pure classification table over native function identities and remains reused.
The original get_mincircle reads native spell/class/spec metadata and stays native.
Their genuine metadata outcomes are not newly qualified by controlled fixtures.

Each affect slot captures its modifier into double ONCE, then retains all original
fresh location/type tests and native stat_factor reads. Invalid-race notice stays
in place with original current identity/location arguments and captured mod; later
rules can observe diagnostic mutations while retaining the earlier modifier. Keep
dam/hit multiplier, positive/negative stats, HP/move/mana/regen thresholds, racial
rules, AC absolute value, signed saving throws/pulse, max-stat sqrt and all literals.
After the loop, read current weapon/type/dice/backstab/armor facts, then extra two-hand
flag, multiply, floor to1, and test forced types at their original short-circuit
points. Forced notice observes fresh native OBJ_SHORT/OBJ_VNUM only when accumulated
value differs from1; return1 remains committed even if that notice changes the type.
The final ordinary return preserves native implicit double-to-int truncation.

## Executed complete ORIGINAL feasibility

The entire original itemvalue body is compiled byte-exact, not a reduced reference
implementation. Private `bin/tests/domain-itemvalue-feasibility-20261007` retains
45 controls: 26 apply/type/modifier matrix cases, 18 additional object cases and null.
Every control passes under BOTH original baseline and separate -Og supplement.
Canonical obj_data/obj_affected_type/index_data, native masks/types and original
IS_BACKSTABBER/CAN_WEAR/OBJ_VNUM macros are used. Controlled endpoints are circle
lookup, proc-value lookup and debug; native stats/prototype rows are declared
fixture inputs. The proc function identity is never invoked; it asserts if called.
The unreachable canonical noreturn corruption endpoint aborts if entered. No
native item/world/source/lifecycle/admission/publication capability is fabricated.

Controls cover positive/negative/threshold and signed-byte endpoints, null/base,
word5 noncontribution, word4 BIT32, native greater-spirit additive multiplier,
weapon/backstab, armor/AC, wear/two-hand interaction, proc decode/single/all integer
average versus fractional product, ordinary minimum and forced-key/teleport routing.
The helper mutation control starts with single-mode packed1000001001. First circle
changes value5 to0, level/chance and current prototype index. Captured spells/mod
stay intact; three calls remain1,1,0. Fresh proc lookup contributes7 and changes
next affect to STR3, giving15. Preloading single mode would instead change that
valuation, so this is a real observation-order control. Single packed1000002001 with
mod2.5 gives2 (integer average1 then int product2); all packed2001 gives7 after
preserving fractional7.5 to final return.

Invalid-race debug changes current slot location to STR_MAX/modifier10 while its
original captured modifier is0; the later multiplier still sees new location and
the native double result truncates to114 under BOTH supported profiles. Final
forced-key debug changes item type to LIGHT, yet native return stays1. These are
controlled ORIGINAL observations, not claims that genuine metadata helpers mutate
world state. Original assertions are established before extraction.

The first private fixture compilation lacked canonical objmisc.h, then the macro's
corruption binder, then incorrectly returned from the declared noreturn binder.
Each actual CPP/compiler log/result is retained. Canonical include and aborting
unreachable binder close the fixture without deleting/changing native source or
silencing a warning. No runtime control failed. The earlier42-control successful
CPP/results and strengthened selection test remain retained; three new controls
were added for integer/fractional proc arithmetic and native BIT32 before extraction.

Baseline recipe is C++20/Wall/Wextra/Werror/debug/ASan/UBSan/frame-pointer/section-GC,
canonical src/libxml/MySQL includes, with compile120/runtime30 guards. Supplement
adds ONLY -Og to that recipe. Neither replaces the original baseline. The actual
maintained strict Makefile/backend profiles are later, separate qualification.

| Original input | SHA256 |
|---|---|
| whole src/economy/tradeskill.c | `d71fed61642677307d95bdffeb63a3615a23563f13796814b2ceab68037d70d7` |
| complete529-line native itemvalue body | `d854888f9c593e9aac233c285d31790498f99dff3a3a0014059ec5186c429d90` |
| final complete45-control original CPP | `1cdbc778eddb91b3fdf5df6a655e9029dd939f8fcab01d7103bc5b04ce97a42d` |

The private32-file SHA256/byte-count proof index is
`6c6f769d04be4bab9069be1bd490ef7871b4cd4c6ad9bb0cfdc091dd57dc832d`.
It includes actual CPP/binaries/baseline/Og logs/results, all preserved compilation
closures and earlier successful controls. No artifact is committed.

## Qualification, ablation and owner limits

After exact boundary approval, implement only these3 files. Execute identical
complete original/extracted45 controls under unchanged baseline AND Og recipes,
retain original assertion strings/native function bodies/endpoints and preserve
all failures. Run relevant existing enhancement/crafting/valuation/forced-load
contracts, preserving original unrelated fixture failures; do not edit another
shared driver to obtain a pass. Format changed native lines/new header, whitespace,
and both strict maintained SQL/flat builds. Pin actual committed inputs/CPP/logs,
terminal status and retained native ELFs/object graph. Actual bare-current complete
and production packages must apply independently without optional R0-R10; execute
complete component and full tradeskill.c SQL/flat module checks on immutable exports,
authenticate actual patches/tree/archive/bodies and publish exact dependencies.
Current754 owner-reported builds/native coin changes remain separate from owned740
and this calculation proof. No native journey or adoption is inferred.

Ablation removes whole-object early snapshots, unrelated metadata table extraction,
per-coefficient wrappers, generic price/service layers, changes to producers or
level/material/RNG selection, native cost/effect/admission/lifetime/payment/custody,
shared codec fixes, schemas/receipts/publication/ACK/recovery, and reopening any
intentional refusal/deprecated/declined routes. The complete operation, its small
native provider and first direct actual-body regression package suffice.

Quest prep owns the newly found three-canonical-provider codec fixture closure;
this reservation does not duplicate that work or unblock its genuine native quest
prerequisites. Primary owns current native source/coin/room-pile work and remaining
integrated Plans1-5/applicable original R1-R8 qualification. Review/adoption remains
unknown until published. This proposal is useful continuing work, not a new finish
line or milestone. No deployment/activation; primary never waits for optional work.

## Verified native statement boundaries — implementation checkpoint

Direct inspection of pinned native tradeskill.c lines2026-2028 shows:

```cpp
spellcirclesum = get_mincircle(spells[0]);
spellcirclesum += get_mincircle(spells[1]);
spellcirclesum += get_mincircle(spells[2]);
```

These are THREE separate full expressions, not one addition expression. The
current extraction substitutes the same native observation in each statement,
retaining that assignment/compound-assignment grouping and sequencing. The
commented diagnostic with three call arguments is not executable code. Original
whole-file/body pins d71fed616.../d854888f... remain unchanged and both actual
original/extracted baseline/Og45-control executions pass. No calculation edit or
new temporary is needed for this clarification; do not replace native statements
with a newly grouped addition. This corrects the later draft-review phrasing while
preserving its exact-rule audit and approved boundary. R10's native barter operand
expression remains a separate, unsequenced expression with its original profile
limits; that observation is not transferred onto these native statements.
