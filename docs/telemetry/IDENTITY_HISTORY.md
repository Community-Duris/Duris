# Reviewed identity history and observed effort

The identity contract in `scripts/telemetry/identity_history.py` implements dated
reviewed account/controller associations and exact interval attribution for the
accepted balance expansion. Native account lifetime/token preparation is implemented
alongside that contract. Typed authenticated live capture, restricted SQL review
registration and immutable generation identity reservations are implemented.
Identity wire handoff is implemented. Published effort/report integration remains required.
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
unchanged. SQL registration retains both versions, and generation reservations
pin an explicit chosen version and digest. The balance publisher must use that
reservation when it publishes effort and reports. It must not silently join
historical reports to the current registry.

The validator checks the packet's types, scope, chronology and evidence shape.
It does not authenticate a reviewer or establish that an arbitrary supplied token
was issued by the server. Native capture establishes authenticated ownership;
the restricted SQL registrar below establishes reviewer authority and issued
token scope. A validated offline packet alone supplies neither authority.

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
intervals. The wire retains the last observed account context, UTC boundary and
quality. A fresh authenticated copyover observation uses the reloaded account
authority; attribution begins at its new clock anchor. A transferred, missing,
wrong-scope or deletion-fenced account cannot inherit authority from the saved
token. The account-load token cache, typed observations and identity handoff are
implemented. Report generation integration remains required.

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
qualification below. Typed source capture and missing-identity gameplay adapters
have focused native qualification below. SQL registration and generation reservations
have their focused qualification below. Wire handoff and effort/report publication
require their remaining integration and real personal-server journeys.

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
authenticated playing-descriptor boundary copies the cached token into an observed
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

Durable preparation, typed authenticated capture and restricted review registration
and identity wire handoff are implemented. Effort/report publication, character
portfolios and their personal-local gameplay qualification remain required.

## Authenticated ownership source observations

Migration `0057_telemetry_ownership_observations` adds two nullable typed columns
to the existing raw fact stream. Kind 9 supplies a nonzero scoped opaque account
token for observed login, reconnect, copyover or ownership change, or token zero
with the explicit unavailable source. Other record kinds leave both columns NULL.
The SQL check enforces this union distinction; the verifier and boot fingerprint
cover the unsigned widths, defaults, order and exact enforced check expression.
Earlier sealed migrations remain unchanged.

The producer observes only the logical PC of an authenticated playing descriptor
with an admitted session and connection. It copies the account-load cache only
when its scope matches and the current bounded account roster contains the PC
exactly once. Missing identity, foreign membership, a blocked account, wrong scope,
duplicate membership or a cyclic/over-capacity roster remains unavailable. At most
16 roster entries are inspected. Capture performs no SQL, filesystem I/O, waiting
or owned allocation and copies no account names or connection addresses.

Presence, context and player evidence deduplicate an unchanged owner. A new
connection observes reconnect; an actual token change or loss creates an observed
boundary. Copyover preserves the original session and emits a fresh observation
from its new producer clock and prepared account cache. These facts do not add
duration or checkpoint totals. Time before the first ownership observation remains
unknown, including after delayed admission or a new process.

Each existing session slot retains one exact unadmitted boundary, including its
original payload and clock point, plus one earliest unknown-loss
marker if another boundary is observed under backpressure. Further changes remain
unknown until a fresh current observation anchors recovery. The producer never
backdates a later owner across the uncertain span. An unadmitted retry gets a fresh
key in transport admission order; successful admission establishes the immutable
key used by downstream SQL retries. Retries drain before checkpoints
and handoff, and closed or detached slots retain pending facts until a later pulse
admits them. A handoff refuses continuity while these facts cannot be admitted.
The bounded outage journal and offline exporter recognize ownership loss evidence.

Native session and gameplay fixtures cover boundaries, replay after admission loss,
bounded overflow, logout retention, malformed membership and reconnect/copyover.
The repository fixture covers exact persisted kind-9 fields, identical/conflicting
replays, unavailable token zero, inactive NULL columns and all existing writer roles.
The mixed SQL rollup fixture validates kind 9 while preserving definition-1/2
playtime and observation totals, cursor/retry semantics and report privileges.

```sh
python3 tests/async/test_telemetry_session_state.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_outage.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --observations
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --observations
```

