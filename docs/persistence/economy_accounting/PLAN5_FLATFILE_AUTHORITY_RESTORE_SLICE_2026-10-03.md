# Plan 5: retained flatfile authority restore qualification

Delivery branch: `codex/accounting-plan5` on Community-Duris/Duris.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Slice base: `6899caa2b08ec5ecd2a49b3fbc7cfbf13d51dd19`, following the published
operator lookup, SQL restore evidence and provenance slices on canonical
`7d2f8e8153f637c38e19054cf1202436d3f9a28a` with migration 0056.
The result is the commit containing this report; the final handoff and ignored
`tmp/plan5/flatfile-authority-evidence.json` record its exact SHA after commit.

The refreshed remote is `f7d26eaa721cd3b675c0b0c65009a2535813f400`. Its only
change from that canonical base is `lib/information/news`; native inputs are
unchanged. This does not incorporate the primary owner's unpublished fixes or
qualify that future combined candidate. Only this dedicated branch is pushed.

## Established defect and complete metadata fix

The actual standalone native qualifier first accepted a healthy private legacy
candidate. It then returned success for the same candidate with
`economic-evidence/authority.eal` containing `corrupt-economic-control`.
The corrupt bytes remained unchanged. The native RED is retained in
`tmp/plan5/flatfile-economic-native-red3.log`. Earlier `red`/`red2` probes failed
fixture setup because of directory permissions or missing legacy directories;
they are not the defect reproduction.

Both native `--state-preflight` and ordinary post-replay qualification now run
an independent metadata reader after candidate authority recovery. The reader
opens files read-only, without following symlinks, and calls no production
economic storage reader, recovery API, lock or writer. It checks:

- Private regular metadata files, one link, bounded complete frames, magic,
  version, length, reserved fields, body hashes and control-bound file hashes.
- Retained lineage, operation identities, mapping capacity/counts and contiguous
  lifetime allocation, epoch ordinal/predecessor/identity chains and latest epoch.
- Every mapping and native bucket, including untracked metadata files and invalid
  bucket filenames, native locator bounds, lifetime revisions, retained retirement
  identities, native key hash buckets, ordering and active/last identities.
- Mapping-to-native and native-to-mapping links, including valid bank rename
  tombstones, retired identities and native identities recreated with a new lifetime.

Each frame is limited to 2 MiB and each decoded bucket to 4096 entries. Either
cross-link pass caches at most eight decoded buckets. This bounds the reader's
working set, but is not a measured release workload or runtime budget.

Absent/empty storage and a native partial inactive bootstrap retain legacy
admission. They are not proof of never-activated state. Retained inactive epochs,
bank rename history and the full unsigned 64-bit revision range remain eligible.
Failures use the existing fixed, alias-free diagnostic and produce no success
receipt. The qualifier's success JSON remains unchanged. The independent reader
does not change bytes, permissions, links, pause state or accounting authority.
Existing candidate recovery may provision an empty directory for legacy state.

## Owned files and narrow shared handoff

- `scripts/qualify_flatfile_economic_authority.h`: independent metadata reader.
- `scripts/qualify_flatfile_restore.cpp`: invokes it after existing candidate recovery.
- `tests/async/flatfile_restore_authority_fixture.cpp`: native-encoded synthetic
  metadata using the existing test-only writer bridge; never selects an active epoch.
- `tests/async/test_flatfile_restore_economic_authority.py`: actual qualifier and
  independent reader regression, including native encoder and reader sanitizer checks.
- `docs/operations/BACKUPS.md` and this report: operator behavior, proof and limits.

No shared accounting interface, schema, wire format, mutation or coordinator
change is requested. The primary owner should register the new regression in
`tests/regression_manifest.json` using the existing native script profile and
refresh any required dependent test classifications. Consumer: the central
native regression/release runner. Required invariant: it must execute the actual
native qualifier's six accepted stores and 46 refusals, and must preserve the
read-only economic evidence assertions. Native Linux, g++, libcrypto and
ASan/UBSan are required; no database or game credentials are consumed. Shared
registration files were not edited. The previously reported native shop harness
linkage request remains in the SQL restore slice report.

## Exact tested source and commands

