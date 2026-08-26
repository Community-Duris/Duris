# Building

Basic setup and first-boot instructions are in the root
[README.md](../README.md). Runtime environment variables are documented in
[CONFIGURATION.md](CONFIGURATION.md). This document covers the build system
itself.

## Standard build

```bash
make                 # server, area editor, and area-generation tools
make -C src          # server only; produces src/dms_new
cp src/dms_new dms   # runtime binary name expected by scripts/
```

The root `Makefile` is the maintained full-project entry point. `make clean`
removes compiled server, editor, and area-tool artifacts but preserves generated
world data. Do not commit binaries or `obj/` output (gitignored).

## Compile flags

From the default build line:

| Flag | Meaning |
|------|---------|
| `-std=c++20` | All `.c` files are compiled as C++20 with g++. |
| `-DTEST_MUD` | Development build: selects the `duris_dev` database credentials in `src/sql.h` and enables test commands. |
| `-D__NO_TESTS__` | Excludes built-in test hooks. |
| `-D__NO_MYSQL__` | Optional; removes MySQL support (stubs live in `sql.c`). Not recommended — help and persistence depend on it. |

Link libraries: `mysqlclient`, `gnutls`, `ssl`, `crypto`, `cjson`, `hiredis`,
`bsd`, `xml2`, `z`, `crypt`, `pthread`.

Chaos gameplay mode is selected at runtime with `CHAOS_MUD=TRUE` or
`CHAOS_MUD=FALSE` in `.env`. The default example disables it. The value must
use uppercase `TRUE` or `FALSE`; numeric values are not supported. See
[CONFIGURATION.md](CONFIGURATION.md) for this and the other runtime switches.

Dependency install lines are in [README.md](../README.md#quick-start). The
`packaging/` directory contains the equivs manifest for the complete developer
toolset, including Git, Python, area-generation utilities, the
compiler-provided sanitizer runtimes, Valgrind, GDB, and clang-format. It
accepts an existing MariaDB or MySQL toolchain and otherwise uses the
distribution's default MySQL-compatible packages.

## Sanitizer build

`scripts/build-san.sh` builds an ASan/UBSan instrumented binary for hunting
memory bugs. The sanitizer runtimes arrive with the compiler declared by the
developer dependency manifest; no separate sanitizer package is needed. Use
this build for debugging sessions, not for production.

## Area file generation

The server boots from combined area files (`areas/world.wld`, `world.mob`,
`world.obj`, `world.zon`, `world.qst`, `world.trg`, plus lookup tables). These
are generated, not hand-edited:

1. Build the compilers: `make build-area-tools` (produces `make_mob`,
   `make_obj`, `make_qst`, `make_shp`, `make_wld`, `make_zon`).
2. Generate: `make world` (runs `areas/m_slow`, including lookup generation).

The six independent compiler builds inherit GNU Make's jobserver, so
`make -j"$(nproc)"` can build them in parallel. Generation scripts stop on the
first failed command instead of leaving a partially refreshed world behind.
The root target records an ignored `areas/.world.stamp` and skips regeneration
when every required output exists and all area sources and tools are unchanged.

Per-area source directories (`areas/wld/`, `areas/mob/`, ...) hold editable
area data; the combined outputs land in `areas/world.*`.

`scripts/cycle_mud.sh` performs both steps automatically when the helper
binaries are missing, so first boot after a fresh clone works without manual
intervention.

## Verifying a build

- Recompile check: `make -C src` must complete without errors or warnings in
  touched files.
- Full build and regression gate: `make test-all`.
- Smoke test on a development port (uses `duris_dev`, never production):

  ```bash
  ./dms 4000 &
  sleep 5 && telnet localhost 4000   # confirm greeting, then shut down
  ```

- If you changed persistence-related code, run the relevant focused test;
  see [TESTING.md](TESTING.md).
