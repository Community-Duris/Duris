# Typed participant and objective evidence

The battle comparability and outcome extension in draft PR #683 extends the
existing native capture, writer, retained source, atomic publication and restricted
reports. Definition 8 publishes versioned build comparison dimensions and typed
participant/objective evidence together with their denominators and qualification
coverage. The complete personal-local command passed on both supported SQL
engines; exact current delivery evidence is recorded below. Native value and
repository fixtures remain separate from actual-server positive outcomes.

## Evidence definitions

Record family 14 carries independently versioned participant or objective
observations. Result definition 1 and producer version 1 distinguish these values:

| Kind | Authoritative evidence | Interpretation |
| --- | --- | --- |
| `death_observed` | Accepted native fatal branch with context retained before teardown | Death of the recorded actor; whole-battle victory remains unknown. |
| `flee_movement` | Accepted flee that actually changes rooms | Successful movement; escape remains unproven. |
| `withdrawal` | Accepted retreat movement or accepted same-room disengagement | The particular native withdrawal action; no inferred victory or escape. |
| `escape_observed` | Complete bounded native watch after an exact preceding movement event | Requires the same original player/session, valid association, no observed opponent, a complete watch and at least 30,000,000 monotonic microseconds. The parent identifies the original movement event. |
| `objective_requested` | Accepted submission of the supported zone-touch operation | Requested objective operation; commitment remains unknown. |
| `objective_committed` | Validated game-thread zone-touch completion receipt, with recovered original claims explicitly marked | Exact supported objective evidence. It does not prove a complete zone clear. |
| `unresolved` | Explicit source, actor, clock, configuration, movement or receipt uncertainty | Preserve the observation and its uncertainty; exclude it from positive outcome counts. |
| `censored` | Explicit lifecycle cut of a pending observation | Preserve the observed prefix and exact parent; no completion is invented. |

The contract checks value shape and declared authority. Positive qualification
must additionally prove that the actual native producer followed the required
gameplay path. A fixture with valid escape flags is not evidence that a live
player escaped. Likewise, a valid objective value is not a replacement for an
exact committed operation/source link.

The target is the participant affected by the observation. An optional source
records a causal actor, such as a killer, when its context is available. Actor
identity, current ownership, optional session, group generation and exact
association references remain separate. Missing association values are zero
tuples rather than guessed battle IDs. NPC and pet identity uses the maintained
runtime generation domain; prototype identity cannot replace it.

## Exact identities and uncertainty

Each value has its original producer and nonzero result sequence. Its optional
parent is an earlier result sequence from that producer. The admitted transport
receipt remains a separate identity. A replay cannot acquire a new receipt and
be counted as a new result. The record header's producer and occurrence clock
must match the value's producer and accepted-observation clock.

Retained pre-action context and accepted-observation clocks are distinct. Elapsed
watch duration uses the monotonic clock. Missing UTC and clock discontinuity
remain explicit; neither is repaired from ingestion time. Configuration-unknown
values clear configuration, classifier, policy, build and content versions and
carry an explicit unresolved/censored reason and context uncertainty.

Objective values retain the exact 16-byte critical operation ID, native payload
version, credited zone and declared participant count. Version-2 values also
retain the physical stone UID; version-1 legacy values explicitly lack that UID.
Recovered receipts carry a separate flag. These fields permit exact linkage and
replay review; they do not authorize counting a recovered claim twice. Missing
original claim evidence must stay unknown until reviewed publication establishes
the correct operation/source relationship.

This record contains no XP, epic or monetary amount. Parent operations and their
participant reward receipts must be reconciled by the canonical reward projection
required by #487, rather than counted again as rewards from an objective record.

## Native producers and bounded escape observation

The native fatal branch copies source and target context before legacy encounter
teardown. Refused, dummy and reincarnated paths cannot become positive deaths.
The supported fatal event establishes that gameplay branch, rather than a durable
corpse, reward receipt or whole-battle victory. Accepted flee, retreat and
disengagement paths retain their pre-action association and actual room transition.
Refused movement remains unresolved. Same-room disengagement establishes a
withdrawal, without starting an escape watch.

An accepted room-changing flee or retreat can start one of 512 fixed watch slots.
The native watch holds less than 256 KiB and does not allocate per event. It
requires the original player lifetime and session, stable group/configuration,
a live target, no opponent and no renewed battle association for at least 30
seconds on the monotonic clock. Re-engagement, effective support, death, logout,
copyover, shutdown, configuration change and unavailable clock or actor evidence
cut the pending watch explicitly. Capture capacity refusal preserves resident
observations rather than evicting another player.

