# Offensive weapon action pilots

Issue #293 connects Avernus, packed offensive weapon spells, and offensive
random-equipment procs to the shared [item action runtime](ITEM_ACTIONS.md).
All three category gates ship off. No balance values or production deployment
are approved by this implementation. [Artifact mana](ARTIFACT_MANA.md) is optional
for these pilots; ordinary zero-cost telegraphs use the same runtime.

## Selection and legacy mapping

| Route | Selection preserved | Accepted delayed effect |
| --- | --- | --- |
| Avernus, vnum 19730 | Existing `CMD_MELEE_HIT` callback and 1/25 roll; worn-source check | Captured `clamp(victim HP + 9, 0, 200)`, captured vamp HP cap, original victim |
| Packed `weapon_proc` | Existing `number(0, value[7]-1)`; packed slots; optional one-of selection | Captured spell IDs, `value[6]` power, `SPELL_TYPE_SPELL`, original victim or wielder |
| Random equipment | Existing set-piece eligibility and selection rolls; existing 15-second timer gate | Chosen offensive spell, level 50, `SPELL_TYPE_SPELL`, opponent captured at trigger time |

Avernus invokes `vamp(damage/2, captured_cap)` before its negative spell damage.
Damage retains `SPLDAM_NODEFLECT | SPLDAM_NOSHRUG | PHSDAM_NOREDUCE`. Healing is
based on selected damage, just as in the legacy callback; it is not retuned to
damage actually received. Neither half occurs if the original target or source
invalidates. Its speech-triggered stone skin and periodic hum retain their
existing paths and timer. Both melee dispatch and `single_stab`'s direct special
callback reach this same selection boundary; backstab does not gain new generic
packed-spell routing.

Packed bundles execute their selected effects in the original descending slot
order. If a bundle contains an offensive effect, the entire selected sequence
shares one windup; this intentionally delays the beneficial members as well.
Beneficial members target the wielder and are skipped if already present at
selection or resolution. A random selection is made once, including a beneficial
result selected from a mixed bundle. Pure beneficial bundles stay on their
instant legacy path and do not publish a new action or consume extra random
numbers. The legacy nonzero-count/sparse-slot behavior is preserved, rather than
silently compressing sparse bundles into a different effect sequence.

Random equipment's defensive `CMD_GOTHIT` and `CMD_GOTNUKED` routes, self-only
effects, and beneficial effects stay legacy. Its offensive route historically
uses `GET_OPPONENT` at the trigger; that identity is captured there and never read
again to choose a completion target. Acceptance commits the existing timer once.
Cancellation retains it. Both legacy and new paths retain the callback's false
return value, so unrelated dispatcher behavior is unchanged.

## Timing, eligibility, and cancellation

An opted-in definition copies up to three selected typed effects into each
action. Later object edits, opponent changes, or random calls cannot rewrite the
selected spell, power, target policy, or Avernus heal cap. The engine retains its
source UID, actor/target runtime identities, original room, and equipment slot.

The default windup is eight pulses (two seconds), with a progress beat halfway
through. The engine clamps durations to its reaction floor and maximum. A beat
that would lie outside a shortened duration moves to its midpoint. Monotonic
deadlines prevent scheduler catch-up from removing the real reaction interval.
Every audience receives direct item messages independent of class/psionic cast
presentation. Avernus has its own drain completion messages; packed custom extra
descriptions are emitted once at release. A fizzle names the live source when
available, or Avernus/a weapon after source extraction.

Admission and each resolution step use `CanDoFightMove` against the original
victim for harmful selections. This rechecks current visibility, reach, room
safety, and offensive eligibility; there is no replacement opponent. Movement
away and back, death, extraction, disarm, transfer, data reload, and configuration
rollback cancel the action. A callback that removes a participant ends the
remaining bundle. Ordinary casting continues alongside passive weapon actions.

Selected actions rejected for busy source/wielder, unavailable mana, invalid
configuration, scheduler failure, or changed eligibility are suppressed. They
never release the legacy instant effect. Disabling a category or template is an
explicit rollback: pending actions cancel and future selections use legacy.

## Configuration and payment

