# R3 enhancement price preparation reservation — 2026-10-07

Continuing queue operation F3-price; proposed for coordinator boundary review,
implementation has not begun. This is one delivery checkpoint in the continuing
project charter, not a new overall completion boundary.

## Candidate and preimages

Producer candidate `275df7f626e12cb396a22da34317a4e7f355e9a1`, charter
`b1ac97c3a9a1d1e50db6657b71862fb65681ce41`, owned branch before continuation
`ee5c8a056c15323849290efc962a1a02e75c4037`. The current `src/item/enhance.c`
and its existing payment regressions are raw-byte unchanged between the original
accounting base and the new producer candidate. This reservation touches no
new native quest/held-retirement/auction/save/coordinator path. Unknown private
primary overlap remains unknown; optional import adds no wait or release gate.

Source SHA-256: `dc50a3fc114c29d63cba855f1e3c790420724274911693a01bf971717aa60f12`. Exact complete function preimages:

- `void enhance(` SHA-256 `00e9089adeea023f57739db010c235cff0db09a574a9d2b43c7dbf285b2a3e90`.
- `static bool perform_superior_enhancement(` SHA-256 `82a248c68f83ee51c47490316e4052b89c3e7b6e633e8d42a715cd3222d82784`.
- `void modenhance(` SHA-256 `cba76997db05e56031b1a6c3f84c4a9e5130bc27cca54b251d093287480ba864`.

## Outcome, files and minimal boundary

Move price selection and numeric validation from the three existing enhancement
producers to `src/economy/enhancement_price.h`, with owned native int item/config
facts and existing int cost outputs. Ordinary enhancement uses source item value,
configured low threshold and low/high prices; negative selected price refuses and
zero remains a free enhancement. Superior enhancement uses the existing int64
`base + item_value * per_value` expression, refuses below zero or above INT_MAX,
and retains zero cost. Essence enhancement uses its separate fixed material-item
value tiers: <=20 costs1000, <=30 costs20000, otherwise100000 copper. Negative
material values keep the existing low tier; no new eligibility policy is added.

Use small named functions for these actual policies, without a generic pricing
strategy/configuration class. Any common range narrowing is shared once. Owned
quote evaluation does not inspect wallets, items, SQL, world tables or RNG.
Wrappers capture exactly the currently used value/config facts at the same point;
only successful checked quotes assign output. Existing native int inputs bound
superior multiplication/addition within int64, including signed extremes.

Production edits: only header include and price regions in `enhance`,
`perform_superior_enhancement`, and `modenhance` in `src/item/enhance.c`.
Keep ordinary price rejection/money messages and their order unchanged. Essence
money insufficiency remains before essence location/prototype probing; select its
same literal1/20/100 platinum message from the returned fee rather than repeating
material-value price rules in the live adapter. No money or item mutation moves.

Extend existing `test_ordinary_enhancement_payment.py`,
`test_superior_enhancement_payment.py`, `test_essence_enhancement_payment.py` to
include and execute the actual header alongside their extracted production
functions. Preserve original numeric/payment/effect/probe/cleanup assertions,
sanitizers, no-PIE flags and30-second deadlines. Add direct owned boundary/sentinel
controls and fixed tier-message checks, including exact thresholds, zero/negative
configured fees and native int extremes. Existing
`test_crafting_enhancement_regressions.py` must follow relocated essence tier
constants and actual caller, while preserving its unrelated assertions.
No new runner/test infrastructure is proposed.

Ablation: do not extract RNG, enhancement effects, source selection, modifier caps,
superior material plan or outcome search in this delivery. Their capture and
compound authority need separate current-owner facts. Pricing can move without
those changes. Do not introduce an unused abstract plan or reimplement current
currency application/admission. This header has three real producer callers.

## Invariants and proof

The enhancement command's active-accounting refusal remains untouched. This
separates retained inactive pricing; it does not create a supported active compound
outcome or promise authority. No effect/UID/custody/native lifetime, debit ordering,
refund, payout, material consumption, retained pouch conservation, description,
receipt/ACK/publication, migration, save, configuration format or spelling changes.

Before code edits require coordinator boundary review of this published proposal.
Continue independent candidate reconciliation while that review is pending.
Run each existing payment harness on retained original and extracted production
functions with the same assertions/controls. Execute direct owned rules; run
adjacent enhancement config/material/stat/module contracts and actual active
refusal checks where applicable. Build both maintained backends from the owned
source, pin exactly what was built, and keep current-candidate import applicability
separate. No old ELF is evidence for the753-provider producer candidate. Actual
native payment/outcome journeys are not implied by fixtures or build success;
record unexecuted scope and any owner dependency explicitly.

After this bundle, rerank the evolving nine-domain queue against latest published
primary requirements. Frozen outcome/compound effect coupling and other feasible
quote/validation seams remain candidates, never a pretext for declaring the
continuing Goal complete at R3.
