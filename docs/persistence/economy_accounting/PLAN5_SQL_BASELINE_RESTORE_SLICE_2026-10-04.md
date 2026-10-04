# Plan 5: independently qualify retained SQL baseline books

This slice starts at `bbb72ecffa2101f751f00f23f34ded89d82fe2a2` on
`codex/accounting-plan5`, worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
The result commit and committed input hashes are bound by
`tmp/plan5/sql-baseline-restore-evidence.json`. Publish this lane's branch for
primary-owner integration; do not push directly to experimental-accounting.

The refreshed integration remote is
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native canonical base is
`7d2f8e8153f637c38e19054cf1202436d3f9a28a`, native tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`. All 1,232 native inputs and
236 migration inputs remain unchanged. Both disposable engines use canonical
migrations through `0056_spell_ward_durability`. This source does not qualify
the primary owner's unpublished combined candidate.

## Established defect and independent fix

The SQL restore qualifier checked canonical EAI1/EAP1 roots and their normalized
evidence, but did not bind opening roots to retained EAB1 witnesses, baseline
heads or reservations. Altering a witness digest left the root and every generic
projection intact and passed the actual full qualifier on both canonical engines.
The RED ran one guarded native test, two engine failures, zero skips, in 216.065
seconds. It used genuine native SQL baseline publication, not synthetic capsule
encoding. Canonical constraints remained enabled. Frozen source and complete
failed artifacts are preserved.

The existing independent `economic_restore_evidence.require_integrity` now also
qualifies the entire retained baseline namespace. It issues SELECT only and
imports no native mutation, storage, coordinator, recovery or producer code.
The unchanged origin verifier supplies its pure EAB1 interpretation and canonical
root regeneration to a second consumer. Its local import avoids the existing
decoder dependency cycle; no second wire decoder is introduced.

The restore reader:

- Requires the three baseline tables to exist as InnoDB sources. It rejects
  witnesses without their exact root/book, successful baseline roots without
  witnesses, and reservations without an exact composite witness match.
- Checks every retained book's epoch/lineage membership, allowed initialization
  receipt state, nonzero IDs, canonical lineage/kind-9 opening key, dense unique
  witness revisions, head revision and exact terminal operation. Initialized
  empty inactive books remain valid with revision zero and no terminal witness.
- Binds each EAB1 digest, size, header, metadata, opening, source/boundary digests,
  numeric holding/UID order and topology to its independently regenerated EAI1/
  EAP1 root and native baseline receipt fields. Existing full-qualifier receipt
  and source-claim checks remain in force.
- Requires the exact per-witness reservation set, including full unsigned IDs
  and claimed scopes. Per-operation reads deliberately include foreign claimed
  books, and database-wide anti-joins catch detached unknown-book reservations.
  Duplicate natural identities are rejected.
- Forbids baseline children, use as another root's child, item references, native
  currency/ownership ledger effects and outbox rows, regardless of status.

New refusal codes are `restore_economic_baseline_source_mismatch`,
`restore_economic_baseline_witness_mismatch`,
`restore_economic_baseline_reservation_mismatch`,
`restore_economic_baseline_book_mismatch` and
`restore_economic_baseline_zero_effect_mismatch`. Existing generic refusals may
occur first when the same damage violates a root or normalized-row invariant.
No error is converted to an adjustment or all-clear.

The existing bounded capsule reader now serves both EAI1/EAP1 and raw EAB1.
It reads 65,536-byte HEX chunks; EAB1 is bounded at 872,144 bytes and a 192-byte
minimum. Root enumeration remains in 256-ID pages. Each reservation query has
an explicit expected-count-plus-one limit, at most 9,072 rows. It retains only
one witness's origins/reservations at a time. These bounds and quiescent ID
pagination do not establish a live watermark or measured workload budgets.

No canonical schema, wire format, accounting contract, shared coordinator,
producer, registry/matrix or activation-owner file changes. All pure decoder
function ASTs in `economic_restore_evidence.py` match the base; only its SQL
qualification function changes. `qualify_database_restore.py` and the origin
verifier remain unchanged.

## Native, restore and compatibility evidence

The pinned tool image is `duris-plan5-origin-sql-tools:local`, ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Toolchain: Ubuntu 24.04, Python 3.12.3, GCC 13.3 and PyMySQL 1.0.2-2ubuntu1.1.
Engines: MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` and MySQL
`8.0.46-0ubuntu0.22.04.4`. The checkout is mounted read-only at `/workspace`,
with only `bin/` separately writable. Private fresh daemons use Unix sockets
with TCP disabled and a clean disposable environment without checkout `.env`.
The strict production-profile SQL server build passes at the unchanged native
tree. Its isolated 174,463,976-byte executable has SHA-256
`b8393c48e6180ae98b9e1bfed51aa4fbf69e1d7402ec9dbc007d68b98675b0aa`.
The previous `bin/server/dms_new` remains byte-identical. This build does not
establish a gameplay journey or requalify the flatfile server profile.

