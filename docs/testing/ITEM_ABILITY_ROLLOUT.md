# Item ability delivery verification — 2026-09-13

[#297](https://github.com/Community-Duris/Duris/issues/297) qualifies the initial roster in the [rollout runbook](../operations/ITEM_ABILITY_ROLLOUT.md). Tests used disposable Linux processes, synthetic accounts, localhost ports and separate flatfile-primary state/journal directories. No production accounts, database, world state or deployment were used. The [machine-readable evidence](item-ability-rollout-evidence.json) contains aggregate results and binary hashes, with item UIDs and settlement timestamps removed.

## Real-server encounters

These journeys drive the actual account/character parser, command dispatcher, combat loop, item callbacks, scheduler, native spells and persistence worker over TCP. They use a deliberately small generated world. Player HP and fixture weapon dice keep combatants alive long enough to observe selection; recorded delays distinguish random selection from warning-to-effect time.

| Scenario | Observed result |
| --- | --- |
| Packed weapon, disabled/enabled | Legacy selection reached damage after 9.259 s of combat; enabled warning preceded damage by 2.002 s. The first number includes random selection and is not legacy spell latency. |
| Packed weapon reaction | Target left the room in 0.250 s after warning; fizzle observed, no delayed damage. |
| Avernus, disabled/enabled | Original 1/25 selection retained. Enabled warning-to-drain was 2.002 s. Legacy combat-to-first-drain was 126.386 s, not a payload-latency measurement. |
| Avernus reaction | Target fled in 0.501 s; pending drain/healing cancelled, no post-warning damage. The fixture handles the native stand-up-first flee behavior and initial combat fumbles. |
| Wand/staff/scroll | All nine off/complete/abort cases passed. New warning-to-damage was 2.252–2.253 s, abort 0.250 s. Charge or scroll consumption remained committed after abort. Disabled command-to-effect was 0.014–0.286 s. |
| Tsunami native tap | Actual 31514 template on a cleric. Legacy immediate vitality; new effect after 2.252 s; abort after 0.250 s. A 2 MP debit remained after cancellation. |
| Tsunami restart/death/loot | Same physical UID and ledger record after save/restart. Native five-minute tap cooldown prevented restart reuse. Actual player death, corpse transfer and loot preserved UID and mana. |
| Studio native binding | Real `T CMD use` / `itemability 1002` `.trg`, actual catalog and native magic missile. Warning-to-effect 2.251 s; abort 0.250 s. No effect after abort. |
| Studio bag and room transfer | Put in carried bag, save/restart, retrieve from bag, drop/get in empty refuge. Same UID, unchanged durable mana; projected reserve increased only by elapsed regeneration. |
| Studio SIGKILL | Kill the owned server during windup after observing the durable debit. Restart restored that debit exactly and no pending effect. A later ordinary save/restart preserved it again. |
| Mirrored ioun | Actual 922 template worn in the ioun slot. A real incoming native spell took the synchronous deflection path and advanced its mana ledger. Combat-to-first-paid-interception was 146.389 s; this includes random selection/regeneration and is not interception delay. |
| Live operator controls | Actual trusted operator commands and separate mortal actor exercise master/category/mana cancellation, re-enable, malformed numeric settings, versioned catalog reload, invalid catalog retention, aggregate metrics and mortal access exclusion. |
| Two equipped weapons | Verify both NPC equipment names, then observe both packed sources warn and release native effects in the same encounter (60.566 s to observe both). The fixture uses a six-second windup. |
| Normal caster with own passive weapon | A mortal level-56 MindFlayer completed an ordinary mana-based `will` cast in 4.755 s while its own pending weapon released native magic missile. Quickchant was disabled. A six-second item windup lets the test overlap the normal attack-command wait with casting; no trusted casting bypass is used. |

The Studio/native journeys use area-debugger boot (`-z`) to load actual Studio bindings without optional full-world subsystems. That mode deliberately skips saved floor/corpse bootstrap. Bag/player restoration is real; post-restart floor transfers use a previously empty refuge, and the corpse is created/looted in the same live session. Full flat-file corpse/floor boot materialization has a separate production-code harness. These journeys are not a full generated-world production startup test.

The matrix uses multiple binary hashes because it was executed as prerequisite slices landed. Later changes fixed the real Studio command-number parser, artifact ownership during death and the mana-gate cancellation barrier. The last two defects were reproduced and their affected journeys rerun with the fixes. Unrelated earlier family measurements are retained with their original hashes rather than relabeled as a single final binary.

The caster overlap case reuses the existing racial spellcast journey's synthetic Chaos creation settings to obtain a mortal level-56 MindFlayer and its real mana-based casting path without spellbook setup. It still uses ordinary `will` parsing, resource consumption, casting events and completion. Other journeys run with Chaos disabled. The dual-weapon network case observes both sources in one encounter; the deterministic native fixture separately asserts that two reservations coexist at once.

## Sustained PvE followed by PvP

`run_item_mana_encounter_journey.py` creates two real player accounts and runs the same Studio prism against an NPC for at least 60 seconds before targeting the other player. The test profile is **10 MP capacity, 3 MP cost, 0.25 MP/s regeneration**, initially allowed to regenerate from empty to full. These are synthetic fixture values.

There were **17 PvE attempts, eight completed abilities**. Immediate hostile-player reserve was **1 MP** and activation was suppressed without a warning or spell. After **7.009 s** of additional recharge it had **3 MP**, admitted a fresh player-targeted action and delivered native damage **2.330 s after warning**. The final committed reserve was zero. The JSON records every attempt's observed starting reserve and actual start/completion result. This demonstrates depletion carrying into a hostile encounter and recharge recovery; it does not establish universal artifact DPS or production balance.

The independent native Mayhem/Symmetry fixture uses controlled selected powers: 10 MP initial, 15/20 completed selections over 60 s, 1 MP remaining, immediate 2 MP power rejected, and one accepted after ten seconds of recharge. This is a forced native callback test, not another networked PvP encounter.

## Deterministic native and fault coverage

The action C++ fixtures compile production runtime, scheduler, adapters and relevant parser/effect entry points with ASan/UBSan. Persistence/restore harnesses also compile production code with warnings as errors; they are not all sanitizer builds. World/spell probes and forced RNG make lifetime and selection cases reproducible. They complement the networked encounters; a probe invocation is not evidence of a landed native spell.

| Contract | Reproducible fixture |
| --- | --- |
| Two weapons; source/wielder/active caps; ordinary casting survives passive actions | `test_item_actions_runtime.py`, `test_weapon_actions_runtime.py` |
| Target/source extraction, leave-return, re-equip, between-effect invalidation; vetoed departure | Foundation and weapon runtime tests |
| Active input queue, abort, retained charge, consumed scroll, original scroll slot order | `test_device_actions_runtime.py` plus real device journeys |
| Staff group/area/ignore filtering and revalidation | Device runtime test |
| All 20 wonder outcomes, captured RNG and native effect lifetime | `test_wonder_actions_runtime.py` |
| Actual Studio event parser, original actor/victim identities, prior native-special precedence, cooldown and reload | `test_studio_abilities_runtime.py` |
| Editor schema round-trip, duplicate/unknown fields and 29 invalid cases | `test_studio_ability_model.py`; public editor stub |
| Ioun incoming damage, Tsunami terrain/groups, necroplasm source-linked form | `test_native_artifact_runtime.py` |
| Sword state cursor, native effects, challenge/flurry/group legality, nova minimum/global-cap rejection | `test_sword_actions_runtime.py` |
| Empty enrollment, independent UIDs, clone denial, conservation floor, no refill on flags/profile changes | `test_artifact_mana_game.py`, `test_artifact_mana_runtime.py` |
| Lost ACK, CAS retry, coalesced spends, stale-worker two-second admission bound | Mana runtime/store test |
| Retained depletion accepted by restore qualifier; corrupt/relabeled authority rejected | `test_artifact_mana_restore.py` |
| Persistent artifact owner retained during death's unequip-to-inventory step | `test_flatfile_artifact_repository.py` |
| Native corpse/floor bootstrap; custody/snapshot recovery | `test_flatfile_corpse_restore.py`, `test_flatfile_player_repository.py` |

The mana ledger is independent of player, bag, locker and corpse payloads. Networked bag/death tests cover representative custody transitions; locker behavior is supported by the existing custody repository and UID contract, not a claim of a new networked locker UI scenario. Private DurisStudio UI integration is explicitly handed off to Faemill in #329.

## Performance evidence and bounds

`run_item_action_benchmark.py` uses the production scheduler/runtime with no-op payload probes and real monotonic timing, compiled with `g++ -O2` without sanitizers. Its accelerated clock drives logical scheduler timing only; the measured wall-clock work is real. The fixture is not the normal `-Og` server build and is not a capacity benchmark for full native spells.

| Simultaneous pending | Admission total, telemetry off/on | Release total, off/on |
| ---: | ---: | ---: |
| 128 | 105 / 73 us | 93 / 84 us |
| 1,024 | 4,865 / 6,954 us | 6,289 / 6,717 us |
| 4,096 | 153,584 / 169,658 us | 170,896 / 153,973 us |

At the 4,096 bound the largest observed individual callback was 843 us and the synchronized release consumed most of a 250 ms pulse. The exact production Studio expiry loop over 65,536 entries took 305 us when unexpired and 887 us when expired. These single samples justify the conservative 128-pending first-wave proposal and further whole-server measurement; differences between off/on samples are noise, not an asserted speedup or overhead estimate.

The live configuration journey separately observed two timed callbacks totaling 9,047 us, maximum 9,031 us, including a real native spell. This is much larger than no-op probe work and near the runbook's proposed 10 ms investigation gate. An idle comparable-host baseline and the staged observation interval are still required before expanding a production wave.

## Reproduction

Use a disposable Linux checkout/container with the dependencies in the repository README. All generated binaries, raw logs and temporary evidence belong under ignored `bin/`; the journeys create and remove only their own temporary state. Never point them at production state. An optional `DURIS_KEEP_ITEM_FAILURE=1` preserves a synthetic failing fixture under `bin/pilot-failure-*` for debugging.

```sh
make -C src -j4 PERSISTENCE_BACKEND=flatfile DMS_BINARY="$PWD/bin/server/dms_item_actions"
python3 tests/async/test_flatfile_player_repository.py --build-inspector bin/tests/item-pilot-inspector
python3 tests/async/run_weapon_actions_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector --family packed
python3 tests/async/run_weapon_actions_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector --family avernus
python3 tests/async/run_device_actions_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector
python3 tests/async/run_item_pilot_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector --family tsunami
python3 tests/async/run_item_pilot_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector --family studio
python3 tests/async/run_item_pilot_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector --family ioun
python3 tests/async/run_item_mana_encounter_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector
python3 tests/async/run_item_config_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector
python3 tests/async/run_item_overlap_journey.py --binary bin/server/dms_item_actions --inspector bin/tests/item-pilot-inspector
python3 tests/async/run_item_action_benchmark.py > bin/item-action-benchmark.json
```

Run the individual focused Python tests listed above directly; they build their native fixtures. Both `make -C src` and the flat-file build passed locally. SQL migration/CAS integration for #292 passed against disposable MariaDB and MySQL with all 16 migrations applied, verified and reapplied idempotently; no production migration was run. Repository CI additionally verifies each PR before merge. Production rollout and later balance expansion remain separately authorized operations under the runbook.
