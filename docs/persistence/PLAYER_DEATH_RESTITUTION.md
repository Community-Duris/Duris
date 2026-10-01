# Audited player death restitution

`player_death_restitution.py` prepares SQL-derived item recovery for the native
staff command or the separately guarded offline SQL application path. It
restores only original item UIDs whose normalized schema-8 death payload and retained
custody row agree. It never mints an item from a vnum, clears a corpse, refunds
currency, or rewrites artifact authority without either an identity-bound
canonical row or the separately approved exact-evidence reconciliation path.

## Supported boundary

- Backend: MySQL 8 or MariaDB 10 with the ownership/death schema and immutable
  migration `0020_player_death_restitution` applied. File/flat-file authority
  is refused. The native bridge exposes the raw wire version separately from
  the normalized schema: death schema 8 is accepted for historical wire
  versions 2, 4, and 6, and for the current writer's wire version 8. Exact
  item encoding is validated by the native codec; unknown or corrupt encodings
  are refused rather than relabeled.
- Target: `ENVIRONMENT=test`, `dev`, `development`, or `local` remains the
  default non-production path. A production-classified target (including the
  legacy database name `duris` or a name containing `prod`, `production`, or
  `live`) is accepted only through exact database-name confirmation, a protected
  target-info probe containing the actual server fingerprint, and a literal
  loopback DB address. The read-only native preparation mode uses that target
  identity without a stopped maintenance boundary or backup. Offline SQL apply
  and production `offline-sql` verification additionally require an explicit
  stopped maintenance boundary and a native backup receipt bound to that target
  and boundary. Production `native` verification instead requires a fresh
  protected target-info artifact bound to the same plan target and an explicit
  stopped/masked validated boundary for that read-only check; it does not require
  a backup receipt. The supported service boundaries are the installed system-manager
  unit (`--maintenance-kind systemd`) and the explicitly owner-bound user
  manager used by `.sbs` deployments (`--maintenance-kind systemd-user` with
  `--maintenance-owner <UID>`). The latter verifies the user manager identity,
  unit identity/state, PIDs, runtime socket, cgroup, and visible process set;
  it supports only these stopped-unit forms: a unit whose
  `UnitFileState` is `masked` or `masked-runtime`, whose `LoadState` is
  `masked` or `loaded`, and whose manager-descendant `ControlGroup` has
  recursively visible process files that are empty; or a masked unit reported
  with `LoadState=masked` and `ControlGroup=` empty after systemd has removed
  its dead cgroup. The second form preserves the empty value and relies on zero
  service PIDs plus the owner-manager process-visibility proof; it never
  fabricates a cgroup path. A loaded unit with no cgroup, an active unit, an
  unexpected cgroup, or incomplete visibility is refused. This is not a
  `--system`/`--user` substitution or a fabricated proof.
- Recipient: this first slice requires the explicit recipient PID to equal the
  death PID. It delivers to ordinary player inventory (`equip_slot=0`) and
  rebuilds nested containers in captured payload order.
- Operator boundary: the real runtime owns the MySQL/MariaDB advisory lock
  `GET_LOCK(CONCAT('duris.player.death.restitution.',DATABASE()),0)` on its
  main SQL session before boot verification or writer-pool startup. Configured
  secondary and replacement connections validate that the original owner is
  still present with `IS_USED_LOCK()`; a lost owner or an unproven reconnect is
  refused. The restitution mutation transaction takes the same lock and
  rechecks its ownership before committing, so a running runtime refuses
  recovery and a recovery session refuses a new runtime boot. The CLI also
  reads a live process inventory and samples database sessions, active work,
  and open InnoDB transactions twice. The proof file below is only
  owner-readable expiry/context metadata; it is not an attestation and cannot
  establish quiescence by itself. Direct pool admission shares the same
  process-wide, atomic loss latch as the connection factory. Once ownership
  is lost, new leases and replacement connections are refused. Already
  borrowed raw connections still prevent recovery through the mandatory
  zero-other-database-sessions fence. The tool does not cancel in-flight SQL.
  Scheduled database writers must also be stopped; an event-scheduler daemon
  is deliberately a refusal, not ignored.
- Currency is classified as `currency_refused`; wallet/currency authority is a
  separate issue and is never changed by this tool.
