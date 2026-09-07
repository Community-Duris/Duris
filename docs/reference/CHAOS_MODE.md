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

- the selected class profile and applicable optional body-slot items directly
  in inventory, ready for `wear` or `wear all`;
- one starter bag containing support consumables, eligible utility tools, and
  the material pouch when enabled;
- every eligible no-specialization epic skill;
- a free-frigate claim, using the persisted `AIP_FREESLOOP` effect as the
  compatibility marker for the existing dock reward flow;
- 20,000 epic points through the critical epic ledger; and
- 1,000,000 bank platinum through the critical currency ledger.

The class equipment bag is part of Chaos mode itself; the
`CHAOS_STARTER_BONUSES` master does not disable it. The material pouch inside
the bag and the other four optional rewards do require the master switch.

Character creation first establishes the player's persistence baseline and
prepares every object before admitting the complete set of detached roots to
the existing pre-entry ownership coordinator. Wearables are individual roots;
the support bag is one root with nested contents. Missing required prototypes,
invalid wear flags, unusable items, or failed batch admission withhold the kit.
Unavailable race/body slots and ineligible weapon slots are omitted. Approval
mode defers the grant until approval succeeds.

Once admitted, roots commit sequentially through the existing ownership path.
Commands and the game prompt wait for completion, so `wear all` cannot race a
partially published kit. The CHAOS prepared message follows completion. A failed
ownership operation stops the remaining grants and reports failure; any earlier
committed roots remain durable. This is atomic queue admission, not a new
kit-wide database transaction or durable retry receipt. Existing ownership and
save/relog topology remain authoritative. Orderly copyover and shutdown cancel
before quiescing while an accepted starter batch is pending; retry the operation
after completion. An abrupt process loss still has the per-root recovery limits
of the existing ownership framework. Disconnecting before game entry cancels
unsubmitted kit roots, retains any already-journaled head for existing durable
recovery, and releases the maintenance fence without a success announcement. A
failed or interrupted partial grant may require staff recovery; it is not
automatically duplicated on login.

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
body-slot entries, shared consumables, and skill-gated utilities. The catalog pipeline separates the
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

The physical starter roles are **Warrior, Ranger, Paladin, Anti-Paladin, Monk,
Rogue, Assassin, Mercenary, Bard, Thief, Berserker, Reaver, Dreadlord, Avenger,
and Dragoon**. These include melee hybrids deliberately. Both profiles reject
positive max-WIS equipment for these roles and use physical-stat scores when
selecting replacements. Shared optional body items follow the same max-WIS
rule. Monk profiles contain neither weapon slots nor weapon-bearing items;
runtime preparation repeats that check for all profile, optional, and support
entries. Generation fails if an applicable core slot has no valid candidate.

Every physical role receives permanent Globe of Invulnerability on its first
neck item (`WEAR_NECK_1`, available to every player race). The generated policy
adds the canonical object `bitvector2` flag `AFF2_GLOBE` to that starter instance.
Active AREA prototypes currently have no such globe-bearing item, so this is an
explicit starter balance rule, not a claimed existing prototype effect. Wearing
the item grants the effect through normal equipment handling; item snapshots
preserve it across save/relog. Area prototypes are unchanged.

Starter instances, including their bag and durable tools, remove
`ITEM_TRANSIENT`, `ITEM_NODROP`, `ITEM_INVISIBLE`, `ITEM_SECRET`, `ITEM_NOSHOW`,
`ITEM_BURIED`, `ITEM_NORENT`, `ITEM2_CRUMBLELOOT`, and every `APPLY_CURSE` affect.
The generator records and validates this policy, and runtime preparation
reapplies it to current object data. Intended magic, role, and wearer-affect
bits remain. Consumables retain their normal type/procedure consumption; an
armed trap can still become secret and expire through the normal trap command.

Normal-profile selection still excludes artifacts, Ioun-slot items, `unique`
keywords, non-portable and class-restricted candidates, forbidden source flags,
quest items, and placeholder VNUM 1252. Named class fundamentals retain their
explicit selection exceptions. Enhanceable selections still pass the boot-time
enhancement predicate; risk remains capped at 4.0. Runtime checks current wear
compatibility and class/race usability before ownership submission.

Utility grants use the character's class/race/specialization skill availability
at level 56, because learned skill values are initialized only on game entry:

| Required skill | Bag grant | Count |
| --- | --- | ---: |
| Fishing | Fishing pole, VNUM 336 | 1 |
| Pick Lock | Lockpicks, VNUM 412 | 1 |
| Trap | Huntsman traps, VNUM 73 | 3 |
| Salvage | Scientific tools, configured VNUM (default 400227) | 3 |

Each utility group is added once; existing kit entries with its resolved VNUM
prevent duplicate grants. Fishing needs no bait and lockpicks are their own
tool. Traps and scientific tools are consumable supplies. The existing optional
craft pouch supplies supported recipe materials. The active Craft/Forge paths
do not require the hammer/parchment tools from the disabled legacy path. A
configured scientific-tool prototype must exist and pass runtime validation.

A sanitized seed records the prior catalog's VNUM/slot choices and its source
commit, without player records or population statistics. Regenerate from this
seed plus current active AREA/enhancement data without database access:

```sh
python3 scripts/chaos_eq_catalog.py --static-seed docs/data/chaos_eq_seed.json \
  --repo-root . --output-dir bin/chaos-catalog \
  --header-out src/account/chaos_eq_data.h \
  --policy-report-out docs/reference/CHAOS_KIT_CATALOG.md
python3 scripts/chaos_eq_validate.py --catalog bin/chaos-catalog/catalog.json --repo-root .
```

The seed is a selection baseline, not new observed-player evidence. The
aggregate analyzer remains available only for an explicitly authorized
development database or restored non-production clone. Review generator policy,
sanitized seed changes, and emitted header together. Never edit generated class
arrays manually. Repository tests establish implementation behavior; deployed
balance still requires game-owner review.

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
