# SQL release and one-time ownership repair

Status: implementation and qualification in progress. This procedure is not
authorization to run a migration, repair, or wipe against production.

## Release boundary

SQL is the first production target. Keep accounting activation guarded until
the native save/custody and accounting proof gates below pass on the same binary
and schema. Flatfile parity is a later release gate, not evidence for SQL.

Record the Git commit, binary SHA-256, migration head, MySQL/MariaDB normalized
schema fingerprints, test fixture versions, and every protected evidence digest
in one release record. Do not mix results from different builds.

## Freeze and evidence

1. Stop listeners, scheduled jobs, worker replay, and every other database
   writer. Retain the player-save and critical-operation journals without
   replaying them. Record the freeze time and all pending operation IDs.
2. Verify two independent, restorable backup generations. Restore one to an
   isolated clone, then verify table counts, row counts, checksums, and the
   journals against the frozen source. Keep the other generation untouched for
   rollback. Give the clone a distinct database name and credentials.
3. Run the read-only item ownership audit and topology classifier against the
   clone. Capture a protected, owner-only evidence packet with exact payload
   bytes and child metadata, source table/row, UID, parent/root, current custody,
   owner revisions, baseline and ledger rows, prototype state, artifact/special
   status, backup occurrences, and relevant committed journal receipts. Include
   player, pet, corpse, locker, account locker, room, shopkeeper, and siege
   stores. Count null/zero UIDs separately. Record query snapshot coordinates
   and a SHA-256 for each evidence input. Routine reports contain counts only.
4. Reconcile pending journals before deciding ownership. A committed operation
   may be replayed only through its idempotent application path; an ambiguous
   operation stays held with its original evidence. Do not use a save projection
   or a description match as proof that an operation committed.

Preserve native journal quarantine archives and persistent PID policy files with
each recovery generation. Register every fenced player and archived operation as
a protected case, independently of current item-custody mismatch counts. A zero-byte
active journal does not prove that archived saves, death dispositions, or rewards
have been resolved. Restore qualification must use the matching native archive
validator and enforce the same fences on login, load, save, SQL, and copyover.
Keep quest obligations with ambiguous historical item/cash delivery evidence
held through the ownership reset; current inventory and balances cannot establish
payment. Their reviewed disposition must be linked to the original obligation
and UID/value evidence before a new accounting opening is accepted.

The current classifier covers player, pet, corpse, locker, account locker,
saved room, shopkeeper, and siege payloads, including null and zero UID rows.
Its protected v2 artifact retains every physical row sharing a duplicate UID;
the aggregate report counts those rows separately. A duplicate row is never
implicitly selected as the winning instance. It also compares a post-baseline
UID's current revision with its creation/transfer event count even when there
is no opening baseline row. A mismatch remains a review case, not an automatic
repair instruction.
The separate read-only custody-history classifier covers current-owner rows
whose lineage is missing or whose revision disagrees with opening plus retained
events, including custody-only items absent from saved payload tables. On a
quiesced clone, its owner-only artifact records one stable evidence ID per
anomalous UID and each retained death-custody source row once, with disposition
operation and payload digests. Exact and root-level death witnesses are distinct
review categories; neither proves a repair disposition. The artifact still needs
the frozen journal, full ledger and baseline rows, cross-backup payloads, and
reviewer decisions before it can become the complete one-time case register.
The protected cross-backup/journal evidence builder and instance-payload proof
are still open work. Do not run the reset from this incomplete evidence packet.

A 2026-09-30 rehearsal restored the captured staging generation on isolated
MariaDB 10.11.14 and retained a protected custody-history artifact tied to the
SQL dump checksum. The clone has 417 custody-history cases and 1,901 physical
topology findings over 73,490 payload rows. These categories overlap. The
append-only migration preserves all checked native rows, original receipts,
and custody evidence; copied quarantine/PID files and locker receipts also
pass native parsing without byte changes. This establishes schema/recovery-file
compatibility. It does not rank owner candidates, choose a disposition, prove
historical accounting, or complete the cross-backup case register. See
[staging preparation](STAGING_SQL_ROLLOUT_PREP.md) for the measured checkpoint.


## One-time case register

Assign each candidate a stable case ID derived from a private run ID, source
table, source row ID, and frozen payload digest. Preserve that ID through every
rehearsal and final disposition. The protected register records:

- frozen source and metadata hashes, UID, prototype, ancestry, and every owner
  candidate with its evidence source and capture time;
- the chosen action (`retain`, `assign`, `reconstruct_ordinary`, or `hold`), the
  evidence rule and reviewer, planned UID and owner revision, and exact SQL
  preconditions;
