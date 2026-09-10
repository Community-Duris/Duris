# NEVENT optimization findings: measured after the movement hotfix

Date: 2026-09-10. Status: **NPC short-affect hotfix implemented and locally profiled**.
The tracing sections below describe the pre-hotfix baseline; the latest result is
recorded at the end and in the [implementation plan](short-affect-scan-hotfix-plan.md).
The running local build includes the movement hotfix and opt-in diagnostic timers.
No scheduler policy, NPC cadence, or affect-expiration behavior was changed here.

## Recommendation

**Prioritize the global character-list scan in `event_short_affect()`, ahead of a
wheel rewrite or blanket NPC spell-up/patrol throttling.** Across four fresh
post-movement windows, this scan consumed **10.0% of measured event-loop time**
and **98.4% of its callback's time**. The whole deferral path consumed **1.5%**;
its sorting phase consumed **0.14%**.

This is the strongest focused avoidable-work candidate found, not proof of a
10% end-to-end speedup. Eliminating a lifetime check requires a correctness proof
and regression tests. The current tracing changes retain that check.

Mundane NPC callbacks remain the largest aggregate category, at 55.2% of event-loop
time. Their targeting, wandering, and spell-up sections each cost about 10–12%.
Those sections contain gameplay work, so their entire measured cost cannot be
counted as recoverable savings. The short-affect scan is a narrower target with a
clear source of repeated work.

Related work:

- [Movement scan hotfix and rollout measurements](movement-scan-hotfix-plan.md).
- [Earlier live casting latency findings](2026-09-10-live-casting-latency-findings.md).

## Current runtime and method

The baseline was the verified movement-only executable from the other agent's
rollout, SHA-256
`679497aaa2bae2c4f1fcc9edac730f7cba1fb53028929f94f311691642bded04`.
HEAD at the resumed investigation was `e66683d96`, which records the rollout;
the movement code is in `c535f2497`.

Two fresh approximately 40-second baseline windows preceded the diagnostic reload:

| Window end, server-log time | Event loop | All callbacks | Short-affect callback |
| --- | ---: | ---: | ---: |
| 15:44:38 | 1.547020 s | 1.388711 s | 0.210868 s / 184 calls |
| 15:45:21 | 1.758224 s | 1.571534 s | 0.520922 s / 339 calls |

The build containing five opt-in timers was compiled and loaded through the existing
local copyover mechanism. Runtime configuration stayed local MariaDB `duris_dev`,
plain port **7777**, under the existing `cycle_mud.sh` launcher. No service was
installed and no scheduler tuning was changed.

The instrumented executable and `/proc/4049211/exe` matched SHA-256:

```text
aec780800d92b475aca69eccf762ddd80a1798f44a88f0e68d23e85156623724
```

Its boot ID was `1789044364632781-4049211`. Callback names were regenerated from the
staged executable before code reload, so the new address map was available at boot.
Only two callbacks across all four captures were unnamed, totaling 35 microseconds.

Using the configured staff account, each capture turned profiling off, reset it,
enabled it for roughly 40 seconds, then disabled and saved it. A `look` every five
seconds exercised responsiveness. Four windows totaled **644 event passes** and
**724,497 callbacks**, approximately **160.2 seconds** of profiled elapsed time.
The character exited afterward. Profiling was left off and HTTP health passed.

These are monotonic elapsed-time measurements, not hardware CPU-cycle accounting.
The diagnostic build was complete before these four windows. The earlier baseline
windows overlapped build activity, and world activity differed across the copyover;
therefore they establish the repeated short-affect cost, not a controlled comparison
of diagnostic overhead.

## Direct phase measurements

| Window end, server-log time | Event loop | All deferral phases | Short-affect callback | Character-list scan |
| --- | ---: | ---: | ---: | ---: |
| 15:47:24 | 1270.028 ms | 39.635 ms | 136.195 ms / 113 calls | 134.359 ms |
| 15:48:07 | 1174.503 ms | 15.336 ms | 132.333 ms / 100 calls | 130.330 ms |
| 15:48:50 | 1412.798 ms | 18.073 ms | 136.395 ms / 115 calls | 133.990 ms |
| 15:49:33 | 1621.566 ms | 11.482 ms | 152.782 ms / 129 calls | 150.230 ms |
| **Total** | **5478.895 ms** | **84.526 ms** | **557.705 ms / 457 calls** | **548.909 ms** |

The scan averaged **1.20 ms per short-affect callback**. Its total was about
**6.5 times all measured deferral work**.

The four deferral phases totaled:

