# Plan 5: independent original-operation identity audit

Delivery branch: `codex/accounting-plan5`, published separately from
`experimental-accounting`. Worktree:
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Slice base: `e9107dcddf381c0baeef0e8be2d11f8df8e5f4cf`.
The result commit is the commit containing this report and is recorded after
commit in `tmp/plan5/original-link-evidence.json`.

The refreshed canonical remote remains
`f7d26eaa721cd3b675c0b0c65009a2535813f400`. Native source remains tree
`d9a9610f3a6c72c14a7d1870d88fed8c018fa87b`, 1,232 tracked native inputs,
from the canonical 0056 candidate. Primary-owner unpublished integration is
outside this qualification. Earlier 0055 results do not qualify this slice.

## Established defect and complete selected-root fix

Native metadata rejects an operation whose original ID equals its own ID,
including reasons for which an original is optional. The old reconciler checked
only whether a required original appeared among selected roots. A self-link
satisfied that lookup and optional originals received no identity check.

At the slice base, a strict ASan/UBSan fixture against seven production sources
in SQL and flatfile modes proved six native decisions: optional absence and
distinct originals are accepted; required absence and both self-links are
refused; a distinct required original is accepted. Both corrupt snapshots were
reported clean by the old reader in each mode, with unchanged inputs. The RED
driver/result are `tmp/plan5/reproduce-native-original-link.py` and
`tmp/plan5/native-original-link-red.log`. An initial fixture compilation used
the wrong enum spelling; its terminal failure is preserved separately in
`native-original-link-compile-failed.log`. It is not native qualification.

The independent reader now validates every present selected root's existing
`original_operation_id` as a nonzero, lower-case, exactly 16-byte ID and refuses
self-reference with `invalid_original_operation`. This applies to committed
and rejected outcomes and optional and required originals. Malformed values
are diagnosed before any hash lookup; list/dict values cannot bypass the check
or cause an unhandled lookup failure. Null/absent optional originals and valid
distinct optional IDs retain their current behavior. Valid required IDs still
require a selected original root; absent or unobserved required originals keep
`missing_original_operation`. The reader does not invent an original root.

Four focused methods protect malformed types/lengths/case/zero values, self
links, legitimate optional/distinct links, required absence/unobserved links,
rejected roots and operator CLI behavior. Exception views at limits 0/1/100
exit 1, retain the global exception and leave input bytes untouched. A validly
encoded self-link remains visible in an operation view with global refusal;
malformed IDs refuse that ID-only view with exit 2 and do not expose aliases.

Owned files:

- `scripts/reconcile_economy_accounting.py`
- `tests/async/test_reconcile_economy_accounting.py`
- `tests/async/run_economic_sql_audit_snapshot_mysql.py`
- this report

The fix uses existing audit fields and requires no schema/interface change.
Shared coordinator, contracts, producers, registry/matrix and activation-owner
files are untouched. Accounting remains inactive; wallet-root item exclusions,
the declined inactive spell change and active blackjack refusal are preserved.
Reading retained stake primitives does not enable gambling.

## Native and disposable database qualification

The existing explicitly enabled `NativeStakeSQLTests` fixture compiles eight
production sources with C++20, strict warnings as errors, ASan/UBSan and no PIE
in both backend modes. It emits native EAI1 intents/EAP1 plans, 104 source
grammar decisions, 1,107 source-policy decisions, and six new original-link
decisions. The independent reader agrees with all six native link decisions.
Both native modes produce identical fixture bytes. New artifacts live under
`bin/tests/plan5-original-link-sql`; earlier stake, grammar and policy artifacts
are preserved and their hashes checked.

Each actual canonical engine has a fresh private datadir/Unix socket with TCP
disabled, bootstrap adoption and migrations through
`0056_spell_ward_durability`. A fixture owner seeds the native bytes and known
opening/native anchors. A separate SELECT-only reader is denied UPDATE with
error 1142. Canonical foreign keys/checks remain enabled. In particular, the
original-operation FK targets the inbox and permits self-reference to an
existing inbox row; it cannot substitute for the native metadata invariant.

