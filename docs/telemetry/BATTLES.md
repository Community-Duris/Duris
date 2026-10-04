# Shared battle association

Enabled, qualified telemetry now captures shared-battle facts at native hostile,
effective-healing support, presence, context and lifecycle boundaries. Its bounded
association engine, portable definition-1 packets and independent kind-10 writer
preserve battle history and conservative uncertainty. This establishes live
association capture. The native contribution accumulator, portable segment
contract and canonical kind-11 writer are qualified separately below. A bounded
history reducer also verifies retained lineage, effective contribution context,
measured lifecycle boundaries and historical exposure intervals. A canonical
source contract binds selected kind-9/10/11 values to a cursor checkpoint and
rolling digest. Migration 0063 persists these private inputs with the existing
generation cursor transaction and required identity reservation. Migration 0064
adds atomic publication of these observations with dated identity attribution,
independent loss coverage and bounded reports. Expanded context/control/prevention/
faction sources, typed outcomes and the complete balance suites remain under
implementation.
[Implementation status](IMPLEMENTATION_STATUS.md) retains the full accepted scope
and the actual personal-local gameplay qualification requirement.

## Association and actor identity

A battle starts on a typed hostile interaction involving an observed player or
player-owned actor. An ordinary NPC-only interaction cannot start one. Subsequent
hostile observations connect actual participants. Effective support can attach
a helper to an already observed, active target; a participating healer does not
attach an otherwise unobserved bystander. A group-presence observation requires
a matching formal generation/revision and known zone. Its native adapter must
also establish actual group presence at the source boundary; matching a zone or
group key alone is insufficient to call that adapter.

The full battle ID is producer boot/process plus a monotonically allocated
sequence. Character actors use positive signed PIDs. NPCs and pets require a
nonzero producer-scoped live generation with bit 63 set. Prototype IDs are
rejected. A pet gaining or losing its owner retains its live actor generation,
so an ownership change cannot create a second copy of that actor. Logical
sessions can retain an earlier producer through copyover; a supplied current
encounter must belong to the observing producer. Missing session/encounter
context remains explicitly absent.

Neither proximity nor matching group/owner tokens automatically merges separate
battles. A typed hostile, support or proven group-presence relationship can
bridge two active components. The earlier allocated battle becomes canonical;
one immutable merge-alias fact identifies the retired component. Retained actor
effort transfers once, and later terminal summaries belong to the canonical
battle. A request to close the retired ID returns unavailable. Pre-bridge
association uncertainty is retained as `CONTEXT_UNKNOWN`; a merge is not proof
that the earlier components were one complete fight throughout their histories.

## Native source values

`telemetry_runtime_game_battle_actor` snapshots actual character context. For
NPCs and pets it uses the server's existing `runtime_id`, whose maintained
allocator supplies a fresh lifetime when `clear_char` prepares reused storage.
The high-bit tag occupies a disjoint namespace from positive character PIDs;
zero and runtime IDs at or above bit 63 are refused instead of truncated. Two
instances of one prototype therefore remain distinct. Changing pet ownership
retains the same live actor ID. An invalid current PC-owner link is refused
instead of being classified as an ordinary PvE NPC. Existing kind-8 actor IDs
retain their earlier meaning; the adapter does not reinterpret those records.

The native formal-group hook advances a 16-bit roster revision once per
accepted mutation, before roster and context callbacks. The first observation
of a lifetime has revision 1. Appointment and leader departure retain its
generation; both allocation sites clear reused revision metadata. Exhaustion
sets revision 0 permanently for that lifetime. Shared context then supplies an
unknown group relationship with `CONTEXT_OVERFLOW`; reading context cannot
restart its revision. A fresh observing producer allocates a new lifetime.

A supplied PC session must match admitted, unclosed runtime session state.
Linkdead retains that logical session; a closed or fabricated native tuple
does not supply a link. An encounter link requires the PC to be active in an
observed current-producer encounter. Missing optional links remain absent with
unknown context, and NPCs/pets inherit neither a PC session nor an owner's
encounter. The context API allocates no authenticated session or participation.

`telemetry_runtime_game_battle_group_presence` requires two distinct actors,
one exact live formal head, one membership each in its bounded terminating
list, one known shared room/zone and one current nonzero roster revision.
Matching the zone and party while occupying different rooms fails the adapter.
Malformed or cyclic lists supply no group relationship. The pure engine still
requires the source to be an already active battle actor. Native value fixtures
fed proven presence into that engine and kept presence-only effort separate
from contributor time and authenticated session coverage.

## Runtime capture and configuration boundaries

The runtime owns the bounded association state and sends typed facts through
the existing queue and worker. Capture performs no SQL or file I/O. Accepted
combat entry, positive actual damage and the existing positive typed control
callback observe hostile relationships. Healing attaches support only when its
effective amount is positive and no greater than the attempted amount. The
legacy kind-8 amounts retain their meaning. The separate native kind-11 capture
described below supplies disjoint shared damage, healing, casting and observed
opponent-link measurements alongside these relationship facts.

Every qualified hostile/support observation can establish actual same-room formal
party presence, including a member arriving while a previously observed hostile
edge continues. Presence alone supplies neither contribution nor an authenticated
session. Ordinary NPC-only combat cannot start a battle. Owned pet interactions
use their maintained actor lifetime and actual current owner rather than a mob
prototype or an invented owner's session.

Accepted room removal ends observed presence before `NOWHERE`; an anchored dummy,
explicit room veto, failed unlink or duplicate removal supplies no departure.
Arrival context updates an already known actor without reactivating it; a later
qualified observation can rejoin it. Group changes cut known contexts after the
single accepted roster revision. Death/flee, extraction and session exit remove
the observed actor; session exit shares its exact native observation clock.
Copyover/shutdown emit censored closures and never infer a winner. Runtime
inactivity uses a retained 30-second grace, closes at the last actual observation,
and excludes the silent grace period from effort.

An admitted configuration change retains battle IDs, roster and cumulative effort.
It seals the preceding mode/context and checkpoints every retained actor in bounded
pairs with final cuts; cumulative effort before each cut belongs to the preceding
context. Environment/season changes require an owning lifecycle boundary. A
failed effective-property reload checkpoints the known prefix and suspends
relationship/context capture. Existing actor teardown remains available, and
the gap accumulates unknown mode until a qualified configuration resumes.
Configuration cuts never refresh the hostile-activity clock. Callback loss or
mutation-capacity refusal stays latched on later facts and cannot restore
qualified side coverage or erase the unknown gap.

## Sides, mode and exposure

The classifier uses observed hostile constraints as opposite-side evidence and
support/formal-group/current ownership constraints as same-side evidence within
an already connected battle. It considers active observed actors. A consistent
connected two-color graph has `qualified_observed_graph` status. Contradictory
constraints, including a three-way hostile cycle or hostile members of the same
known party, produce `ambiguous`. Disconnected active components, mismatched
roster revisions, loss, clock discontinuity and capacity/context overflow produce
`partial`. Neither status assigns invented two-side identities. This is an
observed relationship graph, not a complete racewar coalition census.

Context, roster and new relationship changes seal the preceding measured segment.
Current active hostile edges supply PvE, PvP, mixed or unknown mode. Existing
actor contexts retain class, race, faction, level band, zone, formal group and
versioned links; the compact power field remains a level proxy. Equipment strength,
arena/training classifications and additional reviewed context still need native
source adapters.

Each actor accumulates presence separately from contributor effort. Group presence
alone does not establish a contribution. PvE/PvP/mixed/unknown mode durations
partition measured presence. Outnumbered exposure uses distinct observed owner
characters per qualified side in PvP mode, keeping pets separate as actors while
deduplicating their owner character. Owner characters are not human controllers,
and a pet owner is not given a synthetic authenticated session or PC presence.
Unknown-side exposure remains separately counted. These amounts cannot replace
input-derived activity, reward-credit membership or verified-controller unions.

Leave/rejoin observations retain the battle within its grace window. A known
actor can chase across zones without replacing the shared ID. Changed formal
membership deactivates a presence-only member until a new qualifying observation;
actual contributors retain their independently observed relationship history.

The caller supplies an explicit inactivity grace; facts retain its value and the
last engagement clock. Inactivity closes with a censored end at the last actual
observation. The later decision time is retained separately and silent grace time
adds no measured effort. Copyover/shutdown also remain unresolved battle closures.
A producer-wide backward monotonic clock is refused and marks retained graphs
partial. No close path declares a winner, and no death or kill is promoted into
a whole-battle victory. Reviewed death/escape/objective evidence remains part of
the native integration requirement.

## Portable facts, packets and replay

The shared value types live in `telemetry_types.h` independently of the
association engine. `telemetry_battle_contract.h` supplies exact-length numeric
encoding, named-field replay equality and complete-packet validation. The
definition-1 descriptor in `telemetry_battle_fields.inc` declares 70 ordered,
typed fields. Their canonical representation is 366 bytes in network byte order,
excluding ABI padding and reserved bytes. Signed unknown UTC and PID sentinels
retain their meanings. Reserved actor bytes must be zero before encoding.
The Python reader's field layout is checked against the same descriptor and a
retained semantic digest; widths/order/signedness cannot drift silently.

Intrinsic validation refuses invalid producer/lifetime identities, partial
optional session/encounter keys, foreign current-encounter producers, fabricated
formal revisions, unsupported definitions/enums, incorrect cardinalities, reversed
clocks and nonconserved effort. Mode durations partition presence; contribution,
unknown-side and outnumbered exposure cannot exceed their covered denominators.
Queue/clock/cardinality/context loss cannot claim qualified sides. Terminal
facts require explicit censored closure and the actual observed boundary, so
inactivity grace remains separate from measured time.

