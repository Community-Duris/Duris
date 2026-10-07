# Plan 5 audit input provenance qualification

Three owned flatfile qualification drivers recorded a hand-maintained subset
of their audit inputs. All omitted the newly consumed namespace reader; the
lifecycle driver also omitted Python audit modules it executed. This slice fixes
that source-provenance gap. The affected native suites pass. The additional
managed journey exposed a separate stale test ordering, preserved below and
scheduled for its own fix/commit; it is not reported as a passing restore.

## Source and ownership

Branch/local and remote publication remain `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base is `99120f1375ee3e7a077ac9642ab48a5099427db3`. Result SHA, canonical source comparison, remote tip
and all seven preserved earlier tips are bound by the separate
`flatfile-audit-inputs-delivery-01-20261007/delivery.json` receipt.

Tested archive SHA256 is `8ff0d4e3e148f7fbb12f4cd7a44a703dbdd731b2aaeb681055ff5829a6e42df4`: the base Git archive
with exactly four owned test payloads overlaid. Every other payload, original
mode and all four links remain identical. Native tree is
`4abb609524a1f1682ea4c190f82d75003c4d679b`; migration tree is `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`,
canonical source0062. The twice-refreshed primary is `36e8f6ad78ef027851c1da809acca7570dcd95d6`, whose native
tree differs. No primary or combined-candidate qualification is inferred.

Owned code changes are `tests/async/test_flatfile_restore_baseline_markers.py`,
`tests/async/test_flatfile_restore_lifecycle_receipts.py`,
`tests/async/test_persistence_backup_integration.py` and
`tests/async/test_backup_review_remediations.py`. This report and the owned
remote follow-up are publication files. No shared coordinator, schema, producer,
contract, runner, registry/matrix or activation file changes. No shared interface
request is needed.

## Established defect and complete scoped fix

The red observer extracts the actual recorded-input comprehensions and terminal
guards from each original source, copies their inputs to disposable directories,
changes the namespace header and executes each original terminal guard. All three
guards accept changed source. The actual compiler dependency command
`g++ -std=c++20 -D__NO_MYSQL__ -Isrc -Isrc/no_mysql -MM
scripts/qualify_flatfile_restore.cpp` independently confirms consumption of that
omitted header. This is a provenance defect, not evidence of corrupted backup
data. Earlier namespace delivery separately sealed a complete source archive;
its documented component evidence is not invalidated by this narrower guard gap.

The common `audit_source_inputs()` helper records all matching
`scripts/qualify_flatfile_*.h`, `scripts/qualify_flatfile_*.cpp` and
`scripts/flatfile_*audit.py` files, plus the builder, namespace case helper,
marker helper and native/server build helpers. Current membership is14 files.
Each affected driver captures this map before compiling/executing and compares
the complete map at its terminal guard before publishing evidence. Added,
removed and changed raw bytes refuse. Existing native src fingerprints, fixture
pins and runtime pins stay present. This is an endpoint guard, not continuous
source attestation.

The private test certificate adds `audit_source_inputs`, a repository-relative
name to raw-byte SHA256 map, consumed by these three owned drivers and the sealed
observer. Existing CLI summaries omit the large map and retain their original
fields. This is not a producer/storage schema change. The new focused regression
checks actual current membership and drift. The green observer executes the exact
new capture/terminal guard AST from all three consumers:36 changed/deleted/added
cases refuse, and every restored source control accepts. All18 original-plus-new
backup review methods pass, zero skips.

## Commands, native outcomes and discovered managed gate

Frozen execution uses immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network disabled, two CPUs,4GiB memory, fresh3GiB workspace/2GiB temporary tmpfs.
No runtime environment, production credentials, existing DB or game is supplied.
Native cache is off; original C++20 strict flags and ASan/UBSan fixture recipes
compile fresh. Existing native assertions and deadlines remain exact. The managed
container uses SYS_ADMIN/unconfined seccomp for its original disposable tmpfs and
isolated namespace prerequisites.

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
python3 -u -B -m unittest -v test_backup_review_remediations
python3 -u -B tests/async/test_flatfile_restore_lifecycle_receipts.py \
  --artifacts /workspace/bin/tests/audit-input-receipts
python3 -u -B tests/async/test_flatfile_restore_baseline_markers.py \
  --native-source /workspace --artifacts /workspace/bin/tests/audit-input-markers
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/audit-input-managed \
python3 -u -B -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

Native environment retains `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

