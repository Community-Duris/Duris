# Accounting qualification checkpoint — 2026-09-29

This is a partial, reproducibility-oriented checkpoint for the `finish-accounting`
branch. It is not release qualification, a live authority audit, or permission to
activate accounting. Incomplete gates are listed explicitly.

## Source and build identity

- Tested application source: `136592d82` (`Reject unrecoverable accounted quest
  rewards`).
- Current documentation/test-only follow-up: `993a12dbf`. It changes no C/C++
  sources, so the application sources are identical to the tested source above.
- Later implementation delta: `d9b8de7b8` adds frozen quest-credit metadata to
  continuation version 2. The focused codec/quest tests and a syntax-only quest
  translation-unit compile pass at this revision; maintained full server builds
  and the restart journey were not rerun for this delta.
- Build follow-up: `96eee63f1` corrects a missing aggregate initializer exposed
  by the strict full build. The complete local SQL server build then passed with
  GCC 12.3 and isolated output under `bin/gcc12`; the maintained GCC 13 and
  client-free flatfile builds remain unverified for the frozen-credit change.
- Quest progression follow-up: `9a4a4ebea` adds continuation-v3 admission-time
  skill eligibility and waits for the player-skills save revision before
  acknowledging a recovered skill reward. Focused quest and continuation tests
  pass, as does a full local GCC 12.3 SQL build. No process-restart journey or
  maintained GCC 13/flatfile build was run for this delta.
- SQL profile command: `make -C src -j4` in the maintained `duris-accounting-build`
  container. Compiler reported by that environment: GCC 13.3; the maintained
  SQL server build completed successfully.
- Flatfile profile: a separate client-free build using
  `PERSISTENCE_BACKEND=flatfile`, isolated `BIN_ROOT`, `OBJDIR`, `SERVER_BIN_DIR`
  and `DMS_BINARY` under `bin/quest-flatfile`, and `-D__NO_MYSQL__`; the build
  completed successfully. Its output was used by the restart journey below.
  Exact make arguments:

  ```sh
  make -C src PERSISTENCE_BACKEND=flatfile \
    BIN_ROOT=/opt/duris/bin/quest-flatfile \
    OBJDIR=/opt/duris/bin/quest-flatfile/objects/server/flatfile/development \
    SERVER_BIN_DIR=/opt/duris/bin/quest-flatfile/server \
    DMS_BINARY=/opt/duris/bin/quest-flatfile/server/dms_new -j4
  ```
- The container image tag was `duris-accounting-build`. The image digest and
  full compiler configure string were not captured at build time. Docker is not
  currently reachable from this workspace, so they cannot be recovered here.
- Server executable hashes were not captured. These are build evidence, not a
  frozen release binary.

## Schema and fixtures

- Migration `0046_economic_realized_trade_price` adds nullable realized-price
  storage for SQL shop trades. The migration manifest and runtime compatibility
  metadata were updated with it.
- Complete-schema replay through `0046` was previously run against disposable
  MariaDB 10.11 and MySQL 8.0.46 targets and passed. This is historical schema
  evidence; a replay was not repeated for this checkpoint.
- Quest crash fixture: generated miniature area, synthetic character, and three
  offerings (`acorn`, `branch`, `feather`) for one item-and-1,000-copper reward.
  It uses a client-free flatfile authority and temporary state/journal roots.
  No production database or player data was used.

## Commands and outcomes