The logical fact key is the full battle producer/sequence plus fact sequence.
It has a distinct domain from both legacy encounter identity and the enclosing
transport's admitted replay key. Identical named values compare identically
despite C++ padding; a changed canonical value under the same key conflicts.
The kind-10 writer enforces both identities in durable storage. The original
transport key can replay identical named values. A different transport key for
the same logical fact conflicts, so it cannot claim an unretained receipt.

A complete normal packet ends in one final cut. Initial packets additionally
prove one distinct hostile pair and consistent initial mode/owner counts.
Contexts precede the relationship, repeated context identities are refused and
an alias requires a real bridge relation. Terminal packets contain one unique
summary per retained actor plus their final close, with checked active-actor and
distinct owner-character counts. Every packet shares its scope, revision, clocks
and counts, and has contiguous ordered fact sequences/ordinals. Quality can grow
after a sink failure within one mutation; requiring uniform side status would
hide that loss. Quality cannot disappear before the final cut.

The native packet receiver owns at most 65 facts in a fixed buffer below 32 KiB;
the Python receiver uses the same explicit 65-fact bound. They accept reordered
frames and identical retries, preserve pending packets when another revision
arrives, and latch conflicts until explicit reset. Missing initial frames, middle
frames or the final cut never become complete. Input and returned Python values
are copied, preventing later mutation from rewriting an accepted packet.

Packet completeness is one prerequisite. Cross-packet gaps, retired aliases,
source/contribution linkage, dated identity attribution, outcome evidence and
incident coverage still need the durable capture/publication integration. A
complete later terminal packet does not repair an earlier missing mutation or
establish a winner. Existing persistent kinds 1–9 and report definitions 1–3
retain their meanings. The additive storage integration below preserves every
earlier sealed migration.

## Sealed contribution segments

The pure `telemetry_battle_contribution` module aggregates exact source/recipient
amounts in disjoint segments. Each segment identifies its battle, maintained
live actor and owner, optional actual session/encounter, configuration and native
context, mode/side basis, and first/last association revision and fact sequence.
The owning collector must supply the exact effective association packet/context;
positive references alone do not prove a complete or admitted packet. An
unchanged receipt cannot authorize a changed context.

The domain replay identity is `(battle producer boot/process, segment sequence)`.
The sequence is allocated once across all retained battles and is independent of
the later transport receipt. Actor/battle IDs are attribution, rather than
segment deduplication keys. A changed owner, group revision, mode, configuration
or battle ID seals the preceding amounts before a fresh segment starts at zero.
No contribution totals transfer between alias IDs. Publication must resolve the
immutable battle aliases and count each retained segment exactly once.

Damage increments source dealt and recipient taken once. Healing retains
attempted, effective, overhealing and recipient effective healing separately;
attempted equals effective plus overhealing unless an explicitly flagged counter
saturates. Control retains outgoing applications and recipient applications.
Casting retains actual completions and actual aborts; an unfinished cast at a
context/close boundary contributes one unresolved attempt and `UNCLOSED_TAIL`.
It cannot become an invented abort or completion. Observed opponent-link time
is named `engaged_target_usec`; it establishes neither incoming pressure,
tanking, nor prevented damage. A segment's `battle_ended` boundary establishes no
winner, objective or whole-battle victory.

The availability mask names reviewed producer families: damage, healing,
control, casting and engagement. An unset family's zero counters mean unknown.
Availability does not establish complete historical coverage, and an absent
actor stream does not prove a measured zero. Positive paired events preflight
both actors and sequence/slot capacity before changing either counter. Both old
segments close before either replacement starts, including a full 128-slot
transition whose only reusable slot belongs to the target. Refused capacity,
saturation, backward monotonic clocks, and rejected sink rows retain explicit
quality. A failed finalized row is released rather than copied into a later
segment; source continuity loss remains latched conservatively.

A later decision clock may close an earlier observed prefix. Casting and
engagement duration stop at the supplied observation, which must cover all
retained actual actor observations; grace or decision latency adds no time.
Signed UTC occurrence labels retain the existing `INT64_MIN` unknown sentinel,
real zero/negative epoch labels and explicit clock-reversal quality. A UTC
reversal or unavailable clock during the segment remains visible even if the
final UTC label recovers.

The module retains 128 battles and 64 metric actors per battle in 3,674,176
bytes, guarded by a separate 4 MiB compile-time budget. A payload occupies 400
bytes; its 65 named fields have an exact 385-byte network-order encoding with
no ABI padding or reserved bytes. The independent Python contract and native
decoder enforce the same widths and intrinsic invariants. These are module
allocation bounds, not a measured server allocation or game-loop/writer
performance result.

```sh
python3 tests/async/test_telemetry_battle_contributions.py
python3 tests/async/test_telemetry_battle_contributions.py --sanitize
python3 tests/async/test_telemetry_battle_contribution_contract.py
```

Ten executable journeys qualify counter conservation, actual complete association
packets and aliases, PvP-to-mixed mode and configuration changes, changing
pet owners/kinds, distinct NPC lifetimes, unresolved casting, observed-time
censoring, backward clock/reference refusal, 64-actor/128-battle and sequence
limits, saturation, sink failure, and allocation-free event/codec paths. Nine
cross-language regressions round-trip every actual sealed field and qualify
immutable layout, strict types/widths/lengths, semantic corruption, signed clocks,
unknown metrics and logical keys. Normal and ASan/UBSan executions passed.

The runtime now owns this accumulator and connects it to authoritative gameplay
and association boundaries, as described below. Its kind-11 durable family
preserves the independent segment identity. Earlier record kinds, kind-8 encounter
summaries, kind-10 association storage and report definitions retain their
contracts. Atomic complete battle reports still require source/linkage coverage;
timestamp joins or prototype-based conversion of legacy kind-8 totals cannot
establish that coverage. Actual personal-server journeys and performance
qualification remain required.

## Durable shared-battle storage

Migration `0061_telemetry_shared_battle_facts` activates the independent kind-10
record family and maps all 70 canonical numeric fields to nullable typed columns
in `telemetry_interval`. A battle fact requires every field; another family
requires those columns to be NULL. Numeric unknown UTC/PID sentinels remain
numeric values within the present battle payload. The transport producer and
occurrence must match the battle fact's producer and occurrence. The writer also
qualifies the captured environment, season, configuration and classifier/policy
versions against the immutable configuration projection.

The new unique key is `(battle_boot_id,battle_process_id,battle_seq,battle_fact_sequence)`.
The existing transport key stays independent. Individual facts are append-only;
an accepted frame does not prove that its packet, alias history or battle is
complete. Storage supplies no synthetic authenticated session or legacy encounter
projection from an optional actor reference. The public tagged record remains
bounded by 512 bytes, and schema startup validates the new columns and unique
index through the canonical descriptor.

The native SQL journey generates actual pure-module packets with hostile PCs,
support, a pet, changed group/zone context and a censored copyover close. It
verifies every typed field, signed unknown UTC, lost commit acknowledgements,
exact/conflicting retries, scope/header refusals, protected pending values and
exact quarantine evidence after an injected record-specific SQL failure. Both
MariaDB 10.11.14 and MySQL 8.0.46 passed all ten native record families, the frozen
golden fixtures and startup/effective-permission regressions.

The separate storage qualification applies all 61 immutable steps, checks direct
SQL constraints and logical uniqueness, and runs earlier definitions 1/2/3 over
the mixed stream. These definitions validate battle facts and advance their
cursor without adding battle time, XP or other amounts. Malformed facts and byte
budget failures prevent acknowledgement. Guarded reruns preserve retained facts;
column/default and incident-mask drift is refused and never silently repaired.
Exact fresh/restored schema fingerprints, boot constants and the retained
lifecycle inventory are synchronized for both engines.

```sh
python3 tests/async/test_telemetry_battle_contract.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-storage
```

Outage ledger v2 and independent incident review schema v3 include kind 10; older
histories retain their family limits. See [OUTAGE_STORAGE.md](OUTAGE_STORAGE.md)
and [INCIDENT_COVERAGE.md](INCIDENT_COVERAGE.md). Compact
context/control/prevention/population sources, exact shared contribution linkage,
bounded atomic projections and the complete personal-local gameplay/performance
gate remain required. This layer alone publishes no battle balance result.

## Durable contribution storage

Migration `0062_telemetry_battle_contributions` adds record kind 11 with all 65
definition-1 contribution fields in canonical typed nullable columns. Each kind-11
row requires the complete payload; other record families require those columns
to be NULL. The existing transport receipt remains the delivery identity. The
separate unique `(bc_battle_boot_id,bc_battle_process_id,bc_segment_seq)` key
prevents a second receipt from inventing another retained segment. Battle/actor
aliases never change that key. Exact retries use the original receipt and fields;
a conflicting retry remains a conflict.

The header producer must match the battle producer. Its occurrence label is the
segment's decision UTC; the start, observed prefix and decision clocks remain
separate fields. Optional session/encounter links do not create legacy session
or encounter projections. Native representation validation, immutable
configuration qualification and SQL checks retain the context, association,
quality, metric availability, actor identity and counter partitions. SQL uses
decimal intermediates for counter sums to preserve unsigned 64-bit saturation
boundaries without overflow. Reserved representation bytes remain part of
uncertain-retry quarantine identity. Inactive union bytes and ABI padding do not.

The complete manifest now has 62 steps and the runtime inventory has 250 database
tables. Earlier sealed migrations remain byte-identical. The new verifier checks
all column widths/defaults/order, the logical replay index, exact payload checks
and independent private incident schema v4. Guarded reruns preserve retained
facts/reviews and refuse existing schema drift instead of repairing it silently.

