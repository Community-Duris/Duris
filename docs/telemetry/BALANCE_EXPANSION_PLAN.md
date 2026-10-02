# Telemetry Expansion Plan for Battle and Progression Balance

**Status: accepted implementation scope, reviewed against the checkout on October 2, 2026.**

Implementation is underway. Delivered requirements, local qualification evidence,
and remaining work are tracked in [IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).

This expansion would make telemetry useful for balancing racewar battles, solo
and group PvP, PvE zone attempts, and progression across characters and accounts.
The recommendation is to extend the existing bounded capture, SQL writer,
external rollups, and read-only reports. The first release should produce
explainable measurements for staff review. Balance changes would follow a
separate experiment and rollback decision.

The central change is to record both sides of a battle and distinguish a
character, an account, and a known player controlling several accounts. Without
those distinctions, population, repeated opponents, gear, and character rotation
can look like a race, class, group, or progression problem.

All fields, report names, thresholds, and delivery stages below are proposals.
Existing capabilities are identified separately from the proposed additions.

**The balance questions determine the measurements.**

| Question | Measurements to add or publish | Why they help |
| --- | --- | --- |
| Are racewar sides competitively balanced? | Shared battle identity, side outcomes, participating character and known-player counts over time, external gear and level context, support roles, and faction activity exposure. | Separates advantages in mechanics from population, equipment, organization, and repeated experienced teams. |
| Can solo players meaningfully participate in PvP? | One versus one, one versus several, and group versus group episodes; death, escape, withdrawal, objectives where observed, losses, rewards, and time spent outnumbered. | Measures solo viability and risk instead of treating every unsuccessful kill attempt as the same failure. |
| Do groups gain an appropriate advantage? | Contribution and rewards per character-hour and known-player-hour, changing rosters, number advantage, and class/build composition. | Shows whether the benefit comes from more participants, coordination, support mechanics, or a particular composition. |
| Are zones appropriately difficult and rewarding? | Zone attempt identity, explicit completion or failure evidence, deaths, resource use, participant effort, interruptions by PvP, and committed reward attribution. | Compares reward against the whole cost of an attempt, including unsuccessful runs and recovery. |
| Is progression reasonable for someone playing one character? | Earned XP, death loss, restoration, milestone times, group assistance, rested exposure, level caps, and starting power. | Shows the practical time and risk required to progress through each stage of the game. |
| Does rotating characters or accounts create a disproportionate progression advantage? | Character-to-account links, verified account-to-player links, character switching, rested exposure, total portfolio progression, and overlapping play intervals. | Measures a player's combined advancement even when each individual character appears to progress normally. |

Balanced does not automatically mean identical results. Staff should choose the
intended tradeoffs: viable escape and opportunistic wins for solo players,
appropriate advantages for organized groups, harder zones with worthwhile
rewards, and an acceptable benefit from playing several characters. Telemetry
should describe how far the observed game is from those goals.

**The existing foundation can supply much of the data.**

Session and activity hooks already observe entry, reconnect, unload, copyover,
exclusive active/idle/unknown/linkdead time, and captured cohort dimensions.
Progression records already retain requested, computed, and observed applied XP,
source/reason/modifier flags, and separate level transitions. Encounters already
retain starts, roster changes, terminal outcomes, and participant effort. Combat
summaries already accumulate damage, healing, casting, and tanking in memory.
Reward projection already reads supported committed currency, epic, and frag
ledgers. The general report CLI currently exposes playtime, cohort, return,
UTC-day activity, and faction summaries.

Several source limits matter for this expansion:

- `game_combat_power_band()` currently derives its value from character level.
  It is not an equipment or overall combat-strength measurement.
- Encounter association currently uses scoped zone/group source identity. The
  game adapter derives its group key from the leader's character ID, with the
  individual subject as the solo fallback. This does not establish one shared
  battle containing opponents from several groups.
- The combat-start adapter chooses PvP when the victim is a PC. Player-owned
  opponents and assistance need explicit ownership-aware classification.
- Encounter state can already merge observed modes into `mixed`; preserve that
  capability and add time attribution for the changing mode.
- Combat control has a typed API, but the documented producer coverage currently
  centers on damage, healing, casting, and tanking.
- `account_characters` maps accounts to characters; authenticated descriptors
  carry an account. Neither proves that different accounts belong to one human.
  The existing multiplay whitelist is a host exception, not a player registry.
