# Activation verifier request context — 2026-10-06

The independent activation callback previously received a connection, route
evidence and a source snapshot, but no explicit installation request. This
missing interface is fixed by passing the original request synchronously.
No production verifier is registered and no accounting authority is activated.

## Exact shared contract

```cpp
using economic_sql_activation_verifier = unsigned int (*)(
    MYSQL *, const economic_sql_lifecycle_request &,
    const economic_sql_activation_evidence &,
    const economic_sql_source_snapshot &) noexcept;
```

The owner invokes `verify(connection, request, evidence, snapshot)`. The
borrowed request identifies the intended operation, lineage, epoch, actor and
stable accepted timestamp. It is caller context, not independently authenticated
authority at callback entry. The verifier must authenticate its binding against
retained SQL evidence and must not retain the reference. Existing installation
and request-hash checks still run before a decision is recorded.

Direct in-tree type evolution is appropriate: there is no production registration
or stable external plugin ABI. The sole disposable native harness callback
accepts an unnamed request parameter; its existing body, synthetic-route scope
and cases remain unchanged. The original three-case source contract updates
only its literal invocation assertion. Serialized requests/digests, witness,
receipts, journal, schema and migration histories do not change.

Missing verifier/incomplete evidence refusal, same-session maintenance/writer
guards, capture, savepoint ordering, durable decision/pointer atomicity, exact
committed retry and pause/reactivation retain their original code. An exact
active retry still authenticates the retained decision without calling the
verifier or recapturing changed gameplay holdings. No global callback context,
new CLI, startup activation or online handover is introduced.

## Actual checks

Changed production owner objects pass the original Makefile target with
`PERSISTENCE_BACKEND=mariadb` and `flatfile`, `BUILD_PROFILE=production`, and
the full existing warning/hardening/feature flags. The original lifecycle
harness object passes its original C++20, `-Werror`, ASan/UBSan, non-PIE and
compiler-provided `mysql_config --cflags` recipe. These are object compilations,
not linked runtime, database or sanitizer-execution results.

The final formatted source and all three object hashes are retained under
`bin/tests/activation-request-binding-final-20261006/results.json`.
Earlier object checks and their generic include-path observation remain
separate; the final harness check uses the actual original compiler flags.
No warning, provider, case, timeout or stack policy changes. Changed-line
clang18 formatting and independent read-only source review pass.

All three original activation source cases pass. The regenerated matrix, normal
14-golden accounting validation, writer-site checks, both route-evidence cases
and original 55 coverage cases pass on the maintained candidate. Results/logs
remain in `bin/tests/activation-request-binding-writer-contracts-20261006/`.
All 149 existing source pins are preserved; four actual interface/companion
files add bindings, for 153 total. The 920-row writer census and all 2,876
occurrences / 2,818 unique sites are unchanged. Complete inventory still
validates 920 tests, which does not claim execution of that inventory.

Original major-plan gameplay, both-engine lifecycle, fault and recovery
qualification remain at the agreed coherent batch. Earlier full production
builds retain their earlier source scope; they are not reported as builds of
this subsequent interface edit.

## Narrow Plan 5 handoff

Primary owns the callback type, lifecycle capture/orchestration, coordinator,
writer registry/matrix and production activation owner. Plan 5 owns the
independent verifier and reconciliation evidence. Its implementation must use
the borrowed reconnect-disabled `MYSQL*` and the existing caller transaction,
without starting/committing another transaction, reconnecting or releasing locks.
It must bind the actual request, installation/baseline identity, current capture,
reviewed route manifest, independent audit and tested candidate. Complete counts
and digests must come from checked evidence; partial capture, missing routes or
reconciliation exceptions return nonzero errors. The current separate Python
exporter's `sql_partial`/`complete=False` result supplies no positive activation
authority. This handoff adds no new acceptance gate.

Full R6 still requires the original source-complete stopped-maintenance
procedure. Source inspection establishes that production has no caller of
`install`/`activate_verified`; `make_batch` currently adds holdings but no item
forests; the current native holdings selection omits normalized escrow and
pending claims. The source snapshot expressly disclaims complete physical and
live-world coverage. Connecting a thin caller or accepting current-owner rows
alone cannot complete that opening witness.

Preserve the stopped maintenance boundary: normal boot already reconstructs
runtime authority through `sql_economic_runtime_start` and `recover_runtime`.
Maintenance refuses existing runtime authority, while cutover requires the
initialized/drained coordinator. Complete capture and authentic verification
must precede activation; production remains inactive throughout development.
The corresponding authenticated flat boundary also remains required.

Plan 1's documented original independent deliverable and incomplete-manifest
refusal remain distinct from this full-release R6 integration. This interface
fix is not another independent Plan 1 gate or full R6 completion. R1–R8,
coverage and release remain unfinished; unrelated work and the declined
inactive spell-path boundary are preserved.
