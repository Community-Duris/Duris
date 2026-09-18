# Artifact, unique, and ioun control: current implementation and proposed design

Research date: 2026-09-18. Source: fetched `origin/master`, commit `440248b17eecc3517229a48a4946cf6c0a33ffa5`. This is a source audit and implementation plan, not a statement of production configuration or current live custody. No gameplay settings, world data, database state, or production services were changed.

Implementation handoff: the expanded [normative specification](ARTIFACT_CONTROL_SPEC.md), [in-game/offline/SQL operator contract](ARTIFACT_CONTROL_OPERATIONS.md), [ordered work packages and verification matrix](ARTIFACT_CONTROL_WORK_PACKAGES.md), and [Luna Max goal prompt](ARTIFACT_CONTROL_GOAL.md) turn this research into an implementation-ready package. Their finalized contracts supersede tentative design choices below. Start with [the package index](ARTIFACT_CONTROL_README.md).

## Recommendation

Centralize authoring, inspection, and policy, while retaining small specialized execution modules. Build on the existing item-action scheduler, native adapters, Studio catalog, mana authority, and artifact persistence. A wholesale rewrite into a universal scripting language would duplicate working infrastructure and obscure difficult reactive and stateful behavior.

Use one logical artifact catalog with explicit classification, load policy, lifecycle policy, behavior variants, and power profiles. Keep complex effects in reviewed C++ adapters; make their safe tuning parameters data. Give developers and immortals a common inspection and publication interface backed by that catalog. Preserve the same physical item UID when selecting behavior for different holders.

The most valuable initial deliverables are a truthful `inspect/why` view, an explicit player/NPC mode resolver, and validated per-artifact configuration. Loading and lifetime policies can then migrate behind the same interface in independently reviewable stages.

## 1. What defines an artifact today

These categories are not a single strongly typed definition:

| Concept | Current test | Consequence |
| --- | --- | --- |
| Artifact | `ITEM_ARTIFACT` in `extra_flags` | Enables artifact lifetime/tracking/load rules. |
| Ioun | `ITEM_WEAR_IOUN` wear capability | Classification depends on equipment metadata. |
| Unique | Keyword `unique`, excluding `powerunique` | Editing keywords can affect mechanical classification. |
| Major | Artifact categorized outside the unique/ioun cases | Classification precedence must be preserved per call site. |

See [utils.h](../../src/core/utils.h), especially lines 337–339, and [artifact.c](../../src/guild/artifact.c). Some callers independently inspect keywords rather than these helpers. `check_single_artifact` in [actobj.c](../../src/cmd/actobj.c) uses its own keyword checks for the configurable major-equipment restriction. The paid locator NPC `llyren` in [specs.mobile.c](../../src/specs/specs.mobile.c) uses substring tests. Centralization should first characterize these differences, not silently reclassify existing items.

The checked-in [artifact inventory](../reference/artifact_source_inventory.json) contains **169 templates, 78 literal native bindings, 67 distinct callbacks, 91 templates without a literal binding, and 29 golden-token placeholders**. I reproduced those counts using `python3 scripts/artifact_source_inventory.py --check` under WSL with the compiler's active configuration. A missing native binding does not imply no power: base affects, packed spells, device types, equipment enchantments, and dynamic Studio/proc-library bindings also matter.

I added [artifact-control-source-map.json](artifact-control-source-map.json), joining all 169 inventory records to direct `E/G/O/P` references in the active `areas/AREA` files and locating their native callback definitions. **89 templates have direct references in that scan.** This is not a complete availability proof: indirect object tables, scripts, special procedures, world recovery, staff loads, and actual deployed area data remain separate paths. Lexical last-mob context does not prove that preceding conditional reset commands succeeded.

## 2. How a developer adds an artifact now

