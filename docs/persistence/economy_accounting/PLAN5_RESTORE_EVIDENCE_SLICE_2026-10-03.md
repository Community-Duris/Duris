# Plan 5: retained economic evidence in SQL restore qualification

Delivery branch: `codex/accounting-plan5` on Community-Duris/Duris.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `f2a110c4ddf649a8ab7479ea0797dc650ba43b9a`, the published
provenance slice on canonical `7d2f8e8153f637c38e19054cf1202436d3f9a28a`.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/restore-evidence.json` record its exact SHA after commit.
No push to `experimental-accounting`, activation, deployment or data repair is
part of this slice. Plans 1–4 and shared contracts remain with the primary owner.

## Established defect and change

The actual `qualify_database_restore.main()` accepted a restored root declaring
two postings after one posting was deleted. Existing native wallet/bank/epic
revision and value checks still passed. This RED was reproduced independently
on MySQL 8.0.46 and MariaDB 10.11.14 after fresh canonical migration 0056.

Qualification now independently checks retained rows across all epochs using
SELECT statements. It requires declared counts and contiguous indexes for the
four detail families, durable matching root receipts, exact source claims when
a successful root retains a source, posting/account and child links, durable
provided child receipts, exact native item-ledger/receipt attribution and
one-step item revisions. Wide decimal arithmetic checks denomination-weighted
posting values, root conservation and per-account posting/effect agreement.
Empty inactive histories and durable rejected roots with no effects pass.
The qualification success output and connection boundary remain unchanged.

This does not decode canonical intent/plan blobs, infer missing roots from
external authority, enforce policy-required absent sources, or qualify native
custody, opening/epoch/allocator authority, publication recovery or activation.
These remain independent required gates. The qualifier neither auto-corrects
findings nor updates pause/activation authority.

## Owned files and interface handoff

- `scripts/qualify_database_restore.py`: independent retained-row gate, called
  before qualification succeeds.
- `tests/async/run_restore_accounting_evidence_mysql.py`: fresh disposable SQL
  corruption fixtures invoking the actual main entry with SELECT-only credentials.
- `tests/async/run_restore_native_shop_money_mysql.py`: also applies this gate
  to the existing genuine native shop transaction fixture.
- `docs/operations/BACKUPS.md`: operator behavior and limits.
- This report.

No schema or accounting-field interface request is needed. The native shop
runner currently fails linking three existing item-transfer dependencies:
`sql_room_item_payload_lock_season`, `sql_room_item_payload_prepare` and
`sql_room_item_payload_record`. Narrow primary-owner handoff: include existing
`src/persistence/sql_room_item_payload.c` in `root_sources` of
`tests/async/run_shop_trade_sql_lock_mysql.py`, with its already linked player
snapshot codec. This changes the harness linkage, not fields, invariants or
producer behavior. Consumer: native shop/restore shop fixture and any runner
with that same missing closure. Required tests: strict native shop transaction
and restore shop qualification on the integrated source, followed by applicable
central registration checks. This shared runner was not edited here.
Register the new corruption runner only with its owned fresh daemon/socket
setup; it intentionally refuses a shared existing `duris_restore` database.
Central test registration remains a primary-owner change.

## Exact tested source

Native sources are unchanged from the slice base. Canonical fresh bootstrap,
manifest and migration 0056 are used, not the earlier 0055 evidence.
SHA-256 pins:

| File | SHA-256 |
| --- | --- |
| `scripts/qualify_database_restore.py` | `5e23faf448a88803d8c11fe60a01d0fb9e23a88d62114e407a309d5baeec576a` |
| `tests/async/run_restore_accounting_evidence_mysql.py` | `4999633faf5f16c17917151919dcfa4ec29a7832ad4241553808fa20e7845fb3` |
| `tests/async/run_restore_native_shop_money_mysql.py` | `860c3ae1e7c7d78b2f47acd0c5f0b4b055b10362569eb6541057b3c4265cca8e` |
| `migrations/migration_manifest.json` | `eb946a7a108faf9a9a39bd337edde308a29c9e4332133fb1828662addaf66889` |
| `migrations/bootstrap_multithread_safe.sql` | `aba8628d87381afad609a3d362380576f09e753a8a7b501ecdba38c9f30f6982` |
| `migrations/immutable/0056_spell_ward_durability.sql` | `e53e06ca7a5b482b6efee950a4db69fdb7d796b7ff71ed495e67bd3194133bb1` |
| `migrations/immutable/0056_spell_ward_durability.sh` | `e3fbff0b062da55fc7231411beefe8be216341755df7fa088ba64b1f86653551` |

## Reproduction and evidence

Raw output and disposable-only drivers are retained under ignored `tmp/plan5/`.
No project `.env`, production credentials, existing player database or live
game connection is used. Fixture setup refuses an existing database or reader
identity. Corruption and repair happen only in the fixture-owned schema; the
auditor has only SELECT privileges. Exact pre/post captures prove successful
checks and refusals leave the captured authority/evidence unchanged.

The structural SQL fixtures contain synthetic identities and placeholder
canonical intent/plan blobs; their acceptance proves this gate only. They are
not native gameplay envelopes or full restore/release certification. There are
26 deliberate corruption cases, intact/inactive/rejected admission cases and
a full unsigned-64-bit item revision case. Native money checks still pass in
each deliberately damaged retained-evidence case.

From the worktree, run the following for each `$engine` of `mysql`, `mariadb`
against its newly provisioned task-owned container/socket volume:

```powershell
$taskWorktree = 'C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max'
$taskPymysql = 'C:\Users\alexa\AppData\Roaming\Python\Python312\site-packages\pymysql'
docker run --rm --network "container:codex-plan5-restore-$engine-01a104cf" `
  --mount "type=bind,source=$taskWorktree,target=/workspace,readonly" `
  --mount "type=bind,source=$taskPymysql,target=/opt/python/pymysql,readonly" `
  --mount "type=volume,source=codex-plan5-restore-$engine-socket-01a104cf,target=/plan5-restore-$engine,readonly" `
  --env PYTHONPATH=/opt/python --env TEST_DB_DISPOSABLE=1 --env ENVIRONMENT=test `
  --env DB_HOST=127.0.0.1 --env DB_NAME=duris_restore `
  --env "DB_SOCKET=/plan5-restore-$engine/mysqld.sock" --env DB_USER=root `
  --env DB_PASSWD=plan5-disposable-only --entrypoint python3 `
  duris-accounting-restore-tools:local `
  /workspace/tests/async/run_restore_accounting_evidence_mysql.py
```