| Check | Outcome | Limit |
| --- | --- | --- |
| `make -C src -j4` (maintained SQL container) | Passed | Build only; no SQL server gameplay restart. |
| Client-free flatfile `make` with isolated output directories | Passed | Compile does not establish SQL parity. |
| `tests/async/test_durable_quest_offering.py` | Passed | Focused synthetic regression; not an end-to-end SQL journey. |
| `tests/async/test_item_transfer_version_compatibility.py` at `d9b8de7b8` | Passed | Preserves v1 decode and checks v2 frozen-credit decode, malformed duplicate recipients, and item-transfer payload validation. |
| `tests/async/test_durable_quest_offering.py` at `9a4a4ebea` | Passed | Covers skill eligibility metadata, waiting for a durable player-skills save revision, and idempotent replay after the saved skill is present. |
| `tests/async/test_item_transfer_version_compatibility.py` at `9a4a4ebea` | Passed | Preserves v1/v2 decode, checks v3 skill flags, rejects reward flags on non-skill terms, and validates the item-transfer payload. |
| `g++ -std=c++20 -Isrc -I/usr/include/mysql -fsyntax-only src/world/quest.c` at `d9b8de7b8` | Passed | Syntax-only host check; not the maintained server build. |
| Isolated GCC 12.3 SQL `make` at `96eee63f1` | Passed | Full server build; custom outputs stayed under ignored `bin/gcc12`. The maintained GCC 13 container build and gameplay boot were not run. |
| Isolated GCC 12.3 SQL `make` at `9a4a4ebea` | Passed | Full server build after the quest-skill recovery change; outputs stayed under ignored `bin/gcc12`. |
| `tests/async/run_quest_reward_ack_crash.py` journey body with `expect_recovered=True` on the flatfile server | Passed | The existing test container invoked the Python journey body with the current flatfile binary and inspector. The runner's `__main__` inspector-build step was not invoked in that container. |
| `tests/async/test_player_save_pipeline.py` | Passed | Includes isolated revision/terminal harnesses and source contracts, not a live save/custody race. |
| `tests/async/test_player_save_worker.py` | Passed | Focused source/runtime harness. |
| `tests/async/test_bandage_custody_contract.py` | Passed | Source contract only. |
| `tests/async/test_terminal_save_safety.py` | Passed | Focused contracts; not a process-restart matrix. |
| `./scripts/format.sh --check` | Passed | The WSL helper printed stale Gitdir warnings before reporting formatting OK. |
| `git diff --check` | Passed | Checked at the `993a12dbf` working tree. |

The complete local SQL build command used at `96eee63f1` and `9a4a4ebea` was:

```sh
make -C src CC=g++-12 \
  BIN_ROOT=/mnt/c/Users/alexa/.codex/worktrees/2284/duris-persistence/bin/gcc12 \
  OBJDIR=/mnt/c/Users/alexa/.codex/worktrees/2284/duris-persistence/bin/gcc12/objects/server/mariadb/development \
  SERVER_BIN_DIR=/mnt/c/Users/alexa/.codex/worktrees/2284/duris-persistence/bin/gcc12/server \
  DMS_BINARY=/mnt/c/Users/alexa/.codex/worktrees/2284/duris-persistence/bin/gcc12/server/dms_new -j4
```

The focused quest recovery journey verifies that a process stopped at the
post-publication-ack callback restarts with consumed offerings, one item reward,
the expected wallet increment, and an acknowledged obligation in the synthetic
flatfile fixture. It does not prove typed SQL accounting publication.

## Open gates

- Capture the maintained image digest, compiler version/configuration, and
  executable hashes on the next reachable build.
- Run an end-to-end SQL server restart journey through an actual quest turn-in.
- Exercise crash-before-publication and save/custody races, including delayed
  and out-of-order save completion against a committed item operation.
- Eligible skill recovery now applies an idempotent skill-state assignment and
  waits for the player-skills save revision before acknowledging its obligation.
  Evidence is synthetic only; SQL/flatfile process-restart proof and the
  maintained GCC 13/flatfile builds remain open.
- A first idempotent quest XP receipt path is implemented for solo durable
  turn-ins. Admission freezes the XP award; player snapshot schema 11 carries
  the operation/index/amount receipt, SQL commits its mask with the player save,
  and recovery waits for that save revision before acknowledging the reward.
  Group XP remains refused, and old continuations without a frozen XP award stay
  pending. The focused synthetic retry test does not establish SQL restart proof.
- Spell-component destruction commands now carry a versioned fixed-endian
  envelope, stable effect ID, and bounded effect-specific context in the
  durable item-transfer continuation. Live dispatch uses the ID; replay remains
  retained until operation-scoped effect receipts prevent duplicate spell
  effects after a crash. Version 1 contexts use fixed-width little-endian fields;
  the decoder retains support for the initial unversioned scaffold. The stable critical
  operation ID now reaches each spell effect callback, including restored
  publications, so effect owners can key those receipts to the consuming
  command. The full isolated GCC 12 SQL build and focused context/payload,
  live-callback, and forced-drop regressions pass. No spell process-crash proof
  is claimed, and operation-scoped effect receipts remain unimplemented.
