# Security and Compliance

- Commands use a bounded binary codec, stable operation ID, normalized entity locks, and
  exact expected revisions; decoded payloads cannot omit or substitute declared fences.
- Participant names, account names, room names, and audit descriptions are length-bounded
  and SQL escaped. Equipment and private player-log contents never enter the command,
  failure logs, or unrestricted SQL payloads.
- Player and account rows are locked canonically before mutation. PvP audit, frag,
  epic/currency ledgers, current state, revisions, inbox result, and outbox share one
  InnoDB transaction.
- Durable outbox publication and consumer-side operation identity prevent pre-commit or
  duplicate Redis invalidation and external notification.
- Schema, baseline, reconciliation, and verification tools reject production/non-local
  environments before mutation.
- No credential, private key, log, player export, archive, generated world data, or local
  environment file was added or modified.
