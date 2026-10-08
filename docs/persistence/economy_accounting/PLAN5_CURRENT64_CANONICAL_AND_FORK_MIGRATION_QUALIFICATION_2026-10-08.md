# Plan 5 current64 canonical and fork migration qualification — 2026-10-08

The complete unchanged original canonical/staging/master migration suite passes
on real MariaDB10.11.14 and MySQL8.0.46 at published canonical64. All six schema
profiles reach64, preserve their registered receipts and converge to the pinned
metadata fingerprint for their engine. Original reruns, duplicate refusals,
session/lock/receipt faults, tamper controls and actual compiled boot predicates
pass. This qualifies the recorded original schema/fork suite. Captured historical
dumps, retained economic-root upgrades, full service/player recovery, the private
combined implementation and full Plan5/R1–R8/release remain unqualified here.

## Branch, ownership and source

Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Local and sole publication branch: `codex/accounting-plan5`.
Base: `14096e6dd04509d92c632d17caf400dc1483af22`. The containing report commit is the result; the
post-push `delivery/result.json` binds its exact local/remote SHA and all seven
earlier branch tips as ancestors. Existing owned code and worktrees are preserved.

Only this report and the additive remote follow-up change. No executable source,
original test, schema, migration, coordinator, producer, registry/matrix or
activation file changes. No new interface/schema fields or shared code application
are requested. The prior flat fixture/auction provider and original room seed
handoffs remain open with their previously recorded evidence.

| Executed source | Identity |
| --- | --- |
| Published primary | `58c8e89e6a642dcc4aec1528cba99c50b99816fb` |
| Complete published tree; no owned overlays | `3316f9e9b26439e4c2bd474441cebc867e4c2d87` |
| Native tree | `833d3085815b396861ad18a77635412212381e4b` |
| Migration tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Archive SHA256 | `75543251d8029f885cc6b32189283fcfa2d2df73edda7ccd9cfac75b9c1ad20b` |
| Canonical and registered fork heads | `64 / 0064_auction_custody_history` |

The complete archive authenticates6497 regular bodies and four symlink targets
against Git blob identity and exact canonical archive modes. Both terminal guards
rehash every frozen regular body, check its native mode and verify every source
link target. Original fixtures and compiled contract/header bodies remain exact.
`AI_CONTEXT.md` is absent at this published revision. Required plan, requirements
and latest checkpoint bytes are retained. The refresh before publication still
names the same primary/native/migration source.

The current checkpoint's private128-production-file original flat attempt/pet
candidate has SHA256
`bd4d4a56c71cbabe2ecc8b56880e2046a856905f458cfdfb164ea4409a255ffe`.
It is not included or executed. Its source acceptance and major-plan deferral
remain separate from these published-source results.

## Original commands and actual execution

The original `tests/async/test_staging_migration_fork_mysql.py` already provides
an explicitly guarded disposable loopback transport. Invoke its complete original
entry function without changing any tests:

```python
qualification.run(update=False, lock_only=False,
                  loopback_engine="mariadb10_11", master_bootstrap=None)
qualification.run(update=False, lock_only=False,
                  loopback_engine="mysql8", master_bootstrap=None)
```

These are the same complete calls selected by the original
`--disposable-loopback mariadb10_11` / `--disposable-loopback mysql8` CLI options.
There is no `--update-contract` or lock-only shortcut. The current original
bootstrap is intentionally the test's default for the master31 prefix; no captured
historical master bootstrap is supplied or qualified.

Exact host commands, in the stated worktree:

```text
python -X utf8 D:/Dev/Temp/accounting-plan5-current64-upgrade/launch.py mariadb01
python -X utf8 D:/Dev/Temp/accounting-plan5-current64-upgrade/launch.py mysql01
```

Each observer starts a new task-private daemon/datadir using the existing restore
helper, then grants the original fixture owner privileges on that disposable
instance. The only daemon-argument adaptation enables a validated private
127.0.0.1 port inside the network-disabled container; MySQL also retains the
original compatible authentication choice. The admin setup connection closes
before the original quiescence/session tests. No existing socket, environment,
service, account/player data or production mount is used.

The unchanged suite invokes the actual `scripts/migration_runner.py` and original
migration/verifier bodies. Runtime checks invoke exactly:

```text
bash migrations/verify_runtime_compatibility.sh --schema-only
python3 tests/async/runtime_migration_history_fixture.py
python3 tests/async/test_staging_migration_fork_mysql.py --native-lock-fixture
```

The second command exercises the restore's complete-history selector and compiles
the original selected predicates from `src/sql/sql.c`, using the actual
`src/core/runtime_compatibility_contract.h`. Its original C++20/mysql_config/crypto
compile recipe and digest cache remain unchanged. Generated C++ and binary bodies
are retained under the per-job D: build directory. The predicates cover initial
baseline/head/state/table identity, full migration history, metadata fingerprint
and exact extra-description generated expressions. This does not boot the full
server or check character accounting baselines.

Pinned tools image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Actual GCC13.3.0, Python3.12.3, GNU nm2.42 and mysql_config10.11.14 hashes/versions
are retained by the native seal inventory. Each independent job has2 CPUs/5GiB,
network none, new native RAM-backed source/database scratch and direct D: evidence
and build mounts. The original fixture cache path is a separate direct D: mount
for each job. Regression build cache is off; original source/fixture budgets stay.
The declared outer job ceiling is2400 seconds. No fresh full-server Make build is
claimed for this documentation-only qualification slice.

## Results and preserved invariants