- Artifact identity is two independent native signals: `ITEM_ARTIFACT` (bit 29)
  is the real artifact flag, while the native unique-name predicate is only
  legacy tracking evidence. The codec records both signals explicitly. Normal
  unique tracking never mints an artifact. A normal artifact transfer requires
  `artifact_domain_state.item_uid` to bind the original UID, `owned=1`, matching
  revision/timer/binding fences, and at least one legacy `artifacts`/
  `artifacts_mortal` row owned by the source player at `locType=3` or `locType=5`.
  The current runtime artifact path still reads those legacy tables.
- A missing or unbound domain is recoverable only through the separately
  approved artifact reconciliation path. It requires all of the following in
  the same plan: exact original payload, captured/current custody for that UID,
  an exact legacy timer/binding/ownership match, a canonical baseline/state
  record or an auditable baseline seed, and no surviving competing instance in
  any death or current-owner table. The native Unknown display marker
  (`locType=3`, `location=-2`) is accepted only as that source-player evidence;
  it is never interpreted as an NPC location. The plan records every input and
  assigns a distinct reconciliation classification; it does not infer identity
  from a vnum alone. A domain row is inserted or rebound only after the SQL
  guard has rechecked those fences.
- Reconciliation moves the canonical state to the recipient while preserving
  the opening timer, binding owner/timer, artifact type, metadata, revision
  chain, and legacy timer/binding fields. It inserts no duplicate delivery:
  `item_uid` remains the global idempotency key across deaths. Any payload,
  custody, baseline, legacy, or competitor conflict is refused with no
  reconciliation write. A name-marked unique item such as the unique gloves
  remains eligible only when its ordinary UID/current-owner and custody fences
  pass; no timer is reset.

The JSON files are protected evidence/approval artifacts. They contain player
and item identifiers, are written owner-readable only (`0600`), and must not be
pasted into ordinary logs or committed to the repository.

## Workflow

Run from the repository root with an explicit environment file or exported
variables. The environment file must define `ENVIRONMENT`,
`DB_HOST`, `DB_PORT` (optional), `DB_USER`, `DB_PASSWD`, and `DB_NAME`. The
password is consumed through `MYSQL_PWD`; it is never printed or stored in a
JSON artifact.

```sh
python3 scripts/player_death_restitution.py --env-file /secure/test-db.env \
  inspect --pid 42 --death-revision 7 --recipient-pid 42 \
  --artifact /secure/restitution-42-7.inspect.json

python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution-42-7.inspect.json \
  --preparation-mode offline-sql \
  --approve-artifact-reconciliation \
  --artifact /secure/restitution-42-7.plan.json
```

### Native live preparation workflow

Production-native preparation is a separate read-only mode. First create a
protected target-info artifact with exact production-name confirmation and a
server fingerprint, without maintenance arguments. Then pass it to the read-only
inspection and native plan:

```sh
python3 scripts/player_death_restitution.py \
  target-info --confirm-production-target <exact-production-db-name> \
  --artifact /secure/restitution-target-info.json

python3 scripts/player_death_restitution.py \
  inspect --target-info /secure/restitution-target-info.json \
  --confirm-production-target <exact-production-db-name> \
  --expected-fingerprint <fingerprint-from-target-info> \
  --pid 42 --death-revision 7 --recipient-pid 42 \
  --artifact /secure/restitution-42-7.inspect.json

python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution-42-7.inspect.json \
  --target-info /secure/restitution-target-info.json \
  --preparation-mode native --approve-production \
  --artifact /secure/restitution-42-7.native-plan.json

python3 scripts/player_death_restitution.py \
  export --plan /secure/restitution-42-7.native-plan.json \
  --inspect /secure/restitution-42-7.inspect.json \
  --artifact /secure/restitution-42-7.staff-payload.json \
  --approve --actor operator-repair --reason disputed-death-evidence
```

Native `inspect`, `plan`, and `export` do not create a backup, require an
offline proof, inspect a stopped service boundary, stop/mask a service, or
perform SQL mutation. The production target identity and fingerprint remain
mandatory. The native plan carries the actor/plan identity, explicit approval,
interactive deadline/expiration metadata, recipient-only native fence, and
revision/custody gates into the exact exported payload. It is explicitly
`applyable=false`; SQL `apply` and `mark_verified` refuse it.

### Production policy workflow (offline SQL)

