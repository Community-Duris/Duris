# Dispel Magic duration wear

This follow-up to the renewable ward work makes a resisted Dispel Magic consume
duration on each eligible timed spell it checks. A successful dispel still
removes the effect. The PR is a draft and must remain unmerged until the balance
choices below are finalized.

## Draft balance choices

| Target | Successful check | Failed check |
| --- | --- | --- |
| Ordinary timed character spell | Remove the spell | Remove 10% of remaining duration, rounded up in its timer units, with a minimum of one game tick |
| Finite cast or equipment ward | Remove the cast or break the equipment pool | Consume capacity and proportional duration equal to base Burning Hands damage |
| Timed magical barrier, including Wall of Stone | Remove both sides | Apply existing strength damage and the ordinary duration reduction to both sides |

Confirmed: each active ward source receives its own check; failed wall dispels
reduce both strength and duration. Still provisional: the 10% duration amount,
the one-tick minimum, and the Burning Hands comparison for ward wear. The latter
rolls `4 * dice(5 + abs(level) / 10, 6)`, bounding level magnitude to 1 through
255, for an average of 140 capacity at level 56.

Examples under these draft settings:

- An ordinary spell with 30 ticks remaining loses 3 ticks.
- An ordinary spell with 9 ticks remaining loses 1 tick.
- An ordinary spell with at most one tick left expires immediately.
- A wall with 1,800 pulses remaining loses 300 pulses (one tick), in addition
  to its existing strength damage. The two sides retain the same strength and
  expiry deadline.

## Resolution and messages

Ordinary spells receive one check per spell type, even when the spell has
multiple non-adjacent affect entries. A failed check shortens all of its eligible
timed entries once. Each active finite ward pool is independent, including a
cast and equipment pool of the same kind. Broken and unequipped pools receive
no saving throw or wear roll. A saved or resisted check continues to the next
eligible spell.

Short affects use their scheduled event's remaining time, rather than their
original duration field, and reschedule that event. Ordinary tick affects retain
their tick timer. Duration exhaustion follows normal affect removal, including
the existing Call of the Wild unmorph path. Traversal checks affect liveness
after earlier removals so multi-entry spells cannot leave stale list pointers.

Ward wear reuses damage's capacity debit and proportional lifetime calculation;
it never flows into hit points. Dispelled equipment retains its existing renewal
deadline and broken state. Re-equipping and item callbacks do not refill it.

Barrier matching uses the opposite direction, destination, wall kind, and owner.
Both paired objects receive the same remaining strength and shorter deadline;
exhausting either strength or duration removes both through the existing decay
cleanup. Unrelated wall objects are not removed. Untimed outpost walls retain
their existing one-point strength wear and have no duration to debit.

Caster and target messages name a character spell and say whether it was
weakened or exhausted. Ward messages also identify cast or equipment source.
Barrier messages identify weakening, a successful dispel, or breakage. A partial
success no longer prints the generic total-failure message.

Effects explicitly protected from dispelling remain outside ordinary checks.
Unlimited character affects and NPC-native bits have no finite duration to
debit, so their existing successful/failed dispel behavior remains. Permanent
item enchantments and special object callbacks retain their existing paths.
There are no new persistence fields or migrations.

## Focused verification

Run `python3 tests/async/test_spell_ward_durability.py`. Its existing production
spell/ward harness now also exercises ordinary spells, multiple affect entries,
saves and resistance, short-event deadlines, exhaustion, messages, paired Wall
of Stone wear and removal, and protected/unlimited effects. Event and world
services in this harness are deterministic fixtures.

Build the server with `make -C src`. The existing ward persistence journey also
checks weakened and broken state across SQL save, restart, and exec copyover
using a disposable loopback database; it does not exercise the broader dispel
interaction over a live client.
