# Short-affect scan hotfix: shortest implementation plan

Date: 2026-09-10. Status: **implemented, tested, and running locally; profiling and live smoke passed**.
See the implementation result below for the required scheduler cleanup correction.

## Outcome

Avoid the global character-list search when an NPC's short affect expires. Preserve
expiry time, affect selection, wear-off messages, removal order, and cancellation.
Use the existing scheduler-owned character rather than finding that same NPC again
by walking the world.

The [measured findings](2026-09-10-nevent-wheel-optimization-findings.md) put the
membership scan at 548.909 ms across 457 callbacks: 98.4% of short-affect callback
time and 10.0% of all event-loop time. All bucket-deferral phases together cost
84.526 ms. This is the focused next target; a wheel rewrite is not warranted by
those measurements.

**The capture did not split NPC and PC scan time.** Ten percent is the measured
all-character cost, not a promised saving from the NPC-only fast path. Confirm
its actual benefit with the existing timers before declaring the hotfix successful.

## Selected change: trust the NPC event owner; keep the PC fallback

Change only `event_short_affect()` in `src/magic/affects.c`:

1. Use the callback's `ch` argument instead of marking it unused.
2. Return for null payload, null owner, or `event_data->ch != ch`. Compare pointers
   before dereferencing the payload character. Equality is a consistency check,
   **not** a stale-pointer/lifetime check.
3. For PCs, retain the existing global membership scan and absent-character return.
4. For NPCs, skip that scan, relying on event ownership and cancellation.
5. Keep the existing affect-list search. Use the validated owner for that search,
   `wear_off_message()`, and `affect_remove()`, in the same order as today.
6. Retain the existing `short_affect_liveness` timer around the conditional guard,
   with balanced start/end calls on the PC early return. It will then measure
   the membership guard, including its cheap NPC branch, rather than an
   unconditional list search. Do not add another diagnostic framework.

Conceptual shape, not a patch:

```cpp
auto *event_data = static_cast<event_short_affect_data *>(data);
if (!event_data || !ch || event_data->ch != ch)
    return;

// Existing profiling brackets surround this conditional guard.
if (IS_PC(ch)) {
    // Existing character_list membership scan, unchanged.
    // Return if absent, ending the profiling interval first.
}

// Existing ch->affected pointer-membership search, unchanged.
// If present: wear_off_message(ch, af), then affect_remove(ch, af).
```

Do not introduce `IS_ALIVE`, room, position, or fighting checks: those would change
which existing affects can expire. Do not change the payload layout or producers.
Do not remove the affect-list check along with the character-list scan.

### Why preserve the PC path?

Player materialization and file loading can restore affects before `enter_game()`
links the character into `character_list`. `src/core/files.c` restores affects
through `affect_to_char()`; `src/account/nanny.c` separately performs world-list
insertion. Whether every such intermediate state can span an event pulse is a
larger login/persistence question that this hotfix need not answer.

Keeping the original PC membership behavior avoids broadening that audit. Normal
NPC creation in `read_mobile()` links the NPC into the world list before finishing
initialization. Copyover restores NPC affects through `affect_to_char()` as well.
This is a deliberate small NPC fast path, not a general replacement for liveness
validation throughout the game.

## Lifetime argument to establish before deleting the scan

The intended invariant is: **a runnable short-affect event's NPC owner remains
allocated and globally registered until that event is canceled or finishes**.
A matching owner pointer alone does not establish this invariant.

Already inspected:

- The two direct scheduling sites, `affect_to_char()` and `set_short_affected_by()`,
  pass the same character as scheduler owner and payload character.
- Event creation attaches the event to `ch->nevents`; it also records runtime ID.
- `extract_char()` and `free_char()` remove affects and call
  `disarm_char_nevents(ch, NULL)` before releasing the character.
- `affect_remove()` locates and cancels the corresponding short-affect event.
- `nevent_cancel()` nulls the callback immediately. When another callback is
  executing, physical destruction may be deferred, but the canceled callback
  must not run later in the pass.
- The main loop dispatches the character and payload from that same event.
- NPC copyover restoration calls the existing affect producer, not an alternate
  unowned short-affect callback.

