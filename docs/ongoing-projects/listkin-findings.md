# DurisMUD Master Review — Findings Extraction

> **Source:** `DurisMud_moshehbenavraham_master_review.pdf` (local-only review, dated 2026-08-29 UTC)
> **Target:** LuminariMUD / DurisMUD `master` @ `94d2c84f` (post flat-file restoration merge)
> **Overall posture:** CAUTIOUS — MariaDB remains production authority until the live matrix passes
> **Scope of this document:** all errors, warnings, bugs, issues, risks, and coverage gaps identified in the review
> **Remediation pass:** 2026-08-29 — every finding below was re-verified against the tree and addressed; see the per-finding *Resolution* sections and the Open Repair Checklist.

---

## Findings Overview

| ID | Title | Severity | Confidence | Status |
|----|-------|----------|------------|--------|
| F-01 | Boot-restore completeness | Medium (revised) — high residual scope | Source-confirmed on both heads | **Resolved** — shopkeeper restore wired fail-closed, CTF explicitly fenced, non-minimal full-world boot test added |
| F-02 | Account scalar data and identity membership can split | **High** — integrity risk | Reproduced with external verifier | **Resolved** — both after-images now publish through one authority transaction; regression test reproduces the old split |
| F-03 | Authority transaction image capacity < character-delete call surface | **High** — availability/data-retention risk | Source-confirmed | **Resolved** — cap raised to 32, compile-time contract check against the 17-site delete surface, maximal fixture added |
| F-04 | Backup is atomic per file, not a point-in-time authority snapshot | Medium — operational/data-recovery risk | Source-confirmed | **Resolved** — quiesced capture, generation manifest, restore script, restore drill test |
| F-05 | Full repository gate is not green | Medium — engineering-hygiene risk | Confirmed by full runner on both heads | **Resolved** — `./scripts/format.sh --all` leaves the tracked tree clean; `test_formatting_tooling.py` passes |

---

## F-01 — Boot-Restore Completeness

**Severity (revised):** Medium — high residual scope · **Confidence:** source-confirmed on both heads

### Original issue (first review, HEAD `eea489f0`)
- The mode gate permitted flatfile-primary boot while saved items and shopkeepers were **silently skipped** in client-free mode, with no boot failure — the core of the original release-blocking finding.

### Resolved by merge `94d2c84f`
- `restoreCorpses()` routes flatfile-primary to `flatfile_corpse_restore_catalog()` and treats every result other than `ok`/`not_found` as `fatal_boot_error("corpse", ...)`.
- `restoreSavedItems()` defers to the same corpse-staged shared room catalog.
- Live room item transfers made atomic; corpse decay, intentional destruction, unmaking, compaction, wall of bones, follower raising, player resurrection, and siege imports gained durable paths.

### Resolution (this pass)
- **Shopkeeper restore is connected:** `restore_shopkeepers()` (`src/files.c`) now routes flatfile-primary to `flatfile_shopkeeper_restore_catalog()` and raises `fatal_boot_error("shopkeeper", ...)` for every result other than `ok`/`not_found` — the same fail-closed contract as corpse restore.
- **Client-free CTF is explicitly fenced, not silently inert** (`src/ctf.c`): a `CTF_MUD` build without a database backend now fails boot with a stated reason, `do_ctf()` tells the player the system is unavailable, and `ctf_use_boon()` logs the fence instead of returning a bare `FALSE`.
- **Non-minimal full-world boot is now tested:** `tests/async/test_flatfile_full_world_boot.py` builds the client-free server, boots it against the full `areas/` world without `--minimal`, asserts the corpse and shopkeeper restoration stages actually execute, and requires a clean shutdown plus the same authority topology/permission checks as the minimal preflight. **Passing.**

---

## F-02 — Account Scalar Data and Identity Membership Can Split

**Severity:** High integrity risk · **Confidence:** reproduced with a temporary external verifier

### Defect
- `flatfile_account_state_save()` wrote the account file and updated the in-memory revision **before** calling `flatfile_identity_sync_account()`.
- These were **separate repositories, locks, and publications** — there was no single commit point, so an identity failure left the account scalar write published.

