# Plan 5: independent auction credit creators — 2026-10-07

`--economic-auction-source-credit-audit` independently derives version-1 auction
source credits from authenticated retained bid/settlement facts, money effects
and native receipts. A matching native claim/source total can reference an absent
creator; the new command reports this and other unproven credits. Findings never
trigger recovery, mutation or correction. Canonical consumers, frozen claim
digests, complete origins and release qualification remain open.

## Exact source and ownership

- Local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `1252fd0ae18c586f006344c2a2649407e14b176e`. Exact result/remote equality is recorded in
  `D:/CodexEvidence/accounting-plan5/bin/flatfile-auction-attribution-delivery-01-20261007/delivery.json`.
- Frozen code tree: `ff5aa5a688d28aa256e0c640279a5424bf3f9ded`.
- Tested canonical archive SHA256: `540d2605310daed66fa19c944e39250a58704fd2e7122d9b95bd4e1e475bfb09`;
  6413 regular source bodies/modes and four links are authenticated.
  Publication adds this report and updates the follow-up. Delivery verifies every
  nonpublication body/mode/link against the tested archive.
- Unchanged native tree: `4abb609524a1f1682ea4c190f82d75003c4d679b`; unchanged canonical migrations 0–62:
  `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. Native source and migrations are untouched.
- Refreshed primary: `275df7f626e12cb396a22da34317a4e7f355e9a1`, native
  `4f6d56c3daae5f474341f02c4b5ae5b0aa2eb471`, migrations
  `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`. The auction repository and three
  accounting compilers match byte-for-byte. `auction_command.c` differs by the
  newer version-2 native context extension; its diff is retained. This qualification
  covers the exact version-1 source above, not that distinct combined candidate.

All seven earlier alternate-branch tips remain preserved ancestors; their
follow-ups continue on this same remote branch. Primary owns shared contracts,
producer integration, coordinator, registry/matrix, activation and combined
publication. No shared interface/schema change is needed for this version-1
reader slice. No shared native, migration, runner or manifest file is edited.
These thirteen owned code files change:

| Owned file |
| --- |
| `scripts/qualify_flatfile_auction_money.h` |
| `scripts/qualify_flatfile_auction_source_balance.h` |
| `scripts/qualify_flatfile_auction_source_credit.h` |
| `scripts/qualify_flatfile_current_money.h` |
| `scripts/qualify_flatfile_economic_authority.h` |
| `scripts/qualify_flatfile_economic_records.h` |
| `scripts/qualify_flatfile_native_auction.h` |
| `scripts/qualify_flatfile_restore.cpp` |
| `tests/async/flatfile_auction_attribution_cases.py` |
| `tests/async/flatfile_auction_money_fixture.cpp` |
| `tests/async/flatfile_restore_authority_fixture.cpp` |
| `tests/async/test_flatfile_restore_baseline_markers.py` |
| `tests/async/test_flatfile_restore_economic_authority.py` |

## Established defect and completed fix

The original native auction/source codecs seed a private root with claim 60 and
source 60. Original accounting/baseline codecs retain matching modeled endpoints
while accounting remains inactive. The source's operation changes to 7 and its
native envelope hash is rebuilt. The complete retained common indexes contain
no operation 7. Original `claim_source_balance_matches` returns true; the existing
source-balance audit exits 0 with its narrow balance flag true and attribution
false. The original cut and every observed retained file remain unchanged by
the queries. The separate red run completes with exit 0 and zero skips.

The new command reports `claim_source_creator_missing_or_unproven`. It scans all
retained successful version-1 writer 9 bid and writer 11 settlement roots, including
expired/removed/zero-proceeds cases. It independently checks original command
fences, policy/compiler 1, domain actor wire value 1, frozen listing/source-event
identity, supported action/result fields, account/money/revision effects and
beneficiary mapping. Positive outbid refunds occupy slot 1; positive seller
proceeds occupy slot 2. Closing fees use exact floor division. Zero proceeds
create no source row. Unknown positive claim writers, rejections, baselines and
unsupported tuples grant no credit proof.

Every native source row, including consumed historical rows, must match the
derived operation/slot, lineage, immutable claim lifetime, beneficiary and amount.
Every derived positive credit needs a retained native row. The creator must have
a native successful receipt whose digest matches SHA256 of the complete original
CCM1 command and whose meaningful result fields match the retained result. Native
event publication and result padding ignored by the original codec carry no
credit authority. AEC1 absent endpoints resolve only to the exact beneficiary
mapping created by this operation, with before balance/revision 0. Another
operation's mapping cannot substitute.

One coherent read lock covers authenticated authority/mappings/common records,
sources and native catalog. Borrowed wire observers default to absent for existing
callers; outputs wait for the complete scan and terminal guards. No native
mutation/recovery codec is imported by the standalone implementation. Root,
mapping, expected-credit, source and native-receipt accumulators are bounded at
32768; findings preserve their exact aggregate beyond the 100 retained rows.
Existing byte/file/entry/deadline, pending-journal and file-safety refusal guards
apply, with no partial proof emitted after refusal.

## Exact tests, backends and results

Final native checks use the canonical archive above in pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with network disabled, CPU 2/memory 4 GiB and disposable private Linux roots.
Windows/PowerShell orchestrates canonical Git transport, formatting, contracts,
sealing and publication. GCC C++20 builds original native code. The focused
standalone reader and original typed economic fixture use ASan/UBSan,
`detect_leaks=1:halt_on_error=1` and fatal UB diagnostics. The native catalog oracle
uses original codecs/private source insertion. This is modeled native evidence;
it does not execute an admitted live auction/player journey or activate accounting.

- `python3 -u -B tests/async/flatfile_auction_attribution_cases.py --native-source /workspace --artifacts /workspace/bin/tests/auction-attribution`:
  exit 0, 239.685969s, all 61 observations,
  zero selected skips, original native/backend and complete owned family hashes
  unchanged. Generated native/independent compiler/link commands and executable
  hashes are retained alongside each cut. The standalone `-MM` result contains
  no `src/` dependency.
- `python3 -u -B -m unittest -v test_backup_review_remediations`: exit 0, 0.767502s.
- `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py`: exit 0, 667.031353s.
- `python3 -u -B -m unittest -v test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration`: exit 0, 234.309710s. Required environment: `DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/auction-attribution-managed`, `DURIS_RUN_BACKUP_INTEGRATION=1`.
- `make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server`: exit 0, 295.436325s; actual 740
  production compilations, no reused-without-compilation objects, no warnings,
  740 byte-identical objects and 1290 authenticated dependency bodies/modes.
  Server SHA256: `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`. The managed driver transparently uses
  the separately preserved matching native server; its fresh-build flag is false.
  The maintained Make result independently binds that executable to this source.
- `python -B scripts/validate_economy_accounting.py`: exit 0.
- `python -B -m unittest discover -s tests/async -p test_economy_writer_coverage_contract.py`:
  exit 0; all 55 writer-coverage methods pass without skips.
- `python -B scripts/validate_economy_accounting.py --release`: expected exit 1
  for missing writer executable evidence.
- Ten changed C++ formatting checks, Python syntax and `git diff --cached --check`
  pass. The complete `wsl --cd` / `bash ./scripts/format.sh --check` invocation,
  including all ten explicit `--file` arguments, is retained in
  `flatfile-auction-attribution-gates-06-20261007/results.json`.
  Inventory 920 registers this helper through the mandatory owned authority driver;
  discovery and isolated passing tests do not establish release completion.

The complete native driver retains 20 healthy/367 refusal controls, 28 money-history,
50 native-domain, 38 wallet/bank, 116 auction-value, 50 source-balance and
61 new creator observations, plus its original metadata,
envelope, history, namespace, baseline, pagination, lock and budget controls.
Backup review passes 18 methods. Managed backup passes one method with two actual
isolated service boots/two retained generations, pending journal read-only
refusal, replay, UID, fault and prune checks. Selected native/managed skips are 0.
SQL checks are not repeated for this flat-file-only change: SQL code/schema is
unchanged, and neither MySQL nor MariaDB nor the newer combined candidate is
qualified by these results.

Five incomplete precursor runs remain recorded; none is promoted:

- `flatfile-auction-attribution-green-01-20261007`: exit 1, 67.295623s; incomplete qualification retained.
- `flatfile-auction-attribution-green-02-20261007`: exit 1, 210.003858s; incomplete qualification retained.
- `flatfile-auction-attribution-green-03-20261007`: exit 1, 210.365370s; incomplete qualification retained.
- `flatfile-auction-attribution-green-04-20261007`: exit 1, 210.461431s; incomplete qualification retained.
- `flatfile-auction-attribution-green-05-20261007`: exit 1, 222.258739s; incomplete qualification retained.

They expose the fixture record argument, required append-before-source staging
order, explicit codec success comparison, the reader's corrected actor constant,
and required initialization of an empty native source catalog. Final evidence
comes only from the rebuilt successor and complete final drivers.

Host preparation observations also preserve the early launch refusals before
the runner files were ready. An early sealing attempt refused while the full
container was still copying artifacts: its result/guard files precede that copy.
The successor seal waits for both container and host process termination. These
orchestration observations do not restart or qualify a live process, and are
retained with the final evidence.

## Evidence and curator packet

Evidence root: `D:/CodexEvidence/accounting-plan5/bin`.

- Original red: `flatfile-auction-attribution-red-01-20261007/`.
- Final focused source/guards/cuts: `flatfile-auction-attribution-green-06-20261007/`.
- Complete native/backup: `flatfile-auction-attribution-full-01-20261007/`.
- Maintained Make: `flatfile-auction-attribution-make-01-20261007/`.
- Final validator/contracts/formatting: `flatfile-auction-attribution-gates-06-20261007/`.
- Refreshed primary/checkpoint/codec diff: `flatfile-auction-attribution-primary-refresh-02-20261007/`.
- Terminal artifact seal: `flatfile-auction-attribution-seal-01-20261007/evidence.json`;
  SHA256 `a198496e1ac9cb684d4ab5e1235ea3b5ba7f9020ac8a70b8ef61324429c57bfc`, 7465 artifacts/2635951095 bytes.
- Canonical commit/remote delivery: `flatfile-auction-attribution-delivery-01-20261007/delivery.json`.

The report, remote follow-up, terminal seal and delivery receipt form the narrow
curator packet for the primary's locally maintained notebook. Application,
acknowledgement and cross-chat notification are not claimed. The user's notebook
delegation is nonblocking. Evidence is retained outside Git; no credentials,
real account/player data, archives or compiled products are committed.

## Remaining gates, limits and genuine findings

- `claim_source_credit_roots_verified` covers matched creation facts/rows/receipts.
  Broader `claim_source_attribution_verified`, claim-source digest, canonical
  consumption order, complete origins, cross-epoch continuity, full native
  holdings, R7/R8 and release flags remain false. A valid historical creator does
  not prove an unknown or wrongly ordered consumer.
- The AEC1 zero-before birth control can verify credit creation while the current
  money reader reports `unknown_legacy_origin` for its unanchored new lifetime.
  That wider origin policy needs its own complete qualification; it is not waived.
- Removal with a prior winner retains an active escrow mapping and unchanged held
  money in its original plan, while the existing current catalog reader excludes
  closed listings and reports `native_domain_missing`. Its credit recipe has no
  positive sources. Qualifying the closed cancellation holding requires a
  separate independent policy/reader slice, with primary owning any contract
  change. This finding is preserved without correction.
- Zero-credit controls explicitly model an initialized empty source catalog with
  positive revision, as required by the native codec. They do not prove that a
  live producer creates that file. Missing source/catalog evidence remains a
  refusal/finding, including for zero balances; no empty catalog is synthesized
  by the operator. Original escrow/listing/winning-bid origins remain unproved.
- Current primary's version-2/native/schema 64 composition, exact combined-head
  backends, genuine producer/player persistence/load/restart journeys,
  production-style retention/restore and release-host evidence remain necessary.
  Primary integrates slices and publishes the tested combined candidate.

There is no external blocker to continuing independent Plan 5 work. Accounting
stays inactive; wallet-root item exclusions and the declined inactive spell-path
change remain preserved. No experimental-accounting push, activation, merge,
deployment, production mutation or audit autocorrection occurs.
