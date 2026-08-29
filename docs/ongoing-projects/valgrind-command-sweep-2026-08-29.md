# Valgrind minimal-boot command sweep — 2026-08-29

Working notes from a full Memcheck session against a minimal-world boot, in
which the staff character executed the entire in-game command list and every
privileged (`wizhelp`) command.

## Session setup

| Item | Value |
| --- | --- |
| Revision | `96a0f3561` (clean worktree) |
| Binary | `bin/server/dms` (copied from the current `bin/server/dms_new` build) |
| Detector | Valgrind 3.22.0, Memcheck |
| World | `--minimal` (tracked `areas_mini` dataset) |
| Port | 4000 (development; the wrapper and this session both refuse 7777) |
| Environment | `.env` `ENVIRONMENT=local`, database `duris_dev`, Redis `duris:local:dev` |
| Character | account/character from `.env` (`Zusuk`, level 62, immortal) |

The supervised local service (`duris-mud.service`, port 7777) was stopped
gracefully before the session so that two servers would not share `duris_dev`
or the staff character. It was left stopped afterwards, as requested.

### Detector invocation

`scripts/valgrind_mud.sh` was not used directly because it has no way to pass
`--minimal` through to the server (everything after `--` goes to Valgrind, not
to `dms`). The session used the wrapper's option set, widened to "record
everything", plus the server's minimal-world flag:

```
valgrind --tool=memcheck \
  --log-file=logs/valgrind/memcheck-minimal-<stamp>.log \
  --suppressions=scripts/valgrind.supp \
  --num-callers=50 --error-limit=no --time-stamp=yes --trace-children=no \
  --leak-check=full --show-leak-kinds=all \
  --errors-for-leak-kinds=definite,indirect \
  --track-origins=yes --track-fds=all \
  --keep-stacktraces=alloc-and-free --show-reachable=yes --verbose \
  bin/server/dms --minimal 4000
```

`--show-leak-kinds=all` and `--show-reachable=yes` widen the wrapper's default
(`definite,indirect`) so the report also carries still-reachable and possibly
lost blocks; `--track-fds=all` records every descriptor, not just leaked ones.

### Reports

Both Memcheck reports are kept next to this document:

| Report | Run | Size |
| --- | --- | --- |
| [`memcheck-minimal-run1-crash.log`](memcheck-minimal-run1-crash.log) | run 1, ends at the `deathsdoor` abort | 534 KB |
| [`memcheck-minimal-20260829-203738.log`](memcheck-minimal-20260829-203738.log) | run 2, boot to clean shutdown | 159 KB |