1. Add or edit the object prototype in `areas/obj/*.obj`: vnum, text/keywords, item type, artifact and wear flags, values, affects, and extra descriptions. Ensure its area participates in `areas/AREA`. The source [object format](../content/AREA_OBJECT_FORMAT.md) documents the numeric fields.
2. Implement or assign behavior. Native callbacks are attached by vnum in [specs.assign.c](../../src/specs/specs.assign.c). Their implementations are scattered across `src/specs/*.c`, especially `specs.object.c`, `specs.underworld.c`, and `specs.ioun.c`. These `.c` files compile as C++20. Ordinary weapon/device behavior can instead come from object values. `_proclib_` extra descriptions and Studio `.trg` files provide additional dynamic behavior.
3. Add a load point in `areas/zon/*.zon`: `M` establishes a mob; `E` equips an object on the last mob; `G` gives it to that mob; `O` loads it in a room; `P` loads it into an object. Conditional flags, object/mob limits, chance, and zone timing participate. Other reset letters can use object tables. A configured NPC inventory load does not automatically equip the artifact or teach the NPC to issue its active commands.
4. Rebuild/install the world through the existing area toolchain and use the normal restart/world-data workflow. `areas/src/zon/make_zon.c` follows `areas/AREA`; searching every file in `areas/zon` without filtering the index produces misleading matches from inactive content. Native callback changes also require a server build. A properties reload does not reparse `.trg` bindings.
5. Exercise the actual acquisition, use, death/loot, expiry, and reload path in an isolated world. Artifact records are updated through object-location hooks; inserting a database row alone neither defines the prototype nor installs a power or load point.

### Verified pilot load references

| Artifact | Active direct reset | Lexical mob and room context | Placement |
| --- | --- | --- | --- |
| Avernus 19730 | `bctdl.zon:287` | Mob 32420, room 32469 | `G`, inventory |
| Mirrored ioun 922 | `juiblex.zon:411` | Juiblex 87544, room 87626 | `G`, inventory |
| Tsunami 31514 | `seakngdm.zon:320` | Poseidon 31528, room 31722 | `E`, slot 17 |
| Living necroplasm 67243 | `negplane.zon:581` | The Dark 26642, room 26859 | `G`, inventory |
| Mayhem 21 / Symmetry 22 | No direct reference found in active `E/G/O/P` scan | Templates and bindings exist | Do not infer absence from all other acquisition paths. |

All four found references have nominal chance 100 and limit 1. **`item_load_check` halves artifact chance using integer division**, so 100 becomes 50 at that check; other reset gates still apply. This helper ignores its `ival` argument. Certain reset paths, such as shopkeeper handling, have additional exceptions. See [utility.c](../../src/core/utility.c), line 7351, and [db.c](../../src/world/db.c), `reset_zone` at line 3230 approximately.

### Respawn and recovery

`artifact.respawn` ships as 1:

- `0`: reset loading of artifacts is suppressed.
- `1`: artifact reset loading is allowed only for the boot reset (`force_item_repop == 2`).
- Other values bypass these two exclusions; 2 is the natural intended regular-reset mode, but this legacy property is not validated as a strict enum.

An owned artifact record prevents normal replacement loading. Object population limits and load chance also participate. `zreset ok` calls ordinary reset; `zreset full` purges and uses force value 1. **Neither is the boot value 2**, so neither overrides boot-only artifact policy.

Recorded-location reconstruction exists in `addOnGroundArtis_sql` and `addOnMobArtis_sql`; NPC world recovery has another artifact-aware load path in [world_recovery_npc_items.c](../../src/world/world_recovery_npc_items.c). A new spawn policy must cover recovery and staff/script entry points as well as the main reset switch. Recovery should restore existing custody, not reroll a fresh loot award.

## 3. What an immortal can control now

