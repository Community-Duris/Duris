# Item ability rollout and rollback

This runbook covers the initial delivery of [#290](https://github.com/Community-Duris/Duris/issues/290), through #291–#297. It prepares a deployment; it does not authorize production changes. The master, every category and the five native artifact flags ship disabled. See the [verification record](../testing/ITEM_ABILITY_ROLLOUT.md) for the distinction between real server encounters, forced native callback fixtures and benchmarks.

## Delivered roster and editor boundary

| Family | Opted-in behavior | Disabled behavior |
| --- | --- | --- |
| Avernus 19730 | Original 1/25 selection; delayed drain and healing together | Immediate native drain/heal; stone and hum unchanged |
| Packed/random weapons | Captured spell selection and victim; passive telegraph | Existing immediate weapon callback |
| Wands, staves, scrolls | Active action; charge/scroll paid before warning; native spell order/filtering | Existing device command |
| Wand of wonder 41350 | Captured original outcome and random arguments; bounded native resolution | Existing native special |
| Studio | Strict versioned catalog and typed HIT/CMD-use binding; shared mana | Existing Studio/native special precedence |
| Tsunami 31514 | Paid cancellable tap/thrust/raise; persisted native cooldowns | Existing native commands |
| Mirrored ioun 922 | Paid synchronous interception before incoming damage | Original reactive deflection |
| Living necroplasm 67243 | Paid transformation linked to source; equipment penalties retained | Existing native equipment special |
| Mayhem 21 / Symmetry 22 | Paid native bundles, cursor, challenge, flurry, eight-second minimum nova | Existing native sword state machine |

Contracts: [foundation](../reference/ITEM_ACTIONS.md), [weapons](../reference/WEAPON_ACTIONS.md), [mana](../reference/ARTIFACT_MANA.md), [Studio](../reference/STUDIO_ITEM_ABILITIES.md), [native pilots](../reference/NATIVE_ARTIFACT_PILOTS.md). The [source inventory](../reference/artifact_source_inventory.json) lists 169 templates, 78 active bindings and 67 callbacks. Other templates retain legacy behavior and require a reviewed contract and measured budget for a later wave. Presence in the source does not establish live availability. This release changes no bulk area or loot data.

The user authorized a stub at the private DurisStudio boundary. [#329](https://github.com/Community-Duris/Duris/issues/329), assigned to **Faemill**, supplies the schema, examples, public validation stub and editor acceptance checklist. The server has real `.trg` parser/runtime evidence. Private editor UI execution and end-to-end editor integration remain Faemill's external handoff; this release does not claim those tests passed.

## Prepare a wave

1. Record the server revision, backend, wave roster, catalog/profile revisions, costs and responsible operator. Synthetic test budgets are test inputs, not approved production balance.
2. Qualify a coherent backup using [BACKUPS.md](BACKUPS.md), including players, item custody, world/corpse/locker state, journals and mana. Flat-file backups must use the managed path that takes the mana writer lock.
3. SQL installations require additive **0016_artifact_mana**, applied and verified through the immutable migration runner after 0015. Preserve history and manifest checksums. It adds the InnoDB per-UID authority without filling items. Flat-file primary uses protected `domains/artifact-mana-<uid>` records and needs no SQL migration. Storage failure never switches mana to another backend.
4. Rehearse restore/startup with the intended binary and schema in isolation. Verify stable UIDs, retained depletion, empty new/cloned pools and schema compatibility. Run the linked verification commands and native permission/group/race/class checks for the selected roster.
5. Complete the private editor handoff before promising an editor authoring workflow.

## Configuration acceptance matrix

Enable properties require exactly `1`. Invalid numeric limits fail closed. Use zero for rollback. Broad `properties reload` cancels pending item actions before reloading properties and validating the Studio catalog.

| Gate/change | Required result |
| --- | --- |
| `itemActions.enabled=0` | Cancel pending work; new native selections use disabled paths. Re-enable does not revive old work. |
| `itemActions.{avernus,weapons,randomWeapons,wands,staves,scrolls,wonder,studio}.enabled=0` | Category rollback; other categories retain their configured state. |
| Per-template switch | Use the adapter's documented property prefix; do not assume device and weapon prefixes match. |
| `itemActions.artifact.<vnum>.enabled=0` | Roll back 21, 22, 922, 31514 or 67243 independently. Cancel pending work; necroplasm removes its own linked grant. |
| `itemActions.mana.enabled=0`, paid adapter enabled | Suppress paid powers; changing this gate cancels all pending item work, including uncharged actions, as a conservative configuration barrier. No free paid fallback. |
| Invalid foundation/profile value | No new migrated action or free paid fallback. Correct the value and submit a fresh action. |
| Disable then enable | Retain committed costs and native cooldowns. Legitimate elapsed regeneration continues while disabled. |
| New definition revision | Cancel affected old work; new invocations use the new immutable definition. Changed definitions need a greater revision. |
| New capacity/rate | Increase profile revision; settle at old rate, clamp to new capacity, adopt new rate. Increased capacity never fills the difference. Profile identity cannot silently change. |
| Malformed Studio catalog | Retain last valid definitions; report validation failure. Broad properties reload still cancels old pending work. |
| Abort, source removal, death or transfer | No ordinary refund; no action follows stale source/target identities. |
| Restart/copyover | Restore no action/token. Read independent mana authority. Studio process-local cooldown resets; persisted native object cooldowns remain. |

For the initial isolated/canary wave, use `itemActions.maxPending=128`, `maxPerWielder=2`, `reactionPulses=4`, `maxPulses=120` (all under `itemActions.`), and the documented eight-pulse category windup. The code ceiling of 4,096 entries is not demonstrated live capacity. Nova requires a global cap of at least 32 pulses; otherwise the enabled sword rejects that selection. One source has one pending reservation, an actor has one active action, and passive work can coexist with ordinary casting.

Use the exact keys and limits in each adapter contract. Mana amounts are integer thousandths of a point. A new pool persists zero before spending and then earns regeneration. Cold/failed reads may suppress first use; changing UID or editing reserve is not a recovery procedure.

## Observe and advance

Enable `itemActions.telemetry.enabled=1`; a trusted operator uses `itemmana metrics`. Mortals inspect only their carried/equipped item with `itemmana <item>`. Aggregate metrics expose no player/item identity or enemy reserve, and produce no per-hit log flood.

Counters are process-local saturating integers. `selected` means runtime-owned selection; `started` means admission/payment succeeded; `completed` means every captured effect was invoked, including when the final native call kills/extracts a participant. Rejections include busy, invalid, scheduling, consumption, insufficient mana and unavailable storage. Cancellation reasons distinguish departures, definition/configuration changes, reload, abort, scheduler rejection and cleanup. `partial` and `effect_failures` mean dispatch stopped after at least one effect ran with captured effects still uninvoked. Native spell calls return no success status: saves, immunity and zero damage are not engine effect failures or completed-hit statistics.

`pending` is a gauge. Peak, callback count, total/max microseconds and invalid-clock count describe the observation window. Timing covers timed callbacks and synchronous interception, including native effects; it excludes other admission/tick work. Off then on resets metrics without cancelling actions. Restart resets the window too.

Before widening, observe at least a 15-minute isolated encounter interval with item counters and existing whole-tick/persistence health. Proposed initial investigation thresholds, to review for the host:

- Stop immediately for unauthorized targets, duplicate effects, UID mix-ups, unearned refills, revived actions, persistence integrity failures or crashes.
- Stop on any effect failure, invalid-clock sample or scheduling rejection in a controlled canary reproduction.
- Pause expansion above 96 pending entries, or above 10% busy suppression among selected attempts over five minutes. Repeated input on one source is expected; distinguish it from population saturation.
- Pause if callback maximum exceeds 10 ms, or whole-tick p99 regresses by over 10% or 5 ms (whichever is larger) for three consecutive one-minute windows. Existing latency tracing supplies p99; item metrics provide counts/total/max only.
- Stop paid-power admission if persistence does not recover after the two-second stale interval, or shutdown repeatedly leaves dirty/outstanding pools. Diagnose storage pressure without deleting ledger rows. Runtime already suppresses affected UIDs at the stale boundary.

These thresholds are investigation gates, not a player-capacity claim. The real Studio configuration journey measured a 9.031 ms native callback on the development host, close to the proposed 10 ms gate; investigate it in an idle comparable-host baseline before authorizing expansion. The no-op benchmark's synchronized 4,096-action release took about 154–171 ms; 128 actions released in 84–93 microseconds. Native effects and real world scans still require canary measurement. One noisy sample cannot establish a telemetry overhead percentage.

Advance from disposable fixtures to an explicitly approved low-population wave, then one family at a time. Record exact roster/profile revisions, counters, latency and sustained reserve trajectory. Each later template needs its own wave evidence and balance review.

## Rollback

**Runtime rollback:** set the affected native/category gate to zero, or the master to zero for a systemic fault. Verify pending work reaches zero on processing and cancelled actions never complete. Preserve costs, UIDs, cooldowns, journals and mana authority. Persist the intended properties/catalog so restart cannot accidentally enable the wave. A mana-only disable suppresses paid abilities; it does not request legacy behavior. Reproduce the resulting native behavior in isolation and retain aggregate evidence.

**Old-binary/schema rollback:** treat this as separate maintenance. Stop new activity, drain persistence and qualify a coherent backup. Check the candidate binary's runtime schema manifest against installed schema/lifecycle policy before startup. Extra 0016 schema must not be assumed compatible with an older executable. Keep 0016 and immutable history when reverting only flags. If an incompatible older binary is required, rehearse a matched pre-upgrade binary/schema/state restore in isolation, then obtain separate production maintenance authorization. Restoring players without matching custody, journals and mana can duplicate or refill assets.

The durability guarantee is a **bounded crash-refund window**, not fsync-before-effect. A crash may refund debits admitted in less than two seconds from the oldest unacknowledged debit, bounded by available reserve/cost/concurrency. The real SIGKILL test waited for a durable debit and recovered it exactly; worker fault tests establish the unacknowledged-window bound. Clean shutdown attempts a five-second drain. Keep these two cases distinct in incident reports.
