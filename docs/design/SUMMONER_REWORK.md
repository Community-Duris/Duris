# Summoner rework

Captured pets retain their complete class mask and specialization. Their trained bodies and finite resources replace encounter-sized stats, elite immunities, and unlimited active abilities. Ordinary captures use the lower of their prototype level and owner's level, including elementals; there are no additional level-45/53/55 elemental caps. The existing greater-orb path for prototypes above level 56 retains the prototype level, including after restoration. Orb eligibility, consumption and the accounting-mode restriction remain unchanged. Existing capture eligibility and active-pet limits remain: four pets, combined levels at most `2 * owner level + 10`, and one pet of level 50 or higher. Creation and restoration count the actual trained level, including the full orb level, toward those limits.

Specialization is available at level 30. Learning new captures requires a specialized Summoner in both solo and group kill paths. Unspecialized Summoners can use `conjure` from level 21 with the default recipe (400003); they receive no specialization bonuses.

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

Role factors, in priority order: bard 0.85; multiclass 0.90; other caster 0.80; rogue/thief/assassin 0.90; other 1.00. Maximum HP, including equipment and buffs, is bounded by the upper end of that racial profile, 5,000 HP, and the old ordinary captured-body allowance. Non-elemental captures can exceed the former necromancer benchmark. Elemental captures retain the necromancer ceiling; specialized Mentalist elementals receive the approved exception to the old-body allowance described below.

At owner level 56 with full Infuse Life, a snow ogre warrior receives 4,546 HP at CHA 100 or 5,000 at CHA 130, compared with 5,427 on an ordinary old summon and 7,598 on its best old roll. Other racial ranges are unchanged: an ordinary ogre warrior still receives 2,545 at CHA 130. The 5,000 ceiling caps maximum HP after equipment/buffs and also rejects a larger saved ceiling. Existing 110% overheal allows a 5,000-max-HP capture to temporarily hold 5,500 current HP.

The old-body ceiling freezes the checked-in normal-mode racial Constitution and class HP factors. It uses the trained level, the old conversion formula and Infuse Life, without the random Charisma or elite bonuses. This conservative ceiling prevents low-HP races from gaining HP through the new profiles. Wild NPC property changes, zone difficulty and Chaos's wild HP division do not change this allowance. For example, at owner level 56, CHA 130 and Infuse Life 100, Bran receives 1,603 HP, A’den 835 and Xavier 974; Bran is not reduced to his 228-HP wild Chaos body.

Non-elemental melee starts with necromancer-style trained dice, 0.70L hitroll, the captured prototype's existing base damroll (up to 100), and base armor -L. All captures blend trained dice count and size halfway toward stronger normal prototype dice, rounding to whole dice. Matching Naturalist animal/plant captures start with one additional trained die and weight the blend 80% toward the prototype. Preserve the trained floor if it is already stronger; the blend cannot exceed the stronger prototype's average. The prototype reference freezes the ordinary pre-rework conversion at the trained level, excluding elite/zone boosts, so wild difficulty and repeated restoration cannot amplify the dice. Hitroll restores half the positive gap to the original normal conversion, rounded up and limited to +15 base hitroll. Its reference uses the converter's melee/dragon/demon/giant classification and level-based 35/25 limits. A level-56 non-elemental's trained 39 hitroll already exceeds the old normal formula and receives no increase. Final damroll, including equipment and buffs, caps at 100. The cap grants no extra damage: a level-56 hard-hitting melee body normally retains its old base damroll of 58.

