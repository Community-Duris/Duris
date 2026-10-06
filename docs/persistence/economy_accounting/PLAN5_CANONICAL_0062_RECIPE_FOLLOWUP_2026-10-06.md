# Plan 5 canonical 0062 recipe follow-up — 2026-10-06

Plan 5 follows the published canonical 0062 head in four existing SQL recipes.
The original native restore method fails a stale 0061 assertion on both private
engines after its native controls pass. Separate fix
`81aad436305d75cacbc3001f0f392105a6912eb9` changes seven expected-head/output
literals, preserving original providers, flags, assertions and deadlines. The
repaired method exposes a shared executable-mode defect in the new 0062
verifier. Full SQL qualification is **not passing**.

## Branch, exact source and ownership

All follow-up stays on remote `codex/accounting-plan5`, in worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.

| Role | Exact commit |
| --- | --- |
| Previous published Plan 5 tip | `a7126dad4c00b7346e12fb62340f20f59b472bfe` |
| Primary canonical 0062 import | `8030e71b55286d77b751fe41044d978dd62fa4cc` |
| Recipe-fix base, exact two-parent import | `8e478f0882ce97d0c342b26d7bae97673c6df187` |
| Separate owned recipe fix | `81aad436305d75cacbc3001f0f392105a6912eb9` |
| Latest primary restore integration | `707b9cdd3d2f72307e70f1bd8dbf3e1ab2674d29` |
| Exact primary refresh merge | `c2ef99f903cc8f64476af5b39c2e2195dfb7db7a` |

The fix owns only these four files:

- `tests/async/run_restore_accounting_evidence_mysql.py`;
- `tests/async/run_native_sql_baseline_audit.py`;
- `tests/async/test_economic_sql_canonical_audit.py`;
- `tests/async/run_plan5_retention_journeys.py`.

The refresh imports four primary qualification documents exactly. Incoming
native restore source/fixture changes were already present from the earlier
solved Plan 5 slice. Two conflicts preserve the pre-merge owned
`AUDIT_OPERATIONS.md` and `test_flatfile_restore_economic_authority.py` exactly:
the primary versions lack the later independent SQL native-mobile reader
documentation and regression cases. No incoming implementation is discarded.
All seven earlier branch tips remain ancestors. No shared implementation,
migration, registry/matrix, activation owner or contract is independently edited.

Native tree: `f0ae5c63273e94035552a75a1b70596d5021e54d`.
Migration tree: `23c07abb009bd925d19c44a7bc55f69c000c56ee`.
All 3,125 tracked code inputs under `src`, `migrations`, `scripts` and `tests` at
the refresh merge match the repaired test/build transport byte-for-byte. The
documentation-only refresh adds no runtime result. Earlier 0055/0061 results
are not promoted to this 0062 candidate.

## Original failure and repaired attempt

Both attempts use this unchanged maintained command:

```text
python3 -u -B tests/async/test_restore_economic_coin_effects.py
```

Raw Git source/modes are authenticated before execution; only the four owned
files are overlaid in the repaired attempt. Neither run makes the verifier
executable. The offline Linux tools image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Each observation uses two CPUs/4 GiB, private workspace/tmp mounts, no network
and no checkout `.env`. Environment:

```text
PYTHONPATH=tests/async
PYTHONDONTWRITEBYTECODE=1
DURIS_REGRESSION_BUILD_CACHE=off
DURIS_RUN_RESTORE_COIN_INTEGRATION=1
DURIS_PLAN5_CANONICAL_EVIDENCE=1
DURIS_RUN_NATIVE_BASELINE_AUDIT=1
DURIS_PLAN5_CANONICAL_NATIVE=1
```

Only the repaired observer additionally sets
`DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/canonical62-original-audit`
for the later audit method, which the first failure prevents from running.

The original method creates fresh private MariaDB/MySQL daemons through the
existing restore provider, verifies an empty schema and invokes the original
canonical migration/reader runner. The outer deadline is 1,800 seconds;
original native compiler/sanitizer flags and child deadlines remain unchanged.

| Attempt | Exit / seconds | Actual result |
| --- | --- | --- |
| Original, no overlays | 1 / 193.645802 | Both engine subtests fail the stale `0061_economic_baseline_equipment` manifest assertion. |
| Four owned head repairs | 1 / 277.700380 | Both engine subtests reach migration verification and fail with `PermissionError` executing the new 0062 verifier. |

