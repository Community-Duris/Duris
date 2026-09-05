# Spellcasting input queue investigation

Investigated on production on 2026-09-05 (UTC). Status: **confirmed live defect;
investigation only, no server fix deployed**.

## Finding

Ordinary mortal players can lose commands typed while casting. The server starts
removing commands from their input queue when their action-wait flag clears,
even if their spell is still running. The interpreter then rejects each removed
command with `You're busy spellcasting!`; those commands are not put back.

This was reproduced twice with a newly created level-56 human cleric using
ordinary Telnet input on production. It also reproduced with the configured
level-62 test character in trusted immortal mode. It is not merely an outdated
source checkout, a client-side macro problem, or an immortal-only observation.
The affected spells eventually completed; a permanently stuck casting flag was
not required or observed.

The earlier conversational explanation that spells should normally queue was
incomplete: queue preservation currently depends on **both** the casting flag
and the separate action-wait flag. Those flags do not always expire together.

## Production and evidence boundary

- Source HEAD: `9ac1801ac8e353acce6baaae570b62150b949177`.
- Running game process: PID `1476236`, started `2026-09-05 10:12:31 UTC`;
  executable `bin/server/dms`, working directory this repository.
- SHA-256 of both `/proc/1476236/exe` and `bin/server/dms`:
  `865a1d42aa685dd2a23f98b3a640931fc4177b18893e2e4b31d2beea80b58d66`.
- The executable contains `get_casting_cmd_from_q`,
  `input_allowed_while_casting`, `CharWait`, `event_wait`, and `event_spellcast`.
  Offline disassembly also shows the game loop calling the selective queue
  function. No debugger was attached to the live process.
- The active supervisor is the **user** unit
  `duris-mud-production.service`, MainPID `1475682`, initially `NRestarts=0`.
  The similarly named system unit was inactive and is not the authority for
  this running instance.
- Initial health response: `{"status":"healthy","persistence":"ready"}`.
- Source was inspected directly. Binary identity and observed behavior are
  recorded separately; the hash does not constitute a build-to-Git attestation.

The owner explicitly authorized in-game reproduction with test characters.
Credentials were loaded from the existing environment file inside the test
client and were not printed or written into this report. The client connected
to the configured production Telnet listener and used normal account menus.
No migrations, deployments, restarts, global setting changes, or artificial
load injection were performed.

## Live reproduction

Commands within each batch were sent as separate CRLF-terminated input lines
without intentional spacing. Times below are client receipt times relative to
the first relevant response, rounded to milliseconds. They describe observed
wall-clock behavior, not exact scheduler deadlines.

### Trusted immortal control

The configured test character was a human warrior at level 62, with `Fog: OFF`.
The batch was three copies of `cast 'armor' self`, followed by `look`.

| Relative time | Response |
|---|---|
| 0.000 s | `You start chanting...` |
| 0.251 s | Busy rejection for second cast |
| 0.502 s | Busy rejection for third cast |
| 0.753 s | Busy rejection for `look` |
| 2.761 s | Spell completes and applies armor |

A separate `cast 'armor' self`, `kick self`, `abort`, `look` batch rejected the
kick during casting, executed `abort`, and then executed `look`. This confirms
that the casting restriction covers commands other than `cast`.

`kick self` outside casting only produces `Aren't we funny today...`; that is a
validation failure and does **not** demonstrate real kick recovery. A valid
combat target was used for the mortal kick comparison below.

Temporarily enabling Fog on the configured warrior correctly made it reject
armor with `You don't know that spell!`. Fog was restored to OFF. Instead of
changing that character's class and resetting its learned skills, a disposable
cleric was created through the same test account's normal character-creation
flow. Production's Chaos creation flow automatically started that character at
level 56, the mortal level cap. It had no immortal trust bypass.

### Mortal control: Quickchant enabled

The cleric rested, prayed for three copies of armor, and verified all three
were memorized. It then stood and sent three copies of `cast 'armor' self`,
followed by `look`.

All three casts completed, in order, and `look` ran afterward. There were zero
busy messages. Observed first-cast-relative times were:

- First completion: 3.963 s.
- Second start/completion: 6.221 / 9.733 s.
- Third start/completion: 11.488 / 13.495 s.
- `look`: 14.750 s.