Captured elementals use deterministic bodies based on existing conjurer formulas, including terrain bonuses and innate earth skin/air flight. Greater bodies require owner level 51 and trained pet level at least 46. For captures outside the Mentalist specialization, the heater's template HP/damroll (700/25), necromancer ceiling and old-body HP allowance still apply to the initial body. Every captured elemental then restores half the positive gap between its trained and original normal base damroll, rounded up; a stronger trained value receives no increase. The reference shares the frozen level/class conversion used by prototype dice and excludes encounter boosts, equipment and buffs. At owner level 56 and CHA 130, level-53 Air Library sentries move from 28/55 to 42/56 base damroll in neutral/favorable terrain; Earth Library sentries move from 39/66 to 48/66. Small pech's favorable 66 remains 66 because it already exceeds the normal level-56 reference of 58. Conjurer creation retains its existing random HP rolls, templates and combat behavior. Full captured classes remain available, subject to resources.

Every captured body keeps the original mobile prototype's size, including authored sizes that differ from the racial default. Training does not resize elementals for terrain. On restore, capture payloads keep the size freshly loaded from the prototype instead of an older saved, terrain-forced size. Other summoned pet kinds retain their existing saved-size restoration.

## Specialization strengths

These bonuses require a Summoner owner with the matching current specialization and level at least 30. Mentalist bonuses require an elemental capture; Naturalist bonuses require an animal or plant capture. The default humanoid recipe, wild NPCs, other classes' summons and players gain none of these bonuses. Each pet receives its own bonus once; additional pets do not multiply it.

| Owner level | Mentalist elemental mana | Mentalist hitroll/damroll premium | Naturalist ordinary melee damage | Naturalist physical damage reduction | Naturalist health regeneration |
| --- | --- | --- | --- | --- | --- |
| 30–40 | 5L | 5% | 5% | 5% | 10% |
| 41–50 | 6L | 10% | 10% | 10% | 15% |
| 51+ | 6L | 10% | 10% | 15% | 20% |

Mentalist HP is weighted toward the original captured body. Let P be that prototype's old ordinary summoned HP under the frozen normal configuration, calculated at its trained level with the owner's Infuse Life. Let C be the strongest comparable conjurer HP in neutral terrain, at that trained level and owner Charisma/Infuse Life, without a Mentalist premium. Set base HP to `clamp(0.80 * P + 0.20 * C, 0.90 * P, 1.10 * P)`, apply a terrain multiplier of 0.95/1.00/1.05 for hostile/neutral/favorable terrain, then round to the nearest HP. There is no additional universal HP premium. Charisma influences only C. The necromancer HP benchmark and 5,000 hard cap remain.

Lesser C uses the maximum neutral ordinary dice roll. Greater C takes the larger of the regular greater formula and the strongest neutral specialized template for that elemental family: air/void 600/20, water/ice 600/25, fire 700/25, earth 800/30 (template HP/damroll). C is a bounded contribution, not a minimum HP guarantee. Fragile air prototypes can remain below a conjurer's HP; their full captured classes, mana and slot recovery provide additional utility. Favorable terrain retains haste. The existing 5%/10% premium applies only to hitroll/damroll, rounded up to whole points.

| Capture | Old ordinary HP | Mentalist neutral / favorable HP | Mentalist mana |
| --- | --- | --- | --- |
| Air Library sentry, level-56 owner, CHA 130 / Infuse 100 | 837 | 908 / 953 | 318 |
| Earth Library sentry, same owner | 1,742 | 1,632 / 1,713 | 318 |
| Small pech, same owner; trained level 56 | 2,042 | 1,910 / 2,005 | 336 |
| Living current, same owner; trained level 56 | 2,986 | 2,687 / 2,822 | 336 |
| Level-30 eddy of smoke, level-30 owner, CHA 130 / Infuse 100 | 174 | 191 / 201 | 150 |
| Level-30 fire elemental, same owner | 439 | 446 / 469 | 150 |

At level 56, A'den and Xavier retain base damroll 73 and use blended 8d7 dice, averaging 105 base damage per unarmed hit. The Naturalist dragonkin seer retains the same base damroll and uses 9d8; its existing 10% ordinary melee bonus gives 124.85 average base hit damage, 18.9% above those Controller examples. The scorpion/warg average 97.9/96.8, the ancient tree 99, and Bran 82.5. These are dice-plus-base-damroll comparisons, before attributes, equipment, defenses and other modifiers, not party DPS guarantees. Naturalist spell/breath/active-skill damage is unchanged.

