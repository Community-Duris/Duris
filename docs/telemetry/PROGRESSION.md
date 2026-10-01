# Telemetry progression facts — #267

This document specifies the first post-foundation progression extension. It
records what the game observed at the existing XP storage and level-transition
boundaries. It does not become an XP authority, change balance, or claim that
an accepted queue record is durable.

## Scope and boundary

`gain_exp` emits one bounded progression fact after the existing XP storage
guard has run. The fact therefore contains the actual mutable `GET_EXP` value
before and after that call. It is not derived from `display_gain`, combat log
text, or the value that would have been displayed to the player. A rejected
storage candidate is still observable as `applied_xp = 0` with its computed
candidate and modifier flags.

The level functions in `src/world/limits.c` emit a separate level transition
fact after the level field is changed and persisted through the existing game
path. XP consumed to cross a level boundary is represented by
`reason = level_threshold`; it is not emitted as a reward or a death loss.
Death loss, resurrection restoration, and the level change caused by a death
loss retain their source/reason classification. Direct administrative level
operations use `system_adjustment`. A future authority may add a more specific
administration adapter; this extension does not classify arbitrary raw writes
to player state that bypass these limits boundaries.

The adapter performs only value copying, validation, sequence allocation, and
bounded queue admission. It does not acquire a database handle, write SQL,
allocate a per-source map, or add a damage-event writer. The generic rollup
engine remains unchanged; progression reports read the tagged fact stream.

## Fact shape

All progression records use `record_kind = 6` in the existing immutable
`telemetry_interval` stream. The additive migration leaves every progression
column nullable so older interval, lifecycle, checkpoint, gap, and configuration
records retain their absent-field representation.

| Field | Meaning |
| --- | --- |
| `progression_kind` | `1` observed XP storage; `2` level advanced; `3` level lost. |
| `progression_source` | Bounded source: damage, healing, kill, death, quest, resurrect, melee, world quest, tanking, boon, administration, system, or unknown. |
| `progression_reason` | `earned`, `death_loss`, `resurrection`, `level_threshold`, `administration`, `system_adjustment`, or unknown. |
| `progression_observation_status` | H emits `observed_mutable` (`1`). `recovered_checkpoint` and `durable_reconciled` are reserved for later recovery/authority work. |
| `progression_modifier_flags` | Fixed bit set for rested/well-rested, over-level-cap, difficulty, PvP, final per-call cap, race, and victim paths. Unknown bits are rejected. |
| `progression_requested_xp` | Original `gain_exp` input. |
| `progression_computed_xp` | Integer candidate after the existing source/modifier calculations and before the existing final per-call bound. |
| `progression_applied_xp` | `after_exp - before_exp` at the storage boundary. This is the observed credit/loss and may be zero. |
| `progression_before_exp`, `progression_after_exp` | Mutable XP snapshots surrounding one XP storage attempt. |
| `progression_before_level`, `progression_after_level` | Same level for an XP fact; changed levels for a transition fact. |
| `progression_threshold_xp` | The explicit threshold used by a level transition, or zero for an administrative/system adjustment. It is never included in XP reward/loss sums. |
| dimensions/config/version fields | The session's existing subject scope, bounded dimensions, effective config identity, classifier version, policy version, and quality flags. |

`progression_applied_xp` is validated against the two stored snapshots with
overflow-safe signed arithmetic. Level facts require zero XP request/candidate/
applied values, zero XP snapshots, and a real direction change. The explicit
threshold is the only XP-adjacent value on a level fact. This prevents a
threshold-consumption record from fabricating an XP loss and prevents a
displayed, capped candidate from being counted as credit.

## Stable source and reason mapping

The existing `gain_exp` type constants map to the bounded source enum without
persisting an arbitrary type or player-controlled text:

| Existing type | Source | Reason |
| --- | --- | --- |
| `EXP_DAMAGE` | damage | earned |
| `EXP_HEALING` | healing | earned |
| `EXP_KILL` | kill | earned |
| `EXP_DEATH` | death | death_loss |
| `EXP_QUEST` | quest | earned |
| `EXP_RESURRECT` | resurrect | resurrection |
| `EXP_MELEE` | melee | earned |
| `EXP_WORLD_QUEST` | world_quest | earned |
| `EXP_TANKING` | tanking | earned |
| `EXP_BOON` | boon | earned |

The adapter marks only bounded modifier paths that it can identify at the
existing boundary. A victim or PvP path, rested state, difficulty path, race
modifier path, over-level-cap path, or final integer cap is a flag—not a copy
of the property bag. Trophy persistence continues through the existing bounded
trophy component; a progression fact does not copy trophy names or arbitrary
metadata. Unsupported future inputs remain `unknown` until a reviewed enum
extension exists.

## Report definitions

Reports should aggregate only `progression_kind = 1` rows for XP totals and
should group by the bounded source/reason dimensions. The immutable replay key
(`boot_id`, `process_id`, `record_seq`) is the first deduplication boundary;
replaying an identical record is a no-op in the repository. A report may use a
second explicit fact key when it materializes an aggregate, but it must not sum
both an original row and its replay.

An observed-source report can use the existing I interface as follows:

```sql
SELECT environment_id,
       season_id,
       progression_source,
       progression_reason,
       SUM(CASE WHEN progression_kind = 1
                THEN progression_applied_xp ELSE 0 END) AS observed_xp,
       COUNT(CASE WHEN progression_kind = 1 THEN 1 END) AS observed_xp_facts,
       SUM(CASE WHEN progression_kind IN (2, 3)
                THEN 1 ELSE 0 END) AS level_transitions,
       BIT_OR(COALESCE(progression_modifier_flags, 0)) AS modifier_flags,
       BIT_OR(COALESCE(quality_flags, 0)) AS quality_flags
FROM telemetry_interval
WHERE record_kind = 6
  AND progression_observation_status = 1
GROUP BY environment_id, season_id, progression_source, progression_reason;
```

Level-threshold rows may be counted as transitions, but their
`progression_threshold_xp` must not be added to `observed_xp`. The report must
also surface queue-drop, sequence-gap, unknown-dimension, and unclosed-tail
quality rather than turning incomplete observation into a durable total.

## Crash, reload, and durability semantics

`observed_mutable` means that the game thread saw the value change and retained
the fact in the bounded telemetry queue. It does not mean that a save, SQL
transaction, or recovery checkpoint committed the same XP value. A crash after
capture and before queue drain leaves an unknown tail; a queue drop is explicit
and does not silently become zero XP. A reload/checkpoint can later explain
current state or promote a fact, but this extension never labels a raw
observation `recovered_checkpoint` or `durable_reconciled` on its own.

The migration is additive and rerunnable. No gameplay save/login dependency,
ledger projection, rollup table, production migration action, or retention
policy is introduced by #267.
