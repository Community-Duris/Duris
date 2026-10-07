# Coordinator boundary review: enhancement prices - 2026-10-07

Published reservation `c97e97458` is approved for the narrow pricing extraction.
The implementation and independent payment checks below are published; maintained
build and adjacent-contract handoff review remain open. This is a delivery checkpoint
under the [continuing project charter](CONTINUING_PROJECT_COORDINATION.md).

## Exact boundary and independent source evidence

Candidate `275df7f626e12cb396a22da34317a4e7f355e9a1`, coordinator documentation
revision `b1ac97c3a9a1d1e50db6657b71862fb65681ce41`. The coordinator inspected
ordinary, superior and essence producer bodies and their existing payment tests,
and independently authenticated every reserved preimage:

| Source/body | SHA256 |
|---|---|
| `src/item/enhance.c` | `dc50a3fc114c29d63cba855f1e3c790420724274911693a01bf971717aa60f12` |
| `void enhance(` | `00e9089adeea023f57739db010c235cff0db09a574a9d2b43c7dbf285b2a3e90` |
| `static bool perform_superior_enhancement(` | `82a248c68f83ee51c47490316e4052b89c3e7b6e633e8d42a715cd3222d82784` |
| `void modenhance(` | `cba76997db05e56031b1a6c3f84c4a9e5130bc27cca54b251d093287480ba864` |

The source is raw-byte identical between the original accounting base and the
new producer candidate. Existing quote regions have three real callers and no
shared native quest/auction/recovery change. Unknown private overlap remains
unknown; optional primary import never requires a wait.

Move only price selection/range validation into the proposed owned helper header
`src/economy/enhancement_price.h`, and replace only the corresponding price regions
and include in the three producers. Preserve native int facts and checked int
outputs, success-only assignment, ordinary selected negative-fee refusal and free
zero fees. Superior keeps its exact wide `base + value * per_value` expression
and [0,INT_MAX] range. Native int32 products plus an int base fit int64 even at
signed extremes; do not broaden input range without preserving that proof.
Essence keeps <=20/<=30/otherwise tiers1000/20000/100000, including existing
negative-material low-tier behavior and exact1/20/100 platinum messages.

Capture facts at their original points. Keep material checks, RNG, outcome search,
modifier caps, output probes, wallet insufficiency, debit admission and material/
item effects in their original order. In particular, essence money insufficiency
must still precede location/prototype probing. Active paid-outcome refusal remains
untouched. No custody/UID/source, refund, receipt/ACK, recovery, migration or
configuration-format change is part of this reservation.

## Required qualification and continuing disposition

Retain and execute the existing ordinary/superior/essence payment expectations
against original and extracted bodies with unchanged sanitizer/no-PIE/deadline
controls. Include the actual new header, add owned threshold/extreme/sentinel
controls, and preserve applicable adjacent config/material/stat/module and
actual active-refusal checks. A relocated source-contract assertion must still
verify the real rule and caller, not simply delete coverage.

Run maintained builds for the changed source and pin exactly what was compiled.
The worker's preserved branch and the newer753-provider candidate are distinct
sources: historical740-provider builds/journeys cannot be relabeled as current
candidate qualification. Keep optional-overlay/current-dependency checks and
current import applicability explicit. Payment fixtures have native/debit seams;
they do not establish a genuine server/DB compound outcome journey.

After this delivery, rerank the evolving queue against actual remaining code and
primary requirements. Superior tribute/material aggregation or other service
quotes may provide further independent boundaries after separate reservation;
already pure native quest coin/cost calculations are reused. Required primary
gameplay/recovery remains unfinished, so R3 completion cannot complete either
worker's continuing Goal or the coordinator Goal.

The coordinator also independently checked all three historical R0/R1/R2 patches
with `git apply --check` at `b1ac97c3a`, without applying them or changing the clean
separate review checkout. All pass. Collector is explicitly deferred in the
primary producer handoff; patch compatibility does not establish adoption or
updated runtime qualification.

## Implementation and independent payment checkpoint

Implementation `48b7a6d3873dc5dee2c7ea5b02ecce3eba8d9304` contains the owned
header and the three producer price substitutions, with payment/source-contract
coverage following the actual helper. Strict-compiler repair
`3a722a00893d4fcd8adeb07d610e387c2191aa93` initializes the superior local price
to zero; the helper success/refusal boundary remains unchanged.

The coordinator inspected both actual diffs and independently executed all three
published payment runners against the original producer bodies and the extracted
bodies at `48b7a6d`: six passes. Both variants use the actual new header and the
same published sanitizer/no-PIE/deadline and payment assertions. A private
controller substitutes only the source body supplied to the existing extractor
for the original variant. It changes neither subprocess controls nor assertions.
Compiler: WSL Ubuntu22 GCC11. The coordinator then independently ran the affected
superior payment runner at `3a722a00`: pass, including overflow/negative refusal,
exact debit, pouch and physical materials.

The worker reports terminal maintained SQL/flat build passes and current-candidate
payment/module type-check passes. Those reports await the exact build evidence
and source pins in its final handoff; this checkpoint does not independently
qualify the whole current candidate. An adjacent configuration assertion still
expects the relocated formula in the producer and is being updated to follow the
real helper while preserving the remaining assertions. Preserve the original
failed strict build log and any unchanged baseline module-contract failures.

Price/payment fixture equivalence does not establish genuine active native
compound enhancement persistence, recovery or accounting support. Primary import
and private adoption remain unknown. Continuing worker/coordinator Goals remain
active beyond this delivery.

## Final declared-scope review, remote publication pending

The coordinator reviewed local handoff
`b18e513bed85cde63d091767289571340687d577` and final source/test revision
`6d544bea7d74f8a671f480a95292c2266831292a`. The relocated configuration contract
checks the actual producer's configured helper arguments and the real wide
formula/range guard; unrelated assertions remain. The coordinator independently
executed that final contract: pass. All seven source/test hashes and every retained
proof-index hash match the actual files.

Both terminal build logs compile `item/enhance.c` once with the maintained strict
flags and complete the full 740-object link; controller records SQL_EXIT=0 and
FLAT_EXIT=0. The coordinator independently hashed both actual owned Docker ELFs:
SQL `951a7d45816510a8d76fe9d16cb95908250bec7592035e4ad8e64fbb9e7aa8e0`,
flat `e4aeda193f455c34679929e014dfbf088b28748e9df95af32723f8da9658dbd0`.
These compile production revision `3a722a00`, preserving the qualified R2 objects;
`6d544bea7` changes only a test. The first uninitialized-price failure is retained.
The coordinator also independently reproduced the identical module reset-call
assertion failure on unchanged producer source and extracted source.

Independent private-index application of R0, R1, R2 and the complete R3 source/test
delta to current accounting `507e4909ee6a570876f274c0787d7a83a987826d` passes.
No primary checkout or branch source is modified. R3 delta `c97e97458..6d544bea7`
SHA256 `90f146d34b5337fea7b10ebcf93991b49e675b1225acf7d20bd95af6a8e09734`;
combined current tree `9f556f8714e22dac84f1b76df9bb655f1143de72`.

Disposition: **reviewed locally at declared source/component/740-provider build
scope; remote final test/handoff publication pending GitHub write recovery**.
The already-published production implementation is at `3a722a00`. New maintained
753-provider overlay builds are now live and constitute additional pending proof,
not a relabeling of these ELFs. Current native compound gameplay/recovery and
primary import remain separate and unproven. The next extraction may proceed
after its own concrete boundary review; the continuing Goal remains active.
