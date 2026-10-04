# Plan 5: independent reason/source-kind policy

Delivery branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `a21cb160f2dc681bcf358933b92238ae6180b280`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/source-policy-evidence.json` record its exact SHA after commit.
Refreshed canonical remote: `f7d26eaa721cd3b675c0b0c65009a2535813f400`.
Native source tree: `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, including
canonical migration 0056. Primary-owner unpublished fixes are outside this
qualification. Only the delivery branch is published by this lane.

## Established defect and independent policy fix

Native metadata constrains source kind by reason even when the source is
optional. For example, a present coin-transfer source must be lifecycle kind
16; quest-reward kind 1 is unauthorized. The previous reader checked source
identity grammar and matching references but did not check that pairing.

A strict ASan/UBSan fixture against seven production sources generated all
46 reasons by 23 present kinds, plus absence for each reason, in SQL and
flatfile modes. Native metadata rejected reason 3/kind 1 and accepted
reason 3/kind 16. At the slice base, matching kind-1 source rows were reported
clean for a selected operation, a retained prior-epoch claim and a retained
UID root. All three false clean results occurred in each native mode, with
unchanged audit inputs. The RED driver and result are
`tmp/plan5/reproduce-native-source-policy.py` and
`tmp/plan5/native-source-policy-red.log`.

The standard-library reader now interprets the current reason/source-kind
policy independently. Its table constrains the same 33 reasons as the native
contract; the other 13 known reasons deliberately permit every valid kind.
Unknown reasons and boolean/string values never inherit that permissive
default. The reader does not import native mutation code or compile a policy
at runtime.

The existing fields consumed are selected `operations.reason/source_event`,
retained `source_claims.operation_reason/source_event`, and retained UID
roots' `reason/source_event`. A selected claim uses its selected operation's
reason and still checks any joined reason for disagreement. An out-of-scope
claim must carry a known integer `operation_reason`; absent/unknown policy
is `unknown_source_claim_policy`, never a clean historical anchor.

