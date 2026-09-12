# Generated pet recovery (#212)

Pet checkpoints now retain the generated summon, rather than reconstructing only
its area prototype. This builds on #232's shared ownership-aware loader and
exclusion of linked player pets from ordinary world recovery.

## Saved state and lifetime

Every necromancer/theurgist creation path records an explicit subtype before the
corpse transfer is admitted. A bounded, versioned payload preserves generated
name, short/long descriptions, rolled base stats, base HP/mana/vitality/armor/
hitroll/damroll/ward, damage dice, classes, race, level, alignment, size, intrinsic
affect flags, aggression, act flags, and undead spell slots. Equipment is restored
through the existing UID/ownership-aware item loader. Derived bonuses are
recomputed from base values, so equipment does not compound across restarts.

Charm and death use absolute Unix deadlines. Offline time counts toward expiry.
Restoring a finite pet cannot make it permanent because its owner acquired a
globe or innate afterward. Legitimately permanent pets retain zero deadlines.
Live charm effects retain the game's minute tick resolution; death is rescheduled
from the saved deadline at event resolution. Restore does not append the owner's
name a second time.

Explicit subtype costs are shared by restore admission and `count_undead`.
The normal level/specialization budget and the separate golem budget apply before
materialization. Restoration does not create additional pets beyond that budget.
Follower announcements are suppressed while either character is unplaced; this
avoids calling room visibility code during login staging.

## Retained records and equipment

When safe restoration cannot be established, the loader retains the complete pet
record and its item tree without creating a mobile or live copies of the items.
The owner receives a notice, and the server logs the owner's PID and held count.
These records participate in ownership reconciliation and every subsequent pet
component save, including quit and death. They are not automatically released on
a later login, even if the owner's capacity increases.

| Hold reason | Meaning |
|---|---|
| 1 | Legacy summon prototype with no generated state |
| 2 | Malformed, unknown-version, or incompatible generated state |
| 3 | Saved charm or death deadline has passed |
| 4 | Current owner capacity would be exceeded |
| 5 | Mobile prototype is unavailable |

Staff must review provenance and every equipment UID before deciding a
disposition. This change provides no automatic deletion, redistribution, or
release command. `docs/operations/pet-recovery-review.sql` is a read-only report
of held rows and legacy player-pet candidates after migration 0013.

Newly marked summoned instances are excluded from ordinary copyover/Redis world
capture even after their owner link disappears. Historic world records have no
reliable provenance: a matching prototype alone is insufficient evidence to
delete a mobile or its equipment. Recovery logs those candidates with VNUM,
room, and instance ID, and retains them for investigation. The historical count
of twelve dracoliches cannot be reconstructed from missing metadata.

## Deployment and compatibility

Run the normal migration runner through additive, guarded migration
`0013_pet_restore_state` before starting the new MariaDB/MySQL binary. It adds
nullable `restore_state` and default-zero `hold_reason` to `player_pets`; it does
not update or delete existing rows. Both supported engine metadata fingerprints
are measured from disposable schemas and sealed in the runtime contract.

Player checkpoint versions 3 (normal) and 4 (death) carry the new fields. The
decoder accepts legacy versions 1/2 and upgrades them in memory. Old summon rows
are retained for review because identity and absolute lifetime cannot be inferred.
The old binary cannot read the new checkpoint versions; do not roll back the
binary over newly written checkpoints without a compatible reader. Preserve
schema columns and checkpoint data when changing deployments.

## Local validation

Acceptance uses local builds and focused checks. CI status is not the acceptance
gate for this change.

- Build both `PERSISTENCE_BACKEND=mariadb` and `PERSISTENCE_BACKEND=flatfile`.
- Run `test_pet_restore_state.py`, `test_player_load_pets.py`, and
  `test_player_snapshot_capture.py` for codec compatibility, malformed/expired/
  capped retention, fixed subtype cost, and held equipment through quit/death.
- Run the focused save-worker, item-codec, world-recovery, copyover, flat-file
  repository, and runtime compatibility regressions.
- `test_pet_restart_journey.py /absolute/path/to/flatfile/server` boots a private
  real server, creates a real account/character, and seeds synthetic pet records
  offline using existing item UIDs. It checks repeated restart identity, base
  stats, a single application of equipment HP bonuses, absolute deadlines,
  finite versus permanent lifetime, and held equipment after quit/restart/save.
  Build its shared inspector first with
  `python3 tests/async/test_flatfile_player_repository.py --build-inspector bin/tests/coin-death-inspector`.
- `pet_repository_mysql_harness.cpp` exercises the production SQL pet writer and
  player loader on a disposable `pet_state_test` schema through migration 0013.
  It tests generated payloads, opaque held payloads (including SQL quoting), item
  affects, UID retention, and repeated replacement. Run separately on MariaDB
  10.11 and MySQL 8 using `TEST_DB_HOST` and `TEST_DB_PASSWORD`; it creates and
  removes only the synthetic PID/UID 212212. The schema must be disposable.
- Migration apply/replay and verifier pass on both supported database engines.

This does not repair #230's separate copyover transport/path problem. The pet
loader used by copyover is shared with the tested login/restart path; a successful
restart test does not establish that all live copyover transports work.
