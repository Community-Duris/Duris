# Plan 5 bounded retained money-history qualification

Four individually valid native ATM records could describe mutually overlapping
wallet/bank histories while the whole retained-operation reader accepted them.
The new opt-in read-only command compares account transitions across accepted
roots and reports the overlap. It also joins a baseline to ordinary effects
using the witnessed native holding revision: the baseline posting's synthetic
0-to-1 initialization is a separate clock. This slice qualifies bounded retained
same-epoch continuity; full reconciliation and release remain open.

## Source, branch and ownership

Work and publication remain `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base is `4f277dcf806ecca667e141ac8628bb06cf0f8e71`, the separately published
[managed pending-source repair](PLAN5_PENDING_SOURCE_AUDIT_QUALIFICATION_2026-10-07.md).
Result/remote commit SHAs and preservation of all seven earlier branch tips are
bound by `flatfile-money-history-delivery-01-20261007/delivery.json` under
`D:/CodexEvidence/accounting-plan5/bin/`. No other publication branch is used.

The completed run preceding the mode supplement uses archive SHA256
`dde75eb772b0380021a69b337cca25922e47c061b64baef4c5ed776d75542144`.
Native tree is `4abb609524a1f1682ea4c190f82d75003c4d679b`; migration tree is
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical source0062.
The final archive overlays exactly these eight owned code inputs:

- `scripts/qualify_flatfile_economic_baseline.h`
- `scripts/qualify_flatfile_economic_records.h`
- `scripts/qualify_flatfile_economic_money_history.h`
- `scripts/qualify_flatfile_restore.cpp`
- `tests/async/flatfile_restore_authority_fixture.cpp`
- `tests/async/flatfile_money_history_cases.py`
- `tests/async/test_flatfile_restore_economic_authority.py`
- `tests/async/test_flatfile_restore_baseline_markers.py`

This report and the existing owned remote follow-up are publication files.
Final preflight/delivery verify every other source payload, mode and four links
against the qualified canonical-mode archive described below. Shared coordinator, contracts, producer, registry/matrix,
migrations, runner/manifests and activation files are untouched. No shared
interface/schema change is requested. The optional validated-plan observer and
verified-baseline-byte return are entirely inside the owned independent reader.

## Established defects and reader behavior

`flatfile-money-history-red-01-20261007` builds frozen base inputs and uses the
original native `paged-records` fixture. Its four successfully retained ATM
records span two physical buckets, with the same wallet/bank identities and
four overlapping 0-to-1 transitions. The original economic reader returns0,
and complete retained content/metadata stays unchanged. This is an actual
cross-record omission; individual plan validation already passes.

A second native check in `flatfile-money-history-clock-red-01-20261007` exposed
an error in the first continuity implementation. The actual baseline writer
retains native revision42, while its balanced baseline posting initializes
0-to-1. The actual currency preparation produces an ordinary42-to-43 transfer.
The original authority reader accepts both; the first continuity implementation
incorrectly reports `revision_gap`. The final fixture and fix preserve the two
distinct clocks and accept that actual native combination.

The baseline verifier now returns the same fully authenticated encoded witness
that it checked. After complete per-record validation, the optional accepted-plan
observer receives the plan and that witness. A baseline ordinary account becomes
an origin anchor with its held balance and the witness's `native_revision`
(offset72 in the112-byte holding); the synthetic posting revision is not used.
Default existing readers have no observer. Whole-scan closure validation still
finishes before the new command emits any positive result.

```sh
bin/tools/qualify_flatfile_restore --economic-money-history-audit /ABS/PRIVATE/ROOT
```

The command holds the existing read-only authority lock, refuses pending native
authority transactions, and uses only the independent retained decoders. It
collects accepted ordinary kinds1-6 and11; rejected envelopes and system-account
kinds7-10 contribute no edges. Every group uses the epoch and full40-byte account
key, preserving lifetime, kind and context identity. Baseline anchors precede
ordinary edges. Revision order defines continuity; physical append and operation
ID order do not. Four-denomination vectors must match exactly at each boundary.
Valid revision jumps, revision-only balance observations and same-revision
observations remain accepted. No state is repaired.

Exit0 means retained same-epoch transitions checked without a finding. The JSON
contains `accepted_roots`, `ordinary_account_edges` (including baseline anchors),
`epoch_accounts`, `baseline_anchored_accounts`, `unanchored_accounts`,
`invalid_accounts`, `same_epoch_transition_continuity_verified`,
`findings_truncated` and `findings`. A finding exposes only epoch/operation/account
IDs and one of `overlapping_revision`, `revision_gap`, `balance_discontinuity`,
or `repeated_baseline_origin`; one finding is returned per invalid account.
Findings produce exit1. Malformed source, writer exclusion or budget refusal
produces exit1, empty stdout and fixed `native_restore_qualification_failed`
stderr, without a partial positive certificate.

All outputs explicitly retain `account_origins_verified:false`,
`cross_epoch_continuity_verified:false`, `native_holdings_compared:false`,
`full_R7_qualified:false` and `release_qualified:false`. Unknown-origin groups
and empty inactive stores can pass continuity without proving origins or holdings.
Authenticated baseline presence does not prove capture completeness.

## Bounded execution and original checks

Accumulation stops at32,768 ordinary rows, including baseline anchors; the next
row throws typed `edge_budget_refused`, derived from the existing audit-budget
refusal. At most100 findings are retained even when more accounts are invalid.
The measured edge struct is160 bytes, so its maximum payload arena is5MiB;
this is not a whole-process RSS measurement. Existing128MiB physical-byte,
16,384 physical-read,8,192 directory-visit and30-second cooperative limits remain.
Larger histories refuse rather than claim completeness. Original outer process
timeouts are unchanged; release-host growing-history/latency qualification is open.

The immutable image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Containers are network-disabled,2CPUs/4GiB, with fresh3GiB workspace and2GiB
temporary tmpfs. The managed journey uses the original isolated namespaces and
tmpfs with SYS_ADMIN/unconfined seccomp. No production data, credentials, remote
game or existing database is supplied. Native cache is disabled. Fixtures and
qualifier compile fresh with original C++20 strict recipes; the standalone
independent reader uses ASan/UBSan with `detect_leaks=1:halt_on_error=1` and
`halt_on_error=1:print_stacktrace=1`. Its retained `g++ -MM` dependency output
contains no `/src/` codec include. All16 audit-family inputs, native src inputs
and owned fixture/driver/helper inputs are pinned before and after execution.

```sh
export PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1
python3 -u -B tests/async/flatfile_money_history_cases.py \
  --native-source /workspace --artifacts /workspace/bin/tests/money-history