Production-classified targets do not bypass the non-production guard. They use
a separate, target-pinned path in which every write command rechecks the actual
database identity, the exact target recorded in the plan, the stopped runtime
boundary, and the plan-bound native backup. The target probe is read-only and
does not authorize a write:

```sh
python3 scripts/player_death_restitution.py \
  target-info \
  --confirm-production-target <exact-production-db-name> \
  --maintenance-kind systemd \
  --maintenance-id duris-mud-production.service \
  --artifact /secure/restitution-target-info.json
```

For a `.sbs` deployment managed by the owner’s user manager, use the same
read-only probe with the explicit numeric UID. The owner must run the command
inside that user manager’s session. The tool refuses a different UID, a live
unit, a loaded unit with no cgroup, non-zero service PIDs, a non-empty service
cgroup, an unexpected cgroup path, or incomplete process visibility. A real
masked, inactive/dead unit may instead report `LoadState=masked` with
`ControlGroup=` empty because systemd already removed the unit cgroup; that
exact empty state is recorded and is accepted only with zero service PIDs and a
complete owner-manager visibility proof. No cgroup identity is synthesized:

```sh
python3 scripts/player_death_restitution.py \
  target-info \
  --confirm-production-target <exact-production-db-name> \
  --maintenance-kind systemd-user \
  --maintenance-id duris-mud-production.service \
  --maintenance-owner "$(id -u)" \
  --artifact /secure/restitution-target-info.json
```

For a disposable Docker rehearsal, use the complete immutable container ID and
`--maintenance-kind docker`. The target-info artifact records the live SQL
server fingerprint and the inspected stopped boundary. It contains no database
password. The command does not stop or mask anything; an active or restarting
runtime is a refusal.

Create the native backup while that same boundary is in force. `backup` calls
the CLI's real process/database quiescence checks; the proof file is context and
expiry metadata, not an attestation:

```sh
python3 scripts/player_death_restitution.py \
  backup \
  --target-info /secure/restitution-target-info.json \
  --receipt /secure/restitution-backup.json \
  --offline-proof /secure/restitution-quiescence.proof \
  --confirm-production-target <exact-production-db-name> \
  --expected-fingerprint <fingerprint-from-target-info> \
  --maintenance-kind systemd \
  --maintenance-id duris-mud-production.service
```

The command runs the vendor `mysqldump` utility with routines, events,
triggers, hex blobs, and a completion footer, then installs an owner-only SQL
dump and receipt without overwriting existing artifacts. Backup integrity is
not permission or quiescence proof. Never restore automatically from this
command.

Bind both protected artifacts into the plan and explicitly approve the exact
production plan. An unapproved production plan is written as a review artifact
with `applyable=false`; it cannot reach SQL mutation:

```sh
python3 scripts/player_death_restitution.py \
  inspect --target-info /secure/restitution-target-info.json \
  --confirm-production-target <exact-production-db-name> \
  --expected-fingerprint <fingerprint-from-target-info> \
  --pid 42 --death-revision 7 --recipient-pid 42 \
  --artifact /secure/restitution-42-7.inspect.json

python3 scripts/player_death_restitution.py \
  plan --inspect /secure/restitution-42-7.inspect.json \
  --target-info /secure/restitution-target-info.json \
  --backup-receipt /secure/restitution-backup.json \
  --preparation-mode offline-sql --approve-production \
  --artifact /secure/restitution-42-7.plan.json
```

Apply still requires the existing explicit `--approve`, the v3 quiescence
proof, and the exact target/boundary arguments. It also reopens and hashes the
receipt's SQL dump, rechecks the server fingerprint and stopped boundary, then
re-inspects all evidence before the existing advisory-lock transaction. A
wrong target or fingerprint, changed dump, live runtime, unapproved plan, or
stale evidence refuses before any restitution row is written:

```sh
python3 scripts/player_death_restitution.py \
  apply --plan /secure/restitution-42-7.plan.json \
  --offline-proof /secure/restitution-quiescence.proof --approve \
  --actor operator-repair --reason disputed-death-evidence \
  --target-info /secure/restitution-target-info.json \
  --confirm-production-target <exact-production-db-name> \
  --expected-fingerprint <fingerprint-from-target-info> \
  --maintenance-kind systemd \
  --maintenance-id duris-mud-production.service
```

