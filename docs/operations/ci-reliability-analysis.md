# CI reliability analysis

Analyzed 2026-09-11 against master `a12a67cbb`, with a snapshot of the latest 150 GitHub Actions runs. Proposed changes are local on `codex/ci-reliability`; no workflow or repository settings have been published.

## What the run history establishes

| Workflow | Successful | Failed | Unfinished at snapshot |
| --- | ---: | ---: | ---: |
| compile test | 10 | 21 | 4 |
| code quality | 57 | 1 | 3 |
| security baseline | 32 | 0 | 2 |
| backup recovery | 15 | 3 | 2 |

No run in this snapshot has conclusion `timed_out`. This does not rule out an internal test deadline or older timeout, but the inspected failures do not establish a GitHub job-timeout problem.

- [PR compile run 34617803663](https://github.com/Community-Duris/Duris/actions/runs/34617803663): build job lasted 48m24s; Python runner took 2675.74s (44m36s), with 461 passing tests and two failures. Those failures were formatting in `tests/async/shopkeeper_population_harness.cpp` and calls to missing `server_build_artifacts.effective_configuration`.
- [Master compile run 34618047382](https://github.com/Community-Duris/Duris/actions/runs/34618047382): the same two failures; 463 passed and two failed in 2694.44s. This demonstrates failures also present on master, rather than establishing a regression introduced by the PR.
- [Earlier documentation-branch compile run 34592026921](https://github.com/Community-Duris/Duris/actions/runs/34592026921): eight failures in 496.65s, including artifact helper crashes when compiler lookup produced `None`. The current helper contains marker-based parsing to address recursive Make output, but its test still calls the old function name.
- [PR backup run 34623454268](https://github.com/Community-Duris/Duris/actions/runs/34623454268): seven tests ran in 140.299s, with five errors at restore qualification. The only surfaced reason is `subprocess_failed`. These must remain failures until their underlying cause is known.
- [Quality run 34623454274](https://github.com/Community-Duris/Duris/actions/runs/34623454274) reports changed-line formatting success on a clean checkout. The separate compile gate checks the whole tracked tree and catches the formatting defect.

## Dominant latency: cache validation and serial journeys

In run 34617803663 the eight serial tests consumed approximately 36m9s. Seven reported artifact timing:

| Test | Total seconds | Build seconds | Artifact lookup/validation seconds |
| --- | ---: | ---: | ---: |
| account recovery | 1107.53 | 160.285 | 932.167 |
| area coin pickup | 216.29 | 0 | 95.545 |
| flatfile boot preflight | 35.72 | 0 | 34.411 |
| new character kit | 262.24 | 0 | 93.117 |
| combat journey | 323.28 | 0 | 34.748 |
| first session currency | 217.18 | 0 | 89.307 |
| full world boot | 122.19 | 0 | 38.741 |

Reported lookup time totals 1318.036s (about 22 minutes). The metric includes fingerprinting, locking and publication work; it is not a CPU profile of hashing alone. Code inspection identifies broad recursive hashing as the prime optimization target: `toolchain_key()` walks `/usr/include`, `/usr/lib/gcc`, `/usr/local/include`, and `/usr/local/lib`, plus compiler library search paths. `input_key()` repeats it for every lookup and twice for a cache miss. `/usr/local/lib` can contain large unrelated installed software. The cache already reuses binaries; adding another cache around the same lookup logic would leave this cost in place.

Recommended implementation:

1. Establish an explicit CI build stage after dependency installation, with a fixed toolchain and compiler flags. Produce immutable artifacts per backend and record source revision, configuration, toolchain identity and binary checksum.
2. Pass that exact run's artifacts to runtime jobs and verify their manifest/checksum. Keep the current conservative content invalidation for developer worktrees until an equally correct replacement is tested. Do not use a cross-run cache hit as sufficient proof of correctness.
3. If keeping dynamic fingerprinting, restrict it to compiler and actual relevant dependency inputs, handle custom include/library flags, and verify invalidation for edited headers, changed flags, and corrupt artifacts. A process-local memo will not eliminate repeated work across separate test processes.
4. Measure cold build, warm artifact verification, and runtime independently. Set improvement targets only after repeated measurements; the table shows opportunity, not a validated speedup.

## Required PR gate structure

Separate fast contracts/formatting, native compilation, runtime journeys, backup recovery and security. First fix the shared artifact cost, then distribute long journeys across separate jobs with isolated ports, temporary state and processes. Do not simply remove their serial classification on one runner: that can increase resource contention and introduce fixture collisions.

Preserve the client-free build in a job where MySQL headers/libraries are absent. CodeQL needs its own correctly instrumented build; avoid accidentally replacing it with an untraced cached binary. The ordinary quality compile duplicates the normal server compile and can be consolidated after checking equivalent flags and coverage.

Keep one stable aggregate required check that depends on every mandatory job, runs even when a dependency fails, and fails for failure/cancellation/unexpected skips. Confirm active repository policy before installing it. The classic master branch-protection endpoint returned "Branch not protected" and the repository rulesets listing was empty during this audit; organization-wide enforcement was not independently established. Do not automatically change repository policy as part of this local patch.

## Timeout and reporting controls

`tests/run_regression_tests.py` captures child output with no outer timeout and prints failure tracebacks only after all tests finish. A hung test can therefore hold the gate indefinitely until a platform limit, and an early known failure can remain unexplained for tens of minutes.

Add per-test deadlines by class, process-group termination with escalation and child reaping, periodic running-test/elapsed-time messages, immediate failure logs, and a machine-readable timing/result artifact. Validate this with a deliberately hung process that spawns a child, and with a deliberate assertion failure. Distinguish assertion, timeout, signal/OOM, dependency setup and cancellation in the summary.

Use measured budgets with headroom; do not apply a 10-minute limit to today's 18-minute account journey before fixing its lookup cost. The compile workflow currently has no explicit job deadline; GitHub's default is 360 minutes. Backup recovery explicitly has 30 minutes, but the sampled failure finished much sooner.

Retries should be bounded to identified transient infrastructure operations, not blanket retries of failed assertions or recovery tests. Infrastructure failure should be labeled accurately and remain non-passing if coverage was not completed.

## Misleading failure reporting

- `scripts/persistence_backup.py:run()` discards stderr and raises only `subprocess_failed`. Preserve bounded, sanitized qualifier diagnostics in disposable CI evidence and report command identity, exit status and phase. Do not dump production credentials or recovered state. Reproduce the five qualifier errors before changing timeout or acceptance policy.
- Security enforcement currently describes a Trivy action failure as a fixed HIGH/CRITICAL vulnerability whenever a nonempty report exists. Validate the report's vulnerability records separately from scanner execution errors, and label skipped scans after an upstream failure correctly. Keep both incomplete scan coverage and genuine policy violations blocking.
- Formatting must compare committed content. The local patch uses the same full-tree scope already enforced in `test_formatting_tooling.py`, so the quality check no longer provides contradictory reassurance.

## Local patch and validation

- Added PR-scoped supersession to all four workflows. The group includes the workflow name; non-PR executions use a unique run ID so every master/scheduled execution is retained.
- Restricted quality push triggers to master; PR updates still trigger quality once through `pull_request`.
- Changed the quality formatter invocation to `./scripts/format.sh --all --check`.
- Updated two stale helper test calls to `compiler_configuration`.
- Ran `test_server_build_artifacts.py` successfully in a Linux container with GNU Make. It covers parsing, inherited overrides, invalidation, corruption, isolation, concurrent publication and failed publication.
- Reproduced the existing harness formatting failure with Ubuntu clang-format 18.1.3. The local patch intentionally leaves the unrelated harness content for its owning change; the full gate will remain red until that defect and backup qualification errors are repaired.
- Parsed all four workflow YAML files and checked trigger/concurrency/formatting configuration; `git diff --check` passed. This is not a hosted Actions execution.
- Full CI, native compilation, runtime journeys and backup integration were not rerun. The pipeline restructuring and fingerprint optimization above are recommendations, not implemented changes.

GitHub references: [workflow syntax and default deadlines](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax), [workflow concurrency](https://docs.github.com/en/actions/how-tos/write-workflows/choose-when-workflows-run/control-workflow-concurrency).