| Engine | Result | Elapsed seconds | Observed commands / launches | Measured normalized metadata fingerprint |
| --- | --- | ---: | ---: | --- |
| MariaDB10.11.14-MariaDB-0ubuntu0.24.04.1 | PASS, exit0 | 93.466087 | 271 / 272 | `9ec7e9c0c109eaf2a7378fbd94159acc51bf962fcc308f9dcf6fc6fec8dbd32a` |
| MySQL8.0.46-0ubuntu0.22.04.4 | PASS, exit0 | 78.156727 | 275 / 276 | `35784bb506230cad05a36d6fe01ab67635acefb8375d03c1c45050f5b378d31b` |

Each engine executes three complete registered profiles:

- Canonical fresh/bootstrap:64 actual registered migrations; the original rerun
  leaves all migration receipts unchanged.
- Staging fork: authentic registered first45 receipts and physical prefix, then
 19 appended registered steps; the first45 receipts and protected description
 rows remain unchanged. Canonical history selection refuses this fork before
 the registered transition; transition rerun preserves the full64-row history.
- Master fork: original first31 registered receipts, then33 appended steps;
  first31 receipts and the deliberately seeded runtime payload remain unchanged.
  Premature boot and canonical selection refuse the prefix, and the registered
  transition and its rerun reach the complete64-row history.

Retained final schema cuts independently record all64 receipts, stored framed
history state, table names and runtime-payload rows for all six profiles. Each
engine also retains the original duplicate and native-lock fault schemas. The
three accepted framed history digests match both registered contract and actual
stored migration state on both engines:

| Profile | Framed/stored history SHA256 |
| --- | --- |
| Canonical | `8dbe4e1771a71d2eff1d496990d7afcf0a55e8fab932c59838ace4c8eb806fe2` |
| Staging0045 | `b2d023f5656a24b152aad61c2cb182fc75922d40eb8d0cc154c81ea7676f78e7` |
| Master0031 | `07b6774ac624152e0c8b850f884393632d1b933e4485e3658b8da9baddc0c8a2` |

Per engine, the shell predicate records6 accepts and10 deliberate refusals; the
compiled/restore-history fixture records the same6/10 outcomes. Total across both:
12 accepts/20 refusals for each consumer,32 shell invocations and32 compiled
fixture invocations. All observed negative outcomes have exit1; healthy outcomes
have exit0. No explicit skips or missed required original cases are accepted.
Old receipt tamper, mixed framed state and changed generated expressions refuse;
restoring each original fixture returns to acceptance. Duplicate complete values
refuse before permanent DDL and preserve the original evidence rows.

Original native session tests prove the same actual connection/advisory lock
across apply, verify and receipt; concurrent-runner exclusion; quiescence on that
session; receipt/head-CAS rollback; killed-session transaction rollback without
reconnect; allocation/cancellation cleanup; SQL-error, output-size and timeout
fencing. These are the original real-database fault cases and their declared
fault injections. No runner logic, assertions or failure expectations change.

## Evidence, handoff and remaining gates

Raw evidence: `D:/Dev/Tests/Duris/accounting-plan5/current64-upgrade-20261008`.
Builds: `D:/Dev/Builds/Duris/accounting-plan5-current64-upgrade-20261008`.
Helpers: `D:/Dev/Temp/accounting-plan5-current64-upgrade`.

`source01.json` and `source-transport-authentication.json` bind all frozen inputs.
`qualification.json` binds counts and final schema profiles. Both job directories
retain actual launcher/Docker argv, every observed parent command/launch and exit,
original report, terminal guard, console/database logs, final receipt/state/payload
cuts, and disposable regular files copied before cleanup. Native lstat/mode/link
metadata is recorded before regular-only Windows copies. The generated predicate
C++ and binaries remain on the direct D: build mounts; the original digest cache
is not presented as multiple fresh compiles.

Raw seal `seal/evidence.json`, SHA256 `feb2b96d7a9bd73aa0b82b0795676ebfbae6c11bebd00b35e2415d5280a06d8b`, authenticates
2658 files/828670844 bytes, six build regular files, zero copied links/reparse
points and three stopped isolated containers. The post-push delivery receipt
rehashes every sealed entry and authenticates the exact remote result, clean
worktree, all seven earlier branch tips and the final primary source refresh.
It is created after this report commit, so its own SHA/result are reported
separately. All prior retention fixes and complete owned controls remain on the
same branch.

No new shared interface request or slice-specific blocker was found. Full R8
still requires captured historical master/legacy database qualification and the
retained nonempty economic EAB1/schema56-to64 upgrade, original legacy shell
migration path, current combined backup/restore/recovery and actual complete
service/player journeys. The original suite's default bootstrap, deliberately
seeded runtime payload and description rows do not establish those historical
or gameplay results. The native compiled predicates also do not establish full
server startup, character baselines or active writer/opening correspondence.

Current flat retention remains blocked at the previously published shared auction
fixture/provider boundary; original room seed retains its separate native UID/
provider handoff. Complete active holdings/UID audit, genuine producer/replay/
lost-reply/player faults, measured workload budgets, private major-plan source
qualification and published combined candidate remain required. Full Plan5/R1–R8,
release and activation are unproven. Inactive behavior, wallet-root ITEM_MONEY
exclusions and the declined inactive spell change remain unchanged. No activation,
production mutation, audit autocorrection, deployment, PR merge or primary-branch
push occurred.

The shared notebook is primary-local and nonblocking per the user. This report
and additive remote follow-up form the curator-ready handoff; no notebook
application/adoption or primary acknowledgement is claimed.
