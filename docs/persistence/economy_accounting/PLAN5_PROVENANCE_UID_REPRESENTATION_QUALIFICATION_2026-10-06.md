# Plan 5 provenance UID representation qualification — 2026-10-06

The provenance query accepted a captured float UID `81.0` for `--uid 81`, and
accepted boolean `true` or float `1.0` for `--uid 1`. Python numeric equality
matched those records and the view emitted the requested integer UID, changing
the captured identity representation. The independent audit retained its
discrepancy, but the displayed provenance row was misleading. The original
reader fails nine regression subtests in the frozen reproducer.

The separate fix requires an exact integer captured UID before equality. Valid
integer provenance, row limits, deduplication, conflicting positions, history
coverage, whole-audit exception counts and CLI status remain intact. Invalid
representations are omitted without repair or an all-clear. The focused test,
137 selected pure reader methods, and original SELECT-only exporter recipe on
both supported SQL engines pass with zero skips. This qualifies the query fix,
not full Plan 5 or a release candidate.

## Branch, exact source and ownership

- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Branch and sole publication destination: `codex/accounting-plan5`.
- Refreshed primary: `f528a46b43e07444b97121a6e63cb9be414fb5d0`.
- Plan 5 base: `1ec48f46590ab80aa4faeabf73d5506a7b8a4cdc`.
- Separately committed fix: `68c6938d9f0ddd2e308f31cea62e9df97746194d`.
- Native tree: `03a97173396f720857b1ad58a2ab69b7859a2387`.
- Migration tree: `2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`, canonical 61.
- The post-publication delivery receipt binds this report's commit and remote
  tip to the qualified fix; documentation adds no new execution claim.

The fix owns only `scripts/reconcile_economy_accounting.py`, its existing
`tests/async/test_reconcile_economy_accounting.py` suite, and the provenance
paragraph in `AUDIT_OPERATIONS.md`. This report and the appended remote
follow-up are the separate evidence documentation. No shared coordinator,
contract, producer, migration, writer registry/matrix or activation file is
edited. These owned files have no shared source pins to update. There is no
shared interface or schema request from this fix.

The frozen failing source is the base with only the new regression overlaid;
its reader is byte-identical to the original base reader. The passing source
overlays exactly the three owned fix files. Its 3,122 tracked native, migration,
script and test inputs match the committed result. The exact new regression
bytes are the same in both candidates. All seven previously preserved Plan 5
branch tips remain ancestors; follow-ups stay on the expected remote branch.

## Reproduced behavior and commands

The new method exercises eight captured representations: integer 81, float
81.0, boolean true, float 1.0, integer 1, boolean false, string `"81"`, and null.
Every case checks limits 0, 1 and 100, full counts and truncation, exact integer
output, unchanged input objects and saved JSON bytes, CLI JSON, and the original
whole-audit exit status. A valid integer 1 remains visible even when another
field in that record disagrees; the query does not hide all discrepant evidence.

Original focused command, with `PYTHONPATH=tests/async`:

```text
python3 -u -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests.test_provenance_uid_lookup_preserves_record_representation
```

| Frozen candidate | Original command result | Elapsed seconds |
| --- | --- | --- |
| Base reader plus regression | FAIL, exit 1, nine subtest failures | 1.468343 |
| Exact committed successor | PASS, exit 0, zero skips | 1.969792 |

The failed observer exits zero only because it verifies the expected original
failure. Its terminal log and exit 1 are retained as failure evidence. It is
not counted as a passing test. A direct Windows Python invocation of the same
method also passes (one method, 2.727 seconds in unittest output); the sealed
Linux results below are the retained reproducible qualification batch.

Original broader reader command, with the same `PYTHONPATH`:

```text
python3 -u -B -m unittest -v test_reconcile_economy_accounting.ReconciliationTests test_economic_sql_uid_scope.UidScopeTests
```

Result: 137 methods, exit 0, zero skips, 43.565646 seconds. Existing scope,
original-plan, numeric bounds, UID history, equipment, quarantine, supply,
source, erasure and global operator-refusal checks remain in this selected
batch. Other native and budget test classes are outside this command.

Original disposable database command, run unchanged once per engine:

```text
/usr/bin/python3 -u -B /workspace/tests/async/run_economic_sql_audit_snapshot_mysql.py
```

