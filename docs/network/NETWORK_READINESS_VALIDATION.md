# Network readiness validation

This change replaces the zero-time `select()` sweep and descriptor-free sleep
with direct `pollfd` registration and a monotonic simulation deadline. The game
thread still owns sockets, descriptors, command dispatch, and world simulation.
The phase and overrun contracts are in [GAME_LOOP_PHASES.md](GAME_LOOP_PHASES.md).

## Reference and implementation choice

The implementation was checked against Luminari's August 30, 2026
[reactor change](https://github.com/LuminariMUD/Luminari-Source/commit/dc17d97260abdc6decaa70ea57424ab087adda14)
and [scheduler bridge](https://github.com/LuminariMUD/Luminari-Source/commit/51a1f2e59c2522373052fb44b89598c86273172b).
Their readiness registration and deadline/budget separation inform this loop.
Duris uses POSIX `poll()` directly: the explicit 256-connection capacity makes a
linear readiness sweep suitable, removes the descriptor-number ceiling, and
requires no new reactor dependency or scheduler migration. The work starts at
the requested `2a647b6ec7b21fba79f58b7af8fbc1cf90ec78da`, then integrates the
latest merged session queues, asynchronous account loading, and game-loop
watchdog at `47085b61acf1e688e85363547a8c58c4efdcf743`. The watchdog still counts
completed world loops; readiness turns do not report simulation progress.

## Measurement method

`tests/async/test_network_readiness_journey.py` runs the real executable in the
existing disposable flatfile fixture. It creates its own account and character,
uses ephemeral loopback listeners and a test TLS certificate, and never opens the
workspace's configured account or database. The baseline executable was built
from the starting HEAD before any source edits and retained separately.

Both probes use the same script and random seed. Each protocol has 24 connections,
with seeded 15–170 ms delays before connecting to vary their simulation phase.
Telnet measures connect-to-first-negotiation-byte latency; TLS measures connect
through the real SSL handshake; WebSocket measures connect through the HTTP 101
response. These are transport measurements, not gameplay execution latency.
The burst measures opening 96 connections and receiving the first byte from all
of them. CPU is process user/system time over a three-second idle sample, including
the process's worker threads, at the host's `/proc` accounting granularity.

For command timing, the fixture sends eight `score` commands in advance and
identifies each response by the character name and its framed prompt. Seven
inter-response intervals are measured over both Telnet and TLS. TLS reconnect
crosses the real password worker. An additional changed-server admission check
requires all 256 connections to receive negotiation and the 257th to be refused.

The observations are from Ubuntu 22.04 under WSL, using GCC 12 and the flatfile
development build on October 2, 2026, America/Denver. Other local work can share
the host; this is a small local before/after probe, not a controlled production
load benchmark. JSON reports and binaries are retained under ignored
`bin/network-readiness-baseline/` and `bin/network-readiness/` in the worktree.

## Measured results

| Measurement (ms) | Before p50 | After p50 | Before p95 | After p95 |
| --- | ---: | ---: | ---: | ---: |
| Telnet first negotiation byte | 158.215 | 0.348 | 231.744 | 0.512 |
| TLS handshake | 183.903 | 2.549 | 227.956 | 3.062 |
| WebSocket HTTP upgrade | 406.332 | 0.443 | 475.667 | 0.520 |
| Queued Telnet command spacing | 250.233 | 250.557 | 250.339 | 250.592 |
| Queued TLS command spacing | 250.219 | 250.529 | 252.822 | 250.634 |

All 96 Telnet peers received their first byte in **752.926 ms before**
and **59.741 ms after**. The changed server admitted
**256** peers and refused the extra connection.

Idle CPU measured **0.000% before** and
**0.000% after** over approximately three seconds each.
This short, quantized observation does not establish a CPU improvement. Transport
latency decreased in this local sample while eligible command spacing stayed
near 250 ms over both protocols.

The final production-seam executable measured **837 microseconds** from input
publication to servicing and **72 microseconds** from worker hint
publication to servicing, following an intentional 20 ms delayed producer in each
case. Its pass/fail bound allows less than 100 ms total wait, including that delay.
These are single local observations, not tail-latency guarantees.

## Executable coverage

`test_network_readiness_runtime.py` extracts and executes the actual production
registration, turn, deadline, input-read, and connection-boundary code with
AddressSanitizer and UndefinedBehaviorSanitizer. Other game owners and TLS faults
are isolated. Real sockets, a real IPv6 listener, and the real wakeup pipe cover:

- A descriptor above `FD_SETSIZE`, prompt wakeup for input and worker publication,
  full-pipe coalescing, and no early return from a simulation deadline.
- Handshake and receive retries in both TLS readiness directions, independently
  retained send directions, and silent handshake expiry.
- Slow readers with retained output, a busy producer beside a quiet reader, byte
  quota exhaustion, and idle waits without write-interest spinning.
- A 96-connection IPv6 burst with 32 accepts per turn, close-aware traversal,
  orderly EOF with a single input opportunity, hangup quiescence while a TLS
  send owns its retry, and the existing TCP urgent-data exception policy.
- Monotonic deadline rounding and an overrun that advances no extra logical tick.

The existing Telnet output harness executes production compression, partial
writes, retained TLS records, and bounded flushes. The WebSocket runtime harness
executes production protocol parsing, backpressure, fragmentation, compression,
FIFO staging, boundary-only application dispatch, and descriptor lifetime during
login/logout callbacks. Its real IPv6 sockets also test fragmented trusted PROXY
headers, TCP4/TCP6 addresses, invalid/untrusted sources, and mapped IPv4 peers.
The real listener journey supplements injected TLS retries with real handshakes,
authentication, output, admission, idle CPU, and command timing.

## Validation results

- Formatting: `scripts/format.sh` and the final `scripts/format.sh --check` passed.
- Both MariaDB and flatfile `make -C src` builds passed with the repository's strict
  warning flags and GCC 12 after integrating the latest merged session queues,
  asynchronous account loading, and game-loop watchdog.
- All 57 selected networking, phase, password, persistence, and scheduler checks
  passed, including all twelve `test_nevent_*` checks and the new session-queue,
  account-loader, and watchdog regressions. The runner had 56 passes and one
  missing link dependency in the copyover custody harness. Its focused rerun
  passed after linking the new watchdog module. There were no skipped checks.
- The production readiness seam and account-loader notification/lifecycle checks
  passed ASan/UBSan; the readiness executable was also run to record wakeup timing.
- The real listener/timing/admission journey passed against the final flatfile
  executable. The following existing real journeys also passed with that binary:

  - game-loop session/save/quit/reconnect: 234.199 seconds.
  - password recovery with SMTP: 10.228 seconds.
  - legacy password recovery disabled: 6.059 seconds.

## Reproduction

Use a GCC version that supports this repository's C++20 atomic shared pointers
and warning flags. Standalone harnesses invoke `g++` directly, so that executable
on `PATH` must also provide the selected compiler. This host used GCC 12,
clang-format from the local toolchain, and hiredis 1.4.1 headers/libraries.
The paths below are relative to the worktree
because its Windows directory contains spaces; compiled artifacts stay in `bin/`.

```sh
./scripts/format.sh
./scripts/format.sh --check
make -C src CC=g++-12 BIN_ROOT=../bin \
  OBJDIR=../bin/objects/network-readiness-mariadb \
  EXTRA_LDFLAGS=-Wl,--no-keep-memory -j2
make -C src CC=g++-12 PERSISTENCE_BACKEND=flatfile \
  BIN_ROOT=../bin/network-readiness \
  OBJDIR=../bin/objects/network-readiness-flatfile \
  EXTRA_LDFLAGS=-Wl,--no-keep-memory -j2
python3 tests/async/test_network_readiness_runtime.py
python3 tests/async/test_network_readiness_journey.py \
  --binary bin/network-readiness-baseline/server/dms_new --measure-only \
  --report bin/network-readiness-baseline/measurements-final.json
python3 tests/async/test_network_readiness_journey.py \
  --binary bin/network-readiness/server/dms_new \
  --report bin/network-readiness/measurements-final.json
```

The regression runner serializes the real readiness journey with other tests that
build/start full servers. Individual networking, phase, password, persistence,
and scheduler tests remain directly executable under `tests/async/`.

## Practical limits

Network readiness does not make a gameplay command eligible early. Authentication
completion and application output retain their simulation boundaries; durable
completion publication still follows `ne_events()` on the established every-two-
pulse cadence. A wakeup hint causes prompt poll servicing without publishing the
authoritative worker result between phases.

The loop cannot preempt a synchronous world callback. An overrun schedules the
next world pulse a full interval after work finishes and discards missed real-time
slots. One pulse still advances one logical tick; the scheduler's existing callback
backlog, budgets, ordering, and deferrals remain independent. A bounded network
sweep that crosses its deadline finishes before the next pulse. Millisecond
`poll()` rounding and host scheduling can add timing jitter.

Read allowances retain the previous per-pulse byte limits, and pending WebSocket
application messages are bounded before additional parsing. The 256-connection
policy is explicit; this change does not claim scalability beyond that policy.
Production Internet latency, live SQL load, and world-scale CPU load have not
been benchmarked by these local fixtures. No schema change or migration is part
of this implementation.