This is a passing control, not proof that Quickchant prevents every occurrence.
Successful Quickchant can shorten the actual chant while retaining longer
action lag, creating extra time before the faulty dequeue condition is exposed.

### Mortal failure: Quickchant disabled, five casts

After `toggle quickchant` reported `Quickchant is disabled.`, the cleric rested
and prepared more armor. `pray` showed eight memorized copies before this batch:

```text
stand
cast 'armor' self
cast 'armor' self
cast 'armor' self
cast 'armor' self
cast 'armor' self
look
```

| Relative time | Response |
|---|---|
| 0.000 s | First cast starts |
| 2.007 s | `Casting: armor *` |
| 3.462 s | Busy rejection 1 |
| 3.713 s | Busy rejection 2; casting progress also appears |
| 3.964 s | Busy rejection 3 |
| 4.215 s | Busy rejection 4 |
| 4.467 s | Busy rejection 5 |
| 4.968 s | First spell completes |

Only one spell started/completed. The other four cast commands and `look` were
discarded. A subsequent `pray` showed seven armor spells memorized, consistent
with one spell being consumed. The lost commands did not execute after the
completion or during the remainder of the 30-second observation.

### Mortal repeat: three casts

With Quickchant still disabled, a later independent batch of three armor casts
and `look` reproduced the failure again:

- First cast started at 0.000 s.
- Three busy messages arrived at 3.964, 4.215, and 4.466 s.
- First spell completed at 6.473 s.
- The other two casts never started, and `look` never ran during the 20-second
  observation.

### Mortal abort control

With Quickchant disabled, the batch `cast 'armor' self`, `look`, `abort`
started casting, then aborted after about 0.251 s. `abort` passed the earlier
queued `look`; that `look` remained queued and ran approximately 5.017 s after
the abort. There were no busy rejections. This demonstrates the intended
selective dequeue behavior while the action-wait flag remains set.

### Mortal kick comparison

The same cleric walked north/east to the beginner area's ball of light and sent:

```text
kick light
kick light
kick light
look
```

The first kick killed the NPC immediately. After about 5.719 s of recovery, the
second command returned `Kick who?`, the third did the same 0.251 s later, and
`look` ran another 0.251 s afterward. No busy messages occurred. This verifies
preservation of the remaining input through real kick lag; it does not claim
three successful kicks against a surviving target. No loot was taken.

## Code-level cause

The relevant symbols and line numbers below refer to the inspected HEAD.

1. [The input loop](../../src/net/comm.c), lines 1546–1559, selects the
   casting-specific dequeue only when `!CAN_ACT(t_ch)` **and** `AFF2_CASTING`
   are true (plus ordinary playing/no-pager/no-editor conditions).
2. [CAN_ACT](../../src/core/utils.h), line 628, only tests `PLR2_WAIT`.
   Once this bit clears, the loop chooses the normal playing dequeue even if
   `AFF2_CASTING` remains true. With no relevant transaction pending, that path
   removes the queue head through `get_from_q`.
3. [The interpreter](../../src/cmd/interp.c), lines 1557–1570, rejects every
   command except `abort`, `petition`, and `return` while `AFF2_CASTING` is set.
   It prints the busy message and returns without reinserting the input.

For an ordinary playing descriptor with no other transaction gate:

| Action wait | Casting | Non-whitelisted queued command |
|---|---|---|
| OFF | OFF | Dequeued and dispatched normally |
| ON | OFF | Preserved until recovery ends |
| ON | ON | Preserved; permitted casting commands can pass it |
| OFF | ON | **Dequeued, rejected, and lost** |

There are at least two ways to reach the last row:

- **Trusted immortals:** [CharWait](../../src/world/events.c), lines 411–418,
  sets `PLR2_WAIT` only for characters that are not trusted. Spellcasting still
  sets `AFF2_CASTING`. This is a direct, reproducible mismatch even without
  postulating a broken wait event.