Pending-watch selection shares the existing periodic build budget: at most 16
combined selections per pulse, with a single bounded world pass of at most 4,096
characters. The existing 16 full build reads per second, 512 build-cache entries,
128 battles and 64 actors per battle remain unchanged. An absent watch adds no
world scan. A pre-action token is consumed once; its captured native lifetime is
resolved before a potentially expired supplied character pointer is dereferenced.

The supported objective producer runs at accepted zone-touch submission and the
game-thread completion pump. The request retains its exact operation ID, stone
UID, payload version, zone and credited participant count. A matching validated
commit retains those identities and original claimant. Pending, failed or
malformed receipts remain unresolved. A recovered original claim is retained
separately and cannot count as a newly committed objective. The worker outbox
delivery path has no native result adapter; an outbox authority declaration alone
is unsupported for positive qualification.

## Reviewed build comparison dimensions

Comparison catalog version 1 is grounded in the native class masks and currently
populated specialization catalog. Each exact build point has 17 independently
classified dimensions:

| Dimension group | Recorded fields and interpretation |
| --- | --- |
| Level, classes, specialization, race and faction | Exact declarations; unknown class bits and unsupported specialization cells remain unclassified. The existing level-derived power band remains a level proxy. |
| Base setup and learned epics | Recorded intrinsic/base fields and learned-skill fingerprint; no claim of complete intrinsic setup. |
| Equipment | Exact bounded equipment declarations and fingerprint; no claim that every declared item effect was active. |
| Effective setup, current resources, saving modifiers and effective flags | Separate observed state families; they cannot be collapsed into intrinsic setup or gear. |
| Listed effects | Exact bounded effect fields; an incomplete listing remains partial. |
| Temporary-effect origin and support origin | Unclassified when no native provenance establishes origin. Base/effective differences and generic flags cannot identify a caster. |
| Arena room and arena roster | Separate room flag and native arena membership declarations. Unclassified membership stays explicit. |

Each dimension is `observed`, `partial`, `unavailable` or `unclassified`, separately
from point association, clock, configuration and loss qualification. Matching must
name its selected dimensions; only observed, qualified values within the same
configuration can match. Equal selected fields do not imply equal combat strength.
A recorded point establishes its observation time rather than continuous build
exposure, and no damage, healing or outcome is attributed to that point.

Effective native healing/support remains separately observable through exact
kind-10 relationships and kind-11 contributions. Those facts do not supply missing
generic buff provenance. Account/controller attribution continues to require the
existing authenticated ownership and reviewed identity evidence; a group or shared
host cannot establish a known human controller.

## Wire, writer and loss storage

The independently checked Python and native contracts define 74 named fields in
a 375-byte portable value. The native payload is 408 bytes. The tagged record
remains 488 bytes within the existing 512-byte record envelope. Result detail
uses ordinary detail admission and cannot consume the lifecycle reserve.

Additive migration `0069_telemetry_battle_results` adds nullable `bout_*` columns,
the unique `(bout_boot_id,bout_process_id,bout_sequence)` domain index, complete
payload and inactive-family guards, and insert/update validation triggers. The
writer validates exact configuration/build/content identity before accepting a
configured result. Binary operation bytes use the existing explicit SQL hex
path. Quarantine retains the named identity and reserved-field representation.

The worker's startup metadata scan accommodates the declared 537-column tagged
table and its previous 49-column allowance for optional additions. Other table
scans retain their 512-column limit. This startup change does not increase native
capture, queue, record, source-publication or report budgets.

Outage ledger version 6 (`DMSTLJ06`) recognizes families 1–14. Versions 1–5 retain
their original family masks, word geometry and read-only byte semantics. A
checksummed older ledger still cannot claim family-14 loss. Independent reviewed
incident schema 7 stores typed-result loss in `telemetry_incident_registry_v7`
and `telemetry_incident_v7`; earlier reviewed inventories remain sealed.

Existing buildable report definitions 1, 2, 3, 5, 6 and 7 validate new raw result
detail and advance their mixed-stream cursor without changing their retained
source or metric meaning. Historical definition 4 remains read-only. Definition 8
retains the new exact values and configuration bindings without changing those
earlier definitions.

## Atomic publication and restricted readback

