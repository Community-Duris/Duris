# Production deployment tracker

Last verified: 2026-08-30 18:28 UTC

## Objective

Run DurisMUD as a persistent production service for `duris.sbs`, backed by an
account-local MariaDB installation. Use ports that were confirmed free before
binding, keep secrets out of the repository, and verify public connectivity,
TLS, persistence, backups, restart recovery, and boot recovery.

This file intentionally records no passwords, API tokens, tunnel tokens, or
private keys. Those remain in owner-controlled ignored files.

## Production topology

| Component | Endpoint or location | State |
| --- | --- | --- |
| Checkout | `/home/duris/duris` | Deployed from `master` |
| MariaDB | `127.0.0.1:3307` | Active, account-local |
| Database | `duris`, user `duris_prod@127.0.0.1` | 173 tables; runtime contract valid |
| Plain telnet | `74.208.126.44:7777` | Listening locally; optional unencrypted ingress remains blocked by Plesk |
| TLS telnet | `mud.duris.sbs:4001` | Public and playable with a trusted certificate |
| WebSocket/health origin | `127.0.0.1:4050` | Healthy; intentionally loopback-only |
| Public WebSocket/health | `https://ws.duris.sbs` / `wss://ws.duris.sbs` | Live through Cloudflare Tunnel |
| Raw-MUD DNS | `mud.duris.sbs` | DNS-only A record to the server |
| TLS certificate | `mud.duris.sbs` | Let's Encrypt; expires 2026-11-28 |
| Tunnel | `duris-production` (`aec07955-bcc1-4faa-9588-f28d45edc474`) | Healthy, four edge connections |

## Implemented

- [x] Clone the repository into the project root.
- [x] Install MariaDB as an account-local service without replacing the host's
  system database service.
- [x] Select free loopback database port 3307 and restrict it to
  `127.0.0.1`.
- [x] Create the production schema and loopback-only production database user.
- [x] Bootstrap and verify all 173 runtime tables and four immutable migrations.
- [x] Import 2,155 help pages and all three `mud_info` records.
- [x] Copy `.env.example` to ignored `.env`, add production database and network
  values, retain the supplied Cloudflare values, and enforce mode `0600`.
- [x] Configure MariaDB-primary persistence and owner-only journal directories.
- [x] Build the production server and full world with account-local build/runtime
  dependencies under `/home/duris/.local/opt/duris-deps`.
- [x] Add explicit development and production build profiles, isolate their
  object trees, remove `TEST_MUD` from production, and make the production
  launcher reject an unstamped or development-profile binary.
- [x] Confirm ports 7777, 7778, and 4050 were unused immediately before the
  initial start. Later confirm port 4001 was locally free and publicly allowed
  before assigning it as the independent production TLS port.
- [x] Issue and install a trusted Let's Encrypt certificate for
  `mud.duris.sbs` using Cloudflare DNS validation.
- [x] Install and verify an account-level certificate renewal timer and a full
  staging renewal dry run. Add and exercise a deploy hook that restarts the
  game only after Certbot successfully deploys a renewed certificate.
- [x] Install persistent user services for MariaDB and DurisMUD.
- [x] Enable user lingering so the account services start at boot and survive
  logout.
- [x] Promote the stamped `mariadb/production` binary with a controlled
  game-service restart; verify a new runtime PID, listener recovery, healthy
  persistence, the expected build ID, and no `TEST_MUD` marker.
- [x] Verify pre-boot compressed database backups under `db/Backup`.
- [x] Install checksum-verified `cloudflared` 2026.8.2 account-locally.
- [x] Create a remotely managed Cloudflare Tunnel and proxied
  `ws.duris.sbs` route to the loopback WebSocket/health origin.
- [x] Verify public `GET /health` through Cloudflare returns
  `{"status":"healthy","persistence":"ready"}`.
- [x] Verify a public WSS upgrade through Cloudflare returns HTTP 101 and the
  game greeting `Welcome to NewDuris MUD!`.
- [x] Add `DURIS_TLS_PORT` so production TLS can use an independently selected
  port without moving the plain listener. Deploy port 4001 after proving it was
  unused locally and reachable through the existing Plesk policy.
- [x] Verify secure telnet on `mud.duris.sbs:4001` from five independent
  external regions, negotiate TLS 1.3 with hostname validation, and reach the
  live account prompt.
- [x] Verify all 350 repository tests pass in 292.08 seconds after repairing
  three brittle or environment-sensitive test harnesses and adding the TLS-port
  contract; formatting and focused production build/service checks also pass.

## Service and configuration locations

- MariaDB configuration:
  `/home/duris/.config/duris-mariadb/my.cnf`
- MariaDB data:
  `/home/duris/.local/share/duris-mariadb`
- MariaDB runtime state/socket:
  `/home/duris/.local/state/duris-mariadb`
- MariaDB service:
  `/home/duris/.config/systemd/user/duris-mariadb.service`
- Game service:
  `/home/duris/.config/systemd/user/duris-mud-production.service`
- Cloudflare connector service:
  `/home/duris/.config/systemd/user/duris-cloudflared.service`
- Cloudflare connector launcher:
  `/home/duris/.local/libexec/duris-cloudflared`
- Certificate renewal service/timer:
  `/home/duris/.config/systemd/user/duris-certbot-renew.service` and
  `/home/duris/.config/systemd/user/duris-certbot-renew.timer`
- Certificate deployment hook:
  `/home/duris/.local/libexec/duris-certbot-deploy`
- Certificate storage:
  `/home/duris/.config/letsencrypt`
- Runtime certificate links:
  `/home/duris/duris/duris.crt` and `/home/duris/duris/duris.key`
- Production secrets and connection values:
  `/home/duris/duris/.env` (ignored, mode `0600`)

## Optional follow-up work

- [ ] If unencrypted public telnet is desired in addition to the deployed TLS
  and WSS transports, allow inbound TCP 7777 in the root-owned Plesk firewall.
  It is not required for secure production access.
- [ ] Decide whether `duris.sbs` should host a browser client or landing page.
  The game WebSocket is publicly available at `wss://ws.duris.sbs`, but this
  repository does not contain a deployable browser frontend.

## Verification commands

Run these without printing `.env`:

```bash
systemctl --user is-active \
  duris-mariadb.service \
  duris-mud-production.service \
  duris-cloudflared.service

./scripts/cycle_mud.sh --production --check-config
./migrations/verify_runtime_compatibility.sh
./scripts/healthcheck.sh
curl --fail --silent --show-error https://ws.duris.sbs/health

openssl s_client \
  -connect mud.duris.sbs:4001 \
  -servername mud.duris.sbs \
  -verify_hostname mud.duris.sbs \
  -verify_return_error </dev/null
```

Expected health payload:

```json
{"status":"healthy","persistence":"ready"}
```

## Security invariants

- MariaDB and WebSocket origins remain loopback-only.
- Production database credentials and Cloudflare credentials remain only in
  `.env`; this document and tracked files contain no secret values.
- The MariaDB account is limited to the `duris` schema and loopback source.
- The production launcher accepts only a stamped MariaDB/production build, and
  the active binary contains no development-only `TEST_MUD` behavior.
- The TLS private key and `.env` are owner-controlled with mode `0600`.
- The certificate renewal deploy hook is owner-controlled with mode `0700` and
  restarts the game only after a successful certificate deployment.
- The public WebSocket origin allow-list remains restricted to
  `https://duris.sbs` and `https://www.duris.sbs`.
- The Cloudflare connector retrieves its tunnel token at startup, removes the
  account API credentials from its environment before executing `cloudflared`,
  and exposes metrics only on verified-free loopback port 20242.
