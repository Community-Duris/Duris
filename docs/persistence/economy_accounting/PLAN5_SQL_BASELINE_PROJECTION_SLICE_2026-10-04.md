# Plan 5: reconcile normalized SQL baseline projections

This slice starts at `45c895ff472cc9023d4211bd140a0b0568ab2d5c` on
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. The result
commit, committed input hashes, complete commands/results and retained artifact
hashes are recorded in `tmp/plan5/sql-baseline-projection-evidence.json`.
Publication uses this lane's remote branch for integration by the primary owner.

The integration remote was refreshed at
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b` remains unchanged: 1,232 native
inputs and 236 migration inputs match the preceding slice. Native canonical
base is `7d2f8e8153f637c38e19054cf1202436d3f9a28a`. Every fresh disposable
SQL integration applies canonical migrations through
`0056_spell_ward_durability`. This result does not qualify an unpublished
combined candidate, and earlier 0055 results do not qualify it either.

## Reproduced defect and fix

The independent origin reader verified EAB1/EAI1/EAP1 and their committed root,
but did not compare retained baseline account effects, postings or reservations
with those canonical bytes. Ordinary operation views exclude reason 38. Deleting
one posting written by the actual native baseline owner therefore left the
audit's findings unchanged. Both canonical engines reproduced this RED: one
guarded test, two failures, zero skips, 153.120 seconds. The private fixture
owner restored its deleted row; no audit implementation performed a repair.

The independent reader now compares every field in three persisted families:

- Account effects: operation, dense account index, full account key, all before
  and after denominations, and both revisions, against the verified EAP1 plan.
- Postings: operation, dense line/event indexes, account/child indexes, every
  delta denomination and weighted copper value, against the same verified plan.
- Reservations: lineage, epoch, identity kind, full unsigned lifetime/UID and
  baseline operation, against EAB1's ordinary holdings and item identities.

The root verifier returns its already decoded plan to the projection comparison;
the audit imports no mutation, coordinator or producer code. Missing, extra,
duplicate or changed details refuse selected-book capture with
`EAB1 SQL projection mismatch`. Retained-book disagreement preserves the
existing `baseline_source_claim` diagnostic. A retained reservation rebound
to the selected root also contaminates the selected book and refuses capture.

Reservation reads include both their claimed lineage/epoch and their referenced
witness's lineage/epoch. Wrong-scope rows attached to a known baseline cannot
disappear through an inner join or a claimed-scope filter. The reader requires
all seven baseline source/detail tables to be InnoDB, within the existing
REPEATABLE READ, consistent READ ONLY transaction; success and refusal end with
rollback and cursor closure.

The three fixed-width projection families share a 100,000-row limit. A count
preflight bounds them before fetching; each SELECT also has a remaining-budget
limit. The full exporter applies the same combined bound across the retained
lineage before reading another epoch's book. The existing capsule bound remains
32 MiB. These structural limits do not establish measured workload budgets or
a fair live sweep watermark.

Existing audit formats, contracts, schema, native owners and activation state
are unchanged. The new internal source/count helpers have two real callers:
the selected origin reader and the retained-book source-claim exporter. No new
shared interface or schema request is required.

## Validation and exact inputs

Environment: `duris-plan5-origin-sql-tools:local`, image ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`;
Ubuntu 24.04, Python 3.12.3, GCC 13.3, PyMySQL 1.0.2-2ubuntu1.1.
MariaDB is `10.11.14-MariaDB-0ubuntu0.24.04.1`; MySQL is
`8.0.46-0ubuntu0.22.04.4`. The checkout is mounted read-only; only `bin/` has a
separate writable mount. Private disposable daemons use Unix sockets with TCP
disabled and a clean test environment. Checkout `.env` is not used.

The actual native SQL fixture initializes two books, applies one nonempty batch
to each, exactly replays and reconciles both, and leaves `active_epoch` NULL.
The existing 13 compile inputs are unchanged. Both C++20 binaries retain strict
warnings, ASan and UBSan. SQL binary SHA-256 is
`7985d95effd41456d445a5924e466a5ae5ad25fa9506d141d2ff1dd1168c4ee2`;
client-free is
`efaee7f33a1ebc38fe9b6afe9baaa32b66b8b3db1746679dc58c05cf39a9b027`.
Client-free initialize/apply/reconcile continue to refuse with `ENOTSUP`.

The final native matrix executes 121 damage cuts per engine: 60 new projection
cuts and the existing 61 root/source/book cuts. There are 55 capture refusals
and 66 exact diagnostic cuts. Six canonical constraint probes per engine verify
source composite FK, source operation uniqueness, both reservation foreign-book
FKs, posting line/event equality and allowed reservation kinds. Exact failures
are 1452, 1062, and CHECK codes 4025 on MariaDB / 3819 on MySQL.

All 15 captured authority/evidence tables remain byte-for-byte equivalent
during each audit; SELECT-only audit credentials reject UPDATE with 1142.
Eleven damaged-import cuts use only the private owner's foreign-key switch.
Foreign keys are enabled for every audit and native replay, and CHECK/UNIQUE
constraints remain enforced. Repairs run only in the private fixture owner,
after which native replay reproduces the original result and state.

