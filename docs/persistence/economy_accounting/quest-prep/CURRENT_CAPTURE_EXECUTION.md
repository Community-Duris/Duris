# Current birth-origin capture execution reservation — 2026-10-07

Reservation/compatibility bundle7429e4f21d83d9bd8d07c5447bd33b13e2f5ec0c is pushed.
Base accounting275df7f626e12cb396a22da34317a4e7f355e9a1; history-preserving merge
1fe9f048f. Research55905eac1906cf59405764407f9d22497cccfff3. Continuing Goal ACTIVE;
this execution is a selected checkpoint, not a completion boundary.

Owned reader change: require schema63 quest_mobile_native_birth_origin to be an
InnoDB snapshot source, bound its aggregate canonical_origin BLOB/row material
before fetching, capture original birth_operation/publication_revision/bytes and
include actual birth operations in receipt/book queries. Missing origins remain
missing evidence. No value is manufactured, and an origin's bytes alone are not
native reconstruction authority. Use the maintained
quest_mobile_native_origin_sql_lock owner for complete command/receipt and
immutable-reference validation; its lock/transaction responsibility is separate
from this SELECT-only snapshot. No copied origin codec is added.

Planned isolated execution: new source archive and SQL ELF from the exact published
prep commit containing this reader, then original QP06 XP-ACK/later-move journey
through run_quest_execution.py. Production Kord prototypes/Q terms and original
specials, secret-item discovery, fault calibration, deadlines, input/reward UIDs,
cash/XP/ACK, first and second cold state checks remain unchanged. Disposable
quest_journey_test_<12hex> schema only; reader uses explicit legacy-no-epoch mode.
This run will prove current schema64 statements and genuine legacy activity,
not active native item-GIVE or original reset birth. Native origin rows must stay
empty in this legacy scope. Previous source6db artifacts/results are preserved.

## Inspector link qualification

Fresh source archive36bfef3c9e9a97b5dd94fbf620ffe2f21e02a8d1, SHA256
e86d70278def3bfc18754244e69859c296af651ea7e7c3739e97a993b78eabe0.
Original owned inspector invocation reproduced undefined references to
auction_native_command_decode, lockpick_retirement_payload_valid and the new
native_quest_cost/coin_give project/encode/decode methods. These are replaced
dependency seams, not executed production behavior failures. The initial compiler
failure remains in the tool transcript; no test assertion was relaxed.

The owned build_quest_inspector invocation now adds only the actual providers
src/economy/native_quest_cost.c, native_quest_coin_give.c,
auction_native_command_context.c and src/item/lockpick_retirement_continuation.c
to its already-added economic_baseline_codec.c/adapter.c. All original60 sources,
FLAGS, LINK_FLAGS and wrappers remain. Shared _flatfile_player_fixture.py is
unchanged; its owner should add these six current dependencies at its boundary.

Executed in the owned container, cwd/candidate:

```bash
python3 -B -c "import sys; sys.path.insert(0,'tests/async/quest_accounting_prep'); import run_quest_execution as r; print(r.build_quest_inspector())"
```

Final PASS:66 actual providers,73.468s compile/0.912s link/4.953s verified lookup;
ELF SHA256 a6b118eae743c034504a6f45936b827ed88cb2ce72a5ad511b2bd8729e86dafd.
Private /candidate/bin/regression-artifacts/native/binaries/
fe039f2ace581a4aa22039faa2aca78c5ba62d2c22eecac0940e48356725b356-ebqf6rb1.
This verifies the inspector link, not the server or native acceptance. Only the
owned Python helper is overlaid onto the archived source; production inputs stay36bf.

## Executed server, SQL statements and actual journey

Fresh full maintained build PASS,753 providers and full link,565.856s:

```bash
make -C /candidate/src -j2 BIN_ROOT=/evidence/build-sql
```

