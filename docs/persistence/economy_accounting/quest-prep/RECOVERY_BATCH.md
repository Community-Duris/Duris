# Integrated quest recovery batch

Prep source pin: `a5a1f4b196496d50a6f46afecfec03aba3e66190`.
Prior pin: `2c4e17f363ecff0f0d7eb3451ddb229abdd63d9a`; original base:
`17c033d69316b21da8598791fc95cae79baa8dc2`; research:
`55905eac1906cf59405764407f9d22497cccfff3`.
Schema manifest SHA256:
`1fddd009cc67bba940efba28c2afc3d16d65ea8a476410034cd77c90a0a9aea2`.
The owning primary must record its integrated candidate commit, exact binary
SHA256, complete migration list (including 0062 on this pin), genuine lifecycle
activation and original birth/world/context proof. This document prepares the
major batch; no native run is reported here.

## Working directory and prerequisites

Run from the integrated repository root on Linux/WSL. Prep development cwd is
`/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max`.
Import the owned bundles before building the integrated candidate. Set
`QUALIFIED_SQL_BINARY`, `QUALIFIED_FLAT_BINARY`, `SQL_BINARY_SHA256` and
`FLAT_BINARY_SHA256` to its existing binaries and actual recorded hashes.
Have gdb, python3, maintained Python dependencies, mysql client and disposable
loopback SQL authority available. Existing native setup must create the epoch,
reset birth and original receipts. Do not insert surrogate evidence.

SQL driver environment names: `TEST_DB_DISPOSABLE=1`,
`TEST_DB_HOST=127.0.0.1`, `TEST_DB_PORT`, `TEST_DB_USER`, `TEST_DB_PASSWORD`.
Use the owner's disposable credentials without printing or committing values.
No .env is loaded by the owned collector. Existing driver creates a unique
`quest_journey_test_<12hex>` schema, bootstraps/adopts/runs migrations and drops
it in finally. Evidence hooks must run BEFORE that cleanup.

Retain binary/source/schema/fixture provenance, command exit status, stdout,
actual receipt/custody/XP observations and original paired journal proof under
an external/private batch evidence directory; generated data is not committed.
The existing wrapper deletes its temporary run roots and schema. Stdout alone
cannot retain the required SQL/journal cuts.

## Ready maintained legacy calibration and real Kord cases

These are existing executable commands, with their existing scope. They do not
qualify active native GIVE or authentic reset birth in the relocated mini zone.
Verify each binary hash before running:

```bash
printf '%s  %s\n' "$SQL_BINARY_SHA256" "$QUALIFIED_SQL_BINARY" | sha256sum --check
printf '%s  %s\n' "$FLAT_BINARY_SHA256" "$QUALIFIED_FLAT_BINARY" | sha256sum --check
python3 tests/async/run_quest_reward_ack_crash.py --server "$QUALIFIED_FLAT_BINARY" --backend flatfile --quest-case synthetic --fault-phase offering
python3 tests/async/run_quest_reward_ack_crash.py --server "$QUALIFIED_FLAT_BINARY" --backend flatfile --quest-case synthetic --fault-phase xp-ack
python3 tests/async/run_quest_reward_ack_crash.py --server "$QUALIFIED_SQL_BINARY" --backend mariadb --quest-case QP06 --fault-phase offering
python3 tests/async/run_quest_reward_ack_crash.py --server "$QUALIFIED_SQL_BINARY" --backend mariadb --quest-case QP06 --fault-phase xp-ack --move-reward
python3 tests/async/run_quest_reward_ack_crash.py --server "$QUALIFIED_FLAT_BINARY" --backend flatfile --quest-case QP06 --fault-phase offering
python3 tests/async/run_quest_reward_ack_crash.py --server "$QUALIFIED_FLAT_BINARY" --backend flatfile --quest-case QP06 --fault-phase xp-ack --move-reward
```

