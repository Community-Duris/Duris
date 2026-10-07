# Plan 5 durable flatfile authority cross-link pages — 2026-10-06

The independent operator now saves bounded progress over both directions of the
authority mapping/native-locator relationship. It checks at most two selected
links in a bounded bucket, records semantic disagreements privately, and rotates
through 256 mapping buckets and 256 locator buckets. Refused pages keep their
cursor and fence while siblings continue. All whole-reconciliation, native-value
comparison and release flags remain false. This is metadata cross-link coverage,
not current holdings or complete immutable-evidence reconstruction.

## Publication and ownership

Branch `codex/accounting-plan5`; worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `0f40bbd9cfef523571d9cee74963cf45c5da7446`. The delivery receipt below binds the exact result SHA
and remote head after commit/publication. All earlier branch tips and follow-ups
remain ancestors. No history rewrite or experimental-accounting push is used.

Owned code/test files:

- `scripts/flatfile_economic_audit.py`
- `scripts/qualify_flatfile_economic_authority.h`
- `scripts/qualify_flatfile_restore.cpp`
- `tests/async/flatfile_restore_authority_fixture.cpp`
- `tests/async/test_flatfile_restore_economic_authority.py`

Publication files are this report and `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
Native producer/storage code, migrations, accounting contracts, coordinator,
writer registry/matrix, activation ownership and shared test manifests are
unchanged. No shared interface/schema or test-registration request is required.
The existing registered native authority regression gains the new controls.

## Defect, implementation and qualification scope

The protected red observer against the prior published native binary finds both
required page entry points missing. Both refuse, with unchanged native files.
The new page path reuses the original independent framing/locator decoders and
the exact existing cross-link checks, extracted for two real callers: the whole
authority audit and the new bounded page. It never calls mutation/storage codecs
to decide whether retained metadata is valid. Historical retired mappings and
retired bank aliases keep their original policies.

Native mode: `--economic-authority-page ROOT {mapping|native} BUCKET CURSOR_OR_- CEILING_OR_-`.
Mapping cursors encode numeric authority IDs as eight big-endian bytes; native
cursors encode the original locator key bytes. Native key order is a traversal
fence, not commit order. Nonempty saved cursor and ceiling must remain indexed.
Selected semantic link failures become sticky findings; budget failures refuse
the whole page. Local index/context failures never advance progress. The same
existing private read-only native lock excludes writers and pending journals.

Operator mode: `python3 -B scripts/flatfile_economic_audit.py --state-root ABS_ROOT
--qualifier ABS_BINARY --progress ABS_PRIVATE_PATH --scope authority-links`.
The strict `flatfile_economic_authority_progress_v1` checkpoint has 512 bucket
states and is separate from retained-root scope. It shares existing protected
checkpoint I/O, source/lineage binding, atomic replacement and owner locking.
Routine output contains counts; locator keys/aliases stay in private progress.
Use a fresh protected progress path for this scope or changed executable bytes;
an old source/scope checkpoint is refused and preserved. No audit corrects data.
An uninitialized authority returns incomplete observations without an initialized
checkpoint. Completed historical ranges never establish release coverage.

Limits: two selected links, 32 MiB admitted reads, 64 framed reads, 30-second
cooperative admission deadline, 45-second helper timeout, 128 KiB progress and
32 retained sticky findings. These are mode limits, not measured release-host
latency/storage budgets or a hard kernel I/O deadline.

The native fixture creates 608 mappings through the existing native test API,
including retired/recreated wallet lifetimes, renamed bank history, one-byte and
50-byte bank aliases. It preserves an inactive epoch. Tests cover both directions,
512 actual pages with durable saves/loads and later continuation, framed link
disagreement, sticky CLI failures, refusal fairness, lost previously indexed
native anchors, interrupted replacement, wrong scope, timeout, actual native
writer/journal exclusion and sanitizer budget refusal during link checks.

The first run had a fixture cursor past the damaged row; the corrected setup
explicitly starts that private observation at the damaged row. The second run
had a file-only domain observer that could not inventory the journal fixture’s
directory; the observer now includes directory entries. A repair-helper count
assertion then prevented the edit while the shell launched duplicate old source.
That owned duplicate was explicitly stopped and preserved with exit 137; it earns
no qualification. Both failed runs and the stopped run remain retained. The final
full native regression runs the corrected fixture/observer.

## Exact source and executed checks

Final native regression and uninitialized-scope archive SHA256:
`ee0973bd6c389bd1e6056ebf3e27504227e40220cc6f9dc2ba2108c3615eda9b`.
SQL/marker component archive SHA256:
`826abefeb34c4010a17592c812aadef5f151eae24d17051f639f091672e23a37`.
Maintained production-build archive SHA256:
`c65731c40815cf89f929105e30f6707599ad517d24f5c0acbc1a691b508faeb1`.
Each archive contains 6377 regular files, four original links and five overlays
with exact Git export modes. Only the authority regression’s `check_authority_pages()`
body differs across these archives. All other file bytes/modes/links and AST module
bodies, including imported `build_fixture`, are identical. All component and server
build inputs remain exact; these scopes are recorded separately.

Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b`; migrations tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical 0062.
Refreshed primary `a455dcb94df3e9be8cabf3487e4cc5eda3afa816` retains native tree
`01291db15446d94f36e066032aa3a1eea28ef354` and the same migrations tree.
Its private producer, cold-world and journal journeys are not qualified here;
primary owns integration and the tested combined candidate. Earlier 0055/0056
qualification does not substitute for any current combined qualification.

