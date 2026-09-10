# Slow ticks attributed to the command phase — 2026-09-10

## Result

**Confirmed from the production status log.** All four reported stalls were a
synchronous bcrypt password hash or verify in the account-creation nanny on a
connection that was not in the game (`player_id=-1`). The player casting chill
touch had nothing to do with it. The 2026-09-10 login fix (`00542423f`) made
only the login password check asynchronous; the new-account password states
still hash on the game thread.

| Tick  | Local time | State | Meaning                          | Operation  |
| ----: | ---------- | ----: | -------------------------------- | ---------: |
| 67848 | 18:11:16   | 68    | `CON_GET_NEW_ACCT_PASSWD` (hash) | 241.581 ms |
| 67853 | 18:11:17   | 69    | `CON_VERIFY_NEW_ACCT_PASSWD` (verify) | 243.239 ms |
| 67865 | 18:11:20   | 68    | `CON_GET_NEW_ACCT_PASSWD` (hash) | 257.064 ms |
| 67872 | 18:11:22   | 69    | `CON_VERIFY_NEW_ACCT_PASSWD` (verify) | 243.801 ms |

Each `COMMAND SWEEP SLOW` record shows the nanny operation as the entire sweep
(`unattributed_sweep_us` of 15–21 µs; five other descriptors totalled under
15 µs). The 68/69/68/69 sequence is someone entering a new account password,
confirming it, then entering and confirming again. The same pair occurred
earlier in the boot at ticks 35111 and 35117 (15:54:42–43, 237 ms and 241 ms),
which were the only other slow command operations in the boot's 7,155 status
records. Production host: `duris-prod` (`ubuntu-16gb-ash-1`), checkout
`/home/duris/duris` at commit `e32f3ceb3`, process 904174. Read-only access;
player names redacted here.

Note: `ssh duris` (plesk.luminarimud.com) is a different install still running
the Sep 8 build and has no attribution records; do not look for these there.

Casting was additionally ruled out by direct measurement on a local server
(section below), not only inferred from the code. Every reported overrun is in the command sweep (`commands_us`), while
the phase that actually runs spell effects (`ne_events_us`) stayed under 8 ms.
The four stalls have a near-identical fixed cost of 241–257 ms, which is the
signature of one synchronous bcrypt hash or verify at work factor 12, not of a
spammed spell. The 2026-09-10 login fix (`00542423f`) moved only the account
login check off the game loop; several other bcrypt calls still run
synchronously in the command sweep, and one of them is an ordinary playing
command. The grep used to confirm it is kept below for future incidents.

## Evidence from the report

Boot `1789046861843372-904174` started at 13:27:41 UTC. Ticks 67848–67872 are
about 16,962 s later, so the stalls happened at roughly 18:10 UTC across a
six-second window.

| Tick  | Loop total | commands_us | ne_events_us | combat_us | activities_us |
| ----: | ---------: | ----------: | -----------: | --------: | ------------: |
| 67848 | 251.5 ms   | 241.6 ms    | 7.4 ms       | 1.9 ms    | 0             |
| 67853 | 250.3 ms   | 243.3 ms    | 5.6 ms       | 0         | 1.2 ms        |
| 67865 | 265.4 ms   | 257.1 ms    | 6.8 ms       | 0         | 1.2 ms        |
| 67872 | 251.0 ms   | 243.8 ms    | 6.6 ms       | 0         | 0             |

Observations:

- 96–97% of each overrun is inside the command sweep. Events, combat, and
  activities are normal.
- The per-stall cost is flat (spread of 16 ms). Work that scales with a spam
  rate or a world scan does not produce four identical durations; a fixed-cost
  CPU operation does.
- The stalls are on separate ticks, 5–12 pulses apart, which matches a human
  typing four lines with a small delay between them.

## Why casting is ruled out

- `do_cast` in [sparser.c](../../src/net/sparser.c) only validates, prints
  the chant message, schedules `event_spellcast`, and sets the wait gate. The
  spell function itself runs later from the event wheel, so its cost lands in
  `ne_events_us`, which was 5.6–7.4 ms here.
- `spell_chill_touch` in [magic.c](../../src/magic/magic.c) is a saving throw,
  one `spell_damage` call, and up to three `affect_to_char` calls. No world
  scans, no persistence, no logging.
- `parse_spell` only walks the global character list for `TAR_CHAR_WORLD`
  spells. Chill touch is `TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_AGGRO`.
- The live `debug profile` capture from 11:04 UTC today (recorded in
  `docs/ongoing-projects/2026-09-10-live-casting-latency-findings.md` as of
  commit `e32f3ceb3`, removed by the consolidation in `206c95dea`) measured
  `event_spellcast` at 4,819 calls in 0.037 s, about 8 µs per callback,
  including NPC casts.
