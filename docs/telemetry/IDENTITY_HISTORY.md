# Reviewed identity history and observed effort

The offline contract in `scripts/telemetry/identity_history.py` implements dated
reviewed account/controller associations and exact interval attribution for the
accepted balance expansion. Authenticated account token allocation, live capture,
restricted SQL registration and published report integration remain required.
This module does not query current account ownership or identify a person from a
name, email, IP address or device. The authoritative delivery record is
[IMPLEMENTATION_STATUS.md](IMPLEMENTATION_STATUS.md).

## Reviewed versions

One packet is a complete, bounded association snapshot for an explicit environment
and season. It has a consecutive registry version, the digest of its exact
preceding packet, a reviewer token and evidence digest, and a reviewed UTC window.
The UTC window is half open: `[reviewed_from, reviewed_through)`. Its end must not
be later than the review time. Association list order does not affect identity;
SHA-256 covers every reviewed field in canonical JSON, sorted by association ID.

Each association retains an opaque account token, a confirmed controller token
or explicit unknown status, a known valid-from point, an optional valid-through
point, reviewed provenance and an evidence digest. Confirmed associations require
a controller; explicit unknown associations cannot carry one. Active ranges for
the same account cannot overlap. Several accounts may share one confirmed
controller. Withdrawn rows remain in the snapshot and supply no current linkage.
An absent row supplies unknown linkage, not a new or independent person.

A NULL valid-through does not assert indefinite continuation. Linkage ends at the
reviewed window boundary. A point outside that window remains unreviewed even
when an association row has an open end. An uncertain start cannot be submitted
as a confirmed dated start; that period remains unknown until evidence establishes
it. Review time cannot supply a missing historical ownership boundary.

Exact retries retain the same packet digest. A correction creates a new version
and retains all previous association IDs; withdrawing a row is explicit, and an
association ID cannot change its account token. Earlier validated versions remain
unchanged. Future SQL registration must retain both versions and publish the
chosen version with its report generation. It must not silently join historical
reports to the current registry.

The validator checks the packet's types, scope, chronology and evidence shape.
It does not authenticate a reviewer or establish that an arbitrary supplied token
was issued by the server. The authenticated capture adapter and restricted review
role must establish those authorities during integration.

## Ownership and clock boundaries

`OwnershipObservation` uses the immutable original session, subject/PID, observation
producer/replay key, observed monotonic point, scoped account token and an explicit
authenticated source family. An unavailable observation carries no account token.
These values must come from authenticated observations, not a present-day lookup
of old characters. No account name is accepted by this contract.

`attribute_interval()` partitions an observed interval at actual ownership cuts
and relevant reviewed association boundaries. Time before the first same-producer
ownership observation remains unknown. An ownership transfer or loss of identity
starts at its observed boundary. Input order and exact repeated ownership facts
do not change the result. Conflicting replay values, conflicting accounts at the
same point, and scope/session/subject mismatches refuse attribution.

Copyover retains an original session identity, but starts another producer's
monotonic clock. Old-producer ownership timestamps cannot label the new process's
intervals. A fresh authenticated copyover observation can retain a known account
token from the handoff; attribution begins at its new clock anchor. The live
handoff and token cache are still pending implementation.

Controller attribution requires compatible UTC labels: both endpoints are known,
their difference equals monotonic duration, and no clock-discontinuity or UTC
unknown/mismatch/backward/fanout flag is present. Otherwise the observed monotonic character
effort remains available with its captured account, while UTC union time and
dated controller linkage remain unknown. A matching timestamp alone is not proof
of a continuous, complete source history; incident/drop and retained-source
coverage must accompany the eventual published reports.

## Effort denominators

`union_effort()` retains separate account and confirmed-controller cells for each
environment/season/registry version/configuration/category. It sums observed
character duration and independently calculates the union of compatible UTC
intervals. Different configuration cells remain separate. Presence-derived effort
and input-derived active time have separate categories; one cannot inflate the
other. Active, idle, unknown and linkdead measurements retain their exclusive
source-interval contract.

| Supplied observation | Character effort | Confirmed controller union |
| --- | ---: | ---: |
| Six linked characters overlap for one hour | Six character-hours | One observed controller-hour |
| Two linked characters play consecutive hours | Two character-hours | Two observed controller-hours |
| Two accounts have unknown controllers | Two character-hours | Unavailable; unknown accounts cannot be combined into one person |
| Captured account, ambiguous UTC clock | Known monotonic effort | Unavailable |

Exact repeated slices are deduplicated. Conflicting values under a replay key,
overlapping slices of one input, overlapping activity from the same original
session/producer, mixed registry versions and unsigned total overflow refuse
aggregation. Covered character duration and unknown-clock character duration are
separate. Known accounts expose the union of their covered subset; any unknown
clock portion makes the complete supplied account union NULL. Unknown-controller
populations expose character effort and account counts without a combined human
clock. Distinct account counts remain account counts. Aggregates retain supported
source/rollup quality, including cardinality loss, late input and incident gaps,
and derive UTC unknown/mismatch flags from their own endpoints when necessary.

The results describe exactly the supplied input. They are not a complete season
or cohort claim. The report integration must declare retained-source bounds,
truncation, missing identity/context, incident coverage and sample sufficiency.
Union time follows the existing observation heuristic and does not establish
continuous human attention. Rates and causal claims are not computed here.

## Bounds and offline qualification

Packets contain at most 1,024 associations and occupy at most 1 MiB. The shared
evidence reader reserves a one-byte overflow sentinel before decoding, rejects
duplicate JSON keys, nonfinite constants, excessive numeric encodings and invalid
nesting, and retains the incident family's existing 128 KiB limit. Attribution
accepts at most 1,024 ownership observations per interval; effort aggregation
accepts at most 16,384 slices. Overflow refuses the candidate instead of removing
earlier associations or declaring a complete truncated result.

The offline validator reports only scope, version, digest and count. Packet
failure output includes no source payload, account/controller list, reviewer
token or local path. It performs no SQL writes and creates no registry file.

```sh
python3 scripts/telemetry/identity_history.py /private/identity-v1.json
python3 scripts/telemetry/identity_history.py /private/identity-v2.json --previous /private/identity-v1.json
python3 tests/async/test_telemetry_identity_history.py
python3 tests/async/test_telemetry_incidents.py
```

The focused fixture covers dated changes and gaps, retained corrections and
withdrawal, replay/conflict, fresh copyover clocks, first-observation boundaries,
clock ambiguity, overlapping and sequential characters, independent presence,
unknown-controller populations, configuration separation and bounds. These are
offline executable semantics. Account rename/deletion/recreation, actual
authenticated cache preparation, missing-identity gameplay, wire handoff and
real SQL roles/publication require the source integration and its own journeys.
