# Summoner rework

Captured pets retain their complete class mask and specialization. Their trained bodies and finite resources replace encounter-sized stats, elite immunities, and unlimited active abilities. Ordinary captures never exceed their owner's level. Existing capture eligibility and active-pet limits remain: four pets, combined levels at most `2 * owner level + 10`, and one pet of level 50 or higher.

## HP and combat bodies

For a non-elemental of trained level L, start with `120 + 0.30 * L²`. Multiply by the racial factor below, role factor, and `1 + Infuse Life / 500`. Charisma interpolates through the racial range: `q = clamp((CHA - 60) / 70, 0, 1)`.

| Race family | HP multiplier |
| --- | --- |
| Halfling, gnome, goblin, kobold, drow, grey elf, faerie | 0.70–0.90 |
| Human and other ordinary humanoids | 0.95–1.15 |
| Dwarf, duergar, troll, wight, revenant, gargoyle, golem, construct | 1.15–1.40 |
| Ogre, minotaur, firbolg | 1.60–2.00 |
| Giants, snow ogre, titan, avatar | 2.10–2.60 |
| Insect, arachnid, flying animal, parasite | 0.65–0.85 |
| Dragon, dragonkin, dracolich, quadruped, carnivore, herbivore, centaur, purple worm | 1.20–1.65 |
| Plant, slime | 1.45–1.90 |

Role factors, in priority order: bard 0.85; multiclass 0.90; other caster 0.80; rogue/thief/assassin 0.90; other 1.00. HP, including equipment and buffs, is bounded by the upper end of that racial profile and the strongest level-appropriate necromancer body. That benchmark uses existing undead/golem recipes, mean HP dice, actual spell unlock levels, and Infuse Life; it excludes artifact bonuses. Non-elemental melee uses necromancer-style dice, 0.70L hitroll and 0.55L damroll, with base armor -L.

Elementals use the same body builder as conjurer-created elementals, including terrain bonuses and innate earth skin/air flight. Lesser bodies cap at level 45; greater bodies cap at 53, or 55 for a level-56 owner. The heater's template HP/damroll (700/25) bounds greater elemental bodies, and the necromancer HP ceiling still applies. Conjurer creation also uses this deterministic builder, so matching trained elementals have matching bodies; this replaces the old random HP rolls. Full captured classes remain available, subject to resources.

Trained pets use neutral base attributes and a damage multiplier of 1 rather than wild NPC racial/zone stat tuning. Capture removes authored elite/ignore/no-bash/paralysis-immunity and automatic breath flags. Class buffs and equipment can still change combat stats within the HP and mana bounds. Captured non-elemental immaterial bodies use the player-style 10% takedown dodge; elemental defenses remain.

## Resources

| Resource or action | Budget / cost |
| --- | --- |
| Elemental mana | 4 × trained level |
| Bard mana | 6 × trained level |
| Other captured pet mana | 8 × trained level |
| Light active skills, e.g. kick, rescue, disarm, dirt toss | 12 mana per attempt |
| Standard active skills, e.g. bash, backstab, grapple, rage | 24 mana per attempt |
| Major active skills, e.g. breath, gaze, whirlwind, throat crush | 42 mana per attempt |
| Conventional spell | One existing shared spell-circle slot |
| Psionic spell | Existing mana rules: normally 7 × circle, with Spatial Focus discounts |
| Useful healing song pulse | 20 mana for the group |
| Useful flight application | 6 mana for the group |
| Useful flight movement regeneration | 3 mana per second for the group |
| Other useful support song pulse | 10 mana for the group |
| Aggressive song pulse | 24 mana |
| Riff | Its normal useful-pulse cost plus 30 mana |

Normal attacks and movement cost no pet mana. Ordered and autonomous active class attacks use the same costs. Conventional spells share one slot pool per circle across all captured classes, using the existing NPC/lich slot capacities and lich recovery timing. Psionics retain their mana-based casting. Insufficient resources prevent execution and tell the owner: **“a vortex looks too exhausted for that.”** (using the actual pet name).

Idle full-health healing pulses are free; applicable status cures count as useful. Existing support buffs cost again only when they need application. Flight regeneration is charged only while it contributes to recovering missing movement, once per group time interval. At the first flight level, 31, a bard has 186 mana: 6 for application plus 180 for 60 seconds of useful running. Echoes and riffs are also metered; exhaustion stops singing and cancels pending echoes.

Mana recovers by 2% of capacity every six seconds while quiet. Both mana and spell-slot recovery pause during owner combat/casting, for 20 seconds after owner movement commands, and during pet combat/casting/singing. Owner-side debt is retained per prepared slot (1–4), including fractional movement costs. Dismissal, changing prototypes, and restart cannot reset that debt; switching to a larger mana pool only gains the difference in capacity. Inactive mana debt continues recovering while the owner is quiet. Circle recovery resumes when the pet is present.

## Level-56 Chaos recipes

When Chaos is enabled, a level-56 summoner learns the approved recipes for their current specialization on using `conjure`. Grants are idempotent and use the existing spellbook persistence. These are learned options; normal eligibility, control limits, HP caps, and resource rules still apply.

| Specialization | Approved recipes (vnum; wild level) |
| --- | --- |
| Controller | Bran Boru (142408; 56), A’den the Bard (27035; 56), Xavier (82507; 56) |
| Mentalist | Earth Library sentry (35543; 53), Air Library sentry (35542; 53), small pech (30623; 56) |
| Naturalist | dragonkin seer (135214; 56), black scorpion (42204; 54), huge black warg (78483; 52) |

For example, Bran retains Warrior/Cleric/Antipaladin and his specialization, but loses his encounter body/immunities, gets 448 mana, and shares his casting circles. A level-31 Harrow bard retains Bard and its specialization with 186 mana and useful-pulse costs. Plane elementals retain their complete classes but receive the matching conjurer body, bounded by heater and necromancer HP benchmarks, plus 4L mana and shared casting slots.

## Persistence and validation

The existing pet payload gains a version-2 extension for specialization, prepared resource slot, and HP ceiling. Version-1 payloads remain readable; legacy summoner pets are identified through the owner's spellbook and normalized on restore. Resource banks use existing stored player affects; no database migration is required. Invalid, expired, ineligible, or over-limit restored pets retain their saved equipment in the existing held-pet flow.

Focused checks: `test_summoner_rework.py` executes the production body/resource code with real spell-circle tables and necromancer recipes; `test_pet_restore_state.py` checks version-2 round trips and version-1 compatibility. Existing hydration, snapshot-capture, and regeneration/death checks cover the integration. These checks verify bounds and resource behavior, not party DPS equivalence; the numeric tuning still needs gameplay balance feedback.