| Property | Default | Meaning |
| --- | ---: | --- |
| `itemActions.avernus.enabled` | 0 | Avernus offensive callback |
| `itemActions.weapons.enabled` | 0 | Packed offensive/mixed bundles |
| `itemActions.randomWeapons.enabled` | 0 | Offensive random equipment |
| Each category's `.windupPulses` | 8 | Integer 4–600, also subject to engine bounds |
| `itemActions.weapon.<vnum>.enabled` | 1 | Independent template rollback under the category/master gates |
| `itemActions.weapon.<vnum>.manaCost` | 0 | Fixed-point debit; 1,000 units equal one mana point |
| `itemActions.weapon.<vnum>.manaCapacity` | 0 | Pool capacity in the same units |
| `itemActions.weapon.<vnum>.manaRegen` | 0 | Units regenerated per second |
| `itemActions.weapon.<vnum>.manaRevision` | 1 | Explicit monotonic resource-profile revision |

Numeric values must be finite integers; enabled switches accept only 0/1. Mana
amounts accept 0–10,000,000 units and revisions 1–10,000,000. Positive cost requires
capacity at least equal to cost and the shared mana gate enabled. Paid profiles
use the template vnum as profile identity. Changing capacity/regeneration requires
a higher mana revision; invalid publication suppresses the route.

Payment occurs only after the scheduler owns the action, before any warning.
New/cold UID reserves follow the mana runtime's empty initialization and durable
readiness rules. A rejected debit changes neither the timer nor reserve. Once
accepted, costs and random-equipment cooldowns remain spent after ordinary
cancellation, partial effects, operator rollback, or technical interruption.
There is no refund or refill on re-enable. The documented bounded persistence
acknowledgement window from #292 applies; this adapter does not add another ledger.

## Verification

`python3 tests/async/test_weapon_actions_runtime.py` compiles the production
Avernus helper/callback, packed dispatcher, random callback, action engine, and
scheduler with ASan/UBSan. Its world/effect endpoints record flags, order, targets,
messages and payment. It covers failed selection, original parameter capture,
speech/periodic isolation, direct backstab callback, progress and catch-up,
two weapons with ordinary casting, exhausted/unready mana, rejected scheduling,
busy suppression, cancellation, partial bundles, pure/mixed/sparse/random spells,
and defensive random-equipment parity. The real mana arithmetic is used at the
debit boundary; worker, repository, and game-bridge persistence tests live in #292.

The explicit live test is:

```sh
make -C src PERSISTENCE_BACKEND=flatfile
python3 tests/async/test_flatfile_player_repository.py \
  --build-inspector bin/tests/coin-death-inspector
python3 tests/async/run_weapon_actions_journey.py \
  --binary bin/server/dms_new --output bin/weapon-actions-journey.json
```

It boots isolated flatfile authorities and a miniature world, creates real
accounts/characters over Telnet, equips an NPC with a synthetic magic-missile
weapon, and exercises actual combat, spell damage, warning output, and flee.
The fixture uses the ordinary `value[7]=1` selection rate; it adds no production
debug endpoint or replacement combat implementation. It uses increased health
properties for the synthetic NPC/player and a normal starter mace, because the
server recalculates NPC hit points after loading raw area dice. Its three cases are legacy,
enabled completion, and enabled successful reaction. This is a packed-weapon
combat pilot; Avernus's bespoke drain parity is separately exercised by the
sanitizer callback test.

On 2026-09-13, the initial live run measured 2.002 seconds from warning to damage
and a 0.250-second response from warning to successful room exit. The fleeing
player observed the fizzle and no proc damage. The legacy case emitted its spell
without a windup warning. These are single-run disposable observations, not
network latency guarantees or a production balance study.

The deterministic harness additionally drives 200 already-selected Avernus procs
at four per second through a two-second windup and reports completed/suppressed
counts. The measured result was 200 completed legacy drains versus 25 completed
and 175 suppressed delayed drains (87.5% suppressed). This deliberately saturated
workload measures pending suppression; it
does not model the natural 1/25 trigger probability or infer PvP balance. Enabled
timing is a throughput change even when effect power and trigger probability are
preserved. Sustained mana use and later artifact-specific rollout evidence remain
tracked by #297.
