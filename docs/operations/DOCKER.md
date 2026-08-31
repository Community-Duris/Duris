# Docker deployment

Docker Compose is the maintained container alternative for a complete local
DurisMUD setup. It runs the full world with MariaDB persistence and keeps the
host free of project build and database dependencies. Production remains on the
systemd path in the [operations runbook](RUNBOOK.md#production-systemd-service).

## Prerequisites

- Docker Engine with the Compose plugin (`docker compose version`).
- Python 3, used once to generate random local database credentials.
- Enough build capacity for the C++20 server. The default uses two compiler
  jobs; lower `DURIS_BUILD_JOBS` in `.env.docker` on a memory-constrained host.

## First deployment

From the repository root:

```bash
./scripts/init_docker_env.sh
docker compose --env-file .env.docker up --build --detach --wait
docker compose --env-file .env.docker ps
curl http://127.0.0.1:4050/health
```

The initializer creates `.env.docker` once, with mode `0600` and independent
random application and root database passwords. It refuses to overwrite an
existing file. Edit its non-secret values before the first `up` to change the
build concurrency, loopback bind address, or published ports.

The first start performs the following guarded sequence:

1. MariaDB creates `duris_dev` in a new database volume and loads
   `migrations/bootstrap_multithread_safe.sql`.
2. The game container waits for that initialized database through a shared
   Unix socket. This keeps the existing loopback-only migration safety boundary
   intact and does not publish MariaDB to the host.
3. The tracked help/login content is imported, the exact fresh baseline is
   adopted, pending immutable migrations are applied, and runtime compatibility
   is verified.
4. The full world data is generated and the server starts as the unprivileged
   `duris` user. Compose reports the service healthy only after the HTTP
   endpoint reports ready persistence.

Connect to the plain local listener with:

```bash
nc 127.0.0.1 4000
```

The default listeners are plain telnet on `4000`, self-signed TLS telnet on
`4001`, and WebSocket/HTTP health on `4050`. They are published on host loopback
only. The generated certificate is intended for local use and persists in the
`duris-runtime` volume.

## Routine lifecycle

Always pass the generated environment file so Compose can render the stack:

```bash
# Follow server and database output.
docker compose --env-file .env.docker logs --follow game mariadb

# Stop containers while retaining all data.
docker compose --env-file .env.docker down

# Start them again without rebuilding.
docker compose --env-file .env.docker up --detach --wait

# Rebuild after pulling source changes; named-volume data is retained.
docker compose --env-file .env.docker build --pull game
docker compose --env-file .env.docker up --detach --wait
```

The game launcher takes a database backup before each boot. Backups, player-save
and critical-command journals, maintenance state, and the local TLS key are in
the `duris-runtime` volume. Game logs are in `duris-logs`; MariaDB data is in
`mariadb-data`. The transient `mariadb-socket` volume carries no durable data.

Inspect a health failure without exposing credentials:

```bash
docker compose --env-file .env.docker ps
docker compose --env-file .env.docker logs --tail=200 game
docker compose --env-file .env.docker exec game ./scripts/healthcheck.sh
```

An existing native `.env` is neither copied into the image nor loaded by
Compose. Container configuration comes only from `compose.yaml` and the secret
values interpolated from `.env.docker`.

## Clean local reset

Ordinary `down`, rebuild, and image removal preserve the named volumes. The
following command permanently deletes the Docker database, backups, journals,
certificate, and logs for this Compose project:

```bash
docker compose --env-file .env.docker down --volumes
```

Use that reset only for a disposable local deployment. It cannot upgrade or
adopt a populated legacy database; follow the guarded clone-and-migration
procedure in the [database guide](../reference/DATABASE.md) for existing data.

## Production boundary

The Compose stack intentionally selects `ENVIRONMENT=local`, a development
server build, loopback host publishing, a self-signed certificate, and automatic
fresh/local migration application. Do not expose it as the production service.
Production requires operator-provided certificates, remote database TLS,
qualified migration rollout, backups, and the checked-in systemd service.
