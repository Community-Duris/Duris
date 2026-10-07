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

Exact build/run commands, server ELF/source/schema pins, actual results and cleanup will
be appended after execution. No current gameplay result is claimed by reservation.
Private artifacts remain ignored under bin/tests/quest-current-275; no credentials,
logs, players, generated worlds or binaries enter commits. New authoring is limited
to the owned reader and this documentation; shared test drivers are unchanged.
