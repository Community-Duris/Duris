# Reviewed incident coverage

Migration `0054_telemetry_incident_coverage` and `scripts/telemetry/incident.py`
provide a reviewed inventory of telemetry gaps. Registration is an external
operation; the game and its telemetry worker do not acquire this authority.
Migration `0059_telemetry_ownership_incident_coverage` adds an independent schema-v2
review history covering families 1–9, including authenticated account ownership.
The sealed schema-v1 inventory still covers families 1–8. Both histories use the
same validator, registrar and existing scoped publication snapshot tables.
The worker's [outage ledger](OUTAGE_STORAGE.md) supplies bounded evidence for a
review. A registration does not replay payloads or invent absent observations.

## Review packet and source authority

Each packet is a complete inventory for one environment/season at a consecutive
registry version. It contains at most 64 incidents and 128 KiB of strict JSON.
Fields are typed numeric identities, nullable UTC labels, typed dispositions and
SHA-256 digests. Raw commands, SQL, account/character names, IPs, credentials and
free-form log text are rejected. Retain the underlying review evidence privately;
its digest is a reference, not proof that the evidence was independently obtained.
The opaque reviewer token identifies the authorized review in that evidence.

An incident records its affected producer when known, record families, optional
sequence bounds, nullable start/end, a fix reference, the first verified post-fix
observation, backlog disposition, and original/unavailable/separate reconstruction
provenance. Unknown ends remain SQL NULL and JSON null. A last journal sample is
not a proven crash end, and rejected admissions are not an exact missing-fact
count. No duration or exact-loss estimate is derived from these registrations.

When a reviewer supplies a post-fix observation, registration point-reads its
immutable replay key in `telemetry_interval` and checks the committed record's
family, scope and occurrence label. An unknown clock requires a null UTC label.
The review must establish why this is the first trustworthy post-fix observation
in its inspected evidence; the point-read alone does not prove the absence of an
earlier observation. A fix reference without a verified fact remains unresolved.
An unchanged verified reference in a later inventory retains its earlier review
even after raw-source retention. New or corrected references require a new check.

Backlog is explicitly `unknown`, `none`, `delivered`, `abandoned`, `mixed`, or
`retained_unresolved`. These are reviewed dispositions, not inferred from an SQL
watermark. `reconstructed_separately` retains the gap and marks reconstruction
provenance. Reconstructed evidence must use a distinct, separately reviewed
source; it must not impersonate the missing producer's original raw keys.
Unavailable authoritative evidence is a valid explicit disposition and cannot
supply reconstructed battle, XP or playtime facts.

Stored inventories are revalidated against their canonical packet digest before
an exact retry, correction or publication. Changed rows and incomplete inventories
are refused. Packets cannot silently remove earlier incidents. A corrected inventory retains
each incident ID and may mark an erroneous registration `withdrawn`. Prior
versions remain intact. The scope lock serializes registration and publication;
one transaction inserts the header and all rows. Identical retries return
`already_registered`, including after later versions. Reusing a version with
changed bytes fails. If a commit reply is lost, retry the exact packet to resolve
the outcome. Never submit a changed packet under that same version.

## Published coverage

Publication copies the current reviewed version into
`telemetry_rollup_incident_coverage` and `telemetry_rollup_incident` in the same
transaction that makes the aggregate generation visible. A publication retry
preserves that snapshot. A later registry correction appears in a newly built
generation. Report reads use the published copy in the same read-only snapshot
as their other aggregates; they do not consult the restricted registry or raw
history. The report cache preserves the same coverage and invalidates older cache
formats that lack it.

Every report's `coverage.incident_coverage` includes the registry version/digest,
reviewed time bounds, bounded incident details, relevant incident count, unknown
end count and unresolved backlog count. `not_registered` differs from an explicitly
reviewed empty inventory. A generation published before this facility is
`not_published`. Both missing states set inventory-unknown quality bit 28; a relevant
active incident sets gap quality bit 27. Existing v1 totals, denominators and report
definitions retain their meaning. Coverage does not change an unclosed session
into an exit or replace missing activity with zero.

Incidents are labeled overlapping, possibly overlapping, or outside the report's
occurrence window. Unknown ends/clocks are conservative possible overlaps.
Ambiguous rollup UTC coverage cannot establish that a gap is outside the window.
V1 incremental generations can advance their ingest watermark; their reviewed
version stays fixed while overlap/count annotations use that report snapshot's
current occurrence window. Report filters do not prove producer- or subject-level
coverage, so these annotations remain scope-wide. A reviewed inventory never
asserts universal completeness or implies zero gameplay activity.

## Local commands and roles

Create and validate a private packet without connecting:

```sh
python3 scripts/telemetry/incident.py --template
python3 scripts/telemetry/incident.py /private/path/reviewed-incidents.json
```

