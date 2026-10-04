# Plan 5: independent retained source-event grammar

Delivery branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `cc224b2d12a894dc5cac195297dbc4db03029548`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/source-event-evidence.json` record its exact SHA after commit.
Refreshed canonical remote: `f7d26eaa721cd3b675c0b0c65009a2535813f400`.
Native source tree: `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, including
canonical migration 0056. Primary-owner unpublished fixes are outside this
qualification. Only the delivery branch is published by this lane.

## Established defect and grammar fix

The independent reader previously accepted a source event when it was 96 lower
case hex characters and its operation/claim references agreed. Native S48
decode additionally requires version 1, a current source kind 1 through 23,
and nonzero 16-byte source and generation identities. Native sequence and slot
fields permit zero and their full unsigned 64-bit/32-bit ranges.

A strict ASan/UBSan fixture against seven production sources in SQL and
flatfile modes produced 104 native decisions: 92 valid events and 12 refusals.
At the slice base, nine native-refused fixed-length events were reported clean
by the reader in each mode: kinds 0/24/65535, versions 0/2/65535, zero source,
zero generation, and both identities zero. The other three native refusals
were short, long and empty encodings, already caught by the old lexical check.
The failing comparison is retained in `tmp/plan5/native-source-event-red.log`
with its driver `tmp/plan5/reproduce-native-source-event.py`. Its first strict
fixture compile failed on an uncast UINT16_MAX initializer; the corrected
compile and actual reader defect are separate from that retained failure.

The standard-library reader now independently decodes this grammar. Present
operation source events produce `invalid_source_event` when malformed, including
rejected roots. Selected and prior-epoch claims produce `invalid_source_claim`
even when their retained references agree. UID reference roots and the existing
coin-pile lifecycle reader apply the same grammar before their existing checks.
Optional absent source events keep their existing behavior. Required-source,
duplicate-claim, baseline-claim and domain-specific coin-pile checks still run.

