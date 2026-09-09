# Transient starter gear disappears on reload

Investigated 2026-09-09 against checkout `ef692161f`, including starter-container fix `dabe03550`.

The investigation below records the pre-fix behavior. The implementation and deployment follow-up at the end records the subsequent correction.

## Finding

`ITEM_TRANSIENT` is **not** `ITEM_NORENT`. Transient equipment should retain its dissolve-on-drop behavior while surviving saves and reloads when otherwise eligible for persistence. The current code has an ownership gap: transient objects bypass the player creation grant, snapshots can still serialize them, and the login loader skips their saved payloads when there is no authoritative ownership record. This is a concrete code defect consistent with the report, rather than an intentional no-rent policy.

The attached transcript alone cannot prove which writes committed before that particular reboot. No affected-character records, incident logs, or deployed executable were examined. The diagnosis below establishes the mechanism in the current source; it does not claim a live reproduction of the incident.

## Reported symptoms

The supplied `pasted-text-1.txt` shows inventory falling from 16 to 11 items and equipment falling from 12 occupied slots to just the primary sword after a reboot. Five inventory objects disappear: the small round wooden shield, leggings, sleeves, leather sack, and dwarven hide. Eleven worn objects disappear. Weapons and four bandages remain.

Before the reboot, `put all sack` returned exactly:

> Nothing was put away; the container lacks authoritative ownership.

That message establishes that the sack lacked a runtime ownership entry when the bulk put command tried to use it. It is not a capacity or item-count error.

## Evidence and causal path

| Stage | Current source evidence | Consequence |
| --- | --- | --- |
| Flag semantics | [`defines.h`](../../src/core/defines.h), `ITEM_TRANSIENT` / `ITEM_NORENT`; [`handler.c`](../../src/world/handler.c), `obj_to_room()` near line 2712 | Transient schedules decay when placed in a room. No-rent is a separate flag. Their numeric masks are 524288 and 8388608 respectively. |
| Starter creation | [`nanny.c`](../../src/account/nanny.c), `load_obj_to_newbies()` near line 746 | Regular kit items outside the food, weapon, container, quiver, spellbook, light, and totem exceptions receive `ITEM_TRANSIENT`. Prototype flags also survive instantiation. |
| Ownership grant | [`handler.c`](../../src/world/handler.c), `obj_to_char()` near line 1853 | The grant guard explicitly excludes transient objects. They can become visible in inventory without the grant that establishes durable player custody. |
| Snapshot capture | [`player_snapshot_capture.c`](../../src/player/player_snapshot_capture.c), `capture_item_tree()` near line 301; player capture near line 633 | Capture omits `ITEM_NORENT`, not `ITEM_TRANSIENT`. Missing ownership emits an audit message but does not stop serialization. The row retains `extra_flags` and the object UID. |
| SQL save | [`player_snapshot_repository.c`](../../src/player/player_snapshot_repository.c), `insert_item_rows()` near line 327 and `apply_items()` near line 431 | The writer inserts the captured payload into `player_items`, including flags and UID. This replacement save does not establish `item_current_owner` custody for an ungranted item. |
| SQL reload | [`player_load_repository.c`](../../src/player/player_load_repository.c), `load_items()` near line 666 and `parse_item_payload()` near line 520 | Saved payloads are left-joined to `item_current_owner`. A missing ownership row returns `item_row_outcome::skipped` near line 621. There is no transient exemption. |
| Later save | `apply_items()` above | Once reload has omitted an item, a subsequent full item save replaces the old payload rows with the reduced live inventory. Remaining recovery evidence can therefore disappear. |

The same missing-ownership failure exists in the flat-file loader: [`flatfile_player_repository.c`](../../src/flatfile/flatfile_player_repository.c), `build_item_identities()` near line 160, skips payloads absent from active custody. Its first-snapshot baseline can establish ownership, but later ordinary snapshot updates do not repeat that baseline. New-player entry in `nanny.c` deliberately saves the baseline before granting the normal kit. The local configuration declares production with `mariadb-primary`; this investigation did not operate on that database.

