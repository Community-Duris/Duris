# R13 complete shop item acceptance reservation — 2026-10-07

This proposes exactly three maintained files, subject to coordinator review before
implementation. R12 deliverye74/final accounting973 is closed at declared scope.
No R13 maintained implementation has been edited. The continuing Goal remains
ACTIVE/no budget through the original primary finish line; no activation occurs.

Outcome: move the COMPLETE trade_with item classification policy into a domain
calculation, preserving its native int result and every original observation/query.
The public `int trade_with(P_obj, int, char)` wrapper remains. Its actual common
get_selling_obj caller serves sale, valuation and repair, including their distinct
repairing modes. Cost, no-sell/transient flags, configured-type search, empty wand/
staff rejection, armor/worn compatibility and keyword-query behavior form one
connected acceptance operation. This is a business validation boundary rather
than a price coefficient or a new native admission owner.

Non-goals: get_selling_obj lookup/messages/body, evaluate_expression parser/body,
shop_producing membership, customer/keeper eligibility, sale/value/purchase quotes,
repair/smith effects/material lifetime/payment, active trade admission, snapshots,
UID/custody/capability, publication/ACK, proof/recovery, metadata/config owners,
shared runners and every public signature/schema/format. No new null/malformed
input checks, config-list bounds, normalization, allocation, suspension, service
or durable token. Valid original sentinel-terminated configured arrays remain
the supported input; malformed arrays/null objects are unqualified.

Files:

- New `src/economy/shop_item_acceptance.h`, owning the entire original classifier
  with canonical ITEM/OBJECT constants and int counter/result types. It stores no
  native pointer, provider, callback, config or authority.
- `src/economy/shop.c`, limited to one include in common stable context and the
  trade_with calculation replacement by one local synchronous borrowed provider.
  No change to get_selling_obj or any shop/native/metadata operation.
- New `tests/async/test_shop_item_acceptance.py`, first complete classifier AND
  real get_selling_obj execution harness with the exact38-control PRELUDE/DRIVER.
  Existing R12's controlled selection fixture and original86 assertions remain
  unchanged. Replacing that selection fixture would change the established quote
  controls, so this separate uncovered acceptance harness is necessary. It has no
  optional R0-R12 header/body dependency or shared-driver refresh.

Proof: preserve original38 baseline/Og controls first; execute identical extracted
actual classifier/selector controls, smallest adjacent shop checks, formatting and
both strict maintained builds. Full three-file/production two-file patches must
apply to then-current bare primary without optional ancestry, qualify the complete
native shop module under SQL/flat types with actual compiler inputs and run the
actual new runner in the full export. Pin code/source/CPP/logs/ELFs/graphs/imports/
trees/archives/all exported bodies and terminal states for final review. Owned740
does not establish current754, native journeys or adoption.

## Complete policy and exact native observations

The domain classifier retains int counter and the original char repairing input.
The borrowed wrapper provider retains P_obj and shop index for this synchronous
call. It supplies current int object cost, canonical native unsigned-long extra
flag tests, `::byte` item type, int value[2], current int configured buy-type by
index and the real native `evaluate_expression(item, SHOP_BUYWORD(shop_nr, index))`
call returning int. No early copy of the config array, fields or keyword string
is made. Current SHOP_BUYTYPE/WORD macros and actual metadata helper remain native.

Cost<1 returns OBJECT_NOTOK before flag/config/query observations. Native
`(NOSELL && !repairing) || TRANSIENT` short circuit remains: nonzero repairing
bypasses NOSELL, including native negative char, while TRANSIENT still refuses.
The loop rereads configured types at each original sentinel/match/fallback point
and reads object type/value at the original points. Matching wand/staff with
value[2]==0 returns OBJECT_DEAD before keyword evaluation; negative charges retain
their native accepted behavior. Exact ITEM_ARMOR config with ITEM_WORN object
returns OBJECT_OK without keyword evaluation; the reverse is not added.