Keep original known-loss calibration on the original historical binary using
`--backend flatfile --quest-case synthetic --confirm-loss`; do not change its
fixture, deadline, assertions or failure meaning to suit a fixed candidate.
The current gdb boundaries are `complete_quest_offering` and
`quest_reward_recovery_save_acknowledged if receipt_count > 0`, with original
120-second boot and 30-second crash waits. QP06 has ear29262/scalp29263/toe29264,
Kord29257, dagger29237, C3000, nominal E2500 but actual frozen capped XP.
Expected wrapper success is exit0, one original reward, stable money/XP through
cold recovery, and (with --move-reward) original UID in room custody3/state1 at
an advanced revision through second cold boot. This supported move is AFTER
recovered save. It does not cover move-before-lost-ACK or native pair retirement.

## Actual SQL cuts and executable state assertions

Once primary supplies the capture callback below, invoke this owned reader at
quiescent BEFORE, committed-publication, saved-ACK, later-move, first-cold and
second-cold cuts. The variables come from actual setup/operations; never invent
IDs. `QUEST_TEST_DATABASE` is the still-existing isolated schema, `PLAYER_PID`
its player, `LINEAGE`/`EPOCH` its active owner identities,
`ORIGINAL_NPC_INSTANCE`/`REPLACEMENT_NPC_INSTANCE` its native births,
`OFFERING_OPERATION` the original durable ID. Include both mobile IDs in every
cut to keep binding and replacement-stock scope identical.

```bash
python3 tests/async/quest_accounting_prep/capture_quest_cut.py --case QP06 --database "$QUEST_TEST_DATABASE" --pid "$PLAYER_PID" --mobile-instance "$ORIGINAL_NPC_INSTANCE" --operation "$OFFERING_OPERATION" --lineage "$LINEAGE" --epoch "$EPOCH" --server "$QUALIFIED_SQL_BINARY" --server-sha256 "$SQL_BINARY_SHA256" --output "$EVIDENCE_DIR/before.json"
python3 tests/async/quest_accounting_prep/capture_quest_cut.py --case QP06 --database "$QUEST_TEST_DATABASE" --pid "$PLAYER_PID" --mobile-instance "$ORIGINAL_NPC_INSTANCE" --operation "$OFFERING_OPERATION" --lineage "$LINEAGE" --epoch "$EPOCH" --server "$QUALIFIED_SQL_BINARY" --server-sha256 "$SQL_BINARY_SHA256" --output "$EVIDENCE_DIR/published.json"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP06 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/published.json" --check complete --selected "$EAR_UID" "$SCALP_UID" "$TOE_UID" --rewards "$DAGGER_UID" --spares "$SPARE_EAR_UID" --reward-vnum 29237 --expected-xp "$ORIGINAL_FROZEN_XP"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP06 --before "$EVIDENCE_DIR/published.json" --after "$EVIDENCE_DIR/saved-ack.json" --check ack --offering-operation "$OFFERING_OPERATION"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP06 --before "$EVIDENCE_DIR/saved-ack.json" --after "$EVIDENCE_DIR/later-move.json" --check later-move --rewards "$DAGGER_UID"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP06 --before "$EVIDENCE_DIR/later-move.json" --after "$EVIDENCE_DIR/second-cold.json" --check replay
```

The reader performs only SELECT in one repeatable-read read-only snapshot and
rolls back before writing an exclusively created private artifact. Its actual
SQL statements remain unexecuted until this batch. Oracle success proves its
captured row predicates; supplement with maintained native command/context,
world adoption, historical ACK and paired retirement validators. It is not the
Plan5 canonical reconciler or proof of journal authority.

For other static cases, use the same capture command with the actual case ID,
then --check complete with actual distinct selected/reward/spare UIDs and:

| Case | Required selected kinds | Reward selector / outputs | Exact net coins / XP |
|---|---|---|---|
| QP01 | 43703,43752,43753,44164 | 44192 / one item | 0 / 0 |
| QP02 | 19006 x1,x2,x3,x4 | 19007,19008,19009,19010 respectively | -1000,-2000,0,-10000 / 0 |
| QP03 | 16013,16014,16080 | 16075 / 16015 AND16075 | 0 / 0 |
| QP05 | 16019 x3 or16021 x2 | 16048 or16050 | 0 / 0 |

Before/after definite refusal or stale callback uses --check refused. A retry or
second cold boot uses --check replay; both must leave original state stable.
These checks do not distinguish started-unreturned from definitive refusal;
keep such a result visibly held and authenticate original native authority.
QP03 D-specific native stock/cash/image checks are the next owned addition.

