# Plan 5 required flatfile baseline controls — 2026-10-07

The independent operator now discovers required initialized baseline books from
the authenticated epoch catalogue, including books with no retained roots. Its
new bounded page checks each selected head, all 16 reservation shards and the
original roots/witnesses referenced by those controls. This qualifies the
control/reference component only; complete baseline history and release remain
separate gates.

## Publication and ownership

Branch `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`, base
`b5fcd8434fffe46f2458f31cb51d0f5d13f58aaa`. The protected delivery receipt binds
the exact result commit, remote head, clean worktree and tested source
payloads/modes/links. All seven earlier branch tips and their follow-ups remain
ancestors. Publication stays on this branch without rewriting history or
pushing independently to experimental-accounting.

Owned files are the three independent C++ operator inputs
`scripts/qualify_flatfile_economic_baseline.h`,
`scripts/qualify_flatfile_economic_records.h`,
`scripts/qualify_flatfile_restore.cpp`, Python operator
`scripts/flatfile_economic_audit.py`, two existing registered regressions
`tests/async/test_flatfile_restore_economic_authority.py` and
`tests/async/test_flatfile_restore_lifecycle_receipts.py`,
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`, this report and
`docs/persistence/economy_accounting/PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
No shared contract, schema, producer, coordinator, writer registry/matrix,
activation owner, manifest or runner change/request is required.

## Established gap and complete owned implementation

Fresh original source archive SHA256
`30beca29d817f1414ec862cfcf6fe7fd7261f3160e0b2e65a323bf372863017c`
passes all 69 original baseline-marker controls. Original native fixtures
produce both a rootless initialized book and the supported maximum book.
The missing native page exits 1 with its fixed diagnostic; the missing Python
scope exits 2 as an invalid choice. Removing all 17 controls from the rootless
book makes the whole independent audit refuse, while the retained-root page
correctly returns zero rows within its narrower scope. The missing operator
scope prevents durable control-loss discovery for such books.

The initial retained-root budget hypothesis was disproved: the original
64-read/32 MiB page admits the native 3,071-holding/6,000-item baseline root.
Those root-page budgets are unchanged.

`--economic-baseline-controls-page ROOT BUCKET AFTER CEILING` selects at most
two initialized epoch IDs in the authenticated catalogue's first-byte bucket,
including inactive epochs. Catalogue membership binds required file discovery;
loss of every book file cannot make an initialized book disappear from scope.
The implementation reuses the original independent head, shard, witness,
common-root/index/segment and semantic-record decoders. The one-shot baseline
reader also uses the extracted head/shard/witness membership routines and
retains its consecutive-revision, complete-root-count and terminal-history
checks. No native mutation/storage codec is substituted into the audit.

Each selected book binds its opening account, revision-zero initializer,
nonzero terminal, all shard checksums, canonical ordered kind/ID/operation
members and native slot membership. Every reservation operation and nonzero
revision terminal must resolve to the same epoch's original baseline root and
witness, with unique bounded revisions and exact terminal revision. Reconstructed
lifetime/UID membership must exactly match all 16 shards. Healthy sibling books
remain verifiable when another selected book has an invalid control.

The Python `--scope baseline-controls` uses format
`flatfile_economic_baseline_controls_progress_v1`, report
`flatfile_economic_baseline_controls_page_v1` and scope
`required_baseline_control_reference_page`. Epoch findings and current-page
consistency use the existing sticky-finding policy; a later healthy page keeps
its local true result while earlier findings keep CLI status 1. Private atomic
checkpoints, exclusive ownership, source/lineage binding, epoch ceilings and
fair refusal/timeout rotation remain. Budget refusal leaves the selected cursor
unchanged. The read-only authority lock and pending-journal refusal remain.

Earlier empty roots can have no reservation references. This component does
not establish consecutive complete history, total root count, orphan/unknown
initialization closure, current native holdings or full source/writer coverage.
All complete/sweep/control/book/orphan/native-holdings/release fields remain
false. Existing native fixture-producing function bodies are unchanged.

## Exact tested source, commands, backends and results

Final component archive SHA256
`a11cec5d30f3897202489a718734fd2d2e6e67f9a887403d2181ce265883f348`
contains 6,382 regular payloads and four links. Only the six operator/test inputs
are overlaid on the recorded base; every other payload, mode and link is exact.
Publication/operator documents are excluded from executable qualification.
Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain exact, canonical 0062.

Fresh qualification and builds use immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network disabled, two CPUs, 4 GiB memory, 3 GiB workspace/2 GiB temporary tmpfs,
cache off and no maintained runtime/database mounts. Each Docker argument array,
preparation helper, extraction/source verification loader, command environment,
stdout/stderr log, source digest and terminal container state is retained.
Native fixture and independent probes retain ASAN/UBSAN instrumentation;
the standalone restore qualifier retains its original non-sanitized build.

