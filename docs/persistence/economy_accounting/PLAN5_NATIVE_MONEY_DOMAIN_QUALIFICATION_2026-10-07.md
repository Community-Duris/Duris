# Plan 5: independent native money-domain decoder — 2026-10-07

The independent reader now decodes the complete native wallet and bank files
without native storage, locking, recovery, or mutation calls. Its production
format rules agree with the original native loader on 50 disposable comparisons.
This is a prerequisite for native-holding reconciliation, consumed by the existing
registered qualification driver. It adds no operator CLI or current-holding join.
`native_domains_decoded` is true; `native_holdings_compared`, full R7/R8,
combined-candidate and release qualification remain false.

## Source and delivery

- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Local/remote branch: `codex/accounting-plan5`.
- Base: `45666a547e09303f027c539cae56e21c633ad1f7`. The sealed delivery receipt records the result SHA
  and verifies the matching remote after commit/publication.
- Tested canonical source archive: `376e196998ea953427468d9b2faf1ee78d26faa2f1d2b4631cb4ee6f51b94d3c`; staged tree `ff376a535bc4ce6abae6b97c10827711cb933fb4`.
- Native tree: `4abb609524a1f1682ea4c190f82d75003c4d679b`; unchanged canonical0–62 migration tree:
  `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`.
- Latest refreshed primary: `7dbc472123a729f2fedc345e5309a586ba8a02d8`, with native
  `2d0453e1e1d1755ae83fe42dbd261b8157eafd23` and migrations
  `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. Its distinct source is not
  relabelled as this slice's tested combined candidate.

The result stays on the same remote branch and preserves all seven earlier
ancestral tips listed in the existing follow-up. Nothing is independently
published to `experimental-accounting`. Primary owns combined integration.
Integration requires the preceding `45666a547` money-history slice, whose
existing driver and source-family changes this commit extends.

## Established independence requirement and implementation

The native public loader acquires an authority lock and recovers authority,
player-domain and currency journals before reading. The final executable case
removes only the lock from a disposable inactive native root. The independent
reader returns exact balances/revisions with the full inventory unchanged.
Calling the original native loader returns the same values but creates
`domains/.critical-authority.lock`; other retained files remain exact.
That observed write establishes why it cannot be the operator reader.

The new pure decoder authenticates the 56-byte envelope, exact payload size,
versions 1–4 and SHA256; checks complete body consumption; canonicalizes native
bank names and requires wallet names already canonical; and validates PID,
requested bank account/context, gameplay counters, ordered bounded deaths/zones,
base-stat revision/value rules and retained receipt structure. Nonzero unique
receipt IDs, result-size limits and version 4 quest proof are checked without
interpreting native mutation results as economic authority.

Wallet money uses `wallet_revision`, independently of the envelope's maximum
wallet/epic/frag/base-stat clock. All four denominations and unsigned native
revisions remain exact, including UINT64_MAX; a later economic comparison must
apply its own signed-vector/range rules. Per-file input stays bounded at 64KiB;
20 deaths, 1,024 zones, 512 receipts and 2,048-byte receipt payloads retain native limits.
Canonical account names are internal join inputs and are absent from emitted JSON.

`g++ -MM` records the standalone reader's dependency closure: it contains the
owned independent headers and no `src/` include or native provider. The native
fixture separately builds the original restore qualifier's complete provider
closure with original source, C++20, warnings-as-errors and no mutation stubs.
Its writer seeds only a fresh empty private disposable root; accounting is never
initialized or activated. The native loader serves solely as the test oracle.

## Exact checks and evidence

Evidence is retained under `D:/CodexEvidence/accounting-plan5/bin/`.
Frozen inputs include 6,398 regular source members, four canonical archive links,
1,304 native-source files and 18 independently pinned audit-family inputs.
Terminal source payload/mode guards pass. Each ordinary differential observation
compares complete retained bytes, inode/link counts, sizes, modes and mtimes
before and after both readers. The missing-lock demonstration records the
intentional native oracle write separately.

The same immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
uses an isolated flatfile filesystem, network disabled, 2 CPUs, 4GiB memory and
private tmpfs workspaces. The managed method retains its original disposable
service namespaces and runtime/fault budgets. No SQL service or production
authority is contacted by this flatfile-only change.

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
python3 -u -B tests/async/flatfile_native_domain_cases.py \
  --native-source /workspace --artifacts /workspace/bin/tests/native-domains
python3 -u -B -m unittest -v test_backup_review_remediations
python3 -u -B tests/async/test_flatfile_restore_economic_authority.py
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/native-domain-managed \
python3 -u -B -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile \
  BIN_ROOT=/workspace/bin/tests/money-history-build/bin \
  OBJDIR=/workspace/bin/tests/money-history-build/objects \
  DMS_BINARY=/workspace/bin/tests/money-history-build/server
```