The native baseline fixture has 13 unchanged C++20 inputs, strict warnings,
ASan and UBSan. Its SQL binary SHA-256 remains
`7985d95effd41456d445a5924e466a5ae5ad25fa9506d141d2ff1dd1168c4ee2`;
client-free remains
`efaee7f33a1ebc38fe9b6afe9baaa32b66b8b3db1746679dc58c05cf39a9b027`.
The fixture publishes two nonempty baseline books, exactly replays and
reconciles them, and keeps `active_epoch` NULL. Client-free initialize/apply/
reconcile retain `ENOTSUP` refusal.

The existing native audit matrix additionally invokes the independent economic
restore gate for every corruption cut through a SELECT-only reader's consistent
read-only transaction. Both paths preserve all 18 captured tables. Failure is
required to carry a specific `restore_economic_` code; SQL/transport exceptions
are not accepted as semantic refusals. Existing audit findings/capture refusals,
seven canonical constraint refusals and explicit damaged-import labels remain
required. The original digest corruption also invokes the real full qualifier,
as do intact controls and the empty-book opening checks.
The successful native run collected one guarded test, passed both engines with
zero skips, and took 437.922 seconds. Each engine requires all 145 restore
refusals alongside the original 65 capture refusals and 80 diagnostic cuts;
17 cuts are explicitly damaged imports and seven violations are refused by
canonical SQL constraints before an audit.

The cold-clone check dumps the intact private candidate with `mysqldump`
single-transaction, no lock tables, HEX blobs, routines, triggers and events.
It imports into a newly initialized private daemon of the same engine, through
the existing schema-only importer. The actual full restore CLI must pass there,
all 18 captured tables must equal the original, and the genuine native owner
must produce the exact same replay result without changing a row. Dumps remain
protected generated artifacts. This proves two-book baseline continuity across
dump/import and cold native replay; it does not establish a full-world restore,
service/login journey, independent production backups or production reopening.
The MariaDB dump is 332,977 bytes, SHA-256
`f00b13ba505f9ab1e4d42a245768352c6c472d364ec1bf87238044ae8b2d3e14`;
MySQL is 341,737 bytes, SHA-256
`cbd0cfbb8522336a82dd8f0d51e364996c876b1b0ba77ddc07b5d2735876201c`.

The sibling exporter/operator matrix retains its source grammar/policy,
original-link, orphan, unsigned money/UID, late lower-ID commit, interrupted-cut,
provenance, global-status and exact-lookup checks. The native canonical fixture
checks both SQL and `__NO_MYSQL__` decoder decisions and the existing canonical
restore/coin matrices. Its default and decoder corpus remain unchanged.

The first compatibility attempt established a fixture defect: its generic SQL
history used a reason-38 root without a retained baseline book or EAB1, and its
compound variant carried mutation children/custody effects forbidden for a
baseline. Both engines correctly refused it after this fix; the attempt took
340.248 seconds with two failures and zero skips. The 3,026 native decoder
decisions had already passed. The fixture now has a separate native-encoded
`--restore-history` mode using reason-3 transfer metadata and coherent bank/
wallet effects. The SQL worker derives its metadata and effects from those
native bytes. The actor-kind corruption uses a value different from that new
control. Original decoder/coin oracle bytes and semantics remain pinned.
The second attempt reached the forged-projection case and failed on both engines
in 779.057 seconds. That cut still relied on the opening account's balance
exemption; with a bank transfer, its before/after vector no longer matched the
forged posting. The repaired cut forges both sides coherently, preserving generic
balance checks so canonical-byte binding is the refusal under test. A separate
native maximum-size transfer intent preserves the 8,192-byte admitted control
without borrowing the old baseline metadata. Both failed attempts retain frozen
inputs and separate artifact copies.