python3 -u -B -m unittest -v test_backup_review_remediations
python3 -u -B tests/async/test_flatfile_restore_economic_authority.py
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/money-history-managed \
python3 -u -B -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

| Final command | Actual result | Seconds | Evidence directory |
| --- | --- | ---: | --- |
| Focused native/independent money-history driver |28 observations pass,0 skips |146.943028 |`flatfile-money-history-green-06-20261007` |
| Backup review regressions |18 methods pass,0 skips |0.665997 |`flatfile-money-history-full-03-20261007` |
| Complete native authority driver |Original authority cases plus28 new observations pass,0 skips |390.910391 |`flatfile-money-history-full-03-20261007` |
| Complete managed lifecycle method |1 complete method passes,0 skips |202.299964 |`flatfile-money-history-full-03-20261007` |

The28 focused observations include native forks, revision-ordered chains,
denomination discontinuity, revision gaps/jumps throughUINT64_MAX, context
separation, unchanged-balance revisions, epoch separation, actual witnessed42-to-43
continuity, fabricated1-to-2 overlap and a missing first43-to-44 edge. Original
rejected/inactive stores, malformed roots, the native65,536-reservation fixture,
all four cooperative budgets and the actual native writer lock are checked.
Semantic fixture cuts retain their exact inputs and unchanged complete stat/payload
inventories. The ordinary kind3/4 and5/6 cases are native-wire-valid altered
fixtures, not gameplay journeys. The edge/finding/stake projection unit explicitly
claims `authenticated_source_qualified:false`.

The final full driver preserves20 positive stores,367 corruption refusals,
54 native semantic decodes,1,058 metadata comparisons,574 command-envelope
comparisons,11 envelope records/10 refusals,12 boundary cases,9 audit-limit cases,
28 root-page cases,29 authority-page cases,30 baseline-control cases,
77 baseline-history cases and49 namespace cases, plus all28 new observations.
The new helper executes through the existing registered authority driver;
inventory stays920 and shared manifests are unchanged. Inventory itself is not
release evidence.

The complete managed outcome is `completed:true`: healthy-source acceptance,
read-only pending-source refusal, native pending-transaction replay, WAL drain,
critical dedupe, UID high-water advance, preserved old inactive receipts and two
actual isolated service boots. Two generations remain and the unretained oldest
generation is pruned. Both manifest loss/corruption probes, a checksum-valid
corrupt receipt and a required receipt absent before capture refuse before boot.
Original complete source/generation preservation and16 terminal audit-input
hash checks pass. Its certificate still has `full_R8_qualified:false` and
`accounting_activated:false`.