The template deliberately has required digest placeholders. It is not a reviewed
packet. To append a review to a provisioned local database:

```sh
python3 scripts/telemetry/incident.py /private/path/reviewed-incidents.json --register
```

Only `TELEMETRY_INCIDENT_DB_*` supplies this connection. It uses the same explicit
host/database/user/password, verified remote transport and finite timeouts as the
external rollup. It never reads gameplay credentials or runs migrations/grants.
The registration transaction is bounded by the existing statement/socket/lock
limits and a ten-second invocation deadline. Exit 2 is a sanitized refusal.

| Identity | Additional required authority |
| --- | --- |
| Restricted incident registrar | SELECT/INSERT on `telemetry_incident_registry` and `telemetry_incident`; SELECT on `telemetry_interval` for a bounded verified-fact point-read. No registry UPDATE/DELETE or gameplay access. |
| External rollup | SELECT on the two registry tables; SELECT/INSERT on the two published incident tables. Existing aggregate/state authority remains. |
| Report reader | SELECT only on the two published incident tables, in addition to its existing published aggregates. No registry or raw facts. |

All four tables have protected retained lifecycle inventory entries. Backups,
restores and report rebuild procedures must retain registry versions and published
snapshots together. At capacity, registration refuses rather than evicting history.
An operator must review a future capacity/retention change; current code never
purges incidents or rewrites an old registry version.

## Ownership-loss review contract

Schema v2 uses `telemetry_incident_registry_v2` and `telemetry_incident_v2`.
Its registry version sequence is independent of v1. The packet's explicit
`registry_schema_version` is part of its canonical digest; the same fields in
different schemas cannot share review identity. Exact historical retries remain
valid after a correction, all previous incident IDs must remain, and withdrawal
is explicit. The bounds remain 64 incidents and 128 KiB. Family 9 and a committed
kind-9 verified post-fix reference are accepted; family 10, future mask bits,
partial references and a mismatched committed scope/kind/time are refused.

Create its template with:

```sh
python3 scripts/telemetry/incident.py --template --registry-schema-version 2
python3 scripts/telemetry/incident.py /private/path/reviewed-ownership-incidents.json --register
```

Registration selects its tables only from the validated schema constant. Its
result states the schema and registry version. A lost commit acknowledgement
drops the ambiguous connection; an exact retry on a fresh connection verifies
the retained inventory. No current account name, token lookup or reconstructed
ownership is added to an incident packet. Unknown tails, nullable clocks and
separate reconstruction keep their existing coverage meaning.

The incident registrar additionally needs SELECT/INSERT on the two v2 input
tables; the rollup needs SELECT. The report reader's authority stays on the same
two published snapshot tables. It cannot read either private review history or
raw ownership facts. Both new tables are protected in the retained lifecycle
inventory; migration 0059's complete inventory contained 246 database tables.

Report definitions 1 and 2 use the v1 history. Definition 3 uses the v2 history
and preserves the chosen reviewed inventory in the
existing `(definition, generation, environment, season)` snapshot. The common
snapshot/read seam supports kind 9 and atomic parent/detail rollback. Definition
3 now integrates this snapshot into [atomic identity effort and observed XP
portfolio publication](IDENTITY_PUBLICATION.md). The input retention header,
incident snapshot, identity outputs and published status commit together. A
qualified incident snapshot alone does not publish a balance report or establish
complete ownership coverage. Known-ended ownership loss also requires a real
same-producer anchor with comparable clocks before attribution can recover.

The following qualification uses disposable local databases, both supported SQL
engines, synthetic identities and committed facts. It reads no personal setup
credentials and does not require production/staging access:

```sh
python3 tests/async/test_telemetry_incidents.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --incidents
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --incidents
```

The SQL fixture applies the entire immutable manifest, verifies the new schema,
and exercises committed post-fix references and mismatches, exact/conflicting
retries, retained corrections, lost commit acknowledgements, atomic publication,
unknown tails, preserved old snapshots, read-only reports, cache, and permission
negatives. Synthetic evidence qualifies the implementation; it does not establish
a real historical incident. A real inventory must retain unknown/unavailable
history until a reviewer has evidence to narrow it.

## Shared-battle loss review contract

Migration `0061_telemetry_shared_battle_facts` adds independent
`telemetry_incident_registry_v3` and `telemetry_incident_v3` histories for families
1–10. Definition 4 uses this explicit review schema.
Definitions 1/2 and 3 retain their v1 and v2 review histories and published
snapshots. A v3 packet, digest or correction cannot rewrite either earlier history.
Migration 0061's retained lifecycle inventory contained 248 database tables.

