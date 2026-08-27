# Security and Compliance Review

- Commands contain fixed-width numeric identities and a hard 12-item bound. Owner types,
  reasons, states, nonzero IDs, positive vnums, ordered unique UIDs, complete parent
  reachability, and terminal destruction rules are validated before database work.
- Player owners use canonical player coordinator keys. Other owner identities are typed;
  context-bearing identities use SHA-256-derived fence keys without display names or
  serialized object content becoming authority.
- Repository data values use prepared statements. Owner rows and root-range items lock in
  canonical order, and every optimistic update is revision guarded.
- Current state, item and owner revisions, immutable ledger rows, exact inbox result, and
  outbox event share one InnoDB transaction. Stable semantic rejection writes no ledger
  or outbox mutation.
- The allocator reserves ranges under a singleton `FOR UPDATE` transaction, rejects
  overflow, is mutex-protected in process, and fails closed on exhaustion. Ambiguous
  commit wastes at most a range and cannot reuse one.
- Migration changes are additive and re-runnable. Baseline and reconciliation scripts
  enforce local/development environment and database-name guards; ambiguous legacy data
  is retained as bounded evidence, never resolved by timestamp.
- The synthetic adapter is pointer-free and carries no character, object, descriptor,
  room, or callback pointer across the worker boundary.
- `make security-check` passed. No credentials, private keys, production operations,
  external dependencies, data deletion, or privacy exports were introduced.

Result: pass; no unresolved security or privacy findings.
