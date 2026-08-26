# Implementation Notes

**Session ID**: `phase00-session09-private-chest-password-hardening`
**Started**: 2026-08-27
**Last Updated**: 2026-08-27

## Session Progress

| Metric | Value |
|--------|-------|
| Tasks Completed | 16 / 16 |
| Estimated Remaining | Complete |
| Blockers | 0 |

## Implementation Summary

- Extracted the existing account bcrypt contract into a reentrant shared module using `crypt_gensalt_rn()` and `crypt_r()` with cost 12.
- Added strict bcrypt identification, constant-time bcrypt comparison, and constant-time legacy SHA-256 verification with temporary-buffer cleansing.
- Changed private-chest create and reset paths to hash in process before SQL escaping and storage.
- Added checked set/remove behavior, explicit no-password verification, and a 72-byte chest-secret limit with user-facing command feedback.
- Replaced SQL `SHA2()` verification with a stored-hash read and in-process bcrypt/legacy checks.
- Added conditional legacy upgrade and current-state re-verification when the compare-and-swap is stale.
- Confirmed the existing 64-character schema width fits bcrypt's self-versioned 60-character representation, so no migration was added.
- Added a standalone crypto runtime harness and focused lifecycle/schema contracts.

## Verification Evidence

- Standalone bcrypt/legacy runtime and source/schema contract: PASS.
- Locker result, locker pipeline, and persistence log hygiene regressions: PASS.
- `./scripts/format.sh --check`: PASS.
- `make -C src`: PASS with the C++20 warning-as-error profile.
- `make test-all`: PASS; 176/176 Python regressions plus signal-handler checks.
- `git diff --check`: PASS.

## Review Repair

Review rejected non-bcrypt backend failure markers, distinguished no-row password updates from success, added clear overlong-input feedback, and made a stale legacy upgrade re-verify current credential state before access.

## Scope Notes

- No migration was necessary or executed.
- No configured database, chest hash, plaintext password, credential, player/account data, or production system was read or changed during validation.
- Broader locker ownership and item transaction work remains in Phase 02.