### Why these particular items fit

The tracked [`heavens.obj`](../../areas/obj/heavens.obj) contains matching starter prototypes:

- Leggings 1102, boots 1103, gloves 1104, bracer 1105, cap 1106, and body armor 1107 already carry the transient bit in their prototype flags. This is not solely a flag added by the kit loader.
- The little round wooden shield 458 also already has that bit (`extra_flags=524296`). Although the supplemental shield is marked non-regular in [`newbie_kit_plan.c`](../../src/account/newbie_kit_plan.c), it still bypasses the ownership grant because of its inherited flag.
- The leather sack 612 starts without the transient bit. Before `dabe03550`, the regular-kit policy added it. Dwarven hide 570 is type 23 and remains subject to that policy.
- The long steel sword 1108 has no transient bit in its tracked prototype, and weapons are exempt from the kit's added transient flag. Bandages are supplemental non-regular kit entries. Their survival is consistent with taking the normal ownership-grant path.

These are source-template matches, not recovered UIDs or proof that the incident server used identical world data. The 16 total missing objects would also fit below the current loader's 32-row stale-item refusal threshold, allowing login with reduced gear.

## Existing sack fix and remaining defect

Commit `dabe03550` adds containers and quivers to the exceptions when assigning the starter transient flag. `test_newbie_kit_plan.py` verifies those exceptions. For the tracked sack template, newly issued sacks can now take the ownership-grant path and support durable contents.

That change neither registers already-created unowned sacks nor addresses other transient equipment. It also does not clear a transient flag inherited from a prototype. It is therefore a partial resolution of this report, not a general persistence fix.

[`actobj.c`](../../src/cmd/actobj.c), `uses_generic_item_ownership()` near line 145, already allows **owned transient objects** to participate in durable movement. The bulk put path near line 4937 emits the exact reported message when the target container lookup fails. This supports addressing ownership establishment while preserving the transient flag.

The earlier command failures are separate immediate constraints: `remove_item()` requires inventory count below the carry limit, so 16/10 prevents removing the sword; the wield path checks free hands, consistent with the displayed sword and shield. The kit loader publishes its selected items without a carry-count gate. These constraints make the unusable sack more disruptive but do not explain selective loss on reload.

## Recommended correction and verification

Preserve the distinction between transient and no-rent. Eligible transient gear needs authoritative ownership before player publication, while retaining dissolution on drop. Review the creation-grant exclusion and transient movement/destruction callers together; do not weaken login's custody checks or turn transient into a save-exclusion flag. Simply stopping the kit loader from setting transient would also miss the prototypes that already contain it and would change gameplay semantics.

Before implementing, cover these cases with focused runtime regressions:

1. Grant a normal starter kit containing prototype-transient armor, loader-marked transient items, a shield, weapons, bandages, and a sack. Confirm active player custody before publication.
2. Save, unload, and reload; confirm eligible transient inventory and equipment survive with the same UIDs, flags, equipment slots, and container relationships. Keep a separate no-rent control that remains excluded.
3. Exercise `put`, `put all`, get, equip/remove, and drop for owned transients. Confirm durable topology and that dropping still dissolves them without leaving active orphan custody.
4. Test grant failure and queue pressure for duplication or visible unowned items; verify both supported persistence backends as appropriate.

For the reported character, recovery requires preserved payloads/backups or other item evidence. Investigate before another full save where possible. Do not blindly create ownership for every orphan: an item may have moved to another owner or been destroyed. No recovery or data repair was performed here.

Useful diagnostic strings for a targeted incident investigation are `player_snapshot_capture: component=items outcome=unowned_object` (UID/vnum) and `player_load_materialize: component=items ... outcome=stale_rows_skipped` (PID/count). They are debug-log messages, so their absence would not disprove this mechanism.

