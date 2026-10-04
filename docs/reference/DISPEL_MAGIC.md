# Dispel Magic duration wear

This follow-up to the renewable ward work makes a resisted Dispel Magic consume
duration on each eligible timed spell it checks. A successful dispel still
removes the effect.

## Balance rules

| Target | Successful check | Failed check |
| --- | --- | --- |
| Ordinary timed character spell | Remove the spell | Remove 10% of remaining duration, rounded up in its timer units, with a minimum of one game tick |
| Finite cast or equipment ward | Remove the cast or break the equipment pool | Consume capacity and proportional duration equal to base Burning Hands damage |
| Timed magical barrier, including Wall of Stone | Remove both sides | Apply existing strength damage and the ordinary duration reduction to both sides |
| Portal with the existing door, wormhole, or ether-portal callback | Remove the matched pair under the existing level rules | Remove 10% of remaining lifetime, rounded up, with a minimum of one pulse; synchronize both existing decay timers |
| Moonstone or bloodstone object | Retain the ordinary enchantment check and its existing destruction chance | Keep the existing one-tick expiry cap; also shorten a nearly expired anchor by 10%, with a one-pulse minimum |

Each active ward source receives its own check; failed wall dispels reduce both
strength and duration. Ward wear rolls the base Burning Hands amount,
`4 * dice(5 + abs(level) / 10, 6)`, bounding level magnitude to 1 through
255, for an average of 140 capacity at level 56.

The spell preserves the existing portal balance: below character level 46 the
attempt has no effect, levels 46 through 49 succeed half
the time, and level 50 or higher succeeds. These thresholds use the character's
level, not the supplied spell level. The raidable restriction is
removed from entering, looking through, and dispelling portals. Commented copies
in the door and general portal-creation helpers have also been removed.

Examples:

- An ordinary spell with 30 ticks remaining loses 3 ticks.
- An ordinary spell with 9 ticks remaining loses 1 tick.
- An ordinary spell with at most one tick left expires immediately.
- A wall with 1,800 pulses remaining loses 300 pulses (one tick), in addition
  to its existing strength damage. The two sides retain the same strength and
  expiry deadline.
- A portal with 120 pulses remaining loses 12 pulses, rather than being
  exhausted by the ordinary one-tick minimum of 300 pulses.
- A stone anchor with 3,000 pulses remaining is capped at 300 pulses, as before.
  An anchor with 80 pulses remaining is shortened to 72 instead of being extended
  to 300. Dispelling an anchor can never move its expiry later.

## Module boundary

`src/magic/spell_dispel_magic.c` owns Dispel Magic's eligibility decisions,
checks, per-target wear policies, pair validation, and spell messages. Its
helpers and target dispatcher are private; the existing spell entry point is
unchanged. `spell_alignment_dispel.c` retains Dispel Good and Dispel Evil.

The shared ward module remains responsible for capacity debit, proportional
lifetime, breakage, and renewal. The spell chooses whether each ward's check
succeeded and how much failed wear to apply. Object callbacks retain entering,
looking, stabilization, throughput, decay messages, and anchor owner cleanup;
they no longer implement `CMD_DISPEL`. Portal creation and owner lifetime checks
remain in `spell_portals.c`.

A private dispatcher distinguishes a consumed custom attempt from the stone
anchor's deliberate continuation into ordinary enchantment dispelling. This
prevents a blocked portal check from destroying or stripping the object through
the normal enchantment path. It requires no registry, new public interface,
stored effect metadata, or general effect-system rewrite.

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
their existing one-point strength wear; other untimed wall objects retain their
existing level-based strength roll. Neither gains an expiry timer or an extra
damage packet. Ordinary nonmagical objects remain unaffected.

Portal counterpart lookup validates the destination before indexing the world,
then requires the same prototype and pair ID and a reciprocal destination.
Missing, invalid, or ambiguous counterparts consume the attempt without touching
another object or falling through to enchantment dispelling. Untimed portals
receive no duration wear. Legitimate one-way magic pools do not use these custom
portal callbacks and retain their existing handling.

Portal owner events retain only the caster ID and one-way flag. They use the
live object's room and destination to enforce the existing owner-position rule,
avoiding a stored pointer to a counterpart that may already have decayed. A
one-way portal schedules no owner event for a nonexistent second object.

Stone anchors keep their existing object decay callback to notify their owner
and remove the character's protected tracking affect. A missing decay affect
still receives the legacy one-pulse expiry repair; a missing event on an existing
decay affect now receives the same repair. Ordinary enchantment dispelling still
runs after the expiry is shortened, including its existing one-in-five
destruction chance when the object bears `ITEM2_MAGIC`.

Caster and target messages name a character spell and say whether it was
weakened or exhausted. Ward messages also identify cast or equipment source.
Barrier messages identify weakening, a successful dispel, or breakage. A partial
success no longer prints the generic total-failure message.

Effects explicitly protected from dispelling remain outside ordinary checks.
Unlimited character affects and NPC-native bits have no finite duration to
debit, so their existing successful/failed dispel behavior remains. Permanent
item enchantments and other special object callbacks retain their existing paths.
There are no new persistence fields or migrations.

## Focused verification

Run `python3 tests/async/test_spell_ward_durability.py`. Its existing production
spell/ward harness now also exercises ordinary spells, multiple affect entries,
saves and resistance, short-event deadlines, exhaustion, messages, paired wear
and removal for every magical wall type, and protected/unlimited effects. It also
covers the three custom portal handlers with non-raidable casters and travelers,
blocked/invalid/ambiguous pair handling, short portal lifetimes, untimed wall
damage, stone expiry caps and missing-event repair,
ordinary enchantment continuation, and the production portal action, anchor
decay, and portal owner callbacks. Event and world services are deterministic
fixtures; the portal travel body is a stub that records allowed enter/look
dispatch rather than moving a live character.

Build the server with `make -C src`. The existing ward persistence journey also
checks weakened and broken state across SQL save, restart, and exec copyover
using a disposable loopback database; it does not exercise the broader dispel
interaction over a live client.