```sh
python3 tests/async/test_telemetry_battle_contribution_contract.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --contribution-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --contribution-storage
```

The native repository fixture generates actual disjoint segments with damage,
healing, control, casting, engagement and a changed zone/association context. It
checks every field through SQL, conserved totals, unresolved casting, signed
clocks, exact/conflicting retries, lost acknowledgements, scope/header refusals,
NULL family separation and quarantine evidence. The separate full-chain storage
fixture covers actual emitted pet/NPC, saturated counter, clock uncertainty and
source-gap rows, unavailable metrics, direct CHECK refusals, maintained review CLI,
private permissions, retained corrections, atomic review rollback, immutable
snapshot retries and fresh/restored schema fingerprints. Definitions 1/2/3
strictly validate kind 11 and advance their cursor without adding its amounts.

Outage ledger v3 and private incident schema v4 include contribution loss;
earlier versions retain their original family limits. The common coverage
snapshot seam selects v4 for future definitions beginning with 5 and keeps
definition 4 on v3. The current report catalog still contains definitions 1/2/3.
Neither this storage nor that seam publishes a battle report. The native runtime
capture below retains exact packet references; complete published association/
alias/loss linkage, context/prevention/population,
atomic battle projections and actual personal-local gameplay/performance
qualification remain required under the full expansion in #258.

## Native contribution capture

The runtime owns the fixed 3,674,176-byte contribution state and emits sealed
kind-11 records through the existing queue, worker and canonical writer. Native
damage, effective/attempted healing, casting and actual `GET_OPPONENT` observations
use maintained live actor IDs and the current shared battle's scope, actor/owner,
group revision, mode, side and complete-packet reference. Each combat observation
uses one monotonic/UTC pair for relationship capture, formal party presence and
its metric update. Legacy kind-8 amounts are independently preserved.

The initial native delivery used availability mask **27**: damage (1), healing
(2), casting (8) and observed opponent-link time (16). The accepted blindness and
stun producers described below add control (4), so new native segments use mask
**31**. Older segments retain their original mask and unavailable control remains
NULL. Prevention and measured incoming pressure remain required sources. An
availability bit identifies implemented producer families; it does not prove that
every effect, lifecycle or actor was observed. A zero describes the observed
segment from those producers, and an absent stream cannot establish a measured zero.

Positive effective healing can admit useful support. Ineffective attempted
healing counts only when both actual actors already belong to the same battle;
it cannot admit an unrelated healer. Self-healing retains one exact attempted/
effective/overhealing partition. Proven formal presence alone creates no metric
stream. Pets retain their own maintained lifetime, current owner and event
modifiers; teardown uses the retained stream kind even if owner context is
unavailable by the time the pet leaves.

After an association mutation, affected retained streams observe the committed
packet basis. Changes of battle/alias, mode/side, owner/kind, configuration,
dimensions or group revision seal old amounts before further activity can start
a replacement. A pending cast at that boundary becomes unresolved in its old
segment. A later terminal callback cannot relabel that old attempt or copy its
elapsed time. Explicit leave uses its actual cut, including the same source
clock as session exit. A changed context without a newer usable packet basis
retains an explicit source-gap prefix. Configuration withdrawal also seals
source-gap prefixes; recovery preserves the association's partial coverage.
An actual opponent pointer without a usable native lifetime seals a source gap
and keeps missing coverage explicit; it cannot end engagement as a measured zero.

Real casting/opponent context observations can extend an active battle's last
observed prefix without refreshing its hostile clock or publishing a duplicate
packet. Inactive actors cannot extend that prefix. Observations in another battle
cannot advance a retained stream's measured time. The collector's close callback
seals metrics before its association slot is released. Inactivity supplies the
original observed prefix and a later decision; copyover/shutdown supply their
actual observed cut. The kind-11 header labels the decision UTC. These boundaries
do not establish a whole-battle winner.

The gameplay fixture checks conserved damage, exact healing partitions, terminal
casting retries, unresolved casts, both sides, party arrival/departure, different
NPC generations, owner/kind changes, configuration changes, aliases, retained
pet teardown and exact session exit. A second native fixture processes two
independent inactivity closures at one supplied future pulse clock and checks
both earlier observed prefixes. That future pulse is a fixture seam, rather than
a running personal-server clock journey. The SQL fixture compares all 65 emitted
fields, validates complete first/last association packet references and checks
private writer permissions, NULL family separation and empty quarantine.

Both disposable full-62-step MariaDB 10.11.14 and MySQL 8.0.46 journeys passed:
123 shared facts in 38 complete packets and 28 native contribution segments per
engine. Damage dealt/taken both equal 112. Attempted healing 45 partitions into
effective 15 and overhealing 30; six casting attempts partition into one
completion, one abort and four unresolved attempts. All 65 contribution fields
and both packet references match the emitted source. Fresh metadata fingerprints
retain their sealed head-62 values. The temporary databases/roles were removed
and both fixture engines were stopped. These journeys use synthetic gameplay
objects and a private connection factory seam; real personal-server source,
persistence and performance journeys remain required.
The unavailable-opponent fixture verifies an actual native source-gap segment
through the writer and preserves the known amounts and unresolved cast.

Each production copyover flush retains its 250 ms cap. The SQL correctness fixture
permits bounded generation retries within five seconds and requires unchanged
source totals and one censored close. This is a correctness check; the final
personal-local performance gate still needs measured latency and memory budgets.

```sh
python3 tests/async/test_telemetry_gameplay_adapters.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_battles.py --sanitize
python3 tests/async/test_telemetry_runtime_integration.py
python3 tests/async/test_telemetry_runtime_exhaustion.py
python3 tests/async/test_telemetry_runtime_outage.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
```

## Bounds and qualification

The state retains 128 battles, 64 actors per battle, a 64-entry terminal cache,
and 4,096 normal mutation facts per battle. A normal relationship mutation emits at most five facts;
an unchanged relationship updates clocks without rows or graph reconstruction.
Cuts retain revision, fact ordinal and expected count. A consumer must verify
the complete packet and its terminal cut before treating that revision as
complete. A sink refusal preserves loss quality and partial graph status.
Mutation-budget overflow emits one partial cut; terminal summaries remain bounded
and available for a best-effort close. Actor/merge capacity refusal preserves
the existing components and makes their results partial. Sequence exhaustion
refuses another identity.

A configuration boundary checkpoints actor pairs in packets of at most three
facts. A full 64-actor battle needs at most 96 facts; all 128 retained battles
need at most 12,288 facts in one boundary operation. They consume the existing
per-battle mutation budget and queue admission. Capacity is unchanged, and a
refused frame or exceeded budget leaves explicit partial coverage.

The qualified build measures 1,719,392 bytes for the complete fixed state and
392 bytes for a fact. Compile-time guards cap the state at 2 MiB and keep a fact
plus the existing record header within 512 bytes. Allocation traps cover the
observation and close paths; the module owns no dynamic containers, character
pointers, SQL, strings or I/O. These bounds do not establish actual server/writer
performance or representative racewar capacity.

```sh
python3 tests/async/test_telemetry_battles.py
python3 tests/async/test_telemetry_battles.py --sanitize
python3 tests/async/test_telemetry_battle_contract.py
python3 tests/async/test_telemetry_contract_headers.py
python3 tests/async/test_telemetry_group_hooks.py
python3 tests/async/test_telemetry_combat_hooks.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_room_hooks.py
python3 tests/async/test_telemetry_runtime_integration.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
make -C src
```

The pure-module fixtures qualified duels, reinforcement, healer-only participation,
proven group presence, membership changes, leave/rejoin, zone chase, changing
modes, pets/owner loss, distinct NPC generations, retained aliases, exact
64-actor merges, oversized merge refusal, roster ambiguity, unknown clocks,
inactivity bounds, inline expiry accounting, callback loss, row/actor/slot caps,
and sequence exhaustion. Complete packets and duration conservation passed;
normal, AddressSanitizer and UndefinedBehaviorSanitizer executions passed.

Twelve additional contract regressions exercise the actual native fixtures through
the independent Python parser and back into native verification. They qualify
exact numeric round trips, immutable layout, every retained real mutation packet,
accepted-frame loss, reordered/identical/conflicting replay, all 65 terminal
ordinals, changing in-packet quality, semantic/packet corruption, width/type/
length refusals, unknown clocks, NPC/pet attribution and copied receiver values.
Standalone public headers retain C++20 compilation and the earlier golden
record fixtures. Allocation traps include the native codec and packet receiver.

The live gameplay-adapter fixture executes the actual runtime, queue, worker and
native writer. Each disposable full-61-step MariaDB 10.11.14 and MySQL 8.0.46
chain committed 80 battle facts in 27 complete packets. Every one of the 70
canonical values matched SQL readback; inactive-family fields remained NULL,
configuration references matched, and quarantine remained empty. The dedicated
writer could not read gameplay accounts, delete gameplay rows or update raw/
quarantine facts. The gameplay objects and private connection factory are test
seams, and authenticated identity stays unknown. This proof starts no game
server and does not replace the final real personal-server journey or measured
gameplay/performance qualification.

The next integration gate uses the qualified history reducer below in retained
source and atomic publication transactions, completes compact context, control/
prevention and faction exposure sources, and adds dated identity and typed outcome
evidence. Existing kinds 1–9, sealed migrations and report
definitions retain their meanings. The four full balance suites and actual
personal-local gameplay/persistence/performance gate remain required by issue #258.

## Bounded history qualification for publication

