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
  `codex/rollback-sbs-20260923`. That branch is `a6a2124c1` plus these
  cherry-picks:
  - `0695680df`: the port change
  - `01f401b5a`: the login mode banner (`be50f6c0e` on `master`)
  - `dbc3e5e34`, `4958a654f` and `afdee58cc`: the Stromvok ferry from #618
- `.env` sets `DURIS_STAGING=TRUE`, so the login screen shows
  `*** STAGING | CHAOS | ALL-RACES | ALL-CLASSES ***`, blinking, above the
  account-name prompt. The server is built with
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

### Access for the staging account

- **SSH.** `duris-staging` accepts `~/.ssh/duris_ed25519` plus the two other
  ed25519 keys copied from Plesk's `duris` account. One has no comment; the other
  is labelled `hermes-agent duris@plesk.luminarimud.com 2026-08-31`. The account
  has no sudo. Host-level work, such as `ufw` rules, packages and the system
  journal, goes through `duris-prod`.
- **MariaDB admin.** `'duris-staging'@'localhost'` has
  `ALL PRIVILEGES ON *.* WITH GRANT OPTION` through `unix_socket`, so no password
  is needed:
  `~/.local/opt/mariadb/usr/bin/mariadb --no-defaults --socket=$HOME/.local/state/duris-mariadb/mariadb.sock`.
  The MUD itself connects over TCP as `duris_prod`, which has full rights on its
  schemas only. The `root@localhost` and `duris@localhost` accounts from Plesk
  are unusable here. The service runs in a systemd user namespace, which shows
  every other local user, root included, as `nobody` to socket authentication.
- **Redis admin.** The `duris-admin` ACL user (`+@all ~* &*`) has its password in
  `~/.config/redis/admin.pass` (mode `0600`):
  `REDISCLI_AUTH="$(cat ~/.config/redis/admin.pass)" redis-cli -p 6381 --user duris-admin`.
  The MUD still uses its own restricted ACL users.
- **Everything else** belongs to the account and needs no root:
  - the checkout and `.env`
  - the user units, including restarts
  - the MariaDB and Redis configuration and data
  - the certbot directory and hooks
  - the backup policy and generations
  - core files, which are written to the MUD's working directory (`core.%e.%p`)
  - the user journal
  - `crontab`
- **Restarts.** Stop the MUD when the in-flight world snapshot is about
  200–235 seconds old: find the latest `starting bounded world recovery capture`
  line in `logs/log/sys`. It then stops in 2–3 seconds instead of risking the
  #621 SIGKILL. Restarting `duris-mariadb` or `duris-redis` also restarts the
  MUD, because the MUD unit `Requires=` both.

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

## Post-move review (2026-09-23, about 08:40 UTC)

Staging is healthy, and nothing depends on the Duris install on Plesk. That
install is stopped and disabled, so it won't start again after a reboot.
Deleting its data is a separate decision; see the list at the end.

### Checked

- **Services.** The MUD, MariaDB and Redis are active with no restarts. The
  backup and certbot timers are active, lingering is on and no user units have
  failed.
- **Logs.**
  - Redis is connected on 6381, and world snapshots publish about every 4
    minutes. There are no persistence alerts or errors.
  - The only noise is a charset warning from the `mysql` command-line client
    at each boot. The unpacked MariaDB client reads MySQL 8's
    `/usr/share/mysql/charsets`, which is cosmetic.
- **Logins.** The test account logged in over plain telnet and TLS. Two other
  characters also logged in; one entered at level 56, the Chaos rebuild level.
- **Data.** The last Plesk backup and the first backup on the new host share
  the runtime schema hash and have identical locker receipts.
- **Backups.** The minute-by-minute status checks return `ok`, and the policy
  creates a generation every hour.
- **TLS renewal.** `certbot renew --dry-run` succeeded from the new host.
- **DNS.**
  - `mud.duris.sbs` resolves to `178.156.165.10` at four public resolvers and
    at Cloudflare's authoritative server. No record points at `74.208.126.44`.
  - `duris.sbs`, `www` and `ws` are CNAMEs to the stopped Plesk tunnels and
    return Cloudflare 1033. That is expected, because staging has no website.
- **Configuration.** Mail is off (`MAIL_ENABLED` is unset, as it was on Plesk).
  No code reads the I3 or Ollama settings.
- **Production.**
  - The MUD has the same PID, health is `healthy`, the website returns 200 and
    the journal has no warnings.
  - Its average tick was 8.2 ms before the move and 9.5 ms after, against a
    250 ms budget. Ticks of 1–2 s occur both before and after.
  - Production's MySQL uses about one core continuously, which predates the
    move.
- **Repository and workstation.** No CI workflow or script references Plesk.

### Findings

1. **Stops can end in SIGKILL ([#621](https://github.com/Community-Duris/Duris/issues/621)).**
   This existing behavior also hit production at 05:40:41 today, ending in a
   SIGKILL at 05:42:11. After the players are saved, a shutdown cancels itself if a
   player save fails. It also cancels if a world snapshot can't finish within
   3 seconds (`src/net/comm.c:2598`). systemd then kills the process after 90
   seconds.
   - The Plesk stop hit the failed-save case. The new host wasn't tested,
     because a player was online.
   - The certbot deploy hook restarts the MUD at renewal, around October 29,
     and that restart goes through the same path.
2. **Custody-mismatch saves stay rejected
   ([#622](https://github.com/Community-Duris/Duris/issues/622)).** This
   affects `a6a2124c1`, the code staging runs now; production's older build is
   not affected. Overnight on Plesk:
   - 1,115 rejections across 12 characters
   - 489 failed terminal saves, mostly refused logouts
   - death recovery waited up to 6.4 hours
   - two characters were still being rejected at the stop, and their progress
     since 06:26:50 and 01:15:35 was lost

   The fixes (`e998ffd5e`, `fc59cb570`, `c555ecca1`, `72238a8f5`) exist only
   on `codex/master-stable`, with no PRs.
3. **World snapshots stop completing after long uptime
   ([#623](https://github.com/Community-Duris/Duris/issues/623)).** On Plesk
   the last good snapshot was at 03:05:26. The next 73 attempts all failed:
   - 14 hit the 300-second age limit
   - 52 hit the 64 MiB size limit
   - 7 failed to publish

   Snapshots on the new host already take about 250 seconds. A restart clears
   the failures, and they affect only world-state crash recovery, not player
   data.
4. **One saved item tree was held back at boot.** It is kept in the database
   but not placed in the world. This is most likely from the SIGKILL on Plesk.
5. **Maintenance scheduler state started fresh**, because `bin/` wasn't
   copied. The recurring jobs restart their passes, which is harmless.

### Only on Plesk

These are lost if the Plesk `duris` account is deleted:

- `~/.duris-backups` (2.9 GB): backup history and pre-upgrade dumps
- `~/private` (0.9 GB): incident working sets
- `~/.local/state/durisweb-backups` (1.2 GB)
- `~/durisweb` (0.6 GB)
- `~/duris-bag-recovery-20260920`, `~/duris-deploy-20260918T115716Z` and
  `~/.local/state/duris-migrations`

Leftover processes from an earlier session are also still running there:
orphaned `tail` and `ugrep` processes, a stale `pgrep` loop and a VS Code
server.
