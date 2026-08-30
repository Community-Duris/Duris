# Local MariaDB deployment

## Scope and environment

This log records the end-to-end local deployment performed on 2026-08-30 from
a clean `master` checkout. The target is a loopback-only development server
using MariaDB as the authoritative persistence backend. Secrets remain only in
the ignored, owner-readable `.env` file and are not reproduced here.

Initial observations:

- The repository was clean and `.env` was absent.
- The host MariaDB service was active on `127.0.0.1:3306`. Administrator access
  was supplied explicitly for this deployment, so the documented native
  MariaDB setup path is used with a project-specific schema and database user.
- Redis was already listening locally, but it is optional and outside this
  MariaDB deployment scope, so the local configuration explicitly disables it.

## Local configuration

The ignored `.env` selects `ENVIRONMENT=local`, `mariadb-primary`, database
`duris_dev`, and the exact allow-listed target `127.0.0.1/duris_dev`. It also
sets absolute player-save and critical-command journal directories beneath
`runtime/`, and contains a dedicated game test account for real login checks.
The file mode is `0600`; the journal directories are `0700`.

## Deployment log

- Confirmed the host database required administrator access, then received
  explicit authorization to provision the project-specific local schema and
  database user through the native MariaDB service.
- Created the initial local configuration and deployment record.
- Created `duris_dev` with `utf8mb4_unicode_ci`, created the loopback-only
  `duris_local` database identity, and granted it access only to that schema.
- Verified the database configuration with
  `./scripts/cycle_mud.sh --dev --check-config` and confirmed the new database
  contained zero tables before bootstrap.
- Loaded `migrations/bootstrap_multithread_safe.sql`, adopted the sealed fresh
  baseline, and applied the immutable post-baseline migrations.
- The first launcher pass exposed a schema gap: `cycle_mud.sh` records graceful
  shutdowns in `server_reboots`, but the fresh schema did not create that table.
  Added immutable migration `0004_server_reboots`, its verifier, lifecycle
  metadata, both supported-engine fingerprints, and all affected schema-count
  contracts. The migration uses an atomic shadow-table swap to preserve and
  converge records from the pre-b029 launcher-created schema, and is guarded
  and re-runnable through the immutable runner. Dual-engine regressions cover
  fresh creation, legacy conversion, exact row preservation, and replay.
- `migrations/verify_runtime_compatibility.sh` passed against MariaDB 10.11.14.
  The resulting schema has 173 InnoDB/`utf8mb4_unicode_ci` runtime tables, one
  sealed baseline record, and immutable migration state at applied count 4.
- Ran the tracked help-content importer in dry-run and live modes. A live-mode
  parser bug used literal backslash-newline delimiters and silently reported
  zero imported entries even though dry-run parsing succeeded. Corrected both
  live parsers, made parse/import errors fail closed, removed three nonexistent
  legacy source declarations, and added a regression contract. The clean import
  loaded all three `mud_info` records and produced 2,155 unique `pages` rows
  from 1,505 help-index and 1,367 parsed source entries (overlapping titles are
  intentionally replaced).
- Added the content-seeding commands to the fresh-database README path. Replaced
  the unavailable telnet-client example with `nc` and added `netcat-openbsd` to
  the build dependency package.
- Created the ignored local TLS certificate/key pair with
  `scripts/generate_localhost_cert.sh`; the private key is owner-readable only.
- Built the complete world data and server through `cycle_mud.sh --dev`. The
  server listens only on loopback ports 4000 (plain), 4001 (TLS), and 4050
  (WebSocket/HTTP health). `scripts/healthcheck.sh` reports ready MariaDB
  persistence.
- Created the configured test account and a level-1 test character through the
  actual game protocol, entered the world, requested a save, stopped cleanly,
  restarted, and logged back into the same character. Level, room, and save
  revision persisted, and `server_reboots` recorded each graceful shutdown.
- That gameplay check exposed a denormalized identity gap: the canonical
  `account_characters` row was present while `player_data.account_name` remained
  null. New saves now persist the identity inside the character transaction,
  and the login-time core projection repairs older null rows. The post-fix
  database check confirms both projections agree.
- Added the critical-command journal directory to `.gitignore`; both configured
  local journals remain empty after clean drains and have mode `0700`.

## Validation results

- `make -C src` passed with the repository's strict C++20 warning set and
  `-Werror`.
- `make test-all` passed all 340 repository regression tests, including the
  native signal-handler gate and full-world flat-file boot coverage.
- `scripts/warning-inventory.sh` completed with build exit 0 and zero findings
  in all six tracked warning categories. A strict rebuild passed afterward.
- `scripts/format.sh --check` passed for all touched C/C++ lines.
- Runtime schema verification passed locally and in disposable Docker runs for
  both MySQL 8 and MariaDB 10.11, including full-schema drift checks.
- Focused regression suites passed for immutable migrations, runtime boot
  compatibility, data lifecycle/archive behavior, personal-data export,
  account erasure, the help importer/catalog, documentation, and transactional
  player/account identity persistence.
- The final boot completed without warning/error/fatal matches in
  `logs/log/*` or `logs/duris-console.log`; the health endpoint and all three
  expected listeners are live.

## Final local state

MariaDB is the authoritative local persistence backend and the development
server remains running under `./scripts/cycle_mud.sh --dev`. Generated world
outputs, compressed database backups, logs, journals, certificates, `.env`, and
all account/player data remain ignored and uncommitted. No credentials or
player data are recorded in this document.