### Basic melee DPS comparison

These comparison tables record the earlier Strength/prototype-trait trial, before the additional Dexterity, accuracy and elemental damroll blends described in this document. Their DPS figures are historical baselines, not recalculated estimates for the current implementation.

The combat path uses average body dice plus `(damroll + Strength damage bonus) × damroll.mod × 0.95`; the checked-in `damroll.mod` is 0.75 and the 0.95 is the average 90–100% damroll roll. Apply Naturalist's ordinary melee bonus after that. Estimated basic melee DPS is this hit amount multiplied by expected attacks per volley, divided by volley duration. Attack skills, haste and racial combat pulses matter independently of body dice. The dispatcher runs every 0.25 seconds and the countdown gives `(integer combat pulse + 1) × 0.25` seconds between volleys for an idle, standing attacker with no pulse effects.

The following source-calculated examples use a level-56 owner, no weapons or externally supplied buffs, a neutral NPC damage multiplier, every attempted swing landing, and no criticals, damage procs, active attacks or spell damage. Summoner bodies blend Strength halfway toward stronger normal racial factors and keep Dexterity 100. Authored haste/blur are included while the initial casts remain active on non-elementals; eligible elemental effects and non-spell traits such as flurry remain intrinsic. The greater dracoliches have racial Strength 250 after their creation stat correction and innate haste; the silver variant's Swordsman specialization also adds quadruple-attack chances. Ordinary vampire/lich rows assume base attributes 100 before their existing racial factors and maximum pet levels 50/46. Values are approximations before integer rounding and target defenses, not measured fight DPS. This unarmed comparison cannot measure Bran's additional attacks from equipping his third/fourth weapon slots.

| Pet | Body dice | Base damroll | Average ordinary hit | Estimated basic melee DPS |
| --- | --- | --- | --- | --- |
| Necromancer greater bronze/gold dracolich | 6d7 | 80–100 | 113–127 | 92–104 |
| Necromancer greater silver dracolich | 6d7 | 80–100 | 113–127 | 113–128 |
| Necromancer vampire | 5d5 | 21–29 | 41–46 | 33–38 |
| Necromancer lich | 5d5 | 19–27 | 33–39 | 18–21, plus spell damage |
| Controller Bran | 7d6 | 58 | 77 | 110 |
| Controller A'den | 8d7 | 73 | 93 | 61 |
| Naturalist dragonkin seer | 9d8 | 73 | 114 | 66 |
| Naturalist black scorpion | 8d7 | 57 | 90 | 87 |
| Mentalist Earth Library sentry, neutral / favorable | 8d6 | 39 / 66 | 69 / 89 | 47 / 60 |
| Mentalist Air Library sentry, neutral / favorable | 8d6 | 28 / 55 | 57 / 76 | 48 / 64 |

The seer's ordinary hit is about 23% above A'den after Strength and damroll weighting, but its slower racial timing and Druid attack skill reduce the basic DPS advantage to about 8%. The scorpion has faster Insect timing and Warrior extra attacks, so it is a stronger sustained melee example. Bran approaches the greater silver dracolich benchmark and can exceed greater bronze/gold basic DPS. A captured pet's full class kit can add burst damage or utility; finite mana/shared spell slots bound that sustained contribution. Casting can also suppress ordinary melee volleys. Weapons replace a player pet's body dice, so equipping them changes this comparison.

### Before / resource rework / prototype-trait trial

Old values include authored combat traits and normal racial Strength/Dexterity, with raw attribute rolls 91–100. The middle column shows the prior rework with neutral attributes and stripped combat traits; the last is the earlier Strength/trait trial with its authored attack buffs active. Base body HP, damroll, mana and slots did not change in that trial. Normal spell modifiers can alter current combat stats within the existing limits. The old figures use ordinary normal-mode body conversion, including Bran's authored elite dice bonus, without random additional elite dice or encounter boosts. Non-elemental haste/blur now expire normally, so their rows do not describe indefinitely sustained damage without recasting.

