# Incident Response

Use the main [Operations Runbook](../RUNBOOK.md) as the executable source of truth.
Preserve logs and aggregate health evidence without copying credentials, SQL, player
values, or raw private gate output into tickets or commits.

## Server Does Not Start

1. Inspect the status and console logs named in the main runbook.
2. Check the pre-service database, migration, schema, listener, and certificate reason.
3. Correct configuration or restore the known development backup; do not bypass a
   compatibility failure.
4. Restart and verify with `scripts/healthcheck.sh`.

## Persistence Dependency Failure

1. Inspect the privileged in-game persistence status and aggregate queue/journal age.
2. Keep retryable state and journals intact; do not delete or hand-edit recovery files.
3. Follow the dependency and terminal-save procedures in the main runbook.
4. Reconcile every affected durable domain before declaring recovery complete.

## Suspected Data Exposure

Stop further distribution, preserve minimal evidence, and follow the private reporting
process in [`SECURITY.md`](../../SECURITY.md). Do not attach database dumps, export
packages, credentials, or player/account values to a public issue.