Definitions 1 and 2 validate and advance past ownership without projecting account
or controller amounts. Reviewed incident publication version 1 retains its sealed
families 1–8 contract. Migration 0059 supplies an independent schema-v2 reviewed
inventory including kind 9, using the existing incident snapshot tables for
balance generations. The identity report generation must consume that coverage,
the reserved reviewed version and retained corrections in exact effort/portfolio
publication. Those requirements and
the real personal-local gameplay gate remain in the accepted scope.

## Authenticated reviewed association registration

Migration `0058_telemetry_identity_review` adds a scoped reviewer authority table,
an append-only reviewed registry and its dated associations, and immutable
generation identity reservations. The migration does not provision reviewers or
invent controller links. An owner approves a database principal and opaque reviewer
token for an explicit environment/season. The registrar obtains `CURRENT_USER()`
from the authenticated SQL connection and matches its binary principal and reviewer
token against an enabled authority row. A shared row lock keeps that authority
stable until the registration transaction completes. The packet cannot choose its
authenticated principal. A copied reviewer token without the approved credential
does not authorize registration.

The maintained registrar performs these scope and reviewer checks. Its storage
credential is trusted to use this entry point; MySQL table grants do not provide
row-level isolation between seasons or independently apply the packet validator.
Keep that credential private and provision it only for approved reviewers.

Each newly registered version checks the server's UTC time against the supplied
review time, validates its exact retained predecessor and digest, and verifies every
account token against the native scoped issuance store. This includes withdrawn
rows. The association table also has a scoped token foreign key. Registration
does not read current account names, account ownership, lifetime bindings, IPs,
emails or devices. Confirmed controller tokens represent the staff-reviewed
association evidence; SQL authentication does not independently prove that every
controller is a distinct human. Missing links remain unknown.

The registry retains the authenticated SQL principal and server registration time
as private authority evidence outside the canonical packet digest. Row count,
canonical digest, enum/time boundaries, nonoverlap and retained association identity
are revalidated before a version is retried or used. Corrections append a complete
new version. They cannot remove an earlier association ID, change the account
behind that ID, or overwrite a retained review. Exact old retries remain valid
after a newer correction. Revoking a reviewer prevents new submissions, including
retry submissions, while retained versions and reservations remain available.

Header and association inserts share one bounded existing database transaction
and advisory scope lock. At most 1,024 association rows are inserted in one bounded
statement; packet size remains at most 1 MiB. An insert failure rolls back the
header and all associations. A lost commit reply discards the ambiguous connection;
retrying the same packet on a new connection reconciles the retained digest.

Use distinct credentials with these minimum grants. Only an owner can provision or
revoke authority rows. These are table/column permissions, not database-wide grants:

| Role | Permissions |
| --- | --- |
| Review registrar | SELECT on `telemetry_identity_reviewer`; SELECT, INSERT on `telemetry_identity_registry` and `telemetry_identity_association`; SELECT only `environment_id`, `season_id`, `account_token` on `telemetry_account_token`. |
| Balance rollup | SELECT on `telemetry_identity_registry` and `telemetry_identity_association`; SELECT, INSERT on `telemetry_generation_identity`. |
| Report reader | SELECT on `telemetry_generation_identity`; its separately documented aggregate permissions. |
| Native telemetry writer | Its existing raw writer permissions; no reviewed identity or private account stores. |

The review credential has no UPDATE/DELETE permission on retained history or
authority and cannot read the token's lifetime ID or its account-name mapping.
The report credential cannot read reviewed associations, reviewer authority,
raw telemetry or private token stores. Provision one SQL credential per approved
reviewer when independent reviewer attribution is needed.

Registration reads explicit `TELEMETRY_IDENTITY_REVIEW_DB_*` connection settings
using the same restricted connector as the rollup worker. Passwords remain in the
environment; command output includes only status, version, digest and count. Error
output omits packets, tokens, principal names, SQL and connector diagnostics.

```sh
# Offline validation retains its preceding-file requirement.
python3 scripts/telemetry/identity_history.py /private/identity-v2.json --previous /private/identity-v1.json
# SQL registration loads and validates its predecessor from retained SQL history.
python3 scripts/telemetry/identity_history.py /private/identity-v1.json --register
python3 scripts/telemetry/identity_history.py /private/identity-v2.json --register
```

## Immutable generation identity reservations

Before building a balance generation, the rollup reserves an explicit reviewed
version and digest using `TELEMETRY_IDENTITY_ROLLUP_DB_*` credentials. The key is
definition version, generation, environment and season. Identity reservations
require definition 3 or later; definitions 1 and 2 keep their existing contracts.
The CLI matches the packet digest against the actual registered version. An
adapter can instead reserve explicit NULL identity when no reviewed version is
available. That NULL reservation is also immutable; it cannot later acquire a
mapping in the same generation. A new review requires a new generation.

