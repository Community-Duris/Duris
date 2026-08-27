# Session Specification

**Session ID**: `phase00-session09-private-chest-password-hardening`
**Phase**: 00 - Correctness and Immediate Lag Removal
**Status**: Complete
**Created**: 2026-08-27
**Base Commit**: `b0297c07`
**Work Window**: Private-chest password creation, reset, verification, legacy upgrade, input bounds, and storage compatibility.

---

## 1. Session Overview

Private chests currently send plaintext passwords into SQL `SHA2()` expressions and store deterministic unsalted SHA-256 values. Creation, reset, and open each own part of the secret lifecycle. Duris already uses bcrypt cost 12 for accounts, and its 60-character self-versioned encoding fits the chest column's existing `VARCHAR(64)`.

## 2. Objectives

1. Reuse a thread-safe bcrypt primitive with unique salts and cost 12 for all new chest hashes.
2. Keep plaintext chest passwords out of SQL and persistence diagnostics.
3. Verify bcrypt in process and recognize legacy 64-hex SHA-256 values with a constant-time comparison.
4. Upgrade a valid legacy row to bcrypt without overwriting a concurrently changed credential.
5. Preserve explicit no-password behavior and reject inputs beyond bcrypt's 72-byte boundary.

## 3. Scope

### In Scope

- A narrow reusable password-hash module and existing account callers.
- Private-chest create, password change/removal, and open verification paths.
- Existing schema/bootstraps only where a compatibility correction is necessary.
- Focused source and standalone runtime hash regressions plus the full suite.

### Outside This Work Window

- Account authentication policy redesign, locker ownership redesign, chest-item transactions, production migration execution, or reading/exporting existing hashes.

## 4. Technical Approach

Move the existing bcrypt helpers from `account.c` into a small module using `crypt_gensalt_rn()` and `crypt_r()`. Expose bcrypt identification and legacy SHA-256 verification without logging inputs. Hash chest passwords before SQL escaping, and query the stored hash for in-process verification. On successful legacy verification, generate a fresh bcrypt hash and issue a compare-and-swap update against the exact legacy value. A failed or stale upgrade does not expose the secret and does not silently reject a password that was valid at the read boundary. Empty input verifies only a `NULL` hash. Chest inputs over 72 bytes fail before hashing or SQL.

## 5. Deliverables

| File | Change |
|------|--------|
| `src/password_hash.h`, `src/password_hash.c`, `src/Makefile` | Thread-safe bcrypt and legacy SHA-256 primitives |
| `src/account.c`, `src/ws_handlers.c` | Consume the shared helper without ad hoc extern declarations |
| `src/sql_player.h`, `src/sql_player.c` | Hash create/reset values, verify in process, and upgrade legacy rows |
| `src/storage_lockers.c` | Route password changes/removal through the checked SQL helper |
| `tests/async/test_private_chest_password_hardening.py` | Source, runtime, uniqueness, bounds, legacy, and schema contracts |

## 6. Success Criteria

- [x] New and reset hashes are independently salted bcrypt cost 12 values.
- [x] Plaintext password values never enter chest SQL, logs, or error text.
- [x] Correct bcrypt and legacy passwords pass; incorrect and overlong passwords fail.
- [x] Successful legacy verification attempts a conditional bcrypt upgrade.
- [x] Empty input succeeds only for a chest with no password.
- [x] Existing `VARCHAR(64)` definitions remain sufficient and consistent; no migration is needed.
- [x] Focused tests, formatting, C++20 build, and full regression suite pass.

## 7. Risks And Resolutions

- **Bcrypt truncation**: reject chest secrets longer than 72 bytes instead of accepting aliases.
- **Upgrade race**: condition the update on the exact legacy hash so it cannot overwrite a newer password.
- **Legacy lockout**: authenticate a successfully verified legacy value even if its best-effort upgrade fails, while recording only a categorical failure.
- **Static crypt buffers**: use reentrant salt generation and hashing in the shared helper.

## Next Steps

Session complete. Continue with `phase00-session10-security-policy-and-dependency-baseline`.
