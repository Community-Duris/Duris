# Casting pulse and concentration policy

Spell landing time starts with `SpellCastTime()`: the spell's base beats are
multiplied by the racial, global, equipment, haste, and flurry modifiers. The
casting-pulse policy then applies class/mastery acceleration, quick chant, and
the maximum-circle concentration roll.

The cast's own command wait is scheduled from the final landing duration. This
means successful quick chant reduces both the spell timer and the casting gate;
the gate no longer outlives the chant. `AFF2_CASTING` continues to hold typed
commands until completion. The ordinary four-pulse post-cast recovery still
starts when the spell resolves, and an independently imposed longer wait, such
as a combat stun, still wins and is never shortened by casting.

## Configuration

All settings are numeric entries in `lib/duris.properties` and take effect for
new casts after the normal `properties reload` command. Non-finite values use
the shipped fallback, and finite values are clamped to the effective range.

| Key | Shipped value | Effective range | Meaning |
| --- | ---: | ---: | --- |
| `spellcast.quickChant.durationMultiplier` | 0.5 | 0.1–1.0 | Multiplier applied after a successful quick chant. The result has a one-beat floor. |
| `spellcast.quickChant.tankSuccessPercent` | 75 | 0–100 | Chance that being actively tanked permits a quick-chant skill attempt. Untanked casters skip this roll. |
| `spellcast.quickChant.skillBasePercent` | 0 | -100–100 | Base percentage in the quick-chant skill curve. |
| `spellcast.quickChant.skillPercentPerPoint` | 1 | 0–5 | Percentage points added per bounded skill point. The final chance is clamped to 0–100. |
| `spellcast.maxCircleAbort.basePercent` | 50 | 0–100 | Concentration-abort chance before agility reduction and the cap. |
| `spellcast.maxCircleAbort.agilityReductionPerPoint` | 0.5 | 0–5 | Percentage points removed from the abort chance per agility point. |
| `spellcast.maxCircleAbort.capPercent` | 5 | 0–100 | Final ceiling for maximum-circle aborts. Set to zero to disable the roll. |

Percentages use 0.01-percentage-point resolution. A quick-chant-enabled player
receives an explicit message when either the tank gate or skill curve fails.
Automatic Druid and Blighter acceleration does not produce a failure message.
At the shipped curve, skill 100 succeeds on every permitted attempt; it no
longer retains the old one-percent miss.

The abort formula is:

`min(capPercent, clamp(basePercent - agility * agilityReductionPerPoint, 0, 100))`

The five-percent shipped cap reduces the former roughly twenty-percent abort
rate at 60 agility while retaining an agility benefit. Raising the cap to 100,
with base 50 and reduction 0.5, restores the old formula's approximate range;
the new roll uses an exact 1–10000 percentage boundary instead of the old
inclusive 0–100 comparison.

## Rollback

Set `spellcast.quickChant.durationMultiplier=1` to disable quick-chant timing
acceleration, set `spellcast.quickChant.tankSuccessPercent=50` for the former
tank gate, or set `spellcast.maxCircleAbort.capPercent=0` to disable
maximum-circle concentration aborts. Property changes do not rewrite an
already scheduled cast; they apply to the next cast.
