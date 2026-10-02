# Recovered telemetry follow-ups and no-deployment qualification

This work restores reviewable implementation and acceptance ownership for
[#561](https://github.com/Community-Duris/Duris/issues/561),
[#565](https://github.com/Community-Duris/Duris/issues/565),
[#566](https://github.com/Community-Duris/Duris/issues/566), and
[#567](https://github.com/Community-Duris/Duris/issues/567). Their closure messages
refer to discussions 636, 634, 627, and 633 respectively; those destinations were
not resolvable when this work began. Closure is not completion evidence.

Baseline: `f152b8d6927cccd96da27a03afa13883f9c997e9` on `master`.
The SQL record-kind round-trip gate is already delivered by #591, and its
accounting port by #669. Shared telemetry changes target master first. Any
accounting port must be deliberately qualified against that branch's authoritative
manifest; historical migrations must not be renumbered or rewritten in this PR.
Accounting-specific reconciliation/reward projection remains #487, not this PR.

## Scope and acceptance ownership

- **#561:** startup validates all serialized columns, critical SQL types and
  signedness, required replay identities/indexes, and the dedicated writer's
  permissions. Schema and permission incompatibilities must be payload-free,
  distinguishable, and detected before useful telemetry admission. Gameplay
  remains available with telemetry disabled/degraded. Both SQL engines need
  executable compatible-schema, column/type/index drift, and permission negatives.
- **#565:** serialization and startup validation consume a shared canonical
  descriptor; every field has an explicit database mapping or a reviewed exclusion.
  Prefixes, replay identities, sensitivity and existing immutable migration content
  remain stable. Full record-kind round trips must continue to pass.
- **#566:** a bounded durable outage/loss ledger is sufficient; a complete payload
  spool is not required. Register producer lifetime before admissions, persist on
  the worker only, classify clean drains/abandonment/ambiguous or unknown tails,
  and refuse silent overwrite, corruption, conflicting writers and disk-full
  evidence loss. Test graceful shutdown, real process restart/exec, abrupt kill,
  SQL outage, checksum failure and idempotent recovery. Never invent exact losses
  or extend a last-observed watermark into a proven crash boundary.
- **#567:** register reviewed incident producer/scope/time/record families, preserve
  nullable unknown ends, track the first verified post-fix observation and backlog
  disposition, and make registered gaps visible to reports and rollups. An unclosed
  session tail is not a normal exit. Reconstructed observations require distinct
  provenance; absent or unreliable authoritative logs cannot supply missing facts.

Current implementation and execution evidence are recorded in
[IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md). The shared column contract,
startup schema/permission validation and admission gate have passed local
MariaDB/MySQL qualification. [Durable outage evidence](OUTAGE_STORAGE.md) now
passes worker/runtime, process restart/exec/kill and storage-failure qualification.
[Reviewed incident coverage](INCIDENT_COVERAGE.md) now supplies consecutive retained
inventory versions, committed post-fix references, nullable unknown ends and
explicit backlog/reconstruction dispositions. Publication preserves a reviewed
snapshot and both rollup and administrator reports display its gaps. Historical
incident facts still require an evidence-backed review; local synthetic fixtures
qualify the implementation without asserting a production history.
The presence of a specification, draft PR or fixture does not satisfy acceptance.

The owner has authorized a personal local test setup as the technical acceptance
environment for this expansion. Production or staging access is not required.
The production inspection/activation guidance below applies only to a later,
separately authorized deployment; it is not a blocker for local implementation
qualification or issue completion backed by that evidence.

## Non-mutating evidence assessment

`scripts/telemetry/preflight.py` validates a strictly typed, sanitized observation
packet and classifies technical blockers, separately approved rollout actions,
and required live readback. It performs no connection, migration, grant, restart,
configuration update or deployment. It cannot prove that caller-supplied evidence
was independently obtained: collect and retain that provenance separately.

```sh
python3 scripts/telemetry/preflight.py --template
python3 scripts/telemetry/preflight.py /private/path/telemetry-observations.json
python3 tests/async/test_telemetry_preflight.py
```

The template contains only unknown values and is deliberately NOT a ready-to-run
production configuration. Do not put passwords, connection strings, player data,
or raw command/log payloads in this packet. Unknown evidence is not false gameplay
activity and is not successful qualification. Exit 2 means blocked/refused input;
exit 3 means authorized rollout or live verification remains pending; exit 0 means
all stated evidence checks pass (not that this tool performed the inspection).

## Production preflight: read-only only

1. Prove the live process's cwd/executable, checkout and binary identity. A sibling
   repository or staged executable is not proof of the running build.
2. Derive the actual database target and schema head without printing secrets.
   Use short `SELECT`/`SHOW` checks inside a read-only transaction, with bounded
   query work; never run a fixture harness or write-permission mutation on prod.
3. Check dedicated writer credential presence and least-privilege grants. Never
   substitute gameplay credentials. Provisioning/changing a database principal
   requires later explicit approval.
4. Resolve the reviewed full property digest against the effective server settings
   and retain historical catalog mappings. Merely setting `TELEMETRY_ENABLED=1`
   cannot repair an absent or incompatible catalog.
5. Verify protected durable ledger storage and exclusive ownership, historical
   incident registration, privacy/lifecycle review and a binary/schema-compatible
   rollback. Do not erase incident evidence during rollback.
6. Inspect only aggregate ingest timestamps/watermarks/health. Distinguish disabled
   capture, startup validation refusal, unhealthy ingestion and genuinely empty
   coverage. Record unresolved incident end/backlog as unknown, not reconstructed.

Private production facts belong in a protected local evidence directory, not in
this repository. Publish only sanitized aggregate blockers and evidence limits.

## Separately approved activation (NOT performed by this PR)

- Deploy a qualified candidate only after approval; do not overwrite the running
  executable or promote a staged binary during preparation.
- Provision missing least-privilege roles, catalogs and durable ledger paths only
  after approval, then enable observational collection through explicit settings.
- A controlled ordinary restart may be required because startup resolves process
  environment. Copyover is not automatically a substitute for wrapper/environment
  reload or incompatible binary/schema changes.
- Read back the new process identity and effective configuration. Prove fresh
  session/progression records, advancing health and normal login/save/gameplay.
  Use `world telemetry` and payload-free `telemetry_health` status events.
- Record the first trustworthy post-fix observation, classify the historical backlog,
  and keep incident gaps distinct from zero activity. The expansion's #258
  acceptance is the documented local observational journey and qualification
  gate; a production rollout is a separate operational decision. Automatic balance
  application remains excluded.
