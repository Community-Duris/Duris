# DurisMUD Chaos-Mode Starting Gear Reference

Issue #69: CHAOS Gear Sets

Status: generated from the July 2026 production snapshot and wired into the local `chaos-eq-system` branch. This document is a design/reference artifact; live balance still needs Chaos playtesting.

## Decision summary

- Grant one nested starter bag through the existing durable creation-grant path instead of queueing every item as a separate critical write.
- Provide two operator-selectable profiles: `standard` (observed high-end gear plus explicit class fundamentals) and `enhanceable` (strict boot-enhance-index-compatible alternatives).
- Begin the Chaos grant after rules acceptance and the durable character baseline, before `CON_RMOTD`/`CON_PLAYING`, so persistence overlaps the last creation screens.
- Do not block commands for this pre-entry grant; announce `Your Chaos Equipment has been prepared!!` only after durable completion and entry.
- When approval mode is enabled, withhold Chaos preparation in `CON_ACCEPTWAIT` and schedule it only from the successful staff approval transition.
- Keep normal equipment portable at the item-data level and apply runtime `can_char_use_item()` and body-slot checks before putting an object in the bag.
- Treat a missing object, nesting failure, or grant-queue failure as a fail-closed kit failure; skip individual objects rejected by runtime usability or unavailable race/body slots and continue with the remaining kit.
- Keep consumables inside the bag, with bounded starter quantities based on high-level carried/container usage rather than copying observed stockpiles.
- Treat Bard instruments, the master spellbook, and the Shaman three-sphere totem as explicit fundamentals instead of accidentally filtering them as ordinary gear.

## Evidence snapshot

- Cohort: active characters at level >= 50.
- Characters: 131; level 51+: 100; level 56+: 26; observed maximum: 62.
- Static object sources: 351 active area files and 19124 parsed object records.
- Candidate templates after database/static reconciliation: 2733; generated class profiles: 30 standard and 30 enhanceable.
- Analyzer input rows: `characters` 131; `items` 23503; `item_affects` 8561; `wiki_objects` 19661; `wiki_affects` 15931; `wiki_effects` 3304; `wiki_slots` 16135; `wiki_classes` 36257; `wiki_races` 2491; `classes` 31; `races` 101.
- The archive contains duplicate object-UID groups, so object UID is not used as the container-tree key; row `id` is the authoritative parent/child identity.
- Analytical cohort query rows: `player_data` 131; `player_items` 23503; saved item affects 8561; nested object metadata available in archive.
- Source selection: `areas/obj` constrained by `areas/AREA`; item affects use saved instance rows when present and prototype affects otherwise.

## Analysis design

1. Read the archived SQL dump through an isolated local MariaDB instance; the archive itself remains unchanged.
2. Use `player_items.id` as the container-tree identity and `container_id` as its parent reference. Persisted equipment slots are 1-based; the analyzer maps them to runtime slots by subtracting one.
3. Reconcile SQL/wiki templates with active `.obj` area sources and runtime-normalize armor/wear flags, including loader-derived belt/back flags.
4. Analyze worn effects, saved numeric affects, permanent statuses, class/race restrictions, nested carried consumables, Bard instrument type IDs, Shaman sphere masks, and enhance configuration.
5. Rank per-slot candidates by class support, global adoption, bounded power, and risk; reject policy violations before catalog output.
6. Validate the generated catalog again from active area sources before emitting the C header.

The audited `can_char_use_item()`/`wear()` admission path has no alignment- or racewar-specific item gate; the selected item profiles therefore do not add one. Racewar and alignment still exist in other character/gameplay systems, and body-plan slot rules are documented separately below.

## Observed effect priorities

These are aggregate worn-item observations from the level-50+ cohort; they are not per-player records.

### Persistent/status effects

| Effect | Item instances | Players |
|---|---:|---:|
| Prot Fire | 168 | 89 |
| Sense Life | 117 | 79 |
| Farsee | 116 | 78 |
| Detect Magic | 97 | 72 |
| Major Mental | 103 | 66 |
| Prot Acid | 97 | 66 |
| Detect Evil | 86 | 63 |
| Detect Good | 86 | 63 |
| Haste | 68 | 62 |
| Protect Good | 93 | 61 |
| Detect Invisible | 76 | 61 |
| Iceshield | 84 | 60 |
| Protect Evil | 94 | 58 |
| Fly | 50 | 46 |
| Invisible | 49 | 46 |
| Aware | 62 | 43 |
| Levitate | 42 | 35 |
| Waterbreath | 39 | 34 |
| Sneak | 30 | 30 |
| Infravision | 29 | 29 |

### Numeric affects

| Affect | Item instances | Players | Signed modifier total |
|---|---:|---:|---:|
| `hit` | 801 | 115 | 15563 |
| `svspell` | 311 | 106 | -1026 |
| `apply_move` | 205 | 99 | 5839 |
| `con_max` | 355 | 95 | 1496 |
| `hitroll` | 281 | 93 | 1059 |
| `svpara` | 173 | 90 | -589 |
| `apply_saving_breath` | 178 | 85 | -751 |
| `damroll` | 385 | 84 | 1327 |
| `apply_move_reg` | 149 | 79 | 896 |
| `svfear` | 120 | 77 | -382 |
| `con` | 112 | 71 | 637 |
| `int_max` | 249 | 70 | 962 |
| `str_max` | 163 | 65 | 684 |
| `agi_max` | 105 | 62 | 511 |
| `wis_max` | 212 | 59 | 749 |
| `agi` | 81 | 58 | 509 |
| `str` | 75 | 49 | 345 |
| `dex` | 61 | 47 | 364 |
| `dex_max` | 177 | 46 | 802 |
| `ac` | 71 | 46 | -3111 |
| `int` | 62 | 45 | 362 |
| `apply_luck` | 47 | 35 | 287 |
| `pow_max` | 91 | 34 | 429 |
| `apply_luck_max` | 70 | 34 | 403 |

Interpretation: the recurring player pattern is broad survivability and mobility (`Prot Fire`, `Sense Life`, `Farsee`, `Detect Magic`, `Major Mental`, `Prot Acid`, `Haste`, `Fly`) combined with hit points, max-stat effects, and negative save modifiers for spell/paralysis/fear saves. The generator uses those observations as selection evidence, not as a promise that every profile receives every effect.

## Fundamental class items

These are separate from the ordinary wearable matrix because the runtime treats them as class tools or special support. Standard fundamentals are allowed explicit quest/no-sell exceptions only where named below.

### standard

| Role | VNUM | Item | Classes/use | Enhanceable | Reason |
|---|---:|---|---|:---:|---|
| Spellbook | 7 | the master spellbook | Eight spellbook classes in standard; strict alternative in enhanceable | no | runtime master spellbook; dynamic spell filling remains in read_object() |
| Bard flute | 138536 | the legendary flute of sleeping and charming | Bard | no | legendary series preferred for standard profile; quest-item status is an explicit fundamental exception when applicable |
| Bard lyre | 138535 | the legendary lyre of healing and harming | Bard | no | legendary series preferred for standard profile; quest-item status is an explicit fundamental exception when applicable |
| Bard mandolin | 138537 | the legendary mandolin of revelations and forgetfulness | Bard | no | legendary series preferred for standard profile; quest-item status is an explicit fundamental exception when applicable |
| Bard harp | 138538 | the legendary harp of peace and calming | Bard | no | legendary series preferred for standard profile; quest-item status is an explicit fundamental exception when applicable |
| Bard drums | 138533 | the legendary drums of chaos and heroism | Bard | no | legendary series preferred for standard profile; quest-item status is an explicit fundamental exception when applicable |
| Bard horn | 138534 | the legendary horn of flight and dragons | Bard | no | legendary series preferred for standard profile; quest-item status is an explicit fundamental exception when applicable |
| Shaman three-sphere totem | 88315 | a fire-imbued totem of Kossuth | Shaman | yes | value0 mask 63: all three high-circle spheres |

### enhanceable

| Role | VNUM | Item | Classes/use | Enhanceable | Reason |
|---|---:|---|---|:---:|---|
| Spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | Eight spellbook classes in standard; strict alternative in enhanceable | yes | strict enhance-index alternative spellbook |
| Bard flute | 1734 | a steel flute | Bard | yes | strict boot enhance-index alternative |
| Bard lyre | 33705 | a crystalline lyre | Bard | yes | strict boot enhance-index alternative |
| Bard mandolin | 1736 | an old mandolin | Bard | yes | strict boot enhance-index alternative |
| Bard harp | 1737 | an old brass harp | Bard | yes | strict boot enhance-index alternative |
| Bard drums | 1738 | a strange purple drum | Bard | yes | strict boot enhance-index alternative |
| Bard horn | 28971 | a finely curved horn | Bard | yes | strict boot enhance-index alternative |
| Shaman three-sphere totem | 88315 | a fire-imbued totem of Kossuth | Shaman | yes | value0 mask 63 and strict enhance-index eligibility |

Standard spellbook note: VNUM 7 is dynamically filled by the existing master-spellbook loader. Its active area prototype was made belt-attachable while retaining take/hold. The strict alternative uses a normal beltable spellbook and preserves the existing level-one spell population behavior.

## Shared consumable support

The same bounded support pool is placed inside the starter bag for every class. Choices come from high-level carried or nested-container adoption; counts are intentionally starter quantities rather than observed stockpiles.

| Category | VNUM | Item | Starter count | Observed players | Median carried quantity | Reason |
|---|---:|---|---:|---:|---:|---|
| potion | 93915 | a dark misty potion | 3 | 65 | 22.0 | high-level carried/contained potion adoption |
| potion | 1716 | a green potion with black chunks | 3 | 36 | 16.5 | high-level carried/contained potion adoption |
| potion | 15119 | a fiery red potion | 3 | 35 | 3.0 | high-level carried/contained potion adoption |
| potion | 2012 | a brown potion | 3 | 25 | 14.0 | high-level carried/contained potion adoption |
| scroll | 74021 | a scroll of Numbla | 2 | 38 | 42.5 | high-level carried/contained scroll adoption |
| scroll | 138280 | A scroll of curse removal | 2 | 16 | 8.5 | high-level carried/contained scroll adoption |
| scroll | 31314 | a pack of tarot cards | 2 | 14 | 17.0 | high-level carried/contained scroll adoption |
| food | 132508 | a succulent roast | 4 | 37 | 11.0 | high-level carried/contained food adoption |
| bandage | 369 | a large medicated bandage | 4 | 27 | 6.0 | high-level carried/contained bandage adoption |
| herb | 831 | some landrace Medicus | 3 | 59 | 6.0 | high-level carried/contained herb adoption |
| drinkcon | 85711 | a bottle of dark ale | 1 | 3 | 10.0 | high-level carried/contained drinkcon adoption |

Charged staves and wands were kept outside the normal support policy. Placeholder VNUM 1252 is absent.

## Cross-class common items

These are the items selected for at least 80% of generated class profiles in that profile.

| Profile | Slot | VNUM | Item | Class profiles |
|---|---|---:|---|---:|
| standard | `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 24/30 |
| enhanceable | `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 25/30 |
| enhanceable | `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 25/30 |
| enhanceable | `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 28/30 |

## Race/body-slot variations

Normal class profiles intentionally use the common equipment path. Optional rows are appended only after runtime slot and class/race checks; this avoids granting a universal profile a horse/tail/horn/spider item it cannot use.

| Profile | Variation | Slot | VNUM | Item | Runtime condition | Races/body plans | Status |
|---|---|---|---:|---|---|---|---|
| standard | four_hands | `THIRD_WEAPON` | 40781 | a massive longsword dubbed 'Dusk and Dawn' | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| standard | four_hands | `FOURTH_WEAPON` | 55337 | the rapier called 'Penetration' | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| standard | four_hands | `WEAR_ARMS_2` | 58815 | a shoulder drape of emerald dragonscale | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| standard | four_hands | `WEAR_HANDS_2` | 83505 | some black leather gloves with mithril blades | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| standard | four_hands | `WEAR_WRIST_LR` | 87548 | the bracer of Uz's alliance | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| standard | four_hands | `WEAR_WRIST_LL` | 38450 | an elven bracelet of precision | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| standard | tail | `WEAR_TAIL` | 40067 | a ring of sandstone | `HAS_TAIL()` | Centaur, Minotaur, Shadow Beast, Kobold, Tiefling | available |
| standard | nose | `WEAR_NOSE` | 76656 | a wad of spawn goo | `IS_MINOTAUR()` | Minotaur | available |
| standard | horns | `WEAR_HORN` | 85709 | the horns of endless battles | `runtime horn-bearing race check` | Minotaur, Harpy, Gargoyle, Shadow Beast, Tiefling | available |
| standard | horse_body | `WEAR_HORSE_BODY` | 87585 | the unicorn's saddle of smiting | `has_innate(INNATE_HORSE_BODY)` | Centaur | available |
| standard | spider_body | `WEAR_SPIDER_BODY` | 85714 | a barding saddle of rattling bones | `has_innate(INNATE_SPIDER_BODY)` | Drider | available |
| standard | rear_legs | `WEAR_LEGS_REAR` |  | — | `runtime currently rejects WEAR_LEGS_REAR` | race/body-plan dependent | unavailable |
| standard | rear_feet | `WEAR_FEET_REAR` |  | — | `runtime currently rejects WEAR_FEET_REAR` | race/body-plan dependent | unavailable |
| enhanceable | four_hands | `THIRD_WEAPON` | 40781 | a massive longsword dubbed 'Dusk and Dawn' | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| enhanceable | four_hands | `FOURTH_WEAPON` | 55337 | the rapier called 'Penetration' | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| enhanceable | four_hands | `WEAR_ARMS_2` | 58815 | a shoulder drape of emerald dragonscale | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| enhanceable | four_hands | `WEAR_HANDS_2` | 83505 | some black leather gloves with mithril blades | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| enhanceable | four_hands | `WEAR_WRIST_LR` | 87548 | the bracer of Uz's alliance | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| enhanceable | four_hands | `WEAR_WRIST_LL` | 38450 | an elven bracelet of precision | `HAS_FOUR_HANDS() or AFF3_FOUR_ARMS` | Thri-Kreen | available |
| enhanceable | tail | `WEAR_TAIL` | 40067 | a ring of sandstone | `HAS_TAIL()` | Centaur, Minotaur, Shadow Beast, Kobold, Tiefling | available |
| enhanceable | nose | `WEAR_NOSE` | 76656 | a wad of spawn goo | `IS_MINOTAUR()` | Minotaur | available |
| enhanceable | horns | `WEAR_HORN` | 85709 | the horns of endless battles | `runtime horn-bearing race check` | Minotaur, Harpy, Gargoyle, Shadow Beast, Tiefling | available |
| enhanceable | horse_body | `WEAR_HORSE_BODY` | 87585 | the unicorn's saddle of smiting | `has_innate(INNATE_HORSE_BODY)` | Centaur | available |
| enhanceable | spider_body | `WEAR_SPIDER_BODY` | 85714 | a barding saddle of rattling bones | `has_innate(INNATE_SPIDER_BODY)` | Drider | available |
| enhanceable | rear_legs | `WEAR_LEGS_REAR` |  | — | `runtime currently rejects WEAR_LEGS_REAR` | race/body-plan dependent | unavailable |
| enhanceable | rear_feet | `WEAR_FEET_REAR` |  | — | `runtime currently rejects WEAR_FEET_REAR` | race/body-plan dependent | unavailable |

### Runtime slot restrictions that remain intentional

| Slot family | Affected races/body plans | Consequence |
|---|---|---|
| Fingers / earrings | Thri-Kreen | Runtime rejects both finger and earring slots. |
| Body | Thri-Kreen | Runtime rejects the main body slot. |
| Head | Minotaur, Illithid, Pillithid | Runtime rejects the head slot. |
| Legs | Drider, Centaur, Harpy, Ogre, Firbolg | Runtime rejects the main legs slot. |
| Feet | Drider, Thri-Kreen, Harpy, Minotaur | Runtime rejects the main feet slot. |
| Arms | Ogre, Firbolg | Runtime rejects the main arms slot. |
| Third/fourth weapons and extra limbs | HAS_FOUR_HANDS() | Only emitted through optional slot variants. |
| Horse body | INNATE_HORSE_BODY | Only emitted when the runtime innate is present. |
| Tail | Centaur, Minotaur, Shadow Beast, Kobold, Tiefling | Only emitted when the runtime tail check passes. |
| Nose | Minotaur | Only emitted for Minotaur. |
| Horns | Minotaur, Harpy, Shadow Beast, Tiefling | Only emitted for the runtime horn-bearing races. |
| Spider body | INNATE_SPIDER_BODY | Only emitted when the runtime innate is present. |
| Rear legs / rear feet | None in current runtime | has_eq_slot() currently rejects both slots; documented as unavailable. |

