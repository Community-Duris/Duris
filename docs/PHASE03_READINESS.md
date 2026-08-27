# Phase 03 Final Readiness Gate

This fail-closed gate is the only evidence boundary that can support a 200-player
readiness claim. It never uses `.env` implicitly and must not target a production,
shared, configured development, or default Redis/database/game endpoint.

## Required Inputs

Prepare an untracked permission-restricted JSON configuration outside the repository or
under ignored `tmp/`. It contains no credentials. It names stable evidence IDs,
aggregate counts, isolated non-default ports, at least 200 sanitized identities, an
approved RPO, an approved lifecycle policy, and argv arrays for deployment-owned
qualification, workload, reversible-fault, and aggregate-reconciliation adapters. The
qualification adapter must independently remeasure the aggregate table counts, identity
count, target isolation, production unreachability, and non-default ports; its evidence
must exactly match the declared configuration before any workload adapter can run.

The target must be a backed-up representative clone that production cannot reach. Every
minimum in `tests/async/session14_gate_manifest.json` must be measured from that clone.
Do not fabricate rows, copy player values into evidence, or use the configured database.

Start from `tests/async/session14_gate_config.example.json`. The example is deliberately
unqualified: it uses default ports, zero identities, pending policy/RPO markers, and no
counts. Copy it only into ignored `tmp/`, replace every evidence value from the isolated
deployment, and keep adapter credentials outside the JSON and repository.

## Adapter Protocol

Every adapter is an executable argv array, receives exactly one ASCII JSON request on
standard input, emits exactly one ASCII JSON response on standard output, and returns
nonzero on failure. The runner never invokes a shell and discards adapter standard error.

| Adapter | Required response fields |
|---------|--------------------------|
| Qualification | `schema_version`, `state=qualified`, `qualification_evidence_id`, `target_kind`, `production_unreachable`, `identity_count`, exact `aggregate_table_counts`, exact isolated `ports` |
| Workload | `schema_version`, exact `case_id`, `state=passed`, unique `evidence_id`, complete `metrics` object |
| Fault | `schema_version`, exact `action`, phase-specific `state` (`ready`, `injected`, `verified`, `restored`), unique `detail_id` |
| Reconciliation | `schema_version`, exact `reconciliation_id`, `mismatches=0`, unique `evidence_id` |
| Privacy/migration/restore | `schema_version`, exact `privacy_case_id`, `state=passed`, unique `evidence_id` |

Evidence IDs are bounded ASCII identifiers and must be unique across the complete run.
An adapter error becomes categorical sanitized failure evidence. If fault teardown fails,
all later fault and mutation-capable privacy cases are skipped and the gate fails.

## Preflight

```bash
python3 scripts/session14_gate.py \
  --config tmp/session14-gate/config.json \
  --preflight-only
```

`QUALIFIED` means only that declared inputs passed qualification. It is not readiness
evidence and performs no workload.

## Complete Gate

```bash
python3 scripts/session14_gate.py \
  --config tmp/session14-gate/config.json \
  --output tmp/session14-gate/run
```

The workload adapter must keep each 200-client case alive for at least 1800 monotonic
seconds. Eight profiles require at least four hours of hold time before ramps, faults,
repairs, and reruns. Skipped, shortened, partial, failed, stale, or identity-mismatched
evidence is rejected.

## Evidence And Teardown

Raw output belongs under ignored `tmp/session14-gate/` with directory mode 0700 and
file mode 0600. Never commit credentials, target names, IPs, logs, plans, exports,
player/account values, or clone data. Tracked evidence may contain stable case IDs,
aggregate metrics, counts, categorical results, and checksums only.

Every fault runs preflight, injection, verification, and teardown. Teardown failure
fails the gate. Verify target restoration and production unreachability before retry.

## Failure And Rerun

Do not lower thresholds or remove cases. Repair a demonstrated defect narrowly, add a
focused regression, rerun from the earliest affected checkpoint, invalidate prior
build/configuration evidence, then rerun the complete gate.

Phase 03 engineering is complete. The user explicitly deferred the 200-account,
four-hour capacity execution, so the current sanitized outcome makes no 200-player
readiness claim. The gate remains `UNQUALIFIED` until every binding case passes.
