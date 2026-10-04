# Personal-local selected-control qualification

Run from the repository root with Python 3 and Docker Desktop or a local Docker
daemon:

```text
python tests/async/qualify_telemetry_controls.py --disposable
```

This command builds the maintained tools image, builds `bin/server/dms_new`,
checks touched C++ formatting, runs focused contracts and ASan/UBSan, measures
control capture and the existing transport/report budgets, and starts disposable
MariaDB 10.11.14 and MySQL 8.0.46 fixtures sequentially. Each engine exercises all
six migration histories, exact control storage, the native runtime/writer/report
journey, and a real server journey. It creates no host-facing listener or port.

For an explicitly selected compatible local tools image, the same command accepts
`--tools-image IMAGE`. The image must contain the maintained build dependencies,
`clang-format`, both supported SQL clients and Python's `pymysql`/`cryptography` packages. Omit
this option for the repository's reproducible tools build. `--engines mariadb`
or `--engines mysql` is useful when investigating one engine; a two-engine pass is
required for this slice's acceptance.

## Ownership and allow-list

The runner generates a fresh resource token and labels every created container.
Its runner uses `--network none`; SQL sidecars share only that runner's network
namespace and are reached through explicit `127.0.0.1` TCP connections. It does
not read `.env`, saved SQL connections, live accounts, production or staging.
The provisioning role exists only in the new disposable SQL server. Fresh
database names must match the existing `duris_telemetry_test_*` guard; migration
history fixtures additionally use the maintained `duris_268_*test` guard.
Per-mode names use 33 ASCII characters, including a digest of the unique run and
mode. The runtime's 31-character exclusion prefix then fits MySQL's 64-character
named-lock limit. The real-game fixture rejects longer names before provisioning.

The real game uses two synthetic accounts and a dedicated gameplay SQL role.
Telemetry has a separate writer role with the existing table-level grants.
Retention/publication, reporting and reviewed incidents each use their own role.
The report role cannot read private inputs or mutate the report stores. The game
never authenticates as the provisioning role.

Normal completion and failure both remove only containers created by this run,
after verifying their ownership label. Fixture helpers remove their dedicated
databases and users. Build artifacts, receipts and synthetic failure logs stay
under ignored `bin/tests/duris-controls-<token>/`. An interrupted host process
can leave its labeled containers behind; inspect that exact run token before
manually removing them. Never use a blanket Docker cleanup command.

## What the receipt proves

`qualification.json` records the source digest, phase exit codes and durations,
the exact engine receipts, binary hash, performance samples and cleanup result.
It refuses a pass if qualification source files changed during the run.

The maintained native fixture executes selected apply/remove/expiry/refresh and
aggregation bodies, actual cure/wake/song/damage-release blocks, managed wards,
staff flag/offset writers, equipment and save rebuilding. Its expected ordered
transitions detect both missing observations and false intermediate off/on
events. Temporary or borrowed character identities are excluded. Pure capacity,
clock, loss, overlap and sequence-exhaustion cases protect the fixed accumulator.

The real server journey creates and authenticates the synthetic accounts with
telemetry disabled, verifies zero observations, then enables telemetry. Actual
commands apply major paralysis and blindness, overlap them, cure blindness and
allow paralysis to expire on the real scheduler. Equipment application/removal
and repeated durable saves check final status conservation. Failed copyover
preserves gameplay; successful exec changes producer identity and retains the
logical sessions and establishes a fresh control baseline. A private writer
authentication outage leaves gameplay saves usable and opens the bounded terminal
circuit. Restoring access followed by an explicit operator lifecycle restart
restores capture under a fresh producer. The protected ledger retains abandoned
or unknown backlog; this tail is reviewed as loss, never recast as delivered
activity or measured zero. Exact retained controls
then pass through definition 7 and the restricted report reader.

The storage/publication journeys additionally qualify all 76 fields, immutable
configuration evidence, bounded pages/fanout, independent schema-6 incidents,
rollback, ambiguous commits, replay, corrected generations, unchanged older
definitions/generations, raw-retention independence, guarded migration reruns,
metadata drift refusal and exact restoration.

## Measurement boundaries

The new native measurement compares an off baseline with actual fixed-state
control accumulation plus portable encoding at 50, 200 and 512 resident targets,
4,096 samples per profile and five repetitions. It checks the documented local
capture guards: p99 at most 1 ms and p99.9 at most 5 ms. Its allocation guard
aborts on event-time heap allocation. The fixed state is 213,048 bytes for 512
target slots; exhaustion emits explicit uncertainty and never overwrites a
resident target. This measurement excludes runtime context refresh, SQL and
network latency. The existing transport/rollup/report gate runs separately with
its original budgets and injected faults.

The real journey records ten telemetry-off save round trips and ten saves while
telemetry is enabled and a selected equipment status is present. These include
Telnet scheduling and authoritative persistence; they are evidence of this
disposable host, not isolated callback timings or production latency guarantees.
ASan/UBSan is enforced. TSan is not run by this command; the earlier Docker host
probe was unsupported. No full burn-in or production load test is claimed.