| Backend | Server version | Result | Elapsed seconds |
| --- | --- | --- | --- |
| MariaDB | `10.11.14-MariaDB-0ubuntu0.24.04.1` | PASS, exit 0, zero skips | 10.185530 |
| MySQL | `8.0.46-0ubuntu0.22.04.4` | PASS, exit 0, zero skips | 11.349435 |

The original recipes verify a consistent partial cut, SELECT-only permission,
prior-epoch creation and unattributed UID retirement, bounded provenance and
operator CLI output, exact original-plan/numeric/source checks, interrupted and
late-commit cuts, and unchanged authority rows. Their global refusal on
incomplete modeled SQL evidence is preserved. The recipes use their own
minimal disposable fixture DDL; this batch does not qualify fresh/upgrade of
the complete canonical 61 database or any player journey.

## Retained source and evidence

Protected root: `D:\CodexEvidence\accounting-plan5\bin`.

- `provenance-uid-red-01-20261006`: original failing log, result, source archive,
  per-file transport hashes, exact observer, execution helper and Docker command.
  Archive SHA256:
  `243e1f075721ab1afa010e7e77416bfda4222fe06e831d10bd0c73610c4ff5f3`.
- `provenance-uid-green-01-20261006`: focused and pure logs/results, both original
  SQL exporter logs and private daemon logs, source archive and transport,
  original commands, exact observers and source-unchanged receipt. Archive SHA256:
  `8c8f3dee23389f5773747b89bb0868df51505cacaaf5901593c7ffc34174e3d0`.
- `provenance-uid-final-seal-01-20261006/evidence.json`: all 3,122 source input
  hashes, 25 retained artifact hashes, original results, both engine versions,
  container terminal states, and prior native-build seal binding. SHA256:
  `6917707ebd932a4ecc8014d55cd5492faef39bdc100f7b9c4049d8f2f623aca9`.
- Ignored observer sources: `tmp/plan5/qualify-provenance-uid-representation.py`,
  `tmp/plan5/qualify-provenance-uid-sql.py`, and
  `tmp/plan5/seal-provenance-uid-representation.py`. Executed copies are protected
  with the evidence. The diagnostic probe is retained separately under
  `tmp/plan5/provenance-uid-representation-probe.json`.
- Post-publication receipt: `tmp/plan5/provenance-uid-representation-delivery.json`.
  It records fix/report/base/remote commits, report and seal hashes, clean
  worktree, preserved old tips and exact unchanged qualified inputs.

Both candidates execute the authenticated Git archive in an empty RAM workspace,
using tools image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
with network disabled, two CPUs, 4 GiB memory, and separate 2 GiB workspace/tmp
limits. No live checkout, project `.env`, player data or production service is
mounted. Private daemons use only container loopback and fresh temporary data
directories. An ephemeral setup account seeds each original random fixture;
the audit runs with its original SELECT-only user. Both schemas are removed
and both daemons stop. Temporary database/fixture outputs are not retained as
production evidence; the original recipes, terminal logs and sealed source are.
Neither container is OOM-killed. No native rebuild is claimed for this Python
change: native and migration bytes match the earlier current-source production
build qualification exactly.

## Remaining gates and curator handoff

There are no skips or unresolved blockers for this query defect. R6 still needs
complete native opening and physical item forests, auction/claim selection and
mapping recovery, retained partial claim consumption, and the authentic
independent activation verifier. The precise shared implementation boundary
remains in
[the primary auction/claim handoff](SQL_OPENING_AUCTION_CLAIM_IMPLEMENTATION_HANDOFF_2026-10-06.md).
Full real writer/player/publication/ACK/lost-reply/cold journeys, flatfile parity,
typed active erasure and current full lifecycle/retention qualification, and
mixed release-host size/latency/storage/checkpoint/reconciliation budgets remain
required. Inventory, synthetic fixtures and isolated passes do not close them.

The primary locally maintains the shared notebook. This owned report, appended
remote follow-up and sealed delivery receipt form its curator packet; notebook
ownership is nonblocking. Accounting remains inactive. Wallet-root exclusions
and the declined inactive spell-path change are preserved. No production data,
audit findings or balances are changed; no accounting activation, deployment,
PR merge or independent `experimental-accounting` push occurs.