These are body-plan rules in `has_eq_slot()`/`wear()`, not item-level race restrictions. The loader also calls `can_char_use_item()` as a defense-in-depth check.

## Exclusions and balance guardrails

The pre-change source audit found 127 resolved placeholder positions and 17 distinct selected VNUMs violating the requested artifact/unique/Ioun policy. The generated profiles remove those rows.

### Pre-change selected-profile violations

- artifact: 424, 430, 906, 921, 922, 1230, 6826, 16262, 23805, 28973, 51401, 67262, 67274, 87546, 87612, 139004
- unique_keyword: 424, 23805, 28973, 67262, 87546
- ioun_slot: 906, 921, 922, 59318

### Catalog-wide exclusions observed during analysis

| Exclusion | Catalog rows |
|---|---:|
| `quest_item` | 231 |
| `item_nosell` | 188 |
| `artifact` | 70 |
| `unique_keyword` | 37 |
| `item_transient` | 27 |
| `anti_race_flag` | 25 |
| `allowed_race_list_is_not_portable` | 12 |
| `ioun` | 9 |
| `item_norent` | 2 |
| `item_noshow` | 2 |
| `placeholder_vnum_1252` | 1 |

### Selection policy

- Normal equipment excludes artifact flags, Ioun wear, `unique` keywords, item-level race restrictions, item-level class restrictions, transient/no-rent/no-show/no-sell objects, and quest objects.
- Explicit standard fundamentals are the only named exceptions: the master spellbook and the six legendary bard instruments. The standard shaman totem is not an artifact/unique/Ioun object.
- The enhanceable profile requires the configured boot-time enhance predicate: VNUM range, takeability, non-artifact/non-transient economics, allowed bitvector masks, non-charged gear type, and configured pool exclusions.
- Per-item risk is capped at 4.0 in the generated catalog. Extreme outliers such as boots of speed VNUM 36753 are excluded from the starter profile even though they are common enough to appear in the source cohort.
- The selected rows are intentionally not copied from one character wholesale; they are per-slot aggregate winners with low-risk portable fallbacks for classes that have few or no direct observations.

## Full standard equipment profiles

`power` and `risk` are analyzer heuristics used to suppress extreme outliers. They are not the game’s combat formula and should be retuned after live Chaos playtesting.

### Warrior

Observed high-level characters: 10 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 38763 | a ring of regeneration | 4 | 62.5 | 0.9 | hit +25, hitroll +5; offense: hit/damage |
| `WEAR_NECK_1` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_2` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 88301 | a crown of burning flames | 5 | 17.9 | 0.0 | apply_combat_pulse -1, damroll +3; status: Aware, Prot Fire, Sense Life; status: Aware, Prot Fire, Sense Life; offense: hit/damage |
| `WEAR_LEGS` | 44174 | the leggings of a MaDMaN | 4 | 77.9 | 2.748 | damroll +3, dex_max +5, hit +30; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 83665 | some finely-detailed platinum armplates | 1 | 53.1 | 0.0 | hit +25, svspell -4; good saves: svspell |
| `WEAR_SHIELD` | 142416 | the holy shield of blessed defense | 3 | 83.0 | 3.36 | hit +40, svspell -5; good saves: svspell |
| `WEAR_ABOUT` | 19916 | a black hooded cloak of Isha the Wanderer | 2 | 22.5 | 0.0 | damroll +5, hitroll +2; offense: hit/damage |
| `WEAR_WAIST` | 500033 | the mystical sash of the Netherworld | 8 | 7.0 | 0.0 | apply_saving_breath -5, svspell -5; good saves: svspell |
| `WEAR_WRIST_R` | 83235 | a feathered bracelet of Nax'Varan nobility | 2 | 63.1 | 0.972 | hit +31, svfear -3; good saves: svfear |
| `WEAR_WRIST_L` | 38774 | a bracer of might | 2 | 51.7 | 0.0 | damroll +3, hit +22; offense: hit/damage |
| `PRIMARY_WEAPON` | 142419 | the legendary cutlass 'Sanguine Song' | 6 | 31.5 | 0.0 | damroll +5, hitroll +5; offense: hit/damage |
| `SECONDARY_WEAPON` | 22032 | the demonic warmace 'Pure-Dark' | 3 | 26.8 | 0.0 | damroll +4, hitroll +4; status: Absorb, Protect Good; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 91065 | the grim visage of a MaDWoMaN | 3 | 47.2 | 0.0 | hit +20, svfear -6; status: Sneak; good saves: svfear |
| `WEAR_EARRING_R` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 98911 | a huge zanthium broad sword | 1 | 36.5 | 0.0 | damroll +5, hitroll +5; status: Iceshield; status: Iceshield; offense: hit/damage |
| `WEAR_ATTACH_BELT_1` | 139805 | A swirling mass of black smoke and hot ash | 4 | 66.0 | 1.32 | apply_move +30, svpara -8; status: Absorb; good saves: svpara |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Ranger

Observed high-level characters: 3 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 19901 | a shiny crimson ring | 3 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FINGER_L` | 19901 | a shiny crimson ring | 3 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_1` | 98949 | a necklace of halfling ears | 7 | 17.6 | 0.0 | damroll +2, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_2` | 87523 | the bloody eyeballs on a sinew | 4 | 24.2 | 0.0 | con_max +5, str_max +6; max-stat focus: con_max, str_max |
| `WEAR_BODY` | 70954 | the chameleon suit of transformation | 5 | 37.4 | 0.0 | agi_max +8, dex_max +9; max-stat focus: agi_max, dex_max |
| `WEAR_HEAD` | 83312 | a spiked chromium dragonscale helmet | 2 | 27.6 | 0.0 | con_max +4, str_max +4; status: Aware, Iceshield, Major Mental; max-stat focus: con_max, str_max; status: Aware, Iceshield, Major Mental |
| `WEAR_LEGS` | 42169 | some flexible leggings of hellfire | 3 | 21.4 | 0.0 | damroll +2, dex_max +4; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire; offense: hit/damage |
| `WEAR_FEET` | 83486 | some black leather boots fit with adamantium heel blades | 4 | 18.7 | 0.0 | damroll +3, dex_max +4; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 31517 | the pauldrons of the tide | 3 | 11.6 | 0.0 | str_max +4, svspell -2; max-stat focus: str_max; good saves: svspell |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 28954 | a curtain of elemental fire | 7 | 12.6 | 0.0 | status: Clarity, Fireshield, Prot Fire, Sneak; status: Fireshield, Prot Fire |
| `WEAR_WAIST` | 87707 | an array of flesh hooks and chains | 2 | 12.5 | 0.0 | con -10, damroll +3; status: Aware, Protect Evil, Protect Good; status: Aware; offense: hit/damage |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 87523 | the bloody eyeballs on a sinew | 4 | 24.2 | 0.0 | con_max +5, str_max +6; max-stat focus: con_max, str_max |
| `PRIMARY_WEAPON` | 40781 | a massive longsword dubbed 'Dusk and Dawn' | 3 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `SECONDARY_WEAPON` | 99700 | a flaming longsword emblazoned 'Cinder' | 1 | 24.9 | 0.0 | damroll +3, hitroll +3; status: Prot Fire; status: Prot Fire; offense: hit/damage |
| `WEAR_EYES` | 83372 | some marksman's goggles | 1 | 21.8 | 0.0 | dex_max +4, hitroll +4; status: Farsee; max-stat focus: dex_max; status: Farsee; offense: hit/damage |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 29424 | a gnomish ear amplifier | 4 | 37.0 | 0.0 | apply_move +10, hit +10; observed high-level equipment usage |
| `WEAR_EARRING_L` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 139806 | Ehkahk's badge of Honor | 5 | 17.6 | 0.0 | dex_max +4, str_max +4; max-stat focus: dex_max, str_max |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Psionicist

Observed high-level characters: 6 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 58417 | a band of parting prevented | 1 | 79.8 | 2.976 | apply_hit_reg +37, con_max +6; max-stat focus: con_max |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 42226 | an amulet of Nine Hells | 1 | 38.0 | 0.0 | apply_saving_breath -2, hit +20; observed high-level equipment usage |
| `WEAR_BODY` | 38772 | the legendary platemail of defense | 4 | 85.8 | 3.696 | con_max +4, hit +40; status: Aware; max-stat focus: con_max; status: Aware |
| `WEAR_HEAD` | 55429 | the spiked crown of mental resistance | 2 | 46.8 | 0.0 | hit +20, pow_max +4; max-stat focus: pow_max |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 20924 | a pair of bear-hide mountain boots | 1 | 72.0 | 2.04 | apply_move +35, hitroll +3; offense: hit/damage |
| `WEAR_HANDS` | 70968 | the gloves of magic fingertips | 1 | 50.4 | 0.0 | hit +18, int +9; stat focus: int |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 19638 | the shield of the battle dragon | 3 | 8.8 | 0.0 | ac -20, con_max +4; max-stat focus: con_max |
| `WEAR_ABOUT` | 3536 | the cloak of the demon kings | 2 | 61.2 | 0.744 | hit +30, svspell -3; good saves: svspell |
| `WEAR_WAIST` | 16217 | a belt of fresh blood | 7 | 17.6 | 0.0 | con_max +5, damroll +2; max-stat focus: con_max; offense: hit/damage |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 3546 | a bracer of pit worm scales | 2 | 53.8 | 0.0 | hit +24, svspell -3; status: Prot Acid; good saves: svspell; status: Prot Acid |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 27030 | a stone mask | 1 | 67.6 | 1.512 | con +8, hit +28; stat focus: con |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 28981 | a hoop of twisting obsidian | 1 | 42.2 | 0.0 | hit +20, svspell -3; good saves: svspell |
| `WEAR_QUIVER` | 38444 | a glowing bard sack | 7 | 32.4 | 0.0 | apply_move +15, dex +3; stat focus: dex |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 40425 | a backpack made of hemp | 3 | 23.4 | 0.0 | apply_move +10, str +3; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 139805 | A swirling mass of black smoke and hot ash | 4 | 66.0 | 1.32 | apply_move +30, svpara -8; status: Absorb; good saves: svpara |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Paladin

Observed high-level characters: 3 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 66231 | the steel ring of greater physical resistance | 7 | 0.0 | 0.0 | ac -30; observed high-level equipment usage |
| `WEAR_NECK_1` | 83346 | a polished mithril gorget of Hammerhelm | 8 | 19.0 | 0.0 | ac -40, hit +10; observed high-level equipment usage |
| `WEAR_NECK_2` | 83324 | a mithril and silver neckguard | 4 | 9.0 | 0.0 | ac -22, str +5; stat focus: str |
| `WEAR_BODY` | 142447 | a mithril breastplate with a golden cross | 7 | 30.8 | 0.0 | damroll +6, str_max +5; max-stat focus: str_max; offense: hit/damage |
| `WEAR_HEAD` | 83383 | a polished mithril and dragonscale helmet | 7 | 0.0 | 0.0 | ac -40, apply_saving_breath -3; observed high-level equipment usage |
| `WEAR_LEGS` | 83382 | some mithril and dragonscale leg plates | 7 | 0.0 | 0.0 | ac -40, apply_saving_breath -3; observed high-level equipment usage |
| `WEAR_FEET` | 44194 | the boots of a MaDMaN | 12 | 4.0 | 0.0 | str -5; status: Prot Gas |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 70979 | some scintillating sleeves | 10 | 18.0 | 0.0 | agi +5, dex +5; stat focus: agi, dex |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 5503 | the terrifying shroud of undead power | 9 | 0.0 | 0.0 | ac -85, con -15; observed high-level equipment usage |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 83228 | a spiked mithril wristguard | 8 | 13.2 | 0.0 | ac -20, damroll +4; offense: hit/damage |
| `WEAR_WRIST_L` | 120013 | some heavy mithril bracers | 6 | 10.8 | 0.0 | ac -20, str +6; stat focus: str |
| `PRIMARY_WEAPON` | 78417 | a massive blackrock hammer named 'Searing Fist' | 2 | 31.8 | 0.0 | damroll +6, hitroll +4; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 83233 | some catseye inventor's goggles | 6 | 26.4 | 0.0 | apply_luck_max +7, pow_max +5; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 33703 | a silver tooth earring | 12 | 9.0 | 0.0 | ac -15, hitroll +3; offense: hit/damage |
| `WEAR_EARRING_L` | 33703 | a silver tooth earring | 12 | 9.0 | 0.0 | ac -15, hitroll +3; offense: hit/damage |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 77740 | a frost dragon's eye | 3 | 18.8 | 0.0 | apply_saving_breath -7, con +10; status: Detect Invisible; stat focus: con |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 25757 | the quiver of holding | 4 | 20.0 | 0.0 | agi_max +5, hitroll +3; max-stat focus: agi_max; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Anti-Paladin

Observed high-level characters: 4 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 42903 | a spiked electrum ring | 1 | 37.2 | 0.0 | damroll +2, dex +17; stat focus: dex; offense: hit/damage |
| `WEAR_NECK_1` | 38610 | an amulet of fire dragon's blood | 4 | 48.8 | 0.0 | apply_saving_breath -6, hit +22; status: Farsee, Prot Fire |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87530 | some blue-tinted chainmail leggings | 3 | 51.2 | 0.0 | damroll +4, hit +20; offense: hit/damage |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 5815 | a set of jagged blood crystal arm plates | 3 | 36.6 | 0.0 | con +13, damroll +4; stat focus: con; offense: hit/damage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 41204 | the cloak of shadow dragons | 3 | 55.0 | 0.0 | hit +20, hitroll +3; status: Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 16217 | a belt of fresh blood | 7 | 17.6 | 0.0 | con_max +5, damroll +2; max-stat focus: con_max; offense: hit/damage |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 38660 | the ancestral bracelet of Sevenoaks | 4 | 18.4 | 0.0 | con_max +4, wis_max +4; status: Barkskin; max-stat focus: con_max, wis_max |
| `PRIMARY_WEAPON` | 21633 | a huge gladiators blade | 1 | 28.8 | 0.0 | damroll +6, hitroll +3; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 13053 | glyphs of power tattooed around one eye | 3 | 13.2 | 0.0 | damroll +2, dex_max +3; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FACE` | 33701 | a lithixl beak | 13 | 22.8 | 0.0 | ac -15, hit +12; observed high-level equipment usage |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_QUIVER` | 89169 | a pantherhide quiver | 4 | 11.0 | 0.0 | agi_max +2, dex_max +3; max-stat focus: agi_max, dex_max |
| `GUILD_INSIGNIA` | 31312 | a necromantic death shroud | 9 | 28.3 | 0.0 | apply_move +10, hit +5; status: Absorb; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 18000 | a longsword named 'the Answerer' | 2 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 40730 | a mist viper venom sac | 1 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 40730 | a mist viper venom sac | 1 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |

### Cleric

Observed high-level characters: 12 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 38720 | a necklace of vampiric power | 2 | 54.1 | 0.0 | hit +19, str +10; stat focus: str |
| `WEAR_NECK_2` | 87601 | the amulet named "Flow" | 3 | 46.8 | 0.0 | hit +20, wis_max +4; max-stat focus: wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 21641 | the royal crown of Aravne | 1 | 49.0 | 0.0 | hit +20, wis_max +5; max-stat focus: wis_max |
| `WEAR_LEGS` | 91048 | a pair of blood-stained wyrm scale leggings | 1 | 53.5 | 0.0 | apply_move_reg +6, apply_saving_breath -5, hit +17; status: Detect Evil, Detect Good, Detect Magic, Fly; status: Fly |
| `WEAR_FEET` | 87540 | the slippers of rot and decay | 3 | 30.4 | 0.0 | con_max +6, svpara -4; status: Prot Acid, Prot Fire, Protect Evil, Slow Poison; max-stat focus: con_max; good saves: svpara; status: Prot Acid, Prot Fire |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 20254 | some scintilating sleeves | 2 | 38.4 | 0.0 | hit +18, svpara -3; good saves: svpara |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 6802 | a tattered dragon-scale bodycloak | 1 | 73.5 | 2.22 | hit +35, svspell -5; good saves: svspell |
| `WEAR_WAIST` | 6738 | a silken sash of raw silk | 2 | 75.8 | 2.496 | apply_move +21, hit +20; observed high-level equipment usage |
| `WEAR_WRIST_R` | 130021 | a sapphire bracelet | 1 | 46.4 | 0.0 | apply_move +20, con_max +4; status: Protect Evil, Protect Good; max-stat focus: con_max |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 142449 | an astral metal hammer named 'Frozen Star' | 4 | 34.8 | 0.0 | damroll +6, hitroll +5; offense: hit/damage |
| `SECONDARY_WEAPON` | 138254 | a kau sin ke whipping chain | 2 | 25.2 | 0.0 | damroll +4, hitroll +4; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 25405 | a white hot mask of living flame | 5 | 43.0 | 0.0 | con_max +4, hit +18; max-stat focus: con_max |
| `WEAR_EARRING_R` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_EARRING_L` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_QUIVER` | 55040 | a silken backsheath of flight | 22 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Monk

