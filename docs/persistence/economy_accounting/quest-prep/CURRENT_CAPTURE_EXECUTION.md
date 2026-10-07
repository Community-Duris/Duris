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

Exact build/run commands, ELF/source/schema pins, actual results and cleanup will
be appended after execution. No current gameplay result is claimed by reservation.
Private artifacts remain ignored under bin/tests/quest-current-275; no credentials,
logs, players, generated worlds or binaries enter commits. New authoring is limited
to the owned reader and this documentation; shared test drivers are unchanged.