Additive migration `0070_telemetry_result_publication` creates four independent
version-8 source, input, coverage and row tables. The existing cursor transaction
retains original source/configuration evidence, producer/result keys, exact actor
and ownership contexts, association revisions, immutable aliases and pre-action
roster references. Existing source, byte, page, row and payload bounds apply.

One atomic snapshot publishes the existing battle, contribution, build and
selected-control reports with `battle_build_comparisons` and `battle_outcomes`.
The comparison report is an alias of the same build-point rows, preserving exact
values once and adding classification status and matching usability. Outcomes
retain all 74 raw fields alongside linkage, clock, chain, objective and source
statuses. A restricted report role reads only published rows and coverage, and
cannot read raw inputs, configuration or incident/identity registration tables.

Event evidence qualification and battle-context qualification are independent.
Positive event qualification requires a supported native producer, verified
occurrence clock/configuration and independent family-14 loss review. Escape also
requires its exact same-producer movement parent and a verified actor/session/
association chain. An objective commit requires a verified matching request;
recovered, duplicated, cross-producer or conflicting commitments do not count as
new qualified commits. A qualified battle context additionally requires verified
association and the exact observed pre-action roster. Sessionless objective
receipts may reference only their exact request's historical context and retain
unknown current authenticated identity.

The shared coverage header conserves result totals across all eight kinds,
qualified/unknown event evidence, qualified battle context, configuration gaps,
recovered/duplicate objectives and qualified commits. It also conserves all 17
dimension cells across observed/partial/unavailable/unclassified statuses and
records matching-usable cells. Missing configuration, review, stale references,
loss, clocks or retained projection quality cannot be erased to make a value
qualify. No report asserts whole-battle victory, full zone clear, complete
population coverage or a universal strength comparison. These are evidence
denominators rather than complete attempt/success rates.

## Maintained qualification

The existing personal-local command remains:

```text
python tests/async/qualify_telemetry_controls.py --disposable
```

Its expanded checks include standalone tagged headers, cross-language result
values, sanitizer runs, versioned loss compatibility, native repository replay
on each SQL engine, all positive result value shapes in SQL, active/inactive
family refusal and independent private incident review. These checks are
explicitly labelled value/repository fixtures. They remain separate from the
real-server gameplay journey and do not satisfy that journey by themselves.

The native runtime fixture now exercises pre-teardown death, accepted movement,
withdrawal, a watch completion, cancellation/lifecycle cuts and validated objective
receipt paths through the native writer and restricted definition-8 publication on
both SQL engines. Synthetic watch time in that fixture is explicitly labelled;
it cannot qualify a live escape. The actual-server journey additionally requires
ordinary equipment changes, effective support, death, accepted flee, real 30-second
watch completion, changing participation and exact committed zone-touch identity.

The maintained command also checks full native adapters under ASan/UBSan, safe
freed-target rejection, 256 admitted live sessions, world-scan refusal, per-pulse
selection limits and zero event-time heap/cryptographic calls. Its separate
optimized probe measures pending-watch pulses and begin/finish callbacks at 50,
200 and 256 admitted sessions with 4,096 samples and five repetitions under the
existing p99 1 ms / p99.9 5 ms guards. This probe uses a private writer seam and
synthetic completion time, and claims no actual-server positive outcome or SQL
latency measurement.

## Qualified local delivery: 2026-10-05

The exact compatible-tools command passed **38/38 phases** on both SQL engines:

```text
python tests/async/qualify_telemetry_controls.py --disposable --tools-image duris-telemetry-control-tools:local
```

Receipt: `bin/tests/duris-controls-89625dc38352/qualification.json`; SHA-256
`b58929cf5e11684e9a0ee24b194f8236dacb8dd86f1e434372914c221f1b2268`. The receipt records source SHA-256
`3a7b66aa14d15d951fd70daa68291012b2ba95704197d007f0d53b9b7a156f42`, source stability throughout the run, tools image
`sha256:74b699976165c15fc29cf92b9c2dbefcdbca35505a08efc84d14bf644cbf6d5b`, actual gameplay and removal of all created containers.
Summed phase duration was 5959.469 seconds. Both running servers used binary
SHA-256 `82c7c82da46125b56b49dc12e04ff46838506f2f0b426588f4a5e1ada601469a`. Receipt files and synthetic logs remain ignored under `bin/`.