## Validation and limits

Existing focused checks run successfully:

- `python3 tests/async/test_newbie_kit_plan.py`: 36,360 selections, 471 legacy table cells, pure preparation and container-policy contracts.
- `python3 tests/async/test_player_load_items.py`: synthetic item hydration and source contracts.
- `python3 tests/async/test_durable_container_put_contract.py`: 15 tests.
- `python3 tests/async/test_player_snapshot_capture.py`: capture contracts and native death-capture checks.

These passing checks do not cover an end-to-end transient starter grant/save/reload cycle. The existing MySQL harness explicitly expects an orphan payload to disappear on load (`tests/async/player_load_repository_mysql_harness.cpp`, near line 524), but it was inspected, not executed: its wrapper defaults to the production-configured `.env` and requires a separate local/test database. No server restart, production mutation, or live gameplay reproduction was performed. No C/C++ files changed, so no server build was required. This deliverable is an investigation only; the remaining defect is not fixed.

## Implementation and deployment follow-up — 2026-09-09

The player-publication guard in `src/world/handler.c::obj_to_char()` now includes transient items in the existing ownership grant. An unowned transient stays unpublished until the grant commits; failed grants use the existing cleanup. Money and player-corpse exceptions remain. Neither transient flags nor no-rent filtering changed. The existing movement paths already recognize active owned transient items, so no new queue, persistence format, schema, or alternate save path was added.

Scopeguard and plan-ablation kept the implementation to this publication boundary and existing tests. The runtime regression extracts the actual guard from `handler.c` and exercises the production grant queue, ownership runtime, capture, and codecs. It failed against the old guard at the missing-ownership assertion, then passed with the correction. It covers transient armor/shields/containers, commit-gated publication, duplicate completions, rejection cleanup, saved worn-slot/UID/flag preservation, and a separate transient-plus-no-rent exclusion control. The item-load regression additionally verifies restoration of owned transient gear in equipment and container contents.

Validation commands completed successfully:

```text
python3 tests/async/test_newbie_grant_lifecycle.py
python3 tests/async/test_player_load_items.py
python3 tests/async/test_orphan_item_session_regressions.py
python3 tests/async/test_bulk_drop_put_durable_chain.py
python3 tests/async/test_durable_container_put_contract.py
python3 tests/async/test_copyover_save_guards.py
python3 tests/async/test_newbie_kit_readiness_contract.py
python3 tests/async/test_live_item_movement_contract.py
python3 tests/async/test_newbie_kit_plan.py
python3 tests/async/test_bulk_put_partition_stability_contract.py
./scripts/format.sh
./scripts/format.sh --check
make -C src -j2 PERSISTENCE_BACKEND=mariadb BUILD_PROFILE=production
git diff --check
```

The requested production copyover ran at 11:05 UTC. Logs confirm promotion of `bin/server/dms_new` and restoration of both connected characters. The running executable and `bin/server/dms` both matched the rebuilt SHA-256:

```text
9ba5ec40f717cacce72da84e80ff6a5e09d962024a4304aa7b58cfff3bd4ea85
```

A controlled check on the configured staff test character saved a worn transient bronze cap (1106) and transient leggings (1102) inside a sack (612). Both restored visibly after copyover. Read-only SQL checks before and after verified identical item UIDs, the cap's equipment slot, the leggings' parent UID, active player ownership, and transient flags with no no-rent flag. A further save retained these relationships. The health endpoint returned `healthy` with persistence `ready`.

The live check used wizard-created fixtures, whose grant path already establishes ownership; the regression executing the changed guard proves the repaired publication behavior. A fresh live newbie-creation journey and a flat-file deployment were not exercised. Drop/decay behavior was left intact; the separate documented generic-extraction custody-retirement limitation was not redesigned. This fix prevents the missing-grant cause for newly published items; it does not reconstruct already lost gear or backfill items already unowned in a live inventory.
