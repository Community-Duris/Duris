# Plan 5: independent current auction escrow and claim comparison — 2026-10-07

`--economic-auction-money-audit` now compares current native open-auction escrow
and pending-claim copper amounts and their individual revision clocks with
independently authenticated economic endpoints for the catalogue's current epoch.
It reports disagreements and missing or extra accounts without mutation or recovery.
The complete native auction and claim-source catalogs are structurally decoded.
Claim-source attribution, account origins, other money domains, all native holdings,
full R7/R8 and release completion remain unqualified.

## Source and ownership

- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Local and remote branch: `codex/accounting-plan5`.
- Base: `537b04d10f93e285d5de47617d44ac6fb7a95d46`. The delivery receipt records the result SHA and remote equality.
- Final tested canonical archive: `386f9d8f5fe5e7af88575b4f3ca6640e3395c14c9a06ee6e929b37f67950c789`.
- Frozen code tree: `130466cd8c08a3b6d47d49de6319879d705e4400`; 6407 regular bodies, four links and canonical Git file modes.
- Native tree: `4abb609524a1f1682ea4c190f82d75003c4d679b`; unchanged canonical migrations0–62 tree: `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`.
- Refreshed primary: `d3e34519ae8701baa63d97380340760859dcb452`, native `2d0453e1e1d1755ae83fe42dbd261b8157eafd23`,
  migrations `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. Its six selected auction interfaces
  are byte-identical; that distinct combined source is not qualified by this slice.

The same remote branch retains the seven earlier ancestral tips and every published
Plan5 prerequisite. Primary owns shared contracts, coordinator, producers,
registry/matrix, combined integration and activation. No shared interface or schema
change is requested. Native production code, migrations and shared runners/manifests
remain exact. These eleven owned code files and this report/remote follow-up change:

| File | Purpose |
| --- | --- |
| `scripts/qualify_flatfile_auction_money.h` | Read-only current escrow/claim mapping and native comparison |
| `scripts/qualify_flatfile_current_money.h` | Existing history, mapping, lock, budget and findings logic shared by two actual readers |
| `scripts/qualify_flatfile_native_auction.h` | Pure complete native catalog, receipt and source decoding |
| `scripts/qualify_flatfile_native_domains.h` | Existing envelope reader gains explicit auction size/version limits; old defaults preserved |
| `scripts/qualify_flatfile_restore.cpp` | Scoped auction command and shared bounded finding serialization |
| `scripts/qualify_flatfile_wallet_bank.h` | Uses common comparison core, preserving wallet/bank behavior |
| `tests/async/flatfile_auction_money_cases.py` | Original-decoder differential and sanitizer/operator controls with retained private inputs |
| `tests/async/flatfile_auction_money_fixture.cpp` | Original native codec and query oracle in disposable roots |
| `tests/async/flatfile_restore_authority_fixture.cpp` | Modeled escrow/claim baselines, current epochs and retired lifetimes |
| `tests/async/test_flatfile_restore_baseline_markers.py` | Pins the additional helper in the complete audit source family |
| `tests/async/test_flatfile_restore_economic_authority.py` | Registers controls through the existing mandatory native driver |

## Established defect and behavior

In a new private disposable root the original native codec records escrow70/clock3
and claim60/clock4. Original accounting codecs record modeled baseline endpoints
with those values. A correctly hashed catalog changes escrow70 to900. The public
native query observes900 while the retained-only history audit exits0 and verifies
continuity ending at70. Both preserve the retained inventory. The red process
exits0 in 205.667291s, documenting the absent current comparison.

The new reader reports `native_balance_mismatch` with native `[900,0,0,0]`,
expected `[70,0,0,0]` and both clock3. Current locators resolve immutable economic
account identities. Catalogue order selects the current epoch; earlier epochs
remain separate and cannot substitute for current history. Missing accounts or
history, extra native holders, nonzero retired balances and invalid histories
remain findings. The existing wallet/bank unknown-origin rule remains unchanged.

Only an open listing with a winner holds its current copper price; an unbid open
listing holds zero. Closed/removed listings retain historical prices but contribute
no current escrow holding. Native claim rows, including zero amounts, are current
holdings. Listing and claim clocks are used directly; the catalog envelope clock
does not substitute for either. Signed native prices remain exact, including
INT64_MIN; negative values cannot wrap into economic balances. UINT32_MAX auction
IDs and UINT64_MAX revision clocks remain exact in JSON. An unmapped claim PID
above the economic signed PID limit cannot alias a valid mapped PID.

The pure decoder validates complete catalog versions1/2, hashes, body lengths,
strictly ordered listings/pickups, nonzero clocks/identities, canonical seller
accounts, bounded strings/blob/items, item uniqueness/flags, unique native receipts,
receipt action/item/claim-credit bounds, versioned publication flags and full body
consumption. It preserves native acceptance of receipt event codes and padding.
It matches the original native refusal of a zero-length object blob. Claim-source
version1 rows enforce exact length/count, nonzero operation/slot/lineage/PID/mapping,
amount limits, sorted unique keys and no self-consumption. Structural acceptance
does not prove the referenced accounting operation or consumption provenance.

Public native auction queries acquire authority and run recovery, so the operator
reader never calls them. The independent decoder and current reader have no `src/`
provider in their measured `g++ -MM` closure. Existing independent authority and
history checks are reused by the two actual current-money readers; mutation code
is neither imported nor used for reconciliation.

The existing authority lock is opened read-only without creation and held shared
through the current scan. All three pending native journals, an exclusive writer,
missing lock for a nonempty scan, malformed frames, nonprivate/hardlinked/symlink/FIFO
files and expired/exhausted audit budgets refuse with exit1, no stdout and fixed
stderr `native_restore_qualification_failed`. Root/domain/lock identity and catalog
authority/epoch are rechecked before output. Budgets remain128MiB total bytes,
16384 files,8192 directory entries and30 seconds. Current account scans stop at32768;
published findings stop at100 while preserving the exact aggregate count.

## Operator contract

After building the existing listener-free restore qualifier in a private output
directory, run its executable against a private absolute flatfile root:

```sh
QUALIFIER --economic-auction-money-audit /absolute/private/flatfile-root
```

JSON format is `flatfile_auction_money_audit_v1`, scope
`current_auction_escrow_claim_values_and_revisions`. It reports current epoch,
active/retired/prior account counts, native catalog/source presence and clocks,
listing/receipt/source counts, compared holders, exact finding totals and bounded
opaque findings. Findings contain epoch/operation/account identities, native kind
and ID, and optional native/expected denomination vectors and revision clocks;
seller/buyer names, account aliases and object descriptions are not emitted.

Exit0 means no findings in this scope. `current_auction_money_values_verified`
requires a current initialized epoch and matching tails for every active selected
mapping. `claim_source_attribution_verified`, `account_origins_verified`,
`cross_epoch_continuity_verified`, `other_money_domains_verified`, global
`native_holdings_compared`, `full_R7_qualified` and `release_qualified` remain false.
A missing source file may coexist with matching current claim values; its absent
presence flag and false attribution flag prevent a provenance claim. Native
inactive holdings without economic mappings remain unmapped findings; an empty
inactive root stays an empty, unverified observation and is never initialized.

## Exact checks and evidence

All disposable processes use pinned Linux image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
isolated tmpfs workspaces, private fixture roots, no network,2CPU and4GiB.
Backend is native flatfile with original `__NO_MYSQL__` providers. No SQL endpoint,
production data or user server is used. Original native providers and modeled
economic witnesses are distinguished explicitly; no live auction producer journey
or complete source provenance follows from these modeled baselines.

| Command/check | Exact result |
| --- | --- |
| `python3 -u -B tests/async/flatfile_auction_money_cases.py --native-source /workspace --artifacts /workspace/bin/tests/auction-money --red` | Red established; exit0, 205.667291s |
| Same focused helper without `--red`, first archive |108 controls, exit0, 231.978070s; superseded only by eight added controls |
| `python3 -u -B /evidence/focused-invocation.py` calling the final helper |116 controls, exit0, 23.825350s,0 skips; exact invocation file and binary bindings retained |
| `python3 -u -B -m unittest -v test_backup_review_remediations` |18 methods, exit0, 0.867420s,0 skips |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` |20 healthy/367 damaged stores;28 money-history,50 native-domain,38 wallet/bank and116 auction controls; exit0, 578.329017s |
| `python3 -u -B -m unittest -v test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration` |Complete method; 2 actual boots, 2 retained generations, pending-journal refusal, replay/dedupe/UID/fault/prune controls; exit0, 208.409880s,0 skips |
| `make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server` |740 observed fresh object compilations,0 objects reused without compilation; exit0, 285.142004s,0 warnings/errors |
| `python -B -m unittest discover -s tests/async -p test_economy_writer_coverage_contract.py` |55 methods pass,0 skips |
| `python -B scripts/validate_economy_accounting.py` |Exit0 |
| `python -B scripts/validate_economy_accounting.py --release` |Expected exit1: writer has no executable evidence; no waiver |
| Changed-line `scripts/format.sh --check`, staged whitespace and Python syntax |All pass;920 registered tests; new helper registered through existing authority driver |

