# Collector domain separation handoff — 2026-10-07

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

## Proposed first bundle — not primary-accepted

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