Observed high-level characters: 6 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 5817 | some razor-knuckled blood crystal gloves | 2 | 43.3 | 0.0 | damroll +4, hit +15; status: Detect Evil, Protect Evil; offense: hit/damage |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 28988 | the cloak of multi-colored beads | 2 | 43.2 | 0.0 | hit +20, svpara -3; status: Aware; good saves: svpara; status: Aware |
| `WEAR_WAIST` | 88917 | a belt of liquid links | 5 | 27.6 | 0.0 | apply_luck_max +3, hit +10; status: Waterbreath; max-stat focus: apply_luck_max; status: Waterbreath |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 139826 | Some Disgusting Make-up | 3 | 53.0 | 0.0 | apply_luck_max +20, cha -20, hitroll +3; max-stat focus: apply_luck_max; offense: hit/damage |
| `WEAR_FACE` | 139826 | Some Disgusting Make-up | 3 | 53.0 | 0.0 | apply_luck_max +20, cha -20, hitroll +3; max-stat focus: apply_luck_max; offense: hit/damage |
| `WEAR_EARRING_R` | 6725 | a delicate enchanted snowflake | 6 | 35.4 | 0.0 | hitroll +4, int +8; status: Iceshield, Major Mental; stat focus: int; status: Iceshield, Major Mental; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 87564 | the quiver of the ages | 3 | 24.4 | 0.0 | hit +10, svpara -1; status: Regenerate; good saves: svpara; status: Regenerate |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 29443 | a hand-carved yo-yo | 6 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Druid

Observed high-level characters: 4 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 21645 | the royal ring of Aravne | 2 | 66.5 | 1.38 | hit +35; observed high-level equipment usage |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 6703 | a druidic necklace of fish scales | 1 | 13.4 | 0.0 | wis +5, wis_max +2; max-stat focus: wis_max; stat focus: wis |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 82707 | a pair of feathered gloves | 1 | 29.9 | 0.0 | agi +5, hit +11; stat focus: agi |
| `WEAR_ARMS` | 99538 | a pair of sleeves woven from mist | 2 | 37.0 | 0.0 | apply_move +10, hit +10; observed high-level equipment usage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 83593 | the shroud of corruption | 2 | 36.1 | 0.0 | con_max +3, hit +15; status: Sense Life; max-stat focus: con_max; status: Sense Life |
| `WEAR_WAIST` | 14045 | a belt encrusted with black sapphires | 1 | 51.9 | 0.0 | hit +25, str_max +2; max-stat focus: str_max |
| `WEAR_WRIST_R` | 35229 | a lizard tail bracer | 1 | 32.4 | 0.0 | apply_move +15, apply_move_reg +3; observed high-level equipment usage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 96012 | a huge adamantium mace named 'Mistweave' | 1 | 38.5 | 0.0 | damroll +5, hitroll +7; status: Sense Life; status: Sense Life; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 3537 | a bone visor | 2 | 20.8 | 0.0 | int_max +5, wis_max +4; status: Farsee; max-stat focus: int_max, wis_max; status: Farsee |
| `WEAR_FACE` | 7677 | mask of battles past | 5 | 22.8 | 0.0 | con_max +3, dex +9; max-stat focus: con_max; stat focus: dex |
| `WEAR_EARRING_R` | 29458 | a pair of silver earrings with red rubies | 6 | 14.4 | 0.0 | apply_move +8; observed high-level equipment usage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 35808 | an embroidered illithid-hide backsheath | 2 | 9.0 | 0.0 | agi +5; stat focus: agi |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Shaman

Observed high-level characters: 14 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 58379 | an oathstone pendant | 3 | 47.8 | 0.0 | con_max +4, hit +20; status: Sense Life; max-stat focus: con_max; status: Sense Life |
| `WEAR_NECK_2` | 25712 | an amulet of wisdom | 2 | 39.5 | 0.0 | hit +15, wis_max +5; max-stat focus: wis_max |
| `WEAR_BODY` | 9114 | the hide of the apocalypse demon | 1 | 57.1 | 0.252 | hit +19, hitroll +5; status: Prot Fire; status: Prot Fire; offense: hit/damage |
| `WEAR_HEAD` | 142422 | an ancient crown of elven royalty | 3 | 49.0 | 0.0 | con_max +5, hit +20; max-stat focus: con_max |
| `WEAR_LEGS` | 47082 | some wyvern-skull poleyns | 2 | 54.0 | 0.0 | apply_move +30, apply_saving_breath -3; observed high-level equipment usage |
| `WEAR_FEET` | 32624 | the boots of the dreamer | 3 | 55.2 | 0.024 | agi_max +7, hit +20; status: Aware, Levitate; max-stat focus: agi_max; status: Aware |
| `WEAR_HANDS` | 55404 | a pair of fingerless silk gloves | 1 | 54.4 | 0.0 | agi_max +5, apply_move_reg +3, hit +20; max-stat focus: agi_max |
| `WEAR_ARMS` | 47081 | some wyvern-skull pauldrons | 3 | 72.0 | 2.04 | apply_move +40, apply_saving_breath -4; observed high-level equipment usage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 33702 | a deep hooded cloak of sorrow | 2 | 47.0 | 0.0 | apply_move +5, hit +20; observed high-level equipment usage |
| `WEAR_WAIST` | 87709 | the woven entrails of an unfortunate soul | 3 | 39.4 | 0.0 | apply_hit_reg +14, apply_move_reg +7; status: Protect Evil, Protect Good; observed high-level equipment usage |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 138542 | the epic flute of sleeping and charming | 1 | 60.5 | 0.66 | cha_max +5, hit +25; status: Waterbreath; max-stat focus: cha_max; status: Waterbreath |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| three_sphere_high_circle_totem | 88315 | a fire-imbued totem of Kossuth |

### Sorcerer

Observed high-level characters: 14 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 40768 | the ring of celestial wonders | 4 | 59.0 | 0.48 | hit +24, int_max +2; status: Iceshield, Prot Acid; max-stat focus: int_max; status: Iceshield, Prot Acid |
| `WEAR_NECK_1` | 87607 | a tuft of MADMAN'S hair | 2 | 54.5 | 0.0 | hit +25, svfear -5; good saves: svfear |
| `WEAR_NECK_2` | 38610 | an amulet of fire dragon's blood | 4 | 48.8 | 0.0 | apply_saving_breath -6, hit +22; status: Farsee, Prot Fire |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 25719 | the helm of the dragonlords | 1 | 49.7 | 0.0 | apply_move_reg +9, hit +15; status: Stone Skin |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 32630 | the gloves of the adept | 1 | 45.6 | 0.0 | hit +20, int_max +3; status: Aware; max-stat focus: int_max; status: Aware |
| `WEAR_ARMS` | 32430 | some glowing sleeves made of devil skin | 3 | 43.3 | 0.0 | hit +15, int_max +4; status: Prot Fire; max-stat focus: int_max; status: Prot Fire |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 70813 | opalescent robes of the archmagi | 2 | 75.3 | 2.436 | hit +33, int +7; stat focus: int |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 38621 | a satanic bracelet | 5 | 59.3 | 0.516 | hit +25, int_max +5; status: Protect Good; max-stat focus: int_max |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 44485 | the sapphire eye of a beholder guardian | 1 | 43.2 | 0.0 | hit +20, svspell -3; status: Infravision; good saves: svspell; status: Infravision |
| `WEAR_FACE` | 91065 | the grim visage of a MaDWoMaN | 3 | 47.2 | 0.0 | hit +20, svfear -6; status: Sneak; good saves: svfear |
| `WEAR_EARRING_R` | 66675 | a shimmering shark tooth earring | 1 | 44.2 | 0.0 | hit +20, svpara -3; status: Waterbreath; good saves: svpara; status: Waterbreath |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83480 | a long roc-feather quill | 1 | 28.8 | 0.0 | agi +8, int +8; stat focus: agi, int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |

### Necromancer

Observed high-level characters: 12 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 6843 | a banded mail shirt | 1 | 50.6 | 0.0 | hit +20, wis +7; stat focus: wis |
| `WEAR_HEAD` | 87594 | the troll fez of true clarity | 1 | 64.2 | 1.104 | hit +28, int_max +5; max-stat focus: int_max |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 19911 | mystical boots of Volo the Traveller | 2 | 49.7 | 0.0 | apply_move_reg +12, hit +9, int_max +5; max-stat focus: int_max |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 44827 | a small circular shield of blue flames | 2 | 52.0 | 0.0 | hit +22, svpara -3; status: Prot Fire; good saves: svpara; status: Prot Fire |
| `WEAR_ABOUT` | 31527 | the living oceanic cloak | 3 | 43.4 | 0.0 | con_max +5, hit +16; status: Waterbreath; max-stat focus: con_max; status: Waterbreath |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 9447 | a staff of power | 3 | 75.8 | 2.496 | hit +30, hitroll +6; status: Detect Invisible; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 5048 | some eyes of the salamander | 1 | 41.4 | 0.0 | agi +4, hit +18; stat focus: agi |
| `WEAR_FACE` | 38455 | some silk panties | 4 | 54.0 | 0.0 | agi +15, cha +15; stat focus: agi, cha |
| `WEAR_EARRING_R` | 78036 | an anti-matter earring | 1 | 53.1 | 0.0 | hit +25, svspell -4; good saves: svspell |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 78037 | a massive tome | 1 | 37.5 | 0.0 | hit +15, int +5; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 402052 | a lavishly plumed quill pen | 2 | 19.0 | 0.0 | hit +10; observed high-level equipment usage |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |

### Conjurer

Observed high-level characters: 4 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 43563 | a ring of elemental might | 1 | 50.0 | 0.0 | apply_luck_max +5, hit +20; status: Farsee; max-stat focus: apply_luck_max; status: Farsee |
| `WEAR_FINGER_L` | 6502 | a ring of citizenship | 2 | 35.2 | 0.0 | con +9, hit +10; stat focus: con |
| `WEAR_NECK_1` | 43015 | a cloak of dragons | 3 | 27.8 | 0.0 | con_max +4, hit +10; max-stat focus: con_max |
| `WEAR_NECK_2` | 38663 | an amulet of chaos | 3 | 47.6 | 0.0 | apply_saving_breath -3, hit +24; status: Infravision, Sense Life |
| `WEAR_BODY` | 142435 | the flowing robes of vitality | 2 | 68.0 | 1.56 | hit +30, pow_max +5; max-stat focus: pow_max |
| `WEAR_HEAD` | 76722 | the hood of cunning | 2 | 28.2 | 0.0 | hit +12, int_max +2; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_LEGS` | 7678 | greaves of avoidance | 2 | 39.0 | 0.0 | agi +9, hit +12; stat focus: agi |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 31527 | the living oceanic cloak | 3 | 43.4 | 0.0 | con_max +5, hit +16; status: Waterbreath; max-stat focus: con_max; status: Waterbreath |
| `WEAR_WAIST` | 132024 | the threads of astral projection | 3 | 40.1 | 0.0 | hit +15, pow +6; status: Absorb; stat focus: pow |
| `WEAR_WRIST_R` | 38621 | a satanic bracelet | 5 | 59.3 | 0.516 | hit +25, int_max +5; status: Protect Good; max-stat focus: int_max |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 78064 | a mask of hades | 5 | 18.0 | 0.0 | agi_max +5, svspell -3; status: Detect Invisible, Waterbreath; max-stat focus: agi_max; good saves: svspell; status: Waterbreath |
| `WEAR_EARRING_R` | 23811 | a delicate nipple ring | 4 | 8.8 | 0.0 | cha_max +2, int_max +2; max-stat focus: cha_max, int_max |
| `WEAR_EARRING_L` | 87727 | an emerald disc fashioned from bone | 4 | 30.0 | 0.0 | con +10, con_max +5; status: Aware; max-stat focus: con_max; stat focus: con; status: Aware |
| `WEAR_QUIVER` | 38444 | a glowing bard sack | 7 | 32.4 | 0.0 | apply_move +15, dex +3; stat focus: dex |
| `GUILD_INSIGNIA` | 6736 | a wisp of shimmering faerie light | 3 | 20.0 | 0.0 | hit +10; status: Ultravision |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 33711 | the lost book of 'Magic' | 5 | 9.8 | 0.0 | int +5; status: Levitate; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |

### Rogue

Observed high-level characters: 6 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 26639 | the ring of the void | 3 | 70.1 | 1.812 | hit +33, svfear -4; status: Infravision, Invisible; good saves: svfear; status: Infravision |
| `WEAR_FINGER_L` | 28921 | an obsidian circle | 10 | 83.8 | 3.456 | apply_move +40, dex_max +5; status: Protect Good; max-stat focus: dex_max |
| `WEAR_NECK_1` | 99702 | a fiery chain of hades | 1 | 53.2 | 0.0 | apply_move +10, hit +18; status: Infravision |
| `WEAR_NECK_2` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_BODY` | 70954 | the chameleon suit of transformation | 5 | 37.4 | 0.0 | agi_max +8, dex_max +9; max-stat focus: agi_max, dex_max |
| `WEAR_HEAD` | 83209 | a derroskin admiral's hat | 1 | 62.9 | 0.948 | dex_max +7, hit +25; max-stat focus: dex_max |
| `WEAR_LEGS` | 83666 | some finely-detailed platinum legplates | 1 | 53.1 | 0.0 | hit +25, svspell -4; good saves: svspell |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 87600 | the vambraces named "Ebb" | 2 | 35.2 | 0.0 | agi_max +10, con_max +6; max-stat focus: agi_max, con_max |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 16269 | a tight green cloak | 5 | 12.1 | 0.0 | damroll +1; status: Detect Invisible, Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 132024 | the threads of astral projection | 3 | 40.1 | 0.0 | hit +15, pow +6; status: Absorb; stat focus: pow |
| `WEAR_WRIST_R` | 87538 | the power infused bracelet of dark-mithril | 3 | 39.6 | 0.0 | damroll +8, str_max +6; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 38621 | a satanic bracelet | 5 | 59.3 | 0.516 | hit +25, int_max +5; status: Protect Good; max-stat focus: int_max |
| `PRIMARY_WEAPON` | 11307 | the dark rod named 'Punisher' | 2 | 31.7 | 0.0 | damroll +3, hitroll +7; status: Protect Good; offense: hit/damage |
| `SECONDARY_WEAPON` | 142445 | a green bladed dagger named 'Sepsis' | 3 | 25.5 | 0.0 | damroll +5, hitroll +3; offense: hit/damage |
| `WEAR_EYES` | 25705 | the blindfold of sight | 2 | 45.8 | 0.0 | apply_luck_max +10, cha_max +10; status: Detect Invisible, Sense Life; max-stat focus: apply_luck_max, cha_max; status: Sense Life |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_QUIVER` | 55040 | a silken backsheath of flight | 22 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 20201 | a gleaming blacksmith hammer | 1 | 37.2 | 0.0 | damroll +4, hitroll +8; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 500 | a disguise kit | 7 | 0.8 | 0.0 | status: Blind; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 500 | a disguise kit | 7 | 0.8 | 0.0 | status: Blind; observed high-level equipment usage |

### Assassin

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 142445 | a green bladed dagger named 'Sepsis' | 3 | 25.5 | 0.0 | damroll +5, hitroll +3; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

### Mercenary

Observed high-level characters: 8 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 28921 | an obsidian circle | 10 | 83.8 | 3.456 | apply_move +40, dex_max +5; status: Protect Good; max-stat focus: dex_max |
| `WEAR_FINGER_L` | 28921 | an obsidian circle | 10 | 83.8 | 3.456 | apply_move +40, dex_max +5; status: Protect Good; max-stat focus: dex_max |
| `WEAR_NECK_1` | 66454 | a holy crystal medallion | 1 | 49.2 | 0.0 | apply_hit_reg +20, damroll +4; offense: hit/damage |
| `WEAR_NECK_2` | 66454 | a holy crystal medallion | 1 | 49.2 | 0.0 | apply_hit_reg +20, damroll +4; offense: hit/damage |
| `WEAR_BODY` | 70954 | the chameleon suit of transformation | 5 | 37.4 | 0.0 | agi_max +8, dex_max +9; max-stat focus: agi_max, dex_max |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 44174 | the leggings of a MaDMaN | 4 | 77.9 | 2.748 | damroll +3, dex_max +5, hit +30; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FEET` | 26631 | a pair of boots of stealth | 3 | 53.6 | 0.0 | dex_max +5, hit +22; status: Sneak; max-stat focus: dex_max |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 5815 | a set of jagged blood crystal arm plates | 3 | 36.6 | 0.0 | con +13, damroll +4; stat focus: con; offense: hit/damage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 87584 | the cloak of body control | 2 | 35.8 | 0.0 | agi +15, agi_max +4; max-stat focus: agi_max; stat focus: agi |
| `WEAR_WAIST` | 500033 | the mystical sash of the Netherworld | 8 | 7.0 | 0.0 | apply_saving_breath -5, svspell -5; good saves: svspell |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 142419 | the legendary cutlass 'Sanguine Song' | 6 | 31.5 | 0.0 | damroll +5, hitroll +5; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 15906 | a tiny lightning enshrouded black earring | 1 | 53.2 | 0.0 | apply_saving_breath -5, hit +18; status: Fly, Iceshield, Major Mental, Waterbreath |
| `WEAR_EARRING_L` | 131635 | an efreeti earring of tenflames | 1 | 66.3 | 1.356 | hit +25, wis_max +4; status: Major Mental, Prot Fire; max-stat focus: wis_max; status: Major Mental, Prot Fire |
| `WEAR_QUIVER` | 138542 | the epic flute of sleeping and charming | 1 | 60.5 | 0.66 | cha_max +5, hit +25; status: Waterbreath; max-stat focus: cha_max; status: Waterbreath |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 36894 | a huge double-bladed axe called 'Skull Splitter' | 4 | 45.9 | 0.0 | damroll +5, hitroll +7; status: Detect Magic, Prot Fire, Protect Good, Slow Poison; status: Prot Fire; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 78024 | a jagged titanium boomerang | 2 | 25.2 | 0.0 | damroll +4, hitroll +4; offense: hit/damage |

