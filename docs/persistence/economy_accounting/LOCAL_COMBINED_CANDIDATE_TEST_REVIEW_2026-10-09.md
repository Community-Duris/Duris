# Local combined candidate build and SQL prerequisite review - 2026-10-09

Both fresh production builds and the existing inactive flatfile boot preflight
pass at published source `8e5be494224192709faaa04d59ea2602e3972e26`.
The two maintained SQL restore/service-boot cases execute with zero skips, but
both fail before service boot. Docker is available locally through the Windows
CLI; the remaining SQL prerequisites are concrete schema/contract failures.
These are development checkpoint results. Accounting admission and the original
Plans 2-4, full Plan 5, R1-R8 and release gates remain closed or unfinished.

## Results and required follow-up

| Check | Result | Measured scope/time |
| --- | --- | --- |
| SQL production compile/link | PASS | 785 objects; 419.513 seconds |
| Client-free flatfile production compile/link | PASS | 785 objects; 400.194 seconds |
| Existing flatfile boot preflight | PASS | 3.893 seconds; supplied full binary; zero skips |
| Standalone native restore qualifier and shared fixture links | PASS after repair | Both real binaries link with the maintained strict flags |
| Selected MariaDB/MySQL full-dump restore and isolated service-boot cases | FAIL | 2 tests, 2 errors, 0 skips; 280.027 seconds including fixture builds; neither reaches service boot |

1. **MariaDB 10.11.14: migration 0065 rejects its own fresh result.** The original
   apply reaches `CALL duris_verify_zone_reset_item_origin()` and returns
   `ERROR 1644 (45000): incompatible zone reset item terminal schema`. A second,
   independently initialized disposable database reproduces it. The five-column
   table and both CHECK constraints exist, but `information_schema.columns`
   reports the nullable `terminal_publication_context` default as the string
   `NULL`; the apply and shell verifier require `column_default IS NULL`.
   Resolve the engine representation in the exact metadata checks, retaining
   rejection of non-null defaults and every other shape guard. Preserve the
   immutable-migration/sealed-history contract when publishing that correction;
   do not silently change an already recorded checksum or fabricate a receipt.
2. **MySQL 8.0.46: schema-65 runtime contract is not sealed.** The executable-bit
   repair lets all 65 migration receipts complete. The existing test then fails
   at `persistence_restore.database_qualify`, before backup capture/restore or
   service boot. Independent private reproduction reports history count/max
   `65/65` and runtime verification exit 1. Canonical, staging and master heads
   in `migrations/runtime_compatibility_manifest.json` still name
   `0064_auction_custody_history` with sequence 64. The shell history loop caps
   rows at that expected sequence, so the 65th row fails before later checks.
   `src/core/runtime_compatibility_contract.h` also pins the old heads and
   fingerprints. Measure and coherently seal the required schema-65 runtime
   tables, both engines' metadata, and supported histories in the maintained
   manifest/compiled contract. Changing only the declared sequence is insufficient.

After those prerequisites are corrected, rerun the same two cases against a
freshly built, exact successor candidate. SQL service readiness, copied journal
qualification, restored values and negative reconciliation controls remain
unqualified because the current cases stop before them. Plan 5 retains its
existing backup/restore ownership and the broader batch.

## Narrow repairs included with this review

`scripts/build_restore_qualifier.py` omitted newly required native providers.
The canonical list now includes the ten existing native-mobile birth recovery,
constructor, command, recipe, result and accounting providers, plus
`flatfile_season_state` (11 additions). The shared fixture already inherits this
list; no duplicate list, stub, new API or changed test assertion is introduced.
The original omitted symbols were shared-shop/cash-role recovery and fresh season
enrollment; following their real dependencies also required constructor recipes.

`migrations/immutable/0065_zone_reset_item_birth_origin.sh` changes only Git mode
`100644 -> 100755`. Its content/checksum is unchanged. MySQL previously stopped
with `PermissionError` when the runner directly executed that verifier; the
repaired rerun progresses past it. No schema contents or runtime pins are changed
by this review. The two remaining failures above are handed back explicitly.

## Exact inputs and reproduction

The source archive is a credential-free `git archive` of the published commit,
mounted read-only. Baseline tree identities:

| Input | Git tree / SHA-256 |
| --- | --- |
| `src` tree | `ce5829c9036407f709870c3f64c8447455fca80a` |
| `migrations` tree | `7d7a93c84940439ab8c3e6ba03b588483661cdbb` |
| `tests` tree | `2416b300f80c5dc5a2fda368c32f8d9628129707` |
| `scripts` tree | `5285045a9f68108cf80892866154b9bae1fe5638` |
| Baseline archive SHA-256 | `43f4edf74af1705f0c5eae3b04c56fc3ca45b1d4d9c693def2d1da002c927ba5` |
| SQL server SHA-256 | `31a1d4330657730ffd26b6b27d97d7f8d467e8871cc5951258022eded2be1d76` |
| Flatfile server SHA-256 | `c0013bc68ade92ff0fae805111696e368116344af24161e1539a2fba7422ab16` |