| Existing command/interface | What it actually controls |
| --- | --- |
| `artifact major`, `artifact unique`, `artifact ioun` | Category listing. Trusted users can request `all` or the mortal view. |
| `artifact player ...`, `artifact hunt ...` | Existing owner/location investigation routes. These are not a consolidated definition editor. |
| `artifact timer <set\|add\|subtract> <vnum> <minutes>` | Existing ticking expiry timer; requires `GREATER_G`, clamps to the ten-day horizon. |
| `artifact poof <vnum> ...` | Existing destructive removal/reset flow; consult its built-in help/confirmation syntax. |
| `artifact clear <vnum>` | Clears artifact tracking entries. It is not a definition or spawn-location editor. |
| `artifact swap <vnum1> <vnum2>` | Replaces a tracked artifact with another template, with source/destination ownership constraints. It is not legacy/new-mode selection. |
| `artifact reset ...` | **Resets soul binding**, including repair/sync subcommands. It does not move the load point. |
| `load obj <vnum>` | Creates a prototype instance through the wizard load/custody path. No `legacy`/`telegraphic` variant argument exists. |
| `zreset ok`, `zreset full` | Current-zone reset, with the limitations above. |
| `properties show <pattern>`, `diff` | Inspect numeric runtime settings and changes. |
| `properties set <pattern> <value>`, `save`, `revert`, `reload` | Existing property mutation and persistence controls. Set/reload/save/revert require `FORGER`; command entry is `LESSER_G`. |
| `itemmana <item>`; trusted `itemmana metrics` | Holder mana/readiness inspection and aggregate new-action telemetry. |

The artifact dispatcher permits category lists before its `FORGER`/non-NPC management gate; some subcommands require higher rank. See [artifact.c](../../src/guild/artifact.c), [actwiz.c](../../src/cmd/actwiz.c), [interp.c](../../src/cmd/interp.c), and [properties.c](../../src/world/properties.c).

`properties set` changes **existing matching keys only**; it does not create a missing key. A native pilot's cost/capacity keys are not all present in the shipped file. Seed needed keys in the file and reload before expecting in-game setters to find them. Wildcard setters are broad; there is no artifact-specific staged, atomic multi-field editor. The property store uses floats and a fixed 3,000-entry array, another reason not to place hundreds of detailed artifact definitions into it.

## 4. Timers and persistence are separate systems

| Clock/state | Current owner | Meaning |
| --- | --- | --- |
| Zone reset age/mode | Zone definitions/runtime | When the world considers resetting a load point. |
| Artifact expiry | `artifacts` tracking and corresponding flat-file authority | UTC deadline to poof, associated with artifact vnum and custody. |
| Soul binding | `artifact_bind` and corresponding backend state | Binding owner and transfer/merge timing. |
| Native power cooldown | Object timer slots, e.g. `timer[0/1]` | Artifact-specific reuse limits, generally captured by existing object persistence. |
| Legacy sword energy/cursor | Native object fields/state machine | Old Mayhem/Symmetry power economy and selection state. |
| New windup | `item_actions` scheduler | Owned cancellable action with monotonic reaction-time protection. Not restored after restart. |
| New Studio cooldown | Process-local UID + ability-ID map | Retained across in-process transfer/reload, **reset on restart**. |
| Physical item mana | Independent per-UID mana authority | Reserve/capacity/rate/revision/settled timestamp, separate from owner snapshots. |

NPC custody does not start a fresh artifact expiry timer. First PC acquisition starts the deadline; later location changes preserve the established timer. Room/container/corpse placement does not provide a fresh lifetime. The getter `get_artifact_data_sql` returns whether the record is **owned**, not merely whether a row exists; preserve that distinction in any facade.

The active lifetime uses `ARTIFACT_BLOOD_DAYS = 10` from [db.h](../../src/world/db.h), including multiple repair, cap, and first-acquisition paths in `artifact.c`. The shipped file also contains `artifact.feeding.initial.secs=360000` (100 hours), `artifact.feed`, and accumulated/single feeding fields; searches found **no consumers in `src` for these keys**. They must not be presented as working controls. Audit and deprecate them explicitly.