The core dump Valgrind wrote alongside run 1 was 211 MB and has been deleted;
the abort reproduces on demand (see [Finding 1](#finding-1--deathsdoor-aborts-the-whole-server-critical)).

## Workload

1. Boot the minimal world under Memcheck.
2. Log in through the account menu to the staff character.
3. Page the full `commands` list (11 pages) and the full `wizhelp` list
   (2 pages) out of the running server, and drive from those lists rather than
   from a hand-written inventory.
4. Argument-free pass over every command that is not destructive.
5. Argument-bearing pass over the staff/introspection commands, an object and
   mob create/inspect/destroy cycle, and position/visibility state cycles.
6. Three connect/play/disconnect cycles.
7. Clean in-game shutdown so the end-of-process leak report is produced.

## Result at a glance

| | Run 1 | Run 2 |
| --- | --- | --- |
| Log | [`memcheck-minimal-run1-crash.log`](memcheck-minimal-run1-crash.log) | [`memcheck-minimal-20260829-203738.log`](memcheck-minimal-20260829-203738.log) |
| Ended by | **server abort (SIGABRT)** on the `deathsdoor` command | clean in-game `shutdown ok` |
| Commands driven | 401 of 427 before the abort | 426 argument-free + 111 with arguments + 4 login cycles |
| Memcheck errors | 6 (all leak records) | 20 (all leak records) |
| Invalid read/write, uninitialised value, invalid free | none | none |
| Definitely lost | 498 bytes / 7 blocks | 256,287 bytes / 2,690 blocks |
| Indirectly lost | 0 | 8,711 bytes / 38 blocks |
| Possibly lost | 2,938,324 bytes / 5,947 blocks | 2,545,904 bytes / 3,138 blocks |
| Heap at exit | — | 3,023,264 bytes in 6,033 blocks (193,815 allocs / 187,782 frees) |
| Descriptors at exit | 19 (3 std) | 16 (3 std) |

The two runs are not directly comparable: run 1 died mid-workload, so most
boot-time allocations were still reachable through live globals, while run 2's
clean teardown drops those roots and Memcheck then reports the same one-time
boot data as definitely lost. **No memory-safety error (invalid access,
uninitialised value, bad free) was reported in either run.** Everything
Memcheck flagged is a leak.

---

## Finding 1 — `deathsdoor` aborts the whole server (critical)

Typing `deathsdoor` with no argument killed the process. It is a plain,
non-privileged entry in the `commands` list, so any character at or above
`MIN_LEVEL_FOR_ATTRIBUTES` can trigger it.

```
Process terminating with default action of signal 6 (SIGABRT): dumping core
   ...
   by __fortify_fail (fortify_fail.c:24)
   by __chk_fail (chk_fail.c:28)
   by __snprintf_chk (snprintf_chk.c:29)
   by snprintf (stdio2.h:54)
   by do_deaths_door(char_data*, char*, int) (specs.gellz.c:1042)
   by command_interpreter(char_data*, char*) (interp.c:2001)
```

`src/specs.gellz.c:1042` is the line that closes the "you still need N Str,
N Dex, ..." list:

```c
snprintf(buf + strlen(buf) - 2, MAX_STRING_LENGTH, "&+y.\n");
```

The destination is advanced into `buf`, but the size argument is still the full
`MAX_STRING_LENGTH`. With `_FORTIFY_SOURCE` active, glibc knows the object at
`buf + strlen(buf) - 2` is smaller than the size claimed and aborts the process
rather than writing. The size should be the remaining space,
`MAX_STRING_LENGTH - (strlen(buf) - 2)`. Every other write in this function
already uses `checked_snprintf` with a correctly computed remainder; this last
one is the exception.

Two secondary problems in the same block:

- if a character somehow reaches the branch with all eight base stats already
  at 100, `strlen(buf) - 2` backs into the header text instead of trimming a
  trailing `", "`;
- `CMD_DEATHS_DOOR` (832) is declared in `src/interp.h` but never registered in
  `interp.c`'s command table, unlike its neighbours `CMD_BEEP` (831) and
  `CMD_OFFLINEMSG` (833). The command still dispatches — so it is reached
  through the achievement/spec path — but it has no level or position guard of
  its own.

Reproduce: log in any character of sufficient level that lacks the
`ACH_DEATHSDOOR` affect and type `deathsdoor`. The full abort, with the
surrounding descriptor and leak state, is in
[`memcheck-minimal-run1-crash.log`](memcheck-minimal-run1-crash.log); search it
for `Process terminating`. Valgrind also dumped a 211 MB core beside that log,
which has since been deleted as too large to keep.

---

## Finding 2 — a destroyed item can leave a character permanently unloadable (critical)

Not a memory bug, and the most serious thing this session turned up, so it has
its own write-up: **[Orphan `player_items` row locks a character out of the
game](orphan-player-item-lockout-2026-08-29.md)**.

In short: the argument pass loaded two objects with the staff `load` command,
picked them up, destroyed one with `junk` and one with `purge`, then saved and
quit. Every later attempt to enter the game with that character failed with
"Sorry, I couldn't load that character!" — deterministically, on every retry.
Item destruction had removed the `item_current_owner` row but left the
`player_items` row behind, and a payload row with no ownership row makes the
entire items component of the load fail rather than being skipped like other
suspect rows. The character was recovered by deleting the orphan row from
`duris_dev`; three connect/play/disconnect cycles then succeeded. Whether
ordinary mortal item-destruction paths reach the same state is untested and is
the first follow-up.

---

## Finding 3 — repeatable leaks with Duris frames

These are the leak records whose stack is Duris code and whose allocation is
tied to a repeatable action rather than to one-time boot data.

**`generate_modif()` leaks its own scratch copy — `src/utility.c:5236-5254`**

```c
buf = str_dup(modifier_descs[number(0, num_modifiers - 1)]);
...
return str_dup(buf);          // `buf` is never freed
```

Every call leaks the intermediate duplicate.

**`generate_desc()` drops every generated string — `src/utility.c:5295-5314`**

`generate_shape()`, `generate_appear()` and `generate_modif()` each return a
`str_dup`'d buffer, and `generate_desc()` passes those returns straight into
`snprintf` and never frees them. Memcheck caught all three in run 2:

```
54 bytes ... generate_shape  (utility.c:5228) <- generate_desc (utility.c:5297) <- do_testdesc
56 bytes ... generate_appear (utility.c:5233) <- generate_desc (utility.c:5298) <- do_testdesc
62 bytes ... generate_modif  (utility.c:5243/5254) <- generate_desc <- do_testdesc
```

The reachable trigger is the `ztestdesc` staff command (`do_testdesc`), which
calls `generate_desc()` for *every* descriptor in the game, so one invocation
leaks proportionally to the number of connected players. `generate_desc()` also
overwrites `ch->player.short_descr` with a fresh `str_dup` without freeing the
previous value.

**`apply_string()` overwrites player strings without freeing — `src/player_load_materialize.c:121-135`**

```c
char *copy = str_dup(entry.value.c_str());
...
case player_status_string_field::short_description:
        ch->player.short_descr = copy;      // previous pointer is dropped
```

Seen as `77 bytes in 1 blocks definitely lost` under `load_char_into_game`. This
runs on every character load, so it scales with logins rather than with uptime.

**`do_build()` — `src/buildings.c:213`** leaks 112 bytes per invocation.

**Boot-time one-shots** (each allocated once and never freed; visible only in the
clean-shutdown run): `boot_social_messages()` via `fread_action()`
(~106 KB across `actcomm.c:1559/1572/1576`), `boot_world()` string and exit data
(`db.c:1170/1177/1268-1270`), `setup_dir()` and `boot_the_shops()` via
`fread_string()`. These are world data that lives for the process lifetime;
they are only worth touching if a zero-leak shutdown is wanted.

**Descriptors at exit**: 16 open, of which the interesting ones are
`areas_mini/mini.mob` and `areas_mini/mini.obj` — the boot reader never closes
the world files. The rest are the listeners (4000/4001/4050), the accepted
client socket, five MySQL connections and the Redis worker sockets.

---

## Finding 4 — `recline` silently disabled a quarter of the command list

Not a bug in the server, but worth recording because it invalidated part of the
first sweep. After `recline`, 96 subsequent commands answered "Sorry, you can't
do that while laying around." and were never actually exercised. The second run
reads the position out of the prompt after every command and stands the
character back up, which is the only way an automated sweep covers the
position-gated commands. The same applies to `kneel` (position `kneeling`) and
to `sit`/`rest`/`sleep`.

---

## Finding 5 — scheduled shutdown is cancelled if the issuer disconnects

`shutdown ok <reason>` schedules the shutdown but `timedShutdown()` cancels it
when it cannot find the issuing character in the game
(`src/actwiz.c:4536-4547`). Issuing the command and then closing the connection
leaves the server running with no shutdown pending. This is documented in the
command's own help text, and it is intentional, but it makes "issue shutdown,
then disconnect" an unreliable way to stop the server from a script — the
session had to reconnect and hold the link open until the process exited.

---

## Command-list observations

- 502 distinct commands are visible to a level-62 immortal through `commands`
  (11 pages); `wizhelp` lists 116 privileged commands across levels 57-62.
- 426 were driven argument-free in run 2. 75 were deliberately skipped as
  destructive, irreversible or session-ending (`shutdown`, `purge`, `sql`,
  `redis`, `switch`, `advance`, `pwipe`-family, `zreset`, `newchar`, ...), plus
  `deathsdoor` after run 1 proved it aborts the server. The skip list is
  recorded at the top of the run notes in the session scratchpad.
- 17 commands answer "Sorry, but that command has yet to be implemented":
  `lflags`, `trap`, `analyze`, `shadow`, `speak`, `lwitness`, `house`,
  `poofinsound`, `poofoutsound`, `condition`, `sack`, `reloadhelp`, `defend`,
  `hone`, `heroescall`, `tether`, `add`.
- 3 answer only "Pardon?": `return`, `introduce`, `squidrage`.
- Slowest commands under Memcheck (native cost is roughly 20-50x lower):
  `spells` 33.6 s / 54 KB, `skills` 25.0 s / 27 KB, `news` 23.1 s / 22 KB,
  `mlist` 10.5 s, `rlist` 8.0 s. All are large paged tables; nothing else took
  more than 5 seconds.
- `stat <vnum>` is not a supported form — `stat` requires
  `stat char|mob #|'name'`, so `stat 7` and `purge 7` both failed on a mob that
  was present in the room. Targeting by keyword (`purge dracolich`) worked.

## What was not covered

- The 75 skipped commands, including everything under `shutdown`, `pwipe`,
  `sql`, `redis`, `switch`, `advance`, `ban`, `freeze` and the world-reset
  family. Exercising those needs a throwaway database, not `duris_dev`.
- Combat: nothing was killed, so damage, death, corpse and looting paths are
  untested here.
- Copyover, which is deliberately excluded (`--trace-children=no`) — Memcheck
  does not follow the `exec`.
- Helgrind and DRD. The Redis presence worker and the save/SQL worker threads
  are the obvious candidates and were not checked in this session.
- Whether ordinary mortal item-destruction paths reproduce Finding 2.

## Session bookkeeping

- The supervised `duris-mud.service` was stopped before the session and left
  stopped, as requested. Restart with `systemctl --user start duris-mud.service`
  or `./scripts/start_mud.sh`.
- The staff character was left standing, visible, saved and loadable, holding a
  corpse object in the minimal world's start room. The two wizard-loaded
  entities created during the run (a bronze dracolich and a dagger) were purged.
- One row was deleted from `duris_dev.player_items` to undo Finding 2; the exact
  statement is in [the orphan-row write-up](orphan-player-item-lockout-2026-08-29.md#repair).
- Both Memcheck logs were moved out of the git-ignored `logs/valgrind/` and now
  sit beside this document in `docs/ongoing-projects/`; they are untracked, so
  decide deliberately whether to commit them. The project's memory-checking
  standard says not to commit tool logs, and `docs/` is a tracked tree. The run-1
  core was deleted.
- `scripts/valgrind_mud.sh` still cannot start a minimal-world boot: everything
  after `--` is passed to Valgrind, not to `dms`. A `--minimal` pass-through
  would make this session reproducible with the checked-in wrapper alone.
