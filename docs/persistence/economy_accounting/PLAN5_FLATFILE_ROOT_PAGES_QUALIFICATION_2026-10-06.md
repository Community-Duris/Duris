# Plan 5 flatfile retained-root page qualification — 2026-10-06

## Result and exact source

The independent flatfile operator can now save bounded progress over retained
record roots. Each invocation checks at most two indexed records in one bucket,
then rotates among 256 buckets. A refused bucket keeps its cursor and captured
ceiling; its sticky finding persists while other buckets continue. A completed
historical range restarts from the beginning on the next visit, so a delayed
lower operation ID is eventually observed. Operation IDs are not commit watermarks.

This qualifies retained-record pages only. Every routine report keeps
`complete`, `consistent_entire_sweep`, `release_qualified`,
`native_holdings_compared`, `baseline_books_closed`, `lifecycle_receipts_closed`,
and `orphan_namespace_closed` false. Completed ranges count historical index
visits, not complete independent reconciliation or release acceptance.

Branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `ece7a6280e209a43636b7717f848892e4377f1a9`. The exact result SHA is bound by the remote delivery
receipt below; a commit cannot contain its own SHA. All prior branch tips and
their follow-ups remain on this publication branch. No history rewrite or
independent push to experimental-accounting is used.

Final regression archive SHA256 `037af95d84cbd967b64384c819b3a754f7e5a5939b29ec4d1571307fbad3f162` contains
6376 regular files, 4 original links,
original Git modes and eight explicit executable/test overlays. Native tree
`4abb609524a1f1682ea4c190f82d75003c4d679b`; migrations tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical 0062.
Database/marker checks and both production builds used archive
`ab06ed9db51bc5290cbf3b808b8eb97c0d37e4265e845fdc8228e8e087f08040`. The only archive difference is the
private-corruption setup inside `check_root_pages()` in the authority Python
regression; it is corrected and fully run in the final archive. All other 6375
files/modes and four links match. AST comparison proves all other authority-module
bodies, including the imported `build_fixture`, are identical. All component
inputs and maintained build inputs remain exact. These source scopes are explicit;
no earlier 0055 or 0056 result substitutes for them.

Refreshed primary `299ee884ec31895175af8e6afc15a3d3ac6b868c` has native tree
`01291db15446d94f36e066032aa3a1eea28ef354`. Its latest finish-plan checkpoint
reports warm MySQL producer publication passing, cold-world recovery refusing,
and the original journal-fault journey not yet run. That newer/private producer
candidate is not imported or independently qualified here. Primary owns the
tested combined candidate, shared coordinator changes and activation decision.

Owned executable/test files:

- `scripts/economic_audit_progress.py`
- `scripts/flatfile_economic_audit.py`
- `scripts/economic_sql_canonical_audit.py`
- `scripts/qualify_flatfile_economic_authority.h`
- `scripts/qualify_flatfile_economic_records.h`
- `scripts/qualify_flatfile_restore.cpp`
- `tests/async/flatfile_restore_authority_fixture.cpp`
- `tests/async/test_flatfile_restore_economic_authority.py`