The corrected compatibility run passes one guarded test, both canonical engines,
zero skips, in 896.482 seconds. Both SQL and `__NO_MYSQL__` binaries have SHA-256
`faf3038738747058d6d1273c31cb986351e3ae34a768d92c7ad6190cd8565edc`.
All 3,026 native decoder decisions agree independently (1,054 accepted and 1,972
refused), retaining corpus SHA-256
`c2c3954d21d8a56defe59447b844af1a412670f506f4f03d3b0670f0ba526a71`.
Original framed default output remains
`0196c2e6489287091dbe742e10dc6883b5208b79f58f267fae66139714ea5b54`.
Each engine passes 39 canonical cuts (36 refusals, three admitted controls),
six full-entry cases, the 259-root two-page control, 26 earlier corruption cases
plus the initial full-entry intent refusal, and 32 coin oracles (30 audited,
two explicit constraint refusals). The forged rows reach
`restore_economic_canonical_account_mismatch`; the native maximum-size intent
passes. These are structural/component cuts, not real economic writers.

**Diagnostic correction:** the original `NATIVE_RESTORE_HISTORY` output
hard-coded `ordinary_reason: 1`. The native `coin_transfer` enum, EAI1/EAP1 bytes
and seeded SQL metadata are all reason 3; the capsule and SQL checks above passed
against those actual values. The diagnostic now reads the native plan's reason.
`tmp/plan5/restore-reason-diagnostic-evidence.json` binds the correction to an
isolated check of the exact previously qualified native artifacts. Original logs,
manifests and bytes remain preserved. The full native/database results above
remain pinned to their original source; this display correction does not claim
a new full compatibility run.

That first attempt reused the older canonical artifact directory and replaced
its two engine logs. The exact original logs were recovered from the protected
`canonical-green2.log` transcript, verified against their previously recorded
SHA-256 values, and restored before preservation checks. The failed attempt is
copied separately. New compatibility outputs use
`bin/tests/plan5-sql-baseline-restore-canonical`; no prior evidence is relabeled
as evidence for this candidate.

| Exact command | Evidence |
| --- | --- |
| `DURIS_RUN_NATIVE_BASELINE_AUDIT=1 python3 -u tests/async/test_native_sql_baseline_audit.py -v` at `/workspace` | `tmp/plan5/baseline-restore-green.log`, `bin/tests/plan5-baseline-sql-restore/{mariadb,mysql}.log`; both canonical engines, full-entry controls/refusals, paired audit/restore matrix, native replay and cold dump/import. |
| `DURIS_RUN_RESTORE_COIN_INTEGRATION=1 DURIS_PLAN5_CANONICAL_EVIDENCE=1 python3 -u tests/async/test_restore_economic_coin_effects.py -v` at `/workspace` | `tmp/plan5/baseline-restore-canonical-green3.log`; native decoder parity, generic canonical/coin restore compatibility on both engines. |
| `python3 -m unittest test_reconcile_economy_accounting test_economic_sql_audit_origins -v` at `/workspace/tests/async` | 124 collected, 119 passed, five explicit skips, 14.576 seconds. `tmp/plan5/baseline-restore-components.log`. |
| `make -C src BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb DMS_BINARY=/workspace/bin/server/plan5-baseline-restore-dms_new -j2` | `tmp/plan5/baseline-restore-make.log`; isolated server binary, original executable preserved. |
| `clang-format --style=file -lines=250:270 -i tests/async/restore_coin_effects_fixture.cpp`, then `-lines=317:333`; `bash scripts/format.sh --check --file tests/async/restore_coin_effects_fixture.cpp` | `tmp/plan5/baseline-restore-format-final.log`; changed lines formatted, entire touched fixture passes clang-format 18.1.3 using existing `duris-regression-tools:review-v23`. The SQL tool image has no formatter; that failed discovery is recorded separately. |
| `python -m py_compile scripts/economic_restore_evidence.py tests/async/run_native_sql_baseline_audit.py tests/async/test_native_sql_baseline_audit.py tests/async/run_restore_accounting_evidence_mysql.py tests/async/test_restore_economic_coin_effects.py`; `git diff --check`; `git diff --cached --check` | `tmp/plan5/baseline-restore-final-checks.log`. |

