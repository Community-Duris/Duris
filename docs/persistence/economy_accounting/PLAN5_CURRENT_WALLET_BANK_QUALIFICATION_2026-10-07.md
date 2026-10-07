# Plan 5: independent current wallet and bank comparison — 2026-10-07

The new `--economic-wallet-bank-audit` command compares current native wallet and
bank values and their money revision clocks with independently authenticated
retained economic endpoints for the catalog's current epoch. It detects stale
values, stale clocks, missing/extra domains, missing histories, nonzero retired
balances and unknown legacy origins. Findings never cause a write or recovery.
Matching scoped values do not certify origins, cross-epoch continuity, other
money domains, all native holdings, full R7/R8 or release completion.

## Source, ownership and delivery

- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Local/remote branch: `codex/accounting-plan5`.
- Base and decoder prerequisite: `1e79b8ad72530cd6d65e335336cd11e205eb3251`. The sealed delivery
  receipt records the result SHA and verifies the matching remote after publication.
- Final tested canonical archive: `b9936c34b8f6842006c8c2ee8770fe5c92e8344816880205cdd1bf7487a0613c`; staged tree `bb03d72a8f0d0d37dc5e30aefbcd68b3a649212e`.
- Native tree: `4abb609524a1f1682ea4c190f82d75003c4d679b`; unchanged canonical0–62 migrations:
  `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`.
