# Additive claim-consumption schema0062 qualification

Primary integrates the qualified successor of source `f528a46b43e07444b97121a6e63cb9be414fb5d0`
at preintegration head `8dc0f98eb01bee9e40d227debb3e556d2d0fdbcf`. This solves the missing
registered schema contract for immutable partial pending-claim debits and explicit
new opening origins. Auction, baseline-origin creation, spending, publication and
recovery implementations remain separate unfinished slices. No Plan or release
gate is marked complete, and accounting remains inactive.

## Registered change

Immutable0062 adds `economic_pending_claim_consumption`, keyed by original spending
operation, source operation and source slot. Positive amount/slot constraints,
both restricted foreign keys, and the original source lookup index are enforced.
Original source rows, amounts and legacy whole-consumption links remain unchanged.
The baseline witness gains nullable `claim_origin_version`: historical rows stay
NULL; version1 is reserved for authenticated new opening-origin creation. No
mapping, origin, balance, epoch or activation decision is backfilled.

All three migration manifests append the same sealed0062 entry. Their previous61
entries and all126 original immutable files remain byte-identical. Runtime
manifest and compiled contract now bind62 receipts and229 tables, with measured
engine-specific complete metadata. Runtime signedness/CHECK enforcement scopes
include both retained pending-source and consumption tables. Read-only exact
five-table verification and current three-table baseline metadata use the actual
measured engine forms. No old immutable verifier is edited.

The lifecycle manifest registers the new table with its source/root dependencies
and the existing protected replay/restore horizon. The normal lifecycle schema
inventory includes0062. Exactly two existing writer source pins are refreshed,
and the matrix is regenerated without changing route classifications or coverage.
Controller decisions, erasure/export/retention qualification, independent reader
consumption and full backup/restore remain original release obligations.

## Original qualification

The original canonical61 runner plus additive0062/rerun produced exact five-table
component fingerprints on MySQL8.0.46 and MariaDB10.11.19. Those observations
register immutable verifier checksums before the complete qualification.

The original10 runtime compatibility methods pass with zero skips. The unchanged
complete migration-fork recipe, `test_staging_migration_fork_mysql.py
--update-contract --report <private-report>`, then passes in
1146.031 seconds. Both engines qualify:

- Fresh canonical history and immutable staging0045/master0031 upgrades.
- Exact first45/31 receipts and native runtime payload preservation; all receipt
  rows remain unchanged across idempotent reruns.
- Duplicate refusal before DDL without deleting evidence.
- Same-session migration lock/quiescence, concurrent exclusion, failed history
  CAS rollback, killed-session non-reconnection, cancellation/allocation abort,
  and SQL/output-limit/timeout fences.
- Shell and actual compiled boot predicates on all six databases, including
  refusal of old-receipt edits, mixed-fork state and generated-expression tamper.

Full runtime metadata fingerprints are `cac2ac37e272a6c7f9eb77406f466f4b2e219a7a8e22e01a6bfa95765117acfd`
(MySQL) and `49d2b98fcbff08d2554fa508d629ec660e4fcba5c7541b758dbebb6731e20fff` (MariaDB).

Qualification exposed two stale hardcoded55 receipt assertions in the original
staging/master test and a history helper whose default silently stopped at55.
The assertions now use the selected complete manifest length; the default history
comparison uses all registered receipts. Explicit historical45/31 prefix checks,
native payload controls, original flags, faults and tamper assertions remain.
Failed attempts/logs are retained; only the failed gate was rerun.

On the actual integrated source, manifest, lifecycle, runtime source, normal
accounting contracts and matrix checks pass. Both touched C/C++ files pass
clang-format18; the one header formatting repair preserves all literal values.
The original full production Make recipes retain all warnings and hardening:

| Backend | Fresh objects/dependencies | Result | Seconds |
| --- | --- | --- | --- |
| MariaDB | 738/738 | PASS, no warnings/errors | 1332.685 |
| Flatfile | 738/738 | PASS, no warnings/errors | 1276.790 |

The builds use raw authenticated source bytes in separate owned offline
containers, without runtime credentials, private player data or unrelated WIP.
Existing scanner results remain920 routes and2818 unique mapped sites;
`coverage_complete=false` and `release=BLOCKED` remain explicit.

## Retained evidence and remaining integration

Private original source/engine measurements, attempt logs, terminal reports and
immutable preimages remain in `tmp/sql-money-opening-schema-source-20261006`.
Formatted input receipt SHA256:
`c2bf4b8bf3928770a0a0f3cf31088880aa3f9204104d37c311367093efe62de1`.
Original integrated checks, source archive/pins, build logs, all objects and
servers remain in `bin/tests/claim-consumption-schema-primary-20261006`.
MariaDB server SHA256 `e75280fe14be3f8943458a8a24cab4b9c28af46da41104553ec88b7ff76796bc`;
flatfile server SHA256 `a0f2236b3b0e4bb73ea2eb0bb322057e7dc72a43bd6a02eafc569128496806c2`.

The [money-opening handoff](SQL_OPENING_AUCTION_CLAIM_IMPLEMENTATION_HANDOFF_2026-10-06.md)
retains the source/origin representation contract. Its reviewed source successor
and separate first-endpoint slice still need primary producer/lifecycle/provider
composition and the original native, gameplay, spending, replay/rollback/lost-reply
and cold recovery batch. Independent Plan5 readers must consume actual new
allocations and original lifetimes. This schema qualification grants no activation
authority and does not certify those routes, a production clone or full R1–R8.

Inactive behavior, the declined inactive spell-path boundary and all three
unrelated worktree changes remain intact. No production DB, live game, deployment,
merge or history rewrite is used.