Each engine reads 41 cuts, including two new self-link faults and their exact
restorations: the optional-original retained stake root and required-original
terminal payout root. Each fault has exactly `invalid_original_operation`.
Every read uses SELECT/transaction statements, one rollback/cursor close, and
leaves all seven compared SQL tables and in-memory inputs unchanged. Existing
money, grammar, reason/source-policy and clean restoration checks still pass.
These are native metadata/evidence-reader tests with supplied opening anchors;
they do not qualify gambling gameplay, authority publication or replay.

The broader guarded SQL runner also passes both engines with its explicit
minimal fixture DDL, including two self-link faults per engine in selected
money/UID roots. These retain the original three incomplete/unsupported
refusals and add exactly one link exception. Every fault preserves all 21
compared tables and verifies rollback/close and byte-identical restored cuts.
The existing 54 grammar and 132 source-kind fault captures remain passing.

## Commands and evidence

Linux jobs use `duris-plan5-origin-sql-tools:local`, image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`:
Ubuntu 24.04, Python 3.12.3, GCC 13.3.0, PyMySQL package 1.0.2-2ubuntu1.1,
MySQL 8.0.46-0ubuntu0.22.04.4 and
MariaDB 10.11.14-MariaDB-0ubuntu0.24.04.1.

Common invocation is `docker run --rm --mount
type=bind,source=<worktree>,target=/workspace,readonly --workdir /workspace
<image> <command>`. Native jobs additionally mount `<worktree>/bin` read-write
at `/workspace/bin` and pass the explicit gate with Docker `--env`. No ports
are published or capabilities added. Disposable helpers strip checkout
credentials/environment. The broader existing loopback harness changes only
its new private daemons to ephemeral 127.0.0.1 ports inside the container.

| Command inside container | Result | Evidence |
| --- | --- | --- |
| `DURIS_RUN_STAKE_SQL_INTEGRATION=1 python3 -u tests/async/test_reconcile_economy_accounting.py -v` | 88 PASS, zero skips, 246.705 seconds; both native modes, six link decisions per mode, both fresh canonical engines through 0056, 82 read-only cuts including four self-link faults | `tmp/plan5/original-link-qualified-green.log` |
| `python3 -u tmp/plan5/qualify-source-event-snapshot.py` | PASS both engines; four new self-link fault captures, all prior broader checks passing | `tmp/plan5/original-link-snapshot-green.log` |
| `python3 -u tests/async/test_data_lifecycle_manifest.py -v` | 22 PASS, zero skips, 41.551 seconds | `tmp/plan5/lifecycle-current-manifest.log` |
| `python3 -u tests/async/test_account_erasure.py -v` | 7 PASS, zero skips, 2.242 seconds | `tmp/plan5/lifecycle-current-erasure.log` |
| `python3 -u tests/async/test_personal_data_export.py -v` | 6 PASS, zero skips, 1.943 seconds | `tmp/plan5/lifecycle-current-export.log` |
| `python -m py_compile scripts/reconcile_economy_accounting.py tests/async/test_reconcile_economy_accounting.py tests/async/run_economic_sql_audit_snapshot_mysql.py` | PASS on host | evidence manifest |
| `git diff --check` | PASS | evidence manifest |

Initial focused run: 87 PASS, one explicitly gated native/SQL skip, 3.227
seconds (`original-link-unit-first.log`). The enabled run supersedes that skip
for qualification. `original-link-frozen-inputs.json` pins all three executable
changes before final tests. `record-original-link-evidence.py --committed`
checks frozen inputs, native source identity, result ownership and prior
artifacts, and records the result SHA, image, commands, logs, drivers, migrations
and SHA-256 values in `original-link-evidence.json`.

## Narrow shared interface handoff: durable metadata headers

Owner: the primary accounting-contract/integration owner. No requested field
or contract change is implemented by this slice.

`operation_rows` and retained-root normalization in the SQL capture omit
metadata headers already present in canonical SQL/native evidence. The same
RED native probe proves policy/compiler version 2 and a mismatched actor kind
are refused while version 1/domain actor is accepted. Canonical SQL checks
require positive policy/compiler versions and actor kind 1 or 2, rather than
the current exact version and reason/actor pairing. These are direct source
and native-metadata observations; SQL header fault capture is not claimed here.

Requested fields, preserving existing SQL/native names:

| Field | Required independent invariant for current version |
| --- | --- |
| `accounting_version` | exact integer 1 |
| `policy_version` | exact integer 1 |
| `compiler_version` | exact integer 1 |
| `writer_id` | exact integer 1 through UINT32_MAX |
| `actor_kind` | exact integer: operator 2 for reasons 38-42; domain 1 for other current reasons |
| `actor_id` | exact integer 1 through UINT64_MAX |

Carry those six fields directly in selected `operations` and retained normalized
root records. For joined source-claim and pending-claim source/consumer records,
request a bounded `operation_metadata` / `source_operation_metadata` /
`consumer_operation_metadata` object with those exact six fields. Also retain
the existing `original_operation_id` in any out-of-scope root metadata used
for linkage. Missing headers must leave metadata qualification explicitly
incomplete; they must not receive default version/actor IDs. The contract owner
must decide required-field/version compatibility for old audit artifacts before
the reader/exporter is tightened. Canonical SQL columns already exist; no
migration or mutation schema change is proposed.

Consumers: `operation_rows`/`read_evidence`, retained lineage source claims,
lineage UID roots, mapping creation/retirement roots, pending-claim source and
consumer roots, coin-pile lifecycle root evidence, and lineage realized-price
roots; their corresponding independent `Reconciler` consumers and bounded
operator operation view. A selected joined root must agree with authoritative
selected metadata; prior-epoch roots must carry their own observed metadata.
Baseline opening/install metadata must remain compatible with its explicit
native witness authority rather than being inferred from a current mapping.

Requested tests: native version/actor/writer/ID bounds and missing/type-invalid
headers in both backend modes; actual SELECT-only canonical SQL corruption
cuts for policy/compiler version 2 and wrong actor kind on both engines through
0056 with all CHECK/FK constraints enabled; selected/joined/prior-epoch agreement,
each retained root family, safe bounded operator output, exact rollback/close,
unchanged authority and explicit legacy-artifact refusal/incompleteness. Any
needed registration belongs to the shared coordinator owner. Structural
metadata checks do not prove writer entitlement, actor permission, plan-row
binding or historical original-operation authority.

## Remaining gates and notebook status

Central qualification registration remains a primary-owner handoff: enable
`NativeStakeSQLTests.test_native_stake_sql_both_engines` with
`DURIS_RUN_STAKE_SQL_INTEGRATION=1`, both native modes, both actual engines and
native/PyMySQL tools; retain the broader guarded snapshot runner. This fixture
now includes all six original-link decisions and four canonical self-link
faults. Shared coordinator files remain untouched.

The lifecycle inspection finds protected retained accounting stores and
disabled general erasure/shared export pending controller/disclosure decisions.
The 35 current lifecycle tests are policy/inventory and protocol evidence only.
They do not prove native account erasure retains every accounting identity,
approve a purge horizon, authorize disclosure, or enable export. The existing
native deletion paths and R8 governance/real lifecycle journeys remain open.

This slice does not qualify out-of-scope original roots, complete metadata
authority/entitlement, all durable plan/row bindings, complete flatfile audit
capture, populated upgrades, gameplay fault/restart/replay, or measured budgets.
Earlier restore/origin results remain separate source-specific component
evidence. Full backup/restore, auxiliary origins/invariant tests and maintained
server builds were not rerun: their source inputs are unchanged by this read-only
audit fix. Native fixture compilation is the appropriate C++ check here.

The previously requested Plan 1 retained-baseline marker and other outstanding
shared interfaces/registrations remain open. The primary must integrate and
publish a tested combined candidate including canonical 0056. This branch's
isolated passing tests do not certify that candidate or release completion.

`AI_CONTEXT.md` and the required curator workflow/notebook reference remain
unavailable after repository/environment discovery; the existing input request
is unanswered. This report is an evidence handoff, not a curator notebook
update. Notebook upkeep remains a specific external-input dependency. No
accounting activation, production access, automatic correction, merge or
deployment occurred.
