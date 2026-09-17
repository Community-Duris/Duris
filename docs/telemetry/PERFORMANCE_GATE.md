# Telemetry performance and degradation gate (#272)

This gate is a local, offline evidence package for the telemetry transport and
the pure rollup/report stages. It proves bounded behavior under matched
synthetic workloads and injected repository faults. It is not a production
load qualification, a SQL benchmark, or a claim that telemetry has been
enabled on a live server.

The gate deliberately does not:

- connect to MySQL/MariaDB or read credentials;
- start the game server or generate player/gameplay traffic;
- apply a migration, publish a rollup generation, or change runtime
  activation/balance policy;
- run CI checks (the project task explicitly excludes CI for this package).

## Reproducible workload

The frozen profiles are 50, 200, and 1,000 synthetic telemetry interval facts.
Each profile uses the same value-only record shape, producer identity, monotonic
ordering, and dimensions across all stages. The default runner takes five
repetitions; focused tests use three repetitions to shorten the local test
cycle. The workload is generated in memory and contains no player, account,
credential, or production row.

The queue benchmark uses a 128-record worker batch and flushes between batches,
so the normal profile never relies on an unbounded queue. The capacity fault
case separately uses a 16-record queue with a four-record control reserve to
force both detail and control admission rejection.

## Measured stages

`tests/async/telemetry_capacity_272_harness.cc` runs the C++ stages against an
injected fixed repository and clock:

| Stage | What is measured | What is intentionally absent |
| --- | --- | --- |
| `off` | matched record construction baseline | telemetry validation and queue admission |
| `capture_only` | record validation and value-only capture work | repository and queue writes |
| `capture_write` | actual bounded enqueue plus synchronous in-memory worker batches | SQL/network/disk latency |

`scripts/telemetry/benchmark_272.py` measures the existing pure Python
`build_page_contributions` rollup path and the existing administrator report
service over synthetic in-memory rows. These stages exercise the real bounded
semantics and response shaping without opening a database.

Every C++ stage reports p50, p95, p99, and p99.9 sample latency, wall time,
CPU time, admitted/dropped records, repository calls, and checksum. The
capture/write stage also reports worker-batch latency, queue high-water, and
the fixed queue storage footprint plus `producer_allocation_bytes: 0`. The
Python stages report the same latency percentiles, CPU time,
`allocation_bytes`/`memory_bytes` from peak `tracemalloc`, and an explicit
`database_calls: 0` marker.

The source contract checked by the runner verifies that the producer/transport
sources contain no heap allocation, SQL connector, file-open, or blocking
thread primitive. The queue is checked for its atomic SPSC head/tail storage.
The reported queue footprint is the fixed compile-time queue storage, not an
estimate of production RSS.

## Local guard budgets

These are reproducibility guards for this disposable harness, not public
service-level objectives. A slower host should produce a visible gate failure
that is reviewed with the report rather than silently being treated as a
production result.

| Metric | Guard |
| --- | ---: |
| normal `capture_write` p99 | ≤ 1 ms |
| normal `capture_write` p99.9 | ≤ 5 ms |
| normal in-memory worker p99.9 | ≤ 50 ms |
| pure rollup p99.9 | ≤ 5 s |
| pure report p99.9 | ≤ 5 s |
| slow-write producer admission maximum | ≤ 100 ms |
| stop request while a write callback is blocked | ≤ 100 ms |
| normal capture/write drops | 0 |
| normal queue high-water | ≤ 128 records |

The gate is a pass only when the structural checks and these local guards pass
for all three workload sizes. A passing result means “bounded under the
documented synthetic fixture on this host”; it does not authorize production
activation or claim a live-database latency budget.

## Fault and teardown matrix

`tests/async/telemetry_fault_272_harness.cc` uses the same transport seam as
the existing telemetry tests and emits one sanitized JSON object per case.

| Case | Injected condition | Evidence required |
| --- | --- | --- |
| `queue_saturation` | 16-slot queue, four-slot control reserve | explicit detail/control rejection and queue peak exactly 16 |
| `repository_recovery` | first three repository initialization attempts unavailable | retained records catch up after reconnect; no pending tail |
| `ambiguous_commit` | first apply returns lost-COMMIT ambiguity | immutable retry is accepted as three duplicate-identical records |
| `slow_write` | worker callback held while the producer continues | producer admissions and stop request remain bounded; drain completes |
| `copyover_shutdown` | quiesce, reject admission, resume, drain, stop | quiesce rejects exactly the paused record and teardown leaves no tail |
| `churn` | 20 init/drain/shutdown cycles | no retained queue state or cross-cycle identity leakage |

The slow-write case intentionally models the repository callback as a local
blocking fixture. The transport contract says a connector that violates its
worker cancellation contract cannot be preempted by a caller deadline; the
test therefore measures the nonblocking producer/stop requests and releases
the callback before the off-game-thread drain/reap step.

## Commands

From the repository root:

```text
python scripts/telemetry/benchmark_272.py \
  --workloads 50 200 1000 \
  --repetitions 5 \
  --output bin/tests/telemetry_capacity_272.report.json

python tests/async/test_telemetry_capacity_272.py
python tests/async/test_telemetry_fault_272.py
```

The runner uses `CXX` when supplied, then native `g++`, and finally a local
`wsl.exe`/`g++` fallback on Windows. Compiled binaries and JSON reports stay
under `bin/tests/` and are disposable evidence; they are not source or schema
artifacts.

## Review checklist

A reviewer should confirm all of the following from the generated report and
the harness source:

1. The three workload sizes are present and use the same stage definitions.
2. p50/p95/p99/p99.9, CPU, allocation, queue, and repository/DB fields are
   present; `database_calls` remains zero for the pure Python stages.
3. Normal capture/write has no drops and stays within the 128-record batch
   high-water guard.
4. Every fault case has the expected bounded outcome, including explicit loss
   on saturation and duplicate reconciliation after ambiguity.
5. Stop/quiesce/teardown evidence is captured after callbacks are released and
   no test performs a production migration, load, activation, or CI action.

The closeout should attach the sanitized JSON report, record the compiler and
Python versions, note host/runtime limitations, and keep any follow-up live
database qualification as a separate issue.