Implementation must finish a narrow audit of NPC creation, extraction, direct
freeing, and copyover restoration. Check any direct NPC unlink/release paths and
any event-owner reassignment that can bypass cancellation. The existing runtime-ID
field is useful for diagnostics; do not claim it is validated automatically before
every callback merely because it is stored.

If the invariant is false for a reachable path, **do not ship the scan deletion**.
Record that exact path and reassess the hotfix instead of silently adding a global
registry or an unrelated lifecycle repair. The fast path has no defensive fallback
for a broken NPC ownership invariant.

## Files and inherited work

| File | Planned change |
| --- | --- |
| `src/magic/affects.c` | NPC-only scan bypass, owner/payload consistency guard, short ownership comment; preserve the inherited profiler. |
| `tests/async/test_nevent_scheduler_runtime.py` | Extend the existing real-scheduler harness with the production short-affect callback and a focused test mode. |
| This plan and the findings document | Record implementation status, actual verification, runtime identity, and measured result. |

The current master includes the movement hotfix and rollout documentation.
Uncommitted profiling changes in `src/core/profile.h`, `src/world/new_events.c`,
and `src/magic/affects.c`, plus the findings document, are inherited work and must
be retained. The proposed hotfix needs no additional production edits in the
scheduler or profile header. Review its incremental diff against that starting state.

## Proof: extend the existing harness

Reuse the real scheduler included by `test_nevent_scheduler_runtime.py` and the
existing `_paths.extract_function()` helper to compile the production
`event_short_affect()` body. Do not copy its logic into a lookalike test callback.
Add narrow message/removal fixtures; fixture removal should use the real event
cancellation APIs. Keep the existing allocator and ownership accounting checks.

Cover these behaviors in one new short-affect mode:

1. NPC expiry removes exactly the matching affect and emits its message before
   removal. Other affects remain. PC expiry preserves the same behavior.
2. A still-allocated PC outside `character_list` is ignored as before.
3. Null data, null owner, and mismatched payload owner cause no dereference or
   message/removal; a missing affect causes no removal.
4. Prior affect removal cancels the event. Owner disarming followed by real heap
   deletion prevents expiry from dereferencing the freed NPC.
5. Another callback disarms/deletes the owner before its expiry in the same pass;
   deferred destruction must not allow the canceled callback to run.
6. Reuse owner storage and affect storage after cancellation. An old event must
   not expire a new occupant's affect. Use placement reuse separately from the
   heap-free ASan case; they test different failure modes.
7. Multiple short affects on one NPC expire independently. Self-cancellation from
   affect removal during the callback leaves the scheduler balanced.
8. In a large synthetic character list, NPC expiry performs zero global-list
   visits while a PC still follows the existing scan. Count visits in the test
   instrumentation rather than imposing a machine-dependent timing threshold.

Add small source contracts in that same test for both production scheduling sites
and the cancellation-before-release hooks in `extract_char()` / `free_char()`.
These tie the lifetime fixtures to production, but do not label the fixtures a
full execution of extraction or player loading. No new test framework or standalone
benchmark is needed.

## Implementation sequence and verification

1. Capture the inherited diff and record the currently running binary hash. Finish
   the NPC ownership audit above.
2. Extend the existing harness and implement the single callback change. Format
   changed lines and run the relevant gates:

```bash
python3 tests/async/test_nevent_scheduler_runtime.py
python3 tests/async/test_affect_wear_off_message_contract.py
python3 tests/async/test_profile_monotonic_runtime.py
./scripts/format.sh --check
make -C src -j2
git diff --check
```

3. Preserve the current local runtime binary as rollback. For the local rollout,
   regenerate callback names from the candidate binary before code copyover;
   ordinary reboot does not promote staged code. Use the existing local launcher
   and port 7777. Verify the running executable hash after reload.
4. Using the existing staff account and profiler, take four approximately
   40-second windows after startup activity settles. Measure the short-affect
   callback and membership guard, callback count, event-loop time, deferral work,
   and lateness. Compare per-call cost as well as totals; world activity can change.
5. Exercise an actual short-duration affect through expiry, plus ordinary movement
   and responsiveness. Confirm exactly-once expiry behavior and health. Leave
   profiling off and exit the test character.
