# Incident Response

Use the main [Operations Runbook](RUNBOOK.md) as the executable source of truth.
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

## DurisWeb Hook Mismatch Or Bridge Failure

1. Open Hook Control and record only the hook id, WEB/MUD/effective states,
   bridge scheme/host, and sanitized error. Do not copy secrets or payloads.
2. If a networked bridge uses `ws://`, leave it blocked. Restore the local TLS
   reverse proxy and a certificate-valid `wss://` endpoint; never disable
   certificate verification. Loopback-only `ws://` is the sole plaintext case.
3. For urgent containment, disable the website end first. For a MUD-gated hook,
   use the runbook's Set Both Ends flow or the in-game property command; if the
   latter is used, run `properties save` after verifying the state.
4. On reconnect, wait for a fresh complete `hook_state` frame. Treat an omitted
   id or absent frame as `UNKNOWN`, not enabled, and do not open the website
   gate until the intended MUD state is observed.
5. If rotation is in progress, verify the current/previous secret deployment
   order. The backend retries the previous key once; repeated rejection is a
   configuration or compromise signal, not a reason to loop or log a key.
6. Confirm WEB, MUD, and effective state agree, exercise the affected path, and
   then remove temporary previous credentials according to the runbook.

## Suspected Data Exposure

Stop further distribution, preserve minimal evidence, and follow the private reporting
process in [`SECURITY.md`](../../SECURITY.md). Do not attach database dumps, export
packages, credentials, or player/account values to a public issue.
