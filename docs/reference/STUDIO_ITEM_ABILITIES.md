# Typed Studio item abilities

[#295](https://github.com/Community-Duris/Duris/issues/295) adds an object-only
`itemability <id>` action, action ID **24**. Existing action IDs 0–23, event IDs
0–15, mob `cast`/`MobCastSpell`, and legacy object HIT actor/`$n` semantics stay
unchanged. The new action requests the [shared runtime](ITEM_ACTIONS.md) and
[artifact mana authority](ARTIFACT_MANA.md); Studio owns no second timer or mana
engine. The shipped `lib/item_abilities.json` is an empty version 1 catalog and
`itemActions.studio.enabled=0`.

## Authoring contract

The [JSON schema](studio-item-abilities.schema.json), [example catalog](../examples/studio-item-abilities.json)
and [example trigger references](../examples/studio-item-abilities.trg) form the
private editor handoff. Examples use a synthetic template, not a deployed
artifact roster. The C++ parser and runtime are authoritative where JSON Schema
cannot express a cross-field or world-state check.

```text
#22802 O
T HIT
itemability 1001
~
T CMD use
itemability 1002
~
S
#~
```

An `itemability` must be the only action in its trigger, with chance 100 and no
legacy `if` conditions. Admission eligibility belongs in the definition and
shared runtime. This prevents a failed legacy condition from accidentally
falling through to an instant ordinary device command. Other triggers and
legacy action records keep their existing rules and limits.

Every definition has these explicit fields:

| Field | Contract |
| --- | --- |
| `id`, `revision` | Stable ability ID 1–268435455; revision 1–1000000000. Changing any definition field requires a greater revision. Reordering JSON keys has no effect. |
| `vnum` | Existing object prototype. No mob or room prototype can own this action. |
| `trigger`, `mode`, `source` | `hit` requires `passive` and `equipped`; `use` requires `active`, with `equipped` or directly `carried` source. |
| `minimumLevel` | Actor level 0–60, checked at admission and completion along with shared eligibility. |
| `windupPulses`, `progressPulses` | Windup 4–600 pulses, clamped by shared bounds; optional progress 0 or strictly inside that delay. All effects follow the same real warning window. |
| `cooldownMs` | 0–3600000; begins at accepted payment, keyed by item UID and stable ability ID. |
| `concurrency` | `shared`: one pending action per item, the configured shared actor/global caps, and one active action per actor. Other policies are rejected. |
| `ownership` | `studio-action`: owns this particular authored action and addressed USE command. It does not replace a native callback. |
| `cost`, `mana` | Cost in thousandths of a mana point; null mana requires zero cost. A named profile has stable ID/revision, capacity, regeneration per second, and passive floor. Definitions sharing a profile must agree. |
| `effects` | One to three ordered typed spell effects: numeric spell ID, power 0–60, target and `spell`/`wand` call convention. Invalid indices, absent pointers and incompatible targets reject the catalog. |
| `presentation` | Literal begin/progress/complete/cancel text, 1–240 UTF-8 bytes each. Controls and `$` substitution tokens are rejected; the adapter supplies actor/item/victim presentation through `act()`. |

The file is limited to one MiB, 4096 definitions and 4096 distinct ability IDs
over a process lifetime, including removed-version history. Unknown/duplicate
keys, duplicate IDs, nonintegral numbers, unsupported schema versions, malformed
costs/delays and inconsistent mana profiles reject the **entire candidate** with
a field path or ability ID in the diagnostic.

## Explicit identities and targets

The object dispatch context separately captures original activator runtime ID
and struck victim runtime ID. Existing HIT code still presents the victim as
legacy `actor`/`$n`; the new typed executor resolves the original activator from
its separate ID. It never obtains the wielder from that legacy alias.

| Target | Meaning at selection and release |
| --- | --- |
| `activator` | The original character who owns the selected action; resolved by runtime ID. |
| `holder` | Current holder must still be that original activator, in the original custody/slot. Transfer cancels rather than retargeting. |
| `victim` | Original struck victim for HIT; explicit local name or current local opponent captured once for USE. No later opponent substitution. |
| `item` | Source item UID and original custody, passed as the spell's object target; requires an inventory/equipment spell. |
| `room` | Original world room, with the actor still there; only native area/ignore spell signatures are supported. |

World/remote targets, arbitrary commands, raw callbacks, delayed reactive
interception and native transformations are rejected. They require a separately
reviewed typed adapter. A room effect invokes the native area spell once; it
does not turn a single-character spell into an invented area spell.

At admission and before every effect, recheck life, awake/resting posture,
mobility/stun, magic, level, visibility, single-file restrictions, safe room/zone,
peace/gaze/nonoffensive state, master protection, combat permission and bound
ownership as applicable. Typed use is nonverbal. Targets and source pointers
are borrowed only for one call. Any effect that moves, kills or extracts a
participant cancels the remaining ordered effects through the shared hooks.

## Ownership and compatibility

The displaced C object callback still runs first and a TRUE result still wins.
The proclib displaced-proc bridge retains its precedence. This schema cannot
claim or suppress an entire native event in order to replace one power in a
combined callback. Such a migration must split or adapt that specific power
explicitly, as the native pilots in #293/#294 do. `studio-action` therefore
means authored action ownership, not automatic native artifact migration.

Once a typed USE is selected, admission failure, low mana, cooldown or a shared
cap swallows that addressed activation. It cannot fall through to the ordinary
instant wand/staff wrapper. Commands addressed to a different object continue
normally. Master/category/per-ability rollback returns control to the legacy
route. Missing catalog IDs suppress stale authored references while enabled.

The existing trigger recursion latch, depth ceiling, 24-action bound, command
permissions and movement guards remain in effect. Delayed spell callbacks can
produce ordinary game events, but an already pending source cannot admit a
recursive second action.

## Publication, resource state and rollback

Boot loads `lib/item_abilities.json` after spell pointers/prototypes exist, before
parsing `.trg` references. `properties reload` also reloads the catalog; it does
not reparse `.trg` files. Changes to trigger bindings require the normal world
rebuild/restart workflow.

`studio_abilities_load` validates the complete candidate, current world spell
signatures, version history and mana bindings before publication. Invalid data
leaves the last valid catalog and its pending actions active. An unchanged valid
catalog leaves pending actions active. A changed or removed definition cancels
only that ability's accepted actions. Remember that `properties reload` itself
crosses the existing global item-action reload barrier first; that broader
explicit operation cancels pending actions even if its catalog file is bad.

`itemActions.enabled`, `itemActions.studio.enabled` and
`itemActions.ability.<id>.enabled` gate routing. The last defaults to 1; the first
two ship off. An effective toggle cancels corresponding pending work. Re-enable
never releases canceled effects, restores cooldowns or refills mana.

Mana profile identity is independent of action order and Studio counter slots.
Costs are committed after scheduler acceptance, before presentation; rejected
payment changes no cooldown. Active powers may spend the reserved passive
floor; passive powers preserve it. Transfer, abort, definition reload, and
partial completion retain committed cost. Mana persistence, offline regeneration
and the bounded crash-refund window remain exactly the shared service policy.

Cooldowns are bounded in-memory state (65536 live UID/ability entries) using the
monotonic clock. They survive transfer, abort, save and catalog reordering/reload
within the process, and expire normally. **They reset on process restart** and
must not be used as a substitute for durable mana or a durable economy limit.

## Available and deferred event hooks

| Hook | Timing and scope |
| --- | --- |
| Object HIT / `CMD_MELEE_HIT` | Existing hit callback; observes the original struck character and schedules future selected effects. It does not intercept the damage that already triggered it. |
| Object `CMD use` | Existing command callback before the ordinary wrapper; captures addressed source and original activator. |
| Existing GOT_HIT / GOT_NUKED | Remain legacy post-resolution callbacks with `proc_data`; no pretend pre-damage reflection via this delayed action. |
| Extraction, departure, unequip, transfer | Shared cancellation hooks; never retained-pointer callbacks. |
| Reactive interception / stateful form changes | Require the narrow native adapters in #296, with explicit pre/post-resolution semantics and shared mana identity. Not accepted by version 1 JSON. |

## Editor stub and validation

The private DurisStudio repository is owned by Faemill. This public repository
provides the contract and a tested offline serialization stub; it does not claim
that an editor UI has been implemented or connected.

```sh
g++ -std=c++20 -Isrc scripts/studio_ability_validator.cpp \
  src/item/studio_ability_model.c -lcjson -o bin/studio-ability-validator
python3 scripts/studio_ability_editor_stub.py \
  docs/examples/studio-item-abilities.json bin/editor-roundtrip.json \
  --validator bin/studio-ability-validator --emit-trg bin/editor-roundtrip.trg
```

The stub validates both input and output through the actual C++ model parser,
preserves stable IDs/resources/effect order when keys reorder, and emits object
trigger references. It never connects to a game server, edits old `.trg` records,
or installs an output. Faemill's editor integration should consume the schema,
surface server diagnostics, preserve existing mob casts and IDs, and run these
fixtures through the private editor's export/import cycle.

Focused tests are `test_studio_ability_model.py`,
`test_studio_abilities_runtime.py`, and `test_studio_editor_stub.py`. They execute
the real strict parser, item runtime, object callback bridge, typed executor and
recursion latch with controlled world/mana boundaries. Existing Studio command,
movement, remote-counter and proclib bridge tests remain regression gates.
Production spell/world validation and live integrated mana/Studio measurements
are additionally exercised in the #297 rollout workstream; boundary doubles
alone do not establish production balance or private editor completion.
