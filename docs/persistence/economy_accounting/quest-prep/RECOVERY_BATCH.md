> Current 2026-10-07 implementation and execution supersedes prep-only status below.
> See [HANDOFF.md](HANDOFF.md) and [EXECUTED_JOURNEYS.md](EXECUTED_JOURNEYS.md).
> QP07 original-attempt effects and QP02 native availability/dispatch are fixed
> and component-qualified. Genuine legacy SQL capture and supported journeys
> now execute. Active native authority/refund/original retirement remain separate.

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
Set `INTEGRATED_SOURCE_COMMIT` to the actual source commit of those binaries;
the collector records that explicit owner pin separately from its prep base.
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
python3 tests/async/quest_accounting_prep/capture_quest_cut.py --case QP06 --database "$QUEST_TEST_DATABASE" --pid "$PLAYER_PID" --mobile-instance "$ORIGINAL_NPC_INSTANCE" --operation "$OFFERING_OPERATION" --lineage "$LINEAGE" --epoch "$EPOCH" --server "$QUALIFIED_SQL_BINARY" --server-sha256 "$SQL_BINARY_SHA256" --source-commit "$INTEGRATED_SOURCE_COMMIT" --output "$EVIDENCE_DIR/before.json"
python3 tests/async/quest_accounting_prep/capture_quest_cut.py --case QP06 --database "$QUEST_TEST_DATABASE" --pid "$PLAYER_PID" --mobile-instance "$ORIGINAL_NPC_INSTANCE" --operation "$OFFERING_OPERATION" --lineage "$LINEAGE" --epoch "$EPOCH" --server "$QUALIFIED_SQL_BINARY" --server-sha256 "$SQL_BINARY_SHA256" --source-commit "$INTEGRATED_SOURCE_COMMIT" --output "$EVIDENCE_DIR/published.json"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP06 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/published.json" --check complete --original-mobile "$ORIGINAL_NPC_INSTANCE" --selected "$EAR_UID" "$SCALP_UID" "$TOE_UID" --rewards "$DAGGER_UID" --spares "$SPARE_EAR_UID" --reward-vnum 29237 --expected-xp "$ORIGINAL_FROZEN_XP"
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

Before/after definite refusal or stale callback uses --check refused.
For native recipe refusal, start AFTER already-committed legitimate GIVE
handover; compare the refused branch attempt only. Earlier native handover
custody receipts remain real effects and must not be erased or classified as
a refused transfer. A whole successful journey may capture before handovers
and validate all their actual events plus final consumption/publication. A retry or
second cold boot uses --check replay; both must leave original state stable.
These checks do not distinguish started-unreturned from definitive refusal;
keep such a result visibly held and authenticate original native authority.
For active NPC-owned selections pass --original-mobile with the exact original
instance, never a replacement. The retired check requires a separate D-only cut
AFTER input consumption/reward publication and BEFORE original remaining-stock
retirement; the final cut must preserve all already-issued player rewards.
Capture both original and replacement instances on both sides. Actual unknown
v1 cash is refused. The original wallet key below comes from its authentic SQL
mapping lifetime, not mobile UID. Validate full literal stock/command/world
correlation using existing native providers separately.

```bash
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP03 --before "$EVIDENCE_DIR/pre-d.json" --after "$EVIDENCE_DIR/post-d.json" --check retired --original-mobile "$ORIGINAL_NPC_INSTANCE" --replacement-mobile "$REPLACEMENT_NPC_INSTANCE" --native-cash-account "$ORIGINAL_NATIVE_WALLET_KEY"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP03 --before "$EVIDENCE_DIR/held-first.json" --after "$EVIDENCE_DIR/held-second.json" --check held --offering-operation "$OFFERING_OPERATION"
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP03 --before "$EVIDENCE_DIR/post-d.json" --after "$EVIDENCE_DIR/second-cold.json" --check replay
```