- **Mortals whose spell outlasts their wait:** `do_cast` schedules a single
  wait using the initial cast duration, while `event_spellcast` completes the
  spell through a chain of callbacks, usually up to four pulses apart.
  The [spell continuation](../../src/net/sparser.c), lines 2649–2695, schedules
  each next step relative to the tick at which the previous step actually ran.
  Delayed steps therefore extend total chant time. The one-shot `event_wait`
  independently clears `PLR2_WAIT` without checking whether casting finished.

The game loop processes commands before its event pass. If the wait callback
runs but the completion callback remains pending, the next command pass can
consume input during that gap. [The scheduler](../../src/world/new_events.c)
orders work by due tick, priority, and sequence and can defer a due suffix once
its budget is exhausted. Player priority is not a guarantee of same-tick
completion, nor does it synchronize the wait and spell event chains.

An isolated ASan/UBSan probe extracted the actual casting-selection expression
and actual queue functions from source. It exercised all four rows above and
confirmed command removal specifically for casting-without-wait. Its character
state and outer dispatch were synthetic; it was not a full server simulation.
The live mortal tests supply the integrated reproduction missing from that probe.

### Production timing evidence and limits

A read-only scan of new status-log content, from shortly after authentication
through `23:27:43 UTC`, found 1,986 `NEVENT BUDGET` records. Across those records:

- Deferred events per record: 537–17,751.
- Maximum callback lateness reported per record: 1–11 pulses.
- Maximum deferral count reported per record: 1–11.

No `PLAYER EVENT TIMING` records were available in that interval. No new
`command gate: clearing stuck PLR2_WAIT` or
`CharWait: event_wait schedule failed` messages were found in the monitored logs.
The server's ordinary command loop remained responsive at roughly one command
per 0.25 s while these failures happened.

The mortal flag mismatch and command loss are confirmed. Existing scheduler
debt plus the independently timed event chains provide a supported explanation
for the mismatch. The exact due/execution ticks of the test character's wait
and cast callbacks were not captured, so this report does not attribute a
particular rejection to a specific deferred callback, assert that scheduler
debt is the only possible trigger, or diagnose the underlying source of the
world's event backlog. No global tracing was enabled to manufacture that evidence.

Both Telnet and gameplay WebSocket input normally enter the descriptor queue
([comm.c](../../src/net/comm.c), line 3795;
[websocket.c](../../src/net/websocket.c), lines 1723–1737). A WebSocket exposure
is therefore supported by the shared source path, but this investigation's
in-game reproductions used Telnet, not the browser client.

## Recommended correction and acceptance checks

Select the casting-specific queue based on the active casting state regardless
of `CAN_ACT`. Preserve the interpreter's restriction on overlapping actions,
the existing recovery lag, the transaction gates, and the ability of `abort`
to pass earlier blocked commands. A fixed extra delay or removing the busy
check would not establish the required queue invariant.

Before implementing this, decide explicitly whether trusted immortals should
also preserve type-ahead. The existing comments promise preservation during
casting, and the live immortal behavior currently contradicts that promise.
The smallest consistent correction applies preservation to both populations.

Required verification for a future fix:

1. Exercise all combinations of wait/casting state through the actual dispatch
   selection, including trusted characters and the transition where wait clears
   before casting does. Assert queue contents, not just message text.
2. Exercise a delayed spell continuation/completion independently of its wait
   event with a controlled scheduler. A queued cast, movement, and `look` must
   remain pending throughout the gap.
3. Verify `abort`, `petition`, and `return` can still pass blocked type-ahead;
   abort must preserve pending commands and its recovery delay.
4. Keep ordinary kick recovery, post-spell recovery, transaction-dependent
   commands, pager/editor handling, and successful completion behavior intact.
5. Audit failed spell-event scheduling/cancellation separately. Simply
   preserving input must not turn a genuinely orphaned casting flag into an
   indefinite silent queue. No orphaned cast was reproduced here.
6. Repeat the mortal Quickchant-OFF and Quickchant-ON live cases, the trusted
   control, and the kick control after an authorized deployment. Also verify
   the browser/WebSocket route.

## Verification performed

All of these passed against the inspected checkout:

```bash
python3 -B tests/async/test_casting_input_queue_runtime.py
python3 -B tests/async/test_spell_abort_command.py
python3 -B tests/async/test_command_gate_recovery.py
python3 -B tests/async/test_nevent_scheduler_runtime.py
python3 -B tests/async/test_player_event_timing_contract.py
python3 -B tests/async/test_nevent_budget_contract.py
python3 -B tests/async/test_documentation_contract.py
git diff --check
git diff --no-index --check /dev/null docs/ongoing-projects/spellcasting-queue.md
```

The queue runtime test validates selective extraction and list integrity. It
does not execute `game_loop`'s selection of that helper. The abort source test
actually asserts the current `!CAN_ACT && AFF2_CASTING` expression. The scheduler
runtime test passes its timing, priority, aging, catch-up, API, and integrity
scenarios under ASan/UBSan but stubs spell callbacks; it does not exercise the
real wait/casting/command interaction. Green results therefore do not refute
the production reproduction.

Additional investigation commands included `git status --short`,
`git rev-parse HEAD`, focused `git log`, `rg`/`sed`/`cat` source and guide reads,
`ps`, `/proc` executable/cgroup inspection, `stat`, `readlink`, `sha256sum`,
`nm -C`, offline `objdump`, `systemctl show` (system and user scopes), the local
HTTP health request, and Python scripts for bounded log summaries and the
temporary Telnet client. A final read-only `mysql` invocation returned only
counts for the newly created test character and its account mapping to verify
the unsuccessful cleanup described below. No credential or raw player-log output is included
in this document.

The temporary extracted-predicate probe was run with
`python3 -B /tmp/spellcasting-gate-probe.py` and passed under ASan/UBSan. Its
compiler reported only three unused helper-function warnings inherited from
the queue-test harness. It was an investigation tool, not a new checked-in
regression test.

No server build, full regression suite, database suite, migration verification,
or frontend/backend package checks were run: the requested deliverable changes
documentation only. The server bug remains unfixed.

## Cleanup and final checks

The configured immortal's Fog setting was restored to OFF and verified again
after reconnecting. Its class and learned skills were not modified. The self-cast
armor effect was left to expire normally.

The mortal exited through its normal `quit`/camp flow and returned to the account
menu. The subsequent account-menu deletion attempt reported success, but
verification showed a separate cleanup defect:

- `deleteCharacter(): failed to soft-delete pid <test>` appeared in the debug log.
- A read-only query found one matching `player_data` row and one still-active
  `account_characters` mapping.
- A fresh login still listed the disposable level-56 cleric on the test account.
  An immortal `where` lookup returned `Nothing found.`, consistent with the
  character having left the live world rather than remaining link-dead.

The temporary character therefore **remains on the test account, logged out**.
For operator cleanup it is the newly created human cleric with the synthetic
name `Queueprobe`; the original account and character identities are intentionally
omitted. No direct database writes were used to override the failed deletion.
The account deletion handler in [account.c](../../src/account/account.c), lines
2379–2382, ignores the result of `deleteCharacter` and prints success regardless.
That false success is confirmed; the underlying soft-delete failure was not
fully investigated or repaired as part of this spellcasting task.

The valid kick test killed one ordinary beginner NPC; its corpse was left for
normal world cleanup and no loot was taken. No test NPCs or objects were spawned.

The documentation contract passed all 12 checks, and `git diff --check` passed.
Because the report is newly untracked, it was also checked explicitly with
`git diff --no-index --check /dev/null docs/ongoing-projects/spellcasting-queue.md`.
An additional content check verified required findings, absence of configured
account credentials/identities and IP addresses, and removal of draft placeholders.
The only checkout change is this requested Markdown document. The findings and
recommended correction have not been applied to the server.

Both authenticated test connections ended at the account menu with `0`, the
goodbye message, and server EOF; the temporary client processes exited. After
a 30-second post-logout observation, the final check at `23:35:35 UTC` still
showed game PID `1476236`, supervisor PID `1475682`, `NRestarts=0`, an active
user service, and healthy/ready HTTP status. The user-service journal had no
matching crash/exit patterns in the inspected interval. New game logs had no
matching panic, corruption, sanitizer, wait-scheduling-failure, or wait-gate
self-heal messages. The one soft-delete failure above remains an explicit
cleanup exception. These checks are scoped observations, not a full production
stability qualification.
