# Shared battle association

Enabled, qualified telemetry now captures shared-battle facts at native hostile,
effective-healing support, presence, context and lifecycle boundaries. Its bounded
association engine, portable definition-1 packets and independent kind-10 writer
preserve battle history and conservative uncertainty. This establishes live
association capture. A bounded contribution accumulator and portable segment
contract are qualified separately below; their native capture, durable linkage,
expanded context/control/prevention/faction sources and atomic balance projections
remain under implementation.
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
legacy kind-8 amounts retain their meaning; these relationship observations do
not yet supply exact shared-battle damage/healing/control/prevention totals.

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
limits, saturation, sink failure, and allocation-free event/codec paths. Seven
cross-language regressions round-trip every actual sealed field and qualify
immutable layout, strict types/widths/lengths, semantic corruption, signed clocks,
unknown metrics and logical keys. Normal and ASan/UBSan executions passed.

This layer is compiled into the maintained server but is not yet allocated or
called by the runtime. The existing record kinds 1–10, kind-8 encounter summaries,
kind-10 association storage and report definitions retain their contracts. The
next required integration connects this accumulator at authoritative gameplay
and association boundaries, admits a separate typed contribution family through
the native writer/replay/outage/incident contracts, and atomically publishes
complete battle reports with source/linkage coverage. No timestamp join or
prototype-based conversion of legacy kind-8 totals is acceptable. Actual
personal-server journeys and performance qualification remain required.

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

The next integration gate completes compact context, control/prevention and
faction exposure sources, links actual shared contributions and publishes bounded
atomic balance projections. Existing kinds 1–9, sealed migrations and report
definitions retain their meanings. The four full balance suites and actual
personal-local gameplay/persistence/performance gate remain required by issue #258.