The full driver also preserves its existing semantic, metadata, envelope,
boundary, budget, page, baseline/history and namespace controls. Its exact
aggregate receipt is retained rather than inferred from inventory coverage.
Of the116 final observations,76 directly compare original native decoding with
the independent sanitized decoder; the remaining controls exercise the regression,
current audit and its operating boundaries. Standalone pure-reader compilation uses C++20, strict warnings/Werror and
ASan/UBSan; its full command, dependency closure and input objects are retained.
The final focused invocation authenticates and reuses three original native test
executables from the first green archive because every compilation input's bytes
and modes remain equal; only the Python case list changed. It freshly compiles the
sanitizer reader. The full authority command independently rebuilds the original
fixture/operator/oracle. This reuse is not labelled as a fresh compilation.

Make authenticates all1304 native inputs and1290 measured dependency inputs;
740 resulting objects and server are byte-identical to the original native build.
Server SHA256: `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`. The managed method uses that same authenticated
server from the earlier build and does not claim a fresh managed server link.
Terminal guards authenticate all source bodies, canonical modes and four links.
Final focused controls retain complete regular-file input objects and before/after
root/file metadata and symlink targets. Artifact copying is complete and original
process/container handles are terminal before sealing.

Evidence root: `D:/CodexEvidence/accounting-plan5/bin/`.
Directories: `flatfile-auction-money-red-01-20261007`,
`flatfile-auction-money-green-01-20261007`, `flatfile-auction-money-green-02-20261007`,
`flatfile-auction-money-full-01-20261007`, `flatfile-auction-money-make-01-20261007`,
`flatfile-auction-money-gates-02-20261007`, `flatfile-auction-money-primary-refresh-01-20261007`
and both changed-line format receipts. Seal:
`flatfile-auction-money-seal-01-20261007/evidence.json`, SHA256 `d85fbc0784dfa9e8614cc575c7945b1b26bd8d3b12a180b01a53d4488d081c9f`,
authenticating 5948 artifacts/1387855945 bytes.
Delivery follows at `flatfile-auction-money-delivery-01-20261007/delivery.json`,
with result/remote SHA, all nonpublication payloads/modes/links, owned-file closure
and seven earlier tips checked. The first publication helper's Windows default
encoding failure and dependent owned-file preflight refusal are preserved in
`flatfile-auction-money-publication-encoding-observation-01-20261007/evidence.json`.
Explicit UTF-8 decoding repairs publication; all native source and sealed runtime
evidence remain exact and native checks are not repeated. Generated evidence and private inputs are not committed.

