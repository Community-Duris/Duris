# Device channels and scroll recitation

[#294](https://github.com/Community-Duris/Duris/issues/294) adds opt-in adapters to
the [shared item action runtime](ITEM_ACTIONS.md). Wands and staves channel;
scrolls recite all selected slots in their original order. The wand of wonder
has its own typed adapter for its native outcomes. Every switch ships off.

## Routing and configuration

| Property | Default | Accepted integer values |
| --- | ---: | --- |
| `itemActions.wands.enabled` | 0 | 0 or 1 |
| `itemActions.staves.enabled` | 0 | 0 or 1 |
| `itemActions.scrolls.enabled` | 0 | 0 or 1 |
| `itemActions.wonder.enabled` | 0 | 0 or 1 |
| `itemActions.<category>.windupPulses` | 8 | 4–600, clamped by shared bounds |
| `itemActions.device.<vnum>.enabled` | 1 | 0 or 1 |

The master `itemActions.enabled` must also be 1. Categories above are `wands`,
`staves`, `scrolls`, and `wonder`. Eight pulses are two seconds; the halfway
message is presentation only. These defaults are pilot timing, not a claim about
production balance. Nonfinite, fractional or out-of-range values reject new
admission. Changing effective configuration cancels that ability's pending work;
rollback never releases an effect or refunds a consumed resource.

Ordinary hooks run in `do_use` and `do_recite` before their legacy presentation,
wait or consumption. Master/category/template off retains the old route. A
selected new route that cannot start is suppressed without instant fallback.
An active action uses the existing shared input and melee gate. `abort` can pass
queued ordinary input; retained commands resume afterward. This does not borrow
personal spell slots, spell timing, `AFF2_CASTING`, or an ordinary wait event.

Separately, the legacy wand's anti-PK guard now inspects the parsed victim.
It previously ran while `tmp_char` was initialized to null, so that guard was
inert. The targeted repair retains legacy charge-before-parse consumption and
has its own regression cases for forbidden PC targets, parse failure, beneficial
spells and self targets. It does not claim other downstream PK defenses failed.

## Selection, resources and cancellation

At selection, validate each nonzero spell index and pointer, capture item level,
parse the target once, and copy bounded arguments (500 bytes maximum). Wands and
scrolls support local character/self/object targets and targetless/area spells.
World target flags are rejected before calling the legacy parser, which can
create temporary remote dummy characters. Ranged-capable spells use the normal
parser but must resolve to the original room; remote targets are rejected before
consumption. Remote device routing is not part of this pilot. Staff spells must support area, ignore, room-character or self
targets. Unsupported definitions consume nothing on the new route.

All required targets must be valid before admission and before each effect.
Checks include source UID/custody, original room, life, visibility, object
location, self restrictions, single-file adjacency, safe rooms/zones, peace,
gaze, nonoffensive state, combat permission and account-bound ownership. The
actor must be awake, at least resting, mobile, unstunned and in magic. Charmed
NPCs cannot activate devices. Scrolls require speech throughout; wands and
staves do not. No target is replaced by a later opponent.

An accepted wand/staff/wonder action consumes exactly one charge **after the
scheduler accepts the payload and before the first warning**. The exact last
charge works. Admission or scheduling rejection spends nothing. Abort, target
loss, death, transfer, rollback, or partial completion retain the cost.

An accepted scroll clears all three spell slots before the first warning. Its
physical item stays attached to the invocation during recitation. Completion
or cancellation marks it for extraction by `device_actions_pulse`, after the
current event or object transition has unwound. This avoids extracting an object
recursively inside its own transfer/unequip callback. It cannot be used twice,
and transfer or abort does not restore the ink. An already extracted scroll is
not extracted again. The cleanup queue is capped at 4096 entries.

Charges and consumed ink use the existing item save authority. This slice does
not add a synchronous durable ledger for ordinary devices. A crash before the
next item snapshot retains that authority's existing recovery limitations; a
saved blank scroll may remain as a harmless spent item if shutdown interrupts
deferred removal. Artifact mana's separate crash policy is documented in
[ARTIFACT_MANA.md](ARTIFACT_MANA.md).

## Effect mapping

| Source | Preserved selected behavior | New timing/lifetime policy |
| --- | --- | --- |
| Ordinary wand | `value[3]`, captured `value[0]`, `SPELL_TYPE_SPELL`; character/object or ignored arguments | One channel; validate original target again, then invoke once |
| Ordinary staff | `value[3]`, captured `value[0]`, `SPELL_TYPE_SPELL`; existing group/self rules | Capture origin; enumerate current room runtime IDs at release; reacquire actor/source/target after every spell |
| Scroll | Nonzero slots 1, 2, 3 in ascending order; captured item level; `SPELL_TYPE_SPELL`; individual typed targets and arguments | Delay the whole ordered sequence; stop remaining slots if any required captured target departs, dies, becomes invalid or is extracted |
| Wand of wonder 41350 | Native choice 1–20 and level 20–40 | One active channel; fixed selected outcome and charge; no wrapper double execution |

No beneficial scroll slot is moved ahead of a harmful slot. Loss of a required
target stops the whole remaining sequence, including when an earlier effect
causes that loss. The rule is deliberately conservative. A staff re-enumerates
at release, so entrants can be eligible and leavers cannot be hit. Harmful
room-character spells skip actor and groupmates; beneficial self-only spells
retain the old recipient-as-caster call. True area/ignore spells call the native
spell once. Room enumeration is capped at 4096; overflow dispatches no partial
target list. Native area spell internals retain their own semantics.

The wonder callback still performs its existing unconditional level roll, and
the new adapter selects its branch once before admission. Mapping is exact:

| Choice | Native action |
| --- | --- |
| 1–7 | Minor paralysis, cyclone, lightning bolt, darkness, concealment on original target, fireball, concealment on actor; each keeps `SPELL_TYPE_WAND` |
| 8, 9 | One mobile 5710 in both cases, including the legacy duplicate |
| 10 | `level + 10` mobiles 12803 |
| 11 | One mobile 97514 |
| 12 | Gem vnums 66034–66043; the legacy loop repeatedly rolls its 10–40 bound, then deals one damage per selected gem |
| 13, 14 | Native grass/fog room messages |
| 15, 16 | Age actor by one captured 1–50 roll |
| 17–20 | Native nothing-happens message |

Summoning, gem creation and aging remain native effects. Summon/gem loops check
the action token and reacquire identities before and after callbacks. Capturing
gem choices before object creation preserves branch distributions and bounds;
it does not promise the same global RNG interleaving as immediate object loads.
Self/room outcomes anchor the actor; targeted outcomes keep the original victim.

## Shared invocation ownership

`start_item_action_instance` owns an immutable per-invocation adapter, including
typed target IDs and copied arguments, through the normal scheduler lifecycle.
It avoids a permanent registration per use. Definition validation, source and
actor caps, commit, progress, cancellation and finish remain in the shared
engine. `item_actions_disable(id)` and reload cancel these invocations too.
Additional captured targets are exposed only as read-only identity predicates
to the common departure/extraction hooks. No raw game pointer survives a call.

## Verification

- `test_device_actions_runtime.py`: real command wrappers, real scheduler and
  adapters under ASan/UBSan; last/empty charges, scheduling rejection, invalid
  spell/target/permission, captured level/call/arguments, object movement,
  departure/return, source transfer, abort, silence/posture, rollback, scroll
  cleanup and slot ordering, staff group filtering/entrants/callback lifetime.
- `test_wonder_actions_runtime.py`: actual special callback, all twenty legacy
  and enabled outcomes, RNG bounds/capture, no second spend or dispatch,
  ownership/law/visibility, abort, reload and native-loop interruption.
- `run_device_actions_journey.py --binary bin/server/dms_new`: explicit real
  Telnet A/B using a disposable flatfile server, synthetic ordinary devices,
  account creation, production parsing, spell damage, typeahead and abort.
  Build with `make -C src PERSISTENCE_BACKEND=flatfile` and build the repository
  inspector as described by the existing flatfile combat journey. The fixture
  raises combatant health to measure timing; it is not a damage balance study.

The live journey records binary SHA-256 and measurements in an optional JSON
output. Keep those generated logs under `bin/`; do not commit player/state data.
Full rollout, persistent mana interaction and additional artifact families
remain tracked by #296/#297. No production enablement is performed here.