Native baseline inputs were frozen before its successful check and remain
unchanged. Fourteen executable/helper inputs are frozen separately before the
corrected compatibility run, including its fixture and SQL worker. The manifest
pins their exact bytes and command scope, unchanged native/migration/helper
maps, tool images, native binaries, RED, successful commands, dump hashes and
committed result. Prior slice artifacts remain byte-identical. Generated binaries, logs,
database dumps and evidence JSON remain uncommitted under `bin/` and `tmp/plan5/`.

## Ownership, handoff and remaining gates

Seven owned files: `scripts/economic_restore_evidence.py`,
`tests/async/run_native_sql_baseline_audit.py`,
`tests/async/test_native_sql_baseline_audit.py`,
`tests/async/restore_coin_effects_fixture.cpp`,
`tests/async/run_restore_accounting_evidence_mysql.py`,
`tests/async/test_restore_economic_coin_effects.py` and this report.
No new shared interface is required for binding retained nonempty SQL books.
The primary-owned integration runner must explicitly register the native case
with `DURIS_RUN_NATIVE_BASELINE_AUDIT=1`, both engines and zero skips, and the
canonical compatibility case with both opt-ins above. Registration fields are
`path`, `arguments`, `environment`, `required_cases`, `provider`, `engines` and
timeout; consumers are the integration runner and combined release report.
Provider: existing `scripts/persistence_restore.py:private_database` with the
pinned SQL image and fresh socket-only daemons. Existing per-engine worker
timeouts remain 1,200 seconds for the native baseline case and 1,800 seconds for
canonical compatibility. The primary must allow both workers, native builds and
daemon startup when allocating the overall case timeout; these are test guards,
not economic latency budgets.

**Shared initialization-evidence request:** complete removal of an initialized
empty SQL book is indistinguishable from a never-initialized epoch. A head that
is itself lost cannot prove prior initialization. Preserve the earlier flatfile
request and provide independently retained initialization evidence for SQL too:
per retained epoch, `baseline_initialization_operation_id` (nonzero 16 bytes iff
initialized) and `baseline_opening_account_key` (exact 40-byte kind-9 key, absent
iff never initialized), with version/encoding allocation owned by the primary.
Initialization must atomically bind its marker and book, retain it across
deactivation/epoch turnover, and make a marked missing book refuse restoration
and retry. Compatibility for pre-marker histories must be explicit.

Consumers: Plan 1 SQL lifecycle/epoch and baseline initialization owners,
activation/cutover owner, Plan 5 independent restore/audit and backup capture.
Concrete consumers include `src/persistence/economic_sql_baseline_transaction.c`,
`src/persistence/economic_sql_accounting_lifecycle_transaction.c`,
`src/persistence/economic_sql_activation_receipt.c`,
`src/flatfile/flatfile_accounting_baseline.c`,
`src/flatfile/flatfile_accounting_lifecycle_transaction.c`,
`scripts/economic_restore_evidence.py`, `scripts/economic_sql_audit_origins.py`,
`scripts/qualify_database_restore.py`, `scripts/persistence_backup.py` and
`scripts/persistence_restore.py`. The native/schema files remain primary-owned.
Required tests: native atomic initialization/retry, full empty-head loss,
marker/init-operation/opening tampering, retained epochs/deactivation, rollback
and changed-request refusal. This lane does not allocate a migration, alter
shared catalogs, fabricate historical markers or waive that remaining gate.

Complete native capture, namespace authority, real writer/player journeys,
fault/restart/lost-reply matrices, populated upgrades, full restore/service clone
qualification, retention/erasure/export policy, live sweeps and measured mixed
workload budgets remain open. Component and synthetic fixture passes do not
qualify the primary owner's combined candidate. Budget and native-stake skips
remain explicit; previous retention results stay pinned to their own sources.

`AI_CONTEXT.md` remains absent in both checkouts. No curator capability or
notebook/workflow reference is available; the earlier request is unanswered.
Bounded accessible Pages searches returned no target and do not prove absence.
This report and manifest supply the curator handoff, without claiming a notebook
update. Wallet-root item exclusions, declined inactive spell behavior and active
blackjack refusal remain preserved. No activation, production access, audit
auto-correction, merge or deployment occurred. Release remains unqualified.
