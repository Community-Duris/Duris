# Progression context, milestones and rotation

The observed-XP context, milestone and rotation slice is locally qualified on
`codex/telemetry-balance-expansion` / draft PR #683. Native capture, durable storage,
exact retained inputs/configuration, atomic definition-9 publication and restricted
readback pass the complete 45-phase actual-server gate on both SQL engines.
The existing definition-3 effort and observed XP publisher and definition-8
battle comparisons/outcomes keep their meanings. Issue #258 remains open.

## Source audit

| Native boundary | Meaning to preserve |
| --- | --- |
| `src/world/limits.c:gain_exp` | Requested integer input, post-modifier integer candidate, final per-call bound and observed `GET_EXP` difference are separate. The storage guard can leave the applied amount zero. Dirty status is not a committed save receipt. |
| `src/magic/affects.c:has_active_rested_bonus` | With the automatic gate enabled, presence of an eligible rested tag selects the bonus. With it disabled, `get_spell_from_char` requires a living character and an `AFFTYPE_CUSTOM1` staff tag. Wellrested takes precedence. These tag lookups do not filter `AFFTYPE_NOAPPLY` or duration. The tags are not managed spell wards. |
| `src/world/limits.c:gain_exp`, rested branch | Resurrection bypasses rested multiplication. Other sources pass the selection before source-specific calculations; a later branch can replace the candidate. A selected multiplication is not proof of an increased observed award. |
| `src/combat/fight.c:kill_gain` | Solo and group kill inputs are calculated before `gain_exp`. Group count and highest level use in-room, untrusted PC eligibility. The group divider and level-gap penalties affect the requested input. Formal roster size is a different observation. |
| `src/world/limits.c`, healing/tanking branches | Healing checks self/group relation, an actual opponent and the level-gap boundary. Tanking counts the eligible same-room PCs and requires more than one. Damage can issue a nested tanking award after its actual relation/level gate. These decisions require their own source values. |
| `src/world/limits.c:advance_level_impl` / `lose_level_impl` | Level transitions are distinct from XP changes. Ordinary level-up consumes a threshold; this is not a death loss. A call to the level persistence path does not establish its commit. |
| `src/world/limits.c:update_exp_table` | Thresholds are the actual table values after configured fallback and difficulty scaling. A next-threshold identity includes table/index/version, observed amount and build/content/configuration references. The small existing property catalog alone does not describe every XP control. |
| `src/world/epic.c`, `src/economy/boon.c` | Epic and boon level paths directly consume XP before advancement. Preserve this as level cost/threshold use rather than earned XP or a death penalty. |
| `src/combat/fight.c`, lich death branch | Lich residual XP is adjusted outside `gain_exp` after losing a level. Retain its actual amount and death provenance; do not count the old stage threshold as another generic XP award. |
| `src/combat/chaos.c`, `src/cmd/actwiz.c`, generic staff XP setter | Administrative or system changes need their own labels and cannot enter earned-XP rates. Initialization/materialization is an observed starting state, not an award. |

`gain_exp_modifiers`, `gain_global_exp_modifiers`, race/victim multipliers,
difficulty, level caps and source-specific gates must be reconciled with the
reviewed effective catalog before declaring a configuration comparable. Dynamic
victim, group, nexus, epic and hometown conditions remain source context. No
counterfactual XP is obtained by dividing an observed amount by a multiplier.

## Independent value contract

The definition-1 context value has 82 exact numeric fields: original producer
and sequence, logical session/connection, optional complete source and ownership
receipt keys, scoped account token, paired clocks, configuration/build/content
versions, actual current level/XP and next threshold, class masks/specialization,
race/faction, ten base and effective stats, rested inputs/selection/application,
formal roster count, separately declared assistance eligibility inputs,
starting level, source inventory version and boundary. It includes an exact XP
configuration root and 32-byte canonical digest, the observed faction level cap,
good/evil assistance gaps, property level cap and hardcore/bypass policy inputs.

The native value is 384 bytes; its network-order encoding is 373 bytes. Reserved
native fields must be zero. Unknown flags/versions, partial or foreign producer
keys, an account token without an ownership receipt, stale unavailable families,
unsupported source families and unmarked reversed UTC are refused. Optional
missing identity remains explicit. Shape validation cannot establish that a
referenced receipt was retained, reviewed or committed.

