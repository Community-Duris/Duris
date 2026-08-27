# Code Review

## Findings Repaired

1. **Critical - combat completion initially assigned epic and currency fields outside
   their authoritative transaction modules.** Combat publication now delegates to
   `epic_transaction_publish_balance` and `currency_transaction_publish_balances`; the
   bounded result includes the exact committed bank vector needed for same-account live
   publication.
2. **High - the original coordinator limits could not represent all normalized locks or
   the complete maximum-participant result.** Entity capacity is now 32 and completion
   capacity is 2048 bytes, with compile-time maximum-size assertions and codec tests.
3. **High - decoded combat payloads could otherwise claim a different lock set.** Decode
   rebuilds and compares all player/account keys and every expected frag/epic/wallet/bank
   revision before dispatch.
4. **High - legacy player saves still owned frag columns.** Generic snapshots now leave
   `frags` and `oldfrags` untouched; combat commands exclusively advance those fields and
   `frag_revision`.
5. **High - a combat outcome needed foreign-key-valid epic/currency ledger operations
   without exposing a partial child transaction.** Deterministic child IDs are derived
   from the stable parent ID and their inbox, ledgers, revisions, parent result, and
   outbox all commit in the same transaction.
6. **Medium - live combat publication needed to survive disconnect and group mutation.**
   The pending map is keyed by stable operation/participant identities, retains offline
   results, and never re-reads group membership to redirect effects.
7. **Medium - compatibility audit capture included unbounded mutable equipment/log
   sources.** Those fields are deliberately empty; room and description snapshots are
   bounded in the typed codec.

No unresolved blocking finding remains. The recent-death query used by heaven-time
calculation is the documented Phase 03 read-side exception.