| Phase | Total | Share of event-loop time |
| --- | ---: | ---: |
| Collect due pointers into vector | 31.947 ms | 0.58% |
| Unlink / update bucket and debt | 5.251 ms | 0.10% |
| Sort deferred batch | 7.691 ms | 0.14% |
| Merge into next bucket | 39.637 ms | 0.72% |
| **All phases** | **84.526 ms** | **1.54%** |

There were 54 timed deferral calls. The captured log slices contained 60 budget
reports, a maximum deferred batch of 8,178, and maximum reported executed-event
lateness of two pulses. Log slices include small edges outside enabled profiling,
so the report count and timed deferral count are not interchangeable. These maxima
are observations, not general latency bounds.

## Remaining callback distribution

| Scope | Total measured work | Share of event-loop time |
| --- | ---: | ---: |
| All callbacks | 4.894910 s | 89.3% |
| Event loop minus callback bodies | 0.583985 s | 10.7% |
| Mundane NPC callbacks | 3.026996 s | 55.2% |
| Mundane target selection section | 0.653441 s | 11.9% |
| Mundane wandering section | 0.647410 s | 11.8% |
| Mundane spell-up section | 0.558522 s | 10.2% |
| Short-affect lifetime scan | 0.548909 s | 10.0% |
| Patrol callbacks | 0.031940 s | 0.58% |
| Movement regeneration callbacks | 0.053280 s | 0.97% |

Rows overlap. The mundane sections are inside mundane callbacks; lifetime scanning
is inside short-affect callbacks; deferral is inside event-loop overhead. Do not
sum these rows. Callback-body time includes callback-initiated event scheduling.
The scheduler-minus-callback difference includes dispatch, profiling registration,
event disposal, pending operations, and bookkeeping, not just deferral.

Spell-ups have become a meaningful secondary candidate now that movement is cheap.
Their timer measures decision work and immediate scheduling, not all subsequent
cast/affect/regen work. Determining total downstream cost would require attribution
not provided by these captures. Blanket throttling would change NPC healing,
cures, buffs, and readiness. Ordinary mundane work already slows threefold in
zones without remembered players; movement regeneration already stops when full.

## Why the short-affect scan is the next focused target

In `src/magic/affects.c`, `event_short_affect()` receives both a scheduler owner
argument and a payload containing `ch` and `af`. It currently ignores the owner
argument, then walks the entire global `character_list` until it finds the payload
character. Only afterward does it search that character's affects, issue wear-off
messages, and remove the affect.

The new `short_affect_liveness` timer brackets only that global membership scan.
It excludes affect-list lookup, messages, and removal. The measured result directly
isolates the cost rather than attributing the whole callback to a guessed cause.

Read-only ownership audit found:

- Both direct scheduling sites, `affect_to_char()` and `set_short_affected_by()`,
  pass the same character as event owner and payload character.
- Event creation links the character owner and records its runtime identity.
- `extract_char()` and `free_char()` remove affects and disarm character events.
- `affect_remove()` cancels the matching short-affect event through its handle.
- `nevent_cancel()` nulls the callback before deferring destruction, preventing a
  canceled event from later invoking its callback.
- `find_character_by_runtime_id()` itself walks `character_list`; substituting it
  would preserve the main cost rather than remove it.

These facts support a follow-up that relies on validated event ownership instead
of searching the world for the owner. They are not yet a complete lifetime proof.
The smallest proposed change should audit every scheduling/restoration path and
prove cancellation covers extraction, direct freeing, and address reuse before
removing the scan. Keep the affect-list membership check unless separately justified.
Do not copy the movement counter trick blindly: time passes between scheduling and
execution, and unrelated removals during that interval can invalidate such a cache.

Required follow-up regression cases include normal expiry, prior affect removal,
owner extraction/freeing before expiry, cancellation during another callback,
multiple short affects on one owner, and reused owner/affect addresses. Preserve
wear-off messages, removal order, and event ownership accounting. Run the focused
cases under ASan/UBSan, the scheduler suite, a build, and repeated comparable live
windows. No implementation of that optimization is included here.

## Why the original wheel recommendation changed

Before the movement fix, a 307-pass local capture measured 14.711 seconds in the
event loop and 9.300 seconds in callbacks: **36.8%** was outside callback bodies.
A separate old-boot sample, ticks 5100–5330, had a median 49,945 deferred events,
47.862 ms event passes, and 95 pulses of maximum executed-event lateness.