- Observed XP is not a committed XP ledger. Supported reward projection sources
  must be checked against current gameplay writers before claiming full coverage.

These findings come from [the runtime adapters](../../src/telemetry/telemetry_runtime.c),
[combat start](../../src/combat/fight_state.c),
[the encounter engine](../../src/telemetry/telemetry_encounter.c),
[account structures](../../src/account/account.h),
[the schema](../../migrations/bootstrap_multithread_safe.sql), and
[the current reporting interface](REPORTS.md).

**Identity must support several levels of analysis.**

Keep the existing scoped character subject. Add a stable opaque account token
and an optional opaque controller token for a known player controlling several
accounts. Scope these by environment and season, retain a mapping version, and
record whether the controller association is confirmed or unknown. A controller
token represents a reviewed association; it does not assert that telemetry can
identify every human behind every account.

Use authenticated ownership for character-to-account association and a restricted,
reviewed registry for cross-account association. Resolve and cache the bounded
tokens during login preparation, then copy them into telemetry session context.
The capture path must not introduce SQL, raw names, or expensive resolution on
each command or combat action. Missing mappings leave identity unknown and do
not obstruct gameplay. Reconnect and copyover preserve the captured association;
an actual ownership change creates a dated association boundary.

The association store should retain valid-from/valid-through history and evidence
provenance. Current ownership must not rewrite past sessions. A correction should
produce a new association version and a new report generation, leaving the raw
observation intact. Bootstrap supplies current associations only where the
authority establishes them; older periods remain unknown unless reviewed evidence
establishes the historical relationship. Account deletion and recreation must
allocate distinct lifetimes even when the same name is reused.

Do not infer a human from an IP address, email, device, or similar character name.
Shared households and connections make those unreliable. General report access
should expose only opaque tokens and published aggregates; a separate restricted
role owns reidentification and association corrections. Register the new stores
in the existing lifecycle, export, erasure, season reset, backup, and restore
contracts before activation. Pseudonymous association data remains subject data.

Publish character, account, and verified-controller results separately. Each
report must show linkage coverage by faction, mode, and progression cohort.
Unknown controllers remain an explicit unknown population; account counts must
not be relabeled as people. A linked subset may differ from the broader player
population, so its results cannot establish universal cross-account behavior.

For a known controller, sum XP and other comparable earned rewards across linked
characters, but calculate wall-time effort from the union of observed active
intervals. Keep the summed character effort as a second denominator. Six
characters contributing for an overlapping hour represent six character-hours
and one observed controller-hour when all six have a confirmed common controller.
The existing activity heuristic still does not prove continuous human attention.
Unknown time or clock ambiguity must produce coverage gaps rather than fabricated
controller-hours. Presence-derived battle effort and input-derived activity are
also different measurements and must stay labeled separately.

**Battle context must include opponents and changes during the fight.**

Add a small, fixed-memory battle association module that connects the existing
encounter identities across opposing groups. Its inputs are typed hostile and
support observations plus bounded participant snapshots. It should retain a
shared battle ID, participant identity, side identity, formal group identity and
revision, contribution/presence state, start/end observations, mode, and quality.
Use explicit generation identities for group membership; leader replacement must
not silently create a new group or invalidate prior participation.

Start a battle at the first observed qualifying hostile interaction. Add players
and player-owned actors through actual participation, bounded group presence, or
observed support. Co-location alone does not make everyone in a zone a participant.
Keep present group members, actual contributors, and reward-credit recipients
distinguishable. Pets and summons carry their owner attribution while retaining
their separate contribution; they increase combat presence without increasing
the human count.

Racewar identifies opposing factions, while actual hostile relationships and
observed group/support relationships establish participation. Same-faction PvP,
friendly effects, arena/training modes, and multi-sided fights need explicit
classifications. An ambiguous coalition should remain ambiguous rather than
being forced into a two-side racewar result. Classify attacks on player-owned
actors with their ownership context instead of treating every NPC as ordinary
PvE.

Seal roster and mode segments when observed counts, group composition, ownership,
or important context changes. Retain duration by friendly/enemy count band and
outnumbered state. A duel that receives reinforcements is a changing battle, not
a permanently labeled one-versus-one result. Linking previously separate
encounters must preserve their immutable keys and avoid counting the same
contribution twice. Chases across zones retain observed battle lineage when it
is provable; uncertain joins, splits, and merges carry explicit quality.