DB images are MySQL `sha256:7dcddc01f13bab2f15cde676d44d01f61fc9f99fe7785e86196dfc07d358ae2b`
and MariaDB `sha256:dbe56e20372fc6d6b8e0e396866ba89c4c7f128c38c4f59aaa54d957db95790c`.
The MySQL daemon uses `--innodb-use-native-aio=0` and
`--default-authentication-plugin=mysql_native_password`.
The tools image is
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`,
with Python 3.12.3 and mounted PyMySQL. These are functional checks, with no
latency, load, disk-growth, retention-horizon or native-AIO performance claim.
Fixture bootstrap timeout is 180 seconds. Existing client limits remain
120 seconds / 8 MiB per qualifier SQL command and 30 seconds per fixture reader
query; this is not an established release workload budget.

Original RED logs: `restore-evidence-mysql-red3.log` and
`restore-evidence-mariadb-red3.log` (actual main falsely qualifies missing posting).
Earlier setup-only failures remain in `red.log`, `red2.log` and `green1.log`
for each backend: missing CLI in the first tools image, synthetic player name,
60-second bootstrap timeout, and a fixture metadata-table name correction.

The existing native shop fixture additionally creates its own synthetic active
epoch in a separate disposable schema and invokes actual C++ transaction code.
It never calls the maintenance activation owner or changes configured authority.
Its executable is compiled using the existing strict flags plus the existing
room-payload source through this ignored explicit compiler wrapper:

```sh
#!/bin/sh
exec g++ /workspace/src/persistence/sql_room_item_payload.c "$@"
```

`tmp/plan5/run-native-shop-restore.py` guards a fresh schema
`economic_schema_test_plan5_restore_shop_01a104cf`, imports the canonical fresh
bootstrap, adopts it and applies through 0056 using the native migration client.
It then removes `DB_SOCKET` from the native harness environment (an empty
variable is deliberately insufficient for its guard), sets `DB_PORT=3306`,
`TEST_DB_DISPOSABLE=1` and `ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1`, and runs:

```sh
CXX=/workspace/tmp/plan5/cxx-shop-with-room-payload.sh \
  python3 /workspace/tests/async/run_restore_native_shop_money_mysql.py