For QP04/QP07 use actual full-world runtime producers and quoted fee. Once the
shared owner supplies genuine debit and exact linked refund, run:

```bash
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP04 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/refunded.json" --check refunded --quoted-fee "$ORIGINAL_QUOTED_FEE"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP04 --before "$EVIDENCE_DIR/refunded.json" --after "$EVIDENCE_DIR/second-cold.json" --check replay
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP07 --before "$EVIDENCE_DIR/task-b.json" --after "$EVIDENCE_DIR/stale-callback.json" --check refused
```

Task A's lost callback must not map/abandon replacement B or alter history/fees;
if the original debit is durable, exact refund or visible held original liability
is required. Level11 example quotes220/110/1331 are not assumptions about the
maintained level56 Chaos fixture. Freeze/record actual clock and policy for
abandonment. Existing full-world baseline command from this cwd:

```bash
DURIS_WORLD_QUEST_BACKEND=both DURIS_WORLD_QUEST_BINARY="$QUALIFIED_SQL_BINARY" python3 tests/async/run_world_quest_dual_backend.py
```

Its default giver16553/room16633 is Woodseer, not Quietus1709/1734. It cannot
qualify QP04/QP07 until actual giver/arrival, mortal level, fault and original
attempt controls are supplied. Prepare owned full-world roots with
`prepare_fixture.py --case QP04|QP07 --output <new-private-dir>` (run each
case separately); they link actual production areas and create no task/epoch.

## Minimal shared requests and reproductions

| Shared owner | Current behavior; required change and minimal interface | Requirement / reproducer |
|---|---|---|
| tests/async/run_quest_reward_ack_crash.py::run/run_sql | QP06 options now present; temporary SQL/run-root destroyed without capture. Add optional evidence callback at existing quiescent cuts, exposing actual schema/root/pid/original op/UIDs and original frozen XP before cleanup. No timeout changes. | R1–R8 durable recovery; QP06 commands above and owned cut reader. |
| Same driver active-native route; src/world/quest.c::quest_native_gameplay_owner::pulse | Single `give ear kord` waits for legacy offering text. Add selection of genuine integrated active-native setup and three exact real handovers, retain existing legacy calibration, observe current submit_consumption/publication/retire_completed reachable boundaries. | Original native source/custody and pair proof; QP06 --backend mariadb must reach actual owner before fault is enabled. |
| src/world/quest.c::quest_native_completion_owner::prepare_original | Coin goal refuses before availability, blocking no-fee backpack behind reversed gloves branch. Establish not-matched before unsupported guard when prerequisites absent; preserve genuine coupled fee authority guard. | QP02 --acceptance native selector returns30; command below. |
| src/specs/specs.world_quest.c::world_quest_payment_committed/world_quest_refund_payment; src/core/utility.c::ADD_MONEY | Injected active debit callback emits refund prose but generic credit refuses. Publish genuine debit/refund linked native authority or retain visible held liability; expose original debit/quote/attempt and actual result to capture. | QP04 --acceptance returns30. Hypothetical debit is diagnostic only. |
| Same payment context/callback; src/world/world_quest.c task actions | Context action/fee/giver lacks original attempt; callback mutates B. Carry authenticated attempt/context through original durable service owner; stale callback cannot change replacement task/history; once-only linked restitution or held liability. | QP07 --acceptance returns31. |
| src/item/item_movement_transaction.c::item_native_quest_preparation_owner::submit_consumption; src/world/quest.c::quest_native_frozen_continuation_owner::retire_completed | D path requires original whole-stock/cash lifetime authority and matching historical ACK/pair; absence of actor/frame is insufficient. Publish linked original destruction and authentic cold reconstruction/return evidence; callback exposes original and replacement birth IDs, original paired return. | QP03 retained incarnation component passes; D/native batch pending. No new generic birth registry requested. |

Small reproductions, independent of DB/server qualification:

```bash
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP02 --acceptance
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP04 --acceptance
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP07 --acceptance
python3 tests/async/quest_accounting_prep/test_recipient_retirement.py --acceptance
```

The first three intentionally fail on the pinned actual published owners. The
last passes the resolved observe_give generation predicate only. Private NQF2/
NFR1/NQR5 successors in shared handoffs are not executable published proof.