Retired predicates require the original v2 image and immutable birth reference,
linked genuine birth/source receipts for both incarnations, original residual
UID tombstones and exact D events, zero original native cash with original
mapped effect/posting/revision, and unchanged replacement image/stock/cash.
Held verifies a stable original committed receipt and unacknowledged obligation;
it grants no authority to finish a callback. Native parent/child full preimages
and actual uncertainty disposition must also be retained by the shared owner.
An interruption BEFORE obligation creation needs the owner's original retained
carrier; this check deliberately refuses missing obligation instead of treating
absence as success. Historical ACK checks never infer paired retirement from
missing frames. The primary must authenticate exact original parent/child receipt
bytes and literal frozen terms in one trusted session, confirm rollback cleanup,
and observe actual transition_pair success before asserting retirement.

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
| tests/async/test_static_quest_reward_journey.py::quest_fixture/run; shared crash driver::prepare_quest_fixture/run | Static journey hardcodes synthetic Lapney/acorn; crash selector exposes only synthetic/QP06. Reuse owned fixtures and expose case/recipe/supply plus actual native setup/giver/root selection through maintained runner, retaining default calibration. New owned fixtures already select contracts without deleting competing Q blocks. | QP01/02/03/05 genuine source/custody/publication, fixtures and completion/refusal commands below. |
| Same driver active-native route; src/world/quest.c::quest_native_gameplay_owner::pulse | Single `give ear kord` waits for legacy offering text. Add selection of genuine integrated active-native setup and three exact real handovers, retain existing legacy calibration, observe current submit_consumption/publication/retire_completed reachable boundaries. | Original native source/custody and pair proof; QP06 --backend mariadb must reach actual owner before fault is enabled. |
| src/world/quest.c::quest_native_completion_owner::prepare_original | Coin goal refuses before availability, blocking no-fee backpack behind reversed gloves branch. Establish not-matched before unsupported guard when prerequisites absent; preserve genuine coupled fee authority guard. | QP02 --acceptance native selector returns30; command below. |
| src/specs/specs.world_quest.c::world_quest_payment_committed/world_quest_refund_payment; src/core/utility.c::ADD_MONEY | Injected active debit callback emits refund prose but generic credit refuses. Publish genuine debit/refund linked native authority or retain visible held liability; expose original debit/quote/attempt and actual result to capture. | QP04 --acceptance returns30. Hypothetical debit is diagnostic only. |
| Same payment context/callback; src/world/world_quest.c task actions | Context action/fee/giver lacks original attempt; callback mutates B. Carry authenticated attempt/context through original durable service owner; stale callback cannot change replacement task/history; once-only linked restitution or held liability. | QP07 --acceptance returns31. |
| src/item/item_movement_transaction.c::item_native_quest_publication_owner::native_publish / submit_consumption; src/world/quest.c::quest_native_frozen_continuation_owner::retire_completed | Current native_publish explicitly returns false for successful disappear=true. D requires original whole-stock/cash lifetime authority and matching historical ACK/pair; absence of actor/frame is insufficient. Publish linked original destruction and authentic cold reconstruction/return evidence; callback exposes original and replacement birth IDs, original paired return. | QP03 retained incarnation component passes; D/native batch pending. No new generic birth registry requested. |

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

## Fixture invocations and per-case original supply

Set `FIXTURE_DIR` to a fresh private directory (prepare refuses nonempty output).
These commands PREPARE only and never boot, seed native evidence or migrate SQL.
For native source acquisition, choose --layout world for each of the seven:

```bash
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP01 --layout world --output "$FIXTURE_DIR/QP01-world"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP02 --layout world --output "$FIXTURE_DIR/QP02-world"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP03 --layout world --output "$FIXTURE_DIR/QP03-world"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP04 --layout world --output "$FIXTURE_DIR/QP04-world"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP05 --layout world --output "$FIXTURE_DIR/QP05-world"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP06 --layout world --output "$FIXTURE_DIR/QP06-world"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP07 --layout world --output "$FIXTURE_DIR/QP07-world"
```

The linked production `areas/AREA` order and full boot/reset world remain actual.
Primary generates required world files using its existing make world policy and
qualifies the original SQL-first lifecycle/birth setup. No prototype substitution,
reset cap change, precreated UID/binding or synthetic O is made in this layout.
Do not invoke a --minimal mini-boot and classify it as full-world birth.
Original acquisition anchors, verified in SOURCE_FACTS.json and CASES.md:
QP01 worshiper43710 at43712/43836/43798 holds necklaces and altar44165@44333
contains Orb44164 cap1; QP02 livestock19000 cap6 supplies hide19006;
QP03 warrior16005@16081 supplies sword/armor, dark shaman16087@16161 totem16080;
QP05 brown stock cap6 and huge16021@16040 cap1; QP06 dwarf29279,
barbarian29278 and bandit29277@29290 supply ear/scalp/toe. Observe actual UIDs
and source admission, transfer real goods, capture resulting original NPC stock.
Quietus requires actual world_quest/shop binding at1709@1734.

For supplied-stock negative/success preparation, retain the default mini layout:

```bash
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP01 --reward-vnum 44192 --supply exact --output "$FIXTURE_DIR/QP01-exact"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP01 --reward-vnum 44192 --supply shortage --output "$FIXTURE_DIR/QP01-short"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP01 --reward-vnum 44192 --supply wrong-kind --output "$FIXTURE_DIR/QP01-wrong"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP02 --reward-vnum 19009 --supply exact --output "$FIXTURE_DIR/QP02-backpack"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP03 --reward-vnum 16075 --supply spares --output "$FIXTURE_DIR/QP03-spares"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP05 --reward-vnum 16048 --supply exact --output "$FIXTURE_DIR/QP05-jacket"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP05 --reward-vnum 16050 --supply shortage --output "$FIXTURE_DIR/QP05-coat-short"
python3 tests/async/quest_accounting_prep/prepare_fixture.py --case QP06 --reward-vnum 29237 --supply spares --output "$FIXTURE_DIR/QP06-spares"
```

