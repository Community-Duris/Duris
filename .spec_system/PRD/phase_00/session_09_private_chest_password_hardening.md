# Session 09: Private Chest Password Hardening

**Session ID**: `phase00-session09-private-chest-password-hardening`
**Status**: Not Started
**Work Window**: The complete private-chest secret lifecycle from create and reset
through verification, legacy recognition, upgrade, and schema compatibility.

---

## Objective

Replace unsalted SHA-256 private-chest passwords with versioned adaptive salted hashes
and preserve access through a tested legacy upgrade or explicit reset path.

---

## Scope

### In Scope (MVP)

- Generate adaptive salted hashes for new chest creation and password changes using the
  repository's vetted password-hashing facilities or a narrow shared helper.
- Verify adaptive hashes without sending plaintext passwords or hashes to SQL logs.
- Recognize legacy SHA-256 values safely and upgrade them only after successful
  verification, or require an explicit authenticated reset when automatic upgrade is
  not safe.
- Add an additive, guarded, re-runnable migration only if the existing column cannot
  hold the selected versioned format.
- Add focused regressions for unique salts, correct and incorrect passwords, no-password
  chests, legacy transition, maximum input handling, and failure behavior.

### Out of Scope

- Broader locker ownership, item transaction, or account authentication redesign.
- Running any migration or password write test against production.
- Logging or exporting existing password hashes.

---

## Prerequisites

- [ ] Session 08 connection and logging boundaries are validated.
- [ ] Any schema test runs against an isolated development database.

---

## Deliverables

1. Versioned password hash and verification helpers in the nearest shared authentication
   or locker module.
2. Updated chest create, password-change, and open flows in `src/sql_player.c` and
   `src/storage_lockers.c`.
3. Additive migration material under `migrations/` only if required by the selected
   hash representation.
4. Focused source-contract and isolated runtime regressions under `tests/async/`.

---

## Success Criteria

- [ ] New and reset chest passwords use unique salts and an adaptive work factor.
- [ ] Correct passwords succeed, incorrect passwords fail, and comparisons do not leak
      raw secret or hash values through logs.
- [ ] Legacy values have a tested successful-verification upgrade or authenticated
      reset path that does not lock out valid owners silently.
- [ ] Empty-password behavior remains explicit and compatible.
- [ ] Focused regressions, formatting checks, and `make -C src` pass.