On an exact type match, the existing keyword evaluator is called exactly once
unless the empty wand/staff branch returns first. Both evaluator true AND false
return OBJECT_OK in the ORIGINAL source. That behavior is preserved: no predicate
fix or removal of the callback is proposed, even though the return branches match.
The query may have native diagnostics; the provider delegates at the original
point, and does not synthesize grammar or authority. A controlled mutation changing
item type/cost/charges and the config sentinel during evaluation still returns
OBJECT_OK after one call, exactly as the original. It is not revalidated afterward.

The actual selector keeps its native visible lookup, then this classifier, pointer
return and OBJECT_NOTOK/OBJECT_DEAD message switch, including msg=0 suppression.
Classification is a transient business result, not complete source/lifetime/
custody/admission or publication authority. Native command guards and subsequent
price/effect paths retain their owners. Repair/smith's compound physical operations
remain coupled and are not selected by this reservation.

Plan ablation removes snapshot/config containers, enum translation, a trade
service, keyword-parser refactoring, extra guards and any new prepared transaction
or durable grant. A whole classifier with unchanged native wrapper and first
actual selector/classifier controls is the sufficient scope. The connected
validation has existing real consumers and original executable evidence; it is
not selected merely to fill the queue.

## Executed original feasibility and limits

Immutable primary is `e0b93aa48a0e25ec447e1f3b7dc02f2ff574f318`; inspected
successor973 is documentation only for src/tests. Whole original shop SHA256
`de8bdbe95cfa626c6ca869495ccc34afc0097af4c964d021ad2d66cb99d34393`;
trade_with body `fc6c59c0764a0ffde21a2276b6a7c0e76d2a384b13d48acdea36e482474a7a5a`;
complete get_selling_obj body
`1391832567e708171cc988cd2ce72c23a1bc1e4a4fdd6340bca18e3ecd73bb13`.
Both bodies compile unchanged with canonical structs/macros/constants.

Original-v0 CPP SHA256
`6df147e81ca1eb4ee00e860d0ffad20b254fb767445a10e9383540b21e7d2c99`
executes38 controls per baseline and -Og. All four compile/runtime exits0 retain
C++20/Wall/Wextra/Werror/debug/ASan/UBSan/frame-pointer/sections/GC and compile120/
runtime30 limits; only the supplement adds -Og. No fixture compiler/runtime
failure occurred. Actual compiler dependency capture pins272 input bytes; all34
tracked headers authenticate against immutable primary Git bytes. The read-only
container uses the pinned GCC13.3 image, isolated writable bin and network none;
no database/server/player data or optional extraction header is used.

Controls cover cost -1/0/1/100, char repairing -1/0/1/127 with NOSELL, TRANSIENT
under repair/sale, wand/staff charges -1/0/1, keyword result -1/0/1/2 and null
keywords, armor/worn one-way compatibility, later match, first sentinel and first
duplicate-match precedence, and query mutation after the decision. Complete real
selector controls cover successful pointer/msg0/1/-1 behavior, missing lookup,
cost refusal with suppressed/output messages, exact empty wand/staff message,
repair NOSELL bypass and lookup mutation observed by the subsequent classifier.
The query and visible lookup are controlled native endpoints; checked substitution
is a controlled formatter binding, while the actual DEAD snprintf remains real.
This does not qualify genuine keyword parsing, visibility, object lifetime,
config/world mutation, custody or gameplay/persistence/recovery.

Private evidence is `bin/tests/domain-shop-eligibility-feasibility-20261007`.
The19-file SHA256/byte-length index is
`1a3e84d8d3f78052513e1edc8117d5173b6ac24599bd6562f0131cff327d512d`.
Both profiles' actual CPP/binaries/logs/commands/deadlines/results, complete source,
fixtures and actual compiler manifest are retained. All jobs are terminal.
Independent original proof and the exact boundary await coordinator review;
no extracted compatibility, implementation, new native prerequisite or adoption
is claimed. The [post-R12 dependency disposition](POST_R12_OWNER_DEPENDENCIES_2026-10-07.md)
records the comparison with the remaining nine-family owner work.
