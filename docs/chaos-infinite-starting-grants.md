# Chaos Infinite Starting Grants

This feature applies only to new characters created while `CHAOS_MUD=TRUE`.
Normal-mode behavior is unchanged.

## Configuration

The master switch is:

```text
CHAOS_STARTER_BONUSES=TRUE
```

The master switch can be disabled independently of Chaos mode. These feature switches can then be disabled individually:

```text
CHAOS_STARTER_FRIGATE=TRUE
CHAOS_STARTER_EPIC_SKILLS=TRUE
CHAOS_STARTER_EPIC_POINTS=TRUE
CHAOS_STARTER_BANK_PLATINUM=TRUE
CHAOS_STARTER_MATERIALS=TRUE
```

Values must be exactly `TRUE` or `FALSE`. An invalid value disables that feature. Every accessor also requires `CHAOS_MUD=TRUE`, so setting a starter switch in normal mode has no effect.

## Starting grants

### Tattoo ship

The existing `AIP_FREESLOOP` achievement marker is retained for save compatibility. In Chaos mode, the Sailor's Tattoo claim at the docks is a zero-cost Frigate claim. Normal mode continues to use the existing Sloop reward/discount.

### Epic skills

The starter grants epic skills using the same `epic_rewards` and `epic_teachers` tables as the normal epic-teacher path, evaluated for an unspecialized character:

- class masks and the Thri-Kreen exception are respected;
- teacher rows must exist;
- deny-skill mutual exclusions are respected in deterministic table order;
- prerequisite skill levels are checked;
- skills are granted to the teacher-defined maximum;
- no epic points are spent for the starter unlocks.

### Epic points and bank platinum

After the new-player baseline save, the server submits:

- `+20,000` spendable epic points through the epic ledger;
- `+1,000,000` platinum in the account/racewar bank ledger through the currency ledger.

The grants use the dedicated `chaos_starter_reward` ledger reason. They are not direct in-memory balance writes and are retained until the character is ready if they commit before entry.

### Salvage and encrust materials

Chaos characters receive one compact `Chaos craft pouch` in the starter bag when `CHAOS_STARTER_MATERIALS=TRUE`.

The pouch is a real persisted item and can be carried or attached to a belt. It contains no individual material objects. Use `look in pouch` or `examine pouch` to see its compact catalog:

- all salvage-material tiers `400000` through `400209`;
- all encrust jewels `400291` through `400299`.

When the pouch is carried, nested under a carried container, or attached to belt slots 1–3:

- `craft`, `forge`, and aggregate superior `enhance` treat all raw-material requirements as supplied;
- legacy two-argument `enhance <source> pouch` uses the pouch as a non-consuming universal donor while preserving the normal legacy replacement, cost, level, and affect gates;
- the pouch is not consumed;
- tools, magical essences, platinum fees, level/recipe gates, and output ownership still apply normally;
- `encrust <item> <jewel-vnum>` can select a jewel from `400291` through `400299` without a physical jewel in inventory. The selected jewel is temporary and the pouch remains.

The pouch is protected from salvage and cannot be used as an enhancement or encrust target. If the pouch is absent or `CHAOS_STARTER_MATERIALS=FALSE`, existing inventory-material behavior is unchanged. No material satchel chain, bulk reserve, snapshot-capacity increase, or global transfer-limit change is used.

## Lifecycle

1. Rules acceptance establishes the synchronous player baseline.
2. The durable ledger grants and normal Chaos bag (including the single craft pouch when enabled) are submitted before game entry.
3. The character enters and is linked to the live character list.
4. Epic/bank completions publish through their existing `*_player_ready()` hooks.
5. Chaos level advancement completes.
6. No-spec epic skills and tattoo achievement initialization run for the new character.
7. The normal starter bag completion emits the prepared message; no material continuation follows.

Existing characters are not retroactively modified by the creation-time pouch grant. The pouch's consumer-side behavior is active only while Chaos mode and `CHAOS_STARTER_MATERIALS` are enabled.
