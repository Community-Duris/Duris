# Reviewed incident coverage

Migration `0054_telemetry_incident_coverage` and `scripts/telemetry/incident.py`
provide a reviewed inventory of telemetry gaps. Registration is an external
operation; the game and its telemetry worker do not acquire this authority.
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