```sh
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile \
  BIN_ROOT=/workspace/bin/tests/money-history-build/bin \
  OBJDIR=/workspace/bin/tests/money-history-build/objects \
  DMS_BINARY=/workspace/bin/tests/money-history-build/server
```

`flatfile-money-history-flatfile-build-01-20261007` clean-builds740 objects and
dependency records,0 reuse,0 warnings/errors, in285.220510 seconds, from archive
`a8cd6811d19012fb884184c486a730474aa1c36ec68e73ac5112c0b743e1d709`.
After the final audit/fixture changes, `flatfile-money-history-make-followup-01-20261007`
runs the same Make command against final source in1.387343 seconds, verifying
all1,290 workspace dependency inputs by payload/mode, all740 reused object hashes
and server hash. No fresh native recompilation is claimed in that follow-up.
Native build inputs remain identical. The server actually booted is authenticated
SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`,
also verified against the managed journey's retained server origin. Final audit
headers, standalone reader and native fixtures compile fresh on final source.

```sh
git diff --check
python -B scripts/validate_economy_accounting.py
python -B scripts/validate_economy_accounting.py --release
python -B -m unittest discover -s tests/async \
  -p test_economy_writer_coverage_contract.py
wsl --cd "C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max" \
  bash ./scripts/format.sh --check \
  --file scripts/qualify_flatfile_economic_records.h \
  --file scripts/qualify_flatfile_restore.cpp \
  --file scripts/qualify_flatfile_economic_money_history.h \
  --file scripts/qualify_flatfile_economic_baseline.h \
  --file tests/async/flatfile_restore_authority_fixture.cpp