Close a battle at a reviewed terminal boundary or an inactivity rule. The rule
and any grace interval need their own definition version and executable fixtures.
Record deaths, survival, withdrawal, escape evidence, objective outcomes where
available, and unresolved closure separately. A kill establishes a death event,
not an entire battle victory. Report decisive outcomes only when the definition
and observed roster establish them. Interrupted and unresolved attempts remain
visible with unknown terminal results. Copyover/shutdown/crash boundaries must
not invent a winner or an unobserved continuation.

Capture compact, versioned actor context at entry and meaningful changes:

- Exact level, primary/secondary class and specialization, race, faction, and
  available epic/build progression identifiers with explicit stable mappings.
- Selected effective combat stats and starting/current resource bands, including
  maximum HP/mana, offensive and defensive values, and relevant resistances.
- A reviewed equipment category and bounded external buff/support categories.
  Derive these from already loaded state and fixed equipment slots, with unknown
  values when the classification cannot be established.
- Separate intrinsic race/class traits from external equipment and assistance.
  This allows total-effect comparisons without statistically removing the very
  racial or class mechanic under review.

Do not turn these inputs into an unexplained universal power score. Show the
selected matching dimensions and their version. Cache snapshots at meaningful
boundaries instead of rescanning equipment on every hit. Rate caps and overflow
behavior must preserve known time while marking uncertain attribution.

Extend the reviewed effective-property catalog to the actual PvP, group reward,
XP, and level-cap controls under study. Include build/content versions for
hard-coded race, class, and encounter mechanics. Audit which properties really
affect each route; maintained but unused knobs must not imply a balance change.
Dynamic participant counts, eligibility, zone alignment, and source conditions
remain event-time inputs. Reusing the current small payout/rested catalog alone
would not establish comparable historical combat configurations.

Extend the existing combat accumulator with reviewed effect families: control
attempts, accepted effects, resistance/immunity, and effective control duration;
useful prevention/absorption where the actual mutation boundary exposes it; and
bounded physical/magic contribution categories when needed for a specific
balance question. Overlapping control on a target must not fabricate additional
disabled time. Only add producer hooks after tracing the real effect boundary.
Damage remains one count at the actual damage boundary, and effective healing
remains one count after the actual healing mutation. Support players must remain
observable even when they never land the killing blow.

For racewar population context, accumulate delayed aggregate connected, active,
and participating counts by faction and reporting time band using existing
bounded runtime state. This distinguishes equal-sized fight performance from
the practical effect of one side having more available players. Report historical
aggregates rather than exposing live individual positions.

**A zone attempt is a different unit from one combat encounter.**

Introduce an expedition/zone-attempt ID that links its combat encounters,
participants, objective observations, and supported reward operations. Start on
the first meaningful engagement in a reviewed zone objective. Keep exploration,
repeat farming, and an attempt at an explicit completion objective separate.
Multiple expeditions in the same zone need distinct attempt identities.

Use the existing committed zone-touch outcome as completion evidence for zones
where that is the authoritative objective. Additional objectives need a reviewed
source adapter. Zones with no reliable completion definition can publish activity,
combat effort, and reward rates, while their clear rate remains unavailable.
A mob kill or a stone touch must not be generalized into an invented full-zone
clear definition. Capture source zone, credited zone, content version, and reset
generation only when those values are available at the actual source boundary.

Retain roster changes, participant effort, deaths, wipes when provable, withdraws,
elapsed attempt time, and bounded recovery time with its observation coverage.
Publish completion-time distributions alongside failure/abandonment frequency;
looking only at successful runs hides most of the cost of a difficult zone.
Distinguish PvE deaths from enemy-player interruption, and separate time spent
in PvE, PvP, and mixed activity. A zone can be mechanically reasonable and still
have poor practical progression because it is frequently contested.

Link supported committed rewards through exact operation/source identities.
Preserve the current reward projector's replay/conflict and reconciliation
semantics. Match observations only where lineage establishes the relationship;
time/zone proximity cannot establish a committed reward link. Missing links
remain unattributed. Parent/participant outcomes supply context and must not add
their amounts a second time. Keep gross earned rewards, deaths/losses, and
transfers separate. Do not interpret shared-bank withdrawals or moving equipment
between alts as newly created rewards.

