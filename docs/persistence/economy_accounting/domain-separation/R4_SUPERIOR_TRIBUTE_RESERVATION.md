# R4 superior tribute-count preparation reservation — 2026-10-07

Next continuing operation F3-materials. Proposed only; no code edits before
coordinator boundary review. R3 final handoff is locally committed but normal
publication currently receives GitHub internal server errors. Work and continuing
Goal stay preserved. This checkpoint never supplies an overall finish line.

## Exact boundary and invariants

Producer candidate `275df7f626e12cb396a22da34317a4e7f355e9a1`; owned source has R3
price code at `3a722a008` and relocated config test `6d544bea7`. Reserved material
functions are raw unchanged by R3 and producer integration. Complete body pins:

- `static bool scale_superior_material_count(` SHA256 `07b347c83693a3fbe93bc0293bbe8c121922028e0c782bb218724389974e1735`.
- `static bool superior_plan_add_material(` SHA256 `f23e48aa7f6ae030a9251ef2c1c6f6cd354eb65540afd00921ef94e7a02895f2`.
- `static bool build_superior_enhancement_plan(` SHA256 `45021a6b5fb33ec7e4a11b7a8ce8d647ff02fe7c1d740dee4898bc5d7c3ae2fc`.

Outcome: separate the two numeric tribute counts from native target-table/item
capture and mutable global multiplier. New local header
`src/economy/enhancement_material_quote.h` holds owned low/high counts, exact
existing scaling from explicit multiplier, and preparation from native int target
item value. Inputs are captured target `ival` and exact
`enhance_stat_material_quantity_multiplier`. No material VNUM, live object/stat,
world/SQL/RNG, revision or identity enters this numeric operation.

Move existing `scale_superior_material_count` and local wide
`(int64_t(target->ival)+4) /5 and %5` rules once into the header; remove replaced
source logic. `build_superior_enhancement_plan` retains target selection,
read_object/get_matstart/extract_obj, VNUM calculation and aggregate/slot ordering.
Invoke the quote at the same point. Only successful counts feed unchanged
`superior_plan_add_material`. Remaining-step catalogue, material capacity/overflow
and partial overall-plan failure behavior stay unchanged. No new VNUM policy.

Negative target value refuses; zero remains accepted (different from Craft R1's
minimum1). Finite positive multiplier and nonnegative counts remain required.
Exact `count * multiplier +0.999999` then bounded truncation stays unchanged;
do not replace it with ceil or silently unify R1's policy. Nonfinite or quote
>=INT_MAX+1 refuses before narrowing. Temporary joint output assigns only on
success, preserving sentinels. Those counts were local to the native producer;
its existing partial plan remains unchanged on failure. Tiny positive multipliers
may produce zero tribute as before; no minimum/rebalance policy. Native int input
bounds the int64 target+4 computation; no broader numeric domain is added.

## Files, proof and ablation

Only new header, existing source helper/quote region in `src/item/enhance.c` and
`tests/async/test_superior_enhancement_material_bounds.py` are proposed. Extend
that existing harness with real header and unchanged native plan/aggregation.
Keep original ASan/UBSan/float-cast-overflow, no-PIE flags, assertions and30-second
execution deadline. Execute identical extended expectations on retained original
bodies and extraction. Direct owned cases cover zero/INT_MAX/negative target,
exact fractional rounding quirk, invalid/nonfinite/nonpositive/extreme multiplier,
low-first/high-later overflow, unchanged joint output and repeated preparation.

Run adjacent material/all-stat/config contracts, price controls and actual active
refusal as appropriate; both maintained builds preserve controls and exact pins.
Candidate component/type/import checks remain separate from753-provider native
builds or payment/journeys. Existing `superior_plan_add_material` is already owned:
reuse it rather than add another aggregation strategy. No new test framework.

Ablation: omit target eligibility, stat caps, catalog snapshots, frozen RNG/search,
aggregation refactor, source/admission, debit/effect/refund/receipt/ACK/recovery,
config format and active enhancement support. Useful seam is numeric quote plus
actual producer; other changes need their own concrete reviewed boundary.

After delivery rerank actual nine-domain work and owner dependencies against new
requirements. If ownership blocks this seam, record exact interface and advance
another feasible operation. No primary adoption/wait or continuing Goal closure
is implied by this reservation or R4 completion.