Working controls include `artifact.feeding.epic.point.seconds` (shipped 1200), per-epic-type multipliers, binding switch/loot allowances (120/15 minutes in the file), `artifact.major.limit.one`, `artifact.wars.modifier` (0.5), and the difficulty feeding dial. Transactional award calculation in [artifact_guild_state.c](../../src/guild/artifact_guild_state.c) uses the feeding dial and recent-frag multiplier. A parallel older implementation remains in `epic.c`; centralizing calculation must account for actual callers and backend behavior rather than preserving two competing formulas.

Artifact wars count excess majors, uniques, and iouns and reduce remaining lifetime using the configured modifier, capped at full burn. This affects scarcity/lifetime, independently of action mana.

The registered periodic jobs in [new_events.c](../../src/world/new_events.c), around line 2028, use initial delay / repeat interval of 15 seconds / 7 minutes for binding, 20 seconds / 30 minutes for wars, and 35 seconds / 12 seconds for expiry. Batching and runtime scheduling mean an expiry deadline is not a guarantee of removal at that exact millisecond.

Artifact tracking remains largely vnum-oriented, while mana and physical custody are UID-oriented. SQL deployments also have transactional artifact/guild domain state and outcome handling. These are existing persistence responsibilities to preserve behind an interface; a catalog must not become a second mutable state authority.

## 5. How power is controlled today

There is no universal artifact power level. Effective strength combines prototype dice/stats/affects, selected spell and power, hard-coded callbacks, trigger probability, cooldowns, wearer level/class/race checks, spell-system rules, and new-mode timing/resource constraints.

Examples:

- Avernus: native 1-in-25 selection; drain based on clamped victim HP, maximum 200; its speech stone-skin power is level 60 on a 60-second native timer. The new drain delays both healing and damage; speech and hum stay native.
- Packed weapon spells: object `value[5]` stores packed spells, `value[6]` supplies power, `value[7]` participates in trigger selection; the new adapter captures those values. Random offensive equipment captures level 50 and retains a 15-second selection timer.
- Native swords: class-based 5d5/+5 or 6d6/+6 configurations, selected spell levels, fixed cost multipliers, race rejection, and a substantial state machine. New-mode drain is deliberately retuned to 50 native negative damage and capped healing; it is not simply a visual delay.
- Tsunami: native 300-second tap and 500-second wave cooldowns remain code constants in the pilot. New mode adds payment, cancellation, safer group/target rules, and removes the trusted wave cooldown bypass.
- Iouns: multiple unrelated callback families and ordinary stat/affect definitions. For example `ioun_sustenance` rejects NPC users and uses a native 60-second timer. Only mirrored ioun 922 is in the new native pilot roster.

Safe numeric fields should become typed config; target legality, identity validation, bounded execution, and correct pre-damage interception must remain engine-enforced.

## 6. Classic and telegraphic execution map

| Family | Legacy source | New source and scope |
| --- | --- | --- |
| Avernus 19730 | `specs.underworld.c::avernus` | `item/weapon_actions.c`; offensive drain only |
| Packed/random offensive procs | Native weapon/random dispatch | `item/weapon_actions.c`; defensive/beneficial exclusions remain |
| Tsunami 31514 | `specs.underworld.c::SeaKingdom_Tsunami` | `item/tsunami_actions.c` |
| Mirrored ioun 922 | `specs.underworld.c::deflect_ioun` | `item/ioun_actions.c`; synchronous paid interception |
| Living necroplasm 67243 | `specs.object.c::living_necroplasm` | `item/necroplasm_actions.c`; linked paid transformation, retained native lifecycle |
| Mayhem 21 / Symmetry 22 | `specs.object.c::good_evil_sword` and helpers | `item/sword_actions.c`; paid bundles, cursor/challenge/flurry/nova |
| Wands/staves/scrolls | Native `do_use` / `do_recite` paths | `item/device_actions.c`; charge/ink cost |
| Wonder 41350 | Native wonder special | `item/wonder_actions.c`; captured outcomes |
| Authored Studio actions | `mob/studioproc.c` / `.trg` / proc library | `item/studio_abilities.c`, `studio_ability_model.c`, JSON plus `itemability` trigger |

