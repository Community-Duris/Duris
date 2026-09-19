# Issue #505: stale coin-transfer investigation

## Conclusion

No causal source/destination publication defect was proven from the available source,
SQL fixture, or prior incident evidence. The change in this branch is therefore a
preventive diagnostic, not a claim that it recreates or fixes the historical failure.
It records a bounded revision-gate stage on future `coin_transfer` ESTALE receipts
without recording operation IDs, entity IDs, amounts, or command payloads.

The historical acceptance criterion cannot honestly close from this evidence alone:
the prior incident record has no failed command payload or binary provenance, and no
currency loss was established. A parent-mediated, read-only journal lookup is listed
below; if the journal was checkpointed, that lookup may also be inconclusive.

## Publication trace

The relevant path has three authority boundaries. Admission prepares an immutable
command; persistence is the only place that mutates durable wallet/item state; live
objects are published only from a committed completion.

### Wallet endpoint preparation

`src/economy/currency_transaction.c`:

- `currency_transaction_coin_wallet()` snapshots the live wallet denominations,
  computes the requested before/after vector, and builds an `account_bank` command
  with the character wallet and account-bank revisions read from the live object.
- `currency_transaction_submit_coin()` builds the enclosing `coin_transfer`
  command, rejects fenced keys, and queues the two endpoints. It does not publish
  either endpoint.
- `currency_transaction_submit_wallet_value()` is a separate one-endpoint path for
  rewards, refunds, NPC credit, and ordinary wallet spends. It is not a two-endpoint
  coin-transfer publication and is not classified by the new coin stages.

`src/economy/currency_command.c` keeps the wallet and bank revision fences in the
command's expected-revision vector. `execute_currency_state()` locks the player and
bank rows, reads their current revisions, rejects a non-rebasable mismatch with
`ESTALE`, and only then updates both rows and writes the ledger. The result carries
current wallet/bank revisions and is used to verify the requested after-vector.

### Two-endpoint physical coin paths

All physical coin paths that use the two-endpoint transaction are covered by the
same source-then-destination loop in
`critical_command_repository_apply()`:

| Gameplay path | Source endpoint | Destination endpoint | Publication after commit |
| --- | --- | --- | --- |
| PC-to-PC `give` (`submit_coin_give`) | account-bank wallet debit | account-bank wallet credit | `coin_give_completion()` |
| `put` (`submit_coin_put`) | account-bank wallet debit | item-transfer pile update/creation | `coin_put_custody_completion()` / `publish_coin_put()` |
| `get` (`submit_coin_get`) | item-transfer pile update/destruction | account-bank wallet credit | `coin_get_completion()` |
| death/cleanup wallet conversion (`money_to_inventory()` in `src/world/handler.c`) | account-bank wallet conversion | item-transfer creation owned by the player | `money_inventory_completion()` |

The `submit_coin_debit()` dispatcher also has a one-wallet fallback for a physical
`drop` and for giving to a non-player character. Its committed completion calls
`publish_coin_drop()` or `begin_coin_give_credit()` and is deliberately outside the
`coin_transfer` source/destination stage classifier. It has one wallet revision
fence, not the two-endpoint publication contract investigated here.

`prepare_coin_pile()` captures the pile's before/after denominations, owner
revisions, item revision, target-parent revision, and bounded custody snapshot.
`money_to_inventory()` builds the equivalent system-to-player creation payload.
Neither helper mutates the live pile at admission. The item repository publishes
runtime ownership only after the enclosing database transaction commits.

### Durable apply order and rollback

For a `coin_transfer`, the repository:

1. starts the database transaction and inserts the inbox row;
2. processes the source endpoint, then the destination endpoint;
3. locks and validates wallet/bank, owner, item, parent, and coin-payload fences;
4. applies child mutations and ledger/item events only after the corresponding
   fences pass;
5. on a terminal `ESTALE`, rolls back the savepoint so an earlier source mutation
   is not retained, finishes the inbox with the error and bounded stage, and commits
   the receipt with an empty result payload; and
6. on replay, returns the stored command-hash-matched receipt without reapplying
   the mutation.

A failure stage is an aggregate bit mask. The stable names identify source versus
destination and the refused class: wallet revision, bank revision, owner revision,
item revision, target-parent revision, coin-payload revision, destination rebase,
or unknown. If more than one comparison was stale, the mask retains that fact and
`critical_failure_stage_name()` reports `multiple_revision_gates`. Invalid values
are rejected when the receipt is read.

