# Reviewed identity history and observed effort

The offline contract in `scripts/telemetry/identity_history.py` implements dated
reviewed account/controller associations and exact interval attribution for the
accepted balance expansion. Native account lifetime/token preparation is implemented
alongside that contract. Authenticated live capture, restricted association SQL
registration and published report integration remain required.
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
token from the handoff; attribution begins at its new clock anchor. The account-load
token cache is implemented. Typed authenticated observations and the live identity
handoff are still pending implementation.

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
offline executable semantics. Native lifetime/token allocation has its own SQL
qualification below. Authenticated source capture, missing-identity gameplay,
wire handoff and reviewed association/report publication require their remaining
integration and real personal-server journeys.

## Native account lifetime and scoped token preparation

Migration `0056_telemetry_account_identity` adds two private stores. A retained
`telemetry_account_lifetime` row has a random nonzero 64-bit lifetime ID and a
nullable current account-name binding. The binding follows an actual SQL account
rename through `ON UPDATE CASCADE`. Deleting the account sets the binding to NULL;
the retired ID remains reserved. Creating another account with the same name
allocates a distinct lifetime. No creation timestamp or present-day account name
is used to assign historical ownership.

`telemetry_account_token` retains one random nonzero token per environment, season
and lifetime. Its primary key is the scoped token, and a unique constraint on the
scoped lifetime makes repeated preparation stable. A different environment or
season prepares another token. Tokens are pseudonymous subject data; access to
both private stores can connect their scopes. They are not anonymous, and these
stores do not belong to the report or telemetry writer roles.

`sql_prepare_telemetry_account_token()` uses the existing native account-persistence
connection. It refuses a caller's existing transaction, locks the authoritative
account row, checks the deletion fence, and reuses or allocates the two identities
inside its own transaction. Each allocation gets at most four entropy/collision
attempts. Statements are bounded to 1,024 bytes; account names are escaped and
validated against a 200-byte limit, and all row/key parsing is bounded. Entropy,
allocation, SQL and commit failures leave the output token zero. A lost commit
reply can leave durable records; retry reads their existing keys without issuing
replacement identities.

The preparation hook runs after a successful `read_account()` and stores only
three transient values on the account: token, environment and season. Account
reload clears these values before attempting the read; invalid/missing accounts,
disabled/uninitialized telemetry, shutdown and the client-free backend retain
unknown identity. Capture, presence, combat and progression hooks do not call the
SQL preparation helper. Copyover's existing account load prepares the token again
from retained SQL identity, but does not yet emit a typed identity handoff.

Preparation can precede password verification. It emits no ownership fact and
must not be counted as authentication, participation or an active account. A
future authenticated capture boundary will copy the cached token into an observed
fact. Unknown token zero never substitutes a name, a PID or an invented account;
preparation failure does not change the account-load result or authentication.
Accounts loaded while telemetry is disabled stay unknown until an actual reload
prepares their cache. Capture cannot repair that state with a live SQL lookup.

The native account owner needs SELECT/INSERT on the two new private tables, with
no UPDATE/DELETE on those stores for preparation. MySQL 8.0 `FOR UPDATE` also needs
a write/lock privilege on `accounts`; the fixture qualifies the minimum
`UPDATE(account_name)` alongside `SELECT(account_name,blocked)`. The native account
owner normally already has account write privileges. Those account privileges
are not granted to a telemetry writer or report reader. In a disposable local
setup, the two additional grants use its explicit schema and existing native
account-persistence user:

```sql
GRANT SELECT, INSERT ON personal_test_db.telemetry_account_lifetime TO 'account_owner'@'localhost';
GRANT SELECT, INSERT ON personal_test_db.telemetry_account_token TO 'account_owner'@'localhost';
```

The focused executable test runs the production C++ allocator against each real
SQL engine. It exercises repeat and case-alias preparation, scope separation,
rename, deletion/recreation, missing/fenced accounts, simultaneous loads, quoted
names, caller transaction retention, bounded zero/colliding entropy, entropy and
allocation failures, write/commit rollback, lost commit replies, restricted roles,
constraint negatives and exact verifier drift. The cache tests execute the actual
runtime helper in SQL-header and client-free variants. The SQL wrapper supplies
only explicit disposable credentials and never reads the checkout's environment:

```sh
python3 tests/async/test_telemetry_account_identity.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --identity
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --identity
```

This source increment prepares durable account identity. It does not complete
authenticated ownership observations, controller proof/review registration,
identity generation publication, character portfolios or their personal-local
gameplay qualification.
