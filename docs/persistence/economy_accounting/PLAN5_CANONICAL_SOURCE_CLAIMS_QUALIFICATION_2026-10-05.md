# Plan 5 canonical operator source-claim qualification

The standalone canonical SQL audit ignored the source-claim table. A fresh
native-authored committed root still received `canonical_roots_and_details:
verified` after its claim was deleted. The repaired reader requires the claim
source, bounds its count and independently checks exact coverage/ownership across
all retained books before original capsule verification. It never repairs a
claim or changes native authority. Full release remains incomplete.

## Source, delivery and owned files

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `95273da0514ac4a141bfd40ec4d3e428b69b3575`.
- Refreshed primary consumed: `9a02a0ee5d2e0490a8e09a710aac6af09f358b7d`.
- Native tree: `08727e151d8086c57c82449f2c456cf7dec45782`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, canonical0056.
- Result SHA/remote verification: `tmp/plan5/canonical-source-delivery.json`.

Owned files are `scripts/economic_sql_canonical_audit.py`, its existing
`tests/async/test_economic_sql_canonical_audit.py`, `AUDIT_OPERATIONS.md` and this
report. No shared native, producer, coordinator, schema, contract, registry/matrix
or activation file is independently changed. The existing JSON format and its
unqualified command/source-capture/release flags are preserved.

## Established defect and complete reader correction

Two focused tests establish absent row-budget and claim-consistency refusals;
the original reader passes its other ten tests but fails those two. Both native
configurations then encode a common original source in EAI1/EAP1 and create a
fresh authoritative SQL schema. Deleting its committed claim is allowed by the
schema. The old reader returns a verified report with one root,1168 canonical
bytes and23 queries. The original source event remains in both saved capsules.
The observation records rollback and unchanged data. The native test fails on
that false verification in MariaDB; both probes compiled/ran, but the red test
does not reach MySQL. All exact red sources and attempts are retained.

The reader now requires all15 InnoDB sources, including
`economic_accounting_source_claim`, and refuses more than100,000 claims before
original capsule reads. A fixed SELECT-only relational check requires:

- Every successful committed root with a nonnull source has its exact original
  `(operation_id,lineage,source_event,outcome)` claim.
- Every claim names an existing successful committed root, its nonnull exact
  source identity and lineage, and outcome1. Claims attached to rejected roots,
  source-free roots or another identity/scope refuse.
- Source/lineage pairs and operation owners are unique even if imported evidence
  has lost its schema uniqueness constraints. The preflight spans the database;
  selected/active epoch or lineage filters cannot hide a claim.

The independent EAI/EAP verifier then authenticates the root's original source
metadata and effects. No native mutation implementation is imported. Source-free
roots and rejected roots require no claim; they retain their existing original
capsule/refusal rules. SQL driver/output bounds, read-only repeatable read,
rollback/close, fixed diagnostic output and personal-alias exclusion remain.

The existing native fixture gains an explicit
`DURIS_PLAN5_CANONICAL_SOURCE=1` selection. It writes native original source
metadata and its matching claim, covers missing/foreign/changed/orphan claims and
restores each disposable cut through the separate fixture owner. Normal source
FK updates must fail with1452 and leave the fixture unchanged. Only the isolated
fixture owner temporarily disables its own session's FK checks to model corrupt
imported evidence; checks are restored before the read-only audits. The audit
account stays SELECT-only. Rejected-detail fixture setup temporarily removes and
then restores its own original claim so the native composite FK remains valid.
Original ordinary custody/child/posting/metadata/plan/refusal checks remain.

The fixture's observation files retain the actual result/refusal before an
assertion, so the red false verification is reviewable. The existing native
probe uses controlled valid source IDs and frozen metadata for a synthetic
common root; it does not prove a real producer issued that source or authenticate
a complete command receipt. A source claim is compared against original saved
metadata, never used to fabricate or reseal a root.

## Exact selected checks

Unit command:

```text
python3 -u -B -m unittest -v test_economic_sql_canonical_audit.CanonicalAuditTests
```

Native command, selected separately for source, ordinary player and native-mobile
fixtures:

```text
python3 -u -B -m unittest -v test_economic_sql_canonical_audit.NativeCanonicalAuditTests
```

Every native selection sets `DURIS_PLAN5_CANONICAL_NATIVE=1` and a fresh
`DURIS_PLAN5_CANONICAL_ARTIFACTS` directory. The source selection additionally
sets `DURIS_PLAN5_CANONICAL_SOURCE=1`; the mobile selection instead sets
`DURIS_PLAN5_CANONICAL_MOBILE=1`. The default selection sets neither. Native
SQL/flatfile builds are fresh, strict C++20 with ASan/UBSan; both private SQL
engines use actual bootstrap/immutable migrations through0056 and SELECT-only
reader accounts. Exact native sources/flags, commands, source hashes, binaries,
engine versions, constraint assertions, original bytes, API/CLI queries and
unchanged inventories are retained in each namespace.

All final pure/native selections pass:13 distinct methods,15 method executions,
zero skips. The12 pure methods pass in2.849 seconds. Each native selection runs
the same one method with both native configurations and both private engines:

| Selection | Seconds | API/CLI checks each | Accepted/refused each | Fresh SQL/flatfile binary SHA-256 |
| --- | --- | --- | --- | --- |
| Original source |261.450 |48 |22/26 | `04e3303e70655bf293d671d6f2136cc9ed7b719006e91d5c9df07249d7ab21c0` |
| Source-free player |259.637 |38 |20/18 | `49a4c88f350eb5c956ca2799f613adfed1c030fd6ef6d906d7be25feb44c3b09` |
| Source-free equipped native mobile |259.432 |38 |20/18 | `d85aa4fd903c223eaac01bae2e0259e5c669a0ea749ad82d2f428c49c45ab74f` |

Together these are124 API and124 CLI checks,62 accepted and62 refused per
interface. Every API call rolls back/closes and every audit leaves its fixture
inventory unchanged. Six SELECT roles reject UPDATE with1142. Existing three
schema-check cuts per selection/engine remain enforced, and all six new
foreign/change/orphan claim FK assertions fail normal owner writes with1452
before the isolated corruption cuts. Both private versions are MariaDB10.11.14
and MySQL8.0.46. Source metadata in each original native EAI/EAP is checked;
default/mobile plans remain source-free. No native binary is reused.

The current canonical reader also reruns the preceding equipped-baseline
retention/refusal harness:

```text
python3 -u -B tmp/plan5/run-baseline-equipment-sql.py
```

This verifies the previous sealed original native capsules and unchanged native
inputs before using those retained bytes; no baseline binary reuse is claimed.
It creates four new source schemas and four cold same-engine restore daemons,
uses complete canonical0056 dumps and compares all selected retained SQL values
before origin/canonical API and canonical CLI checks. It preserves the carried
positive and conservative equipped-original refusal for both owner1/owner12.
The origin CLI still lacks a Unix-socket option for these TCP-disabled daemons;
that interface is unselected and disclosed. Native repository execution and
service/gameplay restart are not supplied by these fixture writes/imports.

The current-reader baseline selection passes in448.771 seconds:24 API checks
(eight accepted,16 refused),12 canonical CLI checks (four exit0, eight exit2),
four full dump/cold restore pairs and zero selected skips. Every original SQL
value is preserved, every reader call rolls back/closes, all12 SELECT-role UPDATE
attempts refuse, and audit/dump inventories stay unchanged. Equipped EAB1
disagreements still refuse without a report or correction. Combined current
coverage is148 API checks and136 CLI checks; the canonical reader accounts for
136 of each, with66 accepted and70 refused. The additional12 origin API checks
use the unchanged independent reader. All proof remains scoped to these exact
native bytes, canonical schemas and selected fixture journeys.

All checks run in immutable image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`
with `--network none`, read-only `/workspace`, writable `/workspace/bin` and
private database privileges required by the existing helper. No live `.env`,
existing database, live game or production data is used. The native corpus is
synthetic; the claim row-cap check is a focused refusal test, not a release-host
workload/growth qualification. No CI result is required or claimed. No maintained
server build is rerun for this Python/docs change.

| Final executable input | SHA-256 |
| --- | --- |
| Canonical SQL reader | `b9f38fd19c471e84fbfd7c71129246d0c8a95ade787757d2f26949b49cf744ec` |
| Extended canonical unit/native fixture | `20b1f390b3d5d2f2a19c9c12c0861ccc6c0f2fc75845e6bc69990bbc9950a8c9` |
| Unchanged independent EAP reader | `8dc9aa2e4c1f7a08178854b3410f054a31c696a94765a9b251526d0b13ef4613` |
| Unchanged independent EAB/origin reader | `9d1a5f03950c61b8794eb066c1456fd0ce1d434b1d05cbc4929f9baf0844d620` |

## Evidence, registration and remaining gates

New ordinary/source/mobile evidence is under
`bin/tests/p5-canonical-source-20261005`; current baseline evidence is under
`bin/tests/p5-eab-slot-20261005-final/sql-canonical-source-current`.
`tmp/plan5/canonical-source-evidence.json` seals every new/failed/passing artifact,
exact executed sources and commands, and inherits the preceding sealed index.
No new full old-archive rehash is claimed. `canonical-source-delivery.json`
verifies the committed owned bytes and requested remote branch. Protected
capsules, database files/dumps, binaries, logs and generated sources stay local
and ignored. The preceding equipment handoff remains at its original source
pins; its conservative findings are not relabeled as a completed native repair.

The primary owns registration: raise the existing canonical pure class's count
from10 to12, include the source-claim row-budget and database-wide mismatch
methods, and register the existing native method's additional source selection
with `DURIS_PLAN5_CANONICAL_SOURCE=1`, fresh artifact directory, both native
configurations, both SQL-engine markers and zero-skip enforcement. Preserve its
default and mobile selections. No native field, migration or response schema
change is requested; the source-claim fields/invariants are already retained by
canonical0031. Keep writer/producer completion status unqualified.

This is the curator handoff for the primary's locally maintained notebook;
notebook locality is not a blocker. Plan5 can keep progressing independently.
The separate EAB1 equipment/admission-time format gaps, complete native census
and source capture, actual producer/writer journeys, maintained builds, both
backend gameplay/fault/lost-reply, retained restore/retention/service journeys
and release-host operation/storage-growth budgets remain. Earlier shared build,
SHOP/route contract and publication findings are not newly executed or waived.
No activation, auto-correction, deployment, PR merge, production mutation or
experimental-accounting push occurs. Inactive behavior, wallet-root exclusions
and the declined inactive spell change are preserved. The primary integrates
completed slices and publishes/qualifies the tested combined release.