`scripts/telemetry/battle_history.py` consumes one explicit environment/season
window of canonical kind-10/11 values. It reuses the sealed family and packet
validators. SQL arrival order can change; the immutable producer transport order
and logical fact/segment identities cannot. Identical receipt retries count once.
Conflicting transport values, a second receipt for one logical identity, packet
transport reversal and source-quality regression refuse the reduction.

Completeness includes the initial packet, contiguous revisions and fact sequences,
all declared ordinals and the final cut. Missing initial, middle, terminal or
whole-mutation evidence remains partial even when a later complete summary is
present. The reducer reconstructs bounded actor/relationship state, verifies
declared rosters, owner counts, conservative sides and modes where the source
permits, and checks absolute cumulative effort against the observed clock cuts.
Loss and capacity flags prevent unsupported replay verification. Source packet
integrity, verified replay of an observed graph and full population coverage are
different properties.

A complete alias packet must bridge actors from its two retained components.
An unrelated relationship cannot authorize a merge. Retired components retain
their original keys and source coverage. Their measured contribution segments
resolve once to the canonical battle; inherited absolute actor effort replaces
the preceding observation. Alias rows supply lineage and do not add another copy
of canonical amounts. Missing donor or alias evidence stays uncertain.

Each contribution's first and last references resolve to complete cuts from its
actual battle and producer. Verified linkage checks effective actor identity/kind,
ownership, optional session/encounter, all captured dimensions, formal roster,
configuration versions, mode, side and context quality. It also checks history
between the references: matching endpoints cannot hide an intervening changed
context. Context-change and leave segments need observed boundary evidence;
battle-ended segments need the actual complete close with its exact observed and
decision clocks. Missing lifecycle evidence remains unlinked. A measured prefix
cannot extend beyond an alias retirement or into inactivity grace. Overlapping
segments for one producer actor refuse reduction.

Measured amounts remain separate from their linkage and coverage. An unavailable
family has NULL aggregate metrics; an available family's sum describes the
observed segments and does not prove complete coverage or a missing actor's zero.
Actor effort outputs are the latest absolute cumulative values for a canonical
battle actor. Their latest context is not an attribution of all historical effort
to that class, faction, group or configuration. Historical exposure intervals are
qualified below; dated identity/day/report cells still need the publication
integration. This reducer establishes neither an account/
controller association nor a decisive battle outcome. Empty input does not imply
zero live activity.

The reviewed incident input uses independent schema 4. Family masks, matching
producer/sequence bounds, occurrence windows and the reviewed time range retain
relevant loss or unknown coverage. Unknown or reversed UTC labels retain measured
monotonic amounts and cannot use occurrence time to exclude a possible incident.
Reconstruction or a later packet does not manufacture missing source values.

Hard limits are 16,384 inputs, 512 battle identities, 4,096 packets, 2,048 referenced
snapshots and 64 actors per merged state. Explicit output-row, byte and deadline
budgets fail before returning a result. The internal reservation accounts 8,192
bytes per normalized input, 2,048 per retained actor/trace and 4,096 per output;
the default is 32 MiB. A caller must separately reserve buffering SQL fetches and
its complete invocation. The original reducer-only native fixture reserved
2,230,272 bytes internally. Historical exposure adds the output reservations
below. These are reservation accounts, not measured Python heap or server budgets.

Twenty focused tests passed, including 6,468 emitted facts from 23 existing pure
association journeys. Normal and ASan/UBSan native gameplay exports reduced 123
facts in 38 complete packets and 28 contribution segments into five canonical
battles with all 28 context/lifecycle links verified. Observed damage remains
112 dealt/112 taken and the exact healing/casting partitions remain conserved.
The actual private-writer SQL readback fixture invokes the same reducer on both
disposable engines. Sealed metadata fingerprints and migration head 62 remain
unchanged. These are native/SQL correctness proofs with gameplay object and
connection-factory seams, not a running personal-server journey.

```sh
python3 tests/async/test_telemetry_battle_history.py
python3 tests/async/test_telemetry_battle_contract.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
```

This is the qualified reducer for the next publication step. It writes no SQL,
adds no migration and enables no report definition. The catalog remains 1/2/3.
Retained source/cursor digests, atomic public stores, reserved dated identity,
complete published loss/lineage coverage, remaining native evidence, the four
balance suites and the real personal-local qualification remain required.

## Historical exposure and retained source checkpoint

The reducer's `exposures` values seal positive observed intervals under the actor
context, roles, configuration, battle mode, conservative side and observed owner
counts that applied during that interval. All eight effort fields are separate
interval amounts: present, contributor, PvE, PvP, mixed, unknown mode,
outnumbered owner and unknown side. They reconcile exactly to native cumulative
actor effort in the fully replay-verified fixture. Presence-only actors contribute
measured presence without acquiring a damage/healing/casting stream. Unknown
sides retain NULL side-owner denominators. These counts identify observed character
owners, not accounts or confirmed human controllers.

Original source battle keys and complete start/through association references
remain attached to each interval. An alias resolves these prefixes to one
canonical battle without copying the retired intervals. The actual observed
monotonic/UTC cut seals inactivity; the later closure decision supplies no extra
effort. A zero-duration cut emits no exposure row. Missing whole/middle packet
evidence stops further verified exposure and retains earlier known prefixes;
later cumulative snapshots cannot fill the missing interval. Overlapping exposure
for one actor in one producer incarnation refuses reduction, including
association-only streams with no contribution records.

An independent digest commits the observed active actor context/roles/sides and
active hostile/support/presence relationships. Its domain is
`duris-battle-exposure-roster-v1:`. Numeric actor values use fixed signed 9-byte
encodings in the sealed `ACTOR_VALUES` order; roles and side follow each actor.
Relation records use the relation byte and two unsigned 8-byte actor identities.
Actor/relationship sorting and record tags make the digest deterministic. The
digest is observed roster evidence, not a reviewed team or controller identity.
Adjacent intervals coalesce only when this digest, all captured actor/scope
context, roles/sides, owner counts and clock quality match. An opponent's class
change splits the unchanged actor's exposure as well. Coalescing preserves the
original start and final actual through reference and cannot hide a composition
or coverage change.

UTC unknown, backward or inconsistent labels preserve measured monotonic amounts
and cannot establish a dated comparison. Independent incident-schema-4 coverage
uses kind 10 and the interval's matching producer/transport range. Contribution
loss uses kind 11 independently. The output-row/byte/deadline limits cover these
intervals and their coalesced values. Stress fixtures explicitly permit 10,000
output rows with a 128 MiB reservation; the default remains 2,000 rows/32 MiB.
Source clocks and zero-duration cuts make the native row count and reservation
vary between executions. This variation changes neither conserved effort nor
the source identities.

`scripts/telemetry/battle_source.py` supplies the exact input/checkpoint contract
for the following atomic SQL integration. Each retained row pins the generation,
ingestion ID, producer receipt, family, canonical numeric payload and SHA-256
payload digest. The payload preserves kind-9 ownership observations, all sealed
kind-10/11 values, the original occurrence label and the independent SQL arrival
label when available. Export-only input has an explicit unknown arrival label;
it does not borrow occurrence UTC. Typed projection quality remains separate
from the original fields. Decoder/type/exact-column/canonical-JSON checks refuse
conflicting envelopes, duplicate JSON keys, unexpected fields and changed values.

The generation seed uses `duris-battle-source-v2:`, its exact four-part scope and
the original ingestion boundary encoded as an unsigned eight-byte value.
Every selected ingestion ID and payload digest advances the rolling source
digest. Family counts conserve the selected total, and verification requires the
exact expected scope, original boundary and watermark from the locked generation
state. Every retained input must follow that original boundary. Cursor advancement
over unselected families adds no fact. Missing/reordered/repeated receipts,
changed payloads/counts/digests, scope/checkpoint mismatches and quality regression
refuse the whole source value result. A published checkpoint cannot advance.
Identity registry authority, token issuance and reviewed incident snapshots must
be validated by the existing restricted transaction path. This codec supplies no
review authority or identity attribution.

Limits are 16,384 selected inputs and 8,192 payload bytes. Encoded input buffering
reserves 12,288 bytes per row. Source verification reserves 32,768 bytes per input
plus a 4,096-byte header before returning decoded values; its default is 32 MiB.
The native 151-input window reserves 4,952,064 bytes for this stage. The SQL caller
must reserve its buffering fetch, history reduction, review/incident evidence,
dated exposure cells and complete publication invocation separately. A capacity
or deadline refusal returns no source result and mutates no caller values.

At the retained-input contract stage, definition 5 names the independent incident-schema-4
reservation seam. Those pure helpers did not enable definition 5: the catalog at
that stage remained 1/2/3, and no SQL source checkpoint or public battle table is created by these pure
helpers. Thirty-nine focused regressions qualify context/composition changes,
alias/inactivity/missing-history cuts, all eight effort counters, unknown clocks/
sides, source receipt/digest/cursor negatives, kind-9 value retention and budgets.
The native and private-writer readback fixtures invoke the same source and
exposure checks. Persisted source/checkpoint atomicity, reserved dated identity,
independent reviewed loss snapshots, publication/report transactions and the full
accepted remaining scope stay required under #258.

The final MySQL 8.0.46 readback used an owned, auto-removed disposable fixture
with `mysqld --skip-innodb-use-native-aio`: the host's shared kernel AIO quota
prevented the default fixture from starting. MariaDB 10.11.14 used its normal
disposable setup. Both full 62-step chains and actual private-writer readbacks
passed without changing sealed metadata. This fixture setting qualifies SQL
correctness and supplies no default-engine or real-server performance proof.