### Bard

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 16530 | a dragonbone ring | 2 | 8.0 | 0.0 | damroll +2, svspell -1; good saves: svspell; offense: hit/damage |
| `WEAR_FINGER_L` | 36883 | a shark engraved bone ring | 1 | 63.9 | 1.068 | dex +8, hit +25; status: Waterbreath; stat focus: dex; status: Waterbreath |
| `WEAR_NECK_1` | 11306 | a blood-soaked collar | 2 | 12.0 | 0.0 | apply_saving_breath +3, damroll +2; offense: hit/damage |
| `WEAR_NECK_2` | 11306 | a blood-soaked collar | 2 | 12.0 | 0.0 | apply_saving_breath +3, damroll +2; offense: hit/damage |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 142422 | an ancient crown of elven royalty | 3 | 49.0 | 0.0 | con_max +5, hit +20; max-stat focus: con_max |
| `WEAR_LEGS` | 83580 | some barbed leggings oozing slime | 1 | 25.4 | 0.0 | damroll +4, svpara -3; status: Fly; good saves: svpara; status: Fly; offense: hit/damage |
| `WEAR_FEET` | 26631 | a pair of boots of stealth | 3 | 53.6 | 0.0 | dex_max +5, hit +22; status: Sneak; max-stat focus: dex_max |
| `WEAR_HANDS` | 21662 | the gauntlets of the lifestealer | 3 | 44.2 | 0.0 | damroll +4, hitroll +4; status: Iceshield, Major Mental, Prot Acid, Prot Fire; status: Iceshield, Major Mental, Prot Acid, Prot Fire; offense: hit/damage |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 71025 | the writhing shield of arms | 2 | 17.9 | 0.0 | damroll +3, svfear -4; status: Detect Magic, Protect Evil, Protect Good; good saves: svfear; offense: hit/damage |
| `WEAR_ABOUT` | 41204 | the cloak of shadow dragons | 3 | 55.0 | 0.0 | hit +20, hitroll +3; status: Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 87549 | the translucent belt of anger | 2 | 13.8 | 0.0 | apply_spell_pulse -1, int_max +4; status: Fireshield; max-stat focus: int_max; status: Fireshield |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 26621 | a wicked black dagger named 'Sanguine Blessing' | 2 | 26.8 | 0.0 | damroll +4, hitroll +4; status: Protect Evil, Protect Good; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 76655 | the mask of armageddon | 1 | 15.6 | 0.0 | damroll +2, hitroll +3; offense: hit/damage |
| `WEAR_EARRING_R` | 25402 | a flaming earring | 3 | 8.4 | 0.0 | apply_saving_breath -5, damroll +2; status: Detect Good, Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 78035 | an anti-matter earring | 2 | 16.9 | 0.0 | damroll +3, hitroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 82502 | a myrabolan backpack | 1 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 29443 | a hand-carved yo-yo | 6 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |
| bard_instrument:flute | 138536 | the legendary flute of sleeping and charming |
| bard_instrument:lyre | 138535 | the legendary lyre of healing and harming |
| bard_instrument:mandolin | 138537 | the legendary mandolin of revelations and forgetfulness |
| bard_instrument:harp | 138538 | the legendary harp of peace and calming |
| bard_instrument:drums | 138533 | the legendary drums of chaos and heroism |
| bard_instrument:horn | 138534 | the legendary horn of flight and dragons |

### Thief

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 142445 | a green bladed dagger named 'Sepsis' | 3 | 25.5 | 0.0 | damroll +5, hitroll +3; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

### Warlock

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 53661 | the shadowy staff of damnation | 5 | 27.2 | 0.0 | con_max +6, int_max +6; status: Sneak; max-stat focus: con_max, int_max |
| `SECONDARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

### MindFlayer

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 53661 | the shadowy staff of damnation | 5 | 27.2 | 0.0 | con_max +6, int_max +6; status: Sneak; max-stat focus: con_max, int_max |
| `SECONDARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

### Alchemist

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

### Berserker

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 23064 | the ring of the berserker lord | 1 | 24.2 | 0.0 | dex -20, str_max +6; status: Fireshield, Prot Fire; max-stat focus: str_max; status: Fireshield, Prot Fire |
| `WEAR_FINGER_L` | 6846 | a thick and jagged onyx ring | 1 | 16.5 | 0.0 | damroll +5; offense: hit/damage |
| `WEAR_NECK_1` | 32628 | a slaadi necklace of Chaos | 7 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78511 | a set of spiked dragonscale arm plates | 2 | 16.5 | 0.0 | damroll +3, str_max +3; max-stat focus: str_max; offense: hit/damage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 41204 | the cloak of shadow dragons | 3 | 55.0 | 0.0 | hit +20, hitroll +3; status: Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 87575 | a strip of studded black pudding | 5 | 32.2 | 0.0 | hit +10, str_max +6; max-stat focus: str_max |
| `WEAR_WRIST_R` | 91020 | a lightbringer bracelet of Pelor | 2 | 16.5 | 0.0 | damroll +3, str_max +3; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 91020 | a lightbringer bracelet of Pelor | 2 | 16.5 | 0.0 | damroll +3, str_max +3; max-stat focus: str_max; offense: hit/damage |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 4222 | an armageddon crystal | 2 | 15.9 | 0.0 | damroll +3, hitroll +2; offense: hit/damage |
| `WEAR_EARRING_L` | 78035 | an anti-matter earring | 2 | 16.9 | 0.0 | damroll +3, hitroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_QUIVER` | 55040 | a silken backsheath of flight | 22 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Reaver

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 38763 | a ring of regeneration | 4 | 62.5 | 0.9 | hit +25, hitroll +5; offense: hit/damage |
| `WEAR_FINGER_L` | 38763 | a ring of regeneration | 4 | 62.5 | 0.9 | hit +25, hitroll +5; offense: hit/damage |
| `WEAR_NECK_1` | 58324 | a necklace of dangling bones and relics | 6 | 45.2 | 0.0 | agi_max +5, apply_move_reg +9, dex_max +6; status: Clarity, Major Mental; max-stat focus: agi_max, dex_max; status: Major Mental |
| `WEAR_NECK_2` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_BODY` | 58367 | an embroidered tunic of the Knights of the Raven | 5 | 23.8 | 0.0 | ac -125, con_max +10; status: Aware, Protect Evil; max-stat focus: con_max; status: Aware |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 83486 | some black leather boots fit with adamantium heel blades | 4 | 18.7 | 0.0 | damroll +3, dex_max +4; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_HANDS` | 77723 | a set of glassteel gloves | 1 | 60.7 | 0.684 | damroll +4, hit +25; offense: hit/damage |
| `WEAR_ARMS` | 44173 | the sleeves of a MaDMaN | 1 | 56.3 | 0.156 | agi_max +4, hit +25; max-stat focus: agi_max |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 47092 | a cloak of crackling energy | 2 | 30.4 | 0.0 | agi +12, agi_max +4; max-stat focus: agi_max; stat focus: agi |
| `WEAR_WAIST` | 77749 | Jadem's magical device of protection | 4 | 39.6 | 3.0 | svpara -5, svspell -5; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Protect Evil, Protect Good, Stone Skin; good saves: svspell, svpara; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Stone Skin |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 87548 | the bracer of Uz's alliance | 6 | 16.5 | 0.0 | apply_combat_pulse -1, damroll +5; offense: hit/damage |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 59299 | a fang-bladed dagger named 'Raven's Claw' | 2 | 31.5 | 0.0 | damroll +5, hitroll +5; offense: hit/damage |
| `WEAR_EYES` | 78430 | a black duergar eyepatch | 5 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FACE` | 91065 | the grim visage of a MaDWoMaN | 3 | 47.2 | 0.0 | hit +20, svfear -6; status: Sneak; good saves: svfear |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 58399 | a multi-phased fish bone earring | 3 | 31.5 | 0.0 | agi_max +4, damroll +3, dex_max +4; status: Prot Gas; max-stat focus: agi_max, dex_max; status: Prot Gas; offense: hit/damage |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 139806 | Ehkahk's badge of Honor | 5 | 17.6 | 0.0 | dex_max +4, str_max +4; max-stat focus: dex_max, str_max |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 89136 | a snuggly cuddlebunny | 3 | 20.4 | 0.0 | hit +8, svspell -3; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_ATTACH_BELT_3` | 83140 | a potion of Ard'gral | 1 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |

### Illusionist

Observed high-level characters: 8 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 40768 | the ring of celestial wonders | 4 | 59.0 | 0.48 | hit +24, int_max +2; status: Iceshield, Prot Acid; max-stat focus: int_max; status: Iceshield, Prot Acid |
| `WEAR_NECK_1` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 142422 | an ancient crown of elven royalty | 3 | 49.0 | 0.0 | con_max +5, hit +20; max-stat focus: con_max |
| `WEAR_LEGS` | 66636 | some bright blue leg plates | 4 | 10.8 | 0.0 | con_max +3, svspell -3; max-stat focus: con_max; good saves: svspell |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 5817 | some razor-knuckled blood crystal gloves | 2 | 43.3 | 0.0 | damroll +4, hit +15; status: Detect Evil, Protect Evil; offense: hit/damage |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 5071 | a despair cloak of Vadatorn | 1 | 63.9 | 1.068 | hit +29, svspell -4; status: Detect Evil, Detect Good, Detect Magic, Protect Good; good saves: svspell |
| `WEAR_WAIST` | 77749 | Jadem's magical device of protection | 4 | 39.6 | 3.0 | svpara -5, svspell -5; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Protect Evil, Protect Good, Stone Skin; good saves: svspell, svpara; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Stone Skin |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78467 | a finely woven crystal bracelet | 2 | 51.7 | 0.0 | hit +25, svpara -3; good saves: svpara |
| `PRIMARY_WEAPON` | 9447 | a staff of power | 3 | 75.8 | 2.496 | hit +30, hitroll +6; status: Detect Invisible; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 25405 | a white hot mask of living flame | 5 | 43.0 | 0.0 | con_max +4, hit +18; max-stat focus: con_max |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 6710 | the codex of the muse of dreams | 9 | 18.0 | 0.0 | agi +5, int +5; stat focus: agi, int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |

### Blighter

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 18001 | the crown of dragons | 5 | 3.8 | 0.0 | apply_saving_breath -7, svspell -2; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 76714 | the gloves of the mind | 6 | 11.8 | 0.0 | apply_mana_reg +5, svspell -2; good saves: svspell |
| `WEAR_ARMS` | 94396 | some cracked bone arm bracers | 4 | 9.4 | 0.0 | con_max +3, svpara -2; max-stat focus: con_max; good saves: svpara |
| `WEAR_SHIELD` | 85721 | the shield proclaimed 'Hope' | 2 | 61.4 | 0.768 | apply_move +30, wis_max +3; status: Protect Evil; max-stat focus: wis_max |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 4031 | a thick belt of trollhide | 3 | 13.2 | 0.0 | con_max +3, str_max +3; max-stat focus: con_max, str_max |
| `WEAR_WRIST_R` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `WEAR_WRIST_L` | 81124 | a blackened bluestone bracer | 4 | 12.2 | 0.0 | svspell -4, wis_max +3; max-stat focus: wis_max; good saves: svspell |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 28913 | the bloody battle goggles | 3 | 34.0 | 0.0 | hit +14, wis_max +3; status: Detect Evil; max-stat focus: wis_max |
| `WEAR_FACE` | 6721 | a mask of autumn leaves | 3 | 2.0 | 0.0 | status: Infravision, Sense Life |
| `WEAR_EARRING_R` | 78423 | a tiny endurium earring | 6 | 14.2 | 0.0 | con +3, con_max +4; max-stat focus: con_max; stat focus: con |
| `WEAR_EARRING_L` | 6725 | a delicate enchanted snowflake | 6 | 35.4 | 0.0 | hitroll +4, int +8; status: Iceshield, Major Mental; stat focus: int; status: Iceshield, Major Mental; offense: hit/damage |
| `WEAR_QUIVER` | 138542 | the epic flute of sleeping and charming | 1 | 60.5 | 0.66 | cha_max +5, hit +25; status: Waterbreath; max-stat focus: cha_max; status: Waterbreath |
| `GUILD_INSIGNIA` | 6704 | a long elegant peacock feather | 4 | 12.3 | 0.0 | hit +5, svspell -2; good saves: svspell |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Dreadlord

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

### Ethermancer

Observed high-level characters: 8 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 16244 | the helm of light | 3 | 35.1 | 0.0 | hit +15, wis_max +3; max-stat focus: wis_max |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 32624 | the boots of the dreamer | 3 | 55.2 | 0.024 | agi_max +7, hit +20; status: Aware, Levitate; max-stat focus: agi_max; status: Aware |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 85721 | the shield proclaimed 'Hope' | 2 | 61.4 | 0.768 | apply_move +30, wis_max +3; status: Protect Evil; max-stat focus: wis_max |
| `WEAR_ABOUT` | 36896 | a cloak of Erebus | 1 | 57.0 | 0.24 | hit +28, svspell -2; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_WAIST` | 83495 | a belt of dangling rat and bat skulls | 6 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `WEAR_WRIST_R` | 83440 | a spiked bracelet of dwarven kind | 2 | 59.0 | 0.48 | hit +20, str_max +5; status: Major Mental, Prot Fire; max-stat focus: str_max; status: Major Mental, Prot Fire |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 25405 | a white hot mask of living flame | 5 | 43.0 | 0.0 | con_max +4, hit +18; max-stat focus: con_max |
| `WEAR_EARRING_R` | 83502 | a mithril stud carved like a fist | 6 | 29.4 | 0.0 | hit +12, str_max +3; max-stat focus: str_max |
| `WEAR_EARRING_L` | 83502 | a mithril stud carved like a fist | 6 | 29.4 | 0.0 | hit +12, str_max +3; max-stat focus: str_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 83296 | an emblem of Shanatar royalty | 6 | 13.2 | 0.0 | con_max +3, str_max +3; max-stat focus: con_max, str_max |
| `WEAR_BACK` | 83600 | a huge reed basket with straps | 2 | 36.0 | 0.0 | apply_move +20; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83611 | a bladed adamantium mace | 2 | 13.2 | 0.0 | dex_max +3, str_max +3; max-stat focus: dex_max, str_max |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Avenger

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

### Theurgist

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_FINGER_L` | 138950 | a ring called 'Soulcatcher' | 15 | 54.1 | 0.0 | hit +25, wis_max +3; max-stat focus: wis_max |
| `WEAR_NECK_1` | 142448 | the necklace of the icecrag royalty | 4 | 53.8 | 0.0 | apply_hit_reg +25, con_max +4; max-stat focus: con_max |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 142445 | a green bladed dagger named 'Sepsis' | 3 | 25.5 | 0.0 | damroll +5, hitroll +3; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |

### Summoner

Observed high-level characters: 5 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 88305 | an enchanted ring of balor bone | 3 | 57.3 | 0.276 | hit +25, int_max +4; status: Farsee; max-stat focus: int_max; status: Farsee |
| `WEAR_NECK_1` | 29459 | a choker with red, white and a blue gems | 1 | 21.6 | 0.0 | int +6, wis +6; stat focus: int, wis |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 142422 | an ancient crown of elven royalty | 3 | 49.0 | 0.0 | con_max +5, hit +20; max-stat focus: con_max |
| `WEAR_LEGS` | 78020 | some mantis leggings | 1 | 55.7 | 0.084 | hit +25, svpara -3; status: Prot Acid; good saves: svpara; status: Prot Acid |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 131649 | Tempest, the swirling gauntlets of storms | 1 | 38.4 | 0.0 | con_max +5, svspell -4; status: Detect Invisible, Fly, Haste, Iceshield, Major Mental; max-stat focus: con_max; good saves: svspell; status: Fly, Haste, Iceshield, Major Mental |
| `WEAR_ARMS` | 38140 | some silken red sleeves | 1 | 28.5 | 0.0 | apply_saving_breath -2, hit +15; observed high-level equipment usage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 9308 | a black silken cloak | 3 | 47.2 | 0.0 | damroll +2, hit +12; status: Iceshield, Major Mental, Prot Fire, Protect Good, Waterbreath; status: Iceshield, Major Mental, Prot Fire, Waterbreath; offense: hit/damage |
| `WEAR_WAIST` | 6738 | a silken sash of raw silk | 2 | 75.8 | 2.496 | apply_move +21, hit +20; observed high-level equipment usage |
| `WEAR_WRIST_R` | 131620 | a bracelet of rainbows | 2 | 57.3 | 0.276 | cha_max +6, hit +19; status: Aware, Farsee, Prot Fire; max-stat focus: cha_max; status: Aware, Farsee, Prot Fire |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 38776 | a dagger called 'Stealth' | 2 | 31.7 | 0.0 | damroll +3, hitroll +7; status: Sneak; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 25705 | the blindfold of sight | 2 | 45.8 | 0.0 | apply_luck_max +10, cha_max +10; status: Detect Invisible, Sense Life; max-stat focus: apply_luck_max, cha_max; status: Sense Life |
| `WEAR_FACE` | 81419 | mask of the future | 2 | 73.1 | 2.172 | hit +35, svspell -4; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_EARRING_R` | 6725 | a delicate enchanted snowflake | 6 | 35.4 | 0.0 | hitroll +4, int +8; status: Iceshield, Major Mental; stat focus: int; status: Iceshield, Major Mental; offense: hit/damage |
| `WEAR_EARRING_L` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_QUIVER` | 25757 | the quiver of holding | 4 | 20.0 | 0.0 | agi_max +5, hitroll +3; max-stat focus: agi_max; offense: hit/damage |
| `GUILD_INSIGNIA` | 31312 | a necromantic death shroud | 9 | 28.3 | 0.0 | apply_move +10, hit +5; status: Absorb; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 139805 | A swirling mass of black smoke and hot ash | 4 | 66.0 | 1.32 | apply_move +30, svpara -8; status: Absorb; good saves: svpara |
| `WEAR_ATTACH_BELT_2` | 33705 | a crystalline lyre | 2 | 13.2 | 0.0 | apply_luck_max +3, cha_max +3; max-stat focus: apply_luck_max, cha_max |
| `WEAR_ATTACH_BELT_3` | 402052 | a lavishly plumed quill pen | 2 | 19.0 | 0.0 | hit +10; observed high-level equipment usage |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 7 | the master spellbook |

### Dragoon

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 142445 | a green bladed dagger named 'Sepsis' | 3 | 25.5 | 0.0 | damroll +5, hitroll +3; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 377 | a large leather backpack | 18 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 400222 | a rugged adventurers satchel | 30 | 9.5 | 0.0 | hit +5; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_3` | 402052 | a lavishly plumed quill pen | 2 | 19.0 | 0.0 | hit +10; observed high-level equipment usage |

## Full enhanceable equipment profiles

`power` and `risk` are analyzer heuristics used to suppress extreme outliers. They are not the game’s combat formula and should be retuned after live Chaos playtesting.

### Warrior

Observed high-level characters: 10 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 38763 | a ring of regeneration | 4 | 62.5 | 0.9 | hit +25, hitroll +5; offense: hit/damage |
| `WEAR_NECK_1` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_2` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 88301 | a crown of burning flames | 5 | 17.9 | 0.0 | apply_combat_pulse -1, damroll +3; status: Aware, Prot Fire, Sense Life; status: Aware, Prot Fire, Sense Life; offense: hit/damage |
| `WEAR_LEGS` | 44174 | the leggings of a MaDMaN | 4 | 77.9 | 2.748 | damroll +3, dex_max +5, hit +30; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 83665 | some finely-detailed platinum armplates | 1 | 53.1 | 0.0 | hit +25, svspell -4; good saves: svspell |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 19916 | a black hooded cloak of Isha the Wanderer | 2 | 22.5 | 0.0 | damroll +5, hitroll +2; offense: hit/damage |
| `WEAR_WAIST` | 87575 | a strip of studded black pudding | 5 | 32.2 | 0.0 | hit +10, str_max +6; max-stat focus: str_max |
| `WEAR_WRIST_R` | 83235 | a feathered bracelet of Nax'Varan nobility | 2 | 63.1 | 0.972 | hit +31, svfear -3; good saves: svfear |
| `WEAR_WRIST_L` | 38774 | a bracer of might | 2 | 51.7 | 0.0 | damroll +3, hit +22; offense: hit/damage |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 22032 | the demonic warmace 'Pure-Dark' | 3 | 26.8 | 0.0 | damroll +4, hitroll +4; status: Absorb, Protect Good; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 91065 | the grim visage of a MaDWoMaN | 3 | 47.2 | 0.0 | hit +20, svfear -6; status: Sneak; good saves: svfear |
| `WEAR_EARRING_R` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 83444 | an emblem of the Wyrmslayer | 5 | 0.0 | 0.0 | apply_saving_breath -4; observed high-level equipment usage |
| `WEAR_BACK` | 98911 | a huge zanthium broad sword | 1 | 36.5 | 0.0 | damroll +5, hitroll +5; status: Iceshield; status: Iceshield; offense: hit/damage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Ranger

Observed high-level characters: 3 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 19901 | a shiny crimson ring | 3 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FINGER_L` | 19901 | a shiny crimson ring | 3 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_1` | 98949 | a necklace of halfling ears | 7 | 17.6 | 0.0 | damroll +2, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_2` | 87523 | the bloody eyeballs on a sinew | 4 | 24.2 | 0.0 | con_max +5, str_max +6; max-stat focus: con_max, str_max |
| `WEAR_BODY` | 70954 | the chameleon suit of transformation | 5 | 37.4 | 0.0 | agi_max +8, dex_max +9; max-stat focus: agi_max, dex_max |
| `WEAR_HEAD` | 83312 | a spiked chromium dragonscale helmet | 2 | 27.6 | 0.0 | con_max +4, str_max +4; status: Aware, Iceshield, Major Mental; max-stat focus: con_max, str_max; status: Aware, Iceshield, Major Mental |
| `WEAR_LEGS` | 42169 | some flexible leggings of hellfire | 3 | 21.4 | 0.0 | damroll +2, dex_max +4; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire; offense: hit/damage |
| `WEAR_FEET` | 83486 | some black leather boots fit with adamantium heel blades | 4 | 18.7 | 0.0 | damroll +3, dex_max +4; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 31517 | the pauldrons of the tide | 3 | 11.6 | 0.0 | str_max +4, svspell -2; max-stat focus: str_max; good saves: svspell |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 28954 | a curtain of elemental fire | 7 | 12.6 | 0.0 | status: Clarity, Fireshield, Prot Fire, Sneak; status: Fireshield, Prot Fire |
| `WEAR_WAIST` | 87707 | an array of flesh hooks and chains | 2 | 12.5 | 0.0 | con -10, damroll +3; status: Aware, Protect Evil, Protect Good; status: Aware; offense: hit/damage |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 87523 | the bloody eyeballs on a sinew | 4 | 24.2 | 0.0 | con_max +5, str_max +6; max-stat focus: con_max, str_max |
| `PRIMARY_WEAPON` | 40781 | a massive longsword dubbed 'Dusk and Dawn' | 3 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `SECONDARY_WEAPON` | 99700 | a flaming longsword emblazoned 'Cinder' | 1 | 24.9 | 0.0 | damroll +3, hitroll +3; status: Prot Fire; status: Prot Fire; offense: hit/damage |
| `WEAR_EYES` | 83372 | some marksman's goggles | 1 | 21.8 | 0.0 | dex_max +4, hitroll +4; status: Farsee; max-stat focus: dex_max; status: Farsee; offense: hit/damage |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 29424 | a gnomish ear amplifier | 4 | 37.0 | 0.0 | apply_move +10, hit +10; observed high-level equipment usage |
| `WEAR_EARRING_L` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Psionicist

Observed high-level characters: 6 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 58417 | a band of parting prevented | 1 | 79.8 | 2.976 | apply_hit_reg +37, con_max +6; max-stat focus: con_max |
| `WEAR_NECK_1` | 28153 | a pleasant necklace of flowers | 2 | 86.1 | 3.732 | apply_move +23, hit +23; status: Sense Life |
| `WEAR_NECK_2` | 42226 | an amulet of Nine Hells | 1 | 38.0 | 0.0 | apply_saving_breath -2, hit +20; observed high-level equipment usage |
| `WEAR_BODY` | 38772 | the legendary platemail of defense | 4 | 85.8 | 3.696 | con_max +4, hit +40; status: Aware; max-stat focus: con_max; status: Aware |
| `WEAR_HEAD` | 55429 | the spiked crown of mental resistance | 2 | 46.8 | 0.0 | hit +20, pow_max +4; max-stat focus: pow_max |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 20924 | a pair of bear-hide mountain boots | 1 | 72.0 | 2.04 | apply_move +35, hitroll +3; offense: hit/damage |
| `WEAR_HANDS` | 70968 | the gloves of magic fingertips | 1 | 50.4 | 0.0 | hit +18, int +9; stat focus: int |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 19638 | the shield of the battle dragon | 3 | 8.8 | 0.0 | ac -20, con_max +4; max-stat focus: con_max |
| `WEAR_ABOUT` | 3536 | the cloak of the demon kings | 2 | 61.2 | 0.744 | hit +30, svspell -3; good saves: svspell |
| `WEAR_WAIST` | 16217 | a belt of fresh blood | 7 | 17.6 | 0.0 | con_max +5, damroll +2; max-stat focus: con_max; offense: hit/damage |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 3546 | a bracer of pit worm scales | 2 | 53.8 | 0.0 | hit +24, svspell -3; status: Prot Acid; good saves: svspell; status: Prot Acid |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 27030 | a stone mask | 1 | 67.6 | 1.512 | con +8, hit +28; stat focus: con |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 28981 | a hoop of twisting obsidian | 1 | 42.2 | 0.0 | hit +20, svspell -3; good saves: svspell |
| `WEAR_QUIVER` | 38444 | a glowing bard sack | 7 | 32.4 | 0.0 | apply_move +15, dex +3; stat focus: dex |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 40425 | a backpack made of hemp | 3 | 23.4 | 0.0 | apply_move +10, str +3; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 132015 | a severed illithid tentacle | 1 | 56.3 | 0.156 | hit +25, pow_max +4; max-stat focus: pow_max |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Paladin

Observed high-level characters: 3 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 66231 | the steel ring of greater physical resistance | 7 | 0.0 | 0.0 | ac -30; observed high-level equipment usage |
| `WEAR_NECK_1` | 83346 | a polished mithril gorget of Hammerhelm | 8 | 19.0 | 0.0 | ac -40, hit +10; observed high-level equipment usage |
| `WEAR_NECK_2` | 83324 | a mithril and silver neckguard | 4 | 9.0 | 0.0 | ac -22, str +5; stat focus: str |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 83383 | a polished mithril and dragonscale helmet | 7 | 0.0 | 0.0 | ac -40, apply_saving_breath -3; observed high-level equipment usage |
| `WEAR_LEGS` | 83382 | some mithril and dragonscale leg plates | 7 | 0.0 | 0.0 | ac -40, apply_saving_breath -3; observed high-level equipment usage |
| `WEAR_FEET` | 44194 | the boots of a MaDMaN | 12 | 4.0 | 0.0 | str -5; status: Prot Gas |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 70979 | some scintillating sleeves | 10 | 18.0 | 0.0 | agi +5, dex +5; stat focus: agi, dex |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 5503 | the terrifying shroud of undead power | 9 | 0.0 | 0.0 | ac -85, con -15; observed high-level equipment usage |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 83228 | a spiked mithril wristguard | 8 | 13.2 | 0.0 | ac -20, damroll +4; offense: hit/damage |
| `WEAR_WRIST_L` | 120013 | some heavy mithril bracers | 6 | 10.8 | 0.0 | ac -20, str +6; stat focus: str |
| `PRIMARY_WEAPON` | 78417 | a massive blackrock hammer named 'Searing Fist' | 2 | 31.8 | 0.0 | damroll +6, hitroll +4; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 83233 | some catseye inventor's goggles | 6 | 26.4 | 0.0 | apply_luck_max +7, pow_max +5; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 33703 | a silver tooth earring | 12 | 9.0 | 0.0 | ac -15, hitroll +3; offense: hit/damage |
| `WEAR_EARRING_L` | 33703 | a silver tooth earring | 12 | 9.0 | 0.0 | ac -15, hitroll +3; offense: hit/damage |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 77740 | a frost dragon's eye | 3 | 18.8 | 0.0 | apply_saving_breath -7, con +10; status: Detect Invisible; stat focus: con |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 25757 | the quiver of holding | 4 | 20.0 | 0.0 | agi_max +5, hitroll +3; max-stat focus: agi_max; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Anti-Paladin

Observed high-level characters: 4 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 42903 | a spiked electrum ring | 1 | 37.2 | 0.0 | damroll +2, dex +17; stat focus: dex; offense: hit/damage |
| `WEAR_NECK_1` | 38610 | an amulet of fire dragon's blood | 4 | 48.8 | 0.0 | apply_saving_breath -6, hit +22; status: Farsee, Prot Fire |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87530 | some blue-tinted chainmail leggings | 3 | 51.2 | 0.0 | damroll +4, hit +20; offense: hit/damage |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 5815 | a set of jagged blood crystal arm plates | 3 | 36.6 | 0.0 | con +13, damroll +4; stat focus: con; offense: hit/damage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 41204 | the cloak of shadow dragons | 3 | 55.0 | 0.0 | hit +20, hitroll +3; status: Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 16217 | a belt of fresh blood | 7 | 17.6 | 0.0 | con_max +5, damroll +2; max-stat focus: con_max; offense: hit/damage |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 38660 | the ancestral bracelet of Sevenoaks | 4 | 18.4 | 0.0 | con_max +4, wis_max +4; status: Barkskin; max-stat focus: con_max, wis_max |
| `PRIMARY_WEAPON` | 21633 | a huge gladiators blade | 1 | 28.8 | 0.0 | damroll +6, hitroll +3; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 13053 | glyphs of power tattooed around one eye | 3 | 13.2 | 0.0 | damroll +2, dex_max +3; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FACE` | 33701 | a lithixl beak | 13 | 22.8 | 0.0 | ac -15, hit +12; observed high-level equipment usage |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_QUIVER` | 89169 | a pantherhide quiver | 4 | 11.0 | 0.0 | agi_max +2, dex_max +3; max-stat focus: agi_max, dex_max |
| `GUILD_INSIGNIA` | 31312 | a necromantic death shroud | 9 | 28.3 | 0.0 | apply_move +10, hit +5; status: Absorb; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 18000 | a longsword named 'the Answerer' | 2 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Cleric

Observed high-level characters: 12 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 38644 | a shiny ring of the faeries | 2 | 49.4 | 0.0 | hit +22, svspell -4; status: Farsee, Sense Life; good saves: svspell; status: Farsee, Sense Life |
| `WEAR_FINGER_L` | 26639 | the ring of the void | 3 | 70.1 | 1.812 | hit +33, svfear -4; status: Infravision, Invisible; good saves: svfear; status: Infravision |
| `WEAR_NECK_1` | 38720 | a necklace of vampiric power | 2 | 54.1 | 0.0 | hit +19, str +10; stat focus: str |
| `WEAR_NECK_2` | 87601 | the amulet named "Flow" | 3 | 46.8 | 0.0 | hit +20, wis_max +4; max-stat focus: wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 21641 | the royal crown of Aravne | 1 | 49.0 | 0.0 | hit +20, wis_max +5; max-stat focus: wis_max |
| `WEAR_LEGS` | 91048 | a pair of blood-stained wyrm scale leggings | 1 | 53.5 | 0.0 | apply_move_reg +6, apply_saving_breath -5, hit +17; status: Detect Evil, Detect Good, Detect Magic, Fly; status: Fly |
| `WEAR_FEET` | 87540 | the slippers of rot and decay | 3 | 30.4 | 0.0 | con_max +6, svpara -4; status: Prot Acid, Prot Fire, Protect Evil, Slow Poison; max-stat focus: con_max; good saves: svpara; status: Prot Acid, Prot Fire |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 20254 | some scintilating sleeves | 2 | 38.4 | 0.0 | hit +18, svpara -3; good saves: svpara |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 6802 | a tattered dragon-scale bodycloak | 1 | 73.5 | 2.22 | hit +35, svspell -5; good saves: svspell |
| `WEAR_WAIST` | 6738 | a silken sash of raw silk | 2 | 75.8 | 2.496 | apply_move +21, hit +20; observed high-level equipment usage |
| `WEAR_WRIST_R` | 130021 | a sapphire bracelet | 1 | 46.4 | 0.0 | apply_move +20, con_max +4; status: Protect Evil, Protect Good; max-stat focus: con_max |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 132022 | the holy warhammer 'Discipline' | 1 | 25.2 | 0.0 | damroll +4, hitroll +4; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 25405 | a white hot mask of living flame | 5 | 43.0 | 0.0 | con_max +4, hit +18; max-stat focus: con_max |
| `WEAR_EARRING_R` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_EARRING_L` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_QUIVER` | 55040 | a silken backsheath of flight | 22 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Monk

Observed high-level characters: 6 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 22031 | a tarnished bronze ring | 1 | 31.0 | 0.0 | hit +10, hitroll +4; offense: hit/damage |
| `WEAR_NECK_1` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 5817 | some razor-knuckled blood crystal gloves | 2 | 43.3 | 0.0 | damroll +4, hit +15; status: Detect Evil, Protect Evil; offense: hit/damage |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 28988 | the cloak of multi-colored beads | 2 | 43.2 | 0.0 | hit +20, svpara -3; status: Aware; good saves: svpara; status: Aware |
| `WEAR_WAIST` | 88917 | a belt of liquid links | 5 | 27.6 | 0.0 | apply_luck_max +3, hit +10; status: Waterbreath; max-stat focus: apply_luck_max; status: Waterbreath |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 44877 | the eyes of love | 1 | 56.1 | 0.132 | damroll +2, hit +25; status: Infravision, Sense Life; status: Infravision, Sense Life; offense: hit/damage |
| `WEAR_FACE` | 71028 | a veil named 'Sorrow' | 3 | 4.8 | 0.0 | status: Protect Evil, Regenerate; status: Regenerate |
| `WEAR_EARRING_R` | 6725 | a delicate enchanted snowflake | 6 | 35.4 | 0.0 | hitroll +4, int +8; status: Iceshield, Major Mental; stat focus: int; status: Iceshield, Major Mental; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 87564 | the quiver of the ages | 3 | 24.4 | 0.0 | hit +10, svpara -1; status: Regenerate; good saves: svpara; status: Regenerate |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 29443 | a hand-carved yo-yo | 6 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Druid

Observed high-level characters: 4 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 21645 | the royal ring of Aravne | 2 | 66.5 | 1.38 | hit +35; observed high-level equipment usage |
| `WEAR_FINGER_L` | 6728 | a ring of the water nymph | 2 | 21.0 | 0.0 | hit +10; status: Waterbreath |
| `WEAR_NECK_1` | 6703 | a druidic necklace of fish scales | 1 | 13.4 | 0.0 | wis +5, wis_max +2; max-stat focus: wis_max; stat focus: wis |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 82707 | a pair of feathered gloves | 1 | 29.9 | 0.0 | agi +5, hit +11; stat focus: agi |
| `WEAR_ARMS` | 99538 | a pair of sleeves woven from mist | 2 | 37.0 | 0.0 | apply_move +10, hit +10; observed high-level equipment usage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 83593 | the shroud of corruption | 2 | 36.1 | 0.0 | con_max +3, hit +15; status: Sense Life; max-stat focus: con_max; status: Sense Life |
| `WEAR_WAIST` | 14045 | a belt encrusted with black sapphires | 1 | 51.9 | 0.0 | hit +25, str_max +2; max-stat focus: str_max |
| `WEAR_WRIST_R` | 35229 | a lizard tail bracer | 1 | 32.4 | 0.0 | apply_move +15, apply_move_reg +3; observed high-level equipment usage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 96012 | a huge adamantium mace named 'Mistweave' | 1 | 38.5 | 0.0 | damroll +5, hitroll +7; status: Sense Life; status: Sense Life; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 3537 | a bone visor | 2 | 20.8 | 0.0 | int_max +5, wis_max +4; status: Farsee; max-stat focus: int_max, wis_max; status: Farsee |
| `WEAR_FACE` | 7677 | mask of battles past | 5 | 22.8 | 0.0 | con_max +3, dex +9; max-stat focus: con_max; stat focus: dex |
| `WEAR_EARRING_R` | 29458 | a pair of silver earrings with red rubies | 6 | 14.4 | 0.0 | apply_move +8; observed high-level equipment usage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 35808 | an embroidered illithid-hide backsheath | 2 | 9.0 | 0.0 | agi +5; stat focus: agi |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Shaman

Observed high-level characters: 14 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 58379 | an oathstone pendant | 3 | 47.8 | 0.0 | con_max +4, hit +20; status: Sense Life; max-stat focus: con_max; status: Sense Life |
| `WEAR_NECK_2` | 25712 | an amulet of wisdom | 2 | 39.5 | 0.0 | hit +15, wis_max +5; max-stat focus: wis_max |
| `WEAR_BODY` | 9114 | the hide of the apocalypse demon | 1 | 57.1 | 0.252 | hit +19, hitroll +5; status: Prot Fire; status: Prot Fire; offense: hit/damage |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 47082 | some wyvern-skull poleyns | 2 | 54.0 | 0.0 | apply_move +30, apply_saving_breath -3; observed high-level equipment usage |
| `WEAR_FEET` | 32624 | the boots of the dreamer | 3 | 55.2 | 0.024 | agi_max +7, hit +20; status: Aware, Levitate; max-stat focus: agi_max; status: Aware |
| `WEAR_HANDS` | 55404 | a pair of fingerless silk gloves | 1 | 54.4 | 0.0 | agi_max +5, apply_move_reg +3, hit +20; max-stat focus: agi_max |
| `WEAR_ARMS` | 47081 | some wyvern-skull pauldrons | 3 | 72.0 | 2.04 | apply_move +40, apply_saving_breath -4; observed high-level equipment usage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 33702 | a deep hooded cloak of sorrow | 2 | 47.0 | 0.0 | apply_move +5, hit +20; observed high-level equipment usage |
| `WEAR_WAIST` | 87709 | the woven entrails of an unfortunate soul | 3 | 39.4 | 0.0 | apply_hit_reg +14, apply_move_reg +7; status: Protect Evil, Protect Good; observed high-level equipment usage |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 38444 | a glowing bard sack | 7 | 32.4 | 0.0 | apply_move +15, dex +3; stat focus: dex |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| three_sphere_high_circle_totem | 88315 | a fire-imbued totem of Kossuth |

### Sorcerer

Observed high-level characters: 14 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_FINGER_L` | 40768 | the ring of celestial wonders | 4 | 59.0 | 0.48 | hit +24, int_max +2; status: Iceshield, Prot Acid; max-stat focus: int_max; status: Iceshield, Prot Acid |
| `WEAR_NECK_1` | 87607 | a tuft of MADMAN'S hair | 2 | 54.5 | 0.0 | hit +25, svfear -5; good saves: svfear |
| `WEAR_NECK_2` | 38610 | an amulet of fire dragon's blood | 4 | 48.8 | 0.0 | apply_saving_breath -6, hit +22; status: Farsee, Prot Fire |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 25719 | the helm of the dragonlords | 1 | 49.7 | 0.0 | apply_move_reg +9, hit +15; status: Stone Skin |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 32630 | the gloves of the adept | 1 | 45.6 | 0.0 | hit +20, int_max +3; status: Aware; max-stat focus: int_max; status: Aware |
| `WEAR_ARMS` | 32430 | some glowing sleeves made of devil skin | 3 | 43.3 | 0.0 | hit +15, int_max +4; status: Prot Fire; max-stat focus: int_max; status: Prot Fire |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 70813 | opalescent robes of the archmagi | 2 | 75.3 | 2.436 | hit +33, int +7; stat focus: int |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 38621 | a satanic bracelet | 5 | 59.3 | 0.516 | hit +25, int_max +5; status: Protect Good; max-stat focus: int_max |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 44485 | the sapphire eye of a beholder guardian | 1 | 43.2 | 0.0 | hit +20, svspell -3; status: Infravision; good saves: svspell; status: Infravision |
| `WEAR_FACE` | 91065 | the grim visage of a MaDWoMaN | 3 | 47.2 | 0.0 | hit +20, svfear -6; status: Sneak; good saves: svfear |
| `WEAR_EARRING_R` | 66675 | a shimmering shark tooth earring | 1 | 44.2 | 0.0 | hit +20, svpara -3; status: Waterbreath; good saves: svpara; status: Waterbreath |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83480 | a long roc-feather quill | 1 | 28.8 | 0.0 | agi +8, int +8; stat focus: agi, int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |

### Necromancer

Observed high-level characters: 12 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 97015 | a red earthstone ring | 5 | 35.4 | 0.0 | dex_max +3, hit +12; status: Prot Fire; max-stat focus: dex_max; status: Prot Fire |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 6843 | a banded mail shirt | 1 | 50.6 | 0.0 | hit +20, wis +7; stat focus: wis |
| `WEAR_HEAD` | 87594 | the troll fez of true clarity | 1 | 64.2 | 1.104 | hit +28, int_max +5; max-stat focus: int_max |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 19911 | mystical boots of Volo the Traveller | 2 | 49.7 | 0.0 | apply_move_reg +12, hit +9, int_max +5; max-stat focus: int_max |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 44827 | a small circular shield of blue flames | 2 | 52.0 | 0.0 | hit +22, svpara -3; status: Prot Fire; good saves: svpara; status: Prot Fire |
| `WEAR_ABOUT` | 31527 | the living oceanic cloak | 3 | 43.4 | 0.0 | con_max +5, hit +16; status: Waterbreath; max-stat focus: con_max; status: Waterbreath |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 9447 | a staff of power | 3 | 75.8 | 2.496 | hit +30, hitroll +6; status: Detect Invisible; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 5048 | some eyes of the salamander | 1 | 41.4 | 0.0 | agi +4, hit +18; stat focus: agi |
| `WEAR_FACE` | 38455 | some silk panties | 4 | 54.0 | 0.0 | agi +15, cha +15; stat focus: agi, cha |
| `WEAR_EARRING_R` | 78036 | an anti-matter earring | 1 | 53.1 | 0.0 | hit +25, svspell -4; good saves: svspell |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 78037 | a massive tome | 1 | 37.5 | 0.0 | hit +15, int +5; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |

### Conjurer

Observed high-level characters: 4 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 43563 | a ring of elemental might | 1 | 50.0 | 0.0 | apply_luck_max +5, hit +20; status: Farsee; max-stat focus: apply_luck_max; status: Farsee |
| `WEAR_FINGER_L` | 6502 | a ring of citizenship | 2 | 35.2 | 0.0 | con +9, hit +10; stat focus: con |
| `WEAR_NECK_1` | 43015 | a cloak of dragons | 3 | 27.8 | 0.0 | con_max +4, hit +10; max-stat focus: con_max |
| `WEAR_NECK_2` | 38663 | an amulet of chaos | 3 | 47.6 | 0.0 | apply_saving_breath -3, hit +24; status: Infravision, Sense Life |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 76722 | the hood of cunning | 2 | 28.2 | 0.0 | hit +12, int_max +2; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_LEGS` | 7678 | greaves of avoidance | 2 | 39.0 | 0.0 | agi +9, hit +12; stat focus: agi |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 31527 | the living oceanic cloak | 3 | 43.4 | 0.0 | con_max +5, hit +16; status: Waterbreath; max-stat focus: con_max; status: Waterbreath |
| `WEAR_WAIST` | 132024 | the threads of astral projection | 3 | 40.1 | 0.0 | hit +15, pow +6; status: Absorb; stat focus: pow |
| `WEAR_WRIST_R` | 38621 | a satanic bracelet | 5 | 59.3 | 0.516 | hit +25, int_max +5; status: Protect Good; max-stat focus: int_max |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 78064 | a mask of hades | 5 | 18.0 | 0.0 | agi_max +5, svspell -3; status: Detect Invisible, Waterbreath; max-stat focus: agi_max; good saves: svspell; status: Waterbreath |
| `WEAR_EARRING_R` | 23811 | a delicate nipple ring | 4 | 8.8 | 0.0 | cha_max +2, int_max +2; max-stat focus: cha_max, int_max |
| `WEAR_EARRING_L` | 87727 | an emerald disc fashioned from bone | 4 | 30.0 | 0.0 | con +10, con_max +5; status: Aware; max-stat focus: con_max; stat focus: con; status: Aware |
| `WEAR_QUIVER` | 38444 | a glowing bard sack | 7 | 32.4 | 0.0 | apply_move +15, dex +3; stat focus: dex |
| `GUILD_INSIGNIA` | 6736 | a wisp of shimmering faerie light | 3 | 20.0 | 0.0 | hit +10; status: Ultravision |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 33711 | the lost book of 'Magic' | 5 | 9.8 | 0.0 | int +5; status: Levitate; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |

### Rogue

Observed high-level characters: 6 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 26639 | the ring of the void | 3 | 70.1 | 1.812 | hit +33, svfear -4; status: Infravision, Invisible; good saves: svfear; status: Infravision |
| `WEAR_FINGER_L` | 28921 | an obsidian circle | 10 | 83.8 | 3.456 | apply_move +40, dex_max +5; status: Protect Good; max-stat focus: dex_max |
| `WEAR_NECK_1` | 99702 | a fiery chain of hades | 1 | 53.2 | 0.0 | apply_move +10, hit +18; status: Infravision |
| `WEAR_NECK_2` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_BODY` | 70954 | the chameleon suit of transformation | 5 | 37.4 | 0.0 | agi_max +8, dex_max +9; max-stat focus: agi_max, dex_max |
| `WEAR_HEAD` | 83209 | a derroskin admiral's hat | 1 | 62.9 | 0.948 | dex_max +7, hit +25; max-stat focus: dex_max |
| `WEAR_LEGS` | 83666 | some finely-detailed platinum legplates | 1 | 53.1 | 0.0 | hit +25, svspell -4; good saves: svspell |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 87600 | the vambraces named "Ebb" | 2 | 35.2 | 0.0 | agi_max +10, con_max +6; max-stat focus: agi_max, con_max |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 16269 | a tight green cloak | 5 | 12.1 | 0.0 | damroll +1; status: Detect Invisible, Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 132024 | the threads of astral projection | 3 | 40.1 | 0.0 | hit +15, pow +6; status: Absorb; stat focus: pow |
| `WEAR_WRIST_R` | 87538 | the power infused bracelet of dark-mithril | 3 | 39.6 | 0.0 | damroll +8, str_max +6; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 38621 | a satanic bracelet | 5 | 59.3 | 0.516 | hit +25, int_max +5; status: Protect Good; max-stat focus: int_max |
| `PRIMARY_WEAPON` | 11307 | the dark rod named 'Punisher' | 2 | 31.7 | 0.0 | damroll +3, hitroll +7; status: Protect Good; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 25705 | the blindfold of sight | 2 | 45.8 | 0.0 | apply_luck_max +10, cha_max +10; status: Detect Invisible, Sense Life; max-stat focus: apply_luck_max, cha_max; status: Sense Life |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_QUIVER` | 55040 | a silken backsheath of flight | 22 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 20201 | a gleaming blacksmith hammer | 1 | 37.2 | 0.0 | damroll +4, hitroll +8; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 29443 | a hand-carved yo-yo | 6 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Assassin

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Mercenary

Observed high-level characters: 8 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 28921 | an obsidian circle | 10 | 83.8 | 3.456 | apply_move +40, dex_max +5; status: Protect Good; max-stat focus: dex_max |
| `WEAR_FINGER_L` | 28921 | an obsidian circle | 10 | 83.8 | 3.456 | apply_move +40, dex_max +5; status: Protect Good; max-stat focus: dex_max |
| `WEAR_NECK_1` | 66454 | a holy crystal medallion | 1 | 49.2 | 0.0 | apply_hit_reg +20, damroll +4; offense: hit/damage |
| `WEAR_NECK_2` | 66454 | a holy crystal medallion | 1 | 49.2 | 0.0 | apply_hit_reg +20, damroll +4; offense: hit/damage |
| `WEAR_BODY` | 70954 | the chameleon suit of transformation | 5 | 37.4 | 0.0 | agi_max +8, dex_max +9; max-stat focus: agi_max, dex_max |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 44174 | the leggings of a MaDMaN | 4 | 77.9 | 2.748 | damroll +3, dex_max +5, hit +30; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FEET` | 26631 | a pair of boots of stealth | 3 | 53.6 | 0.0 | dex_max +5, hit +22; status: Sneak; max-stat focus: dex_max |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 5815 | a set of jagged blood crystal arm plates | 3 | 36.6 | 0.0 | con +13, damroll +4; stat focus: con; offense: hit/damage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 87584 | the cloak of body control | 2 | 35.8 | 0.0 | agi +15, agi_max +4; max-stat focus: agi_max; stat focus: agi |
| `WEAR_WAIST` | 14045 | a belt encrusted with black sapphires | 1 | 51.9 | 0.0 | hit +25, str_max +2; max-stat focus: str_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 38725 | a frost brand named 'Stormbringer' | 2 | 47.1 | 0.0 | damroll +7, hitroll +5; status: Iceshield, Major Mental; status: Iceshield, Major Mental; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 15906 | a tiny lightning enshrouded black earring | 1 | 53.2 | 0.0 | apply_saving_breath -5, hit +18; status: Fly, Iceshield, Major Mental, Waterbreath |
| `WEAR_EARRING_L` | 131635 | an efreeti earring of tenflames | 1 | 66.3 | 1.356 | hit +25, wis_max +4; status: Major Mental, Prot Fire; max-stat focus: wis_max; status: Major Mental, Prot Fire |
| `WEAR_QUIVER` | 94324 | a quiver smeared with blood | 7 | 21.6 | 0.0 | agi +4, dex +8; stat focus: agi, dex |
| `GUILD_INSIGNIA` | 31312 | a necromantic death shroud | 9 | 28.3 | 0.0 | apply_move +10, hit +5; status: Absorb; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 36894 | a huge double-bladed axe called 'Skull Splitter' | 4 | 45.9 | 0.0 | damroll +5, hitroll +7; status: Detect Magic, Prot Fire, Protect Good, Slow Poison; status: Prot Fire; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 78024 | a jagged titanium boomerang | 2 | 25.2 | 0.0 | damroll +4, hitroll +4; offense: hit/damage |

