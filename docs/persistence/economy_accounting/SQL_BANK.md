# Typed SQL bank transaction increment

Status: direct repository integration qualified locally on `codex/474-phase4-sql-bank`; draft review and hosted qualification pending.
Based on SQL storage PR #602 plus independent full-boon-result prerequisite #601.
Partial delivery for #476/#477/#480; no gameplay or baseline activation.

## Scope and retained boundaries

This increment reuses the preserved currency preparation, typed bank adapter and
SQL component (`3bc96367e`, `a7db124eb`, `316fd1786`, bank-only rejection hardening
from `b24e4004f`). It includes ATM deposit/withdrawal at the direct SQL root
repository. Other schema-2 families, legacy nested mutation entrypoints, pooled
root admission, coordinator admission and flat-file execution remain closed.
No later coin-wallet, baseline, enrollment or gameplay producer code is imported.

`currency_prepare_mutation` checks holdings, arithmetic and the explicit revision
policy before producing immutable before/after state. The pure typed adapter
selects bank reasons and capabilities internally, binds the actual wallet/bank
lifetimes and produces exact denomination effects. A generic balanced plan or
caller-supplied reason does not confer authority.

The SQL component borrows the root connection; it never commits or retries.
Preparation verifies the exact pending root inbox, frozen intent and native
identity bindings, locks retained lifetimes and native balances, then retains
private prepared state. Both legacy currency execution and accounted bank apply
use the same prepared-state SQL writer. Successful apply is required before
finalization can append financial evidence.

## Root commit and replay order

For a new bank root: start transaction and insert the root inbox; prepare locked
bank effects; apply exact native balances/revisions and legacy ledger; stage
outbox; finalize and reread all accounting evidence; complete inbox; verify the
completed root; COMMIT. Every failure rolls back or retains ambiguity under the
original operation ID. Only the existing root owns the transaction boundary.
The current canonical failure-stage field must remain `none` for these bank
receipts in pending, completion and retained checks.

Duplicate-ID lookup precedes current policy/authority preparation. Identical
canonical bytes return the original result after checking retained evidence and
native bindings; changed bytes fail. Replay does not consult current balances,
active epoch or mapping activity. Rejected receipts are restricted to the
established revision, insufficient-funds and overflow outcomes; their exact
rejection must be reproducible from the retained before-state. Evidence or receipt
tampering remains unresolved rather than becoming a successful replay.

## Required verification

- Pure adapter: both build modes under ASan/UBSan, exact effects, unsupported
  mutations, revision/error policies, failure preservation and randomized cases.
- Native component and direct root on MySQL/MariaDB: apply/finalize misuse,
  exact evidence, rejected-result tampering, failure-stage tampering, rollback,
  original-ID replay, changed payload, and lost commit acknowledgement.
- Legacy currency native regression because the storage writer is shared.
- Client-free refusal, SQL entrypoint gates, touched C++ formatting and both
  full server builds. These checks do not establish coordinator/gameplay journeys.

## Remaining scope

Transport and selected-backend admission, baseline/cutover and publication still
need qualification before a real wallet/bank gameplay journey is complete.
Compound adapters/savepoint handling, append-only application access protections,
other holdings/writers/backends, reconciliation and the original #474 acceptance
criteria remain required. This increment neither closes #477 nor qualifies all
currency writers.