The bounded operator view retains malformed fixed-length source IDs as
diagnostic evidence while preserving the global refusal and omitting aliases.
The view's lexical output checks restrict safe ID rendering; they do not certify
the source grammar. Neither reader repairs evidence or writes authority.

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `tests/async/test_reconcile_economy_accounting.py`
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`
- this report

No shared contract/schema/interface change is needed for this slice. Shared
coordinator, contracts, producers, registry/matrix and activation-owner files
are untouched. Inactive behavior, wallet-root item exclusions, the declined
inactive spell-path change and active blackjack refusal remain preserved.

## Native and disposable SQL proof

Four new fast regression methods cover the current source-kind range and
unsigned boundaries; malformed kinds/versions/identities/lengths/non-hex and
non-string values; matching selected and prior claims; UID/coin-pile readers;
and unchanged input with bounded ID-only operator views. CLI operation views
at limits 0/1/100 exit 1 for corrupt evidence, retain both global exceptions,
omit aliases and leave their input file byte-identical. Valid fixtures now use
canonical S48 identities instead of repeated opaque bytes.

The existing explicitly enabled `NativeStakeSQLTests` case additionally emits
all 104 native source decisions from the same generated fixture as its actual
EAI1 intents/EAP1 plans. It compiles eight production sources plus the fixture
under strict warnings and ASan/UBSan in both backend modes. Both modes produce
identical decisions and bytes. The independent decoder agrees on all 104 cases,
including all 23 kinds and both unsigned boundaries.

Each canonical database has a new private datadir/Unix socket, TCP disabled,
reviewed bootstrap adoption and pending migrations through
`0056_spell_ward_durability`. A fixture owner seeds the native bytes, associated
rows and known opening/native anchors. A separate SELECT-only account reads a
consistent read-only transaction; its private UPDATE attempt is denied with
SQL error 1142 on each engine. Accounting stays inactive.

Each engine completes 14 canonical reads: held and terminal stakes, the existing
nonzero terminal-effect corruption and restoration, nine matching malformed
operation/claim substitutions, and the restored source. Every read has exactly
one rollback/cursor close and leaves all seven compared SQL tables unchanged.
The nine source corruptions each yield exactly `invalid_source_event` and
`invalid_source_claim`. The fixture owner replaces a source/claim pair in one
transaction without disabling the canonical foreign key. The initial direct
UPDATE attempt was correctly rejected by that FK and is retained separately.

The broader existing SQL audit runner also completes on fresh isolated MySQL
and MariaDB instances. It uses explicit minimal fixture DDL, rather than the
canonical migrated schema. Each engine adds 27 corrupt cuts: the same nine
identity defects for a selected money root, a selected UID root and a
prior-epoch claim. They produce their precise source/UID exceptions alongside
the fixture's original three refusal reasons. Each cut verifies SELECT-only
statements, rollback/close, unchanged input and all 21 fixture tables unchanged;
restored fixtures recover their exact original cut. Existing UID, money,
orphan evidence, interrupted capture and operator assertions still pass.

The broader runner requires loopback TCP. Its ignored harness adapts only a
new private daemon's launch to an ephemeral 127.0.0.1 port inside an unexposed
container, creates a temporary fixture owner and lets the guarded runner create
its SELECT-only reader. MySQL 8.0 uses its supported mysql_native_password
authentication for this fixture. The first default-auth attempt required an
uninstalled Python cryptography package and is retained separately; no runtime
dependency or product configuration was added. The canonical proof above keeps
TCP disabled and requires no such adaptation.

## Exact commands and retained artifacts

All Linux jobs use `duris-plan5-origin-sql-tools:local`, immutable image ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It contains Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL package
1.0.2-2ubuntu1.1, MySQL 8.0.46-0ubuntu0.22.04.4 and
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1.

Common invocation: `docker run --rm --mount
type=bind,source=<worktree>,target=/workspace,readonly --workdir /workspace
<image> <command>`. Native commands also mount `<worktree>/bin` read-write at
`/workspace/bin`. No ports are published or additional capabilities granted;
checkout environment/credentials are excluded by the private-database helper.

| Command inside that container | Result | Evidence |
| --- | --- | --- |
| `DURIS_RUN_STAKE_SQL_INTEGRATION=1 python3 -u tests/async/test_reconcile_economy_accounting.py -v` | 79 PASS, zero skips, 256.629 seconds; 104 native cases in both modes, 28 canonical read-only cuts through 0056, 18 source corruptions and two existing money corruptions | `tmp/plan5/source-event-qualified-green.log` |
| `python3 -u tmp/plan5/qualify-source-event-snapshot.py` | Both engines PASS; 54 new source corrupt cuts plus the existing guarded SQL audit suite; minimal fixture DDL | `tmp/plan5/source-event-snapshot-green.log` |
| `python3 -u tests/async/test_economic_sql_audit_origins.py -v` | 15 PASS, two explicitly gated native-origin skips, 0.013 seconds | `tmp/plan5/source-event-origins.log` |
| `python3 -u tests/async/test_audit_accounting_invariants.py -v` | 16 PASS, zero skips, 0.209 seconds | `tmp/plan5/source-event-invariants.log` |
| `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py tests/async/run_economic_sql_audit_snapshot_mysql.py` | PASS on host | recorded with frozen inputs |
| `git diff --check` | PASS | recorded with frozen inputs |

Native command flags are `-std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g
-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -Isrc`
and `-lcrypto`; flatfile adds `-D__NO_MYSQL__`. Runtime sets
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

The final native fixture is isolated under `bin/tests/plan5-source-event-sql`.
Both final binaries have SHA-256
`f5552d5bc582118f5c8b27c3e4ea080bb67f4aa573cde5bcc8c399d495fe439d`;
both encoded outputs have SHA-256
`2fa19add9843a7840b440d6ba114ceb96f7e338b39e4e9817f207e587a8bc1b7`.
The original RED binaries under `bin/tests/plan5-source-event-red` have SHA-256
`4035854a6a6e5d9e4dfa4e11859413adf2f1ed0ac2a529440b2913f2f8490363`.
The earlier retained-stake fixture's original source/binaries/output were
reconstructed from the exact slice base and checked against its existing
recorded hashes after the first expanded fixture reused that directory. Its
preserved binary/output hashes remain fc0973cf.../f9a2c41a..., and the current
test uses the separate directory above. Preservation is logged in
`tmp/plan5/source-event-preserved-prior-artifacts.log`.

`tmp/plan5/source-event-frozen-inputs.json` freezes all three executable files.
`tmp/plan5/source-event-evidence.json` records their full hashes, all 1,232 native
input hashes, migration/helper/registry hashes, artifacts, failed/passing logs,
image/toolchain identities, exact commands and final commit/tree identity.
The recorder is `tmp/plan5/record-source-event-evidence.py --committed`.
Ignored logs, generated binaries, private SQL files and environment data are
not committed.

## Shared handoff and remaining gates

The primary coordinator owner should retain the existing explicit registration
of `NativeStakeSQLTests.test_native_stake_sql_both_engines` with
`DURIS_RUN_STAKE_SQL_INTEGRATION=1`; it now also qualifies 104 source-codec
decisions and matching malformed SQL source rows on both engines. Register the
broader guarded `run_economic_sql_audit_snapshot_mysql.py` invocation for both
engines with its explicit disposable loopback guard. Its new 27 source faults
per engine require precise exceptions and unchanged-table/rollback assertions.
No shared manifest was edited here. Default fast collection has 78 passes and
one native/SQL skip; it does not provide this native/SQL qualification. The two
native-origin cases were deliberately not rerun; they remain explicit skips
in this slice's auxiliary origin command.

This slice certifies source-event identity grammar, not reason/source-kind
compatibility, source entitlement, writer reachability, active admission,
producer publication, replay or a real player journey. Native metadata has
reason/source-kind rules that remain an independent audit gate. SQL fixture
opening/native anchors and command/domain bindings are supplied structural
evidence. Minimal fixture DDL does not qualify canonical migrations; the
separate fresh canonical proof does not qualify populated upgrades.

Maintained server C++ code did not change, so server builds and the full
backup/restore suite were not repeated for this reader-only change. Current
combined primary-owner fixes plus 0056 still need their own published exact
candidate qualification. Authority binding, the shared baseline-marker request
from the baseline-book slice, retained evidence/erasure/export policy, populated
upgrades on both engines, declared measured budgets and the full gameplay/fault/
replay matrix remain open. Coverage inventory and these isolated passes do not
make the release ready. Accounting was not activated or deployed.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable after the prior repository/remote/workspace/tool discovery. The
existing clarification is still unanswered. This report and its evidence are
ready for the curator; they are not claimed as a project notebook update.