## Remaining gates and curator handoff

This closes the current escrow/claim amount-and-clock comparison gap on the exact
owned flatfile source. Claim-source root/slot/mapping/consumption attribution,
unknown native origins, full UID/live/tombstone/current-owner/reference histories,
native piles/treasury and all money domains remain open. Native producer/player,
source-reuse/evidence-loss, production-size latency/load and complete restore/
retention guarantees still require their exact native journeys. Full R7/R8 and
combined release qualification remain open. Isolated fixtures, inventory coverage
and these passing checks cannot close those gates.

MySQL8/MariaDB10.11 fresh/upgrade/live journeys were not repeated for this pure
flatfile reader slice; no schema or SQL/native writer changes occur. Their exact
combined-candidate qualification remains required, including canonical0056 and
later primary integration. Earlier0055 outcomes do not qualify that candidate.
Release validation's missing executable writer evidence remains a gate. No
environment, notebook or permission blocker prevented this owned slice.

This report, remote follow-up and sealed receipts are the packet for the primary's
locally maintained notebook curator workflow. Notebook application, cross-chat
notification and primary acknowledgement are not claimed. Accounting stays
inactive. Wallet-root item exclusions and the declined inactive spell-path change
remain exact. No activation, audit autocorrection, production mutation, deployment,
merge or independent push to `experimental-accounting` occurs.