No original warning/hardening flags changed. Maintained default development
profile includes TEST_MUD and __NO_TESTS__; this is not a production-profile claim.
Server source36bfef3c9e9a97b5dd94fbf620ffe2f21e02a8d1; SQL ELF SHA256
c997fabfb1373f86f84dc8b33e5202f7f8ef2ac4af2646c800f0649c1fb319b6.
Current schema manifest41181dc0b2938366bf408e6ad6ec1b11e4c170d18777f84efa5e93dc02374974;
all five actual cuts contain64 applied migration-history rows. Runtime only
quest-prep-275-runtime, same pinned QA image, networknone,2CPU/3GiB/SYS_PTRACE.
Disposable loopback MariaDB instance/datadir generated inside this container;
credentials generated locally and never printed/committed. TEST_DB_DISPOSABLE=1,
TEST_DB_HOST=127.0.0.1, TEST_DB_PORT=33306, TEST_DB_USER=prep_runner;
TEST_DB_PASSWORD supplied only via the subprocess environment.

Executed original journey command, cwd/candidate:

```bash
python3 -B tests/async/quest_accounting_prep/run_quest_execution.py --server /evidence/build-sql/server/dms_new --server-sha256 c997fabfb1373f86f84dc8b33e5202f7f8ef2ac4af2646c800f0649c1fb319b6 --source-commit 36bfef3c9e9a97b5dd94fbf620ffe2f21e02a8d1 --backend mariadb --case QP06 --fault-phase xp-ack --move-reward --evidence-dir /evidence/current-kord-xp-move
```

**Original aggregate FAIL**, exit1,157.397s owned /159.543s outer runner. Maintained
driver returned after crash, recovery, later drop and second cold restart; all
five cuts exist. Post-run legacy_xp's single-file continuation-reader build then
failed to link the new inline fee decoder's external economic_source_event_decode,
critical_operation_id_is_zero and item_transfer_command_decode_result methods.
This original failed result and stdout/stderr remain untouched. A second small
adapter link attempt exposed shop_trade_recovery_forest methods; their actual
maintained provider was then included. No gameplay assertion/deadline changed.

Actual capture statements execute against schema64. Every cut holds the SQL
consistent read-only snapshot; rollback verification observes transaction0 after
capture. Missing-player, real row-limit and actual continuation BLOB output-limit
controls refuse. InnoDB birth-origin table/current columns are checked and its
SELECT executes with an empty legacy scope. Native birth_origins/epochs are empty;
nonempty native-origin prefetch/decoder/authority proof remains UNRUN, not inferred
from the empty-table compatibility result.

Root schema cleanup after this failed aggregate: **zero quest_journey_test_ schemas**;
owned MariaDB stopped normally. No production or external DB access occurred.

## Adapter repair and exact offline predicates

Owned legacy_xp.decode still calls the maintained quest_reward_continuation_decode
through the unchanged read_quest_continuation.cpp; no decoder substitute. Original
flags -std=c++20 -Wall -Wextra -Werror -Isrc and -lcrypto remain. Actual12 providers:
adapter, item_transfer_command.c, quest_mobile_native_reference.c,
economic_source_event.c, craft_pouch_mutation.c, chaos_pouch_ledger.c,
player_snapshot_codec.c, critical_command.c, native_quest_cost.c,
native_quest_coin_give.c, lockpick_retirement_continuation.c and
shop_trade_recovery_manifest.c. All maintained C++ bytes belong to archive36bf.
Adapter ELF64578320b49680fa8b0ac7586bf77fe59ae0e9c266aeac03e6e6271ef82a38ed;
successful12-provider build3.842s compile/0.115s link/6.217s verified lookup.

The original post-run block was moved without changing its predicates into
verify_cuts, reused by normal execution and --verify-existing. Retained mode checks
source/ELF/schema/options and the exact complete named-cut set; it opens no SQL
session and repeats no gameplay. It writes verified-cuts.json exclusively rather
than changing result.json. No dropped-UID/XP/room/cold expectation was weakened.

