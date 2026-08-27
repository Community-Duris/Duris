# Code Review

## Findings Repaired

1. **High - rejected spends initially emitted a success-shaped outbox event.** Semantic
   rejection now commits its exact inbox result without a ledger or outbox row;
   reconciliation requires outbox only for successful results, and the MySQL harness
   proves the rejection has no event.
2. **High - new players could start without an opening baseline.** Player insert now
   creates the baseline inside the same SQL transaction, and boot fails closed if any
   player lacks coverage.
3. **High - generic snapshots could overwrite a committed epic balance.** Capture omits
   epics, apply explicitly skips legacy epic rows, SQL updates preserve the column, and
   flat-file replay parses but discards the old balance.
4. **Medium - ship upgrade build time did not preserve the legacy upgrade/downgrade
   formulas.** The committed callback now uses the original directional formulas and
   rejects names that cannot fit the bounded continuation without truncation.
5. **Medium - copyover drain could discard completions before typed effects ran.** The
   coordinator now invokes a registered drain observer so epic completions publish on
   the game thread before lifecycle teardown.
6. **Medium - malformed result payloads silently dropped pending work.** They now execute
   the typed rejection callback, increment health, and remove the operation explicitly.
7. **Low - result decoding accepted the unrepresentable minimum signed delta.** Both
   command and result decoders reject it.
8. **Low - epic skill coin-refund staging lost its explicit safe fallback report.** The
   committed callback logs the failure, credits coins directly, and tells the player.

No unresolved blocking findings remain. Process-crash convergence of the authoritative
balance, ledger, inbox, and outbox is provided by the critical command journal and stable
operation ID; dependent live effects are executed only by exact game-thread completions
and retained across disconnect and lifecycle drain.