The full driver runs with `DURIS_REGRESSION_BUILD_CACHE=off`. The independent
reader compiles with ASan/UBSan, original native fixture flags are separate,
and every selected command exits0 with zero selected skips.

| Check | Result | Seconds | Evidence directory |
| --- | --- | ---: | --- |
| Focused differential decoder |50 observations;0 skips |67.869068 |`flatfile-native-domain-green-04-20261007` |
| Backup review regressions |18 methods;0 skips |0.967945 |`flatfile-native-domain-full-01-20261007` |
| Complete registered authority driver |20 valid/367 refusals;28 money-history and50 domain observations |595.818258 |same |
| Complete managed lifecycle method |1 method,2 actual boots,2 retained generations;0 skips |229.236637 |same |
| Maintained flatfile Make |740 compiler invocations;0 warnings/errors;1,290 dependency inputs verified |362.638603 |`flatfile-native-domain-make-01-20261007` |

The native driver also retains its original 54 semantic, 1,058 metadata,
574 command-envelope,12 boundary,9 budget,28 root-page,29 authority-page,
30 baseline-control,77 baseline-history and49 physical-namespace cases.
The managed method asserts replay/drain, source dedupe, UID advancement, old
inactive receipts, retention/pruning, all four pre-boot fault refusals, pending
source audit refusal, and unchanged source/generation inventories. Its certificate
still declines full R8 and activation. The managed server is an authenticated
earlier build of the identical native tree, not a new server compilation for that
method; its SHA256 is `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`.

Canonical source timestamps trigger 740 actual compiler invocations during Make.
The copied original helper labels byte-identical results as 740 reused objects and
declines fresh compilation. Its original log/results are preserved.
`observed-build-classification.json` supersedes that classification with 740
recompiled objects and zero objects used without compilation; it binds all
compiler input/output pairs and the original log. Recompiled object/server
digests match the separately sealed earlier native build.

`flatfile-native-domain-gates-01-20261007` records exact commands/results:
normal validator0; release validator1 with `writer has no executable evidence`;
all 55 coverage contracts pass with 0 skips; changed-line formatting and staged
whitespace checks return 0. Inventory remains 920 owners: the 50 new checks run through
the existing mandatory script driver, with no shared manifest change.

The first fixture setup failure is preserved in `green-01`: it omitted the
native writer's required private `domains` directory. The fixture is corrected
without changing the writer. Passing 31-case `green-02` and 49-case `green-03`
are preserved as superseded source cuts. The final 50-case run uses the exact
source above; no superseded pass is substituted for it.

Seal: `flatfile-native-domain-seal-01-20261007/evidence.json`, SHA256
`0b2e66f9188fd88bf1d74c3bac253a32e5037fa6a30319c8a32491b747ea0494`, authenticates 4,309 artifacts/1,499,090,981
bytes across all eight new evidence directories. The prior money-history runtime
seal and canonical-mode supplement are referenced by exact SHA256, rather than
relabelled as new decoder tests. Primary refresh receipt SHA256:
`aa412a6a37ecb3827d065dbfba8108815f7c6a182bc74378db3eb6fbe98461bc`. Final delivery binds canonical result
payloads/modes/links, seven preserved earlier tips, clean worktree and remote SHA.

## Owned files, handoff and remaining gates

Owned code: `scripts/qualify_flatfile_native_domains.h`,
`tests/async/flatfile_native_domains_fixture.cpp`,
`tests/async/flatfile_native_domain_cases.py`, and the two existing native driver/
source-family helpers `test_flatfile_restore_economic_authority.py` and
`test_flatfile_restore_baseline_markers.py`. Only these five code files and this
report/follow-up change. No shared contract/schema/interface request is needed;
native producers, shared coordinator, registry/matrix and activation stay owned
by primary. Shared runners/manifests and native/migration trees remain exact.

Next independent work must join current wallet/bank locators and native clocks
to the correct retained economic epoch, detect missing/extra histories and stale
balances, and refuse pending authority/player-domain/currency journals without
recovery. The pure decoder alone grants no coherent live cut. Pile, escrow,
claim, treasury, custody/UID and complete origins remain required, alongside
bounded large-history/checkpoint behavior. Final fresh/upgrade SQL, both-engine
fault/replay, real player journeys and one combined release candidate remain
gates. SQL qualification is not repeated for this file-format slice.

There is no blocker to continuing independent Plan5 work. Missing executable
writer proof remains an observed release blocker. Primary's latest checkpoint
and private schema64 progress are separately attributed; private artifacts were
not independently inspected here. No result qualifies the combined candidate.
Accounting stays inactive, wallet-root item exclusions and the declined inactive
spell-path change remain exact. No audit adjustment or production data write,
activation, deployment or PR merge occurs.

This report, the remote follow-up and sealed receipts form the curator packet
for the user's nonblocking notebook maintained locally by primary. Notebook
application, cross-chat notification and primary acknowledgement are not claimed.