```

Use the same tools/PyMySQL/source/socket mounts as the corruption command,
add a writable `bin/` mount for the compiled artifact, and use the tools
container inside the selected owned DB container's network namespace.
The fixture-owned schema is dropped in a `finally` block. Compiler is GCC
13.3.0; flags remain C++20, `-Wall -Wextra -Wpedantic -Werror -O1 -g`, function
sections / garbage collection, MariaDB client flags and OpenSSL. The original
runner's link failure is not converted to a pass: only the explicit-source
invocation qualifies this native component.

| Command | Result and evidence under `tmp/plan5/` |
| --- | --- |
| `python3 tests/async/run_restore_accounting_evidence_mysql.py` (actual main, MySQL 8.0.46) | PASS; `restore-evidence-mysql-green2.log`, 26 damaged-evidence refusals and intact/inactive/rejected/full-uint64 checks. |
| Same command (MariaDB 10.11.14) | PASS; `restore-evidence-mariadb-green2.log`, same cases and unchanged-row proof. |
| `python3 tests/async/run_restore_native_shop_money_mysql.py` (MariaDB, explicit source above) | PASS; `restore-native-shop-mariadb-explicit-room-source2.log`: actual committed economic-only buy/root/receipt/original UID, new integrity gate, exact money histories, bridge dedupe/conflict, opening cuts and corruption refusal. Native executable SHA-256 `337e68654dc690c063527d520d9f53b1e704bad2e97720111307f6b8014ab67d`. |
| Native shop check (MySQL 8.0.46, same explicit source) | PASS; `restore-native-shop-mysql-explicit-room-source.log`, same genuine purchase and restore checks. |
| `python3 tests/async/test_persistence_backup.py` | PASS, 40 tests; `test_persistence_backup-restore.log`. |
| `python3 tests/async/test_backup_review_remediations.py` | PASS, 17 tests; `test_backup_review_remediations-restore.log`. |
| `python3 tests/async/test_immutable_migration_runner.py` | PASS, 24 tests; `test_immutable_migration_runner-restore.log`. |
| `python3 tests/async/test_backup_pfiles.py` | PASS, 2 tests; `test_backup_pfiles-restore.log`. |
| `python scripts/validate_economy_accounting.py` (Windows Python 3.12.10) | PASS: 14 fixtures, 868 routes, 2818 sites, `release_ready=False`; `restore-contract.log`. |
| `python -m py_compile scripts/qualify_database_restore.py tests/async/run_restore_accounting_evidence_mysql.py tests/async/run_restore_native_shop_money_mysql.py` | PASS, Windows Python 3.12.10. |
| `git diff --check` | PASS. |

The four existing Python suites ran in the same Linux tools image using
`docker run --rm --mount type=bind,source=<worktree>,target=/workspace,readonly
--entrypoint python3 duris-accounting-restore-tools:local
/workspace/tests/async/<test>`. They are component tests, not full integration.
Native setup failures are retained in `restore-native-shop-mariadb.log`
(unmodified link closure) and
`restore-native-shop-mariadb-explicit-room-source.log` (driver left an empty
socket environment variable). The latter was corrected in the disposable
driver before the successful native run; no native source changed.

## Remaining gates and notebook

The primary owner must publish a pinned combined candidate with canonical 0056,
current writer evidence and registry/matrix anchors. Earlier isolated passes do
not qualify that candidate. Fresh/upgrade native predicates, both maintained
builds, complete SQL and flatfile dump/import/replay/restores, actual player
journeys, fault/restart/ambiguous-reply behavior, policy and retention decisions,
and measured budgets remain required for release.
No maintained-server build was rerun because no C/C++ source changed; the
strict focused executable above is component proof. Full backup integration
(private dump/import, journal recovery and matching server boot) and upgraded
databases were not run in this slice. No flatfile restore/retention, actual
player journey, load-budget or release completion is claimed. The writer matrix
and release-evidence failures from the preceding provenance handoff remain
primary-owner integration gates; their relevant inputs were not edited here.

`AI_CONTEXT.md` and the notebook curator workflow/reference are absent from the
available checkout and searched workspace. The pending user request asks for
their location and the primary task identity. This committed source handoff is
not a curator update. No notebook completion is claimed.
