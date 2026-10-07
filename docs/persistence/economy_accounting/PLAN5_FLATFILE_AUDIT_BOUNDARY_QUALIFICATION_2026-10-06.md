# Plan5 flatfile read-only audit boundary qualification — 2026-10-06

## Result and exact source

The existing `--economic-evidence-audit` operator mode now independently takes
the native authority's existing shared lock before reading retained economic
evidence and refuses unresolved authority journals. It limits the locked scan
and returns no successful output on contention, an unsafe lock, an unresolved
journal, or exhausted admission budget. The offline restore/candidate paths
keep their existing qualification and recovery rules. Accounting remains inactive.

Branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `6af4e27f9991e51599bfe9294432b4ec34b8554b`. The exact solved result commit is supplied by the
remote delivery receipt: a commit cannot contain its own SHA. Publication
documents are excluded from executable qualification and verified separately.
The previous envelope slice and all seven earlier branch tips remain ancestors.
All follow-ups continue on this same remote branch; no experimental-accounting
push or history rewrite is used.

Frozen source archive SHA256 `f92e752283cd7bd0aca53f60b6ba83639daa3c99c07f364f8da3843a16d30977`:
6373 regular files, 4 original repository links,
original Git modes, seven explicit overlays. Native tree
`4abb609524a1f1682ea4c190f82d75003c4d679b`; migrations tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical0062.
This includes the current canonical sequence beyond0056; earlier0055/0056
qualification is not substituted for the present tests. Fresh primary
`299ee884ec31895175af8e6afc15a3d3ac6b868c` has native tree
`01291db15446d94f36e066032aa3a1eea28ef354`. It was inspected but is not imported
or independently qualified by this slice. Primary owns the tested combined candidate.

Owned executable changes:

- `scripts/qualify_flatfile_economic_authority.h`
- `scripts/qualify_flatfile_economic_baseline.h`
- `scripts/qualify_flatfile_economic_lifecycle.h`
- `scripts/qualify_flatfile_economic_records.h`
- `scripts/qualify_flatfile_restore.cpp`
- `tests/async/flatfile_restore_authority_fixture.cpp`
- `tests/async/test_flatfile_restore_economic_authority.py`

