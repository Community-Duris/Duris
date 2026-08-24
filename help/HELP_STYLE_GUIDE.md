# DurisMUD Help Entry Style Guide

Based on analysis of existing help entries in duris_help.hlp

---

## Entry Structure

Every spell help entry follows this format:

```
[Spell Name] - Last Edited: [DATE] by [AUTHOR]
[======= matching length =======]
* Area of effect: <target>
* Aggressive:     Yes/No
* Cumulative:     Yes/No
* Duration:       [duration text]
* Type of spell:  [type]

[Description paragraph(s)]

==See also==
[related topics]
```

---

## Header Fields

### Area of Effect
Use these exact formats:
- `<victim>` - Single target spell
- `<victim> | <victim> <direction>` - Can be cast at target or into adjacent room
- `<room>` - Affects everyone in the room
- `<direction>` - Creates effect in a direction (walls, etc.)
- `<group>` - Affects caster's group
- `<self>` - Self-only spell
- `Caster's group` - Alternative for group spells

### Aggressive
- `Yes` - Spell causes damage OR debuffs the target
- `No` - Buff, heal, utility, or summon spell
- `N/A` - Rare, for spells that don't fit either category

### Cumulative
- `Yes` - Effect stacks or has multiple waves (DoTs)
- `No` - Single application, doesn't stack

### Duration
Use these phrases (from existing entries):
- `Instantaneous` - Immediate effect, no lingering duration
- `Short` - Brief duration (1-4 ticks)
- `Dependent on level of caster` - Standard scaling duration
- `Up to 3 hours, dependent on level of caster` - For long buffs
- `2 to 5 waves` - For DoT spells
- `Dependent on level of caster or until dispelled` - For removable buffs

### Type of Spell
Common types from existing entries:
- `Fire` / `Cold` / `Electric` / `Acid` - Elemental damage
- `Negative Energy` - Necromantic/unholy damage
- `Holy` - Divine damage
- `Healing` - Restoration spells
- `Protection` - Defensive buffs
- `Enhancement` - Stat buffs
- `Enchantment` - Mind-affecting spells
- `Summoning` - Creature summoning
- `Teleportation` - Movement spells
- `Animal` - Druid/nature spells
- `Generic` - Catch-all for others

---

## Description Writing Style

### Tone
- **Thematic and evocative** - Write like you're describing magic, not code
- **Second person avoided** - Don't say "you cast" or "your target"
- **In-world perspective** - Describe what happens, not game mechanics
- **Concise** - Usually 1-3 sentences, rarely more than a paragraph

### Opening the Description
Good patterns from existing entries:

**Quote the spell name first:**
> "Fireball" is a very potent spell that causes a small ball of flame to shoot forth from the caster's fingertips.

> "Haste" grants the victim a faster pulse speed, allowing them to hit more often than without it.

> "Heal" restores a great deal of hit points, and also cures blindness (if any), but cannot heal completely.

**Or start with "This spell...":**
> This spell will change the skin of the caster's group to a stone-like consistency...

> This potent spell bathes all living things within the vision of the caster in pure mental confusion.

**Or describe the action directly:**
> A conflagration of fire is sent towards the victim, burning their flesh for several rounds.

### Word Choice

**USE these terms:**
- `victim` - The target of a spell (even for buffs)
- `caster` - The person casting the spell
- `recipient` - Alternative for buff targets
- `hit points` - For health/HP (also acceptable: hitpoints, HP)
- `level` - For caster/character level

**AVOID these terms:**
- `target` - Use "victim" instead
- `ticks` - Use "rounds" or time descriptions
- Code-like terms - "modifier", "bitvector", "affect"

### Describing Effects

**Damage spells:**
> ...striking the victim for considerable damage.
> ...burning the victim quite profoundly.
> ...causes a huge bolt of lightning to streak from the caster's outstretched palm.

**DoT spells:**
> ...burning their flesh for several rounds. The burning hot flames scorch and sear the unfortunate creature caught within its grasp. Although the fire burns hotly at first, the burning damage slowly dissipates over time.

**Buff spells:**
> ...grants the victim a faster pulse speed, allowing them to hit more often.
> ...provides protection from magical fire attacks and some protection against physical attacks.

**Debuff spells:**
> ...will reduce the mental faculties of the victim when successful.
> ...causes all exposed to it to react in pure mental confusion.

**Healing spells:**
> ...restores a great deal of hit points...
> ...cures blindness (if any)...

**Protection spells:**
> ...negates almost all damage from physical attacks.
> ...absorb magical energies directed at anyone protected by them.

### Describing Mechanics (When Needed)

If mechanical details matter, weave them into the prose:

> It will last until either the duration expires or a variable number of attacks (of any type, whether they hit or do damage) are made against the victim.

> The spell will cause the victim to forget some of the spells they have memorized among other detrimental effects to the mind. Wisdom is an important factor when using or saving against this spell.

> Because of the spell's detrimental effects, the victim will save against it. Thus, if the victim wants to be the recipient, he should consent the caster.

---

## See Also Section

### Format
```
==See also==
[item], [item], [item]
```

or with bullets:

```
==See also==
* [item]
* [item]
```

