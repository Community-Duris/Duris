# Valgrind

Valgrind is a required development dependency for Duris. It is the tool of
record for memory errors (use-after-free, uninitialised reads, leaks, fd
leaks) and for data races in the threaded save/SQL paths.

```bash
sudo apt-get install valgrind        # Debian/Ubuntu
```

It is also listed in `packaging/duris-build-deps.equivs`, so
`apt install ./bin/packages/duris-build-deps_1.0_all.deb` pulls it in.

## Running

```bash
./scripts/valgrind_mud.sh                  # memcheck, port 4000
./scripts/valgrind_mud.sh --build          # rebuild + refresh bin/server/dms
./scripts/valgrind_mud.sh --tool=helgrind  # data races
make -C src valgrind                       # build, then the same script
```

Reports land in `logs/valgrind/<tool>-<timestamp>.log` (gitignored). The run is
in the foreground; an in-game `shutdown`, `Ctrl-C`, or `kill` ends it. The leak
summary is written when the process exits, so never `kill -9` a run you care
about.

| Option | Meaning |
| --- | --- |
| `--tool=TOOL` | `memcheck` (default), `helgrind`, `drd`, `massif`, `callgrind` |
| `--port N` | Bind port, default 4000. **7777 is refused**: it is `DFLT_PORT`, and `src/sql/sql.c` only redirects to `duris_dev` on other ports. |
| `--build` | `make -C src`, then copy `bin/server/dms_new` to `bin/server/dms` |
| `--gen-suppressions` | Print paste-ready suppression blocks for every error |
| `--trace-children` | Follow `exec()` across a copyover (off by default) |
| `-- ...` | Everything after `--` goes to valgrind verbatim |

The standard build already compiles with `-g` and no `-O`, so no special build
is needed for readable stacks.

## What to expect

- The game boots roughly 20-50x slower under memcheck: measured at ~297s to
  "Entering game loop" on a dev box, against a few seconds native. `helgrind`
  is slower still.
- The MUD's own watchdog may declare the game hung under this slowdown; a
  reboot exit code from a Valgrind run is usually the watchdog, not a crash.
- `--trace-children` is off, so a copyover ends the Valgrind session at the
  `exec()`. Turn it on only when copyover itself is under investigation.
- Reachable-at-exit blocks from libraries are expected; the script's summary
  reports `definitely lost` and `indirectly lost` only.
- A boot-and-shutdown with no players has a large standing baseline (~15 MB
  "definitely lost", mostly one-shot `boot_world()` world data that is never
  freed by design). Judge a change by the delta against a baseline run of the
  same shape, not by the absolute number.

## Suppressions

`scripts/valgrind.supp` is applied automatically. It holds third-party noise
only: glibc's loader, GnuTLS/OpenSSL, the MySQL client, libxml2's global
parser tables, and zlib's deliberate wide reads.

Never suppress a frame in Duris code — fix the bug. To add a legitimate
third-party suppression, run with `--gen-suppressions`, copy the generated
block out of the report, give it a descriptive name, and append it with a
comment saying why the library cannot free that allocation.

## Related

- [MEMORY_CHECKING.md](MEMORY_CHECKING.md) — the project-wide standard this
  guide sits under: which detector to reach for, when a dynamic check is
  required, and how to report results.
- `scripts/build-san.sh` — ASan/UBSan build. Much faster than Valgrind and
  better at stack/global overflows; Valgrind needs no rebuild and catches
  uninitialised reads that ASan misses. Use whichever fits, not both at once
  (the sanitizer runtime and Valgrind conflict).
- `scripts/gdbdms` — plain GDB session on the same binary, defaulting to
  development port 4000 and refusing production port 7777. GDB is declared
  directly in the developer dependency manifest rather than relying on
  Valgrind's package recommendation.