- Latest refreshed primary: `331f622c15ecbc5f7bade45e87c8c92f873733e6`, with native
  `2d0453e1e1d1755ae83fe42dbd261b8157eafd23` and migrations
  `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. This distinct source is not
  labelled as the tested combined candidate.

The result preserves all seven earlier ancestral tips documented in the remote
follow-up. Primary owns integration into `experimental-accounting`, shared
coordinator/contracts, producers, writer registry/matrix and activation.
No shared interface or schema change is requested. This delivery depends on the
published independent decoder at `1e79b8ad7` and money-history reader at `45666a547`.

Owned code files:

| File | Change |
| --- | --- |
| `scripts/qualify_flatfile_wallet_bank.h` | Pure bounded current mapping/history/native comparison |
| `scripts/qualify_flatfile_economic_authority.h` | Read-only mapping/current-epoch access and legacy-journal refusal gate |
| `scripts/qualify_flatfile_economic_money_history.h` | Optional independently checked endpoint observer |
| `scripts/qualify_flatfile_restore.cpp` | Listener-free operator command and bounded opaque findings |
| `tests/async/flatfile_native_domains_fixture.cpp` | Original native writer/probe and actual inactive ATM journey |
| `tests/async/flatfile_restore_authority_fixture.cpp` | Native-codec modeled baseline/effect, epoch, retirement and unknown-origin cuts |
| `tests/async/flatfile_wallet_bank_cases.py` | Independent sanitizer/operator comparisons and retained private input cuts |
| `tests/async/test_flatfile_restore_economic_authority.py` | Registration through the existing mandatory driver |
| `tests/async/test_flatfile_restore_baseline_markers.py` | Complete audit-family source pin |

Only these nine code paths and this report/owned remote follow-up change. Native
producers, migrations, shared manifests/runners and activation files stay exact.

## Established gap and completed fix

The original native writer seeds wallet100/bank50 in a fresh private inactive
root. The original economic codecs record modeled baseline wallet100/clock0 and
bank50/clock1. A valid native frame then changes only wallet100 to900, preserving
the money clock and recomputing its SHA256. The public native loader reads900;
the retained-only money-history audit exits0 with continuity true and endpoint100.
Both readers preserve the complete retained inventory. This demonstrates the
missing current comparison; the old reader's continuity-only scope stays intact.

The new command reports `native_balance_mismatch` and exact native/expected
denomination vectors and revisions. It uses immutable economic account identity,
the current locator, the catalog's current epoch, wallet-specific money clocks
and native bank clocks. It never chooses a tail by lexical operation/epoch ID or
the wallet envelope's maximum epic/frag/base-stat clock. Retired lifetimes cannot
substitute for the recreated active mapping. Earlier epochs remain separate.

The complete retained authority/record closure is authenticated before comparison.
The reader checks canonical native filenames, private regular single-link files,
full native frames and body identities, all four denominations, signed economic
range including weighted total, missing/extra domains and wallet/bank context.
Native unsigned amounts/revisions remain exact in emitted JSON, including
UINT64_MAX observations. Matching but unanchored current histories retain an
`unknown_legacy_origin` finding. Invalid history does not grant endpoint validity.

The existing shared read lock is opened without creation. Authority, player-domain
and currency journals are refused before and after scanning. An exclusive native
writer, absent lock for a nonempty current scan, malformed/checksum/nonprivate/
hardlink/symlink/FIFO inputs and exhausted cooperative budgets produce exit1,
fixed stderr `native_restore_qualification_failed`, and no partial stdout. No
native load, recovery, initialization or mutation API is called by this reader.
Its standalone `g++ -MM` closure contains owned independent headers and no `src/`
provider. The native loader remains only a test oracle.

Scans retain the existing128MiB byte,16,384 file,8,192 directory-entry and30-second
budgets,32768 account/edge bounds and100 finding rows. Native names are collected
under the charged entry budget and sorted; overflow preserves exact finding count
with `findings_truncated`. Directory/control identity and catalog epoch are
rechecked before reporting. Full origin/cross-epoch/other-domain/native-holdings,
R7 and release flags remain false even on healthy scoped observations.

## Operator use and result contract

```sh
python3 scripts/build_restore_qualifier.py
bin/tools/qualify_flatfile_restore --economic-wallet-bank-audit /ABS/PRIVATE/ROOT
```

The root must be a private absolute flatfile authority root whose writers honor
the existing authority lock. The command starts no listener, creates no lock and
never repairs a finding. An empty inactive root returns an unqualified empty
observation. Existing inactive domains without mappings report unmapped native
domains. Exit0 means no findings in this scoped observation; inspect
`initialized`, `wallet_bank_holdings_compared` and
`current_wallet_bank_values_verified` separately. A finding returns exit1 with
JSON; a refusal returns exit1 without JSON. Findings expose opaque economic IDs,
kind/PID and exact mismatch values, without account aliases. JSON consumers must
preserve full integer precision. Economic witnesses in these component fixtures
are modeled through original codecs; their fixed source digests do not prove a
production capture or origin.

## Exact checks and evidence

Evidence root: `D:/CodexEvidence/accounting-plan5/bin/`. The final archive has
6,401 regular members, four canonical links,1,304 native-source
files and20 pinned audit-family inputs. Terminal payload/mode/link guards pass.
Private per-case input inventories bind retained regular payloads by SHA256;
the evidence retains deduplicated payload objects. Current-value reader cases
check bytes, modes, inode/link counts, sizes and mtimes, plus root metadata and
symlink targets. The original gap also checks its complete retained inventory.
Access times are excluded. No selected test is skipped.

The immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
uses isolated flatfile filesystems, network disabled,2 CPUs,4GiB memory and private
tmpfs workspaces. Fixtures keep original native mutation providers. Independent
readers run ASan/UBSan; native fixture/operator closures use their original flags.

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
python3 -u -B tests/async/flatfile_wallet_bank_cases.py \
  --native-source /workspace --artifacts /workspace/bin/tests/wallet-bank
python3 -u -B -m unittest -v test_backup_review_remediations
DURIS_REGRESSION_BUILD_CACHE=off \
python3 -u -B tests/async/test_flatfile_restore_economic_authority.py
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/wallet-bank-managed \
python3 -u -B -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile \
  BIN_ROOT=/workspace/bin/tests/money-history-build/bin \
  OBJDIR=/workspace/bin/tests/money-history-build/objects \
  DMS_BINARY=/workspace/bin/tests/money-history-build/server
```

| Check | Result | Seconds | Evidence directory |
| --- | --- | ---: | --- |
| Original gap | Native900/history100; old continuity0; inventory unchanged |186.896896 |`flatfile-wallet-bank-red-01-20261007` |
| Final focused command/independent reader |38 observations;0 skips; original50 decoder cases retained |207.554998 |`flatfile-wallet-bank-green-03-20261007` |
| Backup review regressions |18 methods;0 skips; exit0 |0.769569 |`flatfile-wallet-bank-full-01-20261007` |
| Complete registered authority driver |20 valid/367 refusals;28 money-history,50 decoder and38 current-value observations; exit0 |472.699356 |same |
| Complete managed lifecycle |1 method,2 actual boots,2 retained generations;0 skips; exit0 |218.635177 |same |
| Maintained flatfile Make |740 actual compilations,0 reused without compilation,0 warnings/errors; exit0 |256.996305 |`flatfile-wallet-bank-make-01-20261007` |