```

Final `flatfile-money-history-gates-02-20261007` records all five outcomes:
diff/normal validation0, release expected1 for `writer has no executable evidence`,
55 writer-contract methods pass with0 skips, and authoritative clang-format14
changed-line check0. The immutable tools image lacks clang-format; the existing
WSL installation performs formatting without an installation or bypass.

## Evidence, historical failures and remaining gates

Protected evidence is under `D:/CodexEvidence/accounting-plan5/bin/`.
`flatfile-money-history-seal-01-20261007/evidence.json` has SHA256
`ed316f7aefe3a18a2cc21ca470a40307e12f9852e67d5030dc1eb82f220e6027` and authenticates 36,493 artifacts/7,841,477,488 bytes
across16 attempt directories. It binds final source, checks, original commands,
logs, retained cuts, binary provenance, terminal container status and immutable
helpers. Preflight/delivery separately bind the canonical result and published
remote SHA. Artifacts and local logs are not committed.

All failed attempts remain: green01 had a harness expected-count error; green02
and03 had an ambiguous generated C++ `checker` name. Green04/05 and full01/02
passed their then-tested scope but preceded the discovered native baseline-clock
boundary; they are superseded for this final qualification. Both native red runs
and their source archives remain. Final green06 and full03 share exactly the
final archive above. No failed test, timeout or metadata/source drift is hidden.

This is a flatfile-only slice. MariaDB/MySQL disposable methods are not repeated;
no SQL decoder, native producer or migration input changes. Native-wire fixtures
and synthetic projection units do not establish active gameplay or full release.
Remaining Plan5 gates include comparison with actual current wallet/bank/pile/
escrow/claim/treasury/stake holdings, known creation/origin completeness, cross-epoch
continuity, UID/topology/full capture, real producer journeys, erasure/lifecycle
qualification, growing history/checkpoints, release-host budgets and combined R7/R8.
No independent scoped blocker or selected skip remains for this solved slice.

Latest refreshed primary is `b41a7d1df27a14eca732f279c9a0451dc33b224f`, native tree
`2d0453e1e1d1755ae83fe42dbd261b8157eafd23`, canonical62. Its latest checkpoint
reports literal-capacity, warm/full-cold and genuine pending-journal/origin-fault
passes on both SQL engines, including cold runs within the original60-second
limit. The earlier cold-origin timeout is historical. Those primary-reported
protected artifacts are not independently inspected by this slice. Its private
auction LIST failure is now diagnosed as custody INSERT1062 for a UID retained
by a claimed row. Its private additive schema64 now has measured MySQL8/MariaDB10.11
canonical migration and both interrupted-DDL resume routes, preserving original
application/history rows; all238 commands have expected outcomes. Runtime
fingerprints, the offline contract, both original753-provider links and nine
contracts pass on that private source. Original producer reruns and maintained
producer/gameplay integration remain open. This branch has not integrated that
new native source; this slice does not qualify any primary combined candidate.
Primary must integrate the published slice and test/publish the combined result.

The immutable runtime seal retains the earlier test-time refresh3461a662. The
subsequent b41a7d1d refresh adds only documentation over that native/migration
tree and is bound separately by
`flatfile-money-history-primary-refresh-01-20261007/evidence.json`, SHA256
`6ca10195fdb4f89411724c383b5b0556aa92896f461ff57d1f13317ae194a8a1`.
It retains the four exact latest checkpoint/plan source documents. Future
RAM-authority guidance is forward-only and preserves the current independent
audit/ownership boundary; it does not reopen this qualified slice or add gates.

Accounting remains inactive. Wallet-root exclusions and the declined inactive
spell-path change remain exact. No activation, production data mutation, audit
autocorrection, deployment, PR merge or independent experimental-accounting push
occurs. Full R7, R8 and release flags remain false. Primary's locally maintained
shared notebook is nonblocking; this report, the owned remote follow-up and sealed
receipts form its curator packet. Application, direct notification and primary
acknowledgement are not claimed.


## Canonical Git mode supplement

The first publication preflight correctly refuses exact source metadata equality.
The earlier transport created the two newly added inputs at0644; canonical Git
archive uses0664. Both files' bytes/SHA256 are identical, and every other
nonpublication payload/mode plus all four links is exact. No global Git setting
or archive default is changed. The failed helper, two-row mismatch and failure
receipt remain in `flatfile-money-history-preflight-01-20261007`.

`flatfile-money-history-mode-full-01-20261007` re-executes all three original full
commands above against canonical staged-tree source archive SHA256
`7ab17f6829313db9654f028b388d12efd0236a8c0fb72b7601cbd1df5bfb20d0`. Only these two runtime-input
metadata differences are qualified:

| Input | Earlier transported mode | Canonical tested mode |
| --- | --- | --- |
| `scripts/qualify_flatfile_economic_money_history.h` |0644 |0664 |
| `tests/async/flatfile_money_history_cases.py` |0644 |0664 |

| Canonical-mode command | Actual result | Seconds |
| --- | --- | ---: |
| Original backup review regressions |18 methods pass,0 skips |0.716576 |
| Complete native authority driver |20 stores/367 refusals, all original pages and28 money observations pass,0 skips |387.204317 |
| Complete managed lifecycle method |1 method passes,2 real boots/all retention and fault assertions,0 skips |201.865460 |

Fixtures, qualifier and standalone independent reader compile fresh on that
canonical source. Both terminal source/audit-input guards and full retained
inventories remain unchanged. The same authenticated native server/input closure
is reused, with no fresh production recompilation claim. All prior original
checks remain retained. This supplement includes the exact later primary
checkpoint and the failed preflight; it grants no new SQL, holdings, origin,
gameplay, R7/R8 or release qualification.

An independent source observation during the native rerun verifies all6,395
archive files' actual modes/payloads and all four links inside the container;
both added inputs are actually0664. The protected live source-mode observation
is included in this supplement. It observes build inputs, not authority data.

`flatfile-money-history-mode-supplement-01-20261007/evidence.json` has SHA256
`bc5bd65dd94856110bd8ce85f2a10687e57b64b128b8b4ba24e0cea3020cf041`, authenticating 3,978 artifacts/449,606,421 bytes.
Final delivery records both earlier and canonical-mode archive SHAs and these
two explicitly qualified metadata differences. Final successful publication
preflight is `flatfile-money-history-preflight-03-20261007/result.json`;
result/remote receipt is `flatfile-money-history-delivery-01-20261007/delivery.json`.
Final publication prose changes are excluded from runtime-input comparisons.
Every runtime payload, canonical mode and all four links match the qualified
archive. Failed preflight01 remains historical evidence, never a passing claim.

The publication helper initially reads the existing UTF-8 follow-up with the
Windows cp1252 default and refuses decoding after writing this report's owned
supplement. Explicit UTF-8 reading repairs that documentation-only failure;
the existing follow-up prefix stays byte-exact. Its partial helper/documents and
failure receipt remain in `flatfile-money-history-publication-repair-01-20261007`,
with evidence SHA256
`40b5db9865c2fa44fb13f2f3d8e2834b9e8f464deb178ec13c1bf0202611f5b2`.
Successful preflight02 is retained as the preliminary runtime comparison;
preflight03 binds both complete final publication documents. No runtime input
or qualification result changes during this repair.