Use --supply spares/exact/shortage with each selected reward in the static table;
wrong-kind is defined for QP01 and both QP05 recipes. This supplies thirty tested
static variations across nine original contracts, not thirty additional cases.
QP02 keeps all four competing Q blocks and runtime reverse order even when only
three hides are supplied. QP05 huge exact supplies TWO synthetic roots openly;
it is not ordinary fresh-world availability at production cap1. Each variant
records required and actual supplied counts, chosen contract, unchanged production
reset declarations and output hashes in quest-prep-provenance.json. Negative
supply should not match that contract; do not assert all recipe families refuse
(e.g. fewer hides can match a cheaper paid QP02 branch when genuinely supported).
Nonmatched native choose_branch advances; unsupported/refused blocks. Both
must lack partial effects until a genuine admitted supported branch applies.

For the primary's capture callback, case-native completion state examples:

```bash
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP01 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/published.json" --check complete --original-mobile "$ORIGINAL_NPC_INSTANCE" --selected "$SAPPHIRE_A_UID" "$SAPPHIRE_B_UID" "$SAPPHIRE_C_UID" "$ORB_UID" --rewards "$NECKLACE_UID" --spares "$SPARE_SAPPHIRE_UID" --reward-vnum 44192
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP02 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/published.json" --check complete --original-mobile "$ORIGINAL_NPC_INSTANCE" --selected "$HIDE1_UID" "$HIDE2_UID" "$HIDE3_UID" --rewards "$BACKPACK_UID" --reward-vnum 19009
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP03 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/published.json" --check complete --original-mobile "$ORIGINAL_NPC_INSTANCE" --selected "$SWORD_UID" "$ARMOR_UID" "$TOTEM_UID" --rewards "$SCALE_UID" "$GOLD_DAGGER_UID" --reward-vnum 16075
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP05 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/published.json" --check complete --original-mobile "$ORIGINAL_NPC_INSTANCE" --selected "$BROWN1_UID" "$BROWN2_UID" "$BROWN3_UID" --rewards "$JACKET_UID" --reward-vnum 16048
python3 tests/async/quest_accounting_prep/quest_cut_checks.py --case QP05 --before "$EVIDENCE_DIR/before.json" --after "$EVIDENCE_DIR/published.json" --check complete --original-mobile "$ORIGINAL_NPC_INSTANCE" --selected "$HUGE1_UID" "$HUGE2_UID" --rewards "$COAT_UID" --reward-vnum 16050
```

QP02 paid contracts use the corresponding 1/2/4 original hide UIDs and exact
reward19007/19008/19010, with native fee1000/2000/10000. No-fee cases permit no
extra offsetting money operations. All static selected NPC roots must belong to
the explicit original incarnation; supplying a replacement ID is a failed check.
Static row predicates allow no missing/extra reward UID or consumed spare.
Use --check refused for definite shortage/unsupported return and --check replay
for subsequent duplicate command/lost-reply/restart cuts on every case. Held
starts from an actual accepted receipt/obligation and tests stability only.
Native success additionally needs existing core/continuation validation.

The existing independent canonical validator can check raw SQL/plan correlation
on the same still-existing disposable schema; it does not replace native world,
paired journal proof or the Plan5 owner's qualification:

```bash
python3 scripts/economic_sql_canonical_audit.py --host 127.0.0.1 --port "$TEST_DB_PORT" --user "$TEST_DB_USER" --password-env TEST_DB_PASSWORD --database "$QUEST_TEST_DATABASE"
```

For actual parent/child retirement, expose the existing owner observation from
src/world/quest.c::quest_native_frozen_continuation_owner::retire_completed after
`critical_command_repository_verify_native_quest_in_transaction` validates both
original receipts, `quest_reward_obligation_repository_read_exact_in_transaction`
validates original literal terms/ACK/native item/cash/complete XP, and transaction
cleanup confirms the same session and idle rollback. Capture the original pair
before and the genuine `transition_pair(*terminal_parent,*current,nullptr)`
return after. No matching parent returns false; journal uncertainty retains both
full preimages. Add an optional observation to the shared driver, not a separate
prep implementation of the coordinator, codec, receipt query or journal.
Reproducer is the QP06 xp-ACK/later-move scenario plus missing-parent,
mismatched-child/receipt/continuation, unacknowledged XP and started-unreturned
controls already defined here. This owner cannot be qualified using the legacy
offering breakpoint alone. Move-before-lost-ACK requires the same original UID
ordinary move BEFORE original ACK/pair cleanup, then historical authority only;
current-player projection must not reissue/repossess the moved reward.