Publication files are this report and `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
No producer/mutation source, accounting contract, migration, coordinator,
writer registry/matrix, activation owner or central test manifest is edited.
No shared interface/schema change is requested for this slice. The existing
registered authority regression is extended in place. Its original body and
discovery/deadline/flags remain preserved; only its existing generated sanitized
reader source gains a root-page budget mode. The checkpoint I/O module has two
actual callers: the existing canonical SQL audit and the new flatfile audit.
SQL query logic and checkpoint representation remain unchanged.

## Established defects and repair

The previous operator offered a bounded whole audit but no durable flatfile
progress. With only fixture/test overlays over the base, all three native page
probes refuse the required entry point; the red regression exits1 as expected
in 125.073798 seconds. The native fixture constructs
four real encoded retained records in an inactive authority store, using the
existing native test stage/commit API, without gameplay activation.

Review of the new candidate found a second defect before publication: the
page accepted a cursor that was never indexed and could earn a completed range
after the saved ceiling disappeared. The protected native observer against the
previous candidate binary reproduces three unsafe acceptances for these two
conditions, including the CLI counter. The fixed reader requires every nonzero
saved cursor and ceiling to remain in the bounded index before advancing.
An absent anchor produces a page refusal, no completed range and no cursor
change. The wrapper retains the exception and rotates to the next bucket.

The first ceiling probe failed because its shortened fixture kept an old frame
length; its appended lower record also violated the shortened index's segment
bound. That attempt is retained, not used as defect proof. The corrected second
probe updates count, byte total, frame length and SHA256, using the original
three-row index. The final regression exercises both valid-frame continuity
controls. Earlier20/26/28-control prototypes and their own source scopes remain
preserved, including the earlier28-control run with the flawed ceiling fixture.

The native `--economic-evidence-page` mode independently opens the existing
read-only authority lock, takes a nonblocking shared lock, and refuses pending
native journals. It validates the control/catalog context, bounded index,
touched segment geometry and all record hashes in those segments. It reuses
the independent semantic decoder for selected records; it does not call native
storage/mutation codecs to decide whether evidence is valid. A record semantic
failure is a retained finding; admission-budget failures refuse the entire page
and never masquerade as corrupt records. Routine output exposes aggregate
counts; operation IDs stay in protected progress/findings.

Private progress binds the resolved authority root, exact qualifier executable,
wrapper and checkpoint I/O bytes. Its strict v1 format fixes 256 bucket states,
32 retained sticky findings and a 128 KiB maximum. It rejects duplicate fields,
foreign source/lineage, unsafe file forms and concurrent owners. The checkpoint
and lock must be outside native authority, including resolved parent aliases.
Writes use a private temp file, fsync, atomic replacement and parent fsync.
No audit updates native authority, recovers journals or corrects findings.
An uninitialized store produces an explicit incomplete observation and no
initialized progress checkpoint. A sticky finding keeps later CLI exits nonzero.

## Commands, backends and evidence

The retained native runs use image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3 and Python 3.12, with native caches disabled. Docker has no network,
ports or production mounts,2 CPU/4 GiB,3 GiB private `/workspace` tmpfs and2 GiB
`/tmp` tmpfs. Logs, exact Docker commands, frozen source, source-transport
records and launch/process receipts are in protected evidence.

- `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py`:
  final archive, exit0, zero skips, 198.283725 seconds.
  Original 20 positive stores and 367 refusal cases pass; 1058 independent native
  metadata comparisons, 574 envelope comparisons, 54 native semantic decodes,
  11 native envelope records and 10 envelope refusals remain. Twelve original
  audit boundary controls and nine admission/concurrency controls pass.
  All 28 added page controls pass, including CLI restarts, a native delayed
  lower-ID append across 511 durable pages, sticky findings, fairness after
  corrupt-index refusal, timeout/no cursor advancement, interrupted checkpoint
  replacement, unsafe progress files, foreign binding, both saved-anchor
  controls, and budget refusal inside the semantic decoder. Every relevant
  native inventory remains unchanged by audits.
- `python3 -u -B tests/async/test_economic_sql_canonical_audit.py`, with
  `DURIS_PLAN5_CANONICAL_NATIVE=1`, `DURIS_PLAN5_CANONICAL_SOURCE=1`,
  `DURIS_PLAN5_CANONICAL_MOBILE=1`, and
  `DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/root-pages-canonical`:
  code archive, all 64 methods pass, zero skips,
  202.800147 seconds. Fresh MariaDB 10.11.14
  and MySQL 8.0.46 are canonical 0062. SELECT-only mutation denial remains 1142
  and complete database inventories remain unchanged.
- `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py
  --native-source /workspace --artifacts /workspace/bin/tests/root-pages-baseline-markers`:
  code archive, all 69 cases pass, zero skips,
  155.320099 seconds: 56 refusals, 7 readable
  unqualified stores and 6 scoped qualification cases; independent/operator
  state and economic evidence remain unchanged.
- Protected native final page-entry observer: three never-initialized cases
  (absent evidence, empty evidence, native-lock/empty evidence) return incomplete
  without a progress checkpoint; actual native exclusive writer and unresolved
  native transaction journal each refuse without successful output or native
  file changes. Five controls pass with zero skips. The journal is produced
  by the native public commit API's real after-image apply failure, not invented
  journal bytes. Fixtures remain synthetic and isolated.
- Production builds run sequentially in fresh private tmpfs under timeout 600:
  `make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND={mariadb|flatfile}
  BIN_ROOT=/workspace/bin/tests/root-pages-build/bin
  OBJDIR=/workspace/bin/tests/root-pages-build/objects
  DMS_BINARY=/workspace/bin/tests/root-pages-build/server`.

- sql: exit0; 740 fresh objects and dependency files; zero reuse, warnings or errors; 302.884957 seconds; server SHA256 `5e3e8529dc5701ec52f32a99f481e4fc77b80591561955835490779baba38ac8`.
- flatfile: exit0; 740 fresh objects and dependency files; zero reuse, warnings or errors; 255.106620 seconds; server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.

Exact production commands:

```sh
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/workspace/bin/tests/root-pages-build/bin OBJDIR=/workspace/bin/tests/root-pages-build/objects DMS_BINARY=/workspace/bin/tests/root-pages-build/server
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/root-pages-build/bin OBJDIR=/workspace/bin/tests/root-pages-build/objects DMS_BINARY=/workspace/bin/tests/root-pages-build/server
```

- `git diff --check` and authoritative `./scripts/format.sh --file ... --check`
  pass for all four touched C++ files (clang-format18.1.8). Normal
  `python -B scripts/validate_economy_accounting.py` passes 14 fixtures,
  920 routes and 2876 sites, with release_ready false. `--release` exits1 because
  a writer has no executable evidence; this remains a real release gate.
- Supplemental Windows pure run at the final archive: `PYTHONPATH=tests/async
  python -B -m unittest -v test_economic_sql_canonical_audit.CanonicalSweepTests
  test_economic_sql_canonical_audit.CompositeSweepTests`; exit0, 26 methods,
  one POSIX flock skip, 1.500000 seconds.
  That case runs in the zero-skip Linux 64-method suite. An earlier unretained
  Windows run had the same 26-method/one-skip scope and is not qualification evidence.

Protected seal `D:/CodexEvidence/accounting-plan5/bin/flatfile-root-pages-seal-01-20261006/evidence.json`, SHA256 `f7f9d0aecc7660c7e18e372913c6c771118e682d547406da9f7a099d14518158`, hashes
15380 artifacts / 11096725895 bytes and records every
closed container state. It includes the red, failed/corrected anchor probes,
all earlier executed prototypes/components/gates/SQL build, final runs and
retained source. A prepared first-attempt flatfile build was never launched;
it earns no pass and is superseded by the executed final build. No artifact is
deleted or rewritten. Delivery receipt
`D:/CodexEvidence/accounting-plan5/bin/flatfile-root-pages-delivery-01-20261006/delivery.json`
binds result commit, exact remote head, all executable file bytes/modes and
repository links, unchanged shared manifests/native/migrations, and all seven
earlier branch tips. Publication documents are verified separately from tests.

## Remaining gates and curator handoff

This is a completed retained-root page slice with no independent slice blocker.
Full durable flatfile audit still needs bounded/fair progress over authority
metadata, baseline books, lifecycle receipts, native holdings and orphan
namespaces, plus complete independent reconstruction. The 32 MiB/64 framed-read
and two-record page limits, 30-second cooperative admission deadline and 45-second
helper timeout bound this mode. They are not release-host latency/storage
measurements or a hard kernel I/O deadline.

Original R7/R8 acceptance, complete supported writer/UID/holding coverage,
genuine gameplay and fault/restart/replay journeys, current combined backup/
restore/retention evidence, workload growth and release-host budgets remain
open. Preserved earlier lifecycle/backup evidence keeps its recorded source
scope. Primary integrates this completed slice and publishes the tested combined
candidate. No whole Plan or release gate is promoted.

Accounting remains inactive; wallet-root exclusions and the declined inactive
spell path are preserved. No production data mutation, audit autocorrection,
activation, deployment, PR merge or independent experimental-accounting push
occurs. The user says primary's locally maintained shared notebook is nonblocking.
This owned report, remote follow-up, seal and delivery receipt are its curator
packet. No direct cross-chat message, primary acknowledgement, notebook application
or independently qualified private producer is claimed.