| Capture | Old basic DPS | Prior rework | Strength/trait trial | Trial change from old |
| --- | --- | --- | --- | --- |
| Bran | 126.74–128.79 | 44.29 | 110.18 | -13–15% |
| A'den | 68.14–68.61 | 39.20 | 60.98 | -11% |
| Xavier | 37.03–37.53 | 32.91 | 32.91 | -11–12% |
| Snow ogre tribal guard | 82.69–86.18 | 44.29 | 69.56 | -16–19% |
| Earth Library sentry, neutral / favorable | 62.15–63.60 | 31.62 / 56.99 | 47.27 / 60.39 | -24–26% / -3–5% |
| Air Library sentry, neutral / favorable | 89.34–91.36 | 34.30 / 63.80 | 47.59 / 63.80 | -47–48% / -29–30% |
| Small pech, neutral / favorable | 47.25–48.34 | 34.88 / 61.39 | 37.43 / 64.88 | -21–23% / +34–37% |
| Black scorpion | 79.04–80.41 | 62.38 | 86.94 | +8–10% |
| Huge black warg | 60.83–61.90 | 47.69 | 66.91 | +8–10% |
| Dragonkin seer | 63.18–64.00 | 40.96 | 65.72 | +3–4% |
| Ancient walking tree | 49.81–51.02 | 50.81 | 53.03 | +4–6% |

Air had the largest remaining melee reduction in this trial. Its old racial Dexterity of about 164–180 supplied two probabilistic extra unarmed attacks; trial Dexterity 100 supplied neither, and neutral elemental damroll was 28 versus the old 56. The current additional blend raises intrinsic Mentalist air Dexterity to 140, restoring the first Dexterity attack opportunity while staying below the second's 150 threshold, and raises neutral Air Library base damroll to 42. Favorable pech receives no additional base damroll. These changes require a new DPS comparison before drawing current party-power conclusions.

Mentalist conventional spell slots take 20% less time to recover; Controller uses the baseline; matching Naturalist captures take 20% more time. The existing combat, casting, singing and owner-travel pauses still prevent recovery while busy. The larger Mentalist mana pool and faster quiet slot recovery improve sustained casting without removing exhaustion or prepared-slot debt, while Naturalist trades slower spell recovery for stronger physical attacks.

Naturalist's damage bonus applies only to the ordinary melee attack path, with no bonus to active-skill, spell, breath or healing amounts. Its physical mitigation multiplies damage after existing defenses, respects attacks that bypass reduction, and does not reduce spell or untyped raw damage. Faster health regeneration applies only to positive existing regeneration outside pet combat; it cannot create healing in no-heal rooms, rescue a dying pet, or accelerate overheal decay. Naturalist max HP, mana/skill costs and damage-stat caps stay unchanged. Controller uses the common halfway dice blend and baseline spell recovery.

Trained pets use raw base attributes of 100 and a damage multiplier of 1. Strength applies `100 + ceil(max(0, normal racial Strength factor - 100) / 2)` using a frozen normal-mode table, rather than live wild NPC tuning. Examples are Wight 118, Snow Ogre 175, Earth Elemental 145, Dragonkin 120 and Insect/Animal 110; weaker racial factors keep 100.

Other restored intrinsic attributes use `min(cap, 100 + round(max(0, P - 100) * weight))`, where P is the frozen normal racial factor for the converted prototype. Their reference assumes raw attribute 100 and does not copy random wild stat rolls or already-buffed saved values. Constitution, Karma and Luck retain the neutral factor, leaving the explicit HP and resource formulas in control.

