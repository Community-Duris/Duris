# R8 ordinary enhancement cascade reservation — 2026-10-07

Proposed next substantial search-control boundary; production/test edits await
coordinator review. Owned source c751ce3cac609527b453de2e6c1b6530f602fc41;
latest primary 6b96e9a08ff4cfe3a654417e76b7ca68514e274e includes coin-owner
fixture4f3ee6550 and final R7 review. Actual continuing Goal ACTIVE/no budget.
R8 here names a worker bundle, not completion of the original R1-R8 acceptance.

## Outcome and exact three-file boundary

New `src/economy/enhancement_original_search.h` owns the complete ordered cascade
control in ordinary enhance: exact-only step0, per-direction candidate arithmetic,
integer/cap bounds, failed-step accounting and first-success stopping. Supply the
existing captured int64 newval/maxsearch and a local synchronous observation
provider. No catalogue snapshot, game pointer storage, dynamic allocation,
std::function, generalized framework or new authority enters the header.

In `src/item/enhance.c`, retain all preconditions, wear/newval/maxsearch capture,
price, RNG calls/gain/messages and the entire post-search payment/publication/
cleanup sequence. Replace only the cascade loop and obsolete local loop variables
with the domain planner invocation. A local provider retains native linked-list
lookup/filter/read code and a reference to the original robj carrier. It supplies:

- `max_roll()` reads the original configured int at EACH outer condition.
- `down_first()` reads direction at EACH nonzero direction probe, not once per step.
- `value_limit()` supplies original int64(ival_cap)+max_roll at the existing
  per-probe bound point, after the candidate passes1..INT_MAX. No new atomic
  configuration snapshot, normalization or clamping is promised.
- `try_value(int)` traverses the original hash bucket and live next links in
  order, checking current entry value/wear, fresh OBJ_VNUM(source) and original
  read_object. Failed reads continue, possibly observing changed configuration,
  next entry or source VNUM. First successful read sets the same native robj and
  returns true. The header receives only success/failure, never an object/UID.

The provider retains captured native int wearflags; it must not refresh them
from source/material after a read. Newval and int64 maxsearch remain the locals
computed earlier, including original luck multiplication. Native catalogue/source/
created-object lifetime and read ownership stay native. The header uses native
int64 step/count/candidate arithmetic with newval's original native-int-sum input
range; it does not extend accepted input ranges or change error policies.

Searchcount increments once only after an unsuccessful WHOLE step, including
all-invalid probes. Strict >maxsearch remains:0/negative budgets still allow the
initial step when max_roll permits it. Step0 makes exactly one lookup. Success
returns before increment; negative roll permits none. Preserve both directions,
live changes during reads, hash collisions, same-VNUM skip, unreadable continuation
and first linked successful result. No repeated probe is deduplicated when a
direction change produces the same candidate twice.

Extend only existing `tests/async/test_ordinary_enhancement_payment.py`, retaining
the complete actual producer/predicate and every R3/R6 numeric/affect/payment/
pouch/refusal assertion. Use its actual header, traced hash/read endpoints and
faithful fixture OBJ_VNUM index to prove these sequences. No new maintained runner
or contract relocation is currently needed; stat/config/payment contracts retain
their source assertions outside the moved control. Verify this directly before
implementation. These are the exact three implementation files.

## Executed original feasibility and dependency evidence

Complete owned original enhance body SHA256
`8f478ed3ead8e94a5a4f3928a40c3eecc2cd41015fb4cbf29c6429046e49ef03`;
complete cascade loop
`02d8922647a1cec577cab4ca2479e379125c423a917715ea9aa2f6d8a669578f`;
existing ordinary regression
`a5a1e8f84a4e1dfc7c3f3d60ac82ca90e803b0d966e53b1f87738437da3c474e`.
Latest primary changes no ordinary producer/search code. Its full producer still
differs at optional R3 pricing; the search/variable/include patch preimages require
their own exact check rather than claiming full body identity.

Private ignored bin/tests/domain-r8-feasibility-20261007 contains the complete
original producer/predicate, all existing ordinary controls plus **23 added search
cases**, generated CPP/expected traces and source/proof pins. Original compiler
and runtime PASS with unchanged C++20 -Wall/-Wextra/-Werror, -O1/-g,
ASan/UBSan/no-recover, no-PIE and30-second budget. Earlier successful narrower
experiments remain separately retained. No maintained source/test edits occur.

The executed cases assert ordered hash probes/native reads and exact cash/debit/
publication/retirement results: exact once, both directions, strict0/1/-1 budgets,
all-invalid step budget, low/INT_MAX/cap+roll boundaries, collision/wear/same-VNUM
skips, unreadable continuation/first success, failed-read direction/roll/cap changes,
next-link and source-VNUM changes, captured wear/maxsearch despite later mutations,
negative roll and refused payment cleaning the selected output. The coordinator
also independently executed its17 original scenarios and the earlier exact worker
native-search CPP with original flags/deadline. These remain controlled component
observations, not genuine source/birth/publication or gameplay proof.

Production is intended to apply independently to bare latest primary: only the
unchanged search block/locals and a common include point before net/comm.h change.
Verify the actual final delta, do not infer independence from this intent.
The COMPLETE test package explicitly retains existing **R3 plus R6** headers and
ordinary-test context. Check exact latest primary+R3+R6 alone and separately check
production-only application; R4/R5/R7 must not silently become prerequisites.
No completed optional rule is rewritten to hide packaging dependencies.

## Non-goals, proof and ablation

RNG probabilities/gains, eligibility/level/material/pouch/wear capture, fee law,
catalogue construction/hash/lifetime, native object creation, debit, effects,
retirement, active refusal, receipts/publication/ACK/recovery and all other domain
owners remain unchanged. The header provides no native ownership/capability.
No new active paid outcome, supported backend route, activation or deployment.

After approval, run identical extended expectations against complete original
and extracted producers, preserving all original controls/flags/limits. Keep
native read/probe/RNG order and exact no-match/payment/refusal effects. Run adjacent
material/all-stat/stat/config/superior/essence/pool and actual active-refusal checks;
retain the unchanged original module reset-count failure. Run both maintained
SQL/flat builds, repository formatting, latest full-module checks and minimal
prerequisite component execution. Pin exact source/ELFs/logs/terminal and declare
old740 graphs separately from current754 overlays and unrun native journeys.

Ablation removes eager catalogue/config/source snapshots, reusable callback
frameworks, new public hooks, search deduplication/clamping, native owner changes,
fresh wear/budget recapture, payment/RNG relocation and extra test files. The
remaining dedicated provider preserves native observations while extracting the
whole search operation. Full continuing Plans1-5/applicable original R1-R8
qualification remains required after this or any later worker bundle.
