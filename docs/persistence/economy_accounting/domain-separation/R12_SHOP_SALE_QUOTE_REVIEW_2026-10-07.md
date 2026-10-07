# Staged shop sale and valuation quotation review - 2026-10-07

Disposition: **exact THREE-file implementation boundary approved.** Reservation
`bd3cd628178a89f0491f960de1ee6300006d2fba`, reservation text SHA256
`1bfc70ac2b44cf0e2821f481231e79480b041313b5c02f2ed50067d2ef0eb558`, selects a
complete connected two-stage quotation operation with two actual native callers.
Implementation, extracted compatibility, current import/build/artifact review
and adoption remain pending. Primary continues without an adoption wait.

## Exact operation and owned files

New src/economy/shop_sale_quote.h owns both complete caller-specific base-price
calculations and the complete common trophy adjustment. Modify src/economy/shop.c
only for one common include, a local synchronous borrowed-observation provider,
the two base/trophy calculation replacements and obsolete float locals. Add the
first direct tests/async/test_shop_sale_quote.py runner. No other maintained file
changes: existing purchase/codec/runtime/source tests, metadata, native completion,
authority, schema, formats, migrations, coverage and Plan5 remain owned.

The header retains explicit sale-base and valuation-base entry points and the
common adjustment. It stores no game pointers, provider, callback, config/RNG,
state or authority and does not allocate or suspend. The local native provider
borrows actor/keeper/selected object/shop index for synchronous native observations
and callback access. It does not decide coefficients, price caps/floors or trophy
eligibility. No pricing service, generic mode framework or duplicate accounting
plan is introduced.

Sale has NO barter. Keep its different-race division by2. and compute base int
price before keeper-cash/guild/roaming and NODROP gates. Only after those gates
pass may it observe trophy count/properties. They test the original BASE price.
Valuation retains its different-race division by2, earlier admission and conditional
barter RNG. Keep its original GET_C_CHA(ch)>number(0,125) expression intact in one
native method: C++ does not define left-to-right operand evaluation here. Do not
split the reads/roll into newly sequenced calls or preload their inputs.

Both base calculations retain original float/double expressions/literals, fresh
native signed-byte charisma/stat-index observations, current race and repeated
buy_percent reads, cap comparison/assignment, native int cost/sh_int condition,
MIN evaluation multiplicity, int truncation and minimum1. Later object cost/
condition reads follow valuation's barter while the computed factor stays captured.
No widening, normalization, saturation, finite check or new malformed-input policy.

The adjustment receives captured int base sale, reads trophy count once, owns
count>1, orig_sale capture, integer deduction, native MAX minimum-percent expression
and final minimum1. Return final int price AND exact trophy count for existing
message branches. Preserve repeated get_property evaluation when the floor wins,
including a later different callback value; no memoization/early snapshots.
Original messages, parser/admission/gates, integer locals and all money/custody/
submission/publication/ACK/recovery effects remain at their native points.

## Independent original feasibility and exact limits

Immutable original primary is0fd938ce2fbae7b7e675346d60b30b3867bea0f7.
Successor6ca40d01652b29bd7faf475db2754ff628bdd228 changes documentation only.
Complete shop.c SHA256 is
`de8bdbe95cfa626c6ca869495ccc34afc0097af4c964d021ad2d66cb99d34393`;
sell body `f473fcc3a1ffcc9c09a2ab3d31ac947ae9018f519c8ef8531ebf42fe1a337ba5`,
value body `a1492a470c97c7476063a1fced14e07a6c99a58d2662f4c02aaa5ed029e3e9e5`.
The complete refusal and real parser/helper bodies are compiled unchanged too.

Final original-v5 CPP SHA256
`7798c1dc2ccf8ee3518ef5362c3cdc2418560aac7c3a1ddc4523d9b26fb8d95a`
preserves all earlier75 controls and adds11 for86 total. All baseline/-Og compile/
runtime exits0 under original C++20/Wall/Wextra/Werror/debug/ASan/UBSan/frame-pointer/
sections/GC and compile120/runtime30 guards; only the supplement adds-Og.
Coordinator separately authenticates complete native bodies/helpers/PRELUDE/DRIVER
and compiles/runs final86 baseline in its own namespace with actual primary headers:
exits0/0. Original GCC mutation15 is measured profile behavior, not language sequencing.

Coordinator authenticates all49 proof-index entries by bytes/hash, index SHA256
`b55f629c4f4467eb969df450d785b4b287ada688bdf8b7c9b63cb7b29fdd1194`.
It rereads all446 actual compiler inputs in the exact isolated original harness
container and verifies all70 tracked headers against current primary Git bytes.
Four original private fixture-closure failures and successful v3/75 controls remain
preserved. No maintained source/test change or production failure is inferred.

The86 controls cover signed/stat/race/condition/percent/truncation bounds, trophy
eligibility and deduction/floor/final1, later field/property mutations and repeated
MAX evaluation. Complete caller controls retain sale's no-barter/gate ordering,
valuation's conditional roll and strict comparison, early admission/refusal,
guild/roaming cash checks and actual prices delivered to controlled SQL/flat routes.
Native structs/macros/body order are real; database/money/world lookup/movement/
payload/submission endpoints are controlled. Active admission is unavailable and
native physical/publication/completion endpoints assert if entered. There is no
genuine world RNG, trophy database, source/lifetime/capability or backend journey.
Root private sale-original-review-RESULT.json records this original scope.

## Implementation and final qualification obligations

Implement only the approved three paths in the existing isolated architecture
worktree. Preserve complete original86 controls/fixtures, native noncalculation
bodies and original failure records. Execute identical original/extracted baseline
and-Og, relevant existing shop checks, changed-line formatting and both strict
maintained builds; pin committed source, generated CPP/logs, actual ELFs and object
graphs. Do not repair other shared drivers in this bundle.

Both full three-file and production two-file patches must independently apply to
then-current bare primary without optional R0-R11/prep ancestry. Authenticate actual
patches, resulting trees, archives/every export body and current compiler/header
inputs. Qualify complete native shop module under SQL/flat types and execute the
new maintained runner in the full export. Distinguish owned740 builds from current
primary754/integrated native qualification; qualify changed inputs if primary code
advances. Publish exact coherent code and canonical handoff for final review.

Quest prep separately owns the reviewed one-file enhancement boundary repair;
there is no duplication or mandatory cross-workstream dependency. Published6ca
native Collector/SHOP/flat boot progress supplies no completed native prerequisite
or primary adoption proof here. Original Plans1-5/applicable R1-R8, backend/gameplay/
persistence/recovery and the owner completion disposition remain the finish line.
Continuing Goal/monitor stay ACTIVE; no deployment or activation occurs.