| Attribute | Weight of the original bonus above 100 | Intrinsic factor cap |
| --- | --- | --- |
| Dexterity, eligible Mentalist elemental | 50% | 145 |
| Dexterity, other captures | 25% | 145 |
| Agility, Intelligence, Wisdom, Power, Charisma | 25% | 125 |

The Mentalist Dexterity weight requires the matching owner specialization and level 30. A runtime-only training weight supplies the same result during preview and hydration before the owner link exists; it is recomputed rather than added to the save format. Once linked, current owner eligibility determines the weight. Low original factors do not reduce the trained 100 baseline. Normal stat/max-stat buffs still work above these intrinsic limits, and recalculation/restoration cannot compound the blend. For example, Mentalist air has Dex 140, Agi 125 and Pow 113; Grey Elf bard captures have Dex 103, Agi 105, Pow 101, Int 104, Wis 105 and Cha 105; dragonkin captures have Dex/Agi 103, Pow/Int/Wis 105 and Cha 100. Mana capacities and prepared-slot counts retain their explicit budgets. The existing NPC slot-time formula still responds to casting attributes; specialization recovery multipliers and combat/casting/singing/travel pauses remain in force.

### Prototype abilities and initial spell buffs

| Capture type | Prototype spell buffs | Non-spell traits |
| --- | --- | --- |
| Non-elemental captures | Cast once at successful creation with the pet as caster and target, using the existing spell's normal duration, modifiers and restrictions | Retain eligible intrinsic traits such as four arms, flurry, ultravision and awareness |
| Elemental captures | Keep eligible prototype affect bits permanently, including authored invisibility, detect invisible, haste, wards, protections, auras and regeneration | Retain eligible intrinsic traits |

The spell mapping covers invisibility and detection, haste/blur, armor/skins, shields, globes, protections, wards, regeneration, battle ecstasy, Vampire Form/Vampiric Touch and other supported spell-backed effects. Temporary form and size spells also use normal durations; the underlying body retains the prototype's size. These initial casts are a creation grant and do not spend or replenish the finite prepared pool; Vampire's player assimilation cannot wipe it. Later ordinary casting uses the existing resource rules. Effects with no corresponding spell remain intrinsic, rather than disappearing through an allowlist. Anatomical four arms stays permanent, restoring Bran's extra weapon-slot capacity; it does not give extra bare-handed attacks.

Elementals preserve the loaded prototype's effects; this does not grant every air elemental an effect that its own prototype lacks. Air captures with authored invisibility, detect invisible and haste keep those effects intrinsically. Elemental Deflect is also retained. Temporary spells that share a permanent bit cannot erase it when they expire, and the existing temporary suppression mechanism still applies. The converter's exclusions for transient sleep, casting, paralysis and similar states remain. Non-elemental Deflect retains its old capture exclusion.

On restoration, intrinsic traits come from the freshly loaded prototype, recovering effects stripped by older rework saves. Saved terrain bonuses are discarded before terrain is recalculated. The initial spell casts run only on fresh creation and never on previews, retraining or login. Existing pet persistence does not save ordinary timed spell affects, so these temporary buffs are lost on pet restoration rather than refreshed or turned permanent; no save-format extension is added for them.

Full captured classes and specializations remain available with existing resource costs. Capture still removes authored elite/ignore/no-bash/paralysis-immunity flags. Prototype breath flags now remain, with ordered and autonomous breath executions admitted by the same 42-mana check in `BreathWeapon`; exhaustion prevents execution. Legacy restored captures recover authored breath flags too. Class buffs and equipment can change combat stats within the HP and mana bounds. Captured non-elemental immaterial bodies use the player-style 10% takedown dodge; elemental defenses remain. Other classes' pets retain their existing attribute, ability and trait restoration paths.

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
| Elemental mana | 4 × trained level; matching Mentalist 5L at owner 30–40, 6L at 41+ |
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

