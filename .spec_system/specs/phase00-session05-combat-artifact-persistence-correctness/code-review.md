# Code Review: Combat and Artifact Persistence Correctness

**Reviewed**: 2026-08-27
**Base commit**: `20e13a1a6916102914c945647b0d211af25ad9b1`
**Result**: RESOLVED

## Scope

Reviewed the complete Session 05 diff: frag mutation/publication ordering, bind lookup API and parsing, all direct callers, focused regressions, and session records.

## Findings

### Critical / High / Medium

None.

### Low - resolved

1. The first real implementation checked for either null output pointer before initializing the other provided pointer. Both the real and no-MySQL implementations now initialize each provided output independently before rejecting an incomplete pair.
2. The first source contract detected failure logs but did not isolate the corresponding false return. It now isolates query, allocation, and malformed-row branches and requires explicit failure status, plus successful-row publication ordering and no-MySQL defaults.

## Behavioral Review

- The loss formula, cast behavior, cache invalidation, boon checks, and player-visible message remain unchanged; only mutation/write order moved.
- Bind no-row remains normal unbound state (`owner_pid = 0`, `timer = 0`, true).
- Database and malformed-data failures retain defaults but return false, and all callers check that status before ownership decisions.
- Parsed values are held in locals until both columns validate, preventing partial publication.

## Verification

- Focused and nearest regressions: PASS.
- C++20 warning-as-error build and formatting: PASS.
- Full suite: PASS, 172/172 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation.
