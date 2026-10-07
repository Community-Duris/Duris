# Plan 5 durable initialized baseline history — 2026-10-07

The independent flatfile operator now checks the complete declared history of
catalogue-required initialized baseline books across durable pages. It detects
lost empty roots that control/reference pages cannot discover. A fixed
authenticated cut prevents mixing changed evidence across invocations. This
qualifies the known initialized history component; broader namespace, current
native holdings, gameplay, R7/R8 and release remain open.

## Publication and ownership

Branch `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`, base
`7962dd8ad1cefd55675b46ec34fbc3b693ac3f11`. The protected delivery receipt binds
the exact result commit, remote head and clean source. All seven earlier branch
tips and follow-ups remain ancestors. No experimental-accounting push or
history rewrite is made.

Owned files are the independent operator headers
`scripts/qualify_flatfile_economic_baseline.h` and
`scripts/qualify_flatfile_economic_records.h`, dispatcher
`scripts/qualify_flatfile_restore.cpp`, Python operator
`scripts/flatfile_economic_audit.py` and new
`scripts/flatfile_baseline_history_audit.py`, native fixture
`tests/async/flatfile_restore_authority_fixture.cpp`, the two existing registered
regressions `tests/async/test_flatfile_restore_economic_authority.py` and
`tests/async/test_flatfile_restore_lifecycle_receipts.py`,
`docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`, this report and
`docs/persistence/economy_accounting/PLAN5_REMOTE_FOLLOWUP_2026-10-06.md`.
The new private progress/report formats are operator outputs, with no shared
accounting contract, schema, producer, coordinator, registry, activation,
manifest or runner change/request.

## Established defect and complete owned fix

The fixture adds explicit modes using original native mutation/storage code to
stage three empty batches, and another mode adding a later nonempty batch plus
a rootless initialized epoch. Those fixture branches are deliberate changes;
no unchanged-fixture-body claim applies to this slice. Server/native accounting
implementations remain unchanged.

Red archive SHA256
`b397f4c03303ab8be2b726883f1f54b14d35c1900715b09fb4f3b7909d402800`
contains the base operator and added fixture modes. All 69 original marker
controls pass. Removing an earlier empty common root, and rehashing the
initialized-bucket bitmap to describe that loss, makes the whole independent
audit refuse while the control/reference page still verifies its book. The
original operator lacks both native history commands and the Python scope.
The successful red probe checks both empty-only and mixed history and unchanged
evidence metadata. It reuses the exact verified original binaries/source from
the fresh marker run, without claiming another fresh marker compilation.
The initial wrong-head locator observation and subsequent missing-parent
preparation error are retained separately.

The new `baseline-history` scope authenticates all initialized common indexes
and catalogue-required initialized heads in a fixed SHA256 cut. Context,
control and root commands recheck this cut under the existing read-only lock,
refusing pending journals. The controls phase reads one book's 16 shards. The
roots phase reads one original common record per invocation and independently
verifies original semantics, baseline witness effects and exact original shard
membership. Per-book revision bitmaps prove dense revisions, root count,
terminal identity/revision and total reservation coverage. This includes empty
roots and revision-zero books. Completed buckets freeze within the cut; later
closed-checkpoint calls reauthenticate it.

Private progress is limited to 2 MiB, uses exclusive ownership and atomic
replacement, and binds root/executable/operator/helper source. Refused root
pages rotate without advancing cursors. Semantic and refusal findings remain
sticky; healthy later pages retain CLI status 1. Exact count/bitmap state is
bounded by 4,096 books and 1,048,576 aggregate declared roots. Native commands
retain 16,384 physical reads, 128 MiB, 8,192 decoder directory entries and a
30-second cooperative deadline; Python retains its 45-second subprocess limit.
No existing page budgets change.

`known_initialized_baseline_books_closed` can become true only after every
required control and indexed record/history check passes. It covers known
initialized books in this fixed cut. Legacy unknown epochs remain counted and
unknown. All full completeness/book/orphan/current-holdings/release fields stay
false; this does not discover orphan filenames or qualify current native state.

## Exact tested source and validation

