# Onboarding

## Prerequisites

- A Debian/Ubuntu development host or equivalent packages from
  [`packaging/duris-build-deps.equivs`](../packaging/duris-build-deps.equivs).
- A local MySQL or MariaDB database reserved for development.
- An owner-controlled `.env` created from `.env.example`.

## Setup

Follow the root [Quick start](../README.md#quick-start) to install dependencies,
configure `.env`, create the development schema, build, and start the server.

## Verify

```bash
make test-all
./scripts/start_mud.sh
```

For a foreground local instance, use `./scripts/cycle_mud.sh --dev`. Confirm the
WebSocket and database readiness endpoint with `scripts/healthcheck.sh`.

Database-backed test suites are intentionally separate; see [Testing](TESTING.md)
before running `make test-db`.