Native sources are unchanged from the slice base, with source tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`. The ignored evidence file also
pins every native qualifier/encoder dependency, build helpers and raw outputs.

| Owned executable input | SHA-256 |
| --- | --- |
| `scripts/qualify_flatfile_economic_authority.h` | `cf56e3d85dc78fc86af292a01c057d6476c01a0be4041f74331fb415d4fa000a` |
| `scripts/qualify_flatfile_restore.cpp` | `d69e3d8bbdd8069516db6c391caad9ed07571780acdd4a86dea963e67d7641b6` |
| `tests/async/flatfile_restore_authority_fixture.cpp` | `08b45596a58efb0445adbd8bd5f2918fca695df64cb05a6567ee686aaf347654` |
| `tests/async/test_flatfile_restore_economic_authority.py` | `496528769462889189c8ffac860eafe29ae317554f1dded90b971ceed578ee9a` |

Tools image:
`sha256:0232af0d2069d00424bfe14ae109224395050f46282896682913049de0b73b25`
(Ubuntu 24.04, Python 3.12.3, GCC 13.3.0). From the worktree, each command runs
inside that disposable tools container with the checkout mounted read-only at
`/workspace` and this worktree's `bin/` mounted writable at `/workspace/bin`:

```sh
python3 tests/async/test_flatfile_restore_economic_authority.py
python3 tests/async/test_persistence_backup.py
python3 tests/async/test_flatfile_backup_manifest.py
make -C src -j2
```

The native qualifier uses the existing `build_restore_qualifier.py` C++20 build
with `-Wall -Wextra -Wpedantic -Werror`, listener-free native repositories and
`-D__NO_MYSQL__`. The encoder and independent reader additionally use
`-fsanitize=address,undefined -fno-omit-frame-pointer -fno-pie -no-pie`, with
`ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.
The independent sanitizer executable links only this reader and libcrypto.
Its separate entry point performs no candidate recovery.

Results:

- Native flatfile: six positive stores and 46 corruption refusals; 104 actual
  native qualifier invocations plus 52 independent sanitized-reader invocations.
  Every invocation verifies retained economic bytes, modes and link counts are
  unchanged. Many corruptions are rehashed and rebound to the control, proving
  structural/cross-link detection beyond checksum refusal. FIFO refusal is bounded
  by the same 30-second invocation timeout and does not block opening the file.
- Backup filesystem regressions: 40 pass. Flatfile backup manifest regressions:
  four pass. These existing tests use disposable synthetic filesystem authority.
- Maintained default SQL/development server build: pass, with the repository's
  strict warning/hardening flags. Binary SHA-256:
  `ac7646a258aa5ebb5a05a95c51f93797ad95e97d74716b3b5a05042910425ace`.
  This is a build result; no configured server is started. The maintained
  flatfile build and full service restore journeys remain separate gates.
- Repository formatter `--check --file` passes for the three touched C++ files.
  `git diff --check` and Python compilation pass. A default changed-lines check
  under WSL cannot resolve this Windows-managed worktree's `.git` pointer;
  its misleading success message is not counted as validation. The explicit
  file checks use `.clang-format` without that Git dependency.

Executable hashes from the successful native regression:

- Qualifier: `ece403442c27d68ccd91d57ce8951110a200488b1c6df99e3be191a97aaf5b2c`.
- Native encoder fixture: `943d6b785d1858b4b46481c71e8d750c81cac1620a9963e80d7314dc90cfb20e`.
- Independent sanitized reader: `b0e2ecad88f69742d645f32890be62d11e1b2ffdaf7a7657e85f4ba64410c4a1`.

Raw logs are under ignored `tmp/plan5/`: `flatfile-authority-native-green.log`,
`flatfile-authority-backup-unit.log`, `flatfile-authority-backup-manifest.log`,
`flatfile-authority-maintained-build.log`, `flatfile-authority-format-check.log`.
`flatfile-authority-native-setup-failure.log` preserves the first regression
fixture failure: existing native recovery creates an empty legacy directory.
The corrected test compares retained evidence rather than equating an absent
directory with economic data. The pre-sanitized-reader pass is also retained.

## Remaining gates and blockers

This completes the established authority-metadata defect, not the full flatfile
evidence store. Retained operation indexes/segments, canonical records, source
dedupe, pile heads and baseline witnesses still require independent checks.
Native balance/custody/opening comparison, economic erasure, exact receipt replay,
missing-evidence pause authority and allocator continuity remain release gates.
Full backup/import/restart/retention journeys, both maintained server builds,
real gameplay/fault journeys, resource budgets, writer census and the combined
candidate's qualification remain open. The namespace prerequisite for later
isolated service journeys was checked successfully with a disposable container;
it is not itself a restore journey.

SQL database checks are not rerun for this flatfile-only metadata change. Previous
MySQL/MariaDB results remain pinned to the SQL restore slice; none are relabeled
as combined candidate qualification. No production database, project `.env`,
live server, accounting activation, PR merge, deployment or adjustment is used.
Wallet-root item exclusions and the declined inactive spell change are untouched.

`AI_CONTEXT.md` and the required curator workflow/notebook reference are still
unavailable in the checked repository and supplied context. The earlier request
for their location remains pending. This report is a source handoff, not a claimed
curator notebook update. That update is blocked on the missing instructions.
