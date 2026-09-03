# Chaos mode

Chaos mode is an optional server-wide ruleset selected at process start. The
runtime implementation in `src/combat/chaos.c` and configuration accessors in
`src/combat/chaos_config.c` are authoritative. With `CHAOS_MUD` disabled, the
ordinary character-creation, equipment, material, and ship-reward paths remain
unchanged.

## Configuration

All Chaos switches are case-sensitive. `CHAOS_MUD` is enabled only by the exact
value `TRUE`; an unset value or `FALSE` disables it, and an invalid value warns
and fails closed. See [CONFIGURATION.md](../operations/CONFIGURATION.md#character-creation-and-gameplay-modes)
for the complete environment-variable table.

`CHAOS_EQ_PROFILE` selects one of two generated new-character equipment sets:

- `standard` is the default high-end profile.
- `enhanceable` restricts equipment and class fundamentals to items accepted by
  the boot-time enhancement index.

An unset or invalid profile uses `standard`. Starter bonuses are controlled by
`CHAOS_STARTER_BONUSES` and independent feature switches for the frigate, epic
skills, epic points, bank platinum, and material pouch. Each feature defaults
to enabled when unset, but every feature still requires both `CHAOS_MUD` and
the master starter-bonus switch. An invalid starter value disables that grant.

`CHAOS_TEST_COMMANDS=TRUE` exposes the bounded Chaos integration helpers only
when `ENVIRONMENT=local`. It does not enable Chaos mode by itself.

## New-character grants

Chaos characters are rebuilt at mortal level 56 on entry. A newly created
character can also receive:

- one starter bag containing the selected class profile, applicable optional
  body-slot items, support consumables, and the material pouch when enabled;
- every eligible no-specialization epic skill;
- a free-frigate claim, using the persisted `AIP_FREESLOOP` effect as the
  compatibility marker for the existing dock reward flow;
- 20,000 epic points through the critical epic ledger; and
- 1,000,000 bank platinum through the critical currency ledger.

The class equipment bag is part of Chaos mode itself; the
`CHAOS_STARTER_BONUSES` master does not disable it. The material pouch inside
the bag and the other four optional rewards do require the master switch.

The equipment grant is a single nested durable root. Character creation first
establishes the player's persistence baseline, builds and validates the entire
bag, then submits one pre-entry creation grant. Missing required prototypes,
invalid nesting, or a failed durable submission withholds the whole kit rather
than publishing a partial one. Items for body slots the character does not have
and items the character cannot use are skipped by the documented runtime
filters. Approval mode defers the equipment grant until the approval-success
transition. The prepared message is emitted only after durable completion and
game entry.

Epic and bank rewards use stable operation identities and pending player flags,
so retry follows the critical-command result instead of applying an anonymous
balance mutation. Completion clears the matching pending flag and marks player
status dirty. Existing characters are not retroactively given the one-time
equipment bag or material pouch.

Epic-skill selection reuses the normal `epic_rewards` and `epic_teachers`
tables for an unspecialized character. It preserves class masks, the
Thri-Kreen exception, teacher existence, deny-skill mutual exclusions in table
order, prerequisite skill levels, and teacher-defined maxima. These starter
unlocks do not spend epic points. In non-Chaos mode, the persisted
`AIP_FREESLOOP` reward retains its ordinary free-sloop behavior.

## Equipment catalog

`src/account/chaos_eq_data.h` is generated, not hand-maintained. It contains a
standard and enhanceable profile for each of the 30 classes, shared optional
body-slot entries, and shared consumables. The catalog pipeline separates the
durable selection policy from dated observations about a particular character
population:

1. `scripts/chaos_eq_analyze.py` analyzes aggregate high-level equipment data
   and reconciles it with active area prototypes and `lib/enhance.cfg`.
2. `scripts/chaos_eq_catalog.py` applies class, race, wear-slot, risk, and
   profile rules and can emit the runtime header.
3. `scripts/chaos_eq_validate.py` validates both profiles against current area
   data and enhancement rules.
4. `scripts/chaos_eq_report.py` can render a human review artifact for the
   generated analysis; that report is evidence for a generation run, not a
   maintained runtime contract.

Normal-profile equipment excludes artifacts, Ioun-slot items, `unique`
keywords, item-level class/race restrictions, non-portable items,
transient/no-rent/no-show/no-sell flags, quest items, and placeholder VNUM
1252. The named class fundamentals (the standard master spellbook and Bard
instruments) are the only explicit standard-profile policy exceptions. The
enhanceable profile additionally requires the current boot-time enhancement
predicate, including takeability, allowed effect masks, economics, item type,
and configured pool exclusions. Generator risk is capped at 4.0; candidates
are aggregate per-slot selections with validated portable fallbacks, never a
copy of one character's equipment. Support consumables use bounded starter
quantities and exclude charged staves and wands.

Run the analyzer only against an explicitly authorized development database or
restored non-production clone. Never use production to discover or tune the
catalog. Review generator-policy changes and the emitted header together; do
not add an exception by editing a generated class array directly.

## Craft pouch

The Chaos craft pouch is a real persisted item but a virtual, non-consuming
source of supported materials. The runtime finds it when directly carried,
nested below a carried container, or attached to belt slots 1 through 3. The
catalog covers salvage-material VNUMs 400000 through 400209 and encrust jewels
400291 through 400299.

`put <material> pouch`, `put all pouch`, and `put all.<keyword> pouch` transfer
supported physical materials into a durable collection operation. On success
the physical item is removed and the pouch's collected count advances. Craft,
forge, enhancement, and encrust paths may then satisfy eligible raw-material
requirements from the pouch without consuming it. Tools, recipes, skills,
levels, fees, output ownership, and every other command-specific check still
apply.

The pouch records generated and collected totals in its persisted item ledger.
`look in pouch` and `examine pouch` display non-zero entries. A failed ownership
operation retains the material and rolls back the provisional score when
possible; an indeterminate double failure is surfaced to the player and staff.
The pouch cannot itself be salvaged, enhanced, or encrusted. When it is absent
or its feature switch is disabled, ordinary inventory-material behavior is
unchanged.

The player-facing command contract lives in `lib/information/helpchaospouch`.
Implementation details are in `src/combat/chaos_materials.c` and
`src/combat/chaos_materials.h`.

## Focused verification

```text
python3 tests/async/test_chaos_env_toggle.py
python3 tests/async/test_chaos_eq_profile.py
python3 tests/async/test_chaos_new_character_kit.py
python3 tests/async/test_chaos_preentry_grant.py
python3 tests/async/test_chaos_infinite_starting_grants.py
python3 tests/async/test_chaos_pouch_help.py
python3 tests/async/test_flatfile_chaos_new_character_kit.py
```