Eligibility is separate from application. Presence/staff flags describe observed
tags, and the living-character flag retains the native staff lookup gate.
Complete affect evidence permits checking the selection inputs. Partial affect
inventory stays partial; an actual decision hook can separately report the
branch that ran. Application and storage gates are admitted only at a linked XP
decision boundary. A point cannot assert contiguous exposure.

The bounded point reader traverses at most 64 distinct affect nodes and reuses
the maintained formal-group bound. Cycles/cap truncation retain observed positive
tags while leaving selection unknown. It reads the actual current threshold,
including an explicit unknown if its source/value/index is unavailable. It copies
class masks and base/effective stats without gear/epic hashing, SQL or an XP
mutation. It establishes no assistance decision from formal group presence.

## Actual XP configuration inventory

The independent definition-1 configuration value enumerates 337 source values:
62 actual level thresholds, all 62 cached `exp_mods` slots, both 101-slot racial
modifier arrays, the actual global storage limit, three effective difficulty
multipliers, and the live good/evil assistance gaps, XP level cap, death fraction,
automatic rested gate, trophy observation gate and good-side death-loss threshold.
The inventory includes cached slots that some award branches do not use; it does
not claim that each observed value was applied to a particular award.

The native snapshot is 6,176 bytes within an 8 KiB bound. Its canonical encoding
is exactly 3,729 bytes, including definition/source inventory and
build/content/classifier/policy versions. Values use explicit signed integer,
float32-bit or float64-bit kinds. Missing source indices, duplicate indices,
unknown versions/kinds, non-finite floats, stale unused capacity and nonzero
reserved fields are refused. The native and independent Python encodings and
SHA-256 fingerprints agree.

The fingerprint excludes the process-local configuration ID; exact retained
configuration receipts still have to bind that ID and source scope. A changed
threshold, modifier or version changes the fingerprint. Capturing the inventory
is a cold bootstrap/property-application operation: it reads already maintained
values and performs no character traversal, SQL, XP mutation or property reload.
Hashing belongs to that cold boundary and retained reconciliation, not each XP
hit. The original 20-property catalog is unchanged.

The inventory uses 15 exact family-16 chunks with at most 24 values per chunk;
each chunk is 400 native bytes and 396 canonical bytes. Its first admitted raw
receipt is the immutable root. Reconciliation requires every ordinal, original
raw receipt, common source/scope/version and matching full digest. Missing,
duplicate, foreign, conflicting or reordered admission identities fail closed.
Delivery order alone does not invalidate a complete exact inventory.

Native bootstrap prepares the inventory before publishing the general
configuration. Property application prepares it after the effective values
change and before the runtime clears the old configuration. Adoption binds only
the matching prepared property revision; it does not rescan or hash the sources.
A failed admission retries one fixed packet per pulse without a new property
hash. A newer configuration cuts an incomplete prior inventory. Point capture
references the root only after all chunks admit; publication must additionally
verify durable retained completeness. Queue admission is not a durability receipt.

SQL faction level caps and hardcore policy decisions need their actual per-award
values as additional decision evidence. Victim, nexus, learned epic and group
conditions remain observed context. This inventory alone is not an award receipt,
proof of committed character state, or a counterfactual rested/group advantage.

## Durable source storage

Family 15 and 16 retain the original six-field transport header in
`telemetry_interval`. The existing physical table stays at 537 columns.
`telemetry_progression_context` adds an 88-column typed source table and
`telemetry_progression_configuration` adds a 101-column typed source table.
Each payload carries the exact original receipt key and header. The bounded raw
reader joins these tables by that key, exposing 714 logical columns without
expanding the older raw table.

The native repository inserts the header and typed payload within the same
batch transaction. An exact replay checks all typed fields. A stored header
without its payload conflicts on replay; replay cannot backfill the missing
evidence. Foreign keys bind payloads to their original raw receipt and cascade
raw retention. Insert guards check header agreement and minimum shape; update
guards preserve immutable payloads. Full value and source-reference validation
still belongs to the native and independent retained-input contracts.