## Persisted battle source preparation

Migration `0063_telemetry_battle_source` adds private `telemetry_battle_source`
and `telemetry_battle_input` tables. The header retains the selected count,
independent ownership/association/contribution counts, scoped rolling digest,
original ingestion boundary, cursor and quality. The original boundary is also
part of the digest seed and must match the state's immutable rebuild boundary.
Inputs preserve the exact canonical kind-9/10/11 payload,
original ingestion identity, producer receipt and separate occurrence/arrival
labels. The input key and scoped unique producer receipt prevent a repeated
source from becoming another retained fact. Neither table depends on a surviving
raw fact: retained evidence remains available after raw retention.

The header requires both the rollup state and immutable generation identity
reservation through restrictive foreign keys. An explicit unknown identity
reservation is valid preparation; it attributes no account/controller effort.
The actual page transaction inserts selected source, updates counts/digest/header
and advances the state cursor together. Failure after source writes but before
cursor update rolls everything back. The existing advisory lock, fixed keyset
bound, deadline and lost-acknowledgement reconciliation remain the execution
path. Different generations keep independent source keys and scoped digests.

At migration 0063's delivery, definition **5 supplied source preparation**. `RollupTarget` and the existing
`run` command accept it; report definitions and the catalog remain **1/2/3**.
Selected kind-9/10/11 facts receive their existing strict family validation and
exact source encoding. Other valid families advance the global cursor without
adding battle source or duplicate playtime/progression rows. Generic capture gaps
retain uncertainty. Source preparation tracks selected occurrence bounds and
late/unknown clock flags independently of SQL arrival and original source values.
At that stage `publish` explicitly refused definition 5 pending its battle projection.
A building source window is not a published balance report or healthy empty
population proof.

`read_battle_source` is a restricted rollup/publication API. It reads state,
header and retained values in one consistent read-only snapshot and releases
the transaction on success or failure. The expected original boundary and cursor,
canonical fields,
payload SHA-256, rolling digest and counts qualify the whole window. It reserves
the encoded buffering fetch **and** decoded verification, including a sentinel,
before selecting rows. Payload selection is capped at 8,193 bytes so an oversized
value is refused rather than silently truncated into a valid payload. The 32 MiB
default is a reservation, not a heap measurement. General report and game-writer
roles receive no access to either private source table; the rollup principal has
SELECT/INSERT on inputs and SELECT/INSERT/UPDATE on the building header.

Forty-five focused history/exposure/source tests passed. A changed original
boundary refuses source verification, including when a caller changes its
expectation to match the changed header. The actual native writer
fixture prepares **153 selected inputs**: two ownership observations, 123
association facts and 28 contribution segments. All original source fields and
arrival labels restore exactly, and the selected battle facts reproduce every
history, actor, contribution and exposure value. An additional synthetic ownership
observation expands the matching scope to 154 inputs; a foreign-scope observation
advances the cursor without adding a fact. This fixture validates ownership source
storage, not a running personal-server authentication journey.

The disposable MySQL 8.0.46 and MariaDB 10.11.14 journeys cover the complete
63-step chain, missing identity reservation, page retry, cursor rollback,
acknowledgement loss before/after commit, missing/changed source and cursor/origin
conflicts, an explicit empty building window with a nonzero original boundary,
input/receipt/foreign-key constraints, bounded reads, raw retention,
maintained CLI preparation/publication refusal, private roles, guarded schema
reruns, weakened payload/kind drift, read-lock release and exact restored metadata.
The MySQL fixture uses the owned native-AIO workaround documented above. No
default-engine or actual game-server performance budget is established here.

```sh
python3 tests/async/test_telemetry_battle_history.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
```

After applying migrations only to an explicit disposable local setup and
preparing its restricted rollup role and identity reservation, the maintained
`scripts/telemetry/rollup.py run --definition-version 5 --generation ...
--environment-id ... --season-id ... --through-ingest-id ...` command prepares
this source window. Existing explicit connection and invocation bounds apply.
Migration 0064 supplies the publication transaction described next. Remaining
native evidence and all seven accepted completion requirements remain open under
#258.

## Atomic battle observation publication

Definition **5** now offers five independent reports through the maintained
`scripts/telemetry/rollup.py` interface. Definitions 1, 2 and 3 preserve their
earlier report meanings. The separate original `report.py` presentation remains
its definition-1 interface.

| Report | Published grain and intended use |
| --- | --- |
| `battle_observations` | Each original battle, its canonical lineage, alias retirement, lifecycle/packet coverage and observed contribution totals. A censoring reason does not establish a winner. |
| `battle_actors` | The latest cumulative actor snapshot for each canonical battle. This is an alternative to historical exposure, so its time must not be added to exposure time. |
| `battle_contributions` | Each original disjoint amount segment, complete association references, canonical battle and available metric families. Exact amounts remain whole when identity changes inside a segment. |
| `battle_exposure` | Positive measured actor intervals cut at observed ownership changes, reviewed linkage boundaries and comparable UTC midnight. Historical class/faction/group/mode/roster context and all eight effort partitions remain exact. |
| `battle_associations` | Every original canonical kind-10 field and producer/ingestion receipt, including ordered packet membership, associations, alias bridges, original clocks and projection quality. Private ownership inputs are excluded. |

The builder consumes the locked retained source window, its original ingestion
boundary/count/digest, the generation's reserved identity version and the
independent schema-4 incident snapshot. Migration 0064 adds
`telemetry_rollup_battle_coverage` and `telemetry_rollup_battle_row`. The existing
publication transaction inserts the coverage header, all public rows and loss
snapshot, marks retained source complete, publishes the generation and supersedes
the earlier generation together. A failure undoes every step. Existing advisory
locking, fixed source bounds, retries, deadlines and statement/socket budgets
remain the execution path. Published source requires a new generation for more
input. Superseded reports retain their own source and reviewed snapshots.

Account attribution requires the same producer lifetime, admitted session,
character PID/owner identity and an observed ownership anchor. A missing session,
NPC or unproven pet owner remains explicit. Controller attribution additionally
requires a confirmed dated association in the reserved reviewed version and
comparable UTC. Unknown UTC can preserve an observed account while leaving the
controller and day unknown. A reviewed ownership-loss end does not itself restore
ownership: a fresh matching observed anchor is required. Unknown incident ends
remain unknown. A correction affects only a new reserved generation.

Exposure partitions conserve native presence and each applicable effort counter.
The header separates PC/non-PC presence, observed/unknown account presence and
confirmed/unlinked controller presence. These are sums of measured actor time;
they do not represent unique controller hours, input activity or continuous human
attention. Amounts are assigned to a known account/controller only when that
identity is uniform over the observed segment. A uniform account can remain known
across changing controller reviews; a controller stays known across consecutive
confirmed association IDs only when it is uniform. No amount is prorated across
an unobserved action boundary. Zero-duration amounts retain unknown attribution.
Older unavailable control counters are public NULLs despite the sealed native
representation's zeros. Availability and partial source coverage remain separate.

Rows use a canonical bounded JSON field/type contract, domain/scoped logical key,
payload SHA-256 and quality mask. A generation snapshot digest covers every
ordered public row receipt. Each report verifies that complete bounded receipt
list before reading its requested projection, so missing or changed receipts
cannot become a valid partial report. Selected payloads are capped at 8,193 bytes
and digest/key selections at one byte beyond their declared widths; an oversized
stored value refuses instead of becoming a truncated valid value. A consistent
read-only transaction covers state, identity reservation, publication, rows and
loss metadata and is released on every outcome.

The builder reserves encoded/decoded source, history, review evidence, temporary
boundary slices and public output before allocating them. Publication has a
4,096-row hard ceiling and the existing 2,000-row default; the default total byte
reservation is 32 MiB. Reports reserve metadata and the complete snapshot receipt
list before selected rows and their sentinel. These are conservative software
reservations, not measured heap or game-loop performance. Capacity/deadline
refusal produces no partial publication.

The restricted rollup role needs SELECT/INSERT on both new publication tables,
the existing public incident snapshot tables and generation reservation;
SELECT/INSERT/UPDATE on state and the private building source header; SELECT/
INSERT on retained inputs; and SELECT on raw telemetry and the selected identity/
schema-4 review inputs. It receives no UPDATE/DELETE on public rows or retained
inputs. The report role needs SELECT only on state, reservation and public battle/
incident stores. It receives no private source, raw account or review-authority
access. Existing protected lifecycle and controller/disclosure decision entries
apply to the new stores.

After migrations and roles have been prepared in an explicit disposable local
setup, reserve a reviewed identity version or explicit unknown identity using
the existing identity API, then run a fixed input window and publish it:

```sh
python3 scripts/telemetry/rollup.py definitions --definition-version 5
python3 scripts/telemetry/rollup.py run --definition-version 5 --generation 1 --environment-id "$environment" --season-id "$season" --through-ingest-id "$through"
python3 scripts/telemetry/rollup.py publish --definition-version 5 --generation 1 --environment-id "$environment" --season-id "$season"
python3 scripts/telemetry/rollup.py report --definition-version 5 --generation 1 --environment-id "$environment" --season-id "$season" --name battle_exposure
```