The final SQL run uses the same source/test trees and unchanged server binaries,
with exactly the helper and file-mode repairs above. Repaired migration tree:
`a22d54a28286200f09d91d11cb0cbd8c782b0b82`; script tree:
`5bc8047b1460495396489cf9d5fb40dda8cc94cf`. Helper SHA-256:
`3eb771f54b168270a9fd729d77ee21a60340b8edd799191f2734719707087460`.
The archived repaired input and exact patch are retained with the evidence.
The repair is rebased onto the current branch for publication; later source
changes, including `4f4507080` metadata reservations, are not qualified by these
frozen server binaries.

```sh
make -C /source/src -k -j4 PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production \
  BIN_ROOT=/source/bin OBJDIR=/source/bin/objects/sql/production \
  DMS_BINARY=/source/bin/server/dms_new
make -C /source/src -k -j4 PERSISTENCE_BACKEND=flatfile BUILD_PROFILE=production \
  BIN_ROOT=/source/bin OBJDIR=/source/bin/objects/flatfile/production \
  DMS_BINARY=/source/bin/server/dms_restore_flatfile
python3 tests/async/test_flatfile_boot_preflight.py \
  --server /source/bin/server/dms_restore_flatfile
DURIS_RUN_BACKUP_INTEGRATION=1 DURIS_RUN_MYSQL_BACKUP_INTEGRATION=1 \
python3 tests/async/test_persistence_backup_integration.py -v \
  PersistenceRecoveryIntegration.test_mariadb_full_dump_schema_history_values_and_isolated_service_boot \
  PersistenceRecoveryIntegration.test_mysql_full_dump_schema_history_values_and_isolated_service_boot
```

Environment: healthy Windows Docker engine 29.7.2; existing
`duris-finish-accounting-qa:local`, image ID
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`;
GCC 13.3.0/libstdc++ 13, MariaDB 10.11.14 and native MySQL 8.0.46.
The Ubuntu WSL Docker integration gap is still present; using the Windows CLI
bypasses it without a restart. No linker aliases or relaxed production flags
are needed in this image. Integration containers require `--cap-add SYS_ADMIN`,
`--security-opt seccomp=unconfined` and `--security-opt apparmor=unconfined` for
the maintained private user/network/PID namespace and tmpfs checks.

Build outputs are mounted directly from D: at `/source/bin`. Scratch uses an
owned D-backed Docker volume at `/t`, with `TMPDIR=/t` for short private sockets.
Databases are freshly initialized, socket-only and synthetic; no existing game,
shared database, runtime `.env` or production migrations are used. The final
SQL invocation has an evidence-only wrapper retaining normally discarded
migration-client stderr; the maintained methods, assertions, deadlines and
refusal/isolation guards are unchanged.

Flatfile preflight covers healthy minimal-world game-loop boots, HTTP readiness,
normal shutdown, missing/stale UID authority refusal, exact restoration and
controlled missing-world refusal. It does not qualify accepting accounting,
player gameplay, full-world boot, publication, terminal/ACK, crash recovery or
the full prospective 32 MiB bound. The earlier `626e33846` flat compile failure
at missing `RENT_CRASH` is superseded by the primary's published `ec632155c`
header repair and the complete rebuilt binaries reported here.

## Retained evidence

Evidence: `D:\Dev\Temp\accounting-candidate-smoke-8e5be4942-20261009`.
Outputs: `D:\Dev\Builds\Duris\accounting-candidate-smoke-8e5be4942-20261009\bin`.
`pinned-inputs.json`, `repaired-inputs.json`, build command/result JSON and logs,
`flatfile-smoke-result.json`, `sql-smoke-executable-result.json` and its log,
`migration-0065-diagnostic*.log`, `mysql-compatibility-diagnostic.log`, exact source
archives and `qualification-repair.patch` are retained locally on D:.
`review-evidence-hashes.json` records evidence and all four native binary hashes.
Failed intermediate fixture-link attempts are retained separately; they are
not reported as SQL service failures. Logs, binaries and synthetic DB data
are not committed.

Focused static checks pass: helper syntax, `git diff --check`,
`generate_economy_writer_coverage.py --check`, and `validate_economy_accounting.py`.
The latter records 14 fixtures, 931 writer routes and 2,911 candidate sites;
coverage remains false and release remains blocked. Original major-plan
qualification requirements are unchanged.
