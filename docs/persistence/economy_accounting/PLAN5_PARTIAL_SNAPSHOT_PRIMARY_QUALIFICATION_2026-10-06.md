# Partial pending-claim snapshot reader primary qualification

2026-10-06. Primary base `f324c877ec56f2e17b5e6cc2f62aae9004720db2`;
Plan5 dependency source `6b0246f1314b77e05ec43637b95c026561d3314b`.
Eight maintained Python inputs match the qualified frozen bytes. Production
code, canonical migrations, inactive behavior and unrelated WIP are unchanged.

## Failure and repair

The retained SQL allocation audit already knew partial spends, but saved
snapshots omitted their allocation rows and compared current claim cash with
the original source amounts. Original controls reproduce missing-export and
residual-balance failures. Snapshots now include bounded exact partial rows and
coverage, preserve original positive lot amounts, and reconcile remaining value.
Every committed claim-account debit needs exact whole/partial source allocation,
including other reasons and epochs. Missing, duplicate, orphan, mixed,
overdrawn, wrong-account and selected-root projection discrepancies refuse.
Historical missing allocation coverage remains an explicit finding.

Posting summaries now accumulate across all 64-pair source batches. A second
boundary defect was established in the actual old collector: its Cartesian
operation/key SQL predicate can return an account from an earlier batch and
double-count an otherwise valid two-account root. The collector counts only
the exact pairs requested by the current batch. Actual duplicates inside that
batch still increment the original counter and refuse; no global deduplication
or query/collection limit was introduced. A SELECT-only two-account SQL control
straddles pair 64, checks remaining cash and restores the complete original cut.
Its copied capsules are explicitly modeled metadata, not authenticated new
canonical roots or real producer evidence.

## Qualification

- Full selected pure origin classes: 42 methods PASS, zero selected skips.
- Full reconciliation and partial-allocation classes: 131 methods PASS, zero
  selected skips; published exact-integer UID provenance remains present.
- Original native/SQL restore entry point PASS on fresh MariaDB 10.11.14 and
  MySQL 8.0.46, canonical 0062. Each engine completes 109 cuts, 90 expected
  refusals and 58 full-entry cuts, preserving all original 105/89 cases. Native
  SQL/flat ASan/UBSan coin fixtures cover 32 cases and 3026 decoder decisions;
  claim allocation controls include 258 source and 257 consumption rows.
- Original NativeSQLOriginTests: both methods PASS on both engines, including
  seven native captures/refusals, 14 rollbacks, two command-preimage cuts and 16
  projected-position refusals per engine. Capture requires all 19 InnoDB sources
  in a SELECT-only consistent transaction; captured authority remains unchanged.
- All 3398 frozen source files/modes authenticate; the successful native wrappers
  verify input bytes unchanged after execution. Maintained imports match those
  pins. Full producer/capture completeness is not inferred from these checks.

The first capture invocation exposed a stale expected schema 61 history string;
only that expectation was updated to canonical 62. The next original tests
passed, then the observer failed copying a binary with `copytree`. That failure
is retained. The final unchanged-source run uses `copy2`, exports the fixture
binary and completes its original post-run source verification. Assertions,
provider graph, sanitizer flags, deadlines and refusal cases remain intact.

## Evidence pins

Local artifacts remain under `bin/` and `tmp/`, outside Git. Reproducible recipes
are `tests/async/test_restore_economic_coin_effects.py` with explicit disposable
Linux/canonical integration flags, and `tests/async/test_economic_sql_audit_origins.py
NativeSQLOriginTests` with its explicit disposable integration flag. No production
configuration, credentials, live game or production data were used.

| Evidence | SHA256 |
| --- | --- |
| pure results | `764ef31181fe3b645cec586cf41801852eaa0f0dbf6b7e21248e2467b2fbcaa9` |
| native restore source archive | `37983f62d21553867f4900214ed795189d71976a8037347d5d1eca2d57544a9e` |
| native restore terminal log | `4c411e99af8eaf90a9c941f4a9fd488ad5fb23e5d00b9f28e6ffb48b642decd3` |
| native capture source archive | `edcb2d0693b331e52dc9132c75821ae0a8fb36b701e0dd53315ee682a6c4677f` |
| native capture terminal log | `94e99a7e98698714b7b3bdfa416cad42a51d0c8ac17b5964c65eddfccabb8f8e` |
| retained native capture fixture | `79cee83d5785830b7f251b09f01072cc5eb7292ab03ef6ab33745b8fc8b9cd38` |

| Maintained input | SHA256 |
| --- | --- |
| `scripts/economic_sql_audit_snapshot.py` | `72d1279518bdb04913ed2a71cebd78edd20d05c5d4943cac0d9a1d0ad9263b8b` |
| `scripts/reconcile_economy_accounting.py` | `a862ca21352b7e08d5e82e8d68a286f82df317dc11089855d36e92ae7811a7e8` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `d5a9110f38ec2afbfb652ccccae4c4fd41248e00f498fb2beb6ad0779a8f1d9a` |
| `tests/async/run_native_sql_baseline_audit.py` | `d6aafbae46894a1ef7afcefd9b9feb1d06bd9e73df7760c4c2eafcf7e139fd9a` |
| `tests/async/run_restore_accounting_evidence_mysql.py` | `a12dc59c597c1ceb14cd40131a8e734560735bf02837f6d715f493bcf15ba7d3` |
| `tests/async/test_economic_sql_audit_origins.py` | `a45ecba432a94b69083143fa844b1813c198e95833e991bba78773b2a075d538` |
| `tests/async/test_reconcile_economy_accounting.py` | `c356f42a79454114fde06f6d4be7a7dd1e5bda7bda79df3e28a3f1d845d900c3` |
| `tests/async/test_restore_economic_coin_effects.py` | `63f1b1ebeee65df444aaa06425089b4d84fd5c93ecd60c2ca7a1ff2f128d2213` |

The native restore wrapper reports fixture hashes but did not export its ELF
files before teardown; its frozen source and original recipe are retained.
The final capture fixture is exported and hashed above.

## Remaining acceptance

This closes saved allocation-reader coverage and the source-batch collection
defect. Original producer policy/PID authentication, complete native capture,
real gameplay/ACK/cold recovery, populated upgrades, complete service restore,
retention workloads and combined release qualification remain open. The peer's
new managed-backup and retention-journey slices are not imported by this issue.
Writer coverage stays incomplete and release BLOCKED; no full Plan or R1–R8
completion is claimed.
