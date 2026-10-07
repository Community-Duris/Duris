# Executed quest batch — 2026-10-07

All paths below are private ignored evidence under
`bin/tests/quest-implementation-20261007/` in the prep worktree. Docker maps that
directory as `/evidence`; repository source is `/work`. Source binary pin is
`a19a67ad021de8e6bbfb31ca9ffea31e41cb6aa7` unless specified. Actual schema/source
and backend distinctions in HANDOFF.md apply. Fresh SQL schemas are individually
created/adopted/migrated by the original driver and dropped in its finally block;
SQL jobs run sequentially because the maintained migration lock is global.

## Terminal results before current compatibility integration

| Evidence directory | Actual result | Scope |
|---|---|---|
| sql-synthetic-offering | PASS, exit0 | Original maintained synthetic calibration; offering crash and two cold recoveries |
| flat-qp06-search-offering | PASS, exit0,137.352s | Genuine Kord secret ingredient search,29262/29263/29264 exact original roots,29237 reward,C3000,XP,ACK and two cold recoveries |
| flat-world-bartender | PASS, exit0,35.663s | Full unchanged production world, Woodseer creation/map/queued abandon+new quest/final abandon |
| sql-world-bartender | PASS, exit0,93.302s | Same actual Woodseer path plus saved SQL task readback; schema quest_journey_test_1cafbb1a4550 absent after cleanup |
| flat-qp02-refusal | PASS, exit0,67.955s | Three19006 hides, actual `give chothe goblin`, definite durable-safe refusal, original UIDs retained through cold load; no native handover claim |
| flat-qp05-white | PASS, exit0,67.436s | Actual `get bear` three times, `give bear darlene`;16048 reward UID820 preserved through cold load |
| sql-qp06-xpslot-offering | Maintained driver PASS; owned posthoc oracle FAIL, exit1,120.397s | BEFORE,crash,recovered,second-restart cuts exist. XP1→521→521, frozen200,slot2 mask4,ACK1. Empty legacy entitlement rows triggered incorrect expected0; owned defect corrected, not double-pay. Schema quest_journey_test_755e1f270a69 absent |
| flat-qp06-xp-move | FAIL retained | XP-ACK crash/recovery succeeds; actual later drop refuses asynchronously, original `You drop` deadline fails. Shared movement owner remains required |
| flat-qp01-exact | FAIL retained | Real secret Orb44164 FOUND special teleports player, so mini get expectation fails; production flags/special retained |
| flat-qp03-exact | FAIL retained | Real invisible recipient16006 cannot be seen by player; no original retirement native proof |

Corrected reader terminal result: `sql-qp06-corrected-xp` PASS, exit0,153.865s; 62 applied migration history rows, actual player Human race1/level1 and active well-rested2108, genuine v5 frozen200/slot2 mask4, effective520, exact original UID custody, C3000, ACK, two cold recoveries, row/BLOB/missing-player controls and native-authority refusal. Cleanup schema `quest_journey_test_bf7fe29c5f2d` count0.
Both first flat shortage/invisibility retries hit the maintained native-cache guard when the
owned C++ read adapter was formatted during inspector compilation. These setup
failures are retained (`flat-qp01-shortage-refusal`, `flat-qp03-invisible-refusal`); the source was frozen and
retried in a fresh evidence directory. No weakened cache check.

## Exact executed commands

Original SQL calibration inside owned runtime (credentials supplied from private
0600 `/tmp/quest-prep-password`; no environment file or secret committed):

```bash
cd /work
export TEST_DB_DISPOSABLE=1 TEST_DB_HOST=127.0.0.1 TEST_DB_PORT=33306 TEST_DB_USER=prep_runner
export TEST_DB_PASSWORD=$(cat /tmp/quest-prep-password)
python3 -B tests/async/run_quest_reward_ack_crash.py --server /evidence/integrated-sql/server/dms_new --backend mariadb --quest-case synthetic --fault-phase offering --evidence-dir /evidence/sql-synthetic-offering
```

Owned runner invocations use the same pins; variables expand to the literal
paths/hashes below. Each evidence directory must be new, and diagnostics remain
private. Original timeouts/defaults/calibration/backend assertions are retained.