### Bard

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 16530 | a dragonbone ring | 2 | 8.0 | 0.0 | damroll +2, svspell -1; good saves: svspell; offense: hit/damage |
| `WEAR_FINGER_L` | 36883 | a shark engraved bone ring | 1 | 63.9 | 1.068 | dex +8, hit +25; status: Waterbreath; stat focus: dex; status: Waterbreath |
| `WEAR_NECK_1` | 11306 | a blood-soaked collar | 2 | 12.0 | 0.0 | apply_saving_breath +3, damroll +2; offense: hit/damage |
| `WEAR_NECK_2` | 11306 | a blood-soaked collar | 2 | 12.0 | 0.0 | apply_saving_breath +3, damroll +2; offense: hit/damage |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 83580 | some barbed leggings oozing slime | 1 | 25.4 | 0.0 | damroll +4, svpara -3; status: Fly; good saves: svpara; status: Fly; offense: hit/damage |
| `WEAR_FEET` | 26631 | a pair of boots of stealth | 3 | 53.6 | 0.0 | dex_max +5, hit +22; status: Sneak; max-stat focus: dex_max |
| `WEAR_HANDS` | 21662 | the gauntlets of the lifestealer | 3 | 44.2 | 0.0 | damroll +4, hitroll +4; status: Iceshield, Major Mental, Prot Acid, Prot Fire; status: Iceshield, Major Mental, Prot Acid, Prot Fire; offense: hit/damage |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 71025 | the writhing shield of arms | 2 | 17.9 | 0.0 | damroll +3, svfear -4; status: Detect Magic, Protect Evil, Protect Good; good saves: svfear; offense: hit/damage |
| `WEAR_ABOUT` | 41204 | the cloak of shadow dragons | 3 | 55.0 | 0.0 | hit +20, hitroll +3; status: Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 87549 | the translucent belt of anger | 2 | 13.8 | 0.0 | apply_spell_pulse -1, int_max +4; status: Fireshield; max-stat focus: int_max; status: Fireshield |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `PRIMARY_WEAPON` | 26621 | a wicked black dagger named 'Sanguine Blessing' | 2 | 26.8 | 0.0 | damroll +4, hitroll +4; status: Protect Evil, Protect Good; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 76655 | the mask of armageddon | 1 | 15.6 | 0.0 | damroll +2, hitroll +3; offense: hit/damage |
| `WEAR_EARRING_R` | 25402 | a flaming earring | 3 | 8.4 | 0.0 | apply_saving_breath -5, damroll +2; status: Detect Good, Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 78035 | an anti-matter earring | 2 | 16.9 | 0.0 | damroll +3, hitroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 82502 | a myrabolan backpack | 1 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 29443 | a hand-carved yo-yo | 6 | 18.9 | 0.0 | damroll +3, hitroll +3; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |
| bard_instrument:flute | 1734 | a steel flute |
| bard_instrument:lyre | 33705 | a crystalline lyre |
| bard_instrument:mandolin | 1736 | an old mandolin |
| bard_instrument:harp | 1737 | an old brass harp |
| bard_instrument:drums | 1738 | a strange purple drum |
| bard_instrument:horn | 28971 | a finely curved horn |

### Thief

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Warlock

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 53661 | the shadowy staff of damnation | 5 | 27.2 | 0.0 | con_max +6, int_max +6; status: Sneak; max-stat focus: con_max, int_max |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### MindFlayer

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 53661 | the shadowy staff of damnation | 5 | 27.2 | 0.0 | con_max +6, int_max +6; status: Sneak; max-stat focus: con_max, int_max |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Alchemist

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Berserker

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 23064 | the ring of the berserker lord | 1 | 24.2 | 0.0 | dex -20, str_max +6; status: Fireshield, Prot Fire; max-stat focus: str_max; status: Fireshield, Prot Fire |
| `WEAR_FINGER_L` | 6846 | a thick and jagged onyx ring | 1 | 16.5 | 0.0 | damroll +5; offense: hit/damage |
| `WEAR_NECK_1` | 32628 | a slaadi necklace of Chaos | 7 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78511 | a set of spiked dragonscale arm plates | 2 | 16.5 | 0.0 | damroll +3, str_max +3; max-stat focus: str_max; offense: hit/damage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 41204 | the cloak of shadow dragons | 3 | 55.0 | 0.0 | hit +20, hitroll +3; status: Fly; status: Fly; offense: hit/damage |
| `WEAR_WAIST` | 87575 | a strip of studded black pudding | 5 | 32.2 | 0.0 | hit +10, str_max +6; max-stat focus: str_max |
| `WEAR_WRIST_R` | 91020 | a lightbringer bracelet of Pelor | 2 | 16.5 | 0.0 | damroll +3, str_max +3; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 91020 | a lightbringer bracelet of Pelor | 2 | 16.5 | 0.0 | damroll +3, str_max +3; max-stat focus: str_max; offense: hit/damage |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 4222 | an armageddon crystal | 2 | 15.9 | 0.0 | damroll +3, hitroll +2; offense: hit/damage |
| `WEAR_EARRING_L` | 78035 | an anti-matter earring | 2 | 16.9 | 0.0 | damroll +3, hitroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_QUIVER` | 55040 | a silken backsheath of flight | 22 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_2` | 78013 | a diamondine axe covered in blood | 6 | 37.8 | 0.0 | damroll +6, hitroll +6; offense: hit/damage |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Reaver

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 38763 | a ring of regeneration | 4 | 62.5 | 0.9 | hit +25, hitroll +5; offense: hit/damage |
| `WEAR_FINGER_L` | 38763 | a ring of regeneration | 4 | 62.5 | 0.9 | hit +25, hitroll +5; offense: hit/damage |
| `WEAR_NECK_1` | 58324 | a necklace of dangling bones and relics | 6 | 45.2 | 0.0 | agi_max +5, apply_move_reg +9, dex_max +6; status: Clarity, Major Mental; max-stat focus: agi_max, dex_max; status: Major Mental |
| `WEAR_NECK_2` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_BODY` | 58367 | an embroidered tunic of the Knights of the Raven | 5 | 23.8 | 0.0 | ac -125, con_max +10; status: Aware, Protect Evil; max-stat focus: con_max; status: Aware |
| `WEAR_HEAD` | 71254 | Yeenoghu's spiked helm of protection | 1 | 50.3 | 0.0 | hit +25, svpara -2; good saves: svpara |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 83486 | some black leather boots fit with adamantium heel blades | 4 | 18.7 | 0.0 | damroll +3, dex_max +4; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_HANDS` | 77723 | a set of glassteel gloves | 1 | 60.7 | 0.684 | damroll +4, hit +25; offense: hit/damage |
| `WEAR_ARMS` | 44173 | the sleeves of a MaDMaN | 1 | 56.3 | 0.156 | agi_max +4, hit +25; max-stat focus: agi_max |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 47092 | a cloak of crackling energy | 2 | 30.4 | 0.0 | agi +12, agi_max +4; max-stat focus: agi_max; stat focus: agi |
| `WEAR_WAIST` | 77749 | Jadem's magical device of protection | 4 | 39.6 | 3.0 | svpara -5, svspell -5; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Protect Evil, Protect Good, Stone Skin; good saves: svspell, svpara; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Stone Skin |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 87548 | the bracer of Uz's alliance | 6 | 16.5 | 0.0 | apply_combat_pulse -1, damroll +5; offense: hit/damage |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 59299 | a fang-bladed dagger named 'Raven's Claw' | 2 | 31.5 | 0.0 | damroll +5, hitroll +5; offense: hit/damage |
| `WEAR_EYES` | 78430 | a black duergar eyepatch | 5 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_FACE` | 91065 | the grim visage of a MaDWoMaN | 3 | 47.2 | 0.0 | hit +20, svfear -6; status: Sneak; good saves: svfear |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 58399 | a multi-phased fish bone earring | 3 | 31.5 | 0.0 | agi_max +4, damroll +3, dex_max +4; status: Prot Gas; max-stat focus: agi_max, dex_max; status: Prot Gas; offense: hit/damage |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 24016 | the bronze Zarbonesti seal of Kryz'Kyssik | 1 | 55.5 | 0.06 | apply_move +15, hit +15; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 89136 | a snuggly cuddlebunny | 3 | 20.4 | 0.0 | hit +8, svspell -3; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |

### Illusionist

Observed high-level characters: 8 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 20205 | a ring of the Tentabeast | 3 | 39.5 | 0.0 | hit +19, svspell -1; status: Waterbreath; good saves: svspell; status: Waterbreath |
| `WEAR_FINGER_L` | 40768 | the ring of celestial wonders | 4 | 59.0 | 0.48 | hit +24, int_max +2; status: Iceshield, Prot Acid; max-stat focus: int_max; status: Iceshield, Prot Acid |
| `WEAR_NECK_1` | 38765 | a scarf of enhanced stealth | 4 | 57.5 | 0.3 | damroll +3, dex_max +4, hit +20; status: Invisible; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 57576 | a prismatic halo of light | 2 | 8.2 | 0.0 | svfear -1, svspell -2; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_LEGS` | 66636 | some bright blue leg plates | 4 | 10.8 | 0.0 | con_max +3, svspell -3; max-stat focus: con_max; good saves: svspell |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 5817 | some razor-knuckled blood crystal gloves | 2 | 43.3 | 0.0 | damroll +4, hit +15; status: Detect Evil, Protect Evil; offense: hit/damage |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 5071 | a despair cloak of Vadatorn | 1 | 63.9 | 1.068 | hit +29, svspell -4; status: Detect Evil, Detect Good, Detect Magic, Protect Good; good saves: svspell |
| `WEAR_WAIST` | 77749 | Jadem's magical device of protection | 4 | 39.6 | 3.0 | svpara -5, svspell -5; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Protect Evil, Protect Good, Stone Skin; good saves: svspell, svpara; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Stone Skin |
| `WEAR_WRIST_R` | 78065 | a bracelet of influence | 8 | 57.0 | 0.24 | hit +20; status: Iceshield, Major Mental, Prot Acid, Prot Fire |
| `WEAR_WRIST_L` | 78467 | a finely woven crystal bracelet | 2 | 51.7 | 0.0 | hit +25, svpara -3; good saves: svpara |
| `PRIMARY_WEAPON` | 9447 | a staff of power | 3 | 75.8 | 2.496 | hit +30, hitroll +6; status: Detect Invisible; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 76656 | a wad of spawn goo | 11 | 40.6 | 0.0 | apply_move +20, svspell -1; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic; good saves: svspell |
| `WEAR_FACE` | 25405 | a white hot mask of living flame | 5 | 43.0 | 0.0 | con_max +4, hit +18; max-stat focus: con_max |
| `WEAR_EARRING_R` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 6710 | the codex of the muse of dreams | 9 | 18.0 | 0.0 | agi +5, int +5; stat focus: agi, int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |

### Blighter

Observed high-level characters: 1 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 21617 | a sun ring | 5 | 52.2 | 0.0 | hit +18, wis +10; stat focus: wis |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 18001 | the crown of dragons | 5 | 3.8 | 0.0 | apply_saving_breath -7, svspell -2; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_LEGS` | 87560 | the bloody skirts of power | 4 | 66.0 | 1.32 | apply_luck_max +20, pow_max +10; max-stat focus: apply_luck_max, pow_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 76714 | the gloves of the mind | 6 | 11.8 | 0.0 | apply_mana_reg +5, svspell -2; good saves: svspell |
| `WEAR_ARMS` | 94396 | some cracked bone arm bracers | 4 | 9.4 | 0.0 | con_max +3, svpara -2; max-stat focus: con_max; good saves: svpara |
| `WEAR_SHIELD` | 85721 | the shield proclaimed 'Hope' | 2 | 61.4 | 0.768 | apply_move +30, wis_max +3; status: Protect Evil; max-stat focus: wis_max |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 4031 | a thick belt of trollhide | 3 | 13.2 | 0.0 | con_max +3, str_max +3; max-stat focus: con_max, str_max |
| `WEAR_WRIST_R` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `WEAR_WRIST_L` | 81124 | a blackened bluestone bracer | 4 | 12.2 | 0.0 | svspell -4, wis_max +3; max-stat focus: wis_max; good saves: svspell |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 28913 | the bloody battle goggles | 3 | 34.0 | 0.0 | hit +14, wis_max +3; status: Detect Evil; max-stat focus: wis_max |
| `WEAR_FACE` | 6721 | a mask of autumn leaves | 3 | 2.0 | 0.0 | status: Infravision, Sense Life |
| `WEAR_EARRING_R` | 78423 | a tiny endurium earring | 6 | 14.2 | 0.0 | con +3, con_max +4; max-stat focus: con_max; stat focus: con |
| `WEAR_EARRING_L` | 6725 | a delicate enchanted snowflake | 6 | 35.4 | 0.0 | hitroll +4, int +8; status: Iceshield, Major Mental; stat focus: int; status: Iceshield, Major Mental; offense: hit/damage |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 6704 | a long elegant peacock feather | 4 | 12.3 | 0.0 | hit +5, svspell -2; good saves: svspell |
| `WEAR_BACK` | 53113 | a black leather backpack | 2 | 43.2 | 0.0 | apply_move +24; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 36891 | Ihsahn, the drow swashbuckler's legacy braid | 1 | 34.2 | 0.0 | apply_luck +10, int +9; stat focus: int |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Dreadlord

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Ethermancer

Observed high-level characters: 8 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 55414 | the ring of the Moon | 2 | 43.6 | 0.0 | hit +20, svpara -4; good saves: svpara |
| `WEAR_FINGER_L` | 11319 | a glowing black ring | 2 | 59.8 | 0.576 | hit +30, svspell -2; good saves: svspell |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 16244 | the helm of light | 3 | 35.1 | 0.0 | hit +15, wis_max +3; max-stat focus: wis_max |
| `WEAR_LEGS` | 20937 | a pair of bone leggings | 1 | 40.3 | 0.0 | apply_move +15, hit +7; observed high-level equipment usage |
| `WEAR_FEET` | 32624 | the boots of the dreamer | 3 | 55.2 | 0.024 | agi_max +7, hit +20; status: Aware, Levitate; max-stat focus: agi_max; status: Aware |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 83660 | some enchanted armplates of pure electrum | 2 | 53.1 | 0.0 | hit +25, svpara -4; good saves: svpara |
| `WEAR_SHIELD` | 85721 | the shield proclaimed 'Hope' | 2 | 61.4 | 0.768 | apply_move +30, wis_max +3; status: Protect Evil; max-stat focus: wis_max |
| `WEAR_ABOUT` | 36896 | a cloak of Erebus | 1 | 57.0 | 0.24 | hit +28, svspell -2; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_WAIST` | 83495 | a belt of dangling rat and bat skulls | 6 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `WEAR_WRIST_R` | 83440 | a spiked bracelet of dwarven kind | 2 | 59.0 | 0.48 | hit +20, str_max +5; status: Major Mental, Prot Fire; max-stat focus: str_max; status: Major Mental, Prot Fire |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 31306 | the spectacles of inner vision | 5 | 46.0 | 0.0 | apply_move +12, hit +12; status: Detect Evil, Detect Good; observed high-level equipment usage |
| `WEAR_FACE` | 25405 | a white hot mask of living flame | 5 | 43.0 | 0.0 | con_max +4, hit +18; max-stat focus: con_max |
| `WEAR_EARRING_R` | 83502 | a mithril stud carved like a fist | 6 | 29.4 | 0.0 | hit +12, str_max +3; max-stat focus: str_max |
| `WEAR_EARRING_L` | 83502 | a mithril stud carved like a fist | 6 | 29.4 | 0.0 | hit +12, str_max +3; max-stat focus: str_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 83296 | an emblem of Shanatar royalty | 6 | 13.2 | 0.0 | con_max +3, str_max +3; max-stat focus: con_max, str_max |
| `WEAR_BACK` | 83600 | a huge reed basket with straps | 2 | 36.0 | 0.0 | apply_move +20; observed high-level equipment usage |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83611 | a bladed adamantium mace | 2 | 13.2 | 0.0 | dex_max +3, str_max +3; max-stat focus: dex_max, str_max |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Avenger

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88320 | a pair of vampiric dragonscale gauntlets | 7 | 56.6 | 0.192 | apply_hit_reg +15, dex_max +3; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate; max-stat focus: dex_max; status: Iceshield, Major Mental, Prot Acid, Prot Fire, Regenerate |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