## Interpretation and next dependency

Definition 7 retains operations and target-state prefixes separately. Qualified
status microseconds require verified predecessor, association/context,
configuration, clock and loss evidence. Other durations remain NULL. Accepted
applications and signed configured ticks remain distinct units. Overlapping
family durations cannot be added to obtain time controlled. Action-restriction
time and caster-attributed duration remain NULL.

This is a selected-control measurement slice. Issue #258 remains open
with all seven final acceptance requirements preserved. The first follow-up is a
normal PvP journey with positively qualified live elapsed prefixes, checking the
native context, paired clocks and complete independent review windows. The current
staff-command journey conserves inputs and uncertainty; zero qualified prefixes
does not establish zero status time. Reviewed build/power comparisons and typed
death, escape and objective outcomes then complete the next battle dependencies.
Prevention/faction coverage, distinct PvE attempts and
committed reward links, progression/portfolio additions and the four complete
balance report suites remain in the accepted expansion.

## Passed selected-control qualification: 2026-10-04

The repository-root command below passed **31/31 phases** in one complete run
(`180756252bd3`), on both MariaDB 10.11.14 and MySQL 8.0.46:

```text
python tests/async/qualify_telemetry_controls.py --disposable
```

The receipt is `bin/tests/duris-controls-180756252bd3/qualification.json`. It records
the unchanged qualification source SHA-256
`1869891999e760c95b0bed881af2cd5e1621acdf38b69e8fa5179d1979f9bb8a` and verified cleanup of every container created by
the command. Fixture databases and dedicated users were removed. Only documentation evidence and inventory line endings changed afterward;
executable, migration and test source bytes were audited unchanged against the
6,080-file frozen snapshot. The normalized inventory retains identical JSON values.

The server build, eleven C++ formatting checks, thirteen focused programs,
84 history tests, runtime/lifecycle validators, both ASan/UBSan journeys and
both performance gates passed. Native adapters verify 18 callback transitions,
58 maintained native transitions across all eight statuses, and 11 scheduled
and canceled timers with safe teardown. Both engines pass all six 71-step /
267-table histories, compiled boot/restoration and tamper rejection, 76 exact
control fields and 455 tagged payload columns, atomic publication/replay/fault
journeys and immutable older definitions/generations. All 144 earlier sealed
migration files retain their original bytes.

| Engine | Native SQL controls / operations / states | Actual server controls / operations / states | Live qualified elapsed prefixes |
| --- | --- | --- | --- |
| mariadb:10.11.14 | 80 / 56 / 24 | 24 / 5 / 19 | 0 |
| mysql:8.0.46 | 80 / 56 / 24 | 24 / 5 / 19 | 0 |

The actual server verifies synthetic-account authentication, disabled capture,
native apply/overlap/cure/expiry, equipment/save rebuilding, failed and successful
copyover, logical-session continuity, a terminal private-writer authentication
outage while gameplay saves remain usable, explicit operator restart recovery,
protected ledger readback and restricted published reports. Reviewed abandoned
or unknown outage evidence remains loss; restoration does not reconstruct it.

Worst capture p99 is **0.553 µs**, p99.9
**1.853 µs**, and maximum **29.851 µs** across
30 profiles, against the 1-ms / 5-ms p99 / p99.9 guards. Fixed state is
213,048 bytes for 512 targets with zero event-time allocation. The original
transport/rollup/report gate passes at 50/200/1,000 workloads with six injected
faults. The capture measurement excludes runtime context refresh and SQL.
The measured compiler is `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` with `-O2`;
Python is `3.12.3 (main, Aug 31 2026, 10:18:26) [GCC 13.3.0]`. The receipt records the
exact tools image ID and binary hashes.

| Engine | Save capture mode | Samples | Median round trip (ms) | Maximum (ms) |
| --- | --- | --- | --- | --- |
| mariadb:10.11.14 | telemetry_off | 10 | 2004.135 | 2004.626 |
| mariadb:10.11.14 | telemetry_on_status_present | 10 | 2004.477 | 2202.315 |
| mysql:8.0.46 | telemetry_off | 10 | 2004.465 | 2004.859 |
| mysql:8.0.46 | telemetry_on_status_present | 10 | 2004.137 | 2004.675 |

These save timings include Telnet scheduling and authoritative persistence;
they do not isolate callback overhead. TSan is unrun by the command and the
earlier host probe was unsupported. No full burn-in or production load result
is claimed. No production or staging access was used.

The reducer has five positively qualified fixture prefixes; native SQL and
these staff-command server journeys preserve unknown elapsed duration.
Zero qualified live prefixes does not establish zero status time. The next
dependency is normal live PvP with positively qualified context, paired clocks
and complete independent review windows, then reviewed build/power comparisons
and typed death/escape/objective evidence. Action-restriction time and
caster-attributed duration remain NULL. Issue #258 stays open with its seven
final acceptance requirements unchanged; the full expansion is unfinished.