### Resolution
One commit point, no acknowledged half-write:
- `flatfile_authority_store` gained an `accounts` store (`root/identities/accounts`), so account files can travel in an authority transaction.
- `flatfile_account_prepare_save()` (plus a public `flatfile_account_lock`) encodes the account after-image under the account lock without publishing it.
- `flatfile_identity_prepare_sync_account()` encodes the membership after-image under the identity lock; the shared mutation logic is factored into `apply_account_sync()` so the direct and prepared paths cannot drift.
- `flatfile_account_state_save()` acquires identity → authority → account locks (matching the deletion path's order), prepares both images, and commits them with `flatfile_authority_transaction_commit_operations()`. The in-memory revision advances only after the transaction commits.

### Regression evidence
`tests/async/flatfile_account_membership_harness.cpp` now breaks the identity authority (the reviewer's fault injection: the names directory replaced by a regular file) and asserts the save fails **and** the account file keeps its old revision and email. Verified to fail on the pre-fix adapter (`account revision advanced despite a failed identity publication`) and pass on the fix.

---

## F-03 — Authority Transaction Image Capacity < Character-Delete Call Surface

**Severity:** High availability/data-retention risk · **Confidence:** source-confirmed

### Defect
- The authority transaction encoder rejected more than 16 images while character deletion has 17 reachable `append_operation` call sites (identity, player, item, locker, corpse/world-item, auction, artifact, ship, and related cleanup), so a fully populated character could exceed capacity even when every domain preparation succeeded.

### Resolution
- The cap is now a shared, documented constant: `flatfile_authority_transaction_maximum_operations = 32` in `flatfile_authority_transaction.h` (the wire format already carried a `uint16_t` count; raising the cap only widens what is accepted). The filename bound was raised to 192 bytes so a maximum-length hex-encoded account filename fits the accounts store.
- `flatfile_character_delete.c` declares `character_delete_maximum_operations = 17` next to an enumeration of the contributing domains and enforces the contract with a `static_assert` against the transaction cap — a new call site that outgrows the encoder now breaks the build instead of a live deletion. The operations vector reserves the same count.
- **Maximal fixture:** the deletion harness now also seeds a boon targeting the deleted character, so every domain of the contract stages an after-image (17 operations) in one transaction, and asserts the boon is released. Confirmed the fixture exceeds the old bound: with the cap forced back to 16 the contract assert fires (`the comparison reduces to '(17 <= 16)'`).

---

## F-04 — Backup Is Atomic per File, Not a Point-in-Time Authority Snapshot

**Severity:** Medium operational/data-recovery risk · **Confidence:** source-confirmed

### Defect
- `backup_pfiles.sh` recursively copied the state tree with no global quiescence barrier and no manifest of revisions captured at one authority point, so a live backup could mix an older account file, a newer identity catalog, and a pending transaction file.

### Resolution
- **Quiesced capture:** the flat-file branch of `scripts/backup_pfiles.sh` now `flock`s the same three publication locks the server uses — `identities/names/.identity.lock`, `domains/.critical-authority.lock`, `identities/accounts/.accounts.lock`, acquired in the server's own order — with a bounded wait (`FLATFILE_LOCK_WAIT`, default 120s). It fails rather than publishing a generation it could not quiesce, and it requires a provisioned authority topology.
- **Generation manifest:** every durable file is digested before and after the copy (lock files excluded). The backup is discarded if the source changed mid-copy or if the copy does not match, and a `MANIFEST.sha256` records the format version, generation id, capture time, source root, whether a pending authority transaction was captured, and the sha256 of every file.
- **Restore drill:** `scripts/restore_flatfile_backup.sh` verifies a generation against its manifest, refuses a non-empty target root, restores with 0700/0600 modes, re-verifies after the copy, and reports a captured pending transaction (which the server replays at boot).
- **Test:** `tests/async/test_flatfile_backup_manifest.py` runs the whole drill — manifest coverage, byte-for-byte capture, refusal while a writer holds the authority lock (with no partial generation left behind), a clean restore into an empty root, and rejection of a tampered generation. **Passing.**

---

## F-05 — Full Repository Gate

**Severity:** Medium engineering-hygiene risk

### Status
`./scripts/format.sh --all` reformats the whole tracked tree and leaves no changes beyond this branch's own edits; `./scripts/format.sh --check` and `python3 tests/async/test_formatting_tooling.py` both pass. The 28-file formatting backlog reported by the review is no longer present on this tree.

Remaining engineering-hygiene item (unchanged, policy not code): require build + full test + format status in CI.

---

## Additional Observations and Risks

- **Checksum is not authentication:** accepted as designed. The flat-file checksum detects corruption; it is not an authenticity mechanism against a process that can already write the state directory and recompute the digest. Defense stays with directory ownership/mode enforcement (0700 roots, owner-only locks) — no code change.
- **Future-dated trophy timestamps accepted:** verified as intentional parity, not a defect. The SQL rule is `TO_DAYS(NOW()) - TO_DAYS(timestamp) <= 7`, which also counts future-dated rows (the difference goes negative). `in_trophy_window()` now documents this parity, and the trophy harness pins it with a future-dated entry that must be counted.
- **History churn / provenance risk:** unchanged, process-level. Takeover should still begin from a clean provenance baseline and a small set of reproducible release commits.
- **Fail-closed corpse path not boot-proven:** closed by the new non-minimal full-world boot test (F-01).
- **Policy gap — merge-ready fence removed:** the concrete seams behind the fence (shopkeeper restore, CTF) are now closed or explicitly fenced, and a full-world flat boot is exercised in the suite. Keeping MariaDB as production primary remains a deliberate posture choice, not a blocked seam.

### Phase-level risks (from chronology)

| Phase | Resulting risk |
|-------|----------------|
| Aug 25–26: foundations | Broad surface area (warnings, build flags, event scheduling, command gates) makes regressions harder to isolate |
| Aug 27: durability & recovery | Concurrency and replay correctness become central risks |
| Aug 28: flat-file substrate | A second durable authority is introduced beside MariaDB |
| Aug 28–29: domain restoration | Cross-domain coverage and boot completeness remain critical |
| Aug 29: merge-ready policy | Flat-primary may boot before legacy/no-DB seams are complete |

---

## Test Evidence Gaps (What a Green Suite Does *Not* Prove)

| Evidence type | Proves | Remains unproven |
|---------------|--------|-------------------|
| Standalone flat repositories (C++ harnesses) | Encoding, bounds, checksums, permissions, revision conflicts, ownership/topology rules | Full runtime call ordering across every domain under player load |
| Source-contract tests | Selected routing/ordering decisions remain present in source | The code path executes under a live non-minimal server |
| Minimal boot preflight | Client-free build, isolated root provisioning, game-loop entry, clean shutdown | Gameplay behaviour after restoration |
| Full-world flat boot (new) | Non-minimal client-free boot reaches the game loop, runs the corpse/saved-item/shopkeeper restoration stages, and shuts down cleanly on an isolated state root | Restoration against a large *populated* production-scale state tree |
| Redis live tests | Ephemeral service behavior, fault classification, recovery, namespace/ACL boundaries | Redis combined with a live Duris world under production-like load |
| Backup drill (new) | Quiesced point-in-time capture, manifest coverage, restore verification, tamper rejection | A production-scale restore-and-boot rehearsal |
| Full regression (335/335) | The whole Python suite passes | DB schema scripts not run |

### Newest test `ec2c9432` (shop trophy history) does not prove
- A live `shopping_sell()` transaction with real player and keeper balances
- Item custody, history, and currency committing as one durable operation
- Restart recovery or backup consistency
- MariaDB behavioral parity beyond the counted window semantics
- (Runtime routing claims for `src/sql.c` / `src/shop.c` are source-text assertions, not a real buy/sell transcript)

---

## Verification Status Summary (this pass)

| Check | Result | Note |
|-------|--------|------|
| `make -C src` (MariaDB build) | Pass | Client-capable build links |
| Client-free flat build | Pass | Built by the boot tests |
| `test_flatfile_account_membership.py` | Pass | Includes the F-02 split-brain reproduction |
| `test_flatfile_account_repository.py` | Pass | Harness now links the authority transaction |
| `test_flatfile_authority_transaction.py` | Pass | Cap and encoder unchanged in behaviour below the bound |
| `test_flatfile_character_delete.py` | Pass | Maximal 17-operation fixture |
| `test_flatfile_shopkeeper_restore.py` | Pass | Restore contract |
| `test_flatfile_shop_trophy_history.py` | Pass | Future-dated parity pinned |
| `test_flatfile_full_world_boot.py` | Pass | Non-minimal full-world client-free boot |
| `test_flatfile_backup_manifest.py` | Pass | Point-in-time capture + restore drill |
| Changed-line format check | Pass | `./scripts/format.sh --check` |
| Full tracked-file formatting | Pass | `./scripts/format.sh --all` leaves the tree clean |
| Full Python regression | **335 passed / 0 failed** | `python3 tests/run_regression_tests.py`, including the two new tests |

---

## Open Repair Checklist (Condensed from Review)

1. ~~Connect `flatfile_shopkeeper_restore_catalog()` at boot with the same fatal-on-failure contract as corpse restore, and add its boot test (F-01).~~ **Done.**
2. ~~Decide explicitly whether client-free CTF is supported or fenced (F-01).~~ **Done — fenced, and it says so.**
3. ~~Replace account-then-identity publication with one transaction/recovery boundary (F-02).~~ **Done.**
4. ~~Raise or contract-check the 16-image authority cap against the delete surface (F-03).~~ **Done — cap 32, `static_assert` against the 17-site contract.**
5. ~~Add a maximal character-deletion fixture and a non-minimal full-world flat boot test (F-01/F-03).~~ **Done.**
6. ~~Create a point-in-time backup protocol with a generation manifest and restore drill (F-04).~~ **Done.**
7. ~~Repair the tracked-file formatting gate (F-05).~~ **Done** — remaining half (require build + full test + format status in CI) is a CI-configuration decision, outside this tree's code.
8. Keep MariaDB as production primary until a production-scale restore/restart rehearsal is run against real world data. *(Posture decision, deliberately left open.)*

---

*Extraction note: findings above are reproduced from the review document; each has since been independently re-verified against the tree and addressed as recorded in its Resolution section.*