`verify` performs the exact authority/payload readback under the same target
and boundary pin. For a production `native` plan, do not reuse the preparation
artifact's boundary-less target-info: create a fresh protected target-info artifact
with the exact same target/fingerprint and the explicit stopped/masked boundary,
then pass it with the matching maintenance arguments. This read-only native
fallback does not require or inspect a plan-bound backup receipt, convert or
replan the original plan, write a receipt status, or accept `--mark-verified`.
It rejects missing/mismatched target-info, a missing/mismatched boundary, or a live
service before receipt/item data reads. For a production `offline-sql` plan, the
existing plan-bound backup and stopped-boundary gates remain unchanged.
`--mark-verified` is a separate database write available only to the offline-sql
path: it requires `--approve`, a fresh proof, the explicit maintenance boundary,
the still-valid plan-bound backup, and the production approval recorded in the
plan. It does not merely relax the old `ENVIRONMENT` check:

```sh
python3 scripts/player_death_restitution.py \
  verify --plan /secure/restitution-42-7.plan.json \
  --mark-verified --approve \
  --offline-proof /secure/restitution-quiescence.proof \
  --target-info /secure/restitution-target-info.json \
  --confirm-production-target <exact-production-db-name> \
  --expected-fingerprint <fingerprint-from-target-info> \
  --maintenance-kind systemd \
  --maintenance-id duris-mud-production.service
```

The dedicated synthetic production-policy journey exercises the complete CLI
path without production access. It creates a network-isolated,
production-named disposable database and a stopped immutable Docker runtime,
then proves target-info, native backup/restore, unapproved-plan refusal,
wrong-target/fingerprint refusal, changed-backup refusal, live-runtime refusal,
guarded apply, exact verify, and mark-verified readback on both engines:

```sh
DURIS_PRODUCTION_POLICY_DB_IMAGE=mariadb:10.11 \
  bash tests/async/run_player_death_restitution_production_policy_mysql.sh
DURIS_PRODUCTION_POLICY_DB_IMAGE=mysql:8.0 \
  bash tests/async/run_player_death_restitution_production_policy_mysql.sh
```

The plan classifies every retained custody UID, including rows with no payload.
Only payload-backed, identity-fenced rows are eligible. A changed live parent is
not rejected merely because it differs from the custody parent: when the
retained root is the same and both parents are members of that captured tree,
the plan records `recoverable_topology_reconciled` and explicitly uses the
payload topology. A root change, outside-tree parent, stale revision, or owner
conflict is refused. Payload evidence is global across related deaths: a later
custody-only reference may recover the earlier exact normalized payload; equal
normalized evidence for one UID is deduplicated, while conflicting evidence is
classified `cross_death_payload_conflict` and refused.

Create the owner-readable context file for the exact disposable database. It
does not acquire the boundary. The live process/database checks and the
transaction-local advisory lock are the safety evidence; the expiry must be in
UTC and in the future:

```text
format=duris-death-restitution-quiescence-v3
database=<exact DB_NAME>
boundary=mysql-advisory-exclusion
guard=duris.player.death.restitution
expires_at=2099-01-01T00:00:00+00:00
```

For a local disposable target, create it with a restrictive umask. Do not use
this file as proof that a server stopped:

```sh
umask 077
printf '%s\n' \
  'format=duris-death-restitution-quiescence-v3' \
  'database=<exact DB_NAME>' \
  'boundary=mysql-advisory-exclusion' \
  'guard=duris.player.death.restitution' \
  'expires_at=2099-01-01T00:00:00+00:00' \
  > /secure/restitution-quiescence.proof
```

Then require an explicit approval and apply:

```sh
chmod 600 /secure/restitution-quiescence.proof
python3 scripts/player_death_restitution.py --env-file /secure/test-db.env \
  apply --plan /secure/restitution-42-7.plan.json \
  --offline-proof /secure/restitution-quiescence.proof \
  --approve --approve-artifact-reconciliation \
  --actor operator-repair --reason disputed-death-evidence
```

Apply uses one transaction and the shared database advisory exclusion lock,
current death-payload
and ownership-revision fences, the global `item_uid` delivery primary key, a
dedicated restitution receipt, an immutable original-payload delivery record,
and a mutable runtime companion updated by the normal SQL snapshot writer. The
receipt identity is in the restitution namespace and is not a generic command
or critical-operation inbox entry; the per-item restitution audit retains exact
source/delivered roots, parents, and revisions. A retry of the same plan is a
no-op after the applied receipt exists. A stale plan, hash collision, duplicate
UID, or changed owner causes a refusal with no delivery.

