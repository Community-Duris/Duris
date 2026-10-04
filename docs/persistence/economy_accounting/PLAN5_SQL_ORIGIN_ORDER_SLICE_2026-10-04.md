# Plan 5: native numeric ordering in read-only SQL baseline origins

Delivery branch: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `c693d18bfc8f06e5da7911ed00f95e40db89a463`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/origin-order-evidence.json` record its exact SHA after commit.
Refreshed canonical remote: `f7d26eaa721cd3b675c0b0c65009a2535813f400`.
Native source tree: `d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, including
canonical migration 0056. Primary-owner unpublished fixes are outside this
source qualification. Only this delivery branch is published by this lane.

## Established defect and correction

At the slice base, the actual native baseline encoder/storage fixture creates
an inactive private store with a canonical witness whose wallet lifetimes are
255, 256 and 512, followed by a bank lifetime of UINT64_MAX. Native ordering
compares lineage bytes, numeric kind, unsigned lifetime and unsigned context.
The SQL origin decoder instead compared hexadecimal encodings of the whole
40-byte key. Because lifetime/context fields are little endian, it rejected
this valid witness. Reordering its holdings by encoded bytes and recomputing
the witness SHA-256 produced a noncanonical witness that the old reader accepted.
Both wrong decisions and the failing assertion are retained in
`tmp/plan5/native-origin-order-red.log`; the driver is
`tmp/plan5/reproduce-native-origin-order.py`.

The independent reader now compares the tuple returned by its existing account
key decoder. It retains the exact 40-byte encoded key in operator output. Its
existing kind restrictions, distinct lifetime namespace, digest checks, bounds,
read-only consistent cut and rollback behavior remain in force. The partial SQL
snapshot exporter consumes this same origin reader. No production codec,
compiler, mutation/storage owner or correction path is imported into the audit.

Owned files are `scripts/economic_sql_audit_origins.py`,
`tests/async/test_economic_sql_audit_origins.py`, and this report. There is no
product interface, schema, accounting contract or activation-owner change.
No shared coordinator, producer, writer registry/matrix or migration is edited.

## Focused and native SQL proof

The existing origin test file now checks numeric ordering around 255/256,
65535/65536, UINT32_MAX/2^32, INT64_MAX/2^63 and UINT64_MAX-1/UINT64_MAX.
It checks numeric regressions that appear sorted as bytes, kind precedence,
UINT64_MAX context, and duplicate lifetimes across kinds/contexts. Both direct
decoding and transaction capture are exercised, including rollback/refusal.

The explicitly enabled native SQL cases reuse the existing 19-source native
baseline fixture and private-database/migration helpers. They extract the four
unchanged EAB1 witnesses, their hash-bound ECR2 records, CCM1 commands, EAI1
intents and EAP1 plans. Fresh databases adopt the reviewed bootstrap and run
pending canonical migrations through `0056_spell_ward_durability`. The private
fixture owner inserts those bytes, corresponding accounting root metadata,
effects, postings, source claims, baseline controls and identity reservations.
It leaves SQL active_epoch null; the original flatfile control also remains
inactive. These are fixtures for the independent reader, not execution of the
native SQL mutation/lifecycle transaction or a qualified gameplay journey.

Each engine uses a distinct SELECT-only reader. An attempted private fixture
UPDATE is denied with SQL error 1142. The reader makes three successful captures
(both retained epochs and the restored first epoch) and refuses three corruptions:
byte-sorted holdings, reversed holdings and a duplicate holding lifetime. Each
corruption has a freshly recomputed valid witness digest. All six captures end
with exactly one rollback and cursor close, and issue only SELECT plus transaction
configuration/start statements. All ten compared SQL tables retain exactly the
same rows before/after each capture. Original native files retain the same bytes,
modes and link counts. Restoring a known test fixture is done by its separate
private owner; the audit reader never repairs findings.

Final validation is **102 PASS, zero skips** in the explicit Linux invocations:

| Command | Result and scope | Evidence |
| --- | --- | --- |
| `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1 python3 -u tests/async/test_economic_sql_audit_origins.py -v` | 17 PASS, zero skips, 154.998 seconds; native baseline fixture, MySQL 8.0.46-0ubuntu0.22.04.4 and MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1 through canonical 0056 | `tmp/plan5/native-origin-order-sql-green.log` |
| `python3 -m unittest discover -s tests/async -p test_reconcile_economy_accounting.py -v` | 69 PASS, 1.546 seconds | `tmp/plan5/origin-order-reconcile.log` |
| `python3 tests/async/test_audit_accounting_invariants.py -v` | 16 PASS, 0.215 seconds | `tmp/plan5/origin-order-invariants.log` |
| `python -m py_compile scripts/economic_sql_audit_origins.py tests/async/test_economic_sql_audit_origins.py` | PASS | Source hashes below |
| `git diff --check` | PASS | Narrow three-file slice |

