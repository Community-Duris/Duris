# Implementation Notes

- 2026-08-27: Session opened from `4785a557` after Session 11 passed 195/195 and
  published.
- Active raw scalar producers are limited to login/logout audit plus obsolete epic-gain
  and zone-touch helpers. The latter two have no callers after Sessions 03 and 11.
- Item audit producers remain only in corpse save/restore compatibility paths; durable
  custody is already owned by the Session 05/06 item ledger and current-owner state.
- The large queue has no direct producer; it only accepts oversized item/scalar SQL
  strings. Boot/copyover still invoked raw fallback replay, which can execute scalar or
  large records as SQL and is the primary retirement target.
- Phase 01 player/world and Phase 02 critical journals are typed, bounded, checksummed,
  permission-restricted, and remain authoritative; they are not legacy raw fallbacks.
- Login/logout audit now uses a bounded player-keyed critical command and immutable
  `session_audit_outcome` row. The payload contains only PID, event kind, and timestamp.
- Raw item, scalar, and large workers no longer start during boot or stop during normal
  shutdown. Raw SQL execution fails closed, and fallback replay only quarantines the
  legacy file after emitting an operator alert.
- The composed local reconciliation gate reports zero mismatches across epic, currency,
  item ownership, combat, artifact/guild, boon, and zone domains. Five owner-revision
  rows created by earlier isolated auction harness runs were removed only after proving
  their auctions, current owners, and ledger histories did not exist.
- Focused codec/source tests, both MySQL transaction harnesses, format, warning-as-error
  build, security checks, all 197 repository regressions, and signal handlers pass.