The full driver also preserves54 semantic,1,058 metadata,574 command-envelope,
12 boundary,9 budget,28 root-page,29 authority-page,30 baseline-control,
77 baseline-history and49 namespace controls. Current-value observations cover
native stale values/clocks, nonmoney clocks, denomination/range boundaries,
missing/extra domains, all three pending journals, writer exclusion, physical
refusals, four exhausted budgets,121 findings truncated to100, the actual native
ATM transfer, successor epoch25 after50, retired nonzero balances, unknown
legacy origin and inactive/empty roots.

The managed method checks restore/replay/drain, source dedupe, UID advancement,
inactive old receipts, retention/pruning, four pre-boot fault refusals, pending
source audit refusal and unchanged source/generation inventories. Its server is
an authenticated earlier build of the identical native tree, with SHA256
`f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`; no fresh-server claim is made for that method. Full R8
and activation remain false.

Make ran archive `2cf6280ba588d6ca27a645256a398ec57a7282ed18eb84332ae898dc8350c50e` before the final fixture-only
probe correction. `final-source-dependency-binding.json` independently binds all
1,304 native bodies/modes and1,290 Make dependency bodies/modes to the final
archive. Only `flatfile_native_domains_fixture.cpp`, absent from that dependency
closure, differs. Make actually recompiled740 objects; their digests and server
match the earlier sealed build. This method does not relabel digest equality as
object reuse or relabel the older archive as the final archive.

Final gates (`flatfile-wallet-bank-gates-03-20261007`): staged whitespace0;
normal validator0; release validator1 with `writer has no executable evidence`;
55 writer-coverage contracts pass with0 skips; all six touched C++ files pass the
repository formatting check. Inventory remains920 owners; new cases run through
the existing mandatory driver. No shared manifest change is needed.

The first two failed frozen runs remain preserved: green01 put a future catalog
epoch after the compared baseline and correctly reported missing current history;
green02's extra-PID seed succeeded but its trailing native probe asked for11 after
seeding12. Both fixture corrections preserve native writer/reader behavior. The
final green03 and complete driver execute the final source; earlier gates and
failures are not substituted for them.

The first seal attempt refused a still-running full-run container after tests
passed but before artifact copy terminated. Its observation and executed helper
are preserved separately. Final sealing waits for the original transport's
terminal exit0; no tests or containers are restarted for that observation.

Seal: `flatfile-wallet-bank-seal-01-20261007/evidence.json`, SHA256 `625b28abeab28d7d95ec0572f67fff1f08a0b4d7b4e21892cf2ef0efdeca6fdd`,
authenticates 6,918 artifacts/1,691,371,889 bytes.
It references the prerequisite native-decoder seal, records terminal container
states, and includes the exact latest primary refresh with SHA256
`2e83b86c8f4568c47637719fdc50ae24bc4170dfe094662f7fd3e1a151649d55`. The final delivery receipt binds
all nonpublication payloads/modes/links, seven earlier tips, clean worktree and
remote result SHA.

## Remaining gates and curator handoff

Full account origins, cross-epoch continuity, pile/escrow/claim/treasury money,
native UID/custody/tombstone/reference histories, growing-history/checkpoint
behavior, final fresh/upgrade SQL on both engines, complete faults/replay,
real player journeys and one primary-published combined release candidate remain
required. SQL methods are not rerun for this flatfile-only reader change. The
release validator's missing executable writer evidence remains an observed
release blocker. Independent Plan5 work can continue without a new interface
request or notebook blocker.

Accounting stays inactive; wallet-root item exclusions and the declined inactive
spell-path change remain exact. No production data write, auto-correction,
activation, deployment, PR merge or independent experimental-accounting push
occurs. Primary's protected artifacts were not independently inspected here.
This report, owned remote follow-up and sealed receipts are the curator packet
for primary's locally maintained nonblocking shared notebook. Notebook application,
cross-chat notification and primary acknowledgement are not claimed.
