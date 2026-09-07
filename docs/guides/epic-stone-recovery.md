# Epic stone reward recovery (#166)

Stone touches now submit a version-2 zone transaction containing the captured
participant amounts, including task and bonus modifiers. The SQL transaction
commits every participant epic ledger, the zone outcome, the stone claim, and
outbox receipts together. A terminal failure rolls back the entire reward.

The game keeps a pending reservation for the stone and ordinary zone. Rejection
leaves the stone and tasks unchanged; uncertain outcomes retain that reservation
for coordinator recovery. Success publishes the balances before applying touch,
errand, boon and level effects. Disconnected participants receive pending effects
when they reconnect during the same process; newer hydrated balances are retained.
The 64-operation admission limit counts only uncommitted transactions. Completed
receipts remain in memory until their offline recipients return; they do not block
new touches. This retained backlog can grow during the process lifetime. Artifact
and guild dispatch also occurs on reconnect, and level eligibility uses the
captured stone level even after the object is removed.
Humming and touch feedback both honor zone eligibility and pending work.

`epic_stone_claim` uses the globally allocated object UID, rather than a zone/boot
counter. Re-touching a surviving claimed stone returns its original receipt and
consumes the stone without another award. A newly loaded stone has a new UID.
This also protects against a lost callback after the journal checkpoint. New
recovery requests receive their own outbox receipt so reconciliation stays clean.

## Compatibility and rollout

- Apply immutable migration `0012_epic_stone_claim` through the standard migration
  runner before starting this binary. Fresh bootstrap includes the same table.
  Boot compatibility is synchronized to migration 0012 and 178 tables, with
  fingerprints measured on MySQL 8.4 and MariaDB 10.11.
- Claims are protected replay records retained alongside the operation inbox
  through season resets. Never delete claims or recycle object UIDs independently.
- Version-1 zone commands and old 88-byte receipts remain metadata-only. New
  receipts are 512 bytes. Do not roll back to a binary that cannot read version-2
  journal commands/receipts or migration 0012; retain a compatible recovery binary.
- Flatfile mode has no atomic zone repository. Stone reward admission now reports
  unavailable and leaves rewards and effects untouched in that mode.
- This does not reconstruct awards lost by older code. The reported player
  incident still needs its original gameplay and deployment evidence.

## Verification and limits

`test_epic_stone_transaction.py` tests codecs and, when explicitly enabled against
a disposable fixture, solo/group awards, metadata-only v1 compatibility, operation
replay, same-UID recovery, and rollback after participant, outcome, outbox and inbox
failures. `test_epic_stone_runtime.py` exercises production transaction orchestration
with controlled admission, terminal/ambiguous completions, disconnects and recovery.

Durable epic credit and duplicate prevention do not depend on a live player or
object. Notifications and secondary gameplay effects are process-local and are
not replayed from a claim after a server crash; replaying them could duplicate
level purchases or unrelated progression. Full process-kill/copyover and live-game
reproduction have not been performed. SQL rollback and replay are tested with
synthetic data, not production data.
