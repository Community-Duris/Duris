# Security and Compliance Review

- Currency commands accept fixed-size typed fields only; they reject empty vectors,
  minimum signed deltas, malformed account identities, unsupported reasons, malformed
  keys/revisions, negative results, and application-bound overflow.
- Account identity is canonicalized without storing display authority in coordinator
  keys. Player and account-bank rows lock in canonical order and expected revisions gate
  stale callers.
- Runtime repository values use prepared statements. Operation IDs and payloads are not
  interpolated into SQL or emitted in user-visible messages.
- Funds/bounds checks, both balance vectors, both revisions, immutable ledger, exact
  inbox result, and success outbox insertion share one InnoDB transaction. Semantic
  rejection commits no ledger or outbox event.
- Duplicate identity uses canonical command/key hashes; conflicting reuse fails closed,
  and ambiguous commits reconcile by stable operation ID after connection repair.
- Schema changes are additive, guarded, re-runnable, indexed, foreign-keyed, and
  fail-closed at boot. Baseline and reconciliation scripts enforce local/development
  guards and do not fabricate historical operations.
- Pending contexts are fixed-capacity and pointer-free. Workers receive immutable typed
  values, never live character, descriptor, room, object, guild, or callback pointers.
- `make security-check` passed. No dependency, credential, private-key, production,
  privacy-export, wipe, or destructive operational change occurred.

Result: pass; no unresolved security or privacy findings.
