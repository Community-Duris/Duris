# Deployment

## Repository-Owned Validation

Pull requests to `master` run build/test, code-quality, and security workflows. The
local equivalents are:

```bash
./scripts/format.sh --check
python3 tests/async/test_compiler_warning_profile.py
make test-all
make security-check
```

## Local Start and Probe

```bash
./scripts/start_mud.sh
scripts/healthcheck.sh
```

The health endpoint is served by the WebSocket listener. Set
`DURIS_WEBSOCKET_PORT` and `DURIS_HEALTH_URL` together when validating an isolated
local instance on a non-default port.

## Release and Rollback Boundary

This repository does not declare a production hosting provider, deployment trigger,
service account, or public URL. Release authorization and platform rollback therefore
remain operator-owned external decisions. Schema changes must first pass on a backed-up
development clone; MySQL DDL recovery uses the known backup rather than a guessed rollback.

For executable startup, migration, restore, and binary-history procedures, use the
[Operations Runbook](RUNBOOK.md). Do not place deployment secrets in tracked files.
