# Primary native stake audit registration — 2026-10-04

The [existing independent stake handoff](PLAN5_RETAINED_STAKE_AUDIT_SLICE_2026-10-04.md)
requires an explicit native/SQL invocation; its default portable unit run skips
the case. Current central manifest already registers the two native origin cases,
but omits `NativeStakeSQLTests.test_native_stake_sql_both_engines`.

`native_sql_stake_audit` now selects that existing class with its opt-in, provider
self-sql and engines once: the single case itself starts both disposable MySQL
and MariaDB with private datadirs/sockets. Existing900-second outer timeout and
central zero-skip/required-case enforcement apply. Require the suite marker and
both engine pass markers. No new test or gameplay gate is introduced.

The native price/stake fixture receives a fresh artifact path under its existing
bin/tests/plan5-price-view-sql boundary using the runner's random token. No prior
fixture output is selected or overwritten. The central tools Dockerfile now
installs python3-pymysql; its existing canonical build-dependency package already
supplies the compiler and libssl. This fixes an actual missing runtime dependency
previously reported by the independent origin slice without changing server
package requirements or installing anything on the host.

Source verification: all previous manifest rows/top-level values remain exact;
JSON decode and AST establish the selected existing case and opt-in; source
inspection confirms its both-engine loop, markers, runner no-skip enforcement,
fresh token/cwd and tools dependency definitions. No tests, container build,
database, native compilation or service run occurs in this registration change.
Execution remains at the original major-plan batch on the complete candidate.

The suite checks generated structural native roots and independent SELECT-only
audit/projection faults; even a later passing row does not prove real commerce,
full holdings capture, activation, player journeys or full R1-R8 completion.
Independent source review confirms manifest SHA256
725e5f5393904f33afa465a714e3f714d88b04afafaf569cafbc4fe0280402a8
and tools Dockerfile SHA256
25735d5cc944712be9432cbe262ad12f0f1c547faa1b710002ab39f4c8e12e83.
The owning native fixture still expects current tracked schema0056. The future
0057/0058 migration integration must refresh that fixture in the corresponding
planned qualification batch; this registration does not relabel old schema proof.
