# First corpse without a world-item catalog (#359)

New corpse creation used to return `not_found` when no world-item catalog had
been written yet. A player could remain held in deferred death finalization.
The room-transfer path already established the catalog on first creation, so
journeys that dropped or looted objects before dying concealed the defect.

The fix allows initial corpse lifecycle upsert and player-to-corpse transfer
with expected revision zero to prepare an empty catalog in the existing locked
authority transaction. Missing updates/loot, malformed state and corrupt
catalogs remain errors. No separate, non-atomic initialization write is added.

The regression removes artificial catalog pre-initialization. Unchanged master
`767e66e2b` fails with `first corpse transfer did not prepare establishment`.
The candidate passes world-item, corpse lifecycle and item ownership tests,
including interrupted first commit, replay, exact item/metadata retention,
missing update/loot and corrupt-catalog rejection. The production corpse boot
restore regression also passes. Both backend builds and changed-line formatting
pass.

The player journey in `run_lethal_floor_journey.py` uses real Telnet accounts,
starting equipment and a real ice-floor impact for the first death. It asserts
that the world-item catalog is absent before impact and checks account-menu
release, the corpse and intact floor, re-entry, actual corpse looting, save,
restart and retained recovered item IDs/death count. Normal carry limits apply;
the fixture recovers a subset of its 27 starting items and verifies that subset
exactly. It does not claim to haul every item at once.

Minimal mode deliberately skips `restoreCorpses()` at boot, so the live journey
loots the corpse before restarting. The separate production corpse-restore test
covers materialization from the catalog. This distinction avoids claiming a
full-world corpse reboot journey from a minimal-mode fixture.

Commands (Linux, from the repository root):

```sh
python3 tests/async/test_flatfile_world_item_repository.py
python3 tests/async/test_flatfile_corpse_repository.py
python3 tests/async/test_flatfile_item_repository.py
python3 tests/async/test_flatfile_corpse_restore.py
python3 tests/async/run_lethal_floor_journey.py /absolute/server lethal
```

The same journey also validates #342. That PR changes post-lethal continuation
and narration, while this fix supplies the independently missing first-corpse
initialization. Neither needs production data to reproduce. No production
mutation or GitHub CI result is part of the local acceptance evidence.