| Original check | Actual result | Seconds |
| --- | --- | ---: |
| Backup review suite and exact source guards |18 methods pass;36 guard drift refusals; restored controls accept |1.000379 |
| Lifecycle native command |Exit0;131 cases,9 accepted/122 refused;46 lifecycle pages,35 controls,69 history,48 namespace cases; zero skips |283.032958 |
| Baseline marker native command |Exit0;69 cases,56 structural refusals/7 readable-unqualified/6 modeled qualified cuts; zero skips |145.319202 |
| Managed lifecycle method |Exit1;1 method,1 error,zero skips; initial source audit refuses before capture/boot |194.783949 |

All three resulting certificates record the exact14-family hashes. The failed
managed method still executes its teardown source guards and produces an
explicit `completed:false`, zero-outcome certificate; those terminal source
comparisons pass. No managed acceptance or service boot is claimed.

The separate managed failure is established at its first `audit(live)`. The test
has already deliberately seeded `.critical-authority-transaction`, but now calls
the online `--economic-evidence-audit` interface. Its independent
`authority_read_lock::no_pending()` correctly refuses this unstable cut without
recovery. Recovery is separately authorized by the manager on a fresh copied
`ISOLATED_RESTORE` candidate. The stale journey must check the healthy source
before the pending seed, assert read-only refusal while pending, then preserve
the complete candidate recovery/boot/retention assertions. This ordering repair
is a separate owned follow-up; the reader's safety boundary must remain intact.

The selected server SHA256 is
`f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4`, from the earlier qualified
flatfile740-unit build and archive
`2eba999163a7a773ebf3a02fde95f4b2639ec92d96e212bdd826885370d5a36c`. Its native payloads/modes,
binary and original build-log hashes were reverified. The failed journey does
not reach server execution. No fresh production build is claimed for this
Python-only change. The previous namespace seal retains its exact build scope.

## Gates and protected evidence

`git diff --check`, normal accounting validation and all55 writer contracts pass.
Regression discovery remains920 entries. Release validation correctly exits1:
`writer has no executable evidence`. No C/C++ formatting/build is required by
this test-only change. No SQL database method is repeated; corrected guards are
flatfile-specific, and no SQL reader, schema or producer changes. Earlier SQL
results retain their original attribution and are not relabeled as this run.

Under `D:/CodexEvidence/accounting-plan5/bin/`, retained directories are
`flatfile-audit-inputs-red-01-20261007`, `flatfile-audit-inputs-red-02-20261007`,
`flatfile-audit-inputs-green-01-20261007`, `flatfile-audit-inputs-full-01-20261007`
and `flatfile-audit-inputs-gates-01-20261007`. The initial red harness selected the
wrong class and stopped before probing; its StopIteration/exit1 remains intact.
The corrected red proves all three original guard omissions. Full native and
managed logs, including the genuine managed error, remain unaltered.

Seal `flatfile-audit-inputs-seal-01-20261007/evidence.json`, SHA256 `0b058da8044679d9bf7fc9ab3c344411a558164cce481bb8033396fea8c2b613`,
binds 13,212 artifacts/1,336,773,155 bytes, complete
commands/logs, archives, compiler dependencies, native binaries, failed managed
certificate and terminal container states. The seal is written after the full
process and artifact-copy wrapper exit. Separate preflight/delivery receipts
compare every nonpublication payload/mode/link with the canonical committed
result and verify remote SHA plus preserved ancestry.

The source-guard defect is resolved. The newly established stale managed test
ordering remains a required owned follow-up. Full current native holdings/UID
reconciliation, whole orphan/forward closure, native producer/recovery/ACK,
genuine gameplay on both backends, release-host budgets, complete trusted
backup/replica/retention/erasure proof, R7/R8 and the primary's tested combined
candidate remain open. Modeled component passes are not release completion.

Accounting remains inactive; wallet-root exclusions and the declined inactive
spell change remain exact. No production mutation, audit correction, deployment,
activation, PR merge or independent experimental-accounting push occurs. This
report, remote follow-up and receipts form the curator packet for the primary's
locally maintained notebook. Its upkeep remains nonblocking; notebook application,
direct notification and primary acknowledgement are not claimed.