Use the explicit `TELEMETRY_ROLLUP_DB_*` connection settings documented by the
existing CLI and select the appropriate rollup or report principal. The focused
`--battle-runtime` local SQL command above now exercises source preparation and
atomic publication together. The dated identity/transfer/review/loss tests use
controlled coherent UTC labels separately from the exact original native writer
readback. They do not establish real personal-server authentication history.
Qualification evidence is recorded in
[the status document](IMPLEMENTATION_STATUS.md#atomic-battle-observation-publication).

These reports supply observation primitives for the accepted balance suites.
They establish no decisive whole-battle outcome, complete control/prevention
coverage, gear power, complete population, reward rate or causal balance result.
The native additions, PvE attempts, progression context, four complete suites,
#487 compatibility and the actual personal-local gameplay/persistence/performance
gate remain required under #258.

## Accepted blindness and stun control capture

`blind()` and each successful `Stun()` branch now observe one accepted application
after `affect_to_char()` installs the effect and before any `stop_fighting()`
teardown. Existing immunity, death, already-active and saving-throw gates retain
their gameplay behavior. The full and half-duration stun branches each produce
one application. The current counters do not record effect type, rejection
reason, duration, overlapping disabled time or subsequent removal. Other direct
affect producers require their own reviewed hooks; these counts are partial
observations and cannot supply a whole-family resistance or control-duration rate.

The runtime uses the same association observation clock and existing bounded
kind-11 accumulator as damage and healing. Distinct source/target actors can
establish a hostile relationship, and both actual sides receive their conserved
application/received count. Native maintained NPC/pet lifetime IDs are measured
before legacy kind-8 identity validation; a missing legacy ID does not lose a
valid native observation. Kind-8's existing accumulator and encounter rules
retain their meaning. Invalid modifier bits are refused before association
mutation, and zero applications create no native stream or hostile edge.

Self-applied or environmentally sourced effects using the same character for both
arguments cannot establish a hostile edge. They are measured only within an
already observed battle, use one actor stream and carry the existing SELF
modifier. This modifier identifies a self-source observation without asserting
that a player chose the effect. Self effects outside a battle leave shared
participation absent. Capture remains bounded and performs no SQL or file I/O.

New native segments declare producer availability mask **31**, including the
accepted `blind`/`Stun` family. This enables exact observed counts, including a
zero when the declared hooks observed none in that segment. The reports keep
`complete_metric_coverage_implied=false`; availability does not establish full
control coverage. Older mask-27 inputs retain public NULL control counters. No
wire fields, definition numbers, sealed migrations or schema shapes change.

The existing gameplay harness compiles the maintained helper bodies and bounded
clamp directly from source. It isolates affect mutation, saves, randomness,
messages and combat teardown as game-service seams, while the runtime, queue,
worker, codec and SQL writer execute their maintained implementations. Its
accepted journey conserves **8 applications / 8 received**, including full/half
stun, blindness, a self observation, a pet and an NPC lacking a legacy ID. Rejected
effects and unrelated self effects produce no shared participation. History and
publication tests preserve these amounts and older unavailable NULLs.

The focused checks use the maintained commands:

```sh
python3 tests/async/test_telemetry_gameplay_adapters.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_battle_history.py
python3 tests/async/test_telemetry_battle_contributions.py
python3 tests/async/test_telemetry_combat_hooks.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
```

The SQL journey adds a fresh native control capture after the original source
proof, pins its ingestion origin, verifies every original field through the
restricted writer, and publishes it through the existing rollup/report roles.
This fixture does not replace the required actual personal-server effect,
persistence and performance journey. Complete reviewed control/prevention,
compact build/power/arena context, faction exposure, typed outcomes and the full
balance suites remain unfinished under #258.

## Native combat build snapshot reader

`telemetry_runtime_game_battle_build_context` reads a bounded, value-only native
snapshot described by `telemetry_battle_build_context.h`, version **1**. Its
current C++ layout is **440 bytes**, with a compile-time **448-byte** ceiling.
This is a reader contract, not an admitted telemetry record or a published
comparison. The existing kind-10/11 fields, level-derived power band, wire
contracts, definition-5 reports and sealed migrations retain their meanings.

The snapshot preserves exact level, raw primary and secondary class masks, the
game's single specialization, race/faction, the live actor key and the already
admitted configuration/build/content versions. NPC/pet keys retain the existing
fresh runtime lifetime and do not borrow a player's build. It creates no battle
or authenticated session and emits no record.

Base and effective values have separate arrays. Their explicit mappings are:

| Values | Order and interpretation |
| --- | --- |
| Stats | Str, Dex, Agi, Con, Pow, Int, Wis, Cha, Kar, Luk; loaded base versus current effective values |
| Resources | Hit, mana, vitality, ward; base, current maximum and actual current values, including signed/zero values |
| Combat values | Armor, hitroll, damroll; raw base and current fields, excluding inferred strength damage or composite ratings |
| Saving modifiers | Para, rod, fear, breath, spell; actual signed `apply_saving_throw` modifiers, not resistance probabilities |
| Flag banks | Native banks 1 through 5, widened without truncation; effective flags, fixed equipment declarations and the observed affect-list prefix remain separate |

Base fields are the game's stored/unmodified inputs. Effective-minus-base is
not attributed to equipment: race/class calculations, trained progression,
equipment, temporary effects and nonlinear rules can all contribute. Race,
class and version context remain available for later reviewed comparisons.

Equipment scanning has a fixed **43-slot** bound. It records occupied slots and
weapon/ranged-weapon/shield/armor/other counts, raw fixed modifiers for hit, mana,
armor, hitroll and damroll, declared flag banks, and the number of items with a
dynamic affect list. The selected fixed-feature SHA-256 digest encodes, in
network byte order: snapshot version (`u16`), content version (`u32`), slot count
(`u16`); then each slot number/presence (`u8` each). Present slots append type and
material (`u8`), condition and craftsmanship (`i16`), eight values (`i32`), five
flag banks (`u64`), wear/extra/extra2/anti/anti2 flags (`u32`), four fixed
location/modifier pairs (`u8`/`i8`), and dynamic-list presence (`u8`). Bounds
checks refuse an encoding overflow. Duplicate item pointers make the equipment
family unavailable and clear its values; other valid families survive.

These declarations do not prove actual equipment eligibility/application or
complete mechanical equivalence. Dynamic affects, prototype procs and their
outcomes remain unclassified. Item names, runtime/database IDs, prototype array
indices, prices and account/player data do not enter the digest. A renamed or
transferred item with the same selected features has the same fingerprint;
condition, slot or selected feature changes alter it.

The epic fingerprint covers the compiled `IS_EPIC_SKILL` catalog and stored
learned ranks for PCs. It encodes version (`u16`), content version (`u32`), first
and last skill IDs (`u16` each), then each catalog skill ID/rank (`u16`/`u8`) in
ascending order, including unlearned catalog entries. Catalog/learned counts are
separate. Wealth, unspent epic points, bonus-policy choices, ordinary skills and
epic spells outside this skill catalog do not define this fingerprint. Missing
or uninitialized catalogs, negative learned values and NPC builds remain
unavailable, with cleared epic values; they are not an observed empty build.

At most **64 unique affect nodes** are visited. The reader retains a known prefix
and marks cap/cycle truncation; it distinguishes `AFFTYPE_NOAPPLY` metadata from
listed applied declarations. Counts identify listed hitroll/damroll, armor and
hit/mana modifiers, regardless of whether they help or harm. This is not an
external-buff classifier or control-duration measurement. `complete` describes
only traversal of the current list. Caster/support origin remains explicitly
unknown, and the generic `context` pointer is never interpreted as an owner.

Arena room flags and the fixed **3-by-20** roster are independent observations.
A room flag cannot establish enrollment. A unique current roster pointer has a
team, player flags, enabled status, type and stage; an open/disabled roster does
not establish an active match. Duplicate pointers or invalid stage/type leave
roster membership uncertain. The arena roster stores no actor lifetime or match
generation, so this reader cannot prove a match identity, victory or historical
participant continuity. Missing room/roster sources remain unavailable.

The focused commands are:

```sh
python3 tests/async/test_telemetry_gameplay_adapters.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_runtime_integration.py
```

The gameplay fixture checks exact values, all 43 occupied slots, the full
309-skill range, selected-feature changes versus unrelated identity/wealth
changes, signed inputs, empty/malformed catalogs, NPC reuse/pet identity,
64/65-node and cyclic lists, metadata-only affects, arena membership ambiguity,
configuration version changes and refusal clearing. Independent Python encoders
verify both SHA-256 fingerprints. OpenSSL allocation callbacks verify zero
crypto heap calls during native reads; digest computation uses stack-owned SHA
state. The runtime fixture also executes the absent native catalog/arena-symbol
case. This does not qualify a complete server performance or persistence journey.

The separately versioned record/SQL/outage/replay contract and bounded cached
capture are implemented below. Full reads are gated before gear/epic scans,
including callbacks that execute on each hit. Retained matching observations,
report publication and actual personal-local source/readback/performance remain
required. Complete reviewed equipment/support classification,
resistance/prevention, effective-property coverage and arena outcomes remain
unfinished under #258.

## Durable selected combat build observations

Record kind **12**, `battle_build`, stores definition-**1** selected observations
from the version-1 native reader. The runtime now captures bounded native entry,
change and periodic points; report publication remains pending. The existing
transport, worker, typed column descriptor, private writer and reviewed incident
registrar handle this family. No separate service or storage transport is added.

The selected payload is **448 bytes** in C++, and the complete fixed record is
**488 bytes**, within the existing 512-byte record limit and 64-KiB batch limit.
Its independent portable encoding is **447 bytes**: **108 exact-width scalar
fields and two 32-byte digests** in the order of
`src/telemetry/telemetry_battle_build_fields.inc`. Scalars use network byte order
and signed fields preserve two's-complement values. ABI padding is excluded.
The native reader's five raw listed-affect flag banks are omitted from this
selected record; the bounded counts and traversal/origin uncertainty remain.
The field-layout seal is
`059f0e9236f3d30340feacf7329d8c0857cee23a2e94c327eb60bb2a8c51b5a5`.

Each point retains the original battle producer/id, live actor key/kind,
environment/season, configuration/build/content versions, exact association
revision and fact sequence, and one monotonic/UTC clock pair. The transport
producer and occurrence UTC must match the payload. The logical key is
`(bctx_battle_boot_id, bctx_battle_process_id, bctx_sequence)`; the producer must
allocate a fresh process-wide context sequence. A second transport receipt
cannot replace the same logical point. NPC/pet keys require the original tagged
runtime generation; they cannot borrow player epic data or another actor's build.
The native-to-record factory refuses mismatched actor/configuration/version or
nonzero reserved values and clears the output on refusal.

The record includes exact level/race/faction, primary/secondary class masks and
specialization, base/effective stats and combat/resource values, current
resources, signed saving modifiers, effective and fixed-equipment flag banks,
selected equipment counts/modifiers/digest, learned-epic counts/digest, bounded
listed-affect counts and independent arena room/roster observations. Availability
is independent by family. Missing families have cleared values; their zeroes are
not observations of an empty build. Known equipment/epic digests must be nonzero,
counts must obey the reader bounds, and partial affect/arena observations retain
their explicit quality markers. Support origin remains unknown.

Snapshot boundaries identify actor entry, reviewed actor changes, configuration
changes, periodic samples or source recovery. Separate **unavailable** points
identify rate limits, source failure or configuration withdrawal and contain no
profile values or stale digests. Configuration withdrawal has zero
configuration/build/content IDs; other points require an existing configuration
with matching environment/season/build/content. The association reference records
which native association the point accompanies. It does not prove that the
matching packet was retained, that the build remained constant between points,
or that an arena match/outcome is known. Those links and coverage must be checked
by the future retained-source/report path before using a point in a comparison.

Migration **0065**, `0065_telemetry_battle_builds`, adds **110 nullable typed
columns**, the logical unique key and **nine enforced CHECK constraints**. Older
families have NULL build fields; kind 12 has NULL fields from every older payload
family. Existing sealed migrations retain their bytes. Guarded reruns preserve
facts and reject incompatible column widths, reordered logical indexes or altered
checks through the verifier instead of repairing them. Writer retries compare
every selected field and digest; lost acknowledgements preserve identical replay.
Conflicts and invalid records use the existing quarantine evidence path.

The native and offline outage readers now write/read **DMSTLJ04** for record
families 1–12. Original v1/v2/v3 inventories remain readable with their original
family ceilings of 9/10/11; both observation and failure masks refuse a future
family. Upgrading an inventory appends through the existing atomic replacement
path. Independent private incident **schema 5** supports kinds 1–12 through
`telemetry_incident_registry_v5` and `telemetry_incident_v5`. Registration checks
the committed first verified post-fix fact, scope and UTC, and rolls back metadata
and detail together. Current published definition 5 continues to reserve schema 4.
No new report definition is activated. Definitions 1/2/3/5 validate and skip kind
12 while advancing ingestion; their inputs, totals and quality meaning are
unchanged. This record is not yet retained or exposed by battle reports.

These matching fields make the next balance comparisons more useful. A class or
faction outcome can be compared within observed level, build, gear and group
context; a temporary resource/effect advantage need not be mistaken for a class
advantage. A gear fingerprint distinguishes selected setups without inventing a
universal gear-power score. Explicit missing observations prevent old or partial
context from silently becoming complete evidence. The points alone cannot
establish causal balance, power equivalence, continuous exposure or player skill.

The focused local commands are:

```sh
python3 tests/async/test_telemetry_battle_build_contract.py
python3 tests/async/test_telemetry_battle_build_contract.py --sanitize
python3 tests/async/test_telemetry_transport.py
python3 tests/async/test_telemetry_outage.py
python3 tests/async/test_telemetry_battle_history.py
bash tests/async/run_telemetry_repository_sql.sh --build-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --build-storage
```

The SQL wrapper creates an owned disposable loopback database and removes it
after testing. It requires the local build/SQL tools described by the repository
test setup, and does not require production or staging access. The field/factory
tests compare native and independent Python encodings, signed limits, digest
bytes, partial families, gaps, corruption, strict row shape and sealed-report
compatibility. SQL journeys apply the complete 65-step chain, compare all fields,
exercise CHECK/logical-key refusals, guarded reruns and schema drift/restoration,
and register schema-5 loss evidence using a restricted reviewer. The private
writer journey covers all twelve families, exact replay, lost acknowledgements,
NULL separation, immutable configuration qualification and quarantine evidence.

## Native cached build capture

Accepted hostile/support observations and proven same-room formal-group
presence now sample the current native actor build after admission to the shared
battle. Context callbacks can sample an existing association; they do not create
a battle. The runtime uses the same reader, kind-12 contract, control reserve,
worker and private SQL writer. Every attempted point uses a fresh process-wide
context sequence, including attempts that fail queue admission. A later recovery
reads current values under a new key instead of replaying an earlier snapshot
as current context.

The game thread keeps **512 fixed cache entries**. Each entry binds an opaque
character address, its runtime lifetime and the original battle/live actor
identity. Successful entry, configuration, recovery and periodic observations
are emitted even when selected values are equal. Accepted mutations coalesce
into a dirty flag; at the next observation an unchanged selected profile emits
no duplicate point. Comparison uses named selected fields, excluding only the
point clock/sequence/boundary and association revision/fact. Availability and
quality are part of the comparison. Leaving or closing a battle forgets the
relevant entry. A reused address or changed native lifetime obtains fresh
context rather than another actor's cached build.

| Capture work | Bound and behavior |
| --- | --- |
| Full native reads | At most **16 per one-second window** across the runtime. Admission checks run before gear/epic hashing. |
| Reads for a cached actor | The existing qualified `context_segments_per_minute` cap, normally **8 per 60-second window**. Equal dirty reads consume this budget too. Leave/close forgets the entry; the process-wide limit remains. |
| Periodic interval | At least `max(config.interval_usec, 10 seconds)`, normally **60 seconds** with the default telemetry interval. Active callbacks also observe due samples. |
| Empty gap markers | At most **16 per one-second window**. A successfully emitted gap is not repeated until its reason/configuration/battle changes or the source recovers. Marker suppression latches context-overflow quality for subsequent points. |
| Periodic selection and lifetime resolution | A rotating selection of at most **16 entries**; one live-world pass of at most **4,096 nodes**. Cached addresses are never dereferenced. The found character must match its runtime lifetime and current association actor identity/kind. |

These are bounded sampling windows, not promises that every change is captured.
Capacity refusal, missing source and configuration withdrawal produce empty
unavailable records where marker admission permits. Unavailable profiles clear
all gear/epic fingerprints and selected values. Configuration withdrawal also
clears configuration/build/content identity. A subsequent accepted point uses
current state and explicit recovery/configuration context. Build-only queue loss
stays on this family; it does not erase otherwise measured damage or control
coverage. Periodic capture runs after battle expiry and never advances engagement
time, participation, roster exposure or the collector's measured prefix.

Equipment changes mark the cache after successful `equip_char`/`unequip_char`
mutation. Effective-state changes mark it after `affect_total` completes; the
deferred `balance_affects` scheduler alone does not publish incomplete values.
The dirty hook copies no profile, writes no SQL and emits no record. The point
clock is the subsequent observation clock, not an asserted exact mutation time.
Periodic samples cover selected values without requiring every native writer to
have a new hook. Exact intermediate gear/stat combinations, every epic/resource
mutation, buff origin, applied proc contribution and continuous build validity
are not established by this capture path.

The extended gameplay fixture qualifies real reader-to-runtime records,
gear/learned-epic changes, equal-profile coalescing, independently cleared
invalid gear, configuration withdrawal/recovery, NPC/pet lifetime separation,
a freed cached character, a temporarily unresolvable live PC, periodic recovery,
actor/global/marker caps and queue-loss recovery with fresh keys. It confirms
that capture cannot keep an inactive battle alive and that build-only loss
preserves measured contribution coverage. Normal and fresh ASan/UBSan runs pass.
The maintained server build, both runtime lifecycle/outage/exhaustion variants,
58 history/source/publication regressions and standalone header contracts pass.
The actual equipment-entry fixture also proves rejected equipment does not mark
a build change.

On both disposable full-65-step **MariaDB 10.11.14** and **MySQL 8.0.46** chains,
the actual runtime/worker/private-writer journey stores **20 points** with exact
values for all **110 selected fields**, including both digests, signed values,
empty gaps and association/configuration references. Existing published battle
rows remain unchanged. The established **123 facts / 38 packets / 28 verified
contribution links**, **112/112 damage**, and fresh **8/8 accepted control**
publication, rollback, drift/restoration, retained source and private roles
continue to qualify. This is an executable adapter/persistence proof; the
game-service fixture seams and future pulse clocks do not establish a running
personal-server or performance journey.

The additional maintained local commands are:

```sh
python3 tests/async/test_telemetry_gameplay_adapters.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_issue590_hidden_pet_items.py
python3 tests/async/test_telemetry_runtime_integration.py
python3 tests/async/test_telemetry_runtime_outage.py
python3 tests/async/test_telemetry_runtime_exhaustion.py
make -C src
bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-runtime
```

**Historical expectation at native-capture delivery (superseded below):** retain exact kind-12 observations and publish
them under a separately versioned comparison definition with independent
schema-5 loss coverage. Verify association packets, aliases, point clocks,
configuration, selected availability and sampling gaps before comparing actors;
preserve current definition-5 meaning and published generations. Then qualify
the actual personal-local gameplay/readback/performance journey. Complete
reviewed native context/classification, typed outcomes, distinct PvE attempts,
progression/portfolio additions, the four balance suites/statistical exports and
#487 reward compatibility remain open under #258. All seven final acceptance
requirements keep their unfinished portions; production/staging access is
unnecessary.

## Retained build point publication

Definition **6** retains kinds **9–12** under its own source and publication
tables. Definition **5** keeps kinds **9–11**, incident schema **4**, its original
tables, row keys and published meaning. Definition 6 uses the independent
incident schema **5** introduced with build storage; earlier reviewed inventories
cannot certify the new family.

Migration **0066_telemetry_build_publication** adds four guarded, re-runnable,
protected tables: private `telemetry_battle_source_v6` and
`telemetry_battle_input_v6`, and public
`telemetry_rollup_battle_coverage_v6` and
`telemetry_rollup_battle_row_v6`. It changes no earlier migration or wire field.
All three supported migration prefixes, lifecycle inventory, runtime metadata
queries and boot fingerprints include the new stores. The full current chain has
**66 steps / 260 tables**.

The existing bounded page transaction retains every selected build field and
both digests, plus the exact configuration row available when the page is
processed. The proof contains configuration/environment/season/build/content
identities. Missing configuration remains unknown; contradictory configuration
refuses the page. Configuration evidence is part of the canonical retained
payload and folded source digest. Later raw retention or catalog edits cannot
change an already retained generation. Arrival timestamps remain separate from
producer occurrence labels.

Publication reuses the existing packet, graph, actor and alias reducer. A build
point references an exact complete association cut with an earlier producer
receipt and a monotonic clock no later than the point. Its actor must be active
with the same runtime kind. Later admitted association changes make an old
reference stale. Original battle identities remain visible alongside canonical
alias lineage; a receipt after alias retirement or battle close, or a clock past
the closed observed prefix, remains outside that prefix. Periodic reads during
inactivity grace do not manufacture measured participation.

`battle_build_points` exposes all **110 selected fields**, their native
availability and quality, occurrence/arrival labels, original reference,
canonical battle, configuration status, point-clock status and
`point_context_verified`. Digests are canonical lowercase hexadecimal.
Unavailable native families retain their raw cleared encoding and **must be
interpreted using `bctx_available` and `bctx_status`**, never as measured zero.
Unknown buff origin remains explicit. A valid actor/time/configuration link does
not make an unavailable gear, epic, affect or arena family comparable.

The point's UTC label is checked against its referenced producer clock.
Unknown, mismatched or discontinuous clocks prevent full point qualification.
Schema-5 loss review covers both delivery of kind 12 and the association prefix
and interval that could contain a missing kind-10 change. Missing review
inventory remains uncertainty. Packet/link verification and point qualification
are separate counts. Capped reads, rate/source/configuration gaps, partial
families and grace tails remain in the retained source and coverage.

These are **point observations**. Equal digests do not prove a build remained
unchanged between reads. No sampled build receives continuous exposure, damage,
healing, rewards, account/controller identity, a universal power score, or an
arena match result. Repeated points are not independent fights or people. The
new report supplies explicit level/race/faction/class/spec, base/effective
resources and stats, gear/epic features, affect-prefix and arena dimensions for
later declared comparisons. The complete four balance suites remain unfinished.

Both definitions use the same locked cursor, identity reservation, bounded
canonical payloads, immutable generation publication and receipt verification.
The existing limits remain: 16,384 retained inputs, 2,048 requested association
snapshots, 4,096 public rows, the 2,000-row default output cap, 8,192-byte payloads,
and a 32 MiB default reservation. Overflow/deadline failure refuses the whole
publication. No unrestricted report query over raw history is introduced.

The definition-6 rollup principal needs the existing state/reservation and public
incident permissions, SELECT/INSERT/UPDATE on its private source header,
SELECT/INSERT on its private inputs and public output, and SELECT on raw
telemetry, `telemetry_config`, identity evidence and schema-5 review stores.
Its report principal needs SELECT only on state, reservation, public
definition-6 battle rows/coverage and public incident snapshots. It cannot read
the private source, raw telemetry, catalog, account tables or review-authority
stores. A review correction is published in a new generation; superseding an
old generation preserves its rows and frozen inventory.

Use explicit disposable-local connection settings and reserve reviewed or
explicitly unknown identity through the existing identity API before processing
a fixed window:

~~~sh
python3 scripts/telemetry/rollup.py definitions --definition-version 6
python3 scripts/telemetry/rollup.py run --definition-version 6 --generation 1 --environment-id "$environment" --season-id "$season" --origin-ingest-id "$origin" --through-ingest-id "$through"
python3 scripts/telemetry/rollup.py publish --definition-version 6 --generation 1 --environment-id "$environment" --season-id "$season"
python3 scripts/telemetry/rollup.py report --definition-version 6 --generation 1 --environment-id "$environment" --season-id "$season" --name battle_build_points
~~~

The focused maintained proofs are
`python3 tests/async/test_telemetry_battle_history.py` and
`python3 tests/async/test_telemetry_battle_runtime_sql.py --sql-fixture`.
The latter requires the existing explicit disposable/loopback/allow-listed
database settings and prepares the full chain. It exercises native capture,
retention, restricted roles, lost acknowledgements, rollback, CLI reports,
kind-12 loss and corrected-review generations, raw/catalog independence, bounded
reads, schema drift and restored fingerprints on both supported engines.
Dated positive/negative fixtures declare their controlled clocks separately.

**Next executable expectation:** complete typed control attempts, resistance,
immunity and overlap-safe duration, the remaining native prevention/faction and
typed death/escape/objective producers and battle reports, then qualify actual
personal-local gameplay/readback/performance.
Distinct PvE attempts/objectives/recovery/reward links, rested/assistance,
milestones and switching/portfolio projections, all four balance suites and
statistical exports, and #487 canonical economic compatibility remain required
under #258. All seven final acceptance requirements retain unfinished portions.
Production and staging access are unnecessary for those implementation and
qualification steps.

## Expanded accepted status-control capture

Six additional maintained spell routes now supply accepted application counts
through the existing native kind-11 accumulator. Each callback follows the actual
effect mutation. The original rejection gates, saving calls, durations, messages,
retaliation and teardown order retain their behavior.

| Producer | Accepted boundary | Existing gates retained |
| --- | --- | --- |
| `spell_major_paralysis` | Major paralysis affect insertion | Caster death, resistance, NPC paralysis immunity, movement freedom and saving throw; trusted-source and negative-level bypasses retain their behavior. |
| `spell_minor_paralysis` | Minor paralysis affect insertion | Resistance, NPC paralysis immunity, movement freedom and saving throw. |
| `spell_slow` | Slow affect insertion | Caster death, already active, monk class, resistance and saving throw. |
| `spell_sleep` | Sleep affect join, including an accepted refresh | Caster death, resistance, positive-level equipment protection, saving/level/race conditions and the existing negative-level bypass. Fighting/position changes precede this mutation in the maintained spell. |
| `spell_silence` | Each of the four accepted duration branches | Live actors, computed percentage, trusted target, greater NPC/elite, already active and resistance. The configured duration is not an elapsed-duration observation. |
| `spell_entangle` | Temporary paralysis affect or direct `AFF_BOUND` mutation | Reviewed outdoor/vegetation predicate, live/non-trusted target, existing entangle/minor paralysis, movement freedom, saving throw and NPC paralysis immunity. Both actual branches supply exactly one application. |

Accepted refreshes count as application operations; they create no additional
disabled-time denominator. The counters aggregate the reviewed routes with the
existing `blind`/`Stun` observations. They do not retain an individual effect
family, attempt, resistance, immunity, removal or duration. No family success
rate, overlapping disabled time or complete producer coverage can be inferred
from these totals. Comparisons still require declared build/content/configuration
and producer coverage. The availability mask remains 31 and earlier unavailable
control remains NULL; no wire field, schema or earlier publication is changed.

The existing native observer preserves self isolation, conservative PC/pet/NPC
lifetimes and exact association references. Segment modifier flags are unions:
one self application can share a segment with later externally received effects.
`SELF` therefore does not apportion that segment's application/received totals.

The expanded executable fixture calls the actual six spell bodies and the
maintained class/affect query helpers. Game-service seams isolate resistance,
saving, affect insertion, retaliation and teardown. It covers rejection and
bypass gates, all silence durations, sleep refresh, both entangle branches,
unrelated self effects and retained self/pet/NPC context. The measured journey
conserves **17 applications / 17 received** through native capture, exact SQL
readback, private source retention and atomic publication. The earlier **8/8**
`blind`/`Stun` journey remains separate and unchanged.

Normal and fresh ASan/UBSan gameplay fixtures, **66** history/publication tests,
changed-line formatting and the maintained server build pass. Both disposable
MariaDB 10.11.14 and MySQL 8.0.46 full-66-step journeys qualify all exact source
values, bounded reports, unknown identity, publication retry and unchanged older
generations. The existing native battle and definition-6 build-point proofs
remain qualified. These seams still require actual personal-server effect,
save/readback and performance qualification.

The maintained commands remain
`python3 tests/async/test_telemetry_gameplay_adapters.py`, its `--sanitize` variant,
`python3 tests/async/test_telemetry_battle_history.py` and the explicitly configured
disposable `python3 tests/async/test_telemetry_battle_runtime_sql.py --sql-fixture`.
Typed control attempts/results and overlap-safe duration, remaining reviewed
effect/prevention producers, faction exposure, typed outcomes and all seven final
acceptance requirements retain unfinished portions under #258.