The intact native fixture still reports one evidence-loss, two
missing-native-holding and two missing-native-item findings. It has no complete
native/world capture; these findings remain visible and accounting is inactive.

| Exact command | Result and evidence |
| --- | --- |
| `DURIS_RUN_NATIVE_BASELINE_AUDIT=1 python3 -u tests/async/test_native_sql_baseline_audit.py -v` in the image at `/workspace` | Pass: one guarded test, both canonical engines, zero skips, 226.083 seconds; 121 cuts/six constraints each, exact native replay and full sibling exporter/CLI matrix. `tmp/plan5/projection-green3.log`, `bin/tests/plan5-baseline-projection-audit/{mariadb,mysql}.log`. |
| `python3 -m unittest test_reconcile_economy_accounting test_economic_sql_audit_origins -v` at `/workspace/tests/async` | 120 collected: 115 passes, five explicit skips, 16.902 seconds. `tmp/plan5/projection-components2.log`. |
| `DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION=1 python3 -m unittest test_economic_sql_audit_origins.NativeSQLOriginTests -v` at `/workspace/tests/async` | Two passes, zero skips, 218.579 seconds; four authentic native EAB1 batches across two epochs, three captures/three refusals/six rollbacks on each canonical engine. `tmp/plan5/projection-origins2.log`. |
| `python tests/async/test_economic_sql_audit_origins.py -v` on Windows | 20 passes, two explicit native-origin skips, 0.019 seconds. `tmp/plan5/projection-host.log`. The native pair above executes the skipped cases. |
| Six changed Python files: `python -m py_compile`; unstaged/staged `git diff --check` | Pass, `tmp/plan5/projection-final-checks.log`. No C/C++ input changed, so no server rebuild or C++ formatting change is needed for this slice. |

The Docker runs use read-only `/workspace` and writable `/workspace/bin` mounts,
`duris-plan5-origin-sql-tools:local`, and only the explicit opt-in environment
named above. Exact frozen executable SHA-256s, Docker image/toolchain, native
source and migration maps are in the manifest. Syntax-check inputs are:

```sh
python -m py_compile scripts/economic_sql_audit_origins.py scripts/economic_sql_audit_snapshot.py tests/async/test_economic_sql_audit_origins.py tests/async/run_economic_sql_audit_snapshot_mysql.py tests/async/test_native_sql_baseline_audit.py tests/async/run_native_sql_baseline_audit.py
```

The first expanded run stopped at a private fixture repair predicate that also
matched the other epoch's reservation (234.728 seconds). The predicate now names
lineage and epoch. The next run passed all 60 new projection cuts on both
engines, then stopped because PyMySQL classifies CHECK errors as
`OperationalError` (252.099 seconds). The constraint probe now catches
`MySQLError` and still requires exact expected numeric codes and unchanged
authority. Both failed runs, their exact inputs and artifacts are preserved.
Core/unit/native-origin inputs remain unchanged after this final probe-only
correction; their successful results apply to the final source. Synthetic
positive fixtures now publish matching baseline details through their private
owner and use explicit old-column posting inserts. They do not qualify native
writer or release completion.

## Ownership, handoff and remaining gates

Seven owned files: `scripts/economic_sql_audit_origins.py`,
`scripts/economic_sql_audit_snapshot.py`,
`tests/async/test_economic_sql_audit_origins.py`,
`tests/async/run_economic_sql_audit_snapshot_mysql.py`,
`tests/async/run_native_sql_baseline_audit.py`,
`tests/async/test_native_sql_baseline_audit.py`, and this report. Native contracts,
mutators, migrations, decoder/reconciler, shared coordinator, registry/matrix and
activation owner remain unchanged. The manifest verifies every preceding slice
artifact and unchanged helper before binding the new commit.

The existing primary-owner release-registration handoff remains: explicitly
register the native baseline case with `DURIS_RUN_NATIVE_BASELINE_AUDIT=1`, both
engines and zero skips, plus the native-origin pair with its opt-in. Fields are
`path`, `arguments`, `environment`, `required_cases`, `provider`, `engines` and
timeout; consumers are the integration runner and combined release report.
Shared registration is outside this lane's edits.

Independent zero-effect child/item-reference/native-ledger/outbox checks and
rootless reservation inventory remain separate audit evidence requirements.
Complete native capture, namespace authority, all real writer/player journeys
and fault/restart/lost-reply matrices, populated upgrades, complete restore/clone
qualification, retention/erasure/export policy, live sweeps and measured mixed
workload budgets remain open. Two near-limit budget cases and one native-stake
SQL opt-in case are skipped here and are not requalified. Prior backup/restore
and retention evidence remains pinned to its own exact source.

The primary owner's combined candidate has not been published and qualified by
this slice. Release and activation remain blocked. Wallet-root item exclusions,
declined inactive spell behavior and active blackjack refusal are preserved.
No activation, production access, merge, deployment or audit auto-correction
occurred.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable; the earlier request is unanswered. Bounded accessible Pages
searches returned no target and do not establish absence. This report and
protected manifest are a curator-ready handoff, not a claimed notebook update.
