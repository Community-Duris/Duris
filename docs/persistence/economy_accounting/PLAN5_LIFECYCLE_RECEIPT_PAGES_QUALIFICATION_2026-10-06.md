# Plan 5 durable lifecycle receipt/root pages — 2026-10-06

The independent flatfile operator now resumes required lifecycle receipt/root
validation through catalogue-driven pages. Each page verifies at most two
original lifecycle receipts and their common baseline roots/witnesses. Whole
required-file loss and old inactive epochs remain discoverable. Durable
progress preserves fences, sticky findings and fair rotation after refusal.
This is scoped read-only evidence; full reconciliation and release stay open.

## Publication and ownership

Branch `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`, base
`7bec84ae9642197642978cd15afba2a4030275b8`. The protected delivery receipt binds the exact result commit,
remote head, clean worktree, source bytes/modes/links and all seven preserved
earlier branch tips. No history rewrite or independent experimental-accounting
push occurs. Existing branch follow-ups remain ancestors.

Owned files:

- `scripts/qualify_flatfile_economic_lifecycle.h`
- `scripts/qualify_flatfile_economic_records.h`
- `scripts/qualify_flatfile_restore.cpp`
- `scripts/flatfile_economic_audit.py`
- `tests/async/flatfile_restore_lifecycle_receipt_fixture.cpp`
- `tests/async/test_flatfile_restore_lifecycle_receipts.py`
- `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`
- this qualification report
- `docs/persistence/economy_accounting/PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`

No shared accounting contract, schema, producer, coordinator, registry/matrix,
activation owner, manifest or runner change is made or requested. The existing
registered `flatfile_restore_lifecycle_receipts` script executes every original
case and the added page function without a shared registration change.

## Established defect and complete owned repair

Fresh original source archive
`5b253d41044281f099244bdf09e7e6547c74be83bc9a9033525bd9faea639676`
passes the whole-store independent audit on both current and retained inactive
epoch fixtures, but `--economic-lifecycle-page ROOT 20 - -` exits 1 with the
fixed native diagnostic. The Python CLI rejects `--scope lifecycle-receipts`
with argparse exit 2. Both original native fixtures remain byte/metadata exact.

The new native page selects `lifecycle_owner` initializers from the authenticated
epoch catalogue rather than discovering only existing filenames. It sorts IDs
by their first-byte bucket, retains a traversal ceiling and requires both saved
anchors to remain catalogue members. Later IDs above an existing fence wait for
the next range. Each selected receipt uses the original independent decoder,
its exact derived baseline operation, the original index/segment geometry and
record hashes, original command/plan/revision/result linkage and independent
witness-to-effect reconstruction. The original touched-segment decoder is
shared with retained-root pages; no mutation/storage reader or new codec is used.

Receipt/link state is isolated per selected operation. A missing or damaged
receipt/root/witness becomes `flatfile_lifecycle_receipt_invalid` with its
original lifecycle operation ID while a healthy sibling can verify. Typed
budget refusal propagates and grants no cursor or completed range. The original
read-only authority lock refuses cooperating writers and pending journals.

The `lifecycle-receipts` CLI scope uses existing atomic private external progress
and checkpoint-owner locking. Its format is
`flatfile_economic_lifecycle_progress_v1`; page reports are
`flatfile_economic_lifecycle_page_v1`, scope
`required_lifecycle_receipt_root_page`. Each call rotates across 256 buckets,
including after refused/expired pages. Sticky findings keep CLI exit 1; refused
pages preserve their cursor/fence and earn no completed range. Source, scope and
lineage mismatch refuse without replacing the checkpoint.

## Exact tested source, commands, backends and results

Final component archive SHA256 `27092ccd47b1bda12d8805df72555aaf506e9543e1fa424cee7f8891b9195c93` contains
6,380 regular payloads, four links and six code/test overlays on the recorded
base. Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` are frozen; canonical migrations run through 0062.
Publication/operator documents are excluded from executable qualification.
Delivery verifies all other published source bytes/modes and links against this
archive.

All complete component commands use immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
no network, two CPUs, 4 GiB memory, 3 GiB workspace and 2 GiB temporary tmpfs,
no maintained runtime/database mounts and build cache off. Original native
fixtures and independent probes use ASAN leak/error and UBSAN halt-on-error.

| Exact command | Result |
| --- | --- |
| `python3 -u -B tests/async/test_flatfile_restore_lifecycle_receipts.py --artifacts /workspace/bin/tests/lifecycle-receipt-pages-receipts` | 131 original cases: 9 accepted / 122 refused; 46 page controls; zero skips, exit 0 |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` | 20 positive stores / 367 corruption refusals; 28 root-page / 29 authority-page controls; unchanged bytes, zero skips, exit 0 |
| `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py --native-source /workspace --artifacts /workspace/bin/tests/lifecycle-receipt-pages-markers` | 69 cases, zero skips, exit 0 |
| `DURIS_PLAN5_CANONICAL_NATIVE=1 DURIS_PLAN5_CANONICAL_SOURCE=1 DURIS_PLAN5_CANONICAL_MOBILE=1 DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/lifecycle-receipt-pages-canonical python3 -u -B tests/async/test_economic_sql_canonical_audit.py` | 67 methods; fresh MariaDB 10.11.14 and MySQL 8.0.46, canonical 0062; zero skips, exit 0 |

The SQL batch retains SELECT-only denial 1142, transaction/cursor cleanup,
unchanged complete database inventories, original native plan/source/mobile
checks and durable scheduling/refusal controls. These are component fixtures,
not genuine gameplay or private producer qualification.

