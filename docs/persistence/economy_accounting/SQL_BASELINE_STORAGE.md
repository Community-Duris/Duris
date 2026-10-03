# SQL baseline retention contract

Migration 0032 adds three InnoDB stores for complete baseline witnesses and
per-epoch opening reservations. It creates no epochs, mappings, game balances,
item identities, coverage records or activation state. The private SQL batch transaction owner now writes these stores atomically;
its production lifecycle caller and native-source authorization remain pending. This storage milestone
does not qualify #479 or activate any gameplay writer.

## Durable records

- `economic_baseline_control`: one row per retained lineage/epoch, its canonical
  40-byte opening account, initialization inbox operation, unsigned book revision
  and nullable last inbox operation. Initialization is revision zero with no last
  operation; a nonzero revision requires a last operation. The lifecycle owner
  takes this row exclusively for initialization/batch progression and never
  resets it because native authority or a current active epoch changed.
- `economic_baseline_witness`: one row per common accounting operation, unique
  lineage/epoch/book revision, complete EAB1 bytes and their SHA-256 digest, version
  and collection counts. SQL enforces version 1, the EAB1 magic, counts at most
  3,071 holdings and 6,000 items, and exact size `192 + 112*holdings + 88*items`
  (at most 872,144 bytes). Empty batches remain 192-byte witnesses with receipts.
- `economic_baseline_reservation`: primary key `(lineage,epoch,identity_kind,
  identity_id)` with nonzero unsigned identity. Kind 1 is an ordinary account
  lifetime across all account kinds/contexts; kind 2 is an item UID. The namespaces
  are separate. Its composite foreign key requires the owning witness in that
  same lineage and epoch. A new preparation ID or changed account kind cannot
  bypass an existing lifetime reservation. A new epoch can open that identity once.

All foreign keys use RESTRICT. No player/name foreign key cascades through these
records. Book revisions and witness revisions are accounting progress, not native
save revisions. The designated opening account itself is not an ordinary holding
reservation. There is no second item custody ledger or mutable economic total.

## Implemented private transaction owner

`economic_sql_baseline_transaction` is private to the future
`economic_sql_accounting_lifecycle_transaction`; only test builds add test access.
It has no production caller or coordinator admission. `apply` and `reconcile`
require a reconnect-disabled READ COMMITTED connection without an existing
transaction. They own START/COMMIT/rollback and verify the same session before
returning. A missing or ambiguous original receipt stays retryable under that ID.

Initialization borrows an already active transaction and requires retained
lineage/epoch membership, the designated opening account and an existing lifecycle
inbox operation. It refuses a duplicate control row. The caller must roll back
on any initialization error and retain its lifecycle receipt; missing control
after an incomplete restore is not proof of never-initialized state.

Batch execution reserves the original inbox first. A duplicate uses its original
command hash, complete stored witness and bound plan before consulting current
preparation inputs. It ignores a replacement preparation supplied with an exact
already-committed command. New batches regenerate the canonical command/plan,
require a completed successful initialization receipt, lock the retained
lineage/epoch and control, reject exhausted book revisions,
and atomically append common accounting evidence, normalized effects/postings,
the baseline source claim, full witness, identity reservations and control/inbox
completion. Empty batches also advance the book and retain a receipt.

Expected normalized rows and reservations are generated only from the canonical
prepared witness. Inserts and readback use at most 64 rows per batch and bounded
128 KiB row/predicate fragments. Counts scoped by operation/epoch reject missing
or additional records without reading an entire epoch's reservation catalog.
The final verification compares complete canonical intent/plan/witness bytes,
digests, source claim, normalized rows, reservation membership, book linkage and
completed inbox. Native currency/item ledgers, child commands and outbox entries
must be absent for the baseline operation. No native writes or publication occur.

Original-ID reconciliation regenerates the bound plan from retained EAB1 bytes
and verifies the original completion, even after other batches or epochs advance.
It checks retained epoch membership rather than the current active epoch or
native balances. Source fingerprints remain evidence bytes, not authentication.
An unsuccessful COMMIT reply returns `ambiguous_commit`; a fresh connection
reconciles the same original ID. A rollback attempt is made on every other exit;
an unusable connection must be discarded by its caller. Allocation failure is
retryable, including command-encoder overflow caused by allocation failure.

The future lifecycle caller still must authenticate the operator, establish and
retain the quiesced maintenance/consistent revision boundary, resolve mappings, verify
complete source and custody coverage, and enforce restore/activation policy.
This private persistence API does not confer those capabilities. Aggregate epoch
budgets and end-to-end activation qualification also remain pending.

Baseline/witness/reservation records need protected backup/restore and narrowly
scoped database grants. Lifecycle metadata transitions may update the control;
witnesses and reservations are append-only. Inherited broad schema grants must
not defeat those boundaries. Existing controller and disclosure decisions remain
pending; no administrative privilege or purge policy is invented by this change.

## Registration and validation

The current bootstrap includes the additive DDL; immutable 0001-0031 bytes and the
sealed 170-table baseline remain unchanged. New migration 0032 follows this
linked stack's 0031 accounting storage head. Canonical master remains at 0030;
recheck allocation before merge. The lifecycle manifest protects all three
retained stores and runtime inventory increases from 212 to 215 tables.

Both shell and native production boot metadata probes include the new tables,
unsigned column types, indexes, foreign keys, CHECK definitions and MySQL CHECK
enforcement. MariaDB session enforcement remains required. The dedicated immutable
verifier is self-contained; the current Python verifier shares the metadata reader
without changing the existing nine-table accounting fingerprint contract.

`tests/async/test_economic_baseline_schema_mysql.py` requires explicit disposable
loopback configuration. Its SQL fixtures test storage constraints, not complete
native witnesses. It exercises rollback, empty batches, maximum combined witness
size, unsigned identities/revisions, per-epoch uniqueness, cross-batch duplicates,
separate item/account namespaces, wrong-epoch/orphan references, RESTRICT deletion,
and two-session concurrent initialization. Corrupted unsignedness, indexes and
CHECK/enforcement fail dedicated verification and the verbatim production native
metadata function. Restoring exact metadata restores validation.

The existing `run_economic_accounting_schema_mysql.sh` disposable wrapper includes
these tests. Fresh bootstrap, upgrade from 0031, repeat migration-runner execution
and both full server builds are reported with this increment when verified. None of
these schema checks proves the pending native baseline/activation rehearsal.

## Native transaction verification

`tests/async/run_economic_sql_baseline_transaction_mysql.py` compiles the actual
owner with ASan/UBSan and a synthetic disposable SQL harness, plus a client-free
build which refuses every entry with ENOTSUP. The harness exercises exact retries,
changed command identity, overlapping reservations, different epochs, empty and
maximum combined witnesses, native-state preservation, corrupt evidence, session
ownership and isolation/reconnect guards. Independent concurrent connections test
same-ID retries, disjoint batches and overlapping lifetime reservations. MariaDB
can choose a pre-commit deadlock victim (1213); the harness retries that specific
retryable outcome on fresh connections, allowing at most three total attempts
with the original ID,
then requires the same exact-once outcome and revision assertions.

Link wrappers inject failures before queries, hide successful write replies
(including COMMIT), and fail successive C++ allocations during apply and replay.
The tests require rollback of uncommitted receipt/evidence state and fresh
original-ID reconciliation after lost COMMIT acknowledgement. These are private
transaction tests; they do not prove the unimplemented gameplay cutover.
