# Session Specification

**Session ID**: `phase02-session03-epic-ledger-and-balance-transactions`
**Phase**: 02 - Transactional Gameplay Domains
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `31c3d7b4`

## Objectives

1. Add an operation-keyed immutable epic ledger, explicit opening baseline, balance
   revision, guarded migration, exact verifier, and read-only reconciliation.
2. Extend the critical destination with typed award/spend commands that atomically lock
   player balance, validate funds, update state/revision, insert ledger/result/outbox,
   and converge by the existing inbox operation ID.
3. Publish committed balance and all dependent gameplay effects only through an exact
   game-thread completion and stable player lookup.
4. Cut every audited award, spend, reset, refund, level, skill, ship, craft, bottle,
   administrator, and reward mutation to the typed boundary.
5. Remove epic balance from independent checkpoint authority while preserving load and
   new-character initialization behavior.

## Design Boundary

The simulation thread computes an immutable final delta and reason, allocates one
operation ID, and registers a bounded typed continuation before coordinator submission.
Workers receive PID, signed delta, reason/type metadata, and expected epic revision only;
they never receive a character, object, descriptor, room, guild, or callback pointer.
The database transaction is authoritative for insufficient funds and committed balance.

Completions return the committed balance/revision through the operation-keyed inbox
result. The game thread resolves the current character by PID, verifies the pending
operation and fence, publishes the balance, then executes the narrowly typed staged
effect. If the player is absent, durable state remains authoritative and the effect is
retained or safely revalidated according to its typed continuation contract.

Legacy `epic_gain` is preserved as historical evidence. Migration creates one explicit
baseline from current `player_data.epics`; it never invents operation IDs for old rows.

## Success Criteria

- [x] Opening balance plus committed deltas equals each materialized epic balance.
- [x] Identical/ambiguous replay produces one balance delta, ledger row, and outbox set.
- [x] Insufficient funds changes no state and publishes no staged gameplay success.
- [x] Awards and spends update live balance, bonus, eligibility, and messages only on ACK.
- [x] Every audited epic mutation route uses a stable operation ID and typed reason.
- [x] Player checkpoints cannot overwrite transactional epic balance.
- [x] Legacy evidence is preserved and discrepancies are reported without fabrication.
- [x] Focused, isolated schema, format, build, security, and full validation gates pass.
