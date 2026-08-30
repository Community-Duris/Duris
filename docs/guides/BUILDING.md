# Building

Basic setup and first-boot instructions are in the root
[README.md](../../README.md). Runtime environment variables are documented in
[CONFIGURATION.md](../operations/CONFIGURATION.md). This document covers the build system
itself.

## Standard build

```bash
make                                    # server, editor, and area tools
make -C src                             # produces bin/server/dms_new
cp bin/server/dms_new bin/server/dms    # manually promote for a direct run
```

The server build has an explicit compile-time persistence dependency boundary:

```bash
make -C src PERSISTENCE_BACKEND=mariadb
make -C src PERSISTENCE_BACKEND=flatfile
```

`mariadb` is the default and includes and links the MySQL-compatible client. The
`flatfile` selection defines `__NO_MYSQL__` and does not add the MySQL include path or
client library. Backend-specific objects are kept under
`bin/objects/server/<backend>/`; the maintained `bin/server/dms_new` path is relinked
when the selected backend changes, so the two commands can be run sequentially without
cleaning or reusing incompatible objects. Each binary accepts only its matching primary
runtime mode. The legacy
`mariadb-primary-flatfile-fallback` token remains recognized but fails closed because
mixed per-operation authority transfer is not supported.

Mixed legacy modules still mention MySQL types while their durable operations are being
moved behind backend repositories. Flat builds resolve those declarations through
`src/no_mysql/`, a client-free compatibility surface whose connection, query, statement,
and thread initialization calls always fail. It neither connects nor persists data and
must not be treated as a backend; flat-file primary routes durable operations through
the corresponding file repositories instead of this compatibility surface.

The root `Makefile` is the maintained full-project entry point. `make clean`
removes compiled server, editor, and area-tool artifacts but preserves generated
world data, the active runtime, package artifacts, and runtime history. Every
compiled artifact belongs below `bin/`, whose contents are gitignored:

| Path | Contents |
| --- | --- |
| `bin/server/` | Staged, active, and sanitizer server executables. |
| `bin/server/history/` | Bounded runtime executable history. |
| `bin/areas/` | Area editor and generation tools. |
| `bin/migrations/`, `bin/tools/` | Standalone maintenance utilities. |
| `bin/objects/` | Object and dependency files for every build flavor. |
| `bin/packages/` | Debian package and package metadata. |
| `bin/tests/` | Native test executables. |

Use `make clean-all` to remove every reproducible developer artifact. It
removes all generated contents below `bin/`, combined and scratch world
outputs, generated lookup tables, diagnostic-build output, coverage/profile
data, and Python/tool caches. It preserves `.env`, logs, player and account
data, and the hand-maintained `areas/world.justice`, `world.tab`, and
`world.weather` runtime inputs.

## Compile flags

From the default build line:

| Flag | Meaning |
|------|---------|
| `-std=c++20` | All `.c` files are compiled as C++20 with g++. |
| `-DTEST_MUD` | Development build: selects the `duris_dev` database credentials in `src/sql.h` and enables test commands. |
| `-D__NO_TESTS__` | Excludes built-in test hooks. |
| `-D__NO_MYSQL__` | Selected by `PERSISTENCE_BACKEND=flatfile`; removes the client compile/link dependency. |

Redis is optional at runtime, but Hiredis and OpenSSL remain build dependencies because
one server binary supports both the enabled and disabled runtime configurations.

`HARDENING_FLAGS` adds `-Og -D_FORTIFY_SOURCE=3 -fstack-protector-strong
-fstack-clash-protection`. `EXTRA_CFLAGS` / `EXTRA_LDFLAGS` are appended last
and exist so a wrapper (notably `scripts/build-san.sh`) can add instrumentation
without discarding the warning profile or the feature defines.

Link libraries: `mysqlclient`, `gnutls`, `ssl`, `crypto`, `cjson`, `hiredis`,
`bsd`, `xml2`, `z`, `crypt`, `pthread`.

Chaos gameplay mode is selected at runtime with `CHAOS_MUD=TRUE` or
`CHAOS_MUD=FALSE` in `.env`. The default example disables it. The value must
use uppercase `TRUE` or `FALSE`; numeric values are not supported. See
[CONFIGURATION.md](../operations/CONFIGURATION.md) for this and the other runtime switches.

