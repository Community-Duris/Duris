# Coin custody and death recovery

**Incomplete; not ready for deployment. Nothing deployed.**

## Implemented so far

- Committed in `5ee2b380c`: atomic wallet/pile coin transactions for MariaDB and
  flatfile, command/completion handling, persisted coin payloads, loader updates,
  and migration `0010_coin_custody_payload`. Covers pile creation, merge, partial
  pickup, retirement, overflow rejection, and operation-ID replay.
- Death snapshot capture, codec, and journal preservation code exists, but is
  preparatory: neither save backend applies death snapshots.
- Uncommitted edits in `src/world/handler.c`: convert the full wallet into a
  custodied pile through one coin transaction, removing the 32,000-per-denomination
  clamp and separate debit. Publish the pile after commit.
- Uncommitted edits in `src/combat/fight.c`: wait for currency completion before
  corpse handoff/extraction; send standalone coin piles through custody transfer.
- Uncommitted edit in `src/core/files.c`: exclude tracked piles from legacy corpse
  money totals to avoid counting their item payloads twice.
- Focused MariaDB/flatfile coin repository tests and coin publication tests passed.
  A wallet-conversion regression was added to `test_currency_input_queue.py`.
  The latest completion and corpse-total edits have not been retested.

## Remaining fixes and verification

- Finish rejected-corpse-transfer recovery. `EMSGSIZE` still leads to repeated
  transfer attempts. Preserve final death state, corpse identity/location, wallet,
  complete affected item payloads/UIDs, and disputed custody evidence durably outside
  active inventory; release to the account menu within **two seconds** and allow
  normal re-entry.
- Connect durable death disposition to both save backends and gameplay. Ensure
  retries/reconnects cannot repeat death consequences, wallet conversion, or asset
  disposition. Prevent a missing live corpse from allowing an empty save to discard
  untransferred assets. Handle wallet-conversion rejection without silently skipping it.
- Test database/journal write failure, preserving live state when neither can save.
- Verify the real sequence on isolated storage: **put → merge → get → move bag →
  die → reconnect**, including injected `EMSGSIZE`, restart/replay, capacity limits,
  exact money totals, pile UIDs, custody topology, and corpse contents.
- Complete formatting checks and the server build. The attempted
  `make -C src -j4` failed because the host lacks `cjson/cJSON.h`; no successful full
  build or measured death-recovery time is available.
