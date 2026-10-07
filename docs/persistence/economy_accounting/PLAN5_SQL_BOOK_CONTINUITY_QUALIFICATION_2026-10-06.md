# Independent baseline book continuity qualification

Branch and remote: `codex/accounting-plan5`.
Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base `07e81085c9f0c2af2964bd1e06f83cd60067b38b`. Exact issue/result and remote commits are bound in
protected `resumable-book-delivery-01-20261006/delivery.json` after publication.
Primary refresh `b687b199fd0c9185f87d793b3318f2d4b6d06ee2` integrated the earlier Plan5
opening-policy/PID reader bytes. Its native/migration inputs match this base;
private startup/world/journal source and canonical0063 are not qualified here.

## Established defect and complete selected-root fix

The original page reader authenticates each baseline root but can ignore a
control reset to revision zero, a revision advanced beyond retained witnesses,
or a terminal changed to a different retained operation. The existing full
origin reader refuses these controls. Three complete new pure methods reproduce
14 false accepts on the base reader. The complete original native origin file
then reproduces the zero-revision false accept on both SQL engines: the typed
reader refuses, both pages have no finding, and full original table inventories
remain equal before/after the page reads. Each engine stops at that first failed
assertion; the native wrapper continues to the other engine. This red result
does not establish the additional revision/terminal pre-fix cuts.

The existing `verify_canonical_baseline` now checks the exact integer control
revision and nonzero terminal identity after authenticating the selected
witness. One projection using `uq_economic_baseline_witness_revision` selects
its predecessor, successor and terminal revisions: at most three expected
rows plus one extra-row sentinel. It compares contiguous revision identities,
committed baseline status, lineage/epoch and terminal operation. Missing,
duplicate, foreign or rejected neighbours and inconsistent controls produce
`restore_economic_baseline_book_mismatch`. Query budget refusal bypasses value
classification, rolls back, closes the cursor and leaves progress unchanged.

The same independent function serves both the root page and full restore reader;
the full reader retains its existing whole-store count/continuity checks. Return
values, checkpoint format, original historical NULL markers, locks, sticky
findings and CLI statuses are preserved. No mutation helper is imported and no
finding is auto-corrected. This closes the selected-root control gap. It does
not enumerate empty/orphan controls or establish one consistent whole-book
view across multiple pages; every page retains incomplete coverage.

Owned executable changes are `scripts/economic_restore_evidence.py` and the two
existing canonical/origin test files. `AUDIT_OPERATIONS.md`, this report and
the remote follow-up document the result. No C/C++ production source, migration,
shared contract/coordinator, producer, registry/matrix, activation owner or
central manifest is changed. No accounting/schema interface change is requested.

## Exact execution and source

- `python3 -B tests/async/test_economic_sql_canonical_audit.py`: exit 0, 53 methods, zero skips, 155.518926 seconds; environment `{"DURIS_PLAN5_CANONICAL_ARTIFACTS": "/workspace/bin/tests/resumable-baseline-canonical", "DURIS_PLAN5_CANONICAL_MOBILE": "1", "DURIS_PLAN5_CANONICAL_NATIVE": "1", "DURIS_PLAN5_CANONICAL_SOURCE": "1"}`; log SHA256 `2311d805d33410c1d11f66d5469b130885911c5176a593030478002d7ddffa87`.
- `python3 -B tests/async/test_economic_sql_audit_origins.py`: exit 0, 46 methods, zero skips, 75.446837 seconds; environment `{"DURIS_RUN_ECONOMIC_ORIGIN_INTEGRATION": "1"}`; log SHA256 `bb6424d817aa0e54572162872cb1f53fb36cb7a75070be616d780b26c1f743ca`.
- `python3 -B tests/async/test_restore_economic_coin_effects.py`: exit 0, 1 methods, zero skips, 274.943885 seconds; environment `{"DURIS_PLAN5_CANONICAL_EVIDENCE": "1", "DURIS_RUN_RESTORE_COIN_INTEGRATION": "1"}`; log SHA256 `5a05384dade0756979c67bc85fb266865f2db70402de7366678ef4147b68ce22`.

All **100 executed methods pass, zero skips**. The exact frozen archive SHA256
is `c02897b9601836ada3c9d10a613071cd87fb4d800acb95e2126a64a1b3175d39`: 6368 regular files, original Git
modes and four repository links, with four owned overlays. Every frozen file
is hashed before/after execution; publication compares the tested bytes again.
Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration tree
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain unchanged. Publication documentation is
excluded from executable qualification and hashed separately in delivery.

Pinned image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
uses GCC13.3/Python3.12, 2 CPUs/4GiB, no network/ports and private RAM work/tmp
trees. Original C++20/Werror/O1/debug/ASan/UBSan/non-PIE/crypto recipes build
fresh SQL/flatfile probes without object/probe reuse. No maintained data or
environment file is mounted. The private root remains writable for the unchanged
original money-recovery method's temporary directory under `/`.