**Progression reports need a character view and a portfolio view.**

Join observed progression facts to scoped session time, battle/zone context,
identity association, and the effective configuration. Add a compact progression
context containing rested tier and actual application, grant provenance where
observable, group-assistance mode, next-level threshold identity, and source
attribution. Sample actual rested eligibility inputs at the bonus decision
boundary and played-time exposure at effect changes or existing interval cuts.
Do not emit speculative offline activity or reparse score/command text.

Keep positive earned XP, death loss, resurrection restoration, administrative
adjustment, and level-threshold consumption in separate columns. Threshold
consumption is not a death loss. Label XP totals as observed unless an existing
authoritative receipt establishes durability for the exact fact. A saved XP value
can reconcile current state but cannot prove every missing historical award.
Extend durable XP reconciliation only through a separately traced authority
adapter; do not build another XP writer or ledger as a telemetry dependency.

Publish character rates within comparable level/race/class and configuration
cells. Total XP across very different stages of progression does not measure
equivalent advancement. Add time-to-milestone distributions and threshold-aware
progress measures with explicit definitions; keep epic, frag, currency, and
equipment development in their own units. Characters that have not reached a
milestone remain censored observations, rather than disappearing from the report.
Do not claim a median milestone time when the observed population cannot support
it, or infer a time-to-level for characters already beyond it at collection start.

For account and known-controller portfolios, report characters actively played,
switches between characters, elapsed time between switches, simultaneous sessions,
rested/well-rested exposure, combined comparable progression, milestones across
characters, and effort by solo/group/PvP/PvE mode. Retain whether high-level
assistance or strong starting equipment was observed. A new account is not proof
of a new player, and the first observed character is not necessarily a main.

The key comparison is whether players rotating several characters spend a much
larger proportion of their playing time under rested bonuses and gain more total
progress per observed player-hour. Also show how much progress they achieve on
each character. These views can distinguish portfolio advantage from the benefit
of concentrating effort on a single character. They should not automatically
classify efficient rotation as abuse. Changing account-level or player-level
bonus policy would require its own mechanics design and reviewed experiment.

Rested flags and pre/post award values support measured associations. They do
not by themselves reconstruct exact XP without rested bonuses: multiplier order,
caps, and integer rounding matter. A later counterfactual study should reuse
the real bounded calculation with complete captured inputs; dividing final XP
by a nominal multiplier is not sufficient.

**Four report suites would make the measurements useful.**

| Proposed suite | Primary views | Example staff decision |
| --- | --- | --- |
| `racewar_balance` | Population exposure; decisive and unresolved battle outcomes; matched size/gear/level comparisons; race, class/build, and composition contribution. | Investigate a racial mechanic when a disadvantage persists in comparable fights instead of immediately modifying a faction's rewards. |
| `pvp_group_balance` | One-versus-one, one-versus-several, and group fights; time outnumbered; survival/escape; support/control value; rewards and losses per effort. | Review escape, control, support stacking, group scaling, or frag distribution according to the observed mechanism. |
| `zone_balance` | Attempts, completions, failures, contested runs, effort and recovery, reward distributions, composition, and source coverage. | Adjust a specific encounter, objective, or reward when difficulty and payout diverge for the intended player group. |
| `progression_balance` | Character milestone times and XP sources; solo/group assistance; rested exposure; account and verified-controller portfolio rates and rotation. | Review XP stages, rested allocation, support rewards, or repeated farming without confusing alts with independent newcomers. |

Extend `report.py` over published aggregates and provide versioned exports for
the existing rested study and shadow recommendation tools. Keep the old v1
reports unchanged in meaning. Existing field names, enums, and the current
shadow policy's one epic-payout target must not be silently reinterpreted as
general combat or progression policy. New tuning targets require a separately
reviewed policy definition. A dashboard can consume these exports once the
reports qualify; it is not required to start useful measurement.

Each report should show its definition/configuration/content/identity versions,
scope, publication generation, occurrence coverage, lag, retained-source bounds,
missing context, excluded observations, and sample sufficiency. Keep failed and
incomplete observations in coverage counts even when they cannot support a
particular rate. Publish appropriate raw totals plus comparisons that limit
repeated player/team/opponent influence. Show distinct characters, accounts,
verified controllers, teams, and battles wherever those identities are available.

