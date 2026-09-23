# Server environments — `duris` and `duris-prod`

Status: complete, 2026-09-23. `duris` (staging) now runs on the `duris-prod`
host as the MUD-only `duris-staging` account, with no website connection. The
Plesk install is stopped and kept for rollback. See
[Migration plan](#migration-plan).

## SSH aliases

All three aliases are defined in the workstation's `~/.ssh/config`. The old
entry is saved as `~/.ssh/config.bak-20260923`.

| Alias | Role | Host | Login | Key |
| --- | --- | --- | --- | --- |
| `duris` | Public staging/dev | `178.156.165.10` | `duris-staging` | `~/.ssh/duris_ed25519` |
| `duris-prod` | Production | `178.156.165.10` | `duris` | `~/.ssh/duris_prod_ed25519` |
| `duris-plesk` | Former staging, stopped | `plesk.luminarimud.com` (`74.208.126.44`) | `duris` | `~/.ssh/duris_ed25519` |

`duris` and `duris-prod` share a machine but are separate accounts and separate
installs. `duris-staging` has no sudo. Host-level work, such as `ufw`, goes
through `duris-prod`.

## `duris` — staging/dev

This is a public, production-like server for testing and development. It runs
[Chaos mode](../reference/CHAOS_MODE.md) with `CREATION_ALL_RACES` and
`CREATION_ALL_CLASSES` on. It connects to no website: there is no WebSocket
listener, no tunnel and no website Redis.

- Players connect to `mud.duris.sbs:4000` for plain telnet and
  `mud.duris.sbs:4001` for TLS. `.env` sets `DURIS_PRODUCTION_PORT=4000`,
  `DURIS_TLS_PORT=4001` and `LISTEN_ADDRESS=178.156.165.10`.
- The checkout is `/home/duris-staging/duris`, on the local branch
  `codex/rollback-sbs-20260923`. That branch is `a6a2124c1` plus `0695680df`,
  the cherry-picked port change. The server is built with
  `make -C src PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production`.
- Everything runs as `systemctl --user` units, with lingering enabled:
  - `duris-mud-production`, which runs `cycle_mud.sh --production`
  - `duris-mariadb`: MariaDB 10.11.14 unpacked in `~/.local/opt/mariadb`, on
    127.0.0.1:3307
  - `duris-redis`: the host's `/usr/bin/redis-server`, on 127.0.0.1:6381
  - `duris-backup-backup.timer` and `duris-certbot-renew.timer`
- `mud.duris.sbs` is a DNS-only A record with a 300-second TTL, pointing at
  `178.156.165.10`. Its certificate is renewed by DNS-01 through the Cloudflare
  token in `.env`, and it is valid until 2026-11-28.

The former Plesk install (`duris-plesk`) has every Duris and DurisWeb user unit
stopped and disabled, including the `duris.sbs` website and both tunnels. Its
data is untouched.

## `duris-prod` — production

This is the live `newduris.com` server. Its topology, services, availability
safeguards and recovery procedures are recorded in
[PRODUCTION_DEPLOYMENT.md](../operations/PRODUCTION_DEPLOYMENT.md). A Valheim
server, run by the `steam` account, shares the host.

Never run migrations, wipes or operational scripts here without the owner's
permission.

## Port check (2026-09-23)

Checked read-only on both hosts.

- The host is a Hetzner Cloud server in `us-east`/`ash-dc1`, and `ufw` is its
  only inbound filter. It denies by default and allows 22, 80/443, 7777/7778
  and 2456–2458/udp for Valheim.
- In the past 7 days `ufw` dropped traffic to about 14,500 distinct ports, so
  no provider firewall filters traffic before it reaches the host. Opening a
  port only needs a `ufw` rule, and the `duris` login has passwordless sudo.
- We don't have access to the Hetzner account, so we can't add a second IPv4
  address.

| Staging use | Port on `duris` | On `duris-prod` |
| --- | --- | --- |
| Plain telnet | public 7777 | **In use** by the production MUD, bound to `178.156.165.10` |
| TLS telnet | public 4001 | Free |
| MUD WebSocket | loopback 4050 | In use by the production MUD; not needed (no website) |
| MariaDB | loopback 3307 | Free |
| MUD Redis | loopback 6380 | In use by the production website cache |
| Website and its Redis | loopback 7770, 7778 | Not moving |

With one IPv4 address, staging's plain telnet has to leave 7777. The launcher
(`scripts/cycle_mud.sh:140`) and the server (`src/sql/sql.c:837`) both refuse
the production role on any port other than 7777. On any other port, staging's
`DB_NAME=duris` is redirected to `duris_dev` (`src/sql/sql.c:1435`,
`scripts/cycle_mud.sh:179`).

Options considered:

- **Configurable production port (chosen).** A small code change that keeps
  every production safeguard.
- **`ENVIRONMENT=staging`.** Every `production` gate in the server and in about
  a dozen scripts would need a staging decision. Missing one would weaken a
  public server or break its backups.
- **Private address plus DNAT.** Needs no code, but adds hidden NAT plumbing to
  the production firewall.
- **Hetzner Floating IP.** Needs the server's renter.

## Migration plan

This happens between player wipes, so a little downtime or lag on either
server is acceptable. The plan does one code change and then moves staging in
a single session.

### Decisions

- **Ports.** Public plain telnet moves to **4000** through a new
  `DURIS_PRODUCTION_PORT`, which defaults to 7777. TLS stays on **4001**. Both
  bind `LISTEN_ADDRESS=178.156.165.10`. MariaDB stays on 127.0.0.1:3307, and
  Redis moves to 127.0.0.1:6381 because production's website cache holds 6380.
- **No website.** Only the MUD moves. `DURIS_WEBSOCKET_*` and
  `DURIS_TRUSTED_PROXY_IP` are removed from `.env`, so the production guard
  refuses to open a WebSocket listener before it binds (`src/net/websocket.c:430`).
  Donations and private presence are already off in staging's `.env`. The
  database and Redis are private to staging. The website, its Redis and both
  tunnels stay on Plesk and are shut off.
- **Account.** A new `duris-staging` account with no sudo, using the existing
  staging key and user units. Staging's unit names are kept. Using production's
  `duris` account would clash on `/home/duris/duris` and the unit names.
- **Database.** MariaDB 10.11.14, the same build staging runs, unpacked with
  `apt-get download` and `dpkg-deb -x`. `apt install` would remove production's
  MySQL because the packages conflict. The datadir is copied as-is. Its
  clients go on the units' `PATH`, because the pre-boot backup needs a
  MariaDB `mysqldump`.
- **Redis and TLS.** Redis uses the host's `/usr/bin/redis-server` 7.0.15, the
  same version. Staging's certbot directory and its Cloudflare DNS-01 hooks
  come across unchanged.
- **Code.** Deploy with the change added. At boot, `cycle_mud.sh` applies any
  pending migrations. Phase 2, step 5 covers why the deploy stayed on
  staging's rollback commit.

### Phase 1 — code

- [x] Add `DURIS_PRODUCTION_PORT` (default 7777) to `src/sql/sql.c:837` and
  `:1435` and to `scripts/cycle_mud.sh:71`, `:140` and `:179`. `DURIS_DEV_PORT`
  must still differ from it. Document it in `.env.example`.
- [x] Update the contracts in `tests/async/test_runtime_connection_trust.py` and
  add a focused regression test. Run `make -C src` and the tests.
  The launcher cases are in `tests/async/test_flatfile_launcher.py`, and
  `tests/async/test_valgrind_tooling.py` follows the new guard.
- [x] Commit to `master`, then cherry-pick onto `codex/master-stable`: `a7644b9f0` on
  `master` and `f2a1bcf18` on `codex/master-stable`. Both builds passed, and
  the focused tests passed on both branches.

### Phase 2 — move (one session)

1. [x] `duris-prod`, with sudo: create `duris-staging` (uid 1002) with
   `~/.ssh/duris_ed25519.pub` and enable lingering. Run
   `ufw allow 4000/tcp` and `ufw allow 4001/tcp`, commented "Duris staging".
2. [x] Plesk: run `systemctl --user disable --now` on every Duris and DurisWeb
   unit and timer. The MUD didn't exit within its 90-second stop timeout and
   was killed. MariaDB then shut down cleanly. The DurisWeb units were linked
   unit files, so disabling them removed their links.
3. [x] Stream Plesk directly to `duris-prod` with `ssh -A` and `tar | zstd`.
   This took 20 seconds: 1.1 GB of checkout and 3.0 GB of MariaDB. Two things
   differ from the plan:
   - Only `bin/` and `logs/old-logs/` were excluded from `~/duris`.
   - The `duris-mud-production.service.d` drop-ins came too. They keep
     `SKIP_PREBOOT_BACKUP=1`, as on Plesk, so the timer does the backups.
4. [x] As `duris-staging`:
   - unpack MariaDB 10.11.14 into `~/.local/opt/mariadb`, and set its
     `basedir`, `lc-messages-dir`, `character-sets-dir` and `plugin-dir` in
     `my.cnf`
   - rewrite the paths and repoint the `duris.crt`/`duris.key` symlinks
   - make the unit, Redis-port and `.env` changes as planned
   - restore the tracked `bin/.gitignore` and `logs/old-logs/.gitignore`,
     which the copy had excluded
5. [x] Deploy and start. There are two deviations:
   - **Code.** On Plesk, staging had been deliberately rolled back from
     `72238a8f5` (the `codex/master-stable` head) to `a6a2124c1` at 05:33 UTC,
     on the local branch `codex/rollback-sbs-20260923`. To keep that rollback,
     the port change was cherry-picked onto it as `0695680df`, rather than
     deploying the `codex/master-stable` head.
   - **Build.** Production mode needs
     `make -C src PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production`.

   The MUD booted in 13.7 seconds. `dms` listens only on
   `178.156.165.10:4000/4001`, with no WebSocket listener.
6. [x] Point the `mud.duris.sbs` A record at `178.156.165.10`, using the
   Cloudflare token in `.env`. It is DNS-only with a 300-second TTL, and it
   resolves at 1.1.1.1.
7. [x] Smoke test, run from the workstation through `mud.duris.sbs`:
   - The test account entered the game and quit cleanly on 4000, and again on
     4001. The TLS connection used TLSv1.3 with a verified `mud.duris.sbs`
     certificate.
   - Production still runs the same `dms` PID, 1704623, on 7777/7778/4050.
   - `mud.newduris.com/health` reports `healthy` and `www.newduris.com`
     returns 200.

   The copy had left `~/.duris-backups`, `~/.local`, `~/.local/share` and
   `duris/Players` group-writable. The first scheduled backups failed with
   `writable_ancestor`. After `chmod go-w` (and `700` on `~/.duris-backups`),
   the backup service produced a generation and returned `ok`.
8. [x] Point `Host duris` in `~/.ssh/config` at `duris-staging@178.156.165.10`,
   and keep the old host as `duris-plesk`.

Rollback: nothing on Plesk is deleted. Re-enable its units, revert the DNS
record to `74.208.126.44` and restore `~/.ssh/config.bak-20260923`. The Plesk
Redis unit points at a deleted binary, so repoint it at `/usr/bin/redis-server`
first. Deleting the Plesk data waits for the owner's OK.

Players: plain telnet is now `mud.duris.sbs:4000`. TLS stays on
`mud.duris.sbs:4001`.
