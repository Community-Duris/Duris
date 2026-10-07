# Plan 5: native auction claim-source balance reconciliation — 2026-10-07

`--economic-auction-source-balance-audit` now checks remaining source totals
against native pending claims and retained immutable beneficiary lifetimes under
the same read lock as current economic/native value comparison. It reports
findings without recovery, mutation or correction. Credit-root provenance,
frozen source digests and canonical consumption order remain separate open gates.

## Exact source and ownership

- Local and remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `af0c8eb229d3627565e6b0da8d3c9753d58388b6`. Result/remote equality is recorded in
  `D:/CodexEvidence/accounting-plan5/bin/flatfile-auction-source-delivery-01-20261007/delivery.json`.
- Tested canonical archive SHA256: `071d30358a3364e441ba1906f15987e1ea85661d949b354ff40b69554f939114`.
- Frozen code tree: `5e8265410d171d28063b565b23191d732cad00df`; 6410 regular bodies,
  four links and canonical Git modes. Publication changes only this report and
  the remote follow-up; all other payloads, modes and links are verified exact.
- Unchanged native tree: `4abb609524a1f1682ea4c190f82d75003c4d679b`.
- Unchanged canonical migrations0–62 tree: `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`.
- Refreshed primary: `65730c44362866a1f86bc268693014716fe21547`, native
  `d425eb9ecf1e40e433315ed106731b7c3f9b8fce`, migrations
  `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`. Four selected auction interfaces
  match byte-for-byte. Its distinct753-provider/newer-schema combined candidate
  is not qualified by this slice.

All seven earlier alternate-branch tips remain preserved ancestors. Their
follow-ups and new slices continue on the same expected remote branch. Primary
owns shared coordinator/contracts/producers/registry/matrix/activation and
combined publication. No shared interface or schema request is needed for this
balance-only slice. Native source, migrations and shared runners/manifests are
untouched. These ten owned code files change:

| File | Purpose |
| --- | --- |
| `scripts/qualify_flatfile_auction_money.h` | Second actual consumer observes the existing coherent scan |
| `scripts/qualify_flatfile_auction_source_balance.h` | Independent remaining totals and retained beneficiary lifetimes |
| `scripts/qualify_flatfile_current_money.h` | Mapping/finalization observers under the existing read lock |
| `scripts/qualify_flatfile_native_auction.h` | Existing structural decoder supplies scalar source rows |
| `scripts/qualify_flatfile_restore.cpp` | Scoped source-balance CLI and bounded findings |
| `tests/async/flatfile_auction_money_fixture.cpp` | Exact original native source-balance predicate |
| `tests/async/flatfile_auction_source_cases.py` | Native differential and sanitizer/coherence/budget controls |
| `tests/async/flatfile_restore_authority_fixture.cpp` | Original zero/retired/recreated claim fixtures |
| `tests/async/test_flatfile_restore_baseline_markers.py` | Pins the additional helper/header in the full audit family |
| `tests/async/test_flatfile_restore_economic_authority.py` | Registers 50 observations through the mandatory owned driver |

## Established defect and completed fix

The original native codec seeds escrow70/clock3, claim60/clock4 and one source
row for60 copper in a fresh private disposable root. Original accounting codecs
create modeled matching baseline endpoints while leaving accounting inactive.
A structurally valid source row changes to61 with its envelope hash recomputed.
The original `claim_source_balance_matches` returns false, while the existing
value-only audit exits0 with current values verified and attribution explicitly
false. Retained inputs remain unchanged. The red process exits0 in
187.193503s and retains its complete private native cut.

The new command reports `claim_source_balance_mismatch`. Its existing generic
finding's observed `native` vector denotes the independently accumulated source
total `[61,0,0,0]`; `expected` denotes the native claim `[60,0,0,0]`. Observed
revision fields are zero because this comparison checks totals rather than a
source-catalog clock against a claim clock. The separate current-value flag is
still true and the source-balance flag false. Deficits are also reported.