Compare like with like before proposing a change. Choose matching dimensions
according to the question: controlling for external gear helps a race comparison,
while controlling away the race's own intrinsic bonus may hide its total effect.
Group composition is important for a class contribution study but is itself part
of the effect when studying composition. Prior observed player experience is a
limited context variable, not a trustworthy universal skill rating.

Repeated battles from one team or player are correlated evidence. Show total
activity and a player/team-balanced sensitivity view, cap repeat influence in
recommendations, and calculate uncertainty at an appropriate independent unit.
Do not count every participant or damage record as an independent battle. Report
sparse comparisons as insufficient evidence. Publish both overall rates and
stratified rates; changing cohort mix can reverse an apparent aggregate trend.

For additive rates use total compatible amount divided by total compatible
exposure, and publish the distribution of individual rates separately. Never
average unweighted XP/hour ratios or derive medians from sums. Milestone-time
distributions require retained episode observations and censoring metadata, or
a versioned approximation clearly labeled as such. Persist the bounded rollup
information needed for these distributions instead of querying unrestricted
raw history from the report role.

The initial numerical definitions should be explicit:

- Earned XP/hour divides positive earned-source XP by covered active hours in
  the same comparable cell. Net observed XP includes death loss and restoration
  separately identified, excluding administrative and threshold-consumption rows.
- Committed epic/frag/currency reward per participant-hour divides supported
  ledger amounts by covered participant effort. Known-controller rates use
  compatible linked rewards and the union of that controller's covered effort.
- Decisive PvP win share divides qualified wins by qualified decisive battles.
  Show the full attempted-battle count, unresolved fraction, and survival/escape
  outcomes alongside it; a high unresolved fraction prevents a general win claim.
- Zone clear share uses objective-complete attempts over attempts with qualified
  known terminal results, with incomplete/censored attempts displayed separately.
  Rewards per attempt and per effort include covered unsuccessful attempts.

**The implementation can be delivered in six stages.**

| Stage | Work and likely code boundaries | Evidence required to finish |
| --- | --- | --- |
| 1. Publish the facts already collected | Extend `scripts/telemetry/rollup_definitions.py`, `rollup_engine.py`, `db_access.py`, and `report.py` for progression, encounter, combat contribution, and supported reward summaries. Freeze the new grains and unsupported metrics first. Verify source authority and coverage for current reward writers. | Replay-safe aggregate generation; observed versus committed labels; failed/unclosed attempts visible; exact amount/exposure denominators; old report behavior preserved; restricted report-role reads. |
| 2. Add identity and actor context | Extend login/session/runtime context, config/catalog identity, and small typed actor/context records. Add the restricted versioned association store and report projections. | Account/character rename, deletion/recreation, transfer, mapping correction, unknown linkage, several accounts per controller, interval unions, and reconnect/copyover fixtures. Missing identity must preserve gameplay. |
| 3. Connect both sides of battles | Add `src/telemetry/telemetry_battle.h` and `.c` as a pure bounded association state machine; extend runtime/encounter/combat adapters and group observations. Add reviewed control/prevention producers and faction exposure counters. | Duel, outnumbered solo, several groups, reinforcements, healer-only participation, pets, leader changes, flee/rejoin, zone chase, mixed PvP/PvE, ambiguous coalitions, duplicate linkage, and capped-state fixtures. |
| 4. Link zone attempts and progression context | Extend the existing zone and progression observation adapters, the reward projector's context linkage, and external aggregate definitions. Add reviewed completion definitions only for sources that can establish them. | Success/failure/abandonment/censored tails; distinct simultaneous expeditions; PvP interruption; delayed/replayed committed outcomes; no duplicated rewards; level threshold conservation; rested and assistance exposure; sequential rotation versus simultaneous characters. |
| 5. Publish the four balance report suites | Extend the existing report/export paths, study adapters, and report catalog. Produce comparison fixtures and versioned statistical definitions. | Matched/cohort comparisons; repeated-team influence; unknown-linkage populations; sparse data; aggregate/cohort reversal; exact and censored milestone examples; no causal claims from observational exports. |
| 6. Qualify and observe before changing mechanics | Run isolated gameplay/persistence journeys and actual database/load/failure tests, then a separately authorized observation pilot. Provision writer/rollup/report identities and scheduled external jobs through the normal deployment process. | Measured game-loop, writer, report, and save-latency budgets; memory/cardinality limits; outage/copyover recovery; representative coverage; sufficient independent observations; a concrete candidate change with baseline and rollback criteria. |