The v3 bounds remain 64 incidents and 128 KiB. Kind-10 verified post-fix references
must point to a committed raw record with the same explicit battle environment,
season, record kind and occurrence. The registrar selects the battle scope from
its typed columns, including unknown occurrence labels; it does not borrow NULL
legacy encounter/session scope. Future family bits and partial, uncommitted or
mismatched references are refused.

```sh
python3 scripts/telemetry/incident.py --template --registry-schema-version 3
python3 scripts/telemetry/incident.py /private/path/reviewed-battle-incidents.json --register
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --battle-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --battle-storage
```

Provision the restricted registrar with SELECT/INSERT on the two v3 input tables
and SELECT on raw telemetry for a verified-fact point read. A future balance
publisher requires SELECT on this review history and its existing published
snapshot authority. The report reader continues to read only published snapshots;
it has no v3 review or raw access. The maintained CLI uses dedicated
`TELEMETRY_INCIDENT_DB_*` credentials and performs no grant or migration work.

Both complete local 61-step chains passed real CLI registration, committed
post-fix binding/refusal, exact retries, retained corrections, lost commit replies,
atomic parent/detail failure, old immutable snapshots, unknown/reconstructed
tails, 64-row capacity, byte reservation and permission negatives. Guarded reruns
preserve facts/reviews; altered defaults and widened family checks fail the exact
verifier and remain unrepaired until explicitly restored. The restored full
schema fingerprint equals the fresh migrated schema on each engine.

The common snapshot seam is qualified for future definition 4; the balance
catalog and atomic battle projections remain pending. A review does not publish
a battle suite, recover missing gameplay facts or establish complete coverage.

## Contribution-loss review contract

Migration `0062_telemetry_battle_contributions` adds the independent
`telemetry_incident_registry_v4` and `telemetry_incident_v4` histories for
families 1–11. Each earlier schema keeps its original family limits, registry
sequence and digest identity. At this delivery the retained lifecycle inventory contained
250 database tables. The bounds remain 64 incidents and 128 KiB per packet.

A kind-11 verified post-fix reference must match a real committed transport
receipt, the explicit contribution environment/season and decision occurrence
label. It cannot borrow legacy NULL session/encounter scope or substitute the
segment's earlier observed endpoint. Corrections, explicit withdrawals,
unknown tails and separate reconstruction retain the existing review semantics.

```sh
python3 scripts/telemetry/incident.py --template --registry-schema-version 4
python3 scripts/telemetry/incident.py /private/path/reviewed-contribution-incidents.json --register
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --contribution-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --contribution-storage
```

The registrar needs SELECT/INSERT on the two private v4 input tables and SELECT
on raw telemetry for the bounded verified-fact read. An external publisher needs
SELECT on that history plus its existing published snapshot authority. Reports
read the existing published snapshot tables and have no raw or private review
access. The game writer has no incident registration authority.

The common atomic snapshot/read seam selects schema v4 beginning with definition
5, preserves v3 for definition 4 and preserves the older selections for
definitions 1/2/3. Definition 5 now publishes battle observations as described in
[BATTLES.md](BATTLES.md#atomic-battle-observation-publication); the complete balance
catalog remains pending. The local
storage fixture exercises real CLI registration, committed reference refusals,
rollback, exact retries after a lost commit reply, retained corrections,
immutable snapshots, private permissions, guarded reruns and refusal/restoration
of metadata drift. No production or staging access is needed.

## Selected build-loss review contract

Migration `0065_telemetry_battle_builds` adds independent
`telemetry_incident_registry_v5` and `telemetry_incident_v5` histories for families
1–12. Schemas 1/2/3/4 retain their original family ceilings, digest and review
sequence. Bounds remain 64 incidents and 128 KiB per packet. The current lifecycle
inventory contains 256 database tables.

A kind-12 verified post-fix reference must match a committed raw receipt and its
explicit build-observation environment/season, kind and occurrence UTC. Missing
or mismatched receipts are refused. Registration, exact retry and metadata/detail
rollback use the existing reviewed registrar. The restricted reviewer requires
SELECT/INSERT on the two private v5 tables and SELECT on raw telemetry for that
bounded reference check. The game writer has no registration authority.

```sh
python3 scripts/telemetry/incident.py --template --registry-schema-version 5
python3 scripts/telemetry/incident.py /private/path/reviewed-build-incidents.json --register
bash tests/async/run_telemetry_repository_sql.sh --build-storage
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --build-storage
```

Schema 5 can be reviewed independently. It is not automatically selected by a
published generation: current definition 5 continues to use schema 4 and excludes
build observations. A future build comparison definition must explicitly consume
schema-5 coverage and qualify its retained-source and atomic publication path.
Neither a reviewed inventory nor an unavailable build marker reconstructs the
missing profile or establishes continuous context between observed points.