Fresh MariaDB10.11.14 and MySQL8.0.46 apply all canonical0062 receipts. Each
engine retains the original 14 native origin reads and two command-preimage
refusals. Each adds three legal control corruptions and three restored controls,
with typed-origin and all-root page comparisons, rollback/close and complete
original table equality. The native fixture contains **four witnesses across
two books**, including the inactive book and uint64 endpoints. The new controls
respect canonical CHECK/FK constraints; no schema guard is disabled.

The log retains **186 measured pages**: 88 original canonical cuts, 18 original
moving-view/delayed-commit observations, four modeled maximum reservation cuts,
64 original native baseline pages and 12 new native book-refusal pages. The
12 additional restored-book pages execute and pass, but their temporary metric
file is removed by the original fixture; those metrics are not claimed retained.
Retained maxima are 93 queries, 6378751 bytes
and 0.320190124s, within the existing component limits.
The 9,071-reservation maximum remains a modeled SQL fixture, separate from the
native baseline encoder and real gameplay. No release-host, game-loop p95/p99,
1000-root mixed native, storage-growth or full-history fairness claim follows.

The original full native coin/restore consumer passes on this exact source.
Its private synthetic activation does not qualify genuine R6 or maintained
activation. Windows selected 50 methods pass with the original POSIX-lock skip;
Linux executes the lock/SIGKILL test, zero skips. Normal accounting validation
and whitespace pass; `--release` keeps `writer has no executable evidence`.

## Narrow primary-owned registration handoff

In `tests/integration_manifest.json`, retain all prior cases and append these
to `plan5_canonical_audit_pure.required_cases`; retain the preceding request
that its arguments select `CanonicalSweepTests`:

- `CanonicalSweepTests.test_baseline_page_book_control_revision_and_terminal_are_authenticated`
- `CanonicalSweepTests.test_baseline_page_book_neighbours_require_contiguous_committed_namespace`
- `CanonicalSweepTests.test_baseline_book_budget_refusal_keeps_progress`

Existing native origin rows still execute their same two methods; no new
argument/environment/required-case name is needed. Consumers are the central
integration runner's exact passed-case validation and qualification reports.
Preserve existing variants, original providers, fresh namespaces and 900-second
deadlines. No registration application/acknowledgement or variant waiver is
claimed. The primary validates the actual integrated source and selected rows.

## Evidence, correction and remaining gates

Protected root `D:/CodexEvidence/accounting-plan5/bin/` retains
`resumable-book-red-01-20261006`, `resumable-book-green-01-20261006` and
`resumable-book-component-gates-01-20261006`. The red test commands exit1 as
expected; its observing process exits0 after retaining those failures. The
original green process exits0. Both terminal containers have no OOM/restart;
observation expiration never restarts a process.

The first seal attempt assumed one red native observation and refused its count
assertion. The original wrapper had continued to the second engine, leaving
two authentic false accepts. The exact failed helper and observation remain in
`resumable-book-seal-failure-01-20261006`; the revised seal authenticates both.
No successful seal was created by the failed attempt and no test was restarted.

Seal `resumable-book-seal-02-20261006/evidence.json`, SHA256 `4fde78aee93359e9b38d045a64db47f6934664b2f9981571ae820a1f3c2efc27`,
inventories 3515 regular artifacts, 2592460281 bytes, zero links,
with streaming no-follow hashes. The delivery receipt binds commits/remote,
tested bytes/modes, owned paths and all seven earlier ancestor tips. Logs,
binaries, dumps, archives, data and scratch helpers are uncommitted.

The earlier baseline-page report called four witnesses four books, and its
immutable delivery receipt's `authentic_native_baseline_books_per_engine=4`
used that same incorrect label. That receipt remains preserved. The follow-up
documentation correction and this seal explicitly record four witnesses over
two books per engine; no execution or earlier scope is retroactively changed.

Whole-store control/orphan enumeration, claim-consumption continuity, complete
receipts/current native holdings and fair durable flatfile sweeps remain open.
Full quiescent comparison remains required. Genuine source capture/borrowed
activation verification, original missing recovery source claims, real producer/
publication/ACK/lost-reply/cold-world journeys, typed erasure, managed backup/
restore/retention and full release budgets remain. Independent Plan1 acceptance
stays completed at its recorded scope. Wallet-root exclusions and the declined
inactive spell path are preserved. Full Plan5/R7/R8/release remain incomplete.

The primary-local notebook remains nonblocking. This owned report/seal/delivery
packet supplies its curator workflow without a cross-chat message or claimed
acknowledgement. Maintained accounting stays inactive; no production operation,
activation, deployment, PR merge or push to experimental-accounting occurs.