| Exact command in the isolated workspace | Result |
| --- | --- |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` | 20 healthy stores / 367 refusals; 28 root-page / 29 authority-page controls; 1,058 metadata / 574 envelope comparisons; all 30 baseline-control checks; exit 0, zero skips |
| `python3 -u -B tests/async/test_flatfile_restore_lifecycle_receipts.py --artifacts /workspace/bin/tests/baseline-controls-receipts` | All 131 original cases (9 accepted / 122 refused), 46 lifecycle pages and 35 baseline-control checks; exit 0, zero skips |
| `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py --native-source /workspace --artifacts /workspace/bin/tests/baseline-controls-markers` | All 69 original native marker controls; exit 0, zero skips |
| `python3 -u -B tests/async/test_economic_sql_canonical_audit.py` with native/source/mobile flags below | All 67 methods, native/source/mobile cases enabled; fresh MariaDB 10.11.14 and MySQL 8.0.46, canonical0062; SELECT-only denial 1142 and unchanged complete application-table inventories; exit 0, zero skips |

The canonical command uses `DURIS_PLAN5_CANONICAL_NATIVE=1`,
`DURIS_PLAN5_CANONICAL_SOURCE=1`, `DURIS_PLAN5_CANONICAL_MOBILE=1` and
`DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/baseline-controls-canonical`.
Its daemons, schema and data are disposable; SELECT-only denial and complete
database inventory comparisons are retained. This is component SQL qualification,
not a managed production backup or genuine producer/gameplay journey.

New native controls cover rootless and rich books, the maximum and native
65,536-member shard, lifecycle/renamed/retired/generic books, complete required
book/head/shard/witness loss, rehashed opening/revision/terminal/member drift,
cursor admission, actual seven-to-nine-epoch append fences, all 255 sibling
buckets before durable resumption, mixed valid/invalid books, sticky findings,
budget/timeout refusal, interrupted checkpoint replacement, wrong scope,
inside-state progress paths and competing checkpoint ownership. Every sampled
native state comparison checks bytes, mode, links, inode, size and mtime_ns.

Default independent budgets remain 16,384 physical reads, 128 MiB bytes,
8,192 directory entries and a cooperative 30-second deadline; the Python child
has its existing 45-second timeout. Actual sanitized measurements:

| Original native fixture | Physical reads | Bytes read | Decoder directory entries |
| --- | ---: | ---: | ---: |
| 3,071 holdings and 6,000 items | 25 | 3,598,994 | 0 |
| 65,536 members in one shard, 22 roots | 109 | 26,679,112 | 0 |
| Two lifecycle maximum books in one page | 46 | 2,940,004 | 0 |

One-file, one-byte and expired-deadline controls all refuse under sanitizers.
These are measured component fixtures, not full-world memory/latency or growing
release-history qualification.

Both fresh production builds use the same final source archive, with 740 new
objects and dependency records per backend, zero reused objects and zero
warnings/errors. Exact original commands:

```text
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/workspace/bin/tests/baseline-controls-build/bin OBJDIR=/workspace/bin/tests/baseline-controls-build/objects DMS_BINARY=/workspace/bin/tests/baseline-controls-build/server
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/baseline-controls-build/bin OBJDIR=/workspace/bin/tests/baseline-controls-build/objects DMS_BINARY=/workspace/bin/tests/baseline-controls-build/server
```

The retained SQL server SHA256 is `5e3e8529dc5701ec52f32a99f481e4fc77b80591561955835490779baba38ac8`; the
flatfile server SHA256 is `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.
These are listener-free build checks; no maintained server is stopped or booted.

Source gates pass: `git diff --check`; normal accounting validation (14 fixtures,
920 routes, 2,876 candidate sites, `release_ready=False`); all 55 writer-coverage
contract tests with zero skips; inventory 920; and
`bash scripts/format.sh --check --file scripts/qualify_flatfile_economic_baseline.h --file scripts/qualify_flatfile_economic_records.h --file scripts/qualify_flatfile_restore.cpp`
through WSL on the worktree. Release validation has expected exit 1 for
`writer has no executable evidence`. The initial Git-wide formatting observation
exceeded its 120-second harness timeout; its command/log/limitation are preserved.
The completed explicit full-file checks cover all three touched C++ inputs.

## Evidence, curator packet and remaining gates

All protected evidence is under `D:/CodexEvidence/accounting-plan5/bin/`:
`flatfile-baseline-controls-red-01-20261007`,
`flatfile-baseline-controls-green-01-20261007`,
`flatfile-baseline-controls-gates-01-20261007`,
`flatfile-baseline-controls-sql-build-01-20261007` and
`flatfile-baseline-controls-flatfile-build-01-20261007`.
The compiled authority qualifier/fixture/sanitized probe and generated probe
source were copied only after compiler/linker completion and must match the
actual test summary hashes; the copy command and comparisons are retained.
Persistent lifecycle/marker/canonical binaries, native fixture outputs,
checkpoint controls and original/red observations are also retained.

Seal `flatfile-baseline-controls-seal-01-20261007/evidence.json` has SHA256
`0a6241c431a68baa3efa145f05f84ffc4850adf5b110cab1572c9ff506117144`, binding 20,815 artifacts /
5,324,043,915 bytes, original/red and final source archives, exact results,
commands, native fixtures, untouched native/migration inputs, six source gates,
both fresh builds and terminal exited-zero container states. Initial formatting
and preparation observation limitations remain preserved; no native/SQL
qualification attempt failed or was skipped.

Delivery is `flatfile-baseline-controls-delivery-01-20261007/delivery.json`;
it binds result/remote/clean source, nine owned files and all seven preserved
branch tips. No compiled artifact, log, archive, credential or player data is
committed. This report plus the remote follow-up is the required curator packet
for the primary's nonblocking local notebook. Notebook application or primary
acknowledgement is not claimed, and no cross-chat message is sent.

Primary was refreshed to `66a3deee3641631b071c727194fdb5d22f146478`
before this slice; its native tree `01291db15446d94f36e066032aa3a1eea28ef354`
and private/combined qualification remain separate from this component source.
Complete baseline history/orphan/current-holdings reconstruction, R7/R8,
genuine producer/gameplay/fault evidence, combined backup/restore/retention,
private producer/cold cases and release-host growth/memory/latency/writer gates
remain open. Earlier 0055 results cannot qualify the combined candidate.
There is no slice blocker or skipped native/SQL case. No maintained accounting
activation, production mutation, deployment or merge occurs. Wallet-root item
exclusions, inactive behavior and the declined inactive spell change remain.
