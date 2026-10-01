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

## Refreshed accounting integration qualification

Application source: `8ea0041efa4f58654fdf2188798cd69732bc4e54`, refreshed after
#667 merged at `0741a98394480673cbbb7967f85393e1ebd1d9bd`. The following
preparation/documentation update changes no application or test source.

- Maintained flat-file and MariaDB builds passed with GCC 13.3 using
  `make -C src -j8 PERSISTENCE_BACKEND=<backend>` and separate output binaries.
- All 15 focused suites listed above passed on the refreshed application source.
- The actual flat-file, MySQL 8.0.46/Redis and MariaDB 10.11.14/Redis overlap
  journeys passed: eight
  alternating-order rounds, 16 exact new UIDs and all prior custody preserved
  through four copyovers, disconnect and cold reload.
- Changed-line and complete touched-file formatting checks passed in WSL with
  the mapped Git directory; `git diff --check origin/experimental-accounting`
  passed. The initial unmapped WSL Git-directory invocation was not used as proof.
- Build image: `sha256:d7fd33b8205b89935b9e66cfa32bd37cf0975d3beadbd2a4b7c8a2bb64291f49`.
- Flat-file binary SHA-256:
  `708271fa4f1d8c08a1251fad2bddb1621a5825d9594edcdc07de1cf693fd4f2e`.
- MariaDB binary SHA-256:
  `14da100713fb16c7381a9f633d285fcc9d477548d686895b3ba7424fe113e504`.

The first MySQL 8.0.46 journey timed out waiting for its second setup copyover,
before the overlap rounds. Its logged save completions succeeded with no custody
diagnosis. The cause was not established from the retained output, so that
attempt is not a passing qualification. A fresh disposable MySQL rerun passed
the unchanged fixture, including all four copyovers, eight overlap rounds,
disconnect and cold reload, with no custody mismatch or terminal save failure.
Both the failed attempt and successful rerun logs remain under ignored `bin/`.

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

No schema, wire format, operation identity or production data is changed.

## Prepared next steps: historical recovery and release evidence

This is the implementation sequence prepared after the user authorized #667 and
#668 integration. It is a plan, not a delivered quarantine-release capability or
full enforcement qualification. Target the remaining work at
`experimental-accounting`; preserve the active-accounting craft and vial guards.

### 1. Classify a historical superseded projection (#664)

Start with a synthetic, retained pre-fix grant-after-capture reproduction. Reuse
the existing save worker/journal, creation-grant and quarantine fixtures. The
current ordinary SQL repository refuses a quarantined PID before beginning its
transaction; replaying that repository or scheduling a recapture cannot by
itself release the fence. The native restore qualifier validates and preserves
archives and PID policy; it does not resolve a custody disagreement.

Before implementing any recovery mutation, establish whether the existing
retained evidence can prove all of the following:

- The rejected snapshot's PID, revision, component set, encoded payload and
  archive integrity, plus every other retained frame for the same PID.
- The exact later committed creation operation, source, recipient, output UID,
  payload, owner revision and subsequent ownership history. A current owner row,
  diagnosis string or higher save revision alone is insufficient.
- A complete authoritative replacement graph, including container/equipment
  topology and all retained non-item component obligations. Reconstructing an
  omitted item must not discard a newer status, skill or effect update.
- Exact disposition of attached death, quest and spell obligations. The first
  recovery slice should exclude receipt-bearing/death frames unless their
  existing owners independently prove the exact required result.

Classify evidence as proven supersession or unresolved conflict without changing
authority. Missing payload/receipt, ambiguous ownership, a reused source, malformed
topology or a conflicting later transfer must remain fenced. If retained data
cannot prove supersession, document that case and its required evidence; do not
invent history or exempt error10001 results.

### 2. Implement one bounded, durable recovery transition (#664)

Only for the proven case, retain a recovery obligation that links the rejected
frame, committed grant and replacement projection before retiring any live
obligation. Preserve the original archive. Make each transition idempotent across
process interruption and distinguish archive retirement from live-save ACK.
Release only the affected PID after the replacement authority and all applicable
obligations are verified. A crash or I/O failure at any boundary must retain
either the old fence or a durable resumable obligation. Healthy PIDs must continue.

Inspect these existing owners before choosing the minimal implementation:

| Owner | Existing files |
| --- | --- |
| Grant admission/publication | `src/item/item_movement_transaction.{c,h}` |
| SQL custody and receipt proof | `src/player/player_snapshot_repository.{c,h}` |
| Worker result and exact ACK ordering | `src/player/player_save_worker.{c,h}` |
| Retained frames, archive and PID policy | `src/player/player_save_journal.{c,h}` |
| Capture, completion and login/save gates | `src/player/player_save_pipeline.{c,h}` |
| Flat-file authority and restore | Current flat-file snapshot/ownership owners and `scripts/build_restore_qualifier.py` |