Dependency install lines are in [README.md](../../README.md#quick-start). The
`packaging/` directory contains the equivs manifest for the complete developer
toolset, including Git, Python, area-generation utilities, the
compiler-provided sanitizer runtimes, Valgrind, GDB, and clang-format. It
accepts an existing MariaDB or MySQL toolchain and otherwise uses the
distribution's default MySQL-compatible packages.

## Warning profile

`src/Makefile`'s `WARNING_FLAGS` is `-Wall -Wextra -Wpedantic -Werror` plus an
explicit list of correctness and hardening diagnostics. There are **no**
`-Wno-*` exceptions, and six categories that are not implied by `-Wall
-Wextra` are named explicitly so the guarantee is deliberate rather than
incidental:

`-Wwrite-strings`, `-Wunused-parameter`, `-Wunused-variable`,
`-Wunused-but-set-variable`, `-Wmissing-field-initializers`,
`-Wunused-function`.

Those six were global exceptions until August 2026, hiding ~9,950 diagnostics.
Clearing them surfaced real defects - five `MAX_STRING_LENGTH`-bounded formats
writing into 256/512-byte caller buffers, a truncated player-save `INSERT`
handed to MySQL as malformed SQL, and a WebSocket handler that was never wired
into its dispatcher - so the rules below exist to keep the signal trustworthy:

1. Never reintroduce a blanket `-Wno-*` flag, a warning pragma, or a file-wide
   suppression. Resolve the diagnostic instead.
   `tests/async/test_compiler_warning_profile.py` fails if one appears.
2. Use `[[maybe_unused]]` only where a build configuration genuinely needs the
   declaration - typically a parameter whose only use sits inside an inactive
   `#if`. Never to hide unexplained dead state.
3. Do not use `const_cast` or a C-style cast to satisfy `-Wwrite-strings`. Make
   the callee `const char *` when it does not write through the pointer; give
   it a mutable buffer when it does (see `writable_arg` in
   [CODEBASE.md](../reference/CODEBASE.md#c-conventions-the-warning-profile-enforces)).
4. Do not delete a set-but-unused calculation until its intended effect is
   understood - it often marks a missing check, charge, or return value.

`scripts/warning-inventory.sh` performs a clean build with the six categories
enabled but non-fatal and writes deduplicated counts by category and by file to
`build/warning-inventory/{raw.log,dedup.txt,report.txt}` (`build/` is
gitignored). The report records the compiler version and the full flag set so
counts stay comparable across runs.

Note that `-Wformat-truncation=2` is fatal: `snprintf` into a fixed buffer must
have a bound the compiler can narrow. Format each row into its own bounded
buffer rather than appending at `buf + strlen(buf)` with a
`MAX_STRING_LENGTH` bound.

When a change touches conditional code, compile-sweep the affected files under
the non-default configurations as well - `REQUIRE_EMAIL_VERIFICATION`,
`CTF_MUD=1`, `SIEGE_ENABLED`, and `MEMCHK` are the ones in use:

```bash
g++ -fsyntax-only -std=c++20 -DTEST_MUD -D__NO_TESTS__ -DCTF_MUD=1 ... src/<file>.c
```

## Sanitizer build

`scripts/build-san.sh` builds an ASan/UBSan instrumented binary for hunting
memory bugs. The sanitizer runtimes arrive with the compiler declared by the
developer dependency manifest; no separate sanitizer package is needed. Use
this build for debugging sessions, not for production.

It appends its flags through `EXTRA_CFLAGS`/`EXTRA_LDFLAGS` so the full warning
profile is preserved, builds objects into `bin/objects/server-san/`, and leaves
its result at `bin/server/dms_san` - it never touches `bin/server/dms`. Do not
switch it back to `export CFLAGS=...`: a Makefile assignment overrides an
exported variable, so the flags would silently never reach the compiler.

Usage, tool selection, and the rules for when a dynamic check is required are
in [MEMORY_CHECKING.md](MEMORY_CHECKING.md).

## Area file generation

The server boots from combined area files (`areas/world.wld`, `world.mob`,
`world.obj`, `world.zon`, `world.qst`, `world.trg`, plus lookup tables). These
are generated, not hand-edited:

1. Build the compilers: `make build-area-tools` (produces `make_mob`,
   `make_obj`, `make_qst`, `make_shp`, `make_wld`, and `make_zon` under
   `bin/areas/tools/`).
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
  ./bin/server/dms 4000 &
  sleep 5 && telnet localhost 4000   # confirm greeting, then shut down
  ```

- If you changed persistence-related code, run the relevant focused test;
  see [TESTING.md](TESTING.md).
