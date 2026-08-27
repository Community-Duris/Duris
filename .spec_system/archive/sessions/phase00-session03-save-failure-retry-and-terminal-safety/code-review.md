# Code Review: Save Failure Retry and Terminal Safety

**Reviewed**: 2026-08-27
**Base commit**: `4f49a11f`
**Result**: RESOLVED

## Scope

The complete diff from the recorded base commit was reviewed, including session state and documents, the deferred policy module, all player and locker terminal callers, copyover/shutdown control flow, focused regressions, and operator documentation.

## Findings

### Critical

None.

### High - resolved

1. Fresh deferred slots assigned `scheduled = 1` before calling the centralized scheduler. Because that scheduler rejects an already-scheduled slot, no initial callback was created and every new request could become permanently inert. The caller now leaves the zero-initialized marker clear and delegates the transition exclusively to `schedule_deferred_save_event()`. The regression isolates the fresh-slot block and rejects any pre-assignment.
2. The legacy locker leave fallback called `PFileToLocker()` under the mistaken assumption that it moved items onto the locker character. It actually moves them from that character back into the dynamic room; the following terminal save could serialize an empty locker and then free the room. Missing locker characters and failed snapshot preparation now veto departure before any occupant ejection or room release, retaining the complete live recovery source.

### Medium - resolved

1. Direct terminal failures without an existing deferred slot retained the character but owned no retry. The shared helper now queues a safe `RENT_CRASH` retry, including for named stat-dead PCs, and never schedules an inventory-extracting terminal type in the background.
2. Copyover cleared client close-on-exec state and announced progress while its state file was still being written. Both actions now occur only after the complete temporary file is closed and atomically published; all failure paths before that point leave transport state unchanged.
3. The global deferred flush still treated stat-dead PCs as missing even though the callback and terminal path can safely persist them. It now uses the same named-character validity rule and reports false only for a genuinely absent/invalid source.

### Low - resolved

1. Trusted quit, camp, inn, and heaven paths emitted successful departure copy before the save gate. Success copy now follows durability; failed camp/inn paths also restore locally changed home and tupor state where applicable.
2. The pwipe quiescence regression asserted the removed unconditional shutdown flush. It now verifies the boolean terminal gate precedes extraction and that worker shutdown remains after the game loop returns.
3. Non-ASCII punctuation already present in two touched source files was normalized to meet the session's ASCII review-surface requirement.

## Behavioral Review

- Fresh, coalesced, callback-failed, direct-flush-failed, and terminal-failed requests all have an explicit scheduling outcome. Successful clears make older queued callbacks harmless through PID lookup.
- Terminal helper failure restores any previous pending nonterminal type. A direct failure creates only a crash-save retry, preventing a later event from extracting inventory without completing the associated gameplay transition.
- Failed `writeCharacter()` restores equipment before affects are reapplied and leaves carrying links intact. Flat fallback success remains separately observable but returns false to terminal callers.
- Copyover and shutdown keep workers, descriptors, characters, and the current process available until every required persistence gate succeeds.
- Artifact and locker dummies remain in the live character list after a failed terminal save. Locker snapshot-preparation failure keeps the dynamic room and occupant in place.
- No reviewed alert includes credentials, SQL text, database errors, player/account identity, object descriptions, host values, or fallback paths.

## Verification Evidence

- Project analyzer: PASS; Session 03 is current and the recorded base matches.
- Focused policy and source-contract regressions: PASS.
- C++20 warning-as-error build: PASS.
- Full suite: PASS, 170/170 Python regressions plus signal-handler checks.
- Local development runtime login/save/persistence-status smoke: PASS; clean process exit.
- Formatting, whitespace, ASCII/LF/final-newline checks: PASS.

## Conclusion

All review findings are resolved. The implementation matches the session specification and is ready for validation.