Every row resolves the exact lineage, immutable pending-claim mapping and
context0, then checks the retained beneficiary. A native PID's old lifetime cannot
fund its recreated mapping. Unknown lineage/mapping and wrong beneficiaries are
findings even on consumed rows. Only unconsumed rows in matching active lifetimes
fund current totals. Retired spendable rows are findings; consumed retired rows
remain valid retained history. A matching new source cannot manufacture missing
economic history. Missing rows/catalogs and sources without native claims are
reported. Present empty sources support a zero claim, while a missing source file
cannot certify even a zero claim. An empty inactive root is an unverified observation.

The complete structural native decoder and independent authority/history checks
are reused by two actual readers. The standalone `g++ -MM` closure has no `src/`
provider. Original native mutation code appears only in test oracles/fixtures.
The audit never calls the public native query, which can run journal recovery.

## Operator contract and bounds

Build the existing listener-free qualifier in a private output directory, then run:

```sh
QUALIFIER --economic-auction-source-balance-audit /absolute/private/flatfile-root
```

Format is `flatfile_auction_source_balance_audit_v1`; scope is
`remaining_claim_sources_native_claims_and_retained_lifetimes`. Output includes
catalog/source presence and revision, consumed/unconsumed/total rows, compared
claims, separate current-value/source-balance verification and bounded findings.
Findings return exit1 with JSON. Exit0 can be an unverified inactive observation.
Structural/coherence/budget refusal returns exit1, no stdout and fixed stderr
`native_restore_qualification_failed`.

Source-balance verification requires initialized authenticated current values,
present native catalog and sources, every current claim compared and no findings.
All attribution, source-consumption-order, origins, cross-epoch, other-domain,
all-native, fullR7 and release flags remain false. Arbitrary nonzero slots or
consumer IDs can satisfy this balance scope; their provenance/order still needs
independent root/digest correlation. The value-only CLI preserves its old contract.

The shared authority lock stays held through source/native/mapping comparison,
findings and final authority/epoch/root/lock guards. All three pending journals,
exclusive writers, absent locks, nonprivate/hardlinked/symlink/FIFO input, malformed
source hash/slot/self-consumption and exhausted budgets refuse without writes.
Existing budgets remain128MiB,16384 files,8192 entries and30seconds. Sources and
selected accounts each cap at32768. The next row refuses without partial output.
Maximum bounded sum32768×INT32_MAX=70368744144896 remains exact. Details cap at100,
with exact aggregate count. Current catalog epoch and source balance do not prove
historical origin or cross-epoch continuity.

## Exact checks and results

Linux image `sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
no network, two CPUs/four GiB, private tmpfs roots, original flatfile providers.
All transported bodies/modes/four links are checked at completion.1304 native
inputs and26 audit-family inputs are pinned. ASAN uses
`detect_leaks=1:halt_on_error=1`; UBSAN uses
`halt_on_error=1:print_stacktrace=1`. No production data or SQL connection is used.

- `python3 -u -B tests/async/flatfile_auction_source_cases.py --native-source /workspace --artifacts /workspace/bin/tests/auction-source --red`:
  original native gap, exit0, 187.193503s, zero skips.
- Same command without `--red`:50 observations, one preserved gap plus49
  source/lifetime/coherence/budget controls; exit0, 204.616681s,
  zero skips. Original native oracle, accounting fixture, operator and sanitizer
  reader rebuilt from the final frozen source. Every case preserves retained
  input identity; full case inventories/content objects are retained.
- `python3 -u -B -m unittest -v test_backup_review_remediations`: exit0, 1.017252s.
- `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py`: exit0, 575.574659s.
- `python3 -u -B -m unittest -v test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration`: exit0, 207.409634s.

The complete native driver retains20 healthy stores/367 refusals,28 money-history,
50 native-domain,38 wallet/bank and116 auction-money observations, plus50 new
source-balance observations. Other semantic/metadata/envelope/root/history/namespace
counts remain in the exact sealed summary. All18 backup review methods pass.
Managed recovery completes one method with zero skips, two actual isolated service
boots/two retained generations, pending-journal refusal, UID/replay, failure and
prune evidence. Its original service binary is reused with authenticated unchanged
native source; a fresh server build is not falsely attributed to those boots.

Maintained command `make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server` exits0 in
264.166083s:740 actual compilations, zero objects reused without compile,
740 byte-identical objects, no warnings/errors and1290 authenticated dependencies.
Server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4` matches the managed service.

