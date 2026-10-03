# Shared battle association

The pure association module is implemented and locally qualified. Its native
runtime callbacks, persistent record kinds, durable writer and published balance
projections remain under implementation. The existing runtime currently captures
more accurate group encounters; enabling telemetry does not yet emit this module's
shared battle facts. [Implementation status](IMPLEMENTATION_STATUS.md) retains
the complete accepted expansion and personal-local qualification requirement.

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

## Bounds and qualification

The state retains 128 battles, 64 actors per battle, a 64-entry terminal cache,
and 4,096 normal mutation facts per battle. A mutation emits at most five facts;
an unchanged relationship updates clocks without rows or graph reconstruction.
Cuts retain revision, fact ordinal and expected count. A consumer must verify
the complete packet and its terminal cut before treating that revision as
complete. A sink refusal preserves loss quality and partial graph status.
Mutation-budget overflow emits one partial cut; terminal summaries remain bounded
and available for a best-effort close. Actor/merge capacity refusal preserves
the existing components and makes their results partial. Sequence exhaustion
refuses another identity.

The qualified build measures 1,719,392 bytes for the complete fixed state and
392 bytes for a fact. Compile-time guards cap the state at 2 MiB and keep a fact
plus the existing record header within 512 bytes. Allocation traps cover the
observation and close paths; the module owns no dynamic containers, character
pointers, SQL, strings or I/O. These bounds do not establish actual server/writer
performance or representative racewar capacity.

```sh
python3 tests/async/test_telemetry_battles.py
python3 tests/async/test_telemetry_battles.py --sanitize
python3 tests/async/test_telemetry_group_hooks.py
python3 tests/async/test_telemetry_combat_hooks.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
make -C src
```

The pure-module fixtures qualified duels, reinforcement, healer-only participation,
proven group presence, membership changes, leave/rejoin, zone chase, changing
modes, pets/owner loss, distinct NPC generations, retained aliases, exact
64-actor merges, oversized merge refusal, roster ambiguity, unknown clocks,
inactivity bounds, inline expiry accounting, callback loss, row/actor/slot caps,
and sequence exhaustion. Complete packets and duration conservation passed;
normal, AddressSanitizer and UndefinedBehaviorSanitizer executions passed.

The next integration gate is actual live NPC generation and group revision
capture; reviewed hostile/support/control/prevention and presence callbacks;
versioned persistent facts and contribution linkage through the existing queue,
writer, replay and loss contracts; additive immutable schema/permissions/lifecycle
registration; and bounded atomic balance publication. Qualify that gate on both
disposable SQL engines and real personal-local gameplay before claiming shared
battle collection. Existing kinds 1–9, sealed migrations and report definitions
retain their meanings. The four full balance suites and final personal-local
gameplay/persistence/performance gate remain required by issue #258.