All new adapters share `item/item_actions.c`; paid powers use `artifact_mana*.c`. Native-pilot gating is in `item/native_artifact_actions.c`. The full legacy callback map for all 169 templates is in the accompanying JSON.

The revised ioun is intentionally synchronous: it must decide whether to block the incoming hit before damage. Applying a generic two-second windup to that existing interception would change its meaning. A future telegraphed defensive stance would be a separately designed ability.

Studio already offers versioned JSON with up to three typed spell effects, power, targets, cost, mana profile, windup, level requirement, and cooldown. `lib/item_abilities.json` ships empty. Native displaced callbacks retain precedence; attaching `itemability` does **not** automatically replace a native artifact callback. Version 1 cannot express arbitrary state machines or synchronous interception. The public editor handoff/stub exists; the private DurisStudio UI integration is not established by this repository.

### Existing switches

| Scope | Current properties |
| --- | --- |
| Master and resources | `itemActions.enabled`, `itemActions.mana.enabled` |
| Native pilots | `itemActions.artifact.<vnum>.enabled` for 21, 22, 922, 31514, 67243 |
| Native pilot tuning | Same prefix: `manaCost`, `manaCapacity`, `manaRegen`, `passiveFloor`, `manaRevision`, `windupPulses` |
| Avernus/packed/random categories | `itemActions.avernus.enabled`, `.weapons.enabled`, `.randomWeapons.enabled`, category windup |
| Weapon template | `itemActions.weapon.<vnum>.enabled`, mana cost/capacity/regen/revision |
| Device categories/template | `itemActions.{wands,staves,scrolls,wonder}.enabled`, category windup; `itemActions.device.<vnum>.enabled` |
| Studio | `itemActions.studio.enabled`, `itemActions.ability.<id>.enabled` |
| Foundation | `reactionPulses`, `maxPulses`, `maxPerWielder`, `maxPending` under `itemActions.` |

Master/category/native-pilot gates ship off. Weapon/device per-template switches default on underneath their category gates. Native pilots require positive cost and valid resource profiles; merely enabling a pilot with the shipped zero cost does not make its paid power usable. Avernus and other weapon pilots can use zero-cost timing. Devices use charges/ink rather than automatically gaining artifact mana.

The usual windup is 8 pulses = 2 seconds. Mana uses integer thousandths of a point. New UIDs enroll **empty**, then regenerate; new capacity does not grant free reserve. Profile changes require revisions. New-mode admission/payment failure suppresses the action, never silently executes the free legacy effect. Explicit master/category/template rollback selects legacy where supported. Disabling mana alone suppresses paid powers. Broad properties reload cancels pending item actions.

See the existing [rollout runbook](../operations/ITEM_ABILITY_ROLLOUT.md), [native pilot contract](../reference/NATIVE_ARTIFACT_PILOTS.md), [weapon contract](../reference/WEAPON_ACTIONS.md), [device contract](../reference/DEVICE_ACTIONS.md), [Studio contract](../reference/STUDIO_ITEM_ABILITIES.md), and [mana contract](../reference/ARTIFACT_MANA.md).

## 7. Can versions be selected independently now?

**Per supported template: yes. Per physical copy or player/NPC holder: no unified support.** The native ownership decision accepts a vnum, not a holder or instance variant. Global/category gates also apply. Neither `load obj` nor normal reset records have a behavior-variant field. Callback-specific NPC exclusions, class checks, or level scaling are not an operator-configurable mode policy.

An existing NPC can use eligible passive/reactive code, but a `G` load may only place the item in inventory, and an active power still needs an NPC command/AI invocation. A new holder selector must not be sold as AI support.

Copying the prototype to a second vnum is an incomplete workaround: native bindings and hard-coded vnums need adaptation, limits/tracking can permit another artifact, and resource profile identity can change. `artifact swap` likewise does not promise same-UID/same-resource conversion. The preferable solution is explicit mode metadata and routing for one logical artifact identity.

## 8. Proposed catalog and modules