### Theurgist

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 87704 | a gory string of cyclops eyes | 12 | 35.8 | 1.5 | apply_move_reg +9, int_max +4, svfear -4; status: Detect Evil, Detect Good, Detect Invisible, Detect Magic, Farsee, Sense Life; max-stat focus: int_max; good saves: svfear; status: Farsee, Sense Life |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |

### Summoner

Observed high-level characters: 5 (observed)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 88317 | an ancient ring of liquid rock | 3 | 80.2 | 3.024 | hit +40, svpara -3; good saves: svpara |
| `WEAR_FINGER_L` | 88305 | an enchanted ring of balor bone | 3 | 57.3 | 0.276 | hit +25, int_max +4; status: Farsee; max-stat focus: int_max; status: Farsee |
| `WEAR_NECK_1` | 29459 | a choker with red, white and a blue gems | 1 | 21.6 | 0.0 | int +6, wis +6; stat focus: int, wis |
| `WEAR_NECK_2` | 77718 | an amulet of the Neogi Lords | 6 | 57.3 | 0.276 | hit +25, int_max +4; status: Ultravision; max-stat focus: int_max; status: Ultravision |
| `WEAR_BODY` | 31313 | a suit embodying ethereal soul shards | 6 | 47.5 | 0.0 | ac -25, hit +25; observed high-level equipment usage |
| `WEAR_HEAD` | 43013 | a golden crown | 2 | 83.0 | 3.36 | apply_move +25, hit +20; observed high-level equipment usage |
| `WEAR_LEGS` | 78020 | some mantis leggings | 1 | 55.7 | 0.084 | hit +25, svpara -3; status: Prot Acid; good saves: svpara; status: Prot Acid |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 131649 | Tempest, the swirling gauntlets of storms | 1 | 38.4 | 0.0 | con_max +5, svspell -4; status: Detect Invisible, Fly, Haste, Iceshield, Major Mental; max-stat focus: con_max; good saves: svspell; status: Fly, Haste, Iceshield, Major Mental |
| `WEAR_ARMS` | 38140 | some silken red sleeves | 1 | 28.5 | 0.0 | apply_saving_breath -2, hit +15; observed high-level equipment usage |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 9308 | a black silken cloak | 3 | 47.2 | 0.0 | damroll +2, hit +12; status: Iceshield, Major Mental, Prot Fire, Protect Good, Waterbreath; status: Iceshield, Major Mental, Prot Fire, Waterbreath; offense: hit/damage |
| `WEAR_WAIST` | 6738 | a silken sash of raw silk | 2 | 75.8 | 2.496 | apply_move +21, hit +20; observed high-level equipment usage |
| `WEAR_WRIST_R` | 131620 | a bracelet of rainbows | 2 | 57.3 | 0.276 | cha_max +6, hit +19; status: Aware, Farsee, Prot Fire; max-stat focus: cha_max; status: Aware, Farsee, Prot Fire |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 38776 | a dagger called 'Stealth' | 2 | 31.7 | 0.0 | damroll +3, hitroll +7; status: Sneak; offense: hit/damage |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 25705 | the blindfold of sight | 2 | 45.8 | 0.0 | apply_luck_max +10, cha_max +10; status: Detect Invisible, Sense Life; max-stat focus: apply_luck_max, cha_max; status: Sense Life |
| `WEAR_FACE` | 81419 | mask of the future | 2 | 73.1 | 2.172 | hit +35, svspell -4; status: Farsee; good saves: svspell; status: Farsee |
| `WEAR_EARRING_R` | 6725 | a delicate enchanted snowflake | 6 | 35.4 | 0.0 | hitroll +4, int +8; status: Iceshield, Major Mental; stat focus: int; status: Iceshield, Major Mental; offense: hit/damage |
| `WEAR_EARRING_L` | 85726 | an earring of bone | 5 | 55.1 | 0.012 | apply_move +10, hit +19; status: Infravision |
| `WEAR_QUIVER` | 25757 | the quiver of holding | 4 | 20.0 | 0.0 | agi_max +5, hitroll +3; max-stat focus: agi_max; offense: hit/damage |
| `GUILD_INSIGNIA` | 31312 | a necromantic death shroud | 9 | 28.3 | 0.0 | apply_move +10, hit +5; status: Absorb; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 33705 | a crystalline lyre | 2 | 13.2 | 0.0 | apply_luck_max +3, cha_max +3; max-stat focus: apply_luck_max, cha_max |
| `WEAR_ATTACH_BELT_3` | 88906 | a weathered cast net | 1 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |

Support items:

| Role | VNUM | Item |
|---|---:|---|
| spellbook | 83336 | a hide-bound spellbook with a glowing Alatorin insignia |

### Dragoon

Observed high-level characters: 0 (role/static fallback)

| Runtime slot | VNUM | Item | Observed players | Power | Risk | Effects / selection reason |
|---|---:|---|---:|---:|---:|---|
| `WEAR_FINGER_R` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_FINGER_L` | 9460 | the ring of the legendary hunter | 13 | 6.8 | 0.0 | svfear -1, svspell -1; status: Haste; good saves: svspell, svfear; status: Haste |
| `WEAR_NECK_1` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_NECK_2` | 94726 | a spectral holy symbol of Berronar Truesilver | 10 | 13.2 | 0.0 | con_max +3, wis_max +3; max-stat focus: con_max, wis_max |
| `WEAR_BODY` | 43123 | a suit of elven chainmail | 6 | 63.6 | 1.032 | hit +30, svspell -3; status: Detect Evil, Detect Good, Detect Magic; good saves: svspell |
| `WEAR_HEAD` | 6222 | the diamond crown of Winduin | 7 | 20.9 | 0.0 | damroll +3, dex_max +5; max-stat focus: dex_max; offense: hit/damage |
| `WEAR_LEGS` | 58365 | some stitched leggings of the Knights of the Raven | 4 | 34.7 | 0.0 | ac -50, con_max +8, hit +9; max-stat focus: con_max |
| `WEAR_FEET` | 70814 | boots of endurance | 6 | 68.4 | 1.608 | apply_move +37, apply_move_reg +1; observed high-level equipment usage |
| `WEAR_HANDS` | 88919 | some snake-rattle gloves | 2 | 47.5 | 0.0 | apply_move +10, hit +15; status: Aware |
| `WEAR_ARMS` | 78415 | some blue dragonscale shoulder guards | 12 | 12.8 | 0.0 | apply_saving_breath -6, int_max +4; status: Major Mental; max-stat focus: int_max; status: Major Mental |
| `WEAR_SHIELD` | 38761 | the shield of the earthwyrm | 6 | 50.5 | 0.0 | con_max +3, damroll +2, hit +15; status: Detect Invisible, Fly; max-stat focus: con_max; status: Fly; offense: hit/damage |
| `WEAR_ABOUT` | 40747 | a griffon feather cloak | 8 | 17.8 | 0.0 | apply_saving_breath -5, svfear -7; status: Fly; good saves: svfear; status: Fly |
| `WEAR_WAIST` | 9438 | a belt of skulls | 18 | 13.2 | 0.0 | con_max +3, pow_max +3; max-stat focus: con_max, pow_max |
| `WEAR_WRIST_R` | 28911 | a bracelet of dracolich hide | 12 | 19.5 | 0.0 | damroll +3, str_max +4; status: Invisible; max-stat focus: str_max; offense: hit/damage |
| `WEAR_WRIST_L` | 6726 | a bracelet of woven willow leaves | 22 | 17.8 | 0.0 | apply_move_reg +5, con +4; status: Protect Evil, Protect Good; stat focus: con |
| `PRIMARY_WEAPON` | 44188 | the mace of mentality | 11 | 51.8 | 0.0 | apply_hit_reg +14, pow_max +8; status: Iceshield, Major Mental; max-stat focus: pow_max; status: Iceshield, Major Mental |
| `SECONDARY_WEAPON` | 87583 | the otherworldly dagger of Lokpan | 2 | 31.1 | 0.0 | damroll +5, dex_max +3; status: Haste, Prot Gas; max-stat focus: dex_max; status: Haste, Prot Gas; offense: hit/damage |
| `WEAR_EYES` | 29437 | goggles of the tinkerer | 7 | 31.3 | 0.0 | apply_luck_max +3, hit +13; max-stat focus: apply_luck_max |
| `WEAR_FACE` | 32816 | mask of the flayed mind | 5 | 52.7 | 0.0 | hit +25, svfear -3; status: Ultravision; good saves: svfear; status: Ultravision |
| `WEAR_EARRING_R` | 24402 | a lightning earring | 8 | 7.6 | 0.0 | apply_saving_breath -5, damroll +2; status: Farsee; status: Farsee; offense: hit/damage |
| `WEAR_EARRING_L` | 67104 | a glowing jade earring | 8 | 55.7 | 0.084 | hit +25, int_max +3; status: Protect Evil, Protect Good; max-stat focus: int_max |
| `WEAR_QUIVER` | 55424 | the ancient sheath of *-* Clan BloodLust *-* | 24 | 9.6 | 0.0 | apply_move_reg +3, svspell -3; good saves: svspell |
| `GUILD_INSIGNIA` | 31315 | a swirling force of light and darkness | 20 | 0.0 | 0.0 | no named numeric/status effect in snapshot; observed high-level equipment usage |
| `WEAR_BACK` | 29401 | a big big bag for a little person | 8 | 9.0 | 0.0 | str +5; stat focus: str |
| `WEAR_ATTACH_BELT_1` | 38664 | a glowing white pearl | 5 | 54.5 | 0.0 | hit +25, svspell -5; good saves: svspell |
| `WEAR_ATTACH_BELT_2` | 83336 | a hide-bound spellbook with a glowing Alatorin insignia | 7 | 5.6 | 0.0 | svspell -4; good saves: svspell |
| `WEAR_ATTACH_BELT_3` | 29404 | lucky alchemist sack | 7 | 29.0 | 0.0 | apply_luck +10, apply_luck_max +5; max-stat focus: apply_luck_max |

## Implementation and verification notes

### Runtime changes

- `src/account/nanny.c`: replaced the old placeholder-heavy Chaos tables and per-item grant chain with generated class/profile data, optional body-slot variants, bounded consumables, runtime slot checks, class/race checks, fail-closed missing/nesting/queue validation, runtime skipping of unusable items, one nested root-bag grant, and pre-entry scheduling after the accepted-rules baseline.
- `scripts/chaos_eq_analyze.py`: parses fundamental object types from `defines.h`, requires the complete Shaman sphere mask, and preserves heterogeneous fundamental CSV columns.
- `scripts/chaos_eq_catalog.py`: derives all-class eligibility checks from the analyzed class-ID map rather than a fixed class count.
- `scripts/chaos_eq_validate.py`: rejects selected ambiguous duplicate prototypes and verifies the master spellbook belt flag from the active object definition.
- `src/item/item_movement_transaction.c` / `.h`: added an explicit direct-to-self pre-entry grant mode that does not block commands and announces completion after entry, including completions retained until `player_ready()`.
- `src/account/chaos_eq_data.h`: generated standard/enhanceable arrays for all 30 classes, optional variations, and shared consumables.
- `src/combat/chaos_config.c` / `.h`: added `CHAOS_EQ_PROFILE=standard|enhanceable`; invalid values fail closed to standard.
- `areas/obj/limbo.obj`: added belt attachment to master spellbook VNUM 7 while preserving take/hold.
- `tests/async/test_chaos_new_character_kit.py`: validates all class arrays, active VNUMs, policy exclusions, class/wear compatibility, fundamentals, optional arrays, and one-root submission.
- `tests/async/test_chaos_preentry_grant.py`: validates pre-entry scheduling, non-blocking command semantics, PID validation exception boundaries, and post-entry announcement behavior.
- `tests/async/test_flatfile_chaos_new_character_kit.py`: exercises real account/character creation, pre-entry scheduling, post-entry readiness messaging, bag contents, consumable visibility, save, and clean shutdown using the generated Warrior profile.

### Reproducible commands

```text
python3 scripts/chaos_eq_analyze.py --repo-root . --docker-container chaos-eq-analysis-mariadb --database duris_prod --level-threshold 50 --area-root areas/obj --area-list areas/AREA --enhance-config lib/enhance.cfg --output-dir <analysis-dir>
python3 scripts/chaos_eq_catalog.py --analysis <analysis-dir>/analysis.json --output-dir <catalog-dir> --header-out <catalog-dir>/chaos_eq_data.h --repo-root .
python3 scripts/chaos_eq_validate.py --catalog <catalog-dir>/catalog.json --repo-root .
python3 tests/async/test_chaos_new_character_kit.py
python3 tests/async/test_chaos_coderabbit_regressions.py
python3 tests/async/test_chaos_preentry_grant.py
python3 tests/async/test_master_spellbook.py
python3 tests/async/test_chaos_env_toggle.py
python3 tests/async/test_flatfile_chaos_new_character_kit.py
make world
make -C src -j2
```

The generated results for this reference are:
- Analyzer dataset: 2733 candidate templates and 131-character cohort.
- Catalog: 30 standard and 30 enhanceable class profiles.
- Runtime/build/test execution is recorded by the repository CI and PR evidence rather than embedded as a stale one-run claim in this generated report.

## Known limitations and follow-up

- The July archive predates some current persistence columns; analysis deliberately uses the compatible direct player/wiki tables rather than current player-load SQL unchanged.
- SQL wiki metadata does not contain every raw runtime field (notably all bitvector provenance and some loader-derived fields), so active area prototypes and the strict runtime loader checks remain authoritative.
- Some status names are display aliases from the current wiki/effect table; unresolved aliases are retained as names rather than guessed into a spell flag.
- The generated lists are balanced against the available snapshot, not against a live Chaos population. Review actual PvP, caster burst, mobility, save stacking, and consumable depletion after deployment to a test environment.
- Standard fundamentals intentionally bypass ordinary enhance/economy filters only where explicitly named. Do not add new exceptions by placing another item in a class array; update the analyzer policy and validator together.
