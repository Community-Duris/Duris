# Plan 5 integration handoff — 2026-10-04

Primary integrated `codex/accounting-plan5` through
`7a78bb065a7979b8dc8ad2ec49295629d6713906` in local merge `432db98be`.
Experimental branch through `f7d26eaa721cd3b675c0b0c65009a2535813f400`
was integrated in `a590fc662`; that incoming delta changes only game news.
No new combined-candidate tests ran. External reports and native input trees
remain branch-specific evidence, not qualification of the primary candidate.

Independent read-only specialist/architect source reviews found these remaining
gates for the Plan 5 owner. Counterexamples below are predictions from source,
**not executed REDs**; follow the agreed major-plan testing cadence.

1. **Stake retirement provenance.** In `stake_snapshot(True)`, change only
   `account_origins[1]["retired_by"]` to funding `OP`. `audit_accounts` verifies
   committed/same-lineage identity, zero aggregate balance and absent native
   holding, but not the stake's terminal effect/revision/reason. Kind11's mapping
   exemption removes that other cross-check. Bind retirement to exact finite-round
   terminal authority or emit a specific unresolved-retirement exception. Durable
   stake/round authority remains separately required; do not qualify it from the
   synthetic clean fixture.
2. **Retained price metadata conflict.** In `price_snapshot()`, change only
   `native.lineage_realized_prices[0]["reason"]` from21 to22, leaving selected root
   reason21/price3000. The overlapping selected/history comparison checks ID,
   epoch and price only, while the operator view exposes both reasons. Compare
   reason and outcome/result consistently along with price; prepare a focused
   case expecting an exception. The comparison gap predates the new view and
   is now part of its expanded conflict contract.
3. **Empty initialized baseline loss.** The independent qualifier discovers
   books from surviving baseline files or successful receipts. Complete loss
   before the first successful receipt is invisible; revision-zero initialization
   operation/opening remain self-asserted. Plan1 owns the native authority-bound
   per-epoch initialization marker and atomic initialization/retention contract.
   Plan5 owns independent marker enumeration/cross-checks and release evidence.
   Agree a narrow versioned record and explicit pre-marker compatibility policy
   before reader implementation. Missing legacy markers must not be silently
   interpreted as never initialized.

Audited ordering matches native numeric account ordering; S48 grammar and the
33 source-kind restrictions match native accounting policy. Operator coverage
and epoch-bearing retained prices are additive views. The three incoming flatfile
qualifier headers match the inspected native formats and fail closed on malformed
retained structures. Structural consistency does not attest original source
balances/custody: witness digests remain assertions until separately qualified.

Primary retains Plans1–4/shared contracts, registry/matrix and activation ownership.
Plan5 should own fixes to its audit/read-only qualifier files and report immutable
commits plus scope/BEFORE/AFTER/fault witnesses for regular local integration.
Preserve SELECT-only controls, both SQL engines, original budgets, populated
upgrade/retention and full release gates. The snapshot-exception13 and
save-death-conflict4 actual-native matrix adapters remain outstanding. No
activation, production operation or full release readiness follows from this merge.