The typed source tables keep the original raw table within its existing
storage limit and preserve sealed migrations. Additive migrations 0071/0072
and their exact verifiers pass on MariaDB 10.11.14 and MySQL 8.0.46. Native
repository fixtures and actual-server gameplay remain separately labelled
evidence in the complete qualified command.

Direct XP changes in epic and boon level paths, lich death, Chaos advancement,
staff advancement, punishment and the staff XP setter now observe the actual
before/after XP values at the existing writer. Threshold costs retain their own
reason; administrative and death adjustments cannot enter earned-XP rates.
These hooks reuse the observation producer and do not establish an award or
character-save commit. Initialization and database/file materialization remain
baseline observations.

## Publication requirements

- Retain exact activity, lifecycle, XP, ownership and context receipts with their
  configuration evidence, original input origin/cursor, counts and rolling
  digest. Independent loss review must include the new family. Refuse capacity
  before fetching; retain the existing 32 MiB invocation, source/page/output and
  record budgets.
- Publish a new definition/generation atomically. Keep old definitions, reviews,
  incident versions, sealed migrations and published generations readable and
  unchanged. Restricted readers use only qualified public stores.
- Preserve character/account/confirmed-controller effort and unknown populations
  using the existing dated attribution and covered unions. A confirmed review is
  a controller association, not a census of unique humans.
- Milestone episodes retain completed, unfinished, already-past, missing-start,
  clock-ambiguous and lifecycle/configuration-cut observations. Connected,
  heuristic active and elapsed exposure have separate coverage. No elapsed
  median is invented from sums or completed episodes alone.
- Switching distinguishes successive character activation from overlap and
  simultaneous sessions. Review/ownership boundaries, incompatible clocks,
  source gaps and unknown controllers prevent unjustified portfolio unions.
- Rates divide compatible earned amounts by covered effort in the same observed
  level/threshold, class/build, configuration, rested-selection and formal-group-size
  exposure stratum. Actual assistance and rested application remain award-decision
  dimensions. The rate is NULL when its denominator or source linkage is unsupported.
  Keep earned, lost, restored, administrative/system and threshold amounts separate.
  Sparse samples and repeated characters/controllers remain visible.
- XP, epic, frag, currency and equipment development retain their own units.
  Canonical economic amounts remain dependent on proven authority and bounded
  reconciliation, including the unresolved #487 compatibility audit. Account
  transfers and overlapping old/new journals cannot create rewards.

## Retained source and atomic publication

Definition 9 binds every point to its exact original XP, activity or lifecycle
receipt and dated ownership receipt. It reconciles all 15 configuration packets
and verifies the actual threshold value. Missing source, ownership,
configuration or general configuration metadata stays explicitly unknown;
conflicting subjects, clocks, owners, digests, ordering or capacity are refused.

Separately counted exact references may predate a rebuild origin. Only the
selected context's original XP/interval/lifecycle, ownership or configuration
references are accepted. Changed, missing, replayed, unrequested or differently
scoped references are refused. A reference already selected by the cursor is one
original source, rather than additional XP or effort. Original references never
advance the selected cursor.

Intervals retain their original transport connection keys. Linked interval, XP
and lifecycle contexts must match all three original source connection
components. Native SQL's `lifecycle` field is normalized to `lifecycle_kind` at
ingestion; conflicting aliases and populated inactive family fields are refused.
Retained decoding remains canonical.

The five additive `0072_telemetry_progression_publication` stores are a private
source header, selected inputs, exact original references, public coverage and
immutable public rows. Page inputs and their digest advance in the existing
cursor transaction. Publication binds reserved dated identity and independent
incident schema 8, then commits all five report families and coverage atomically.
Restricted readers need only public stores and pinned generation metadata.
Definition 3, earlier battle definitions and their generations retain their
original meanings and remain readable after supersession and raw retention.

## Native exposure and lifecycle boundaries

One value-only progression span occupies each existing session slot: at most
256 spans within 128 KiB, with no character pointers or handoff copy. An admitted
activity interval can carry exposure linked to its original receipt and exact
clock endpoints. A positive contiguous flag requires a known live runtime
identity, matching session/connection, admitted ownership, a living actor,
unchanged level/build/configuration/rested-selection/formal-group-size values,
and exact monotonic/UTC coverage. Cached XP is an anchor observation; it is not
claimed constant throughout a span and does not define its duration stratum.

