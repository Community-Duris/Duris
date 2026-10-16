# Issue #110: NPC ship deletion and heartbeat recovery

Tracking: https://github.com/LuminariMUD/Duris/issues/110

## Review handoff (2026-10-16)

[PR #118](https://github.com/LuminariMUD/Duris/pull/118) contains the NPC deletion
occupied-interior and NPC-reference corrections on
`branch/issue-110-npc-ship-deletion`.
This review repair rebases it onto master
`9625ffd7bdad335b5c683d16ee080e971a443a3d`, incorporating 13 intervening commits.
[PR #121](https://github.com/LuminariMUD/Duris/pull/121) is superseded: both its
interior-cleanup and NPC-reference corrections are now included in #118 to
address the two review findings before #118 can be merged.

Leave #110 open for separately authorized production deployment and live
validation. No deployment, restart, migration, merge or release tag is part of
this review repair. Publication checks and broader local validation results
are recorded in the PR conversation; re-fetch refs before continuing.

## Outcome, non-goals, files, and proof

NPC sinking, staff hull purge and normal unload remove the runtime ship once
without attempting persistence. Preserve the 30-tick passenger delay and the
iterator-safe return after sinking; subsequent heartbeats resume later ships'
timers, repairs, movement and reload. Failed player persistence and ownerless
non-NPC deletion preserve the ship, its passengers and its references.

Before a successfully deleted ship releases its loaded rooms, the existing
`clear_ship_content` helper moves players and player corpses outside and removes
NPC crew and other objects. Failed loads skip this traversal because their
abstract room slots can be unassigned. Normal NPC unload uses this same deletion
boundary. Sinking retains its earlier cleanup for cargo/insurance processing.

The changes use `src/ships/ship_base.c`, `src/ships/ship_npc.c`, `src/ships/ships.h`, the existing
`tests/async/test_npc_ship_deletion.py` harness, and this handoff. No new helper,
registry, recovery policy or sweep redesign is needed. Ship-string ownership
and malformed-state recovery are outside this correction.

Proof executes production heartbeat, sinking, purge, unload, hash, runtime
identity, layout/content cleanup and shipyard-listing functions under ASan/UBSan
for SQL and flat-file builds. External world/persistence services are controlled
stubs, so these checks do not establish live-server behavior.

## Verified review regression

`python3 tests/async/test_npc_ship_deletion.py` passes nine scenarios per backend:
sinking, passenger delay, purge, unload, persistence failure/retry, ownerless
players (positive and zero durable IDs), occupied purge, NPC references/docking
and failed-load cleanup.
The occupied case includes a player, NPC crew, loose loot and a player corpse,
then reuses the released room for another ship and checks it is empty.

Before the review repair, occupied purge and successful player-deletion retries
failed their evacuation assertions in both backends. Afterward all cases pass.
A failed persistence attempt or ownerless guard retains the passenger in place;
only successful deletion evacuates them. Failed-load cleanup never traverses
unassigned rooms. No NPC deletion calls persistence, including stale positive
NPC durable IDs.

The original heartbeat investigation also reproduced the SQL NPC sinking stall.
Ordinary flat-file NPCs with `db_id == 0` already skipped persistence; do not
claim that ordinary case reproduced the SQL failure. The regression checks
later shipyard countdown display, repairs, movement and reload after removal.

## NPC references and consolidation

The second review finding reproduced on #118 after the interior repair: sinking
freed an escortee while another engaged NPC retained `escort` and `did_board`.
`ship_loss_on_sink` can dereference the stale escort during combat rewards.
The extended sinking and passenger-delay scenarios fail on that source for both
backends and pass with the reference correction. An engaged escort retains both
pointers during the delay; successful removal clears them while preserving its
unrelated combat target. The test also covers docking, unrelated references and
failed persistence.

The implementation carried from #121 clears matching NPC `escort` and
`did_board` pointers during deletion. Docking clears target locks but preserves
relationships with the still-live ship. An explicit deletion flag on the
existing reference-cleanup function distinguishes its two real callers; no new
registry or sweep is needed. The original #121 production ship files are
preserved unchanged. Its separate PR has no remaining implementation delta and
is superseded by #118.

## Continuation

Review and integrate #118 as a single PR. Do not replay or merge #121 separately.
Use the existing local checks, formatting and server builds for any changed
tree. Hosted CI latency is not a gate, and no review rule should be bypassed.

After separately authorized deployment, record the running commit and controlled
NPC sinking, the 30-tick passenger delay, staff purge cleanup and later shipyard
timer progression before closing #110. Ownerless non-NPC ships remain preserved
and logged for investigation.
