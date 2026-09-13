# Item action foundation

This is the default-off shared runtime for [#291](https://github.com/Community-Duris/Duris/issues/291),
the first workstream in [#290](https://github.com/Community-Duris/Duris/issues/290).
The core registers no gameplay abilities by itself. Optional
[weapon adapters](WEAPON_ACTIONS.md), [device activation](DEVICE_ACTIONS.md), and
[artifact mana](ARTIFACT_MANA.md) use it. [Typed Studio authoring](STUDIO_ITEM_ABILITIES.md)
also requests the shared lifecycle; further artifact migrations remain separate workstreams.

## Routing and admission

`src/item/item_actions.h` exposes a typed adapter and a copied, immutable ability
definition. An ability names its revision, active/passive mode, source custody,
windup, and up to three selected effects (effect id, power and call type). A
registration owns its adapter and survives until all payloads using it are gone.
No API accepts a raw special-procedure function, borrowed callback payload, or
the character's mutable ordinary spellcast state.

A pilot registers a reviewed adapter using `item_actions_publish`, then
calls `start_item_action` only from the selected migration route. Interpret the
result explicitly:

| Result | Caller behavior |
| --- | --- |
| `legacy` | Run the original path with its existing trigger chance, call type and consumption ordering. The master switch is off, or this ability is absent/disabled. |
| `scheduled` | The new action owns this activation. Do not run the instant effect. |
| `suppressed` | The new route rejected this attempt. Do not fall through to the instant effect, add wait, spend resources or announce. |

The framework fixes the original target and source UID at admission. A source
must be in the actor's declared equipment slot or directly carried. Passive
actions require equipment. There is one pending action per physical item, a
configurable bound per wielder, and a global bound. A single actor can have only
one active device action. Initial active admission also rejects ordinary casting
or an existing wait. No random rolls occur in the framework.

Definitions with `selected_effects=true` accept a copied invocation-specific
selection through `start_selected_item_action`. Fixed definitions reject this
override. Typed effects may capture an original-target/self policy and bounded
adapter data; they do not retain pointers. An optional presentation-only
`progress` callback uses the same owned scheduler payload and monotonic floor.
See [weapon pilots](WEAPON_ACTIONS.md) for the first production adapters.

Typed devices can use `start_item_action_instance` to own a reviewed definition
and immutable invocation adapter without keeping a registration per activation.
It has the same lifecycle and bounds. Extra captured character/object targets
participate in departure cancellation through read-only identity predicates.
`item_action_pending(token)` lets a native adapter stop its own multi-callback
loop if an earlier callback cancels the action. See [devices](DEVICE_ACTIONS.md).

## Lifecycle and adapter responsibilities

1. Validate the definition, identities, custody, room, caps and adapter permissions.
2. Accept an owned scheduler payload linked to actor, original target and source.
3. Atomically commit or reserve the adapter's cost. A rejected cost leaves no
   cost or cooldown and cancels the scheduled payload before any announcement.
4. Announce through the adapter and start the real-time reaction window.
5. At dispatch, revalidate participants, custody, room and permissions. If the
   scheduler has caught up faster than real time, transfer the payload to another
   bounded event. Never announce again or pay again during that transfer.
6. Invoke each selected effect at most once, reacquiring all participants before
   the next effect. The actor's current fighting opponent is never substituted.
7. Remove activity/cap bookkeeping and settle the cost exactly once. Payload
   destruction also performs cancellation cleanup when a generic event link is
   broken or scheduling fails.

`validate` is read-only. `commit` is a synchronous resource operation and must not
invoke gameplay callbacks or perform world transitions. It returns `none`,
`committed`, `reserved`, or `rejected`. A rejection must be atomic; the engine
cannot undo an adapter's hidden mutations. `announce` and `resolve` may trigger
world transitions; they may not retain borrowed participant pointers. After an
effect extracts a participant, its own adapter must stop using that pointer.

`finish` receives identities, consumption state, and one of these outcomes:

- `interrupted`: no effect was invoked; the adapter may release its reservation
  according to the ability's declared consumption policy.
- `partially_resolved`: at least one effect was invoked before cancellation;
  settlement must account for that effect and cannot give a pre-cast refund.
- `completed`: the selected effect sequence was invoked successfully.

`finish` may release its resource bookkeeping but must not invoke gameplay
callbacks or start new actions. It cannot assume the source or any participant
still exists. The framework never refunds a committed cost or replenishes an
item when the master switch is re-enabled. Durable mana and restart reservation
policy belong to #292; no mana persistence is claimed by this foundation.

## Cancellation and activity

Room departure cancels both owner and original-target actions after any room
leave veto and before removal. Extraction, unequip (including save-time
unequip), and removal from inventory cancel source actions. Each accepted
action has a single-use transition token, so leave/return and remove/re-equip
cannot revive it. Completion independently verifies the source UID, character
runtime identities, original room, original slot and adapter permissions. The
permission policy is revalidation at dispatch; permission-specific eager hooks
can call the same cancellation APIs when their pilot requires them.

Active actions use an independent activity index shared by the command queue,
command dispatcher, `abort`, `StopCasting`, and melee loop. The queue permits
`abort`, `petition`, and `return`, retaining other type-ahead in order. Device
abort is always available while the device is active; the existing optional
ordinary-spell abort toggle is unchanged. An item abort never removes, clears
or replaces an unrelated `event_wait`/`PLR2_WAIT`.

Passive actions never set `AFF2_CASTING`, consume personal spell slots, install
a wait, stop melee, or enter the restricted command queue. Active player actions
receive the scheduler's existing player priority; passive traffic retains normal
priority and the scheduler's normal aging policy.

## Properties and reload

All properties are numeric and shipped in `lib/duris.properties`:

| Property | Default | Accepted integer range |
| --- | ---: | ---: |
| `itemActions.enabled` | 0 | 0–1 |
| `itemActions.reactionPulses` | 4 | 1–600 |
| `itemActions.maxPulses` | 120 | 1–600 |
| `itemActions.maxPerWielder` | 2 | 1–8 |
| `itemActions.maxPending` | 4096 | 1–4096 |

The reaction floor cannot exceed the maximum duration. NaN, infinity,
out-of-range values and fractional values disable the new route. Missing
properties use the defaults. Timing is clamped to the configured bounds and
checked against both scheduler pulses and a monotonic clock. One pulse is
currently 250 ms; these are framework safeguards, not measured artifact tuning.
Race, haste, quickchant and ordinary spell timing do not enter this calculation.

Changing any effective property cancels pending actions. Applying unchanged
properties does not. Master disable/re-enable does not resolve or revive them.
`item_actions_disable(id)` independently rolls back one ability. A replacement
must increase its revision and cancels only that ability's old actions.
`item_actions_reload()` cancels every action and discards registrations; a data
loader must cross this barrier before republishing its definitions. Initial
property-file loading also crosses it. Future Studio loaders must use this API
rather than mutating live definitions.

## Validation

Run `python3 tests/async/test_item_actions_runtime.py` for ASan/UBSan execution of
the real item runtime and scheduler. It covers admission and cost rejection,
owned-payload cleanup, generic owner/victim/object event cancellation, movement
and return, unequip/transfer invalidation, extraction, target death/identity reuse,
multiweapon limits, immutable revisions, disable/re-enable, clock catch-up,
unrelated waits, reservation settlement, and effects that kill/move/extract a
participant or disable/reload the engine while resolving.

The casting input gate and queue tests cover active devices with wait on and
off, disabled ordinary-spell abort, selective abort dequeue and retained
type-ahead. The existing scheduler, cancellation, typed-payload and spell
rejection suites remain regression gates. Build with `make -C src` and run
`./scripts/format.sh --check` for repository validation.

In-game A/B measurements require an actual pilot adapter (#293/#294). Passing
these framework tests does not establish artifact timing, mana budgets,
production eligibility, or completion of the parent epic.

The initial implementation was validated on 2026-09-13 against master
`9e0bfac62` in a disposable Linux container. The MariaDB server build, item-action
ASan/UBSan harness, all 12 nevent regression scripts, casting queue/gate tests,
spell scheduling rejection and abort tests, player timing contract, room leave
veto contract, difficulty property contract, and formatting checks passed. The
flat-file boot preflight also built its own server and verified game-loop entry,
health responses, clean shutdown, and controlled failure for missing world data.
The full repository regression suite and an enabled in-game pilot were not run
for this foundation slice.