Use one implementation owner for this shared boundary. Any proven need for a new
durable store or public interface requires a separate concrete design review;
do not allocate or rewrite a migration during preparation.

### 3. Qualify the transition and integrate the evidence (#490)

Run focused executable tests first, then disposable real-server journeys in
flat-file and SQL mode. SQL qualification must include supported MySQL 8.0 and
MariaDB 10.11. Record exact source/binary identity, commands, outcomes and retained
evidence; distinguish graceful reload/copyover from an injected process crash.

| Boundary or negative case | Required observation |
| --- | --- |
| Before grant admission; save sealed/queued/in flight | Existing #668 ordering holds; no new ownership is published early. |
| After grant commit, before publication | Restart publishes the same UID once, using the original operation/source. |
| After publication, before replacement save commit | Retained obligation survives; no premature save ACK or login reopening. |
| After replacement commit, before recovery acknowledgement | Replay verifies the exact result; no second grant or lost obligation. |
| During archive/policy/recovery-obligation persistence | Interrupted or failed writes keep admission fenced and original evidence intact. |
| Repeated recovery, disconnect, copyover and cold restart | Stable UID/revision sets, component preservation and no duplicate publication. |
| Missing/corrupt payload or receipt; conflicting topology/owner | Specific bounded refusal; no mutation, fence release or invented success. |
| Retry exhaustion, saturation and disk-full | Bounded retained/fenced outcome; healthy unrelated PIDs remain usable. |

Before claiming the broader combat qualification, refresh the existing
`test_attack_continuation.py` source contract at its current `pv_common` callback
boundary. Its previously reproduced baseline lookup failure is not a passing
attack suite. Keep the integrated writer/backend matrix, independent
reconciliation and predeclared performance/storage budgets as separate #490
release gates; the grant/save journeys alone do not satisfy them.

Preparation checks against refreshed application revision
`8ea0041efa4f58654fdf2188798cd69732bc4e54` reproduced two additional release
evidence gaps:

- `python scripts/validate_economy_accounting.py` exits 1 with
  `economic writer census drift; review new/changed sites`.
- `python scripts/validate_economy_accounting.py --release` exits 1 with
  `writer has no executable evidence`.

Refresh the semantic census and generated mapping against the actual current
routes, then attach current backend proof to each supported/refused route. Do
not mark coverage complete or relax the validator to make these checks pass.
These are source/evidence readiness findings, not a new gameplay failure or
full release audit.

Comparing the validator's current and recorded `(path, family, excerpt)` multisets
narrows the ordinary census drift to one replacement in `src/account/nanny.c`:
the `economic_submit` excerpt for
`item_creation_grant_submit_batch_to_player_before_entry` now wraps before its
arguments. The call still exists. Review that existing caller and refresh its
recorded excerpt/mapping; do not treat a line-wrap difference as a missing writer
implementation or regenerate unrelated classifications.

At that revision, 742 registry entries whose mapped disposition requires
executable proof have an empty evidence field. This is a metadata count, not
742 distinct broken gameplay routes. First link applicable existing executable
and backend journey evidence, verify its revision/meaning, and return actual
implementation gaps to their domain owners. Do not rebuild already delivered
features solely because their registry entry lacks a link.

### 4. Resume the guarded domain owners (#551 and #661)

- #551: bind poison mixing, Encrust and Harvester exchange to the native accounting
  root/source. Couple exact input retirement and frozen output admission, including
  intended failed/zero-output outcomes. Publish only from committed results. Reuse
  `src/classes/salchemist.c`, `src/classes/drannak.c`, the existing craft movement
  owner and focused #551 conservation/runtime-state fixtures. Preserve craft
  reason 34 and update the writer/reconciliation evidence.
- #661: establish a durable reset-generation/spawn source, persist both selected
  and no-vial decisions, and bind the selected VNUM 102 vial to its exact UID.
  Reuse `npc_alchemist.c` and current reset, admission and world-recovery owners.
  Prove source/UID dedupe and depleted-inventory recovery across copyover/restart
  under active accounting in both backends. Preserve authored loot and summon
  exclusions, the independent 10% supply roll, and the roughly 33% caster combat
  cadence. Runtime mobile IDs/once flags alone cannot establish durable issuance.

Each domain should be delivered in its own focused accounting PR with an active
epoch journey. Keep the existing guards until that owner's acceptance criteria
pass. Full release remains owned by #490; production activation and historical
instance restitution are separate operations.
