# Plan 5: repaired maintained SQL build and managed cold restore

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Frozen tested base is `31751b7060c2281507a4f347162b6c76b8e57ba3`, the ordinary
merge of the completed count-reader slice and refreshed primary
`abd6e32cd0ba41e0fdbcd0b2cd0ee5bbba847cd7`. The base was normally pushed to
the Plan 5 branch before qualification. Native tree is
`e018587932abef86ef3bcab9d61a6f3be3afe933`; migration tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
Result commit and verified remote SHA are supplied in delivery.

## Published primary repair and fresh maintained build

The primary's `103fd07c384ac84ba104cbbc0d90167f6b57db5f` implements the narrow
collector row-comparison repair from the preceding Plan 5 handoff. This slice
imports that published change; it makes no independent native or shared edits.
The earlier failed build, original/candidate syntax checks and all old artifacts
remain preserved. New object and executable paths prevent reuse or replacement.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  OBJDIR=/workspace/bin/tests/plan5-sql-managed-repaired/objects-sql \
  DMS_BINARY=/workspace/bin/tests/plan5-sql-managed-repaired/server-sql
```

Exit 0 in 539.8945826 seconds: 716 maintained C++20 compilation commands,
716 completed objects, zero warnings/errors, and a complete linked server.
The maintained hardening and warning flags are unchanged, including `-Werror`;
no warning suppression or diagnostic source substitute is used. Executable size
is 184,956,104 bytes; SHA-256 is
`322cbaf137e4ad43e36617dfcee765b331b6cd2ad6c7622b3139ddf7e3d688ce`.
This is a full maintained SQL build and link, closing the former compiler block
for this exact native tree. It does not qualify private incoming shop source.

## Actual disposable database and service execution

Docker image `duris-plan5-origin-sql-tools:local` is pinned to
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu 24.04.4, GCC 13.3, Python 3.12.3, OpenSSL 3.0.13,
MariaDB 10.11.14 and MySQL 8.0.46. Build and qualification use
`--network none`, a read-only `/workspace` mount and writable `/workspace/bin`.
Service qualification additionally enables `CAP_SYS_ADMIN` and unconfined
seccomp for the existing tmpfs/network-namespace isolation. Database datadirs
are new, private, and TCP-disabled; no runtime `.env` or production data is read.

```sh
python3 -u tmp/plan5/run-sql-managed-repaired.py
```

The retained driver copies and hash-verifies all 3,124 frozen inputs under
`private-checkout/`, imports the existing test module there, and selects only:

- `PersistenceRecoveryIntegration.test_mariadb_full_dump_schema_history_values_and_isolated_service_boot`.
- `PersistenceRecoveryIntegration.test_mysql_full_dump_schema_history_values_and_isolated_service_boot`.

It sets both `DURIS_RUN_BACKUP_INTEGRATION=1` and
`DURIS_RUN_MYSQL_BACKUP_INTEGRATION=1` before import and refuses skipped
invocations. The driver wraps the real backup, restore, qualifier and service
functions to retain observations; it calls their originals with the original
arguments and preserves exceptions. Every existing test assertion remains.
It does not stub qualification, SQL execution, native verification or boot.

The existing class's setup requires both server filenames. The freshly built
SQL executable is copied to its private `bin/server/dms_new`. The previously
qualified flatfile binary, SHA-256
`40f068347c35f46f5c350be96d31bb39b65dc95178c3e65a844103a70d9776eb`,
is copied and verified for that setup prerequisite; neither selected SQL test
executes it, and this slice makes no fresh flatfile build claim.

The listener-free verifier and native receipt fixture are freshly built through
the unchanged `scripts/build_restore_qualifier.py` and
`tests/async/_restore_fixture.py`, with their strict C++20 native source lists
and flags. These commands use `__NO_MYSQL__` for those helper executables;
the actual service boots use the full maintained SQL server. Helper hashes are:

- `qualify_flatfile_restore`: `c97d54e58efdcc1d62e5bc04fde083ce9eab2be18fc886d324463b4b926a15a5`.
- `persistence_restore_fixture`: `840f395eddd501e575f53b1ab4a1e2bdd6cdc7abd35b08aa543a0e91fac8692a`.

The driver exits 0 after 823.1134445 outer seconds. The two original tests take
693.072 unittest seconds; execution plus final staged-source verification takes
701.5960836429149 seconds. There are zero failures, errors or skips. On each
selected engine the source and cold-import database versions match exactly:
`10.11.14-MariaDB-0ubuntu0.24.04.1` and `8.0.46-0ubuntu0.22.04.4`, respectively.
Cold restore uses a different socket/datadir from its source.

Each engine passes 12 valid qualifier cuts and refuses eight damaged cuts:
foreign account association, unwitnessed wallet revision, unwitnessed bank
revision, a missing cancelling currency-history pair, unwitnessed epic revision,
a missing cancelling epic-history pair, changed wallet baseline and changed
migration-history checksum. The two history-pair losses preserve aggregate
values; complete history, not balance equality alone, is required. These are
deliberate disposable fixture mutations; no audit tool corrects findings.

Both backups cover the canonical0056 shape, with a 170-table bootstrap baseline
and 226 current runtime tables. Migration head is `0056_spell_ward_durability`,
sequence 56, with its actual apply/verify/history checksums retained in each
`runtime-schema.json`. Both restores preserve the exact query result
`SyntheticRestore:42:11:15:23`, original source values and generation inventories.
The native locker receipt remains byte-identical after cold restore: 355 bytes,
SHA-256 `cdaad1fb39c9264c866920ae2de2925cf57c5fed01746ebaff1317ad8707a9fa`.

Both restored services enter the actual game loop under the existing isolation
and are terminated by the original qualifier. Their boot checks take
2.153841010062024 seconds (MariaDB) and 2.901652301894501 seconds (MySQL).
Retained service-log SHA-256 values are
`cc032711ec2755f0fc0660144411bad681e6d90d651ad3ec57be38334c4300b3` and
`04ad894e3f51ea504c21fa0ae13fd4d6173c200e894aaab93c789bc9ca94475f`.
Each engine produces an actual `qualified` restore receipt. The existing SQL
persistence mode is called `mariadb-primary` for both selected database engines;
the recorded engine/version assertions establish which database ran.

## Evidence, ownership and remaining gates

Fresh evidence is in `bin/tests/plan5-sql-managed-repaired/`:
`build-sql.log`, `build-command.json`, `build-metrics.json`,
`managed-integration.log`, `managed-integration-command.json`,
`managed-results.json`, the frozen tree lists and retained driver/recorder.
`managed/mariadb/` and `managed/mysql/` each contain a complete captured
generation, its four-file inventory, restore receipt and service boot log.
The original backup inventory is compared with the retained archive bytes.
No datadir, generated key, private runtime environment or production record is
copied into that retained archive. All evidence stays ignored and uncommitted.

Sealed manifest is `tmp/plan5/sql-managed-repaired-evidence.json`, SHA-256
`29f5844e2d83508c5573e891250b2ac1e05c58bf35016cf6b04dabf4b7bd707a`.
It records 4,772 fresh artifacts, verifies all 1,490 native/migration inputs
(1,254 native plus 236 migration files) and 3,124 qualification inputs, and
retains the passing preservation check for all 30,927 prior artifacts. Every
staged tracked input also matches its frozen SHA after both tests.

`python3 scripts/validate_economy_accounting.py` exits 0: 14 fixtures,
886 writer routes and 2,843 candidate sites; `release_ready=False`.
`python3 scripts/generate_economy_writer_coverage.py --check` exits 0:
the current source and checked-in matrix agree, `coverage_complete=False`,
release `BLOCKED`. `git diff --check` passes. No unchanged failing release
acceptance run is repeated. The only owned tracked change in this qualification
slice is this report; no shared interface or schema change is requested.

These are real canonical0056 dumps, cold imports and maintained native boots of
synthetic fixture data. Money-history fixtures are constructed through SQL;
they are not actual accounting producer or gameplay journeys. The services run
with inactive accounting. The test policy has no replica configured and this
slice does not establish SQL retention pruning, production backup capture,
off-host recovery, complete native source/mapping coverage, cutover or release.
Existing flatfile retention evidence remains input-specific and preserved.
Full baseline CCM1 authentication still needs the primary-owned original
admission-time field. Remaining Plans 2–4 producers, complete native capture,
combined-candidate qualification and all applicable R1–R8 gates remain required.
Wallet-root item exclusions and the declined inactive spell path are preserved.

This is a handoff for the primary's local notebook curator. That notebook is
maintained on the primary system and does not block this work; no remote
notebook update is claimed.