Valid identities with the wrong kind produce `unauthorized_source_kind`,
`unauthorized_source_claim`, or `unauthorized_lineage_uid_source` at the
respective consumer. Existing grammar failures remain their existing
exceptions. Optional absent events remain optional, required-source checks
remain, and baseline source claims are refused using the authoritative
selected reason as well as retained metadata. Bounded operator views retain
global refusal and diagnostic IDs without aliases or input modification.

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `tests/async/test_reconcile_economy_accounting.py`
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`
- this report

No new interface fields or schema changes are needed. Shared coordinator,
contracts, producers, registry/matrix and activation-owner files are untouched.
Inactive behavior, wallet-root item exclusions, the declined inactive spell
change and active blackjack refusal remain preserved. The tests read retained
stake primitives and do not enable an active gambling writer.

## Native and disposable SQL proof

Five focused methods cover single- and multiple-kind policies, a native
unrestricted reason, unknown/type-invalid reasons and kinds, selected matching
mismatches, optional absence, required-source refusal, retained claims/UID
roots, authoritative baseline refusal and ID-only operator CLI views at
limits 0/1/100. The CLI exits 1 for corrupt evidence, retains both global
exceptions, omits aliases and leaves its input file byte-identical. Existing
price fixtures now use legitimate source kinds, and the prior-claim fixture
carries its known joined reason.

The existing explicitly enabled `NativeStakeSQLTests` fixture retains native
EAI1 intents/EAP1 plans and all 104 source grammar decisions. It additionally
generates 1,107 native metadata decisions per backend mode:

- 1,058 present source pairs: all 46 reasons by all 23 current kinds.
- 46 absent-source decisions, compared with the existing registry's required
  source flags.
- Three unknown reasons, 0/47/65535, with a valid lifecycle source.

There are 346 accepted and 761 refused decisions. The native fixture uses
valid nonzero lineage/epoch/operation/actor/writer IDs, the required native
actor kind, version/policy/compiler 1, and a distinct supplied original ID.
Those fields remove unrelated metadata failure causes from this comparison;
the matrix does not audit their durable authority or original-operation link.
The independent reason/source-kind decisions and required-source flags agree
with every native result. Both modes produce identical output bytes.

Each canonical database has a fresh private datadir/Unix socket with TCP
disabled, reviewed bootstrap adoption and pending migrations through
`0056_spell_ward_durability`. Native intent/plan bytes and known opening/native
anchors are seeded by a fixture owner. A distinct SELECT-only account reads
consistent read-only transactions; an UPDATE attempt is denied with error
1142 on each engine. Canonical source/root foreign keys remain enabled.

Each engine completes 37 reads: the previous 14 held/terminal, money-corruption,
grammar-corruption and restoration cuts, then all 22 wrong valid source kinds
for the retained payout reason and restoration. Each new mismatch yields
exactly `unauthorized_source_kind` and `unauthorized_source_claim`. Every cut
has one rollback/cursor close, issues only SELECT/transaction statements and
leaves all seven compared SQL tables unchanged. Accounting stays inactive.

The broader guarded SQL audit runner completes on fresh isolated instances of
both engines using its explicit minimal fixture DDL. Each engine adds 66
policy cuts: all 22 wrong valid kinds for a selected coin root, a selected
item-reward UID root, and a retained prior-epoch coin claim. The UID fixture
temporarily uses the existing item-reward reason 43 with its valid kind 18
before testing mismatches, then restores the exact original fixture. Each cut
preserves its original three refusal reasons and adds only the expected
source-policy exceptions. All 21 fixture tables and audit inputs stay
unchanged during reads; rollback/close and exact restored cuts pass. Existing
grammar, money, UID, orphan, interrupted-cut and operator checks still pass.

The ignored existing loopback harness adapts only a new private daemon to an
ephemeral 127.0.0.1 port inside an unexposed container. It creates a disposable
fixture owner and lets the guarded runner create its SELECT-only reader.
MySQL 8.0 uses its supported mysql_native_password authentication for this
fixture. Product configuration/dependencies and existing databases are not
used or changed. The harness's final `source-fault-captures=54` line retains
its original grammar-only count; the two new `SQL source policy: 66...` lines
record the additional 132 policy cuts.

## Commands, source and evidence

All Linux jobs use `duris-plan5-origin-sql-tools:local`, immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
It contains Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL package
1.0.2-2ubuntu1.1, MySQL 8.0.46-0ubuntu0.22.04.4 and
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1.

Common invocation: `docker run --rm --mount
type=bind,source=<worktree>,target=/workspace,readonly --workdir /workspace
<image> <command>`. Native jobs also mount `<worktree>/bin` read-write at
`/workspace/bin`; the qualification flag is supplied with Docker `--env`.
There are no published ports or added capabilities, and the private database
helper strips checkout environment and credentials.

| Command inside that container | Result | Evidence |
| --- | --- | --- |
| `DURIS_RUN_STAKE_SQL_INTEGRATION=1 python3 -u tests/async/test_reconcile_economy_accounting.py -v` | 84 PASS, zero skips, 256.678 seconds; both native modes, 1,107 policy decisions plus 104 grammar decisions per mode, both fresh canonical engines through 0056, 74 read-only cuts including 44 source-kind faults | `tmp/plan5/source-policy-qualified-green.log` |
| `python3 -u tmp/plan5/qualify-source-event-snapshot.py` | Both engines PASS; 132 new policy cuts plus 54 retained grammar cuts and the existing broader SQL audit suite; minimal fixture DDL | `tmp/plan5/source-policy-snapshot-green.log` |
| `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py tests/async/run_economic_sql_audit_snapshot_mysql.py` | PASS on host | frozen inputs/evidence manifest |
| `git diff --check` | PASS | frozen inputs/evidence manifest |

The initial fast run collected 84 tests, with 83 PASS and one explicitly gated
native/SQL skip in 2.371 seconds. The final explicitly enabled run is the
native/SQL proof; default collection does not establish it. Auxiliary native
origin, independent invariant and full backup/restore suites were not repeated
for this source-policy reader change. Maintained server C++ source did not
change, so no server build was repeated.

Native flags are `-std=c++20 -Wall -Wextra -Wpedantic -Werror -O1 -g
-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie -Isrc`
and `-lcrypto`; flatfile adds `-D__NO_MYSQL__`. Runtime sets
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

The new generated native fixture is under `bin/tests/plan5-source-policy-sql`,
preserving the preceding source-event/stake artifact directories. Both final
binaries have SHA-256
`97cda4472613a0c89eb0a2141f72efbc8edbec17d91c47ff8f20ae5cce2ae4bd`;
both output files have SHA-256
`9be4823c38d40b6aeceffc7b71d73f7aa96bd3c8973d2f736740fdeb7e5882fe`.
The original RED binaries under `bin/tests/plan5-source-policy-red` have
SHA-256 `aa8b923dd7bc3053cfde1f1608501bb630803d76e49778e5122710101328e340`.

`tmp/plan5/source-policy-frozen-inputs.json` freezes all three executable files.
`tmp/plan5/source-policy-evidence.json` records those hashes, all 1,232 native
input hashes, migration/helper/registry hashes, native artifacts, logs, image/
toolchain, commands, and committed source/tree identity. The recorder is
`tmp/plan5/record-source-policy-evidence.py --committed`. Logs, generated
binaries, private SQL files and environment data are not committed.

## Shared handoff and remaining gates

The primary coordinator owner should retain the explicit
`NativeStakeSQLTests.test_native_stake_sql_both_engines` registration with
`DURIS_RUN_STAKE_SQL_INTEGRATION=1`. Its required proof now includes both
backend modes, all 1,107 metadata decisions, both actual engines through
0056, precise source-kind exceptions and unchanged-table/rollback checks.
The existing guarded `run_economic_sql_audit_snapshot_mysql.py` invocation
should run on both engines with its explicit disposable loopback guard and
all 66 policy cuts per engine. Shared manifests are not edited here.

This slice checks current reason/source-kind policy, not full versioned
metadata authority, durable source entitlement, actor authorization,
writer coverage, producer admission/publication, replay or a real player
journey. Structural command/domain bindings and SQL opening/native anchors
are fixture inputs. The metadata matrix supplies original IDs without
resolving their durable history. Minimal fixture DDL does not qualify
migrations, and fresh canonical schema proof does not qualify populated
upgrades. Permanently declined active blackjack remains declined.

The primary's combined local fixes plus incoming 0056 still require a
published exact-candidate qualification. Authority binding, the previously
handed-off shared baseline marker, retention/erasure/export governance and
evidence, populated upgrades on both engines, measured budgets, and full
gameplay/fault/replay coverage remain open. These component passes and an
inventory count do not establish release completion. Nothing was activated,
merged, deployed, or repaired in production.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable after prior repository/remote/workspace/tool discovery. The
existing clarification is unanswered. This report/evidence are ready for the
curator and are not claimed as a project notebook update.