Final native/component archive SHA256
`c674e01e957070ca67402fcdf3603fc746585dd83d62f1e4bb536ca903a15983` contains 6,384 exact file payloads/modes and four
links. Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` match the base; canonical migrations end at 0062.
The delivery verifies every nonpublication payload/link against the committed
result and explicitly binds the single qualified archive-mode variant below. Three owned publication documents are excluded from executable testing.

The immutable image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with no network, 2 CPU/4 GiB limits, fresh tmpfs workspace/temp directories,
cache disabled and no maintained state mounts. Commands use the actual frozen
source and native encoders, with ASAN/UBSAN fixtures/readers and strict C++20
`-Wall -Wextra -Wpedantic -Werror` compilation. Original state comparisons cover
bytes, mode, links, inode, size and mtime.

| Exact command | Observed result |
| --- | --- |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` | 20 accepted / 367 refused stores, 28 root pages, 29 authority pages, 1,058 metadata and 574 command-envelope comparisons; 30 control-page cases and 51 new history cases; zero skips |
| `python3 -u -B tests/async/test_flatfile_restore_lifecycle_receipts.py --artifacts /workspace/bin/tests/baseline-history-receipts` | 131 cases: 9 accepted / 122 refused; 46 lifecycle pages, 35 control-page cases and 38 new history cases; zero skips |
| `python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py --native-source /workspace --artifacts /workspace/bin/tests/baseline-history-markers` | 69 cases: 56 structural refusals / 7 readable unqualified / 6 provenance qualified; zero skips |
| `python3 -u -B tests/async/test_economic_sql_canonical_audit.py` | All 67 methods pass with native/source/mobile switches set to 1; zero skips |

The SQL command sets `DURIS_PLAN5_CANONICAL_NATIVE=1`,
`DURIS_PLAN5_CANONICAL_SOURCE=1`, `DURIS_PLAN5_CANONICAL_MOBILE=1` and
`DURIS_PLAN5_CANONICAL_ARTIFACTS=/workspace/bin/tests/baseline-history-canonical`.
Fresh MariaDB 10.11.14 and MySQL 8.0.46 canonical0062 fixtures run under SELECT-only
roles, establish write-denial error 1142 and preserve complete table inventories.
This is independent component evidence, not combined gameplay or release proof.

New history cases cover healthy empty/mixed/rich baseline histories, rootless
initialized authority books, mixed/empty/retained/generic lifecycle stores,
full reservation-shard capacity and two maximum-holding lifecycle books,
empty-root loss, rehashed duplicate revisions, changed cuts, sticky findings,
timeout rotation, interrupted atomic replacement, wrong scope/source,
malformed progress/page data, inside-state checkpoints, exclusive owners,
required head loss and reauthentication after closure. Native context/control/root
page measurements stay inside the unchanged budgets and use zero decoder directory
entries. File/byte/deadline refusals leave native state unchanged. The full-shard
page uses 45 reads / 2,882,812 bytes; the selected two-book maximum-fixture page
uses 29 reads / 1,504,538 bytes. These are measured component pages, not a full
capacity sweep or release-host performance qualification.

Fresh production commands are `make -C src -j2 BUILD_PROFILE=production
PERSISTENCE_BACKEND=mariadb` and the corresponding `PERSISTENCE_BACKEND=flatfile`,
with isolated BIN_ROOT/OBJDIR/DMS_BINARY paths recorded in the sealed commands.
Both build 740 objects and 740 dependency records, with zero reused objects,
warnings or errors. The SQL build uses archive
`af6d58d1855828d557a66634d8cbb7051ec4bcbafb0e7430d325064c3e5c2516`;
the flatfile build uses the final archive. The only changed payload between those
archives is the authority regression's fixture selection/comment/description;
all production compile inputs, native implementations, independent reader inputs,
file modes and links are identical. No production rebuild is inferred from a
reused native object or different server source.

Normal source contracts pass (14 fixtures / 920 writer routes / 2,876 candidate
sites, release_ready=false), all 55 coverage contracts pass, inventory remains
920, diff whitespace checks pass and explicit formatting checks pass for all
four touched C++ inputs. The release validator exits 1 as expected because a
writer has no executable evidence. Inventory and contracts do not close release.