```bash
SOURCE=a19a67ad021de8e6bbfb31ca9ffea31e41cb6aa7
SQL=/evidence/integrated-sql/server/dms_new
SQL_SHA=6e4962a1fd38118d4f538157f70407293de7ec6840a3b57493fae972caf7e81a
FLAT=/evidence/integrated-flat/server/dms_new
FLAT_SHA=aaf8f44b534df5d00a88b49d1c8990affd54505746f6ec485f46bf65842b5bb8
python3 -B tests/async/quest_accounting_prep/run_quest_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP06 --fault-phase offering --evidence-dir /evidence/flat-qp06-search-offering
python3 -B tests/async/quest_accounting_prep/run_quest_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP06 --fault-phase xp-ack --move-reward --evidence-dir /evidence/flat-qp06-xp-move
python3 -B tests/async/quest_accounting_prep/run_quest_execution.py --server "$SQL" --server-sha256 "$SQL_SHA" --source-commit "$SOURCE" --backend mariadb --case QP06 --fault-phase offering --evidence-dir /evidence/sql-qp06-xpslot-offering
python3 -B tests/async/quest_accounting_prep/run_quest_execution.py --server "$SQL" --server-sha256 "$SQL_SHA" --source-commit "$SOURCE" --backend mariadb --case QP06 --fault-phase offering --evidence-dir /evidence/sql-qp06-corrected-xp
python3 -B tests/async/quest_accounting_prep/run_world_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --evidence-dir /evidence/flat-world-bartender
python3 -B tests/async/quest_accounting_prep/run_world_execution.py --server "$SQL" --server-sha256 "$SQL_SHA" --source-commit "$SOURCE" --backend mariadb --evidence-dir /evidence/sql-world-bartender
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP02 --reward-vnum 19009 --expect-refused --evidence-dir /evidence/flat-qp02-refusal
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP05 --reward-vnum 16048 --evidence-dir /evidence/flat-qp05-white
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP01 --reward-vnum 44192 --supply shortage --expect-refused --evidence-dir /evidence/flat-qp01-shortage-refusal-frozen
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP03 --reward-vnum 16015 --expect-refused --evidence-dir /evidence/flat-qp03-invisible-refusal
```

## Focused checks actually run

```bash
python3 tests/async/quest_accounting_prep/test_bartender_settlement.py --case QP07 --acceptance
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP02 --acceptance
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case catalog --acceptance
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP01 --acceptance
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP05 --acceptance
python3 tests/async/quest_accounting_prep/test_native_selectors.py --case QP06 --acceptance
python3 tests/async/test_world_quest_item_completion.py
python3 tests/async/test_world_quest_failure_feedback.py
python3 tests/async/test_world_quest_target_retries.py
python3 -B tests/async/quest_accounting_prep/test_quest_cut_checks.py
python3 -B tests/async/quest_accounting_prep/test_production_terms.py --variants
```

All passed:16 corruption/refusal oracle tests; seven production source cases,
30 static supply variants/nine contracts/all seven world layouts including sorted
index and Kord runtime XP mask checks. The malformed/modeled controls are not
native journey passes. QP07 component31 and QP02 component30 failed on the original
source before the fixes. Maintained full `make -C src` and changed-line format
checks passed for both production bundles; SQL and flat complete build logs and
binary hashes are retained privately. Final integrated primary candidate must
still qualify authentic active accounting/native birth/recovery/restitution.


## Current source/component batch

Merged source `6db65f624836f150ed3dfe33508e1f1719145fdb`, latest accounting
`f04317d9d72aa5594448809baad6041936b09801`. Full SQL/flat maintained builds from `current-source.tar` both PASS (740 original providers each). Terminal current runtime results follow below. `current-components.log`
passes all seven source cases/30 variants, native selectors QP01/02/05/06,
285 catalog recipes, QP03 actual generation/root guard, QP07 actual settlement,
16 oracle methods and three existing world regressions. Separate
`current-qp04-refund-diagnostic.log` reproduces component30/exit1. Original commands
above use `python3 -B` and the same arguments inside `/current`. QP04 remains
hypothetical debit/refund observation, not evidence that active debit occurred.

`flat-qp01-shortage-refusal-frozen`: PASS exit0,87.380s, genuine three statues,
actual incomplete-offering response and unchanged original input UIDs through
cold load. `flat-qp03-invisible-refusal-frozen`: FAIL exit1,42.280s because owned
runner expected the wrong literal. Actual command `give sword dragon` returns
`No one by that name around here.` Corrected actual-response check follows on
the current integrated source; no disappearance/retirement authority inferred.

Read-only existing inspector of `flat-qp06-xp-move/state` (a19a67a) confirms
reward UID821, VNUM29237, exact player root821/parent0, XP521, C3000, player owner
revision8 and room revision7. Critical log reports error116/stale_authority_revision.
Private `authority-after-failure.json` retains the complete original inspection.
No owner-counter change or journal rewrite was performed; the precise failed
shared movement predicate is not inferred solely from the errno.


## Terminal current compatibility batch

Source6db65f624836f150ed3dfe33508e1f1719145fdb; accountingf04317d9d72aa5594448809baad6041936b09801.
Tool metadata/literal fix88fc052630932492ffff148b474f075f72423fb9;
optional actual successful fee transcript429acc4a695db65997e0f2dcfcb4727b3dc407a8.
SQL ELF0d6514c7222dca25cb30e9e5875036cf2088461ebb07731a66153be07683907b;
flat ELF8aedc856f2ff9cfee15ce694b16d28540daba1a5f41dedd40d59de91ad1b1ea5.
Original maintained commands actually executed:

```bash
make -C src -j2 BIN_ROOT=/evidence/current-sql
make -C src -j2 BIN_ROOT=/evidence/current-flat PERSISTENCE_BACKEND=flatfile
make world
```