Both attempts freshly compile the original coin fixtures in SQL and flatfile
modes with zero reused objects. Before SQL failure, 32 coin cases and all 3,026
native/independent decoder comparisons pass, with 1,054 accepted decoder cases.
Both-mode fixture binary SHA256:
`527dde46b2f3300bf3fb5605c4390acbadcf1ab53b4c92e15e37bd71efd2c21b`.
Native history fixture SHA256:
`23cef4c8d3544a383cd9fb252fb2d65deb4924fb6c53d421c9a0f7f5ca57289a`.
Temporary fixture binaries are reported observations, not retained artifacts.
The production server binaries below are retained.

A separate read-only pinned-image observation, using the same fixed PATH as
the original restore provider, identifies `/usr/sbin/mariadbd` as MariaDB
10.11.14 and `/usr/local/bin/mysqld` as MySQL 8.0.46. Migration failure prevents
the runner's later connected `SELECT VERSION()` output. The binary observation
is recorded in `tmp/plan5/canonical62-tool-versions.json`.

The repaired observer stops at its first failed method. The planned full
`NativeCanonicalAuditTests` and `test_native_sql_baseline_audit.py` commands
therefore did not run; neither did managed retention/service boot. The failed
attempt has no final source-unchanged receipt. No skip, reduced assertion,
permission workaround or synthetic replacement establishes those gates.

Local Windows supplementation passes 26 existing pure tests, zero skips, in
0.135 seconds, with `PYTHONPATH=tests/async`:

```text
python -u -B -m unittest -v test_economic_sql_canonical_audit.RestoreProjectionTests test_economic_sql_canonical_audit.CanonicalAuditTests
```

All four owned Python files parse; `git diff --check` passes. These checks do
not qualify new allocation semantics or replace the blocked private SQL method.

## Fresh maintained production builds

Both builds consume the exact repaired transport, fresh empty object directories,
the same pinned image and two CPUs/4 GiB:

```text
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/evidence/build/bin OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/evidence/build/bin OBJDIR=/evidence/build/objects DMS_BINARY=/evidence/build/server
```

| Backend | Result | Retained server SHA256 |
| --- | --- | --- |
| SQL / MariaDB | Exit 0, 503.909962 s | `a76ac6c0a07d6fbddaad09f88634f90d822f8fbefd707e4d2ce625b844afc16d` |
| Flatfile | Exit 0, 478.414440 s | `6219b71aa50272fe4d5cd67804da8d143f5ab02c896cceac8c97bf525c2fdb0d` |

Each compiles 738 objects with 738 dependency records, zero reused objects,
warnings or errors, within the unchanged 600-second outer deadline. Both final
source-unchanged receipts pass. This qualifies compilation, not server boot,
financial writers, cold recovery or release-host journeys.

## Protected evidence and open gates

Under `D:/CodexEvidence/accounting-plan5/bin/`, retain:

- `canonical62-recipe-head-red-01-20261006`: raw source, helper, terminal log,
  original stale-head failure and expected-red observer result;
- `canonical62-recipe-head-green-01-20261006`: repaired source, helper, terminal
  log and actual shared-mode failure;
- `canonical62-maintained-sql-01-20261006`: source, original make log/results,
  retained server and all fresh object/dependency files;
- `canonical62-maintained-flatfile-01-20261006`: corresponding flatfile artifacts.

Red archive SHA256:
`694838357e64f84e2272693cfdcb7e98d419187562c9766492534f3227d46bf3`.
Repaired/build archive SHA256:
`922fc427b835cb9b939bb4a19b58068301eee6493c337fa3cd3be9ef84b7a9ce`.
Full seal: `canonical62-recipe-final-seal-01-20261006/evidence.json`, SHA256
`e167df4ce767ea384b7f5dca304de1588f34c4869d34880b5315d74c3e0257ab`.
It binds 3,125 current code inputs, 2,989 artifacts/2,498,254,372 retained bytes,
commands/results/log hashes, terminal container states, exact ownership/merges
and preserved branch tips. This is retained evidence size, not release storage
growth. Original observer result files retain a literal newline trailer; the
seal explicitly parses that trailer and hashes the unmodified original bytes.

[The narrow primary handoff](PLAN5_0062_EXECUTABLE_MODE_HANDOFF_2026-10-06.md)
requests only the verifier's Git executable bit. No data/wire/schema field,
immutable byte or manifest checksum change is requested. The primary owns the
correction; its publication is required before these raw-source SQL gates pass.

This owned report, seal and remote branch provide the primary's local notebook
curator packet. The notebook is nonblocking and is not independently rewritten.
Independent 0062 source/partial-consumption/versioned-origin authentication,
complete R6 native/live-world capture and actual activation verifier, financial
writers, ACK/lost-reply/cold restart, typed erasure, full managed backup/restore/
retention and release-host mixed-root size/latency/storage budgets remain gates.
Accounting stays inactive; wallet-root exclusions and the declined inactive
spell-path change are preserved. Release completion is not established.
