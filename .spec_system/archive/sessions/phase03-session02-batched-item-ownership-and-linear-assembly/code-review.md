# Code Review

**Result**: RESOLVED
**Base Commit**: `c78bdefc17922a9a2c1775d016172417ecdeb0c5`

## Findings Repaired

1. **Critical - staged container cleanup could extract an object twice after a partial
   link failure.** Container compatibility is now validated for the complete graph
   before any link is mutated. Regression coverage constructs a valid first link
   followed by an invalid descendant and proves that all three staged objects are
   extracted exactly once.
2. **High - a new player without an `item_owner_revision` row was rejected even when
   both authoritative ownership and serialized inventory were empty.** The ownership
   summary now starts from a singleton row, reports revision zero when the row is
   absent, and accepts absence only when the authoritative owned count is zero. The
   guarded MySQL harness covers this never-owned state.
3. **High - legacy equipped-enchantment activation was lost when the normal SQL item
   reload was removed.** Snapshot-loaded equipment now performs the same post-room-entry
   enchant activation as the legacy equipment path, without restoring any synchronous
   item query or publication-time database work.
4. **Medium - the declared linear-operation ceiling did not account for bounded affect
   and description work.** Metadata operations are now counted and the per-item ceiling
   is sized for the explicit fixed metadata bounds. The 200-item regression asserts the
   resulting linear limit.
5. **Medium - allocation failures in metadata parsing, repository affect indexing, and
   runtime batch validation could escape or be classified as malformed input.** The
   spellbook parser is now allocation-free, item metadata returns an explicit allocation
   outcome, and allocating hash insertions are guarded and mapped to retryable/allocation
   failures.

No unresolved blocking finding remains. Direct snapshot equipment placement deliberately
does not call `artifact_update_location_sql`: Phase 02 current-owner state is authoritative,
and reintroducing that legacy call would violate this session's no post-publication item-I/O
boundary. Existing operator reconciliation remains available for legacy artifact state.

## Scope Reviewed

- Load DTO and repository: `src/player_load_repository.c` and
  `src/player_load_repository.h`.
- Game-thread validation and publication: `src/player_load_items.c`,
  `src/player_load_items.h`, and `src/player_load_materialize.c`.
- Atomic runtime ownership: `src/item_ownership_runtime.c` and
  `src/item_ownership_runtime.h`.
- Login and build integration: `src/Makefile`, `src/account.c`, `src/copyover.c`, and
  `src/nanny.c`.
- Regression coverage: `tests/async/test_player_load_items.py`,
  `tests/async/test_player_load_pipeline.py`,
  `tests/async/player_load_repository_mysql_harness.cpp`, and
  `tests/async/run_player_load_repository_mysql.sh`.
- Session records: `spec.md`, `tasks.md`, and `implementation-notes.md` in this session
  directory, plus the spec-state archival move generated during session planning.

The 126 archived Phase 00/01 session files were compared byte-for-byte with their paths
at the base commit; all matched, so the apparent deletions/additions are archival moves
only.

## Review Evidence

- `python3 tests/async/test_player_load_items.py` - passed.
- `python3 tests/async/test_player_load_pipeline.py` - passed.
- `bash tests/async/run_player_load_repository_mysql.sh` - passed against the guarded
  local development database using connection-local temporary item tables.
- `make -C src -j2` - passed with the warning-as-error C++20 profile.
- `./scripts/format.sh --check` - passed.
- `git diff --check` - passed.
- Archive integrity check - 126 files checked, 0 mismatches.
- `make test-all` - 199 passed, 0 failed; signal-handler harness passed.