6. Update the docs with test results and measured benefit. Report PC fallback cost
   and any unrun validation. Revert the hotfix if lifetime failures appear; a lower
   timer reading alone is not a correctness result.

The diagnostic profile currently shows the all-character scan near 1.20 ms per
short-affect call. Acceptance is zero list visits for NPC expiry, unchanged tested
expiry/cancellation behavior, and a repeatable reduction in live short-affect cost
when NPC expirations occur. Do not promise a fixed percentage of whole-server CPU
or player latency from these elapsed-time measurements.

## Non-goals and ablation

Removed from this plan: a character hash registry, changes to `char_data` or event
payloads, runtime-ID lookup substitution, a removal-generation cache, a persistent
ready queue, bucket sorting changes, NPC spell-up/patrol throttling, and extra
tracing infrastructure. None is needed for the selected NPC fast path. Runtime-ID
lookup is itself a global scan; a generation cache spanning scheduling and execution
can be invalidated by unrelated removals and adds state without a demonstrated win.

The PC fallback stays because removing it creates a larger lifecycle proof obligation.
The focused tests stay because the optimization removes a defensive lifetime check.
The existing affect search and profiling stay because they protect behavior and
provide the comparison already available. No full burn-in, schema migration, service
installation, or broad cleanup is part of this hotfix.

The sections above record the original plan. The implementation and verification
below supersede its planned status and its original two-source-file scope.


## Implementation result

The NPC fast path is implemented in `event_short_affect()`. Null/mismatched owner
payloads return early; PCs retain their membership scan; the affect-list search and
message-before-removal order remain. The inherited `short_affect_liveness` timer
now measures the conditional guard. No payload, registry, NPC cadence, or wheel
ordering change was introduced.

### Necessary correction found by the new regression

The real-scheduler test exposed a use-after-free on ordinary short-affect expiry:
`affect_remove()` cancels the event currently executing, placing its handle in
`nevent_pending_cancellations`. The loop then destroyed that event immediately;
end-of-pass pending cleanup read the already-released event. The harness's allocator
actually frees event storage, so ASan reported the invalid access. Production's
reusable pool does not make that queued reference a valid ownership policy.

`ne_events()` now skips immediate destruction for `NEVENT_LIFECYCLE_CANCEL_PENDING`
events. The existing deferred cleanup destroys them exactly once at the end of the
pass. This three-line guard is necessary to make the planned self-cancellation and
lifetime tests pass; it does not change event deadlines, priority, or NPC cadence.
It is the only scheduler behavior change beyond inherited diagnostic timers.

The first sanitizer run failed on that access. After fixing it, all scheduler modes
and the dedicated cancellation suite passed. A subsequent test-fixture setup failure
was corrected by giving heap/reused NPC fixtures a valid standing/normal position;
otherwise the real scheduling API correctly rejected those fixtures.

### Ownership audit outcome

The inspected NPC allocation failure in `read_mobile()` releases storage before
list insertion or event creation. Normal NPC insertion precedes initialization of
owned events. The normal removal path disarms events before global unlink, and
`free_char()` disarms them before scheduling `release_mob_mem()`. Other direct
pool-release sites found in account/SQL/copyover loading concern player allocation
failures. Event owner assignment is in event creation and owner detachment clears
it during cancellation/destruction. NPC restoration uses `affect_to_char()`.

The regression uses the actual callback and scheduler, while affect removal/messages
and character deletion remain controlled fixtures. Source contracts check both
real scheduling producers and real extraction/freeing cancellation hooks. This is
not a claim that the entire extraction or persistence subsystem ran under the new
fixture.

### Completed automated checks

| Check | Result |
| --- | --- |
| `python3 tests/async/test_nevent_scheduler_runtime.py` | Passed all 13 modes under ASan/UBSan, including new short-affect mode. |
| `python3 tests/async/test_nevent_cancellation_runtime.py` | Passed under ASan/UBSan. |
| `python3 tests/async/test_affect_wear_off_message_contract.py` | Passed. |
| `python3 tests/async/test_profile_monotonic_runtime.py` | Passed both deterministic and concurrent-worker checks. |
| `python3 tests/async/test_nevent_budget_contract.py` | Passed. |
| `make -C src -j2` | Passed for MariaDB/development candidate. |
| `./scripts/format.sh --check` and `git diff --check` | Passed. |