Protected evidence root is `D:/CodexEvidence/accounting-plan5/bin/`:

- `flatfile-baseline-history-red-01-20261007` through `red-03`: fresh original
  69-marker run, exact binary reuse, two established empty-root losses and
  preserved locator/preparation failures.
- `flatfile-baseline-history-green-01-20261007` and `green-02`: retained initial
  helper/enum compile errors and regression CLI-flag error; neither qualifies.
- `flatfile-baseline-history-green-03-20261007` and `green-04`: preserved passing
  intermediate observations before the lifecycle fixture description was corrected.
- `flatfile-baseline-history-green-05-20261007`: final frozen source, actual
  native binaries/probe sources, history checkpoints/fixtures, all four direct
  regression logs and disposable database results.
- `flatfile-baseline-history-gates-03-20261007`, the two
  `flatfile-baseline-history-*-build-01-20261007` directories and sealed terminal
  container states. Earlier gate observations remain retained.
- `flatfile-baseline-history-seal-01-20261007/evidence.json`: SHA256
  `de3dce3352f0396384cd866ab702c4fd7ef619496b7dc32650255854fc6dedd9`, 66,130 artifacts / 12,892,787,552 bytes.
- `flatfile-baseline-history-delivery-01-20261007/delivery.json`: exact result,
  remote head, clean source and seven ancestor tips after publication.

Publication preflight found one transport-mode difference: the preparation
helper assigned `0644` to new `scripts/flatfile_baseline_history_audit.py`, while
Git's canonical tar emits `0664`. Working, committed and qualified source bytes
all have SHA256 `eda0360020fabc21ddfcf04ff0e4d1032e4d67b29f9207e6199dde501887f260`;
line endings and code are identical. The initial line-ending diagnosis was
incorrect. No source or shared Git attribute change is needed.

Canonical archive `cecbe9b7bbefa8a5911770618e074b02cedb5f01061e862ea4df0d0fc360d334`
from implementation commit `d4df44355c1ec0a3ac1f2ccac95c4c94f0217fa6` reruns all
51 authority / 38 lifecycle history checks at actual source mode `0664`, with
zero skips. It reuses the sealed native binaries only after proving identical
C++/native inputs and binary hashes; no new native compilation is claimed.
The first supplemental harness omitted the entrypoints' `0077` private-data
umask, and the reader correctly refused those data files. That failed observation
is retained; the corrected harness passes with private fixture files. The
inventory's Windows long-path observation is also retained with its corrected
extended absolute-path check.

`flatfile-baseline-history-mode-supplement-01-20261007/evidence.json` binds both
supplemental terminal states and 4,238 artifacts, SHA256
`75d71151ffcfaf40ba48f832f034eef88eaed03eb2b46697a9a0cb230acc30b0`. Final delivery verifies every payload and link,
all unchanged modes and this explicitly qualified single mode variant. The
publication follow-up changes owned documentation only; implementation/native
inputs remain exact, and published history is preserved.

No validation skips or slice blocker remain. Full Plan 5/release gates below
remain open.

## Remaining gates and curator handoff

Primary refresh at turn start is
`aa252cd8134972548a953a0c5e500da2f8d06af7`. Its native tree
`01291db15446d94f36e066032aa3a1eea28ef354` differs from this slice. The latest
checkpoint imports lifecycle pages and records separate private producer/cold
checks, with MySQL pending-journal cold timeout and auction-finalization/native
item binding work still open. This component cannot qualify that combined
candidate or the private producer source.

The report and appended remote follow-up are the nonblocking local notebook
curator packet. The primary maintains the shared notebook locally; application
or acknowledgement is not claimed, and no cross-chat message is sent. Full
orphan/unknown-initialization closure, current native holdings and source/writer
history, genuine gameplay/faults, combined backup/restore/retention, release-host
growth/memory/latency and full R7/R8/release remain open. No production data,
maintained activation or automatic correction occurs. Wallet-root item
exclusions, inactive behavior and the declined inactive spell change remain.