Publication files: this report and `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
No shared native producer, mutation logic, accounting contract, migration,
coordinator, matrix, registry, activation owner, or central manifest was edited.
No interface/schema change is requested. The existing registered authority
regression is extended in place; its original discovery, flags and deadlines
remain exact. Primary integrates the slice and updates any shared candidate
pins through its normal coordinator workflow.

## Established defect and complete repair

The original operator branch directly called the whole independent checker
without acquiring the native lock. The retained-evidence journey's externally
held shared lock did not prove this operator branch was safe. The operator also
ignored `.critical-authority-transaction`, allowing it to report success while
native after-images were still unresolved.

The red source overlays only the existing fixture and regression. The actual
native `flatfile_authority_lock` holds its exclusive lock through a stdin/stdout
barrier. Separately, the public native transaction commit API serializes and
publishes a real journal; its private target directory makes after-image apply
fail. The native result is io_error with committed publication outcome and a
pending journal. No hand-encoded journal, epoch selection or gameplay effect
is used. The original operator incorrectly accepts both conditions and all
six missing/unsafe lock variants, eight unsafe acceptances among12 observations.
The regression exits1 as expected in 121.367182 seconds;
the enclosing failure observer exits0 after verifying all eight failures.
Every audit leaves the whole private fixture inventory unchanged.

The repaired operator opens the existing lock O_RDONLY/CLOEXEC/NOFOLLOW/NONBLOCK,
checks private ownership, regular type and one link, and acquires LOCK_SH|LOCK_NB.
Native writers use LOCK_EX on the same lock. It never creates a lock, calls a
native storage reader or recovers a journal. Any existing journal object refuses.
Root, domains, evidence-directory and lock inode identity are checked at the
boundary; observed lock-path replacement invalidates the held cut. RAII closes
every descriptor, including construction failures and budget exceptions.

Absent/empty legacy evidence remains readable without initialization or creation
of a missing lock. The no-lock case returns that empty observation directly,
never an unlocked multi-file scan. Initialized/nonempty evidence needs a safe
existing lock. This preserves inactive behavior; the zero-epoch aggregate is
not an assertion of complete economic history or release qualification.

The operator's fixed limits are128 MiB of admitted framed-file input,2048 framed
file reads,8192 directory-entry visits and a30-second cooperative deadline.
Repeated physical reads and repeated directory passes are charged, including
unrelated/ignored filenames. Admission happens before payload allocation/read;
deadline checks cover file reads, decoded fields, directory passes and final
publication. On refusal, stdout is empty, stderr is the existing
`native_restore_qualification_failed`, exit1. No cursor or partial all-clear is
published. Limits apply only inside the operator audit scope: offline candidate
qualification retains the native format limits and original restore rules.

The deadline is cooperative and cannot preempt a blocked kernel call or sort.
It is not a measured30-second hard wall or a release-host latency guarantee.
This is a bounded whole retained-evidence scan against cooperating native
writers, not durable pagination, a complete live-native balance/custody cut,
an authenticated producer capture, or a fair resumable history sweep. Those
requirements remain open; a valid larger store can refuse these fixed limits.

## Commands, backends and results

Linux image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC13.3/Python3.12, network none, no ports/production mounts,2CPU/4GiB,
3GiB private `/workspace` tmpfs,2GiB `/tmp`, native cache off. Original source
archive/hash/mode and post-execution unchanged checks are retained for every
run. No test deadline, manifest marker or compiler flag is weakened.

- `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py`:
  exit0,0 skips,173.718884 seconds. New12 authority-boundary
  cases and9 resource/shared-reader/replacement controls. Every refused audit
  releases its lock, permitting immediate exclusive reacquisition. A1700-file
  unrelated namespace crosses the default directory-visit budget and refuses
  the real CLI, while the existing offline reader still qualifies it. Four
  individually constrained budgets (bytes/files/entries/expired deadline)
  refuse through the same sanitized independent audit implementation.
  The native pending journal is explicitly removed by private fixture cleanup;
  the operator never corrects or recovers it.
- The same original regression retains20 positive stores,
  367 corruptions,50 generic semantic refusals,
  54 native semantic decodes,1058 native metadata comparisons,
  574 native command-envelope comparisons,11 native envelope records and10
  rebound-envelope refusals. Existing mobile8 positives/69 refusals/154
  invocations remain. Evidence bytes remain unchanged.
- `python3 -u -B tests/async/test_economic_sql_canonical_audit.py`, with
  `DURIS_PLAN5_CANONICAL_NATIVE=1`, `DURIS_PLAN5_CANONICAL_SOURCE=1`,
  `DURIS_PLAN5_CANONICAL_MOBILE=1`,
  `DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/boundary-canonical`:
  64 methods,exit0,0 skips,189.187469 seconds.
  New private MariaDB10.11.14 and MySQL8.0.46 daemons, canonical62, SELECT-only
  denial1142 and complete before/after database inventories unchanged on both.
- `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py
  --native-source /workspace --artifacts /workspace/bin/tests/boundary-baseline-markers`:
  69 cases,exit0,0 skips,143.198864 seconds:
  56 structural refusals,7 readable/unqualified and6 scoped provenance-qualified.
  Independent/operator state unchanged. No separate historical native checkout
  is supplied: historical/coherent marker replacements remain synthetic.
- Clean `make -C src -j2 BUILD_PROFILE=production
  PERSISTENCE_BACKEND={mariadb|flatfile}`, each with fresh
  `BIN_ROOT=/workspace/bin/tests/boundary-build/bin`,
  `OBJDIR=/workspace/bin/tests/boundary-build/objects`,
  `DMS_BINARY=/workspace/bin/tests/boundary-build/server`, unchanged600-second
  deadline. Backends build sequentially in private tmpfs, then copy closed
  outputs to protected evidence.

- sql: exit0, 740 fresh objects and dependencies, zero reuse/warnings/errors, 282.534410 seconds; server SHA256 `5e3e8529dc5701ec52f32a99f481e4fc77b80591561955835490779baba38ac8`.
- flatfile: exit0, 740 fresh objects and dependencies, zero reuse/warnings/errors, 266.513728 seconds; server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.

Qualifier SHA256 `22ea42a3d3ad14e0e80184e0384ce37485cafcd0faf48db24fa4dd6e1bc40193`;
native fixture `2af6037b7d348844b4b389a6a6bfa274352af87b316462eef4ba321181e037df`;
sanitized reader `64d9f4f8c1e343a51a3e829907b899cab16f5eed2c77363eeeff35b2d0772748`.
The original authority recipe removes its temporary binaries and state on exit;
their hashes/assertions survive, and marker qualification retains corresponding
native fixture/qualifier artifacts. No claim is made that deleted temporaries
survive. Sanitizers halt on ASan/UBSan errors and leak detection is enabled.

18 original main-build statements remain
in order with identical ASTs after excluding only two new summary counters.
The one changed original statement extends the existing sanitized-reader source
with operator-boundary modes; its argc2 offline path still calls the original
checker. Both original long candidate/corruption blocks remain exact. Shared
regression/integration manifests and discovery are unchanged. Whitespace,
normal contract validation and repository `format.sh --file ... --check`
pass for all six owned C++ files with clang-format18.1.8. Normal validator:
14 fixtures/920 routes/2876 candidate sites, release_ready=False. Release
validator exits1 with `writer has no executable evidence`, a remaining gate.

## Evidence, handoff and remaining gates

Root `D:/CodexEvidence/accounting-plan5/bin/`:

- `flatfile-audit-boundary-red-01-20261006/`: exact original failures, native
  pending-journal hash,12 complete observations, archive/source/process receipts.
- `flatfile-audit-boundary-green-01-20261006/`: original full regression,
  boundary observations, final summary, archive and source-unchanged receipts.
- `flatfile-audit-boundary-components-01-20261006/`: canonical both-engine and
  marker logs/results, retained binaries/private fixture evidence.
- `flatfile-audit-boundary-{sql|flatfile}-build-01-20261006/`: original clean
  make commands/logs,740 fresh object/dependency files and closed server binaries.
- `flatfile-audit-boundary-gates-01-20261006/`: original whitespace, validator
  and formatting commands/results.
- `flatfile-audit-boundary-seal-01-20261006/evidence.json`, SHA256 `839140252776847acceba240210f83825146267b4f7828602cac3307af7fda25`:
  8409 artifacts/5398353826 bytes, no external filesystem links;
  every container is closed. Extended absolute Windows paths avoid MAX_PATH
  truncation when hashing retained long native filenames, without omissions.
- The final `flatfile-audit-boundary-delivery-01-20261006/delivery.json` binds
  the clean result commit, remote branch, tested files/modes/links and seven
  preserved old tips. It is emitted after push and checked independently.

This slice has no remaining fix blocker or final selected skip. Full Plan5,
R7/R8 and release remain incomplete: durable fair flatfile pagination, remaining
native/evidence reconciliation, authenticated complete capture, genuine
producer/publication/ACK/cold-world journeys, populated upgrades, complete
backup/restore/retention and combined release-host budgets remain required.
The new private journal is a native storage failure model; it is not a genuine
producer/cold-world journey. Prior scoped retention and inspector handoffs retain
their exact source scope. Inventory and isolated green checks do not qualify
these remaining requirements or primary's newer combined source.

Accounting activation, production mutation, automatic audit correction,
deployment, PR merge and independent experimental-accounting publication do not
occur. Wallet-root item exclusions and the declined inactive spell path remain.
Primary's local notebook is nonblocking by user direction. This report, the
follow-up, seal and remote delivery are the curator packet; no cross-chat
message, primary acknowledgement or notebook application is claimed.