Final whitespace, seven C++ changed-line format checks, Python syntax, normal
accounting validator and55 writer coverage contracts pass. Release validator
exits1 for missing executable writer evidence. Inventory920 discovers the existing
mandatory owner; helpers register through it without a shared manifest change.

First green build exits1 in 125.797153s
before controls: an unqualified `finding` type was ambiguous between two owned
namespaces. Explicit qualification fixes it; the failed source/log is retained.
The primary checkpoint preview encountered mixed input/output text encoding;
its exact raw Git bytes/hash were preserved without altering source. The initial
documentation-helper draft also had a Python quoting error before any write;
no native checks were repeated for that preparation issue. No live process was
restarted for a waiting timeout. All processes and artifact copies must terminate
before the seal is created.

## Evidence and curator packet

Evidence root: `D:/CodexEvidence/accounting-plan5/bin/`.

- `flatfile-auction-source-red-01-20261007/`: exact source, gap cut and original log.
- `flatfile-auction-source-green-01-20261007/`: failed build and terminal guards.
- `flatfile-auction-source-green-02-20261007/`: final canonical source,50 observations,
  native executables/hashes, standalone dependency closure, private case inputs.
- `flatfile-auction-source-full-01-20261007/`: all three commands/logs, complete
  native/managed evidence and terminal artifact copy.
- `flatfile-auction-source-make-01-20261007/`: maintained command, compiles,
  object/server hashes and dependency/source/mode/link identity.
- `flatfile-auction-source-gates-02-20261007/`, format01/02/03 and primary-refresh01
  directories with the same prefix: validation, formatting and distinct-primary evidence.
- `flatfile-auction-source-seal-01-20261007/evidence.json`: complete seal,
  SHA256 `ea41957e50ab291c67a8994ccc8b931c9f4756c9ebdb598003316cb506dd7fea`; 5002 artifacts/1365845040 bytes.
- `flatfile-auction-source-delivery-01-20261007/delivery.json`: exact base/result,
  remote equality, owned files, seven preserved earlier tips and canonical
  nonpublication bodies/modes/links after publication.

Prior auction-money prerequisite seal: `d85fbc0784dfa9e8614cc575c7945b1b26bd8d3b12a180b01a53d4488d081c9f`.
This report, the remote follow-up and sealed receipts form the evidence-bearing
curator packet for primary's locally maintained shared notebook. Application,
curator acknowledgement and cross-chat notification are not claimed; the notebook
is not a blocker. Primary independently integrates and publishes the combined candidate.

## Remaining gates, skips and blockers

Zero selected skips; no genuine blocker to further owned Plan5 work. SQL checks
are not repeated for this flatfile-only reader. Fresh/upgraded MySQL/MariaDB and
current combined native/schema qualification cannot be inferred from old0055 or
historical0056 proof. Credit roots, frozen digests/first-operation rules, canonical
whole-row consumption, other money/item origins and complete cross-epoch/R7 remain
open. Genuine current producer/player journeys, measured mixed workload/latency,
release-host restore/retention and fullR8/release remain required. No full Plan or
requirement is promoted. Inactive behavior, wallet-root item exclusions and the
declined inactive spell change remain preserved. No activation, merge, deployment,
production mutation or experimental-accounting push occurs.