The item executor reports only local gate classes through
`item_transfer_failure_stage`; the coin repository maps those flags to the source
or destination stage. The persisted `failure_stage` column is `SMALLINT UNSIGNED
NOT NULL DEFAULT 0`. Existing rows remain stage `none`; there is no retroactive
claim about an old failure.

## Regression and verification

The maintained real-SQL harness is
`tests/async/currency_transaction_mysql_harness.cpp`, with
`tests/async/run_currency_transaction_schema_mysql.sh` applying the additive
critical-command migration and schema verifier before compiling the harness.
The harness uses a fresh schema in the task-owned MySQL container; it does not use
the parent #504/#507 database.

The regression covers two distinct stale gates:

- a destination coin-payload mismatch returns terminal `ESTALE`, persists
  `coin_destination_coin_payload_revision`, leaves the wallet and pile unchanged,
  leaves `result_payload` empty, and replays with the same stage/error;
- a source wallet revision mismatch returns terminal `ESTALE`, persists
  `coin_source_wallet_revision`, leaves the wallet/ledger unchanged, leaves
  `result_payload` empty, and replays with the same stage/error.

The same harness also retains the existing SQL fault/rollback and interrupted
transaction probe. A race where the pause finishes before `KILL CONNECTION` is
now treated as an assertion outcome rather than an unrelated fixture SQL abort.

Executed checks on this branch:

- `python3 tests/async/test_coin_command_transaction_contract.py` — passed.
- `python3 tests/async/test_currency_transaction_contract.py` — 10 tests passed.
- `python3 tests/async/test_critical_transaction_contract.py` — passed.
- `python3 tests/async/test_boot_schema_preflight.py` — passed.
- `python3 tests/async/test_auction_ownership_publication.py` — passed.
- `make -C src -j2` — passed.
- Targeted C++ harness compile with `-std=c++20 -Wall -Wextra -Wpedantic -Werror` — passed.
- Real-SQL harness on a fresh schema in the task-owned MySQL 8.0.46 container —
  passed, including schema verification and stale rollback/replay assertions.
- `./scripts/format.sh --check` — passed after formatting touched C/C++ files.
- `./tests/async/run_currency_transaction_schema_mysql.sh` — passed on the
  disposable MariaDB wrapper, including schema migration, coin SQL matrix, and
  player-load companion harness.

## Historical evidence and limits

The prior local #505 result records ESTALE observations during the cited incident
window, no auction activity in the examined period, and no established currency
loss. It also records that the available dump did not contain the failed command
payload or binary provenance. The local issue corpus does not contain an independent
#505 issue payload. Those facts support instrumentation and prevention, but do not
identify whether the historical refusal was a source wallet fence, destination
wallet fence, item owner fence, item revision, parent revision, payload mismatch,
or an upstream admission/publication problem.

The new stage is only populated when this code path executes after the migration;
it cannot reconstruct a pre-migration failure. It also cannot distinguish a failure
that happened before `critical_command_repository_apply()` (for example, admission
fencing or command construction rejection).

## Parent-mediated read-only retrieval that could reduce uncertainty

Do not query production from this worktree. If the parent wants one final read-only
check, use the parent-held clone and redact values before sharing any result:

1. Select only `critical_operation_inbox` rows where `command_type = 17`
   (`coin_transfer`) and `result_code = 116` (`ESTALE`) in the incident time
   window. Retain only timestamps, command/key hash presence, and row counts; do
   not expose operation IDs, payloads, account names, or amounts.
2. Resolve `CRITICAL_COMMAND_JOURNAL_DIR` from the parent-held production `.env`
   and inspect the owner-only file
   `$CRITICAL_COMMAND_JOURNAL_DIR/critical-command.journal` read-only. Filter
   frames by the selected operation IDs and verify the frame command hash against
   the inbox row using the existing journal decoder. Report only an aggregate
   endpoint/gate classification. The journal is append/checkpointed state, so an
   absent frame is not proof that no command existed.
3. If no matching frame remains, the missing artifact is the failed command's
   pre-apply payload or a structured stage emitted at the time of failure. The
   current snapshot and inbox schema cannot supply that historical attribution.

No production access, write, restitution-path change, or private fixture data was
used by this branch.