First offline verification PASS4.545s on original retained cuts; initial result
ad43b4ea3ac0da445049b6fd95ff4df26ca6fdd375f2e128831601e5349daf90 remains retained.
Its executed runner SHA9582bc5969878ad10e1a0fb9a4671246a6ffd96e5eb7f39376f5077d7faefbf2
is the version before the five-line exact named-cut guard. Those exact bytes were
preserved privately as executed-before-cut-guard.py and their hash verified. This
initial result is not relabeled as a run of the subsequently guarded helper46ad.
After adding the exact named-cut guard, final published-helper verification PASS
7.037s against byte-identical copies in /evidence/current-kord-guarded-verification.
All six copied original JSONs match their originals before and after verification.
Exact final command is the journey command above with evidence-dir changed to that
directory and **--verify-existing** appended. Final verified-cuts SHA256
658368d759a5f97698226f45892d8ce7e3d7bc2f4e247e615ba701dbf44b7211.
Original aggregate result SHA143f3c4b44fa4fd52f66853ebfd700c78a86e114fd4c47ec099290a85d4c97df
still says FAIL. Wrong source, wrong ELF and attempted verification overwrite each
exit1 with the expected refusal and unchanged evidence; private controls retain
the exact commands/errors. No second crash journey was run for this postprocessing
link repair.

| Cut | SHA256 |
|---|---|
| before-offering | c3ec8f9fd0817923aed7f4b389d6c00fa4b5fe5cf6a6b597899e08d4ccb3f56a |
| at-crash | 964476ea5ef781601b7b7bc2854873e15b13bf59c4a1e897fba4c63a57a77e4d |
| recovered | 49c21af16affb2e912c08253fb1e5a9b60f58f31c9a1eb0ec128b3ed1a0102bf |
| after-move and second-restart | 6e1c0aeb6a7803c8b6de3db0eccb56ed0eb04077bd49e1d29996961a0e1c947e |

Actual offering operation74efcd77295fee7b1bcd55aafe776566. Exact input roots
UID4/29262,6/29263,8/29264 move player1 revision2→destruction8/state2 revision3.
Reward820/29237 is a root/parentnull, player1/state1 revision1 after recovery;
later room22800/type3 revision2 and identical second-cold custody. Cash0→C3000
represented [0,0,0,3]; XP1→521 once, frozen200/slot2/mask4 and actual Human1.3,
well-rested×2 policy. Obligation acknowledged0 at crash→1 after recovery. Ordinary
drop pays no additional cash/XP. Existing strict native bind rejects invalid
lineage pin; these are genuine **legacy** custody/ledger/XP/ACK/replay/move results.

Published/executed helper hashes (distinct from server/source pins):

| Owned helper | SHA256 |
|---|---|
| run_quest_execution.py | 46ad19c42b0de81e2e2c7258208c0c5e9544689f5f8685cba70b54347a0c4abd |
| legacy_xp.py | 70489d1be00e829c0399dc74662c9512eb6371fc935b4405044507a31cb34ccb |
| unchanged read_quest_continuation.cpp | 3071bbf91c1daf3c6a4a88eb393e59b5a4a0f5c9760b5092ec40e8941802ef32 |
| quest_cut_checks.py | c0074d0a413cc7340c3144e1c21a9d4a1010e9191005c0e99da39821c4583338 |

Current oracle additionally compares retained birth-origin rows/bytes whenever
either cut includes them. Seventeen counterfactual tests PASS0.236s, including
changed birth-operation/publication-revision/bytes, omitted row and missing table
field. Older pinned captures without this new table retain their old scope. These
corruption controls are oracle qualification, not native evidence.

## Import and continuing disposition

Inspector bundle3cc3867c4b3704e0e1f20e0fa5059ba46b9627ad (base36bf) is now pushed;
GitHub rejected four ordinary retries and one HTTP/1.1 retry with Internal Server
Error before the later identical push succeeded. No force-push or alternate branch.
Import current owned postprocessor/oracle changes after the compatible reader and
inspector helpers; do not merge worker synchronization history as production fixes.
Shared manifest request remains six actual provider additions described above.
Native active lifecycle/world/birth, QP04 durable refunds, pre-ACK move and original
parent/child retirement remain exact primary dependencies in
CONTINUING_RECONCILIATION.md. Original mini RED and old source6db full-world control
remain unchanged. Continuing Goal stays ACTIVE; this checkpoint is not completion.

Private artifacts: ignored bin/tests/quest-current-275, including build-sql,
current-kord-xp-move, current-kord-guarded-verification and terminal/guard records.
No credentials, logs, players, worlds or binaries are committed. No new shared
production/driver/schema/registry/manifest edits; original major-batch cadence and
primary integrated final qualification remain.
