# Production deployment tracker

Last verified: 2026-09-14 23:11 UTC

## Objective

Run DurisMUD and the DurisWeb website as persistent production services for
`newduris.com`. Services must recover from any exit on their own, and an
off-host check must alert when they do not.

This file intentionally records no passwords, API tokens, tunnel tokens, or
private keys. Those remain in owner-controlled ignored files. The earlier
`duris.sbs` deployment is not tracked here; its setup record is in this file's
Git history.

## Production topology

| Component | Endpoint or location | Notes |
| --- | --- | --- |
| Host | `178.156.165.10` | Ubuntu 24.04, systemd 255; services run as `duris` |
| MUD checkout | `/home/duris/duris` | Deployed from `master` |
| MUD service | `duris-mud-production.service` | System unit running `scripts/cycle_mud.sh --production` |
| Database | `mysql.service`, `127.0.0.1:3306`, schema `duris_game_prod` | MySQL 8.0; `PERSISTENCE_MODE=mariadb-primary` |
| MUD Redis | `redis-server.service`, `127.0.0.1:6379` | Namespace `duris:production:default` |
| Plain telnet | `mud.newduris.com:7777` | DNS-only A record to the host |
| TLS telnet | `mud.newduris.com:7778` | Let's Encrypt; expires 2026-12-05 |
| MUD WebSocket/health origin | `127.0.0.1:4050` | Loopback-only |
| Public MUD WebSocket/health | `wss://mud.newduris.com`, `https://mud.newduris.com/health` | Nginx TLS proxy to the origin |
| Website checkout | `/home/duris/website` | `Community-Duris/DurisWebApp`, deployed from `master` |
| Website application | `durisweb-production.service`, `127.0.0.1:3001` | Private cache `durisweb-redis.service` on `127.0.0.1:6380` |
| Website tunnel | `durisweb-cloudflared.service`, tunnel `5b7d0472-7d5b-4c6e-8aa3-cd550e2bdb60` | `www.newduris.com` routes to the application; `newduris.com` routes to Nginx port 80, which redirects to `www` and passes `/health` through |
| Tunnel readiness | `http://127.0.0.1:20243/ready` | Loopback-only |
| Watchdog | `durisweb-watchdog.timer` | Runs `/usr/local/sbin/durisweb-watchdog` every minute |

The `newduris.com` zone and website tunnel belong to a different Cloudflare
account from `duris.sbs`. API work for them uses the credentials in
`/home/duris/.config/durisweb/deployment.env`, not a workstation `.env`.

## Availability safeguards

On 2026-09-10 the website tunnel exited cleanly after losing every edge
connection. Its unit restarted only on failure, so `newduris.com` served
Cloudflare error 1033 until the connector was started by hand on 2026-09-14.
Later that day, a maintenance stop and start of the website application stopped
the tunnel again, because the tunnel was bound to the application, and the site
was down for another 40 minutes.

- The website application, cache, and tunnel restart after any exit other than
  configuration refusal, with no start rate limit, and the tunnel is no longer
  bound to the application. DurisWeb `docs/deployment.md` ("Keep the site
  available") describes the policy.
- `durisweb-watchdog.timer` starts any stopped website unit or Nginx. It
  restarts the tunnel after three failed readiness or public-health checks and
  the application after three failed local-health checks, at most once per unit
  every ten minutes.
- `nginx`, `mysql`, and `redis-server` have
  `/etc/systemd/system/<unit>.service.d/10-availability.conf` overrides that set
  `Restart=always`, `RestartSec=5s`, and `StartLimitIntervalSec=0`.
- `duris-mud-production.service` uses `Restart=always` with no start rate limit
  (`deploy/systemd/duris-mud-production.service.in`).
- The `production uptime` workflow probes `https://newduris.com/health` and
  `https://mud.newduris.com/health` every ten minutes from GitHub-hosted runners.
  A failed run notifies through GitHub.
- A Cloudflare Tunnel Health Alert emails the Cloudflare account owner when the
  website tunnel goes down.

Fault tests on 2026-09-14 at 23:10 UTC confirmed the behavior. Stopping and
starting only the application left the tunnel running, with 2 seconds of
gateway errors. A clean tunnel exit restarted by itself within 6 seconds. An
explicitly stopped tunnel was started by a watchdog run.

## Planned maintenance

Pause the watchdog before deliberately stopping a website unit, Nginx, or the
tunnel. The watchdog ignores a pause older than four hours.

```bash
sudo touch /var/lib/durisweb-watchdog/pause
# Maintenance...
sudo systemctl start durisweb-redis durisweb-production durisweb-cloudflared
sudo rm /var/lib/durisweb-watchdog/pause
```

Stopping only the MUD does not require a pause. Before ending maintenance, run
the public health checks below.

## Service and configuration locations

- Website deployment input: `/home/duris/.config/durisweb/deployment.env`
  (mode `0600`)
- Rendered website units: `/home/duris/.local/share/durisweb/rendered`,
  installed as root-owned copies under `/etc/systemd/system`
- Watchdog executable: `/usr/local/sbin/durisweb-watchdog`, a root-owned copy of
  the website checkout's `deploy/scripts/durisweb-watchdog`; reinstall it when
  that script changes
- Watchdog state and pause file: `/var/lib/durisweb-watchdog`
- MUD service unit: `/etc/systemd/system/duris-mud-production.service`, installed
  by `scripts/install-production-service.sh`
- MUD secrets and connection values: `/home/duris/duris/.env` (mode `0600`)
- MUD TLS certificate: `/home/duris/duris/duris.crt` and `duris.key`, linked to
  `/var/lib/duris-mud/tls/` and refreshed by the Certbot deploy hook
  `/etc/letsencrypt/renewal-hooks/deploy/duris-mud`
- Pre-boot database backups: `/home/duris/duris/db/Backup`

## Verification commands

Run these on the host without printing `.env`:

```bash
systemctl is-active duris-mud-production mysql redis-server nginx \
  durisweb-redis durisweb-production durisweb-cloudflared durisweb-watchdog.timer
systemctl list-timers durisweb-watchdog.timer
journalctl -u durisweb-watchdog.service --since -1h
curl --fail --silent --show-error http://127.0.0.1:20243/ready
curl --fail --silent --show-error https://newduris.com/health
curl --fail --silent --show-error https://mud.newduris.com/health

openssl s_client \
  -connect mud.newduris.com:7778 \
  -servername mud.newduris.com \
  -verify_hostname mud.newduris.com \
  -verify_return_error </dev/null
```

The website health response must report `"status":"ok"` and
`"service":"durisweb-backend"` with `ok` database and cache checks. The MUD
health response must be `{"status":"healthy","persistence":"ready"}`.

## Security invariants

- MySQL, both Redis instances, the website application, the MUD WebSocket
  origin, and tunnel metrics listen only on loopback.
- Credentials remain only in the mode-`0600` environment files above; this
  document and tracked files contain no secret values.
- The root-run watchdog executes a root-owned copy, never a script in a checkout
  the service account can modify.
- The MUD WebSocket origin allow-list is restricted to `https://newduris.com`
  and `https://www.newduris.com`.
