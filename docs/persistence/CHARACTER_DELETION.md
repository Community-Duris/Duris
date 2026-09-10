# Character deletion outcomes

The account-menu confirmation calls `delete_character_result()`. The legacy
`deleteCharacter()` API remains available and returns true only for `deleted`.
Neither API consumes the loaded character.

- `deleted`: the selected backend confirmed the deletion. Only then are the
  successful-deletion audit events and success message emitted.
- `refused`: validation or backend cleanup failed. SQL cleanup is rolled back
  before this result is returned; the character and account mapping remain
  available for a fresh load and retry. Flat-file failures retain the existing
  authority coordinator's journal/recovery contract. The message does not claim
  that every byte of state is unchanged.
- `reconciliation_required`: SQL rollback failed, or COMMIT was not acknowledged.
  COMMIT can have reached the server even if its reply was lost. This is not
  success, and a subsequent successful ROLLBACK is not proof that COMMIT failed.
  The menu reports that cleanup may have completed, refreshes the account, and
  asks the player to contact an immortal before retrying. Protected server logs
  contain the stable PID for operator reconciliation.

## Transaction and runtime boundaries

SQL soft deletion, artifact release, visitor locker access removal, the guild
projection without this member, personal locker deletion (when requested), ship
row deletion, and player-row deletion share one owned transaction. An existing
transaction is refused instead of committing another caller's work. The guild's
live member links and frag counters are restored immediately after staging its
SQL projection. A later cleanup failure therefore cannot repeat a guild penalty
or strand a committed soft-delete mapping ahead of a still-live player row.

After commit, runtime guild membership, account-list membership, ship state, and
the player revision cache are released. This path does not call `Guild::kick()`:
that routine changes departure penalties and writes the character. Account-list
removal also skips the ordinary account save, because membership was already
committed by the deletion backend.

The flat-file coordinator retains its existing atomic authority operation. Its
successful result uses the same runtime-only publication path, avoiding a second
ship deletion or account write after the authority transaction has completed.

Confirmation loads exclude items and pets. Success, failure, cancellation, and
replacement selection all detach both descriptor/character references and free
the temporary metadata character. The negotiated terminal type is preserved.
The account is refreshed after a deletion attempt; refresh failure is separately
reported without changing the deletion outcome.

## Verification

`python3 tests/async/test_account_character_delete_runtime.py` compiles and
executes the production menu, deletion coordinator, account-list removal, and
guild staging/publication bodies with injected backend failures under ASan and
UBSan. It covers soft-delete refusal, each later SQL cleanup stage, flat-file
failure and success, retry, repeated confirmation, cancellation, replacement
selection, uncertain commits, rollback failure, and refresh failure. It checks
messages, audit ordering, references, list membership, revision eviction timing,
and one-time guild/ship publication. Persistence adapters are test doubles;
these are runtime control-flow tests, not a live database or Telnet journey.

The existing flat-file character-deletion harness separately exercises the real
journal and repository coordinator.

## Protected operator follow-up for issue 200

The historical disposable identity was deliberately omitted from the public
issue. This checkout has no `.env` or protected evidence identifying that player.
No production database query or player-data mutation was performed for this fix.
The historical check remains pending and must not be inferred from a likely name.

An operator should recover the exact account/PID from the original protected
September 5 investigation, verify that the evidence identifies an authorized
disposable test character, and inspect its current player row, account mapping,
and relevant cleanup domains read-only. Keep those identifiers and query results
out of public issues and pull requests. If absent, record that result privately.
If present, obtain or confirm cleanup authorization for that exact identity and
reconcile its current state before deleting it. Legacy partial cleanup and
unacknowledged commits require this evidence-based review; the new transaction
boundary does not retroactively repair old partial deletions.
