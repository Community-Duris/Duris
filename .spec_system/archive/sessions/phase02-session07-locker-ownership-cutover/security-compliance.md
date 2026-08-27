# Security and Compliance

- Locker and chest identity is numeric and typed; display names, temporary room IDs, and
  synthetic wrapper object IDs are not trusted as durable custody authority.
- Private-chest password verification and access control remain on their existing
  hardened path. Movement commands, outbox rows, and diagnostics contain no password or
  private item payload.
- Failed or stale ownership commands preserve prior custody and do not trigger terminal
  teardown. Copyover aborts if accepted critical or locker work cannot drain.
- The migration wrapper refuses non-local/non-development environments and database
  names. The normalization is additive/re-runnable and does not delete locker items.
- Snapshot SQL remains compatibility state only and does not write current-owner,
  owner-revision, ownership-ledger, or ownership-baseline authority.
