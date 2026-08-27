# Code Review: Private Chest Password Hardening

**Reviewed**: 2026-08-27
**Base commit**: `b0297c07`
**Result**: RESOLVED

## Scope

Reviewed the complete Session 09 diff: shared bcrypt extraction, account compatibility, chest creation/reset/removal, no-password behavior, bcrypt and legacy verification, upgrade concurrency, input bounds, schema width, diagnostics, tests, and session records.

## Findings

### Critical / High

None.

### Medium - resolved

1. `crypt_r()` can return a non-null failure marker. Hash creation now accepts output only when it passes strict 60-character bcrypt-format recognition.
2. A zero-row legacy compare-and-swap can mean another process changed the password after verification. That path now re-reads and verifies current state without attempting another upgrade, so a stale credential cannot authorize access.
3. Password set/removal initially treated a successful statement affecting no chest as success. New hashes require exactly one affected private row; removal verifies the explicit already-NULL private row before reporting success.

### Low - resolved

1. Overlong creation/reset input initially failed only inside SQL helpers with generic command text. Both commands now explain the 72-byte bcrypt limit before hashing.

## Behavioral Review

- New creation and reset values are independently salted bcrypt cost 12 strings before SQL construction.
- Plaintext passwords are absent from chest SQL, persistence logging, and diagnostics.
- Empty input authenticates only `NULL`; nonempty input never authenticates a no-password chest.
- Legacy 64-hex SHA-256 is verified in process with constant-time comparison and upgraded only after successful verification.
- Upgrade statement failure retains access for the credential verified at the read boundary and logs only a category; a stale conditional update requires a current-state recheck.
- Existing account bcrypt callers preserve their function contracts while gaining reentrant salt/hash buffers and constant-time comparison.

## Verification

- Standalone bcrypt and legacy SHA-256 runtime harness plus focused source/schema contracts: PASS.
- Locker result, async pipeline, persistence log hygiene, and related regressions: PASS.
- C++20 warning-as-error build, changed-line formatting, and whitespace checks: PASS.
- Full suite: PASS, 176/176 plus signal-handler checks.

## Conclusion

All findings are resolved. The implementation is ready for validation.