Use an index and small per-artifact JSON documents under `lib/artifacts/`, with reusable balance/lifecycle profiles. JSON fits the existing parser/tooling. This is one logical source of truth, not necessarily one enormous file. New module names below are proposals:

| Module | Responsibility |
| --- | --- |
| `artifact_catalog` | Parse/validate immutable definitions; classify items explicitly; identify supported adapters and source revisions. |
| `artifact_policy` | Resolve effective variant from artifact, per-UID selection, holder role, and load provenance. Return a decision and reason. |
| `artifact_spawn` | Gate fresh creation, enforce logical uniqueness, interpret configured placements; integrate reset, recovery, scripts, staff loads. |
| `artifact_lifecycle` | Expiry/binding/feed/war policy and calculation; call existing durable repositories/transactions. |
| Existing action adapters | Typed configuration and narrow behavior-specific execution. Preserve shared scheduler/mana services. |
| `artifact_admin` | Unified read/preview/validate/publish/rollback commands, permissions, audit trail. |

Persist runtime facts separately: artifact identity/family, UID, selected variant policy/override, spawn provenance where needed, existing custody/expiry/binding, resource state, and durable cooldowns. Configuration contains definitions/defaults, not current owners or reserve balances.

Initial spawn entries should reference existing reset records and expose their effective policy. Later, move those placements into the catalog and generate/integrate their reset operations. Never have both an independent artifact respawner and an old zone reset authoritatively loading the same item. Give placements stable IDs rather than relying permanently on shifting source line numbers.

### Configuration to expose

- Identity: explicit `major/unique/ioun`, logical uniqueness family, vnum(s), supported forms, adapter IDs, definition revision.
- Acquisition: enabled placements, mob/room/container, equip versus inventory, slot, explicit final chance, population limit, boot-only/zone-reset/manual policy, availability conditions. Convert today's hidden chance halving into an explicit compatibility rule before removing it.
- Lifecycle: initial/max lifetime in seconds, feeding coefficients/caps, soul merge and loot allowances, multiple-artifact policy, expiry behavior. Make lifetime-definition changes future-acquisition-only by default; retroactive deadline edits require an explicit state operation.
- Behavior: available legacy/telegraphic/reworked variants, player/wild-NPC/pet policy, per-load-point override, per-UID override policy, supported events and power ownership.
- Power: per-ability chance, spell level or validated level formula, damage/heal cap, cooldown, target cap, windup/progress, per-ability cost, profile capacity/rate/floor. Preserve exact current values in imported compatibility profiles.
- Presentation: reviewed message IDs/templates for warning, progress, success, cancellation. Do not leak private resource or enemy state.

Keep safety and computational limits outside unrestricted content control. Reject unsupported modes, negative/out-of-range numbers, unresolved vnums/rooms/spells, duplicate identities, conflicting power ownership, impossible equipment slots, and incompatible resource bindings.

### Example of the proposed shape — not accepted by current code

```json
{
  "schemaVersion": 1,
  "id": "tsunami",
  "revision": 1,
  "vnum": 31514,
  "classification": "unique",
  "uniquenessFamily": "tsunami",
  "lifecycleProfile": "imported-artifact-lifetime-v1",
  "placementRefs": ["poseidon-tsunami"],
  "behaviorPolicy": {
    "player": "legacy",
    "wildNpc": "telegraphic",
    "playerControlledNpc": "player-policy",
    "onHolderChange": "cancel-and-resolve"
  },
  "variants": {
    "legacy": {"adapter": "tsunami.legacy", "balance": "tsunami.classic-v1"},
    "telegraphic": {"adapter": "tsunami.v2", "balance": "tsunami.canary-v1"}
  },
  "resourceProfile": "tsunami-shared-v1"
}
```

The named profiles must be defined and validated; this example intentionally supplies no unapproved production mana budget. A variant can share an engine with another variant when only configuration differs. Where a semantic retune exists, such as sword drain or lawful ioun redirection, expose that distinction explicitly instead of claiming all modes differ only in their messages.