The new native cases cover current, empty, renamed, retired, retained inactive,
generic and maximum fixtures; two maximum receipts; nine required epochs;
seven-epoch fences followed by authentic native appends; resumed checkpoints;
all 255 sibling buckets before resumption; wholly absent, damaged and unsafe
receipts; missing original index/witness; a fully rehashed changed root revision;
second-page missing receipt; sticky findings; refused and timed-out pages;
interrupted writes; foreign lineage/scope; private path/owner locks; native writer
locks and pending journals. The seven original native fixture modes retain
exact original bytes. Native state comparisons include bytes, modes, links,
inodes, sizes and mtimes. No source capture, lifecycle install or activation is
executed; source descriptors remain explicitly modeled.

Lifecycle pages admit at most 16,384 physical reads, 128 MiB and 30 seconds;
the default 8,192 directory-entry allowance remains. Two supported 3,071-holding
receipts measure 6,154 reads and
17,988,794 bytes under the real default budget/sanitizers.
Fixed catalogue-derived decoder lookups perform no namespace scan (measured zero
decoder enumeration entries). The probe does not measure every metadata syscall;
the CLI also performs its bounded emptiness check. This is not a release-host
I/O or latency measurement.
Read-count, byte and deadline controls refuse. Growing history and release-host
budgets remain unqualified.

Both fresh maintained production builds use archive
`c84f334e484ff3c64abf60c7a63cdfb9b565af009ea3e6eb7243a7a9ea1d0f77`. The final archive changes only the
Python directory-enumeration test control and its generated independent probe
branch: the page performs no directory enumeration. Full AST normalization
proves that exact difference and equality of all other test/provider bodies;
all other 6,379 payloads/modes, four links and every production build/provider
input are exact. The final independent probe is compiled and qualified in the
final component batch. The earlier fixture compilation and erroneous directory
control failures are preserved and excluded from successful component evidence.

```sh
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=mariadb \
  BIN_ROOT=/workspace/bin/tests/lifecycle-receipt-pages-build/bin \
  OBJDIR=/workspace/bin/tests/lifecycle-receipt-pages-build/objects \
  DMS_BINARY=/workspace/bin/tests/lifecycle-receipt-pages-build/server
# Separate fresh container/workspace for PERSISTENCE_BACKEND=flatfile.
```

Both builds compile all 740 objects/dependency records with zero reuse, warnings
or errors, exit 0. SQL: 316.36090466193855 seconds, server SHA256
`5e3e8529dc5701ec52f32a99f481e4fc77b80591561955835490779baba38ac8`. Flatfile: 293.5624863880221 seconds, server SHA256
`f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`. All generated outputs remain under bin/ and protected
evidence, with none committed.

Changed-line and whole touched-file formatting, whitespace, normal accounting
validation and central inventory pass. Normal validator records 14 fixtures,
920 routes, 2,876 candidate sites and `release_ready=False`; central inventory
validates 920 owners. Release mode exits 1, `writer has no executable evidence`.
These checks do not qualify release.

## Evidence and curator handoff

Protected evidence is under `D:/CodexEvidence/accounting-plan5/bin/`:

- `lifecycle-receipt-pages-red-01-20261006`: original missing interface/scope observations
- `lifecycle-receipt-pages-green-03-20261006`: final frozen source, four complete batches and native artifacts
- `lifecycle-receipt-pages-green-01-20261006`: preserved fixture compilation failure
- `lifecycle-receipt-pages-green-02-20261006`: preserved directory-control assertion failure / exact production build source
- `lifecycle-receipt-pages-{sql,flatfile}-build-01-20261006`: both fresh production builds
- `lifecycle-receipt-pages-gates-02-20261006`: final source checks
- `lifecycle-receipt-pages-seal-01-20261006/evidence.json`, SHA256 `d379240bd6c98365bce09d8a52adb7a0946443f268db0e4fc7d5b600aaa625cf`
- `lifecycle-receipt-pages-delivery-01-20261006/delivery.json`: exact result/remote and source/ancestry closure

The seal hashes 19,292 artifacts, 6,356,369,569
bytes and records every container's terminal state. This report, appended remote
follow-up and seal/delivery form the curator packet for the primary's nonblocking
local notebook. No application, acknowledgement or direct cross-chat message is
claimed. Primary owns integration and publication of the tested combined candidate.

## Remaining gates

There is no independent blocker for this completed page slice. Required receipt
root/witness links are scoped; generic/unknown origins, orphan receipt filenames,
complete baseline/reservation/lifecycle namespace closure, native current
holdings, UID/source/writer reconstruction and full R7/R8 remain separate.
Reports keep all full-closure/activation/release flags false even after traversal.
Actual player/fault/restart/replay, current combined backup/restore/retention,
growing-history/release-host budgets and complete writer coverage remain.

Refreshed primary `3b7a465af40cd6867d3dcb3c52aaa9200d38661b` has native tree
`01291db15446d94f36e066032aa3a1eea28ef354`, different from this slice. Its shared
startup/native auction/private producer fixes, cold admission/template prerequisite,
published ANF2/ACT2 interface, pending MariaDB/fault cases and canonical private
0063 qualification require their own combined candidate proof. This slice grants
none. Earlier 0055/0056 results cannot qualify these recorded 0062 inputs.
Maintained accounting stays inactive; wallet-root exclusions and the declined
inactive spell-path change remain exact. No deployment, PR merge, production
change, activation or auto-correction occurs. Full Plan 5/release stays incomplete.
