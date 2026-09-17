# Combat contribution and casting summaries

## Contract

`telemetry_combat_summary_payload` is a close-time control fact. The producer
keeps a fixed-memory accumulator keyed by the existing encounter identity
(`K`: producer boot, producer process, encounter sequence) and emits at most
one row for each retained actor when that encounter closes. Rows use the typed
telemetry stream and are projected into `telemetry_interval` by migration
`0025_telemetry_combat_summaries.sql`; the migration is additive and is not a
production activation step.

The accumulator has a maximum of 128 active encounters and 64 retained actors
per encounter. It performs saturating arithmetic and never stores a character
pointer, name, command, equipment list, combat log, SQL handle, or raw event.
Damage, healing, control, casting, and tanking observations update memory only;
they do not enqueue a row per hit, pulse, effect, or command. A participant cap
sets `TELEMETRY_QUALITY_CARDINALITY_OVERFLOW` and increments
`dropped_participant_count`, so a capped result is visibly partial rather than
silently presented as complete.

## Identity and attribution

Each row carries `E`-style source identity from the encounter source: environment,
season, effective config ID, classifier version, policy version, zone, and group
key. Actor identity is value-only:

- a player has `actor_kind=player`, a positive player ID/PID, and owns itself;
- a player pet has `actor_kind=pet`, no player PID, and the owner's subject ID;
- an NPC has `actor_kind=npc`, no player PID, and no owner subject ID.

Pet rows remain separate from their owners, while `unique_player_count` counts
distinct player subject IDs across player and pet rows. Pets therefore do not
inflate the player denominator, and NPCs do not become players through damage,
healing, control, or tanking attribution. `power_band` and the compact opponent
fields are observed context; they are not a claim that popularity measures
strength.

## Metrics

The retained actor row contains damage dealt and taken, healing attempted,
effective healing, derived overhealing, control applications, casting attempts,
completions and aborts, elapsed casting time, tanking time, modifier bits, and
quality flags. The producer records effective healing after the normal game
mutation boundary. It records damage after the victim's HP mutation, so ward,
safe-room, immunity, and other rejected paths do not become damage facts.

Casting attempts are the denominator. A successful terminal path increments
completion; `StopCasting`, concentration loss, invalid target/interrupt paths,
and other actual abort paths increment abort. If an encounter closes while a
cast is still active, the producer finalizes elapsed time as an abort and marks
`TELEMETRY_QUALITY_UNCLOSED_TAIL`. Thus a consumer can reconcile
`completions + aborts <= attempts` without treating an incomplete tail as a
successful cast. Spell IDs are used only at the in-memory hook boundary and are
not persisted as a raw spell log.

Tanking is an elapsed interval derived from the existing combat opponent
pointer. Context changes close the prior target interval; encounter close
flushes any active interval. The compact opponent count and power band are
descriptive context, not a balance or cohort conclusion.

The module exposes a typed control observation for control applications. The
current producer wiring covers the verified damage, healing, casting, and
tanking boundaries; it does not guess heterogeneous legacy effects from raw
commands or spell names. Additional control producers can call the same
value-only API after their actual effect boundary is reviewed.

## Coverage and use

Rows with queue-drop, clock-discontinuity, unclosed-tail, or cardinality
quality flags remain usable evidence but must be filtered or stratified before
cohort comparison. Encounter identity, source/config versions, retained and
dropped counts, and completion/abort denominators must be part of any report.
The data supports contribution/timing summaries and evaluation of the balance
questions in #252/#253; it does not duplicate or change those mechanics, and it
does not compare popularity with strength.

The producer remains behind the existing default-off telemetry runtime. No
automatic balance activation, live rollout, SQL write from gameplay, production
migration, or production data-completeness claim is part of this package.