- actual transaction/operation ID, before/after hashes, validation result,
  accounting opening reference, and final player remediation state.

Preserve old baseline, ownership ledger, and accounting history. A reset is an
append-only correction and new opening boundary, never a table truncation or a
retroactive claim that historical postings existed. Any physical payload moved
out of gameplay remains in a protected hold store or frozen source backup and
is addressable by case ID. The player-visible state names a pending review
without exposing another player's identity.

## Evidence ranking and dispositions

Use one deterministic policy version for the whole frozen snapshot. A verified
committed critical command with exact UID/payload and final custody outranks a
verified backup with matching UID/payload and owner. Consistent current custody
and native source placement provide corroboration. A payload location or
description alone is insufficient. A later accepted operation supersedes an
earlier owner, while an uncommitted or conflicting operation cannot.

Auto-assign only when one owner wins under that policy, the exact payload and
complete ancestor chain are unique, the UID is not live elsewhere, and no
artifact, special, account-bound, or generated-state rule is violated. Ties,
missing identities, cross-owner children, cycles, duplicate live UIDs, or
conflicting committed evidence become holds with all candidates retained.
Never break a tie by row order, timestamp alone, or player preference.

Reconstruct from a prototype only for an ordinary item with a uniquely proved
UID, owner, vnum, and source event. Mark reconstructed fields and their loss of
original instance metadata in the case register. Artifact, special, coin,
account-bound, unique, and generated-state items require exact instance evidence
or a hold. A missing payload with only a custody row is not proof of a complete
reconstructable item.

## Clone rehearsal and cutover

Apply each approved complete item forest as one guarded transaction. Lock the
source payload, metadata, UID allocator, current owner, owner revision, parent,
quarantine, and case rows in stable order. Recheck frozen hashes and expected
revisions. Allocate a fresh UID only for a reviewed collision case, advance the
allocator beyond every retained UID, write the exact payload and metadata,
append a correction receipt, then change current custody and case status.
Reject on any row-count or hash mismatch. A retry with the same case ID must
return the prior result; changed intent must fail.

Rehearse the complete approved plan on a restored clone. Compare all unaffected
rows byte for byte. Check unique live UID, acyclic ancestry, payload/custody
agreement, owner revisions, allocator floor, quarantine holds, account-bound
rules, and player/pet login and save journeys. Restore the clone from backup
and repeat; the result and case digests must match. Keep held items protected
while allowing affected players to reopen through an explicit degraded/held
state, without silently deleting a payload on their next save.

After the clone report is reviewed, take a fresh production backup during a
full maintenance outage, verify it, and require the owner to authorize the
exact plan digest and target. Recheck source hashes before any production
write. On divergence, stop and restore the full verified backup. Do not
selectively copy old tables or disable foreign keys.

## Accounting opening

Repair native ownership first. Reconcile every live wallet, bank, coin pile,
auction escrow, pending claim, treasury, item UID, and held case at a single
quiesced boundary. Use a new accounting epoch and immutable opening witness
for exact observed holdings; label earlier origin **unknown historical
provenance**. Opening equity explains the new book's starting balances, not
past gameplay or asset creation. Held/disputed items remain in the case
register and out of spendable custody until disposition; they do not receive a
fabricated price or source event. Later restitution or correction uses a new
linked operation and preserves the original opening witness.

Activate accounting only after the release validator has executable evidence
for every supported SQL writer, unsupported routes refuse before native
mutation, independent native-to-ledger reconciliation is clean or lists
approved held exceptions, and restart/replay cannot duplicate an opening or
correction. `validate_economy_accounting.py --release` currently blocks because
writer executable evidence is incomplete.

## Release proof

- Migrate disposable MySQL 8 and MariaDB 10.11 clones through the exact head;
  measure and check in both normalized metadata fingerprints. A build with a
  stale runtime contract must fail boot.
- Run the maintained SQL build, focused save/custody/connection/accounting
  journeys, full regression and isolated DB suites, and a 200-client workload
  on the same candidate binary. Exercise lost reply, database interruption,
  disconnect, copyover, restart, death/corpse, transfer, and replay at the
  durable boundaries. Verify exact UID and denominations before and after.
- Demonstrate backup restore, rollback, pending-journal handling, case lookup,
  held-item player messaging, and independent reconciliation on the clone.
- Record zero unexplained loss, duplicate live UID, silent hold release,
  unbalanced accounting root, or route without executable proof. Any such
  finding blocks production reopening.
