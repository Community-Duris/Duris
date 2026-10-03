# Renewable spell wards

Minor Globe, Spirit Ward, Greater Spirit Ward, and Globe retain their existing
spell-circle boundaries and explicit damage flags. A player ward has one finite
remaining lifetime. Time and damage intercepted at the circle-ward stage both
consume that lifetime. Earlier damage wards, absorption, and evasion run first;
melee, penetrating spells, self-damage, and zero damage do not consume it.

The default fresh budgets are deliberately independent of caster duration
bonuses. This prevents long Shaman casts from gaining several times a Globe's
damage capacity merely through their normal time duration.

| Ward | Fresh damage budget | Equipment duration | Automatic interval |
| --- | ---: | ---: | ---: |
| Minor Globe | 900 | 6 ticks | 3 ticks |
| Spirit Ward | 2,250 | 10 ticks | 5 ticks |
| Greater Spirit Ward | 3,600 | 12 ticks | 6 ticks |
| Globe | 3,200 | 8 ticks | 4 ticks |

The tuning properties `spell.ward.damagePerTick.minorGlobe`, `.spiritWard`,
`.greaterSpiritWard`, and `.globe` default to 150, 225, 300, and 400 respectively.
They multiply the equipment/base durations above to set the opening budget for
both cast and equipment sources. `spell.ward.equipment.spiritLevel` and
`.greaterSpiritLevel` explicitly set the two spirit base durations. Equipment
does not inherit the wearer's class. These are initial tuning choices; live-world
balance has not been measured.

Capacity is persisted in millionths of one damage point. After damage, the
expiry event is shortened in pulses; sub-pulse wear remains in the capacity
value. Time wear uses differences between absolute pulse boundaries, avoiding
loss from repeated one-pulse updates. One-point missiles and individual damage
over time events are charged separately without whole-hit rounding. The breaking
hit blocks only remaining capacity; overflow continues through later defenses.

One eligible pool is selected per damage call: Spirit Ward, Greater Spirit Ward,
Minor Globe, then Globe. Within a kind, the cast pool precedes the equipment
pool. Overflow never consumes a second pool on that same hit. Successful recasts
replace the cast pool and restore the full duration appropriate to that caster.
Group Globe applies this rule independently to every eligible recipient.

Equipment keeps one retained pool per wearer and ward kind. The lowest eligible
item UID identifies the current source; duplicate items do not add pools.
Removal pauses that wearer's equipment lifetime and renewal countdown, preserving
capacity and any broken state. Re-equipping or switching duplicate sources
resumes that pool. Independent cast affects survive equipment removal. The pool
belongs to the wearer, so transferring an item does not transfer another
character's cast or equipment affect history.

Renewals replace the equipment pool every half of its full original duration,
including an active weakened pool. Damage and manual casts never move this
deadline. A broken pool remains down until that deadline or an independent
manual cast. Missed renewals produce a single fresh pool. Untouched equipment
stays protective across renewals. The Vapor callback is treated as an equipment
Globe grant; its on-hit callback cannot refill a broken pool early.

Bard Protection retains its existing missing-effect reapplication rule and uses
the same finite cast pool. NPC-native flags, including elite dispel suppression,
retain their existing behavior. NPC attack selection can choose spells that wear
down finite player wards; prediction queries do not mutate their state.
Equipment affects cannot be removed through ordinary spell dispelling; cast
affects remain dispellable. Existing spell-save bonuses and spirit partial
reductions apply while their corresponding ward is active.

SQL saves, pfiles, and copyover retain capacity, source identity, broken state,
and relative deadlines. Offline time is paused. Managed pfile durations use
pulses, preserving fractional-tick renewal and damage wear. Ward-bearing
snapshot wire envelopes use the normalized accounting schema plus 13 (versions
20, 21, and 23 through 32); existing versions remain readable and retain their
quest, spell-application, crafting, and death evidence meanings. Death restitution
accepts only supported death request envelopes, including ward-bearing variants.

Migration `0056_spell_ward_durability` adds eight guarded columns to
`player_affects` and is registered in all three supported immutable histories.
The runtime schema fingerprints must match the upgraded database before boot.
Player help, the affect display, and GMCP expose remaining damage capacity,
remaining lifetime, source, and equipment renewal timing.

Run `python3 tests/async/test_spell_ward_durability.py` for the executable ward,
timer, overlap, recast, equipment-cycle, and snapshot recovery regression.
`run_spell_ward_persistence_journey.py` exercises weakened and broken ward state
through SQL save, cold restart, and exec copyover using an isolated loopback
database. It takes an absolute MariaDB server executable and requires the
disposable `TEST_DB_*` settings documented by `test_mysql_combat_journey.py`.