### What to Include
- Related spells (group version, similar effect)
- Classes that learn the spell
- Counter-spells or complementary spells
- Related help topics

### Examples
> ==See also==
> Chain Lightning, druid, sorcerer, conjurer

> ==See also==
> * Stone Skin
> * biofeedback

> See also: cureblind, fullheal, blindness

---

## Complete Examples

### Damage Spell (Fireball)
```
* Area of effect: <victim> | <victim> <direction>
* Aggressive:     Yes
* Cumulative:     No
* Duration:       Instantaneous
* Type of spell:  Fire

"Fireball" is a very potent spell that causes a small ball of flame to
shoot forth from the caster's fingertips. Upon contact with the victim, it
explodes in a huge blast, burning the victim quite profoundly.
```

### DoT Spell (Immolate)
```
* Area of effect: <victim>
* Aggressive:     Yes
* Cumulative:     Yes
* Duration:       2 to 5 waves
* Type of Spell:  Fire

A conflagration of fire is sent towards the victim, burning their flesh
for several rounds. The burning hot flames scorch and sear the unfortunate
creature caught within its grasp. Although the fire burns hotly at first,
the burning damage slowly dissipates over time.

==See also==
* Immolure
```

### Buff Spell (Haste)
```
* Area of effect: <victim>
* Aggressive:     No
* Cumulative:     No
* Duration:       Dependent on level of caster
* Type of spell:  Generic

"Haste" grants the victim a faster pulse speed, allowing them to hit more
often than without it. It also adds an archery hit, allowing an archer to
hit more often. When used on casters, it allows them to cast their spell
more quickly.

==See also==
slowness, haste-eq, sorcerer, conjurer, bard
```

### Protection Spell (Group Stone Skin)
```
* Area of effect: <group>
* Aggressive:     No
* Cumulative:     No
* Duration:       Dependent on level of caster
* Type of spell:  Protection

This spell will change the skin of the caster's group to a stone-like
consistency and negates almost all damage from physical attacks. It will
last until either the duration expires or a variable number of attacks
(of any type, whether they hit or do damage) are made against the victim.
The number of attacks it protects against varies with the level of the
caster. If a particularly damaging attack is made, it will provide no
protection at all. Group Stone Skin does not block any damage from magical
spells (though these attacks will "chip away" at the protection).

==See also==
* Stone Skin
* biofeedback
```

### Debuff Spell (Feeblemind)
```
* Area of effect: <victim>
* Aggressive:     Yes
* Cumulative:     No
* Duration:       Short
* Type of spell:  Enchantment

Feeblemind will reduce the mental faculties of the victim when successful.
The spell will cause the victim to forget some of the spells they have
memorized among other detrimental effects to the mind. Wisdom is an
important factor when using or saving against this spell.
```

### Healing Spell (Heal)
```
* Area of effect: <victim>
* Aggressive:     No
* Cumulative:     Yes
* Duration:       Instantaneous
* Type of spell:  Healing

"Heal" restores a great deal of hit points, and also cures blindness
(if any), but cannot heal completely.

See also: cureblind, fullheal, blindness
```

### Area Spell (Earthquake)
```
* Area of effect: <room>
* Aggressive:     Yes
* Cumulative:     No
* Duration:       Instantaneous
* Type of spell:  Generic

"Earthquake" causes the ground to tremble and quake, making everyone
and everything in the room fall to the ground and take damage, unless they
are grouped with the caster.
```

---

## Common Mistakes to Avoid

1. **Generic placeholder text** - Never use "is a spell available to certain classes"
2. **Redundant damage type** - Don't say "negative energy energy"
3. **Code-like language** - Avoid "scales with level" or "modifier"
4. **Missing aggressive flag** - Debuffs ARE aggressive (Yes)
5. **Wrong duration for DoTs** - Use "2 to 5 waves" not "Dependent on..."
6. **Typos** - Watch for "increasess" or "Dependant" vs "Dependent"
7. **Too technical** - Don't expose internal mechanics unless relevant
8. **Too verbose** - Keep it concise, 1-3 sentences usually suffices

---

## Writing Process

1. **Identify the spell type** - Damage? Buff? Heal? Utility?
2. **Set the header fields** correctly based on type
3. **Write the opening** - Quote name or "This spell..."
4. **Describe the effect** - What does it look/feel like?
5. **Add mechanical details** only if players need to know them
6. **List related topics** in See Also

---

## Spell Type Quick Reference

| Spell Type | Aggressive | Cumulative | Duration | Example |
|------------|------------|------------|----------|---------|
| Direct Damage | Yes | No | Instantaneous | Fireball |
| DoT | Yes | Yes | 2 to 5 waves | Immolate |
| Area Damage | Yes | No | Instantaneous | Earthquake |
| Debuff | Yes | No | Short/Dependent | Feeblemind |
| Buff | No | No | Dependent | Haste |
| Group Buff | No | No | Dependent | Group Haste |
| Heal | No | Yes | Instantaneous | Heal |
| Protection | No | No | Dependent | Stone Skin |
| Summon | No | No | Dependent | Conjure Elemental |
| Teleport | No | No | Instantaneous | Word of Recall |
| Wall | No | No | Dependent | Wall of Fire |
| Utility | No | No | Varies | Detect Invisibility |
