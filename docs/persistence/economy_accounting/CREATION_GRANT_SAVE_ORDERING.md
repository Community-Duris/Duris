# Creation grant and player save ordering (#664)

Implementation branch: `codex/creation-grant-save-ordering`, based on
`experimental-accounting` at `903af10c97e717a5a6b05c4599fcc5796faed63c`.
Qualification date: 2026-10-01.

## Ordering rule

Creation admission and snapshot capture run on the game thread. Before a player
creation grant starts its ownership command, the recipient must have no retained,
appending, queued or in-flight sealed save. A worker completion still waiting for
its game-thread acknowledgement counts as pending. Quarantine, terminal and
offline save/login fences, stopped admission and revision overflow also block it.
The existing bounded grant queue retains transient requests for later admission.

Once a grant is admitted, checkpoint capture for its recipient waits until the
grant has published into the live inventory/container. Dirty component marks stay
intact during this wait. The next checkpoint therefore captures the published
graph rather than an earlier projection missing the newly admitted UID.

Dirty but unsealed changes do **not** block grant admission. Deferred grants
waiting for an older save do **not** block capture. Those distinctions prevent a
cycle in which each queue waits for the other. The fence is recipient-specific;
room grants do not change a player graph. Existing creation batches have one
recipient and use the same admission/publication rule.

This guard is intentionally limited to the existing creation-grant owner.
Other item publication owners can themselves require a receipt-bearing save;
blocking every pending movement would deadlock that publication protocol.

## Reproduce and qualify

Build both maintained backends, using separate output names for the journeys:

```sh
make -C src -j8 PERSISTENCE_BACKEND=mariadb DMS_BINARY="$PWD/bin/server/creation-save-mariadb"
make -C src -j8 PERSISTENCE_BACKEND=flatfile DMS_BINARY="$PWD/bin/server/creation-save-flatfile"
```

Run each focused suite directly with `python3 tests/async/<name>`:

- `test_creation_grant_reconciliation.py`
- `test_creation_grant_batch_submission.py`
- `test_player_save_pipeline.py`
- `test_player_save_worker.py`
- `test_player_save_journal.py`
- `test_item_movement_prompt_runtime.py`
- `test_item_movement_input_queue.py`
- `test_corpse_creation_batch.py`
- `test_player_item_custody_write_guard.py`
- `test_publication_ack_checkpoint.py`
- `test_player_quarantine_fail_closed.py`
- `test_player_quarantined_dispatcher.py`
- `test_player_live_failure_quarantine.py`
- `test_durable_quest_offering.py`
- `test_spell_component_retirement_publication.py`

The executable grant regression controls both save-first and grant-first
boundaries with production admission predicates and real revision state. It
covers inbound recipients, queued grants permitting old saves to drain, retained
append/worker results, dirty progress, login/terminal/quarantine fences and genuine
movement conflicts. The checkpoint regression verifies that deferred capture
does not consume dirty state or lose pending spell/quest receipts.

The prompt/input/corpse fixtures compile the production movement code with
sanitizers. Their dependency mocks include the save-admission predicate and the
current spell-effect publication wait query.

Run the actual server journeys:

```sh
python3 tests/async/run_alchemist_crafting_journey.py bin/server/creation-save-flatfile file --creation-save-only
python3 tests/async/run_alchemist_crafting_journey.py bin/server/creation-save-mariadb redis --creation-save-only
```

For SQL, provide `TEST_DB_HOST=127.0.0.1`, `TEST_DB_USER` and
`TEST_DB_PASSWORD` for a **disposable** MySQL 8.0.46 service and a disposable Redis
service on loopback ports 3306/6379. The fixture creates, migrates and removes its
own `alchemist_journey_*` database. Never point it at a production service.
The flat-file journey uses only its own temporary state directory.

Each journey alternates save-first and grant-first commands over eight rounds.
It awaits creation responses without the crafting setup's checkpoint barriers,
checks exactly 16 new unique item UIDs, then verifies the entire UID/VNUM set
through copyover, disconnect and cold reload. The surrounding fixture also
qualifies three prior copyovers, NPC identity retention and a stolen vial. Logs
must contain neither `active_custody_absent_from_snapshot` nor a terminal save
failure. This mode does not execute active-accounting crafting.

## Recorded results

- Both maintained backend builds passed with the repository warning policy.
- All 15 focused suites listed above passed.
- The final alternating-order flat-file and MySQL 8.0.46/Redis journeys passed;
  each retained all 16 new UIDs through four total copyovers, disconnect and cold
  reload, with no terminal save or omitted-custody diagnosis.
- `scripts/format.sh --check --rev origin/experimental-accounting` passed for
  changed lines and complete touched C/C++ files; `git diff --check` passed.

## Recovery limits

The fix prevents new creation grants from superseding a still-sealed player
projection. It does not unquarantine or discard old journal evidence. Existing
terminal failure, exhausted retry, exact operation receipt, rollback and login
checks remain enforced. Synthetic negative tests verify that failed/corrupt
evidence remains fenced across restart and that healthy players can proceed.

The completed player journeys establish cold reload after acknowledged overlap
and copyover; they do not inject a process crash at every grant/save boundary.
Automatic recovery of a previously quarantined superseded projection requires
durable proof identifying the exact grant, UID/revision and replacement snapshot.
That broader #664 acceptance item and #490's integrated crash/fault matrix remain
open. Neither deleting quarantined files nor exempting all error10001 results is
a recovery procedure.

No schema, wire format, operation identity or production data is changed. The
accounting branch still needs integration review before merge or release.
