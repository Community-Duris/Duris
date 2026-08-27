# Code Review

## Findings Repaired

1. **Critical - auction results exceeded the coordinator completion buffer.** The focused
   MySQL harness exposed truncation after a committed bid. The bounded completion frame
   is now 512 bytes and the 320-byte auction result has a maximum-nine-item codec test.
2. **High - the initial auction builder validated before coordinator acceptance time was
   assigned.** It now normalizes keys without requiring the later timestamp, matching
   the established command builders.
3. **High - decoded payloads did not prove their declared fence set.** Decode now rebuilds
   and compares every player, account, auction, item, and expected-revision key.
4. **High - WebSocket/offline delivery initially remained in live callbacks.** External
   publication now flows through the durable outbox worker, a bounded cross-thread queue,
   and game-thread delivery; live callbacks only publish exact local ACK state.
5. **High - generic critical MySQL harness cleanup deleted unrelated domain rows.** The
   harnesses now scope cleanup and counts to command type 1, preserving durable item and
   auction history in shared development databases.
6. **Medium - closing fee and bid extension were hard-coded in repository replay.** Both
   are captured in the typed command and used deterministically.
7. **Medium - a no-buy-price listing was rejected and a first bid equal to the starting
   price was disallowed.** Both legacy-compatible behaviors are restored and covered by
   the expiry and bid harness paths.
8. **Medium - finalization did not independently enforce expiry.** Finalize now rejects a
   still-live auction with `EAGAIN`; buy-now remains the explicit early-close path.
9. **Low - auction wallet ACK publication duplicated the centralized currency mutation
   boundary.** Both command families now use `currency_transaction_publish_balances`.

No unresolved blocking finding remains. Legacy implementations remain unreachable behind
the cut-over public routes for later Session 12 deletion; authoritative gameplay no longer
calls their synchronous mutation paths.