```sh
python3 scripts/telemetry/identity_history.py /private/identity-v2.json --reserve-generation 1 --definition-version 3
```

`reserve_identity_generation()` pins the registered version, digest, review window,
review time and association count. Its foreign key preserves the exact registered
digest. Exact retries and lost commit replies reconcile the same reservation;
changed identity under an existing generation key refuses. Later reviews and
reviewer revocation leave earlier reservations intact. Report readers can obtain
this metadata through bounded `read_generation_identity()` without seeing the
association or reviewer evidence. No current/latest-registry join is required.

A reservation reports `balance_report_published=false` and
`complete_identity_coverage_implied=false`. It does not calculate or publish effort,
count controllers, authorize causal claims, or establish source/incident completeness.
Definition 3 now consumes authenticated retained ownership facts, the reserved
association version and kind-9 loss/incident coverage in its atomic published
effort and observed XP portfolio reports. See [IDENTITY_PUBLICATION.md](IDENTITY_PUBLICATION.md)
for selected input retention, conserved coverage, union clocks and public report
permissions. The reservation metadata itself retains the unpublished flag; only
the committed publication header establishes publication. The actual personal-local
gameplay journeys and the other complete balance requirements remain required.

The focused SQL qualification uses disposable MySQL 8.0.46 and MariaDB 10.11.14,
including the complete immutable migration chain. It exercises absent, disabled,
wrong-principal and wrong-token reviewers; unissued/wrong-season account tokens;
future review dates; dated corrections and withdrawals; exact historical retries;
unknown and conflicting reservations; corruption detection; rollback and lost
commit replies; 1,024-row capacity and simultaneous retries; permission and SQL
constraint negatives; guarded reruns and exact verifier drift. Real connection
status proves that metadata reads release their transactions. Both engines also
prove that restoring deliberate drift returns the exact fresh migrated schema
fingerprint. These journeys and the command-line registration/reservation path
passed against the final migration checksums. Run the qualification locally:

```sh
python3 tests/async/test_telemetry_identity_history.py
TELEMETRY_REPOSITORY_DB_IMAGE=mariadb:10.11.14 bash tests/async/run_telemetry_repository_sql.sh --identity-review
TELEMETRY_REPOSITORY_DB_IMAGE=mysql:8.0.46 bash tests/async/run_telemetry_repository_sql.sh --identity-review
```

## Copyover account context

Outer copyover version 18 writes telemetry trailer version 2. The bounded handoff
adds 24 bytes for the last observed account token, UTC label with an unknown sentinel,
source and quality. It carries no monotonic timestamp or controller association.
Only the old process's original session and prior producer establish continuity.
Recovery retains this value in the existing descriptor/session storage, including
deferred writer qualification or capacity admission; it does not resolve ownership
from the wire. The reviewed controller association stays in the reserved report
generation.

An unavailable sample or pending overflow without a fresh anchor is exported as
unknown ownership. Overflow retains cardinality/drop/sequence quality even after
the retained raw boundaries have drained. An unadmitted boundary still prevents a
successful handoff; a failed worker durability barrier writes an absent handoff.
A handoff cut earlier than its ownership sample refuses instead of moving that
sample into the past. No ownership context supplies elapsed copyover downtime.

The reader consumes the exact sealed telemetry-v1 layout for outer versions
15–17 and imports its session/revision/counters/quality with absent ownership
context. Versions 12–14 use the existing absent-handoff path. New framing cannot
be accepted under an old outer version, and old framing cannot be accepted under
version 18. Invalid ownership source/token/reserved/quality fields discard that
entry while preserving the next world section. Existing runtime validation
handles other malformed continuity fields and falls back to an absent handoff.

The focused commands are:

```sh
python3 tests/async/test_telemetry_copyover_format.py
python3 tests/async/test_telemetry_session_state.py
python3 tests/async/test_telemetry_gameplay_adapters.py --sanitize
python3 tests/async/test_telemetry_contract_headers.py
```

They exercise the actual wire helpers, native state and transport/gameplay
adapters. These fixtures qualify known, absent, unavailable and overflow context,
legacy continuity and following-world alignment, conflicting resumes, fresh
producer clocks, an actual reloaded-token change and unavailable cache/scope/
deletion authority. They do not replace the final real server exec/save/readback
journey on the personal local setup.