Affect insertion/removal/rebuild, equipment/build changes, group changes, level
writes, staff field changes and in-place rested updates invalidate spans. This
includes temporary book-reading class swaps and mutate-and-restore sequences.
Presence checks the fixed build fields before snapshot refresh. Interval delivery
also resolves the live runtime identity and rereads bounded context and current
account membership: native events can run after descriptor presence. Missing or
changed values leave preceding exposure unknown. An unchanged unknown snapshot
does not repeatedly flush activity. These checks never write XP or player state.

Logout captures a lifecycle cut only after the exact matching session-exit
receipt admits, retaining that receipt and its clocks. Successful copyover
admission captures a fresh baseline after observing reloaded ownership. The
session privately retains the last admitted ownership receipt's own monotonic
time; unchanged later presence cannot erase the older interval reference.
Pending loss, changed ownership, membership and exact receipt checks still gate
qualification. An empty secondary capture preserves primary uncertainty even
when the primary update emits no record. The producer's immutable paired clock
and existing UTC/monotonic continuity checks remain unchanged.
Elapsed time uses the steady clock. UTC coordinates retain the existing bounded
projection; time unions across producers remain observations of those coordinates.
Unavailable or conflicting clocks stay unknown.

## Report meanings and uncertainty

| Report | Supported observations |
| --- | --- |
| `progression_context` | Exact point/decision/exposure receipt, source and ownership references, actual level/threshold, effective configuration, observed build, rested and assistance inputs, and explicit missing/partial evidence. |
| `progression_milestones` | Completed, unfinished, already-past and censored level stages, with separate connected, input-heuristic active and elapsed exposure and missing interval coverage. |
| `character_rotation` | Character activation, sequential switches and gaps, concurrent sessions/characters, connected and heuristic-active rotation, and dated character/account/confirmed-controller views. |
| `progression_effort` | Summed character effort and covered account/controller time unions, independent unknown clock and controller populations, without an inferred count of unique humans. |
| `progression_portfolio` | Requested, computed and observed applied XP; earned, death, resurrection, threshold, administrative/system and uncategorized amounts; exposure-qualified rate cells and explicit uncertainty. |

Milestone strata label the starting retained build/rested/group-size point.
`point_context_only` cannot establish continuous values throughout a stage.
Observed changes cut the segment. A qualified full-stage time establishes
retained clock/activity coverage between observed level boundaries, not
continuous build or assistance exposure. Ordinary threshold transitions use
source `system` and reason `level_threshold`; their exact threshold receipt and
configuration can establish observed completion. Administrative changes are
censored. Starting level records the first point in the current native
producer/session collection segment; copyover recovery establishes a fresh
baseline. It cannot establish character creation or lifelong progress.
No completed-only milestone median is published.

Rates require contiguous exposure in the same level/threshold, observed build,
exact configuration, rested selection and formal-group-size stratum. Actual
assistance and rested application are award-decision dimensions, never inferred
durations. Missing or unclassified exposure, an award outside retained exposure,
identity uncertainty or loss makes the rate NULL. Supported rates retain exact
integer numerator/denominator/scale tuples. The denominator scope is
`observed_level_build_configuration_rested_selection_formal_group_size_exposure`.
Input-based active time remains an existing heuristic, not measured attention.

The actual-server rate fixture declares one connected-exposure prefix through
the native kill-share decision, its exact covering interval and immediately
preceding qualified exposure. Native `no misfire` tags cut the wider fight.
Earlier fight awards and uncertain intervals remain in generations 3 and 4 with
unknown rates; subsequent lifecycle/recovery remains in generation 6. Six adjacent
declared windows retain the full progression range under unchanged budgets.
The very short decision window proves the capture/publication path. It cannot
establish the effort to earn the entire kill reward, a whole-fight or population
rate, sample sufficiency, or a causal rested/rotation advantage. A zero
input-heuristic active denominator and its NULL rate remain explicit. Kill-share
evidence requires a positive original `kill` source; damage/melee awards alone
cannot establish it.