- Coin-transfer accounting now derives a stable lifecycle source event from
  the accounting lineage, debit account identity and expected revision, and
  lifecycle transition flags; an allocated destination UID is excluded.
  Successful SQL roots write and verify that source claim transactionally,
  allowing the unique source-claim key to reject a retry under a new critical
  operation ID and newly allocated destination UID. Duplicate-key errors now
  map to terminal `EEXIST` for typed coin roots, with a focused source-contract
  assertion. The
  GCC 12 coin-accounting and coin-item-accounting harnesses, isolated SQL build,
  and format check pass. The source-claim uniqueness path still needs a
  disposable SQL retry journey; no database test was run for this increment.
- Legacy newbie and CHAOS starter grants now carry a stable source ID derived
  from the character PID and grant kind through their bounded preparation queue
  into the batch command. This removes the allocated first-item UID as the sole
  dedupe identity for those per-character grants. Independent grants queued
  behind a preparing batch retain their own source metadata. The focused newbie
  grant lifecycle harness and 36,360-case newbie kit plan parity test pass, as
  do changed-line formatting and an isolated GCC 12 SQL build. No database or
  process-restart journey was run for these source identities.
- Quest continuation v5 and its admission code can freeze the XP amount for
  each credited recipient and XP reward, using the admission-time party and
  levels. The 768-byte callback context carries those levels across async
  publication. The focused quest-offering and continuation-compatibility
  checks pass, and the affected quest/item movement objects compile with GCC 12.
  Active-accounting group XP remains refused until recipient entitlements,
  per-recipient save receipts and login recovery exist. The full SQL build stops
  at the workspace's incompatible hiredis headers.
- Migration 0048 adds a per-recipient XP entitlement table. The SQL critical
  command transaction writes v5 multi-recipient awards beside the parent quest
  obligation, and retained-command verification checks the exact set. Solo v5
  XP continues through the existing mask receipt. The runtime table inventory
  and migration head are updated; runtime fingerprints still need disposable
  MySQL 8 and MariaDB 10.11 measurements. The worktree now includes pending
  entitlement reads, atomic player-save receipt updates, and login recovery,
  and an in-memory acknowledgement retry while other recipients remain
  pending. The changed SQL translation units pass a GCC 12 syntax-only compile and the
  immutable migration runner passes its 14 focused cases in WSL. Full server
  build and runtime qualification remain open. Group XP remains refused. No
  database engine was available for applying 0048.
- XP remains progression state outside the economic double-entry ledger.
  Quest XP receipts fence replay through player saves; ordinary `gain_exp`
  calls and other progression mutations are not economic postings.
- Copyover format version 17 serializes connected death-retry state (corpse UID
  and bounded delay) and restores the private hold before resuming the retry.
  Versions 12-16 retain their prior descriptor record size. A preflight still
  cancels before persistence drains if a pending retry has no eligible preserved
  descriptor or invalid metadata. The focused copyover contract, changed-line
  formatting, production copyover save/exec/recover custody harness, isolated
  GCC 12 SQL and flatfile builds, and the real flatfile socket journey pass. The
  journey confirms actual exec, the original socket, MCCP, and an acknowledged
  post-copyover save. Its runner preserves optional `LD_LIBRARY_PATH` inside the
  isolated fixture. Neither runtime journey seeds a pending death retry, so
  that specific recovery path, legacy-retry policy and the full death/corpse
  matrix remain open.
- Reanchored 86 writer registry locations after the implementation edits. The
  `(path, family, excerpt)` census is unchanged: 2,804 occurrences, 2,746
  unique sites, zero unmapped. `validate_economy_accounting.py` and the
  generated-matrix `--check` pass; `--release` still fails with
  `writer has no executable evidence`.
- Run SQL migration 0048 on disposable MySQL 8 and MariaDB 10.11 databases,
  measure both normalized schema fingerprints, then update the compatibility
  contract. The SQL engine is unavailable in this workspace, so the checked-in
  fingerprints still describe pre-0048 schema and runtime compatibility must
  fail closed until they are measured. Run maintained SQL/flatfile builds and
  process-restart journeys after the XP receipt change; local GCC 12 is the
  available build path.
- Complete death/corpse journeys, ordinary item/money qualification, writer
  evidence, independent native reconciliation, activation refusal/success
  proofs, and flatfile parity as defined in
  `FINISH_ACCOUNTING_PLAN.md`.

**Checkpoint status: partial evidence only; SQL stable-gameplay candidate and
accounting release remain unqualified.**