Retained containers use image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
GCC 13.3/Python 3.12, no network/ports/production mounts, two CPUs/four GiB,
private three-GiB `/workspace` and two-GiB `/tmp` tmpfs, with native caches off.

- `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py`: final
  archive, exit 0, zero skips, 29 authority-page controls and 28 retained-root
  controls; original 20 positive stores/367 refusals, 12 audit boundary and nine
  admission/concurrency controls pass. Original 1058 metadata comparisons and
  574 envelope comparisons remain. All relevant native inventories are unchanged.
  Observed method duration: 204.995695 seconds.
- `python3 -u -B tests/async/test_economic_sql_canonical_audit.py`: component
  archive, 64 methods, zero skips. Environment:
  `DURIS_PLAN5_CANONICAL_NATIVE=1`, `DURIS_PLAN5_CANONICAL_SOURCE=1`,
  `DURIS_PLAN5_CANONICAL_MOBILE=1`, and
  `DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/authority-pages-canonical`.
  Fresh MariaDB 10.11.14 and MySQL 8.0.46 are canonical 0062; SELECT-only mutation
  denial remains 1142 and complete database inventories remain unchanged.
  Observed duration: 193.009185 seconds.
- `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py
  --native-source /workspace --artifacts /workspace/bin/tests/authority-pages-baseline-markers`:
  component archive, all 69 cases, zero skips; 56 refusals, seven readable
  unqualified stores and six scoped qualification cases. Authority/operator
  inventories remain unchanged.
  Observed duration: 149.917642 seconds.
- Supplemental final-archive native observer: three uninitialized CLI cases and
  six raw mapping/native page cases, zero skips, unchanged native inventories
  and no initialized checkpoint.
- `git diff --check`, the authoritative formatter check for all three touched
  C++ files, and normal `python -B scripts/validate_economy_accounting.py` pass.
  The validator checks 14 fixtures/920 routes/2876 sites and keeps release_ready
  false. `--release` still exits 1 for missing executable writer evidence.

Fresh production commands, run sequentially with timeout 600:

```sh
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb BIN_ROOT=/workspace/bin/tests/authority-pages-build/bin OBJDIR=/workspace/bin/tests/authority-pages-build/objects DMS_BINARY=/workspace/bin/tests/authority-pages-build/server
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/authority-pages-build/bin OBJDIR=/workspace/bin/tests/authority-pages-build/objects DMS_BINARY=/workspace/bin/tests/authority-pages-build/server
```

- sql: exit 0; 740 fresh objects/dependency files; no reuse, warnings or errors; 286.030071 seconds; server SHA256 `5e3e8529dc5701ec52f32a99f481e4fc77b80591561955835490779baba38ac8`.
- flatfile: exit 0; 740 fresh objects/dependency files; no reuse, warnings or errors; 274.824129 seconds; server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.

Protected seal `D:/CodexEvidence/accounting-plan5/bin/flatfile-authority-pages-seal-01-20261006/evidence.json`,
SHA256 `9ea826123267a8c9bb5d1f3ed10649a0e2e4d20b5f3018db59e0c07a7a3ef015`, hashes 8464 artifacts /
7247574384 bytes and records all closed container states.
Logs, source transports, exact commands, fresh binaries/objects and prior attempts
remain under `D:/CodexEvidence/accounting-plan5/bin/flatfile-authority-pages-*`.
Delivery receipt
`D:/CodexEvidence/accounting-plan5/bin/flatfile-authority-pages-delivery-01-20261006/delivery.json`
binds result/remote SHA, source closure, unchanged native/migrations/shared manifests
and the seven earlier branch tips. Publication prose is checked separately.

## Remaining gates and curator handoff

No independent blocker remains for this completed metadata page slice. Full
durable flatfile audit still needs baseline-book, lifecycle-receipt and orphan
namespace progress, actual native holdings and complete independent reconstruction.
Full R7/R8, supported writer/UID/holding coverage, genuine gameplay and fault/
restart/replay, current combined backup/restore/retention evidence, workload growth
and release-host budgets remain open. This slice does not qualify the primary’s
newer/private producer or complete any Plan, activation or release gate.

Accounting remains inactive. Wallet-root exclusions and the declined inactive
spell-path decision remain. No production mutation, audit correction, activation,
deployment, PR merge or independent experimental-accounting push occurs.
This report, remote follow-up, seal and delivery are the curator packet for the
primary’s nonblocking locally maintained notebook. No notebook application,
primary acknowledgement or cross-chat message is claimed.