- A caster who keeps typing `cast` while already chanting is served by
  `get_casting_cmd_from_q`, a linear scan of that descriptor's input queue for
  `abort`/`petition`/`return`. That scan is microseconds per line and would
  need an absurd queue depth to approach 240 ms.

If a player was visibly spamming chill touch at the same time, that is a
coincidence of timing, not the mechanism.

## Measured: starting and completing a chill touch cast is cheap

Trial on the local server (boot `1789045777221348-4049211`, built from
today's tree) at 18:29:43–18:30:51 UTC, using the configured test character:
login, `load m 46`, then forty `cast 'chill touch' <mob>` lines about 0.8 s
apart, while the mob was meleeing the caster. All forty chants started and all
forty completed (`You complete your spell...`).

| Window (300 pulses) | commands max | commands avg | ne_events max | total loop max |
| --- | ---: | ---: | ---: | ---: |
| Covers login, mob load, casts 1–~20 | 12.0 ms | 0.28 ms | 39.2 ms | 104.3 ms |
| Covers casts ~20–40, purge, quit | 10.1 ms | 0.20 ms | 44.7 ms | 109.2 ms |
| Idle window after the trial | 0.001 ms | 0 | 29.7 ms | 93.2 ms |

No `COMMAND OP SLOW` record (50 ms threshold) was produced at any point. The
command-phase maximum across the whole trial was 12 ms, and that window also
included login and a mob load; the worst individual sample in the top-ten list
was never a command entry. Even allowing a 2x slower production CPU, the
entire cast start is two orders of magnitude below the 241–257 ms observed.

Caveats: the test character is staff, so quick-chant notching and the memorized
slot check were skipped. Both are constant-time table lookups. The per-command
`special()` hook runs every room, mob, and object procedure in the caster's
room on each command, so a room with an expensive procedure would add cost;
that is room-specific, not spell-specific, and would show as
`operation=cast` in the production status log with a variable duration.

## The fixed-cost candidate: synchronous bcrypt on the game thread

`bcrypt_hash_password` and `bcrypt_verify_password` in
[password_hash.c](../../src/account/password_hash.c) use `crypt_gensalt_rn("$2b$", 12, ...)`.
Work factor 12 is 4,096 Blowfish key-schedule rounds per operation.

| Measurement                                   | Duration      |
| --------------------------------------------- | ------------: |
| Local hash, cost 12 (Core Ultra 9 285H)       | 146–151 ms    |
| Local verify, cost 12 (same box)              | 146–148 ms    |
| Production login verify measured 2026-09-10   | 258.3 ms      |
| Reported command-phase stalls                 | 241–257 ms    |

The production number comes from the earlier live review of this same day,
where the only `MUD TICK TOOK TOO LONG` in that boot was a 258 ms nanny
operation in state 61 (`CON_GET_ACCT_PASSWD`). That path is now asynchronous.
The production CPU is slower than the local box, and the earlier measured cost
on it lines up with today's stalls to within a few milliseconds.

### Sites still running bcrypt synchronously inside the command sweep

All of these are dispatched from the `commands_us` region in
[comm.c](../../src/net/comm.c), either through `nanny()` or through
`command_interpreter()`.

| Trigger                                                     | Code path                                                                 | bcrypt ops per line |
| ----------------------------------------------------------- | ------------------------------------------------------------------------- | ------------------: |
| `open <chest> <password>` at a storage locker, non-owner    | `locker_opencmd` → `sql_verify_chest_password` ([storage_lockers.c:4028](../../src/item/storage_lockers.c)) | 1 verify (+1 hash if legacy SHA-256 upgrade) |
| `eq chest create <name> <password>`                         | `locker_chestcmd` → `sql_create_private_chest` ([sql_player.c:6976](../../src/sql/sql_player.c)) | 1 hash |
| `eq chest password <chest> <password>`                      | `locker_chestcmd` → `sql_set_chest_password` ([sql_player.c:7119](../../src/sql/sql_player.c)) | 1 hash |
| New account: enter password (`CON_GET_NEW_ACCT_PASSWD`)     | `get_new_account_password` ([account.c:974](../../src/account/account.c)) | 1 hash |
| New account: confirm password (`CON_VERIFY_NEW_ACCT_PASSWD`)| `verify_new_account_password` ([account.c:1001](../../src/account/account.c)) | 1 verify |
| Account menu: change password (`CON_ACCT_CHANGE_PASSWD`)    | `get_new_account_password`, then the confirm state above                  | 1 hash + 1 verify |
| Account menu: delete account (`CON_ACCT_DELETE_ACCT`)       | `delete_account` → `account_password_matches` ([account.c:2682](../../src/account/account.c)) | 1 verify |
| Password reset: new password (`CON_ACCT_RESET_NEWPW`)       | `account_recovery_new_password` ([account_recovery_nanny.c:241](../../src/account/account_recovery_nanny.c)) | 1 hash |
| Password reset: confirm (`CON_ACCT_RESET_NEWPW2`)           | `account_recovery_verify_new_password` ([account_recovery_nanny.c:292](../../src/account/account_recovery_nanny.c)) | 1 verify |

Any of the following produces exactly the reported shape of four ~250 ms
stalls in six seconds:

- a player at a locker retrying a wrong chest password four times;
- a new account creation where the confirmation mismatched once
  (hash, verify, hash, verify);
- a menu password change plus a mistyped confirmation.

The player observed casting chill touch was nowhere near a locker, so the
chest path is not the explanation for this incident. The command sweep covers
**every** descriptor in one pass, so the stall need not come from the player
who was being watched at all: another connection sitting in account creation,
the account menu, or a password reset produces exactly this shape while a
caster in the world is visible doing something unrelated. The caster is the
coincidence; the sweep total is what the status line reports.

### Sites that are not candidates for this report

- `ws_cmd_register`, `ws_cmd_change_password`, `ws_cmd_complete_reset` in
  [ws_handlers.c](../../src/net/ws_handlers.c) also hash synchronously, but
  WebSocket JSON commands are executed from `process_input()` during the
  connections phase. Their cost would show in `connections_us`, which was
  67–120 µs here. They are still worth moving off the loop, just not for this
  incident.
- Synchronous MariaDB queries in playing commands (locker listing, wizard
  commands, wiki help) are variable-cost and would not produce four
  near-identical durations, but see the confirmation step below.

## How to confirm on production

The command latency tracker already attributes every slow operation. Its
threshold is 50 ms, so all four stalls will have produced `COMMAND OP SLOW`
records naming the kind, connection state, player, and command. Read-only:

```sh
cd /path/to/duris
grep 'COMMAND OP SLOW' logs/log/status | grep 'boot=1789046861843372-904174' \
  | grep -E 'tick=678(4[0-9]|[5-7][0-9])'
grep 'COMMAND SWEEP SLOW' logs/log/status | grep 'boot=1789046861843372-904174' \
  | grep -E 'tick=678(4[0-9]|[5-7][0-9])'
```

Expected outcomes:

- `kind=nanny state=<n>` confirms an account-menu, account-creation, or
  password-reset state on some other connection. Map `<n>` through the
  `CON_*` enum in [structs.h](../../src/core/structs.h). This is the expected
  result given the caster was not at a locker.
- `kind=playing ... operation=open` with one player on all four lines
  confirms the chest-password path.
- `kind=playing operation=cast` would falsify this analysis; in that case
  capture the `COMMAND SWEEP KIND` lines and the `unattributed_sweep_us`
  value from the same ticks before going further, because `do_cast` has no
  fixed-cost work that could account for 240 ms.

If the status log has rotated (`logs/old-logs/<timestamp>/status`), filter by
the boot ID rather than by file.

## Recommended fix

Tracked in [issue #208](https://github.com/Community-Duris/Duris/issues/208).

Extend the worker pattern from `00542423f` rather than adding a second one.
`password_login_submit` / `password_login_poll` in
[password_hash.h](../../src/account/password_hash.h) already run
verify-and-optional-upgrade on a dedicated thread and hand a handle back to the
game thread. The chest paths need the same shape, with two additions:

1. A hash-only job type for `create`, `set password`, new-account, and reset
   flows. The job already stores the password and produces `new_hash`; the
   verify step is what needs to become optional.
2. A descriptor-independent completion for the locker commands. The login
   flow parks the descriptor in the nanny until the poll completes. For
   `open <chest> <password>` the player is `CON_PLAYING`, so the pending job
   should gate that character's command queue the same way item-movement
   transactions do (`item_movement_transaction_player_busy` in
   [comm.c](../../src/net/comm.c)), and the result must re-check that the
   character is still alive and still in the same locker room before opening
   the chest.

Keep work factor 12. Lowering it to hide loop latency would weaken every
account and chest password on the server; the fix is to take the work off the
loop.

Add a regression test alongside `test_private_chest_password_hardening.py`
and `test_login_crash_regressions.py` that asserts a wrong chest password does
not block the loop, using the existing `COMMAND OP SLOW` instrumentation the
way `test_command_latency_runtime.py` does.

## Caveats

- The account-creation attribution comes from the production tracker's
  per-operation timer, which brackets the whole nanny call for that state.
  Those two functions do nothing else expensive besides bcrypt, and the
  durations match the earlier 258 ms production bcrypt measurement, but the
  hash itself was not timed in isolation on production.
- Local bcrypt timing is on a faster CPU than production; the relevant
  production figure is the 258 ms login verify measured earlier today.
- The earlier finding that casting continuation timers do not subtract
  scheduler lateness (multi-stage casts finishing late) is a separate,
  player-visible casting problem and is unaffected by this report.