The source explains why that backlog was costly: `nevent_defer_suffix()` repeatedly
collects, unlinks, sorts, and merges the overdue suffix into the next bucket after
the callback budget is exhausted. A temporary source-based benchmark repeatedly
moving 40,000 same-deadline events measured median 4.342 ms, p95 6.168 ms. A phase-
instrumented benchmark measured mean sorting of 3.087 ms. That was a synthetic
saturated backlog, not a prediction for the post-movement world.

The movement patch removed the dominant producer-side cost and sharply reduced
deferral pressure. The direct post-fix sorting measurement is now only 7.691 ms
across four windows. This supersedes the earlier recommendation to prioritize a
wheel rewrite. Avoid optimizing the old overloaded distribution as though it were
still the current bottleneck.

The scheduler still compares original due tick before effective priority and
sequence. Player priority cannot leapfrog older overdue NPC events, and normal
aging can reorder equal-deadline work. Future wheel work must preserve these rules,
future revolutions, the current-pass sequence boundary, cancellation, and debt
accounting. Neither deleting sorting unconditionally nor increasing the budget is
an established fix.

## Delivered diagnostics and verification

The inherited four phase timers in `src/core/profile.h` and
`src/world/new_events.c` were retained. One timer was added in
`src/magic/affects.c` for the short-affect scan. All use the existing
`debug profile on/off/reset/save` mechanism and stay inactive by default. No new
service, configuration variable, timer framework, or gameplay policy was introduced.

Verification completed:

- `make -C src -j2` — passed for the diagnostic build.
- `./scripts/format.sh --check` and `git diff --check` — passed.
- `python3 tests/async/test_profile_monotonic_runtime.py` — passed, including the
  concurrent-worker elapsed-time check; repeated after adding the fifth timer.
- `python3 tests/async/test_profile_default_contract.py` — passed.
- `python3 tests/async/test_nevent_budget_contract.py` — passed.
- `python3 tests/async/test_nevent_scheduler_runtime.py` — all 12 modes passed under
  ASan/UBSan after the inherited scheduler instrumentation was rebuilt.
- Four completed live capture windows, responsive `look`, account exit, runtime
  checksum verification, and `./scripts/healthcheck.sh` — passed.
- No panic, corruption, segmentation-fault, or scheduling-failure matches in the
  four captured log slices.

At the end of tracing, the diagnostic edits were uncommitted. That baseline local
binary contains the verified movement fix plus all five timers; profiling is off.
The previous movement-only binary was retained by the normal copyover backup path.

Temporary evidence on this machine includes
`/tmp/duris-post-movement-profile-{1,2}.log`,
`/tmp/duris-deep-profile-{1,2,3,4}.log`, and
`/tmp/duris-deep-profile-summary.json`. The earlier pre-hotfix capture and synthetic
benchmark artifacts use the `/tmp/duris-nevent-*` prefix. These are temporary
artifacts, not durable repository evidence; aggregate results are recorded above.
No raw account credentials or player data are copied into this document.


## Implemented hotfix: follow-up result

The [short-affect hotfix](short-affect-scan-hotfix-plan.md#implementation-result)
now skips global membership scans for NPC event owners, retains the PC fallback,
and preserves affect selection and wear-off/removal order. The new real-scheduler
regression also exposed self-cancellation cleanup reading a released event; a small
scheduler guard now leaves canceled events to the existing deferred destruction
pass. The inherited tracing remains available and disabled outside captures.

Four new approximately 40-second windows measured **451 short-affect callbacks in
7.232 ms**, with **0.769 ms** in the membership guard. The corresponding earlier
tracing baseline was **457 callbacks in 557.705 ms**, with **548.909 ms** in the scan.
Average callback cost fell from **1,220.36 us to 16.04 us** (about **98.7%**).
This is a callback-specific elapsed-time improvement; overall event-loop work and
latency remain affected by the changing world workload.

The runtime candidate hash is
`7b0d1299f28963831f5d424268736619cb119147f6fbd544da5d4f51f35ded7a`,
boot `1789045777221348-4049211`, on local port 7777. The plan's result section records
the full sanitizer gates, ownership audit, comparison table, and rollout details.


Final live verification passed: a temporary NPC's major-paralysis affect and single
expiry event disappeared after its duration, stayed absent, and the NPC resumed
movement. The test NPC was removed, staff movement returned to the original room,
the account exited, profiling was confirmed off, and HTTP health passed. The
scheduler and dedicated cancellation suites pass under ASan/UBSan, including the
new cancellation/deletion/address-reuse cases. Source, tests, diagnostics, and these
documents are included together in the hotfix commit.