Telemetry rows remain observations of mutable player state. Publication,
telemetry SQL commit, dirty status and a call to player persistence cannot
establish an XP-award or character-save commit. The native journey checks actual
save/readback independently against the player store. Unsupported durability
claims, universal scores, altered rows and absent snapshot details are refused.

XP, epic, frag, currency and equipment development retain their separate units.
Other reward units are unavailable in definition 9. Its versioned economic
dependency remains `487`. Issue [#487](https://github.com/Community-Duris/Duris/issues/487)
was closed on 2026-10-03 to consolidate its unfinished work into
[#490, section 4](https://github.com/Community-Duris/Duris/issues/490#4-independent-audit-and-reward-projection).
That current accounting tracker owns the canonical old/new ledger and
parent/participant receipt seam, with openings, transfers and corrections
separate from earned rewards and bounded reconciliation for delayed commits.

The registered normalized schema fingerprints are
`1c7fb936dc997e870189d5400c57b8882696bcf3382e6645da7e9bc5beff2232`
for MariaDB and
`dcd66f88886b2f4041494d45b1249344b3d773f0396f1747754f89b5a23be0a1`
for MySQL. Both compiled runtime contracts match the 75-step / 282-table head.

## Qualified local delivery: 2026-10-06

The maintained compatible-tools command passed **45/45 phases** on both SQL engines:

```text
python tests/async/qualify_telemetry_controls.py --disposable --tools-image duris-telemetry-control-tools:local
```

Run `8a64d6e010a6`; receipt `bin/tests/duris-controls-8a64d6e010a6/qualification.json`; SHA-256
`516ec581aa3cc97f388fe70c737ee42ca1135cb4854b2de9deba9635b1b5d3ea`. Qualification source SHA-256 is
`80cf764b9c1aeacdd8234cd82e7ffaa3d25e160c201ac75624e71c710c2b9fa4`. Sources stayed unchanged throughout the run.
Final documentation evidence is audited separately against the frozen 6,100-file snapshot.
The tools image was `sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`.
Both actual servers used binary SHA-256 `1f0b0bd866a05b56fce9d8a7d0f077271a3f3be9d8e0ce4c8a9eb76a59a16abd`.
All owned containers, disposable databases and users were removed. No production or staging access was used.
Receipts and synthetic logs remain ignored under `bin/`.

The server build, formatting, twenty focused programs, five feature ASan/UBSan phases,
paired-clock ASan/UBSan, runtime/lifecycle contracts and performance guards passed.
Both engines qualify all six 75-step histories through `0072_telemetry_progression_publication`,
282 protected SQL tables, compiled boot/restoration, drift/tamper refusal and guarded reruns.
Native writer checks cover all sixteen families, exact replay, header/payload atomicity,
missing-payload refusal and lost acknowledgements. Definition-9 SQL checks reject 79
immutable-column updates and qualify rollback, committed/rolled-back lost acknowledgements,
raw retention, supersession, preserved older definitions and restricted public readback.

Actual gameplay qualifies native XP/levels, rested and solo/group-share decisions,
same-account sequential switching, overlapping accounts, a dated confirmed-controller fixture
and unknown controllers, independent player-store save/readback, fresh copyover baselines,
native XP/save during a private writer outage and fresh-producer recovery.
Progression fights reissue the same ordinary attack within a bounded wait when
native critical misses stop combat. Native kill-share messages and positive
stored receipts are checked separately; misses and context cuts remain retained.
Earlier ordinary PvP elapsed controls and reviewed battle/outcome paths remain qualified.
Two adjacent definition-7 windows retain the original control-study range, including
the earlier copyover/outage producers and the full recovery-study prefix. Both windows
retain immutable unreviewed generations with NULL durations after reviewed publication.
The actual earlier outage remains in the independent review; overlapping rows retain NULL durations.
A lost endpoint may leave no stored duration prefix inside the gap. Retained active baselines
have unknown duration, and no endpoint is synthesized beyond the last committed receipt.

| Engine | Earlier control window / recovery window | Unreviewed / reviewed generations |
| --- | --- | --- |
| mariadb | (0, 548] / (548, 3228] | 1,2 / 3,4 |
| mysql | (0, 569] / (569, 3669] | 1,2 / 3,4 |

| Engine | Native active baseline ingest IDs | Stored rows overlapping the gap | Last committed / first unknown sequence |
| --- | --- | --- | --- |
| mariadb | 530 | 2 | 77 / 78 |
| mysql | 551 | 0 | 73 / 74 |

| Engine | Native progression input range | Completed / unfinished / qualified full stage | Native kill-share XP / covered connected us | XP / input-heuristic active us |
| --- | --- | --- | --- | --- |
| mariadb:10.11.14 | (5130, 6190] | 2 / 3 / 1 | 572 / 409 | NULL |
| mysql:8.0.46 | (5629, 6818] | 2 / 3 / 1 | 572 / 838 | NULL |

The milestone counts describe generation 1, not completed-only population statistics.
Each engine publishes six adjacent generations with all five restricted reports.
The very short connected-time rate is a native kill-share decision window, not
the effort to earn the entire reward. Earlier fight awards and unknown rates remain
retained in generations 3 and 4. The input-heuristic active denominator records actual
command activity; zero exposure retains a NULL rate. Neither clock measures attention
or total reward-earning effort. No whole-fight, population or causal advantage is claimed.

| Engine / generation | Window | Selected inputs / exact references | Reserved bytes |
| --- | --- | --- | --- |
| mariadb / 1 | milestones | 167 / 0 | 7618560 |
| mariadb / 2 | group_decisions | 210 / 18 | 9555968 |
| mariadb / 3 | rotation | 244 / 18 | 11087872 |
| mariadb / 4 | uncertain_fight_exposure | 34 / 19 | 1626112 |
| mariadb / 5 | comparable_rates | 6 / 16 | 364544 |
| mariadb / 6 | persistence_and_recovery | 164 / 19 | 7483392 |
| mysql / 1 | milestones | 131 / 0 | 5996544 |
| mysql / 2 | group_decisions | 182 / 18 | 8294400 |
| mysql / 3 | rotation | 274 / 18 | 12439552 |
| mysql / 4 | uncertain_fight_exposure | 138 / 19 | 6311936 |
| mysql / 5 | comparable_rates | 6 / 16 | 364544 |
| mysql / 6 | persistence_and_recovery | 188 / 19 | 8564736 |

Source/page/record/output budgets and the 32 MiB invocation limit are unchanged.
Generation totals and time unions are not combined into a population rate.

The native private-writer benchmark passes 75 profiles: five stages, 50/200/256 admitted
players, five repetitions and 4,096 samples. Queue draining before independent lifetime
and watch assertions occurs outside measured callbacks. Existing queue capacity is unchanged.

| Native stage | Worst p99 / p99.9 / maximum (ns) |
| --- | --- |
| native_pending_watch_pulse | 188649 / 641653 / 5083578 |
| native_result_begin_finish | 18781 / 95905 / 825686 |
| native_progression_cached_presence | 14295 / 93853 / 435818 |
| native_progression_mutation_refresh | 30932 / 94267 / 630244 |
| native_progression_observation | 25068 / 90765 / 269271 |

All stages retain the p99 1 ms / p99.9 5 ms guards and zero event-time heap/crypto calls.
Progression state remains 256 value-only spans within 128 KiB. SQL/Telnet and production
load are excluded from callback benchmarks. The existing control encoding, paired clock
and transport/worker/rollup/report workloads pass their original guards.

| Engine | Save mode | Samples | Median / maximum round trip (ms) |
| --- | --- | --- | --- |
| mariadb | telemetry_off | 10 | 2003.785 / 2005.507 |
| mariadb | telemetry_on_status_present | 10 | 2004.236 / 2005.331 |
| mysql | telemetry_off | 10 | 2004.248 / 2004.792 |
| mysql | telemetry_on_status_present | 10 | 2004.447 / 2005.829 |

Save timings include Telnet scheduling and authoritative persistence; telemetry does
not acquire save/award authority from these measurements. TSan is not run by this
command; the earlier Docker host probe was unsupported. No full burn-in or production
load qualification is claimed.

Issue #258 remains OPEN with all seven final checkbox lines verbatim and unchecked.
PR #683 remains DRAFT. Canonical other-unit reward compatibility is the next dependency:
#490 section 4, formerly #487. Distinct PvE attempts, remaining battle producer coverage,
the four complete balance suites and statistical study exports remain unfinished.