The native/SQL invocation runs in `duris-plan5-origin-sql-tools:local`, immutable
image ID `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Its parent is the previously qualified both-engine recovery image
`sha256:192535b64b6212f908bd54befe5ced5285494875d675a15202aee20c571f7058`.
It adds only python3-pymysql 1.0.2-2ubuntu1.1 (Python package version 1.0.2).
The pure suites use the unchanged base tools image
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`.
The toolchain is Ubuntu 24.04, Python 3.12.3 and GCC 13.3.0. Each invocation
mounts this checkout read-only at `/workspace`; the native/SQL invocation also
mounts this worktree's ignored `bin` writable at `/workspace/bin`. It adds no
container capabilities, creates disposable database processes with fresh
datadirs/Unix sockets and TCP disabled, and terminates them before exit.

Equivalent native/SQL wrapper (the evidence recorder pins the exact runtime):

```powershell
docker run --rm --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max,target=/workspace,readonly' --mount 'type=bind,source=C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max\bin,target=/workspace/bin' --workdir /workspace --env DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1 duris-plan5-origin-sql-tools:local python3 -u tests/async/test_economic_sql_audit_origins.py -v
```

Exact frozen executable source SHA-256:

| File | SHA-256 |
| --- | --- |
| `scripts/economic_sql_audit_origins.py` | `64a8fdaeb0399b5f9b275f80170afe2082200eec6c483470e3b176848670078f` |
| `tests/async/test_economic_sql_audit_origins.py` | `8323ada318f0a0d034129f33df5ad7751a3dd9513d13f197173abec66e3eac7e` |

The native ASan/UBSan fixture binary is unchanged from the baseline-book slice,
SHA-256 `70619e34caa45cb4cbac11f6f7277f64f92b1f551bc3d78dbbcd10b70f7510fc`.
The final run verifies/reuses all 19 native objects (zero compile/link time;
16.540 seconds for the checked cache lookup), then actually executes the native
fixture. It does not represent a fresh native build. The evidence manifest pins
all 1,232 native inputs, owned files, reader/helper inputs, migrations, actual
fixture cache artifacts, four witness digests, PyMySQL source files and logs.
Post-commit checks require the committed Git blobs to match all owned and native
raw inputs. The default portable unit invocation skips the two explicitly gated
SQL cases; those skips are separate from the passing final invocation above.

The first integration attempt failed before opening a database because the new
fixture parser treated native segment-relative offsets as absolute offsets.
The corrected parser adds the 48-byte envelope and 32-byte segment header.
Its failed run remains in `tmp/plan5/native-origin-order-sql-first.log`.
The second run extracted the native bytes but lacked the existing audit tools'
PyMySQL dependency, so both SQL tests failed before starting a database. It remains
in `tmp/plan5/native-origin-order-sql-runtime-missing.log`. A private derivative
of the existing recovery image supplies Ubuntu's python3-pymysql package;
its Dockerfile and build log are retained under `tmp/plan5/`. These failures are
not skips or successful qualification of a backend. The next attempt ran the
canonical migrations on both engines, then failed when the test tried to inspect
history using the session already closed by `run_pending`. The final test reads
history in a fresh private SQL session. That failed attempt remains in
`tmp/plan5/native-origin-order-sql-closed-history.log`.

## Shared release-registration handoff

The primary coordinator owner should register an explicit invocation of
`NativeSQLOriginTests.test_native_origins_mysql_8` and
`NativeSQLOriginTests.test_native_origins_mariadb` from the existing test file,
with `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1`, both engine/server tools, g++,
libcrypto and PyMySQL available. The cases create fresh private datadirs/Unix
sockets, disable TCP, and need no existing database, checkout credentials,
game service, Redis, production data or namespace capability.

Relevant shared fields are the integration row's `path`, `arguments`,
`environment`, `required_cases`, `provider`, `engines` and timeout, plus the
runtime package inventory. The required-case invariant is that both selected
engines execute and pass with zero skips; the default fast unit invocation's
two explicit integration skips do not satisfy it. Consumers are the central
integration runner and combined release report. Verify required-case collection,
SDK/tool prerequisites, actual engine versions, fresh canonical migration 0056,
SELECT-only permission enforcement and unchanged fixture state. These shared
files are intentionally left with the primary owner.

The earlier central registration requests for the independent flatfile native
unit and economic-record-loss recovery case remain open. The earlier narrow
Plan 1 request for an authority-bound retained baseline-initialization/opening
marker also remains open; this SQL ordering correction does not solve complete
loss of an initialized empty flatfile namespace.

## Remaining gates and notebook

This component evidence does not qualify the primary owner's combined candidate.
The final published combined source, all R1-R8 acceptance, actual supported
player journeys/fault/restart/replay matrix on both backends, populated upgrades,
strict production builds, complete independent audit/authority binding,
retention/alias erasure/export and measured workload budgets remain required.
Inventory coverage and synthetic/component passes do not establish release
completion. Full backup/restore is not rerun for this reader-only change; its
native/backup/restore inputs remain unchanged from the preceding qualified
baseline-book slice. This report does not transfer that prior run to a new
combined candidate or qualify a full SQL snapshot/CLI/authorization journey.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable. The pending input request is unanswered and no curator capability
is exposed. This source handoff does not substitute for or claim the required
notebook update. Accounting stays inactive; wallet-root item exclusions and the
declined inactive spell-path change are preserved.