## 9. Holder-dependent and independently loaded versions

Support three useful policies:

1. **Follow holder:** same UID uses legacy on a wild NPC and reworked on a player, or the reverse. Re-resolve on legitimate custody/controller transitions.
2. **Pinned instance:** staff or a configured spawn selects a supported variant for this UID, retained after loot and restart. Useful for controlled side-by-side fixtures and staged releases.
3. **Placement policy:** a specific encounter uses an approved override, then either retains it or returns to holder policy after acquisition. Make that choice explicit in data.

Proposed resolver order: emergency power suppression; explicit permitted UID policy; applicable placement policy; holder/controller rule; artifact default. Required capability/resource checks still run afterward. Treat possessed/charmed/pet NPCs under player-control policy unless explicitly specified otherwise, preventing a player from obtaining boss behavior simply by controlling an NPC.

Variant selection does not manufacture another instance. Retain one logical uniqueness family across legacy/new forms. Production aliases must share scarcity limits; isolated test copies must not enter ordinary loot circulation.

On holder/mode transition: cancel pending work first; remove only source-linked grants; retain UID, expiry/binding deadlines, committed mana, and cooldown obligations; resolve and validate the new mode; apply its reversible equipment/form contributions. Reacquire identities after callbacks. Never refill, refund, or revive pending work just because the holder changed. Preserve separate legacy energy state without converting it into paid reserve; switching variants must not reset either economy to a favorable state.

Use a stable per-artifact resource profile across holders. Prefer role-specific costs/effect limits over changing capacity/rate/profile identity at each transfer. Today's new UID starts empty: a modern boss may be weak immediately after boot until its item regenerates. If designers need precharged encounter powers, decide that explicitly. Either keep the physical-item rule and allow preparation time, or implement a separate, non-transferable NPC encounter resource. Do not silently refill the lootable item on NPC equip.

Active NPC powers need a narrow AI policy that invokes the same validated ability admission, obeys cooldown/cost, and emits the same warning. Passive, active, and reactive capabilities must be listed separately per adapter.

Add a true **suppress powers** operational mode separately from **use legacy**. Current off switches often mean legacy rollback; a new central editor must not confuse an emergency stop with enabling an old instant power.

## 10. Unified developer/immortal experience

The following are proposed commands, not existing syntax:

```text
artifact inspect <vnum|uid>
artifact explain <vnum|uid> [player|npc <vnum>]
artifact placements <vnum>
artifact validate <candidate>
artifact preview <candidate>
artifact publish <candidate> <expected-revision>
artifact rollback <catalog-revision>
artifact spawn <vnum> variant <supported-mode> to <destination>
artifact instance <uid> mode <supported-mode|follow-holder>
```

Inspection should answer: what defines it, what category it is, where it can load, nominal and effective chance, why a spawn is blocked, who currently holds it, expiry versus binding versus ability timers, which variant applies and why, which powers remain legacy, effective power values, resource readiness, and whether the mob is equipped/able to invoke it.

Use typed staging and atomic candidate publication. Validate the complete candidate against prototypes/adapters/spells, show effective differences, and publish only if the expected current revision matches. A malformed candidate retains the prior catalog; removal or changed definitions cancel affected actions. Persist the selected catalog revision so restart agrees with the running server. Versioned configuration and runtime state migrations need separate rollback handling; downgrading files must not erase resource/debit fences.

Provide read-only builder access, balance editing rights, placement/lifecycle rights, and restricted instance-state repair rights. Record actor, reason, old/new revisions, affected artifacts, and outcome. Reuse existing command/security conventions, but avoid requiring direct SQL for routine configuration.

## 11. Implementation sequence and acceptance criteria

