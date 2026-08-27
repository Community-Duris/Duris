# Code Review

## Findings Repaired

1. **High - missing-row locking could gap-lock concurrent entity operations.** Inbox
   handling now claims with a unique insert before reading the authoritative row, and
   same-key deadlocks are classified for stable-ID retry.
2. **High - a failed pool replacement could leave reconciliation using a closed
   connection.** Replacement ownership is now assigned even on failure; ambiguous work
   remains retryable when no replacement is available.
3. **High - new database threads lacked native client thread lifecycle.** Coordinator
   repository calls pair `mysql_thread_init/end`, and outbox startup synchronously fails
   closed if its worker cannot initialize the client library.
4. **Medium - delivery COMMIT connection loss was treated as an ordinary failure.** The
   worker now replaces the broken connection and looks up the stable outbox ID before
   deciding whether delivery remains pending.
5. **Medium - failed dispatcher queries returned unhealthy connections to the pool.**
   Connection-class errors now replace the affected slot for fetch, delivery,
   reconciliation, and typed repair paths.
6. **Medium - a persisted BLOB was bounded only by the aggregate batch budget.** Each
   record now also enforces the 65,535-byte application/schema payload limit.
7. **Low - reconciliation checked a row pointer after freeing its result.** Row presence
   is captured before cleanup and only the copied scalar values survive.
8. **Low - the prior lifecycle source contract assumed adjacent coordinator/player
   resume calls.** It now asserts coordinator, outbox, and player resume order.

No unresolved blocking findings remain.