Read back every authority and exact payload field:

```sh
python3 scripts/player_death_restitution.py --env-file /secure/test-db.env \
  verify --plan /secure/restitution-42-7.plan.json
```

Verification checks the receipt, global UID guard, current owner/topology,
player projection, newer inventory set, affects, extra-descriptions, strings,
values, timers, condition, material, bitvectors, original payload digest,
runtime exact-state payload, and—when reconciled—the canonical domain/baseline,
legacy timer/binding, artifact metadata, and recipient transition. For an
`offline-sql` plan only, mark the receipt verified by repeating with a fresh
proof and `--mark-verified`. Native plans support read-only verification only.

## Receipt states

`player_death_restitution_receipt.status` is `1=approved`, `2=applied`, and
`3=verified`. `player_death_restitution_delivery.item_uid` is the cross-death
idempotency boundary; it is unique globally and deliberately survives later
receipt/death selection. `player_death_restitution_item` retains refused
classifications and missing-payload rows without inventing metadata.

## Local verification

The focused real-session exclusion test is:

```sh
tests/async/run_player_death_restitution_guard_mysql.sh
```

It uses real MySQL/MariaDB sessions to hold the runtime lock, proves that a
competing recovery and a new runtime startup both receive `0`, closes the
runtime session to prove the lock is released on lost ownership, then repeats
the inverse recovery-holder/startup refusal and finally proves acquisition
after release. It never uses mocked lifecycle state. The test requires an
explicit non-production `ENVIRONMENT`, `DB_HOST`, `DB_USER`, and `DB_NAME` (plus
the normal password/client variables) and exits with a clear blocker when no
SQL client/target is available.

The disposable restitution runner is the end-to-end CLI integration gate:

```sh
DURIS_TEST_DB_IMAGE=mariadb:10.11 bash tests/async/run_player_death_restitution_mysql.sh
DURIS_TEST_DB_IMAGE=mysql:8.0 bash tests/async/run_player_death_restitution_mysql.sh
```

It exercises actual guarded apply, stale-plan and invalid-proof refusal,
verification/receipt marking, replay, exact-state repository save/load, and
cross-death refusal. The native checker compiles the real SQL factory and
pool, tests competing acquisition, loses the owner session, then verifies
raw-writer visibility, direct lease/reconnect refusal, and pending-result
safety. It is a repository/materializer journey, not an interactive game login.

The focused artifact reconciliation runner covers the formerly refused
legacy-`Unknown`/missing-domain case with a real disposable database:

```sh
DURIS_TEST_DB_IMAGE=mariadb:10.11 bash tests/async/run_player_death_restitution_reconciliation_mysql.sh
DURIS_TEST_DB_IMAGE=mysql:8.0 bash tests/async/run_player_death_restitution_reconciliation_mysql.sh
```

It first proves that an unapproved reconciliation plan is not applyable, then
adds a competing current instance and proves the approved plan becomes stale
with no writes. After removing that competitor it applies and verifies the
canonical state, baseline, legacy rows, preserved timer/binding metadata, and
global UID deduplication. The fixture data is synthetic and contains no live
player identifiers.

`DURIS_RESTITUTION_RUNTIME_ONLY=1` selects a separate component test that
seeds generated SQL directly; it must not be reported as guarded-apply proof.
`DURIS_RESTITUTION_SCHEMA_ONLY=1` selects only schema replay/drift checks.

An original UID with complete serialized metadata can still be refused when
payload, custody, baseline, legacy location, timer/binding, or global competitor
checks disagree. A formerly unbound or missing-domain artifact is recoverable
only with the explicit reconciliation approval and all evidence gates above;
otherwise the tool records a refusal without changing artifact authority.

A production target is accepted only through the target-pinned production-policy workflow; no command stops or masks a runtime automatically. The native live-runtime handoff is distinct from offline SQL apply: `export` requires the exact SQL-derived plan, staff approval, actor, reason, and native revision fences, but does not require `--offline-proof` or a server-wide stop. Direct SQL `apply` remains offline-only and retains every backup, quiescence, custody, authorization, ownership, artifact, and idempotency gate.
