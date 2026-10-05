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
| Giants, titan, avatar | 2.10–2.60 |
| Snow ogre | 3.00–4.00 |
| Insect, arachnid, flying animal, parasite | 0.65–0.85 |
| Dragon, dragonkin, dracolich, quadruped, carnivore, herbivore, centaur, purple worm | 1.20–1.65 |
| Plant, slime | 1.45–1.90 |

Role factors, in priority order: bard 0.85; multiclass 0.90; other caster 0.80; rogue/thief/assassin 0.90; other 1.00. Maximum HP, including equipment and buffs, is bounded by the upper end of that racial profile, 5,000 HP, and the old ordinary captured-body allowance. Non-elemental captures can exceed the former necromancer benchmark; elemental captures retain it alongside their heater limits.

At owner level 56 with full Infuse Life, a snow ogre warrior receives 4,546 HP at CHA 100 or 5,000 at CHA 130, compared with 5,427 on an ordinary old summon and 7,598 on its best old roll. Other racial ranges are unchanged: an ordinary ogre warrior still receives 2,545 at CHA 130. The 5,000 ceiling caps maximum HP after equipment/buffs and also rejects a larger saved ceiling. Existing 110% overheal allows a 5,000-max-HP capture to temporarily hold 5,500 current HP.

The old-body ceiling freezes the checked-in normal-mode racial Constitution and class HP factors. It uses the trained level, the old conversion formula and Infuse Life, without the random Charisma or elite bonuses. This conservative ceiling prevents low-HP races from gaining HP through the new profiles. Wild NPC property changes, zone difficulty and Chaos's wild HP division do not change this allowance. For example, at owner level 56, CHA 130 and Infuse Life 100, Bran receives 1,603 HP, A’den 835 and Xavier 974; Bran is not reduced to his 228-HP wild Chaos body.

Non-elemental melee uses necromancer-style dice, 0.70L hitroll, the captured prototype's existing base damroll (up to 100), and base armor -L. Final damroll, including equipment and buffs, caps at 100. The cap grants no extra damage: a level-56 hard-hitting body normally retains its old base damroll of 58.

Captured elementals use a deterministic body based on existing conjurer formulas, including terrain bonuses and innate earth skin/air flight. Lesser bodies cap at level 45; greater bodies cap at 53, or 55 for a level-56 owner. The heater's template HP/damroll (700/25) bounds greater captured elemental bodies, and the necromancer and old-body HP ceilings still apply. Conjurer creation retains its existing random HP rolls, templates and combat behavior. Full captured classes remain available, subject to resources.

Trained pets use neutral base attributes and a damage multiplier of 1 rather than wild NPC racial/zone stat tuning. Capture removes authored elite/ignore/no-bash/paralysis-immunity and automatic breath flags. Class buffs and equipment can still change combat stats within the HP and mana bounds. Captured non-elemental immaterial bodies use the player-style 10% takedown dodge; elemental defenses remain.

Only Summoner captures receive the new lifesteal limits: passive undead drain at most 10%, other supported lifesteal such as Vampiric Touch at most 25%, and healing/overheal at most 110% of maximum HP. Captured dracoliches do not stack the ordinary dracolich drain with undead drain. Existing lower lifesteal rates stay lower. Players, necromancer summons, conjurer elementals and other NPCs retain their existing rules.

### Historical captured HP limits

The old base-HP cap was 8,000. The following calculations use the checked-in normal configuration (`conFactor = 1`, `NpcPcRatio = 1`, elite multiplier 1.05), Infuse Life 100, and active world prototypes. Ordinary HP has no random Charisma adjustment; best-roll HP adds the maximum 40% Charisma bonus before Infuse Life. These are calculated summon bodies, not a measurement of historical production settings or buffed total HP. Authored HP dice and zone-loaded encounter HP are not the summoned-body formula.

| Capture | Wild level | Ordinary HP | Best old roll |
| --- | --- | --- | --- |
| Snow ogre tribal guard/guardian (87701, 87705, 87707) | 56 | 5,427 | 7,598 |
| Snow ogre tribeswoman/tribesman (87703, 87731) | 57 | 5,718 | 8,000 |
| Snow ogre search party guardian (87738) | 59 | 6,330 | 8,000 |
| Elite snow ogre guardian (87708) | 60 | 6,652 | 8,000 |
| Insane snow ogre berserker (87700) | 61 | 6,168 | 8,000 |
| Snow ogre deathknight (87704) | 61 | 6,667 | 8,000 |
| Huge ancient walking tree (42236; Naturalist) | 56 | 3,342 | 4,677 |
| Living current / large water elemental (45521, 71225; Mentalist) | 56 | 2,986 | 4,180 |

Level-57–61 captures require a greater orb for a level-56 owner; captures above level 56 cannot be conjured without one. The accounting branch blocks orb summoning while item accounting is active, so those 8,000-HP examples are unavailable through the orb path in active accounting mode. In the active prototype survey, the largest level-56 body was 7,598 on the best roll. The rework lowers the hard maximum-HP limit to 5,000, with racial and old-body limits still applying.

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

For example, Bran retains Warrior/Cleric/Antipaladin and his specialization, but loses his encounter body/immunities, gets 448 mana, and shares his casting circles. A level-31 Harrow bard retains Bard and its specialization with 186 mana and useful-pulse costs. Plane elementals retain their complete classes but receive a trained elemental body, bounded by heater, necromancer and old-body HP benchmarks, plus 4L mana and shared casting slots.

## Persistence and validation

The existing pet payload gains a version-2 extension for specialization, prepared resource slot, and HP ceiling. Version-1 payloads remain readable; legacy summoner pets are identified through the owner's spellbook and normalized on restore. Resource banks use existing stored player affects; no database migration is required. Invalid, expired, ineligible, or over-limit restored pets retain their saved equipment in the existing held-pet flow.

Focused checks: `test_summoner_rework.py` executes the production body/resource code with real spell-circle tables and necromancer recipes; `test_pet_restore_state.py` checks version-2 round trips and version-1 compatibility. Existing hydration, snapshot-capture, and regeneration/death checks cover the integration. These checks verify bounds and resource behavior, not party DPS equivalence; the numeric tuning still needs gameplay balance feedback.