The build, changed-line formatting, sixteen focused programs (including 98
history tests), runtime/lifecycle validators, five ASan/UBSan phases and measured
performance checks pass. On each engine all six maintained histories converge
at 73 steps / 273 protected SQL tables. Compiled boot/restoration, guarded reruns,
exact replay and lost acknowledgements, drift/tamper refusal, atomic publication,
private role boundaries and immutable older definitions/generations pass.

| Engine | Retained definition-8 inputs / reserved bytes | Published build points / qualified | Exact result rows / qualified event / battle context | Native escape (us) |
| --- | --- | --- | --- | --- |
| mariadb:10.11.14 | 88 / 2887680 | 25 / 21 | 5 / 5 / 3 | 30053862 |
| mysql:8.0.46 | 87 / 2854912 | 25 / 21 | 5 / 5 / 3 | 30059075 |

Each actual server supplies exactly five kind-14 facts: accepted room-changing
flee, a real scheduler escape of at least 30 seconds, native fatal PvP, accepted
zone-touch request and validated committed objective. All five retain qualified
event evidence; movement/escape/death retain qualified pre-action battle context
and at least two character owners. Objective request/commit do not acquire
invented battle context or current authenticated identity. Exact operation bytes,
physical stone UID, committed inbox and authoritative claim agree.

The two compared equipment points have matched level/class and different native
equipment observations. Effective healing and the healer's group-2 to group-1
departure retain their exact source references. Generic buff/support provenance
remains unclassified. Separate complete clean-drained producers preserve the
existing definition-7 ordinary PvP duration study and the full definition-8 outcome
prefix within unchanged source, byte, page and row budgets. Earlier unreviewed
generations and missing-configuration negatives remain unqualified.

The native runtime/SQL fixture independently exercises withdrawal, unresolved and
censored observations, recovered/missing objective links and all eight value
kinds. Its synthetic escape clock is not counted as actual-server escape proof.
The optimized private-writer probe passes all 30 profiles with zero event-time
heap/cryptographic calls: native_pending_watch_pulse: worst p99 173.196 us, p99.9 653.290 us, maximum 4512.158 us; native_result_begin_finish: worst p99 26.421 us, p99.9 98.215 us, maximum 246.951 us.
It excludes SQL and Telnet and retains the 1 ms / 5 ms p99 / p99.9 guards,
512 watches, 256 admitted sessions, one bounded 4,096-node world pass, shared
16 selections per pulse and 16 build reads per second.

The original control capture/encoding benchmark passes all 30 profiles at
50/200/512 targets with 213,048 bytes of fixed state and zero event allocation:
worst p99 0.575 us, p99.9 37.260 us and maximum 346.280 us. It excludes runtime context refresh and SQL. Production paired
clock sampling in the two ASan/UBSan variants also passes: worst p99 2.221 us, p99.9 25.911 us and maximum 73.305 us; it
excludes context and SQL. The existing transport/worker/rollup/report probe passes
50/200/1,000 workloads and six injected faults under its unchanged capture
1 ms / 5 ms, worker p99.9 50 ms, report/rollup p99.9 5 s and stop/enqueue 100 ms
guards. These fixture budgets do not establish production load qualification.

The existing selected-control elapsed-prefix, save, copyover, private writer
outage and explicit operator-restart recovery paths pass again.

| Engine | Save capture mode | Samples | Median round trip (ms) | Maximum (ms) |
| --- | --- | --- | --- | --- |
| mariadb:10.11.14 | telemetry_off | 10 | 2004.431 | 2005.645 |
| mariadb:10.11.14 | telemetry_on_status_present | 10 | 2004.358 | 2004.944 |
| mysql:8.0.46 | telemetry_off | 10 | 2003.715 | 2504.237 |
| mysql:8.0.46 | telemetry_on_status_present | 10 | 2004.403 | 2004.937 |

These save timings include Telnet scheduling and authoritative persistence;
they do not isolate callback cost. These are controlled synthetic-account
qualification journeys, not population balance rates. TSan is unrun by this command; its earlier host capability probe was
unsupported. No full burn-in or production load qualification is claimed.

Whole-battle wins, full zone clears, continuous build validity, complete intrinsic
setup, generic effect provenance, universal combat strength, complete attempt
rates and proven action-restriction/caster duration remain unavailable. Distinct
PvE attempts, progression/rested/assistance/milestone/portfolio additions,
prevention/faction exposure, the four complete balance suites/statistical exports
and canonical economic projection #487 remain in the accepted scope. PR #683 stays
DRAFT and issue #258 stays OPEN with all seven final checkbox lines unchanged.