Both full builds exit0,740 providers each; source archive SHA256
9f95168377ec1fe39bec1dad0ad972555e6a3625a6ed2721e58a750c8c375d85.
The existing strict development flags are in current-*-build.log; no overrides
weaken warning/sanitizer settings. Two CPU/3GiB per build; artifacts stay ignored.
The source Git commit names the production binary; later eight Python tool lines
are separately pinned and do not relabel an ELF.

| Evidence directory | Exit/result | Seconds | Exact scope |
|---|---|---|---|
| current-sql-qp06-xp-move | 0/PASS |152.032| Original XP-ACK crash, captured exact roots/reward820/C3000/XP200→520/mask4/ACK, later room22800 reward drop revision2 and second-cold exact custody/replay |
| current-flat-qp06-xp-move |1/RED|135.663| Original XP-ACK/recovery assertions pass; later drop errno116/stale_authority_revision. Existing inspector confirms821 root821/parent0/VNUM29237 still player-owned,XP521,wallet[0,0,0,3],player owner revision8/room7 |
| current-flat-QP01-refusal |0/PASS|51.784| Actual shortage response and original UIDs through cold load |
| current-flat-QP02-refusal |0/PASS|68.361| Actual legacy durable-safe refusal and original hide UIDs through cold load |
| current-flat-QP03-refusal |0/PASS|67.786| Actual invisible-recipient response and all original input UIDs through cold load |
| current-sql-qp05-white |0/PASS|113.173| Three original white-bear skins, actual give/save/cold,16048 reward UID819; captured exact custody/ledger/replay |
| current-sql-world |0/PASS|90.523| Full production world Woodseer creation/abandon/replacement/final abandon/SQL task readback/normal shutdown; mapless refusal |
| current-flat-world |0/PASS|35.597| Same actual command path/normal shutdown on flat backend; mapless refusal |

Both world transcripts quote creation1platinum+1gold+2silver=1120 copper and
abandon43platinum+4gold+6silver+4copper=43464 copper. These are actual legacy
quotes, not active-native debit/restitution proof. Earlier a19a67a world runs
qualified successful map issuance; current selections were mapless, so no map
fee was charged here. Source map fee is10*level=560 when actually offered.

Exact current journey invocations (inside the private network-none runtime):

```bash
cd /current
SOURCE=6db65f624836f150ed3dfe33508e1f1719145fdb
SQL=/evidence/current-sql/server/dms_new
SQL_SHA=0d6514c7222dca25cb30e9e5875036cf2088461ebb07731a66153be07683907b
FLAT=/evidence/current-flat/server/dms_new
FLAT_SHA=8aedc856f2ff9cfee15ce694b16d28540daba1a5f41dedd40d59de91ad1b1ea5
export TEST_DB_DISPOSABLE=1 TEST_DB_HOST=127.0.0.1 TEST_DB_PORT=33306 TEST_DB_USER=prep_runner
export TEST_DB_PASSWORD=$(cat /tmp/quest-prep-password)
python3 -B tests/async/quest_accounting_prep/run_quest_execution.py --server "$SQL" --server-sha256 "$SQL_SHA" --source-commit "$SOURCE" --backend mariadb --case QP06 --fault-phase xp-ack --move-reward --evidence-dir /evidence/current-sql-qp06-xp-move
python3 -B tests/async/quest_accounting_prep/run_quest_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP06 --fault-phase xp-ack --move-reward --evidence-dir /evidence/current-flat-qp06-xp-move
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP01 --reward-vnum 44192 --supply shortage --expect-refused --evidence-dir /evidence/current-flat-QP01-refusal
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP02 --reward-vnum 19009 --supply exact --expect-refused --evidence-dir /evidence/current-flat-QP02-refusal
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --case QP03 --reward-vnum 16015 --supply exact --expect-refused --evidence-dir /evidence/current-flat-QP03-refusal
python3 -B tests/async/quest_accounting_prep/run_static_execution.py --server "$SQL" --server-sha256 "$SQL_SHA" --source-commit "$SOURCE" --backend mariadb --case QP05 --reward-vnum 16048 --evidence-dir /evidence/current-sql-qp05-white
python3 -B tests/async/quest_accounting_prep/run_world_execution.py --server "$SQL" --server-sha256 "$SQL_SHA" --source-commit "$SOURCE" --backend mariadb --evidence-dir /evidence/current-sql-world
python3 -B tests/async/quest_accounting_prep/run_world_execution.py --server "$FLAT" --server-sha256 "$FLAT_SHA" --source-commit "$SOURCE" --backend flatfile --evidence-dir /evidence/current-flat-world
```

Current SQL cleanup files confirm schema quest_journey_test_c02d1bc90c93,
quest_journey_test_49f955dfd3a2 and quest_journey_test_fb8e09da0af6 each absent.
Final actual SELECT census of the owned server confirms0 matching schemas
(`owned-schema-cleanup.json`). An initial extra CLI census failed shell quoting;
the read-only Python parameterized successor passed; all original individual
schema-cleanup assertions had already passed. No migrations/writes were run
against any external server. The owned runtime is stopped after inspection;
raw failures, logs, player data and generated worlds remain private/uncommitted.