The sequence is intentional: useful existing summaries come first, identity and
context make later comparisons meaningful, shared battles establish the unit of
PvP analysis, and zone/progression joins supply costs and rewards. Each stage
should be split into narrow reviewable changes at the listed boundary when its
schema, producer, or reporting work cannot be validated together.

Extend the existing fixed-size tagged record contract through explicit versioned
additions; do not overload v1 enum meanings or exceed its 512-byte record limit.
Prefer separate small context/link facts to an unbounded participant array inside
one record. Reuse the existing queue, writer, SQL connection ownership, replay
keys, quarantine, quality flags, control reserve, and generation publication.
All per-action work stays bounded and in memory. New relational storage belongs
in additive immutable migrations allocated from the then-current manifest head,
with matching bootstrap/runtime/lifecycle registration and restricted roles.

Retain the current finite encounter/actor limits as the starting capacity envelope
and measure the new battle association state's complete memory and operation
cost. Row-rate limits must cover roster/context churn as well as normal fights.
Overflow or drop must make a battle or identity-dependent metric visibly partial;
neither truncation nor a new larger limit can silently claim complete racewar
coverage. Test larger battles that exceed the cap and quantify whether expected
traffic requires a reviewed capacity increase.

The focused executable tests should extend the existing telemetry session,
activity, gameplay, encounter, combat-summary, repository, rollup, report,
reward-projection, rested-study, and capacity/fault harnesses. Add a pure battle
harness for the new state machine. C++ changes require the repository build and
changed-line formatting checks. Database and gameplay proofs must use disposable
allow-listed local fixtures on both supported SQL engines, with the disabled and
client-free paths covered. The current synthetic performance gate is useful but
does not establish actual server/database performance for this expansion.

**Reviewing the plan removes several unnecessary first-release components.**

This design does not need a broker, warehouse, new gameplay persistence lane,
durable telemetry spool, or raw per-hit event log. It also does not need automatic
cross-account fingerprinting, a learned universal skill/power score, or a new
frontend before the report definitions qualify. Those additions increase cost
without satisfying a missing requirement in the requested balance questions.
Existing summary accumulators, a narrow battle association state machine,
versioned identity/context, and bounded report projections provide the necessary
information. Exact player linkage that cannot be established remains a declared
limit rather than a speculative inference service.

**The observation and balance loop should remain reviewable.**

Choose target ranges for the intended play modes, then collect enough qualified
observations in the relevant level/gear/composition and identity cohorts. The
initial reports establish associations and candidate mechanisms. A change should
state the mechanic under test, expected directional effect, primary metric,
guardrail metrics, eligible population, observation window, and rollback rule.
Test one interpretable change at a time. Use a holdout where gameplay permits it,
or a comparable staged before/after design while displaying population and
configuration changes that may explain the result. Require human review of
evidence and commit through the existing configuration owner.

No universal minimum battle count or fixed number of observation days is assumed.
Choose sufficiency for the actual variation, repeated players/teams, and smallest
effect worth acting on. The existing small synthetic study thresholds are
engineering fixtures, not live balance qualification. Keep collecting or abstain
when comparisons remain sparse. Automated recommendations can follow qualified
reports; automatic application is a separate future decision.

This plan is complete as a design proposal when each requested balance question
has a defined population, outcome, exposure denominator, source authority,
coverage limit, report, and executable acceptance scenario. Implementation is
complete only when those reports pass the isolated integrated tests and the
authorized observation pilot supplies representative evidence.

Supporting contracts: [activity](ACTIVITY.md), [sessions](SESSION_LIFECYCLE.md),
[progression](PROGRESSION.md), [encounters](ENCOUNTERS.md),
[combat contribution](COMBAT_METRICS.md), [configuration](CONFIG_CONTEXT.md),
[transport](TRANSPORT.md), [rollups](ROLLUPS.md),
[rewards](REWARD_PROJECTION.md), [reports](REPORTS.md),
[rested study](studies/rested-bonus.md),
[shadow policy](balance/SHADOW_POLICY.md),
[reviewed application](balance/APPLICATION.md),
[performance gate](PERFORMANCE_GATE.md), and
[data lifecycle](../persistence/DATA_LIFECYCLE.md).
