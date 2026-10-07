# R6 enhancement affect-policy reservation — 2026-10-07

Proposed next owned validation boundary after R5; code awaits coordinator review.
Current primary `35298aacdf0b55e5026db98103f4e3e7b63094ad`, owned final R5 source
`b31809a0d`, terminal handoff `9d99bdd94`. Actual continuing Goal is ACTIVE/no
budget. R5 review and primary integrated completion remain separate dependencies.

## Outcome and real callers

`is_enhance_banned(P_obj)` mixes native object access with five global configured
allow masks. The stable rule rejects any affect bit outside the matching word's
allow mask. Ordinary enhancement uses it for source and non-pouch material;
superior/essence paths use it for source/material; the boot-time random template
pool adds its zone/VNUM checks after this predicate. Those real callers retain
the shared wrapper. Supply five owned affect words and five explicit allowed
words to a local pure validation helper instead of passing a game pointer or
reading config globals inside the rule.

Predicate body SHA256
`120f3a13f836e276fbb7e4b05b3913fc846f6498cb7d2237e34fc8da86e545a9`
is identical in current352 and the owned source. Ordinary payment regression
preimage SHA256
`7c8db806204d68b5a7d2d851c86abf9ef34145d09beed3ec97f0473eb3c0654b`.
Actual `obj_data` fields and all five globals are **unsigned long**, verified in
`core/structs.h` and `item/enhance.c/h`. Preserve that native width and complement
semantics; do not cast to32/64 bits or infer width from the "32 bits" comment.
Unknown private overlap remains explicit.

## Exact three-file implementation and non-goals

New `src/economy/enhancement_affect_policy.h` uses fixed
`std::array<unsigned long,5>` owned words and checks `bits[i] & ~allowed[i]` in
word order. All-zero affects are accepted; all allowed bits are accepted; any
forbidden bit in any word rejects. No game/SQL/config types enter the helper.

In `src/item/enhance.c`, retain the public wrapper signature and its existing
null-object rejection before reading fields. Capture the five exact object fields
and corresponding globals there, then call the helper. Remove replaced bitwise
checks. Keep every existing caller, pouch-material bypass, error message, pool
filter order, config loader/reset, stat rules, fees and native effects unchanged.
This snapshots supplied values for calculation, not native lifetime, an atomic
config reload or an accounting/receipt authority.

Extend only existing `tests/async/test_ordinary_enhancement_payment.py`. Replace
its unconditional predicate stub with the actual production predicate body,
add the native five unsigned-long fields/globals and actual header. Keep all
original full ordinary producer/payment assertions, ASan/UBSan/no-PIE flags and
30-second deadline. Native FALSE/TRUE fixture constants match the production
predicate. The pre-existing R3 price-header/value controls stay intact; complete
test packaging therefore retains that R3 predecessor. Do not claim the full
test bundle applies independently to bare352. Production validation should have
an independently checked patch boundary.

## Proof and ablation

Run identical extended expectations on original predicate/ordinary producer and
extracted predicate/ordinary producer. Direct owned and native cases cover null
wrapper, zero/all-allowed masks, each word independently, native ULONG_MAX/high
bit, cross-word mask confusion, forbidden bits despite other allowed bits,
repeated snapshots and explicit-input independence from changed globals. Actual
ordinary source and non-pouch material rejection must precede wallet admission,
output probes/publication and retirement; pouch material still bypasses affect
rejection. Restore default zero facts before every original payment case.

Run unchanged pool-filter/config/module checks as appropriate, preserving the
known original module reset-call failure rather than changing its owner. Retain
superior/essence/stat/material and actual active-refusal controls; maintained
SQL/flat builds, formatting, current352 import/module checks and exact source/ELF
pins remain separately scoped. No genuine active compound outcome or NPC/world
journey is inferred from these components.

Ablation: omit pool zone/VNUM capture, enum/flag catalogs, config parsing/reload
changes, native prototype or target snapshots, eligibility rebalancing, RNG,
price/outcome/effect preparation, debit/retirement/receipt/ACK/recovery and any new
active route. Reuse std::array and existing function extraction/payment controls;
no framework or new test runner. The useful operation is the common five-word
allow policy and its actual callers, not another constant-table wrapper.

Current coin SQL reader/UID proof, item/native authority, shop/auction/Collector
captures, quest-prep paths and gambling deprecation remain their owners. Rerank
those nine-family facts after this selected proposal or a meaningful primary
advance; do not duplicate already owned plans or expand to fill a queue. This
reservation and its potential delivery do not close the continuing Goal.