New runtime coverage includes independent NPC affects, normal PC expiry, an allocated
PC outside the world list, malformed inputs/missing affects, prior removal,
self-cancellation, owner deletion before and during dispatch, owner/affect address
reuse, and a 4,097-character list. Measured list visits are **zero for NPC expiry**
and **4,097 for the retained PC path**. No wall-clock test threshold is used.

### Local rollout and measured result

Preserved the tracing baseline executable as
`bin/server/history/dms.short-affect-baseline`, SHA-256
`aec780800d92b475aca69eccf762ddd80a1798f44a88f0e68d23e85156623724`.
Regenerated callback labels from the candidate before local code copyover.
The running `/proc/4049211/exe` matched the new candidate SHA-256:

```text
7b0d1299f28963831f5d424268736619cb119147f6fbd544da5d4f51f35ded7a
```

Boot ID: `1789045777221348-4049211`. The server remains on local port 7777 with the
existing launcher and local MariaDB authority. No service was installed.

Four approximately 40-second windows, after the initial startup backlog cleared:

| Window | Short-affect calls | Callback total | Conditional guard total |
| --- | ---: | ---: | ---: |
| 1 | 115 | 1.081 ms | 0.052 ms |
| 2 | 98 | 1.945 ms | 0.568 ms |
| 3 | 117 | 2.063 ms | 0.072 ms |
| 4 | 121 | 2.143 ms | 0.077 ms |
| **Total** | **451** | **7.232 ms** | **0.769 ms** |

The earlier four-window tracing baseline had 457 calls, 557.705 ms callback work,
and 548.909 ms inside the guard. Average callback time fell from **1,220.36 us to
16.04 us**, approximately **98.7% lower**. Each new window showed a large reduction.
The guard retains any PC scans; there is no separate live NPC/PC counter, so the
remaining 0.769 ms cannot be attributed solely to PCs.

The new windows covered 644 passes and 741,425 callbacks, totaling 5.324547 seconds
inside the event loop. Deferral work totaled 148.053 ms; maximum executed-event
lateness in their budget reports was three pulses. This workload differed from the
earlier capture, including more deferral work, so the callback improvement is not
a claim of 98.7% whole-server acceleration or a fixed latency guarantee.

Evidence artifacts: `/tmp/duris-short-affect-after-{1,2,3,4}.log`,
`/tmp/duris-short-affect-after-summary.json`, `/tmp/duris-short-affect-tests.log`,
`/tmp/duris-short-affect-cancellation.log`, and `/tmp/duris-short-affect-build.log`.
Only aggregated results belong in the repository; these temporary files are not
committed. Source, tests, diagnostics, and documentation are included in the hotfix commit.


### Completed live expiry and cleanup checks

Created a temporary rabbit NPC (prototype 3220), assigned the unique keyword
`shortaffectprobe`, and cast major paralysis through the normal game command path.
A command-synchronized stat capture confirmed exactly one matching affect and one
`event_short_affect` expiry record. After 28 seconds, the affect, `MAJOR_PARA` flag,
and matching expiry event were absent; a later check confirmed they stayed absent.
The NPC resumed wandering. It was located and purged from its current room, then
verified absent by its unique keyword.

The first exploratory client read a spell-completion prompt as the response to a
later command, so its labels were not used as the verification result. The repeated
check waited for the actual stat output and asserted the before/after state. Its
first room-local cleanup missed the now-wandering NPC; a subsequent targeted cleanup
removed it successfully. These were client synchronization/cleanup issues, not a
server expiry failure.

Staff movement up to Eikel's room and down returned to the original room. Profiling
was explicitly confirmed off and the account exited. Final HTTP health passed;
the running executable hash still matched the candidate. A status-log scan from the
hotfix boot onward found zero panic, corruption, segmentation-fault, sanitizer, or
scheduling-failure matches. No temporary test NPC was left behind.

No migration, full burn-in, production rollout, or unrelated gameplay test was run.
The planned focused validation and local rollout are complete. All inherited and
new source/documentation changes are included together in the hotfix commit;
no credentials, logs, or binaries are included in the source changes.