Normal attacks and movement cost no pet mana. Ordered and autonomous active class attacks use the same costs. Conventional spells share one slot pool per circle across all captured classes, using the existing NPC/lich slot capacities and lich recovery timing (20% shorter for matching Mentalist elementals, 20% longer for matching Naturalist animals/plants). Psionics retain their mana-based casting. Insufficient resources prevent execution and tell the owner: **“a vortex looks too exhausted for that.”** (using the actual pet name).

Idle full-health healing pulses are free; applicable status cures count as useful. Existing support buffs cost again only when they need application. Flight regeneration is charged only while it contributes to recovering missing movement, once per group time interval. At the first flight level, 31, a bard has 186 mana: 6 for application plus 180 for 60 seconds of useful running. Echoes and riffs are also metered; exhaustion stops singing and cancels pending echoes.

Mana recovers by 2% of capacity every six seconds while quiet. Both mana and spell-slot recovery pause during owner combat/casting, for 20 seconds after owner movement commands, and during pet combat/casting/singing. Owner-side debt is retained per prepared slot (1–4), including fractional movement costs. Dismissal, changing prototypes, and restart cannot reset that debt; switching to a larger mana pool only gains the difference in capacity. Inactive mana debt continues recovering while the owner is quiet. Circle recovery resumes when the pet is present.

## Level-56 Chaos recipes

When Chaos is enabled, a level-56 summoner learns the approved recipes for their current specialization on using `conjure`. Grants are idempotent and use the existing spellbook persistence. These are learned options; normal eligibility, control limits, HP caps, and resource rules still apply.

| Specialization | Approved recipes (vnum; wild level) |
| --- | --- |
| Controller | Bran Boru (142408; 56), A’den the Bard (27035; 56), Xavier (82507; 56) |
| Mentalist | Earth Library sentry (35543; 53), Air Library sentry (35542; 53), small pech (30623; 56) |
| Naturalist | dragonkin seer (135214; 56), black scorpion (42204; 54), huge black warg (78483; 52) |

For example, Bran retains Warrior/Cleric/Antipaladin and his specialization, but loses his encounter body/immunities, gets 448 mana, and shares his casting circles. A level-31 Harrow bard retains Bard and its specialization with 186 mana and useful-pulse costs. Mentalist plane elementals retain their complete classes and prototype size, receive HP weighted toward their original bodies, and use 5L/6L mana and shared casting slots with faster quiet recovery. Naturalist animals and plants retain the existing HP limits with improved ordinary melee, physical durability and health recovery between fights, trading slower spell recovery for those strengths.

## Persistence and validation

The existing pet payload gains a version-2 extension for specialization, prepared resource slot, and HP ceiling. Version-1 payloads remain readable; legacy summoner pets are identified through the owner's spellbook and normalized on restore. Resource banks use existing stored player affects; no database migration is required. Invalid, expired, ineligible, or over-limit restored pets retain their saved equipment in the existing held-pet flow.

Focused checks: `test_summoner_rework.py` executes the production body/resource code with real spell-circle tables and necromancer recipes, plus the production Strength/Dexterity/Agility/Power/Intelligence/Wisdom/Charisma calculations and selected production buff spells. It covers intrinsic caps and normal stat buffs, the Mentalist level gate and current owner eligibility, immediate stat rebuilding before the owner link, frozen values despite mutated wild factors, repeated preview/restoration without compounding, the positive-gap hitroll limit and elemental damroll across terrain, and isolation from players/wild NPCs/other pets. Existing coverage includes normal initial spell durations, permanent four arms, temporary-buff removal, elemental affect permanence through overlapping spell expiry/suppression, non-recasting restoration, recovery of authored traits from older payloads and removal of stale terrain haste. `test_pet_restore_state.py` checks version-2 round trips and version-1 compatibility. Existing hydration, snapshot-capture, ward, attack-continuation and regeneration/death checks cover the integration. These checks verify bounds and resource behavior, not party DPS equivalence; the numeric tuning still needs gameplay balance feedback.
