# Collector domain separation handoff â€” 2026-10-07

## Goal and checkout evidence

Actual `create_goal` followed by `get_goal` returned `status: active` in chat
`01a11627-5fc2-7960-b5f1-f38e5183a822`. Objective: Implement and qualify the first
bounded domain-separation delivery for Community-Duris/Duris, preserving current
accounting behavior: select one stable domain, extract useful existing rules into
owned-state logic independent of SQL/live game pointers, integrate the production
caller, verify, and publish commits plus an integration handoff.

Separate managed checkout:
`C:\Users\alexa\.codex\worktrees\ac24\NewDuris Max`.
Initial clean detached HEAD and freshly fetched `origin/experimental-accounting`:
`7dbc472123a729f2fedc345e5309a586ba8a02d8`.
Verified no existing local owned branch and no checkout holding that name, then
created `codex/accounting-domain-separation` from this HEAD. All work uses this
checkout. Publication goes only to its same-named remote branch.

## Proposed first bundle â€” not primary-accepted

Selected domain: Collector of Antiquities. Selected operation: preparing the
selected collection item as an empty singleton, retaining its own weight and
literal properties while the existing command fences the complete source root.

Exact production boundary:

- New `src/economy/collector_collection_image.h`: owned `player_item_snapshot`
  plus explicit direct-child weights; validate and sum weights, calculate own
  weight, normalize singleton topology/equipment, encode through the existing
  snapshot codec, retain the existing collector blob bounds.
- `src/economy/collector_collection_preparation.c:capture_exact_blob` only:
  capture native objects and direct-child weights, invoke the separated rules.
  Both `collector_collection_prepare` and `collector_collection_live_matches`
  already use this helper; no second implementation of those rules remains.
- Extend `tests/async/collector_collection_preparation_harness.cpp` and its
  existing Python runner for owned inputs and real production codec evidence.
- This handoff is the only documentation change. No Makefile/manifest change is
  needed for the local header.

Research: collector purchase preparation and collector policy already use
owned snapshots, so repeating them would add no extraction. Auction/shop/coin
recovery/native birth/quest owners are actively changing in the published
primary handoffs and were excluded. Collection's preparation file last changed
at `4764ebf2e` (Collector of Antiquities lifecycle policy). The fetched Plan 5 tip
`45666a547` owns retained history/audit, and quest-prep tip `6db65f624` publishes
bartender attempt and native recipe fixes plus execution tooling. Their current
canonical handoffs were read, as were AGENTS.md, README.md, repository scopeguard
and plan-ablation skills, finish plan/fourth-agent contract, remaining requirements,
and primary shared producer progress. No AI_CONTEXT.md was present.

Overlap risk: the primary's collector publication/integration work may have
unpublished edits. This proposed boundary reserves nothing and requires no
primary acknowledgement. Compare the helper preimage before import; defer or
adapt the small local caller if it changed. No collector transaction/repository,
accounting adapter, audit, quest, shared codec or authority function is edited.

Semantics retained: direct children are released by the existing native owner;
the selected item's collected literal contains no children and has its own
weight. Preserve exact UID/VNUM, all literal fields, existing root/equipment
normalization, negative/overflow rejection, size bounds and failure mapping.
Time, randomness, listing/recipient/source identities, revisions and custody
claims are outside this calculation and remain untouched. Both admission and
publication compare the same bytes. The typed payload still carries the complete
source tree for native SQL proof and existing accounting adapters. An image is
not authority, effect proof, receipt or acknowledgement.

Verification planned: existing collection regression with real codec and owned
state controls, adjacent collector purchase/transaction/accounting regressions,
changed-line format check, maintained `make -C src` with unchanged flags, both
maintained backends if available. Component evidence will be distinguished from
actual database/server/player execution. No production DB use, migration,
activation, deployment or shared mutable outputs.

Import order: this research commit is advisory; the forthcoming implementation
bundle requires only a compatible accounting base containing the existing
collector helper/codec. Implementation, qualification and upstream integration
are currently pending. Further Collector rule extractions are future work and
outside this first Goal.

## Implemented bundle â€” qualification in progress

Implementation commit: `3d2b85b0688684721f8db559cb3ea35b1830a1cc`. Its
parent is research commit `3e9d03db25ea459662b0cd6df8e764f05dd5a0e1`; accounting
base remains `7dbc472123a729f2fedc345e5309a586ba8a02d8`. Only the four proposed
production/test paths changed. No primary adoption or upstream integration is
claimed.

`collector_collection_prepare_image` now owns the original negative/overflow
checks, own-weight subtraction, singleton root/equipment normalization and
bounded production-codec encoding. Inputs are an owned item snapshot and a span
over explicit captured child-weight values, with no SQL or live object access.
`capture_exact_blob` captures those values and moves the snapshot into that
preparer. Native capture, eligibility, topology, custody and revisions still live
in the original adapter/owner. The new weight vector can fail allocation; its
catch retains the existing false/invalid-topology refusal of image preparation.
No frozen IDs, timestamps, recipients or revisions are recalculated here.

Production path: `collector_maintenance.c:process_candidate` calls
`collector_collection_prepare`, which builds the existing typed payload with
the image and complete source-root fence. `collector_transaction.c` calls
`collector_collection_live_matches` before its existing ledger/publication
effects and `collector_collection_detach_live`. Both preparation and comparison
call `capture_exact_blob`, hence the same extracted rules determine the bytes.
Existing SQL repository/accounting owners consume the same payload; locked
evidence, native effects, accounting references and durable boundaries remain
their responsibility. No independent transaction or accounting implementation
was added.

Executed so far: collection and policy runners pass. The collection runner now
links the real snapshot codec (section collection excludes unrelated codec
entrypoints); its original assertions/deadline remain. Added controls protect
identity/properties, child-weight rejection and overflow, zero/max weight,
codec failure, Collector blob limit, deterministic repeated preparation and
unchanged native fixture state. The existing room/corpse/root/claim/exclusion/
stale-publication assertions remain.

Retained adjacent failures: unmodified purchase-preparation, transaction and
purchase-accounting runners fail linking missing `shop_trade_recovery_forest_*`
symbols from their shared item-command source. Private diagnostic addition of
the existing `shop_trade_recovery_manifest.c` makes purchase preparation pass;
transaction exposes further missing native publication/authority dependencies.
These shared runners/owners are unchanged. Their failures are not passes.

Fresh maintained SQL then flatfile builds are running in a task-owned container
with this checkout mounted read-only at `/workspace`, task-owned volume mounted
as its `bin/`, no network, and no inherited objects. Compiler/dependency image:
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`,
Ubuntu 24.04 / GCC 13.3.0. No schema/backend runtime or player journey has run.
Final exact results, build hashes and evidence will follow before Goal completion.
