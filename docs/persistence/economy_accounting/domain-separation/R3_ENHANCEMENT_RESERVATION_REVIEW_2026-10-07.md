# Coordinator boundary review: enhancement prices - 2026-10-07

Published reservation `c97e97458` is approved for the narrow pricing extraction.
Implementation and qualification remain pending; approval is not code completion
or permission to support active enhancement outcomes. This is a delivery checkpoint
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
