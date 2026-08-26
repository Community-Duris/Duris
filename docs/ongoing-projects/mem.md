# Memory-Checking Standard

This document defines the project's routine memory-checking practice. It is a
runbook and review standard, not a proposal to replace Duris's allocators or add
a new diagnostics framework.

## Core rules

1. Treat the server as C++20. The `.c` files in `src/` are compiled with `g++`;
   use the repository's build scripts instead of generic C compiler recipes.
2. Run dynamic memory tools only against a local/development environment and
   development database. Check `.env` before starting a session, and never put
   credentials in commands, logs, or bug reports.
3. Keep the normal build unsanitized. Sanitizer objects and the sanitizer binary
   stay isolated from `obj/` and `./dms`.
4. Do not run a sanitizer-instrumented binary under Valgrind. Use one detector
   at a time.
5. Investigate the first reported error with a Duris frame. Later failures are
   often consequences of the first one.
6. End leak-checking sessions with a clean in-game shutdown. Forced termination
   can prevent useful end-of-process leak reporting.
7. Do not commit diagnostic binaries, object files, cores, or tool logs.

## Which tool to use

| Tool | Use it for |
| --- | --- |
| Normal build (`make -C src`) | Compiler diagnostics and the normal hardened build. Run after every C/C++ change. |
| ASan + UBSan | Heap, stack, and global bounds errors; ordinary use-after-free; invalid frees; and undefined behavior. This is the default dynamic check for pointer, buffer, ownership, and lifetime changes. |
| Valgrind Memcheck | Uninitialised reads, invalid accesses, file-descriptor leaks, and detailed leak comparisons. Use it when those are suspected or when sanitizer coverage is insufficient. |
| Helgrind or DRD | Suspected races in threaded save, SQL, or queue paths. Use a focused workload because both are slow. |
| Massif | Heap-growth investigation. It profiles memory use; it is not a correctness check. |
| In-game `mreport` | A quick view of Duris's tagged allocation and pool counters. It complements external tools but does not detect corruption or prove that memory is safe. |

## ASan and UBSan

Build with the checked-in wrapper, then run the result from the repository root
on a development port:

```bash
./scripts/build-san.sh --clean -j2
mkdir -p logs/log

ASAN_OPTIONS='detect_leaks=1:abort_on_error=1:log_path=logs/log/asan' \
UBSAN_OPTIONS='print_stacktrace=1:halt_on_error=1' \
./src/dms_san 4000
```

`scripts/build-san.sh` places objects in `obj-san/` and writes
`src/dms_san`; it does not replace the normal runtime binary. Stop any other
local server first so that auxiliary listeners do not conflict.

Exercise the smallest path that reproduces the issue, then shut the game down
cleanly. A clean sanitizer run means only that the executed paths passed.

Avoid copyover during an ASan session. The current copyover path executes
`./dms`, so code after that transition is not guaranteed to remain under the
sanitizer build.

## Valgrind

Use the project wrapper rather than assembling Valgrind options by hand:

```bash
./scripts/valgrind_mud.sh --build          # Memcheck, development port 4000
./scripts/valgrind_mud.sh --tool=helgrind
./scripts/valgrind_mud.sh --tool=drd
./scripts/valgrind_mud.sh --tool=massif
```

For an automated check that must fail when Memcheck reports an error:

```bash
./scripts/valgrind_mud.sh --build -- --error-exitcode=99
```

The wrapper supplies the project's standard Memcheck options, applies
`scripts/valgrind.supp`, and writes reports under `logs/valgrind/`. It refuses
port 7777. Use `--trace-children` only when copyover itself is the subject of
the test. See [the detailed Valgrind guide](../valgrind.md) for all supported
tools, options, and expected baseline noise.

Valgrind is much slower than a native or sanitizer run. Prefer one intentional
scenario over broad manual wandering, and allow the server time to boot.

## When checks are required

| Change or investigation | Minimum check |
| --- | --- |
| Any C/C++ change | Normal build plus the smallest relevant regression test. |
| Allocation, ownership, pointer, buffer, or object-lifetime change | ASan/UBSan build and a focused runtime scenario. |
| Crash, corruption, invalid free, or out-of-bounds fix | Reproduce under ASan/UBSan before the fix when practical; rerun the same scenario after the fix. |
| Leak, uninitialised value, or file-descriptor issue | Focused Memcheck run with a clean shutdown. |
| Threaded save, SQL, or queue synchronization change | Focused Helgrind or DRD run. |
| Release candidate or periodic maintenance check | Repeat a documented Memcheck scenario and compare it with the same workload's baseline. |

Expensive dynamic checks are targeted, not a requirement for every unrelated
change. If a required check cannot be run, record what was skipped and why.

## Standard test session

1. Record the revision, detector, command, and relevant environment options.
2. Confirm that `.env` describes a local/development environment.
3. Start on a development port and use the test account from `.env` without
   exposing its credentials.
4. Exercise the exact reproducer. Add nearby create/use/destroy or
   connect/play/disconnect cycles only when they test the same lifetime.
5. Shut down cleanly and preserve the detector report outside version control.
6. Fix the first actionable report and repeat the same workload.
7. Rebuild normally and rerun the focused regression test before considering
   the issue closed.

Leak results are comparable only when startup, player activity, and shutdown
are comparable. Duris has long-lived world allocations, so judge a change by a
repeatable workload and its delta rather than demanding an arbitrary zero-leak
boot baseline.

## Reports and suppressions

A useful report contains:

- revision and detector version;
- exact command and detector options;
- the workload or reproduction steps;
- the first relevant stack trace; and
- the result of rerunning the same workload after the fix.

Do not suppress reports whose actionable stack is in Duris code. Entries in
`scripts/valgrind.supp` are reserved for understood third-party behavior and
must include a short reason. A suppression is not a substitute for an unclear
or intermittent investigation.

## Known limitation

Some objects come from the fixed pools in `src/mm.c`. Releasing one of those
objects returns its slot to a Duris pool rather than to the system allocator,
so ASan and Memcheck may not diagnose every stale reference to a released pool
object. `mreport` can show pool counts but does not close this detection gap.

This standard does not require an allocator redesign. When evidence points to a
pool-lifetime bug, add the smallest investigation-specific instrumentation or
regression test needed to prove it, and document any instrumentation retained
for future use.

## References

- [Sanitizer build wrapper](../../scripts/build-san.sh)
- [Build documentation](../BUILDING.md)
- [Valgrind wrapper](../../scripts/valgrind_mud.sh)
- [Valgrind project guide](../valgrind.md)
- [GCC instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html)
- [Valgrind Memcheck manual](https://valgrind.org/docs/manual/mc-manual.html)