| Stage | Concrete change | Completion criteria |
| --- | --- | --- |
| 1. Inventory and inspection | Extend the existing source inventory to load provenance, dynamic scripts, classifications, defaults, and effective settings; add read-only `inspect/explain`. | Representative majors/uniques/iouns trace from prototype to load to proc. Inactive areas and unsupported claims are clearly marked. Dead properties identified. |
| 2. Catalog compatibility layer | Strict parser and immutable registry; import current defaults without changing mechanics; adapter registry; explicit category normalization report. | Existing behavior matches baseline; invalid candidates retain previous definitions; no duplicate mutable authority. |
| 3. Variant resolver | Resolve by UID/holder/placement; adapt each migrated power boundary; persist selection metadata across all custody routes. | Legacy/new on NPC and PC, pinned/follow-holder modes, charmed/pet cases, and restart all work without duplication or free refill. |
| 4. Extract tuning | Parameterize current native pilot numbers and per-ability costs; import exact defaults; share lifecycle feed calculation; type lifetime policy. | Profile changes reproduce specified effects and preserve cooldown/resource contracts. No silent changes to unmigrated powers. |
| 5. Centralize spawn/lifecycle | Stable placement IDs, one spawn authority, acquisition gate, typed lookup errors, declarative lifetime/binding/war policy. | Boot/reset/manual/recovery/script paths enforce intended uniqueness and policy. Storage error is not treated as permission to create a replacement. |
| 6. Admin authoring | Stage/preview/validate/publish, effective view, history, role controls; extend the Studio contract for supported data. | Atomic publication and restart persistence; exact operator-visible change reports; private editor work verified separately. |
| 7. Controlled expansion | Qualify pilot families, then migrate remaining callbacks individually. | Complete effect contract, measurement, and recovery evidence for each added family. Placeholder tokens gain no invented powers. |

Do not block the first useful delivery on all 169 templates. Start with Tsunami for active powers and holder policy, Avernus for passive compatibility, mirrored ioun for interception, then necroplasm/swords for stateful cleanup. Complete one end-to-end path before expanding the catalog's scope.

### Required verification for implementation

- Baseline legacy output/effects/selection under imported defaults, including native class/race and NPC exclusions.
- All holder/mode combinations, wild versus player-controlled NPCs, G versus E loads, AI invocation, death/loot, give/drop/get, containers, equip/unequip, and same-room transfer.
- Exactly one selected implementation per power; no native-plus-Studio double execution and no legacy fallback on payment failure.
- Source/target departure, extraction, abort, hot reload, forced shutdown/copyover, and stale snapshots; no revived effects or repeated debit.
- Cooldown/expiry/binding separation, offline lifetime, feeding/war caps, old/new profile revisions, capacity reduction, and no free reserve after switching.
- Duplicate spawn attempts across reset/admin/recovery, unavailable persistence, identity collisions, competing placements, and mode aliases sharing one uniqueness family.
- Effective strength measurements: real proc throughput, burst size, sustained mana trajectory, reaction time, interruption rate, and class/PvP/PvE interactions. Preserving damage per proc does not preserve damage over time after adding windups/caps.
- Run existing focused item-action, weapon, native artifact, sword, Studio, mana, artifact/guild, flat-file, and reset tests relevant to each stage; run native/SQL/flat-file lifecycle journeys where their contracts change.

Reuse the [existing rollout evidence and runbook](../operations/ITEM_ABILITY_ROLLOUT.md). Its suggested initial canary cap is 128 pending actions, while the shipped property allows 4096; that maximum is not a demonstrated live capacity guarantee. The existing mana authority permits a bounded crash-refund window, not synchronous durability before every effect. These constraints remain part of the design.

## Research validation and limits

- Fetched current master and isolated the audit in a detached worktree.
- Read the active code, properties, source contracts, area index, templates, reset handlers, admin commands, and native/new adapters.
- Reproduced the existing compiler-aware artifact inventory successfully.
- Produced a 169-record direct-reset/native-callback source map, with explicitly bounded scan semantics.
- Did not boot a game server, query production custody/configuration, retune an artifact, run migrations, or claim private editor verification. Implementation tests above are planned acceptance work, not tests reported as already passed.
