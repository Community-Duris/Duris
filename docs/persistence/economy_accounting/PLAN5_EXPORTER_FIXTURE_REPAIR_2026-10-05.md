# Plan 5 complete disposable exporter fixture repair

The existing partial-export fixture stopped on a missing selected child column
before running its audit assertions. Its manual schema now supplies the current
reader's columns, old inserts explicitly name their original fields, and exact
expected findings include the original-plan proof that these model roots lack.
The complete script passes on both supported engines. Production audit behavior,
shared schemas and mutation logic are unchanged; release remains incomplete.

## Source, ownership and delivery

- Branch: `codex/accounting-plan5`; worktree:
  `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `9d16dec34aa1b6df2f3dbe6a43ae35be0f4236aa`, the preserving merge of the
  refreshed primary into published Plan 5 head
  `e571c52e09bf3afc544beddb5aafef092a7cade4`.
- Refreshed primary consumed: `b844f70f99f22d867fb021fb48dd0287ef98e72a`.
- Native tree: `4847156a4488be7f3dab0e0107ce0c2b207e347b`.
- Migrations tree: `1b0f9a40fef29de409338ba83be015cd3390c9f5`, public0056.
- Owned files: `tests/async/run_economic_sql_audit_snapshot_mysql.py` and this
  report. No shared producer/coordinator, accounting contract, schema, registry,
  matrix or activation file is independently changed.
- Result SHA and remote verification: `tmp/plan5/exporter-fixture-delivery.json`.
- Evidence manifest: `tmp/plan5/exporter-fixture-evidence.json`.

## Reproduction and complete fix

Both disposable engines reproduce error1054:
`Unknown column 'e.domain_id' in 'SELECT'`, originating in `read_evidence`.
Source review establishes the remaining stale schema fields before repair:

| Fixture table | Selected fields now represented |
| --- | --- |
| `economic_accounting_child` | `domain_id`, `discriminator`, `relationship`, `receipt_operation_id` |
| `economic_accounting_item_reference` | `line_index` |
| `item_ownership_ledger` | Source owner type/ID/context and source/destination equipment slots |

The fixture retains its intentionally corruptible manual schema and does not
pretend to exercise canonical migrations. All existing reference rows use
event0/line0. Source-custody and child derivation facts not retained by the legacy
model remain null; they are not invented. Equipment slots for these carried
model items default to0. Explicit insert column lists preserve every original
value after adding columns, including the deliberate orphan child.

The initial selected nonbaseline model contains three committed roots without
original EAP1 capsules. Their exact `missing_original_plan:3` findings and
`original_plans_verified:0` are asserted. A temporary high-revision root adds a
fourth missing plan; the duplicate/revival sequence adds fourth/fifth roots and
restores the original count after fixture-owner cleanup. All earlier corruption,
scope, exception and unchanged-input assertions remain. No original plan is
manufactured from these projected rows. The separate native canonical SQL tests
continue to own original capsule authentication.

## Exact execution and retained attempts

The actual complete subprocess command is:

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
  ENVIRONMENT=test TEST_DB_DISPOSABLE=1 \
  ECONOMIC_ACCOUNTING_DISPOSABLE_SCHEMA=1 \
  DB_HOST=127.0.0.1 DB_USER=root DB_PASSWORD='' \
  DB_SOCKET=<fresh-private-socket> \
  /usr/bin/python3 -u -B \
  /workspace/tests/async/run_economic_sql_audit_snapshot_mysql.py
```

The ignored recipe `tmp/plan5/run-exporter-fixture-checks.py green-linux` starts
each daemon through the existing `persistence_restore.private_database` helper.
It supplies only that daemon's fresh local socket and test flags, never reads a
live environment file, pins native/migration/audit/test inputs before and after,
and preserves the exact executed sources, command, elapsed time and engine
version. Network is disabled. Immutable local Linux image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.

| Backend | Version | Complete script exit | Seconds including setup and artifact copy |
| --- | --- | ---: | ---: |
| MariaDB | `10.11.14-MariaDB-0ubuntu0.24.04.1` | 0 | 31.635 |
| MySQL | `8.0.46-0ubuntu0.22.04.4` | 0 | 52.271 |

Zero selected skips; all pinned sources remained unchanged. The complete script
preserves the SELECT-only audit role, denied mutations, consistent-cut and
rollback/cursor-close checks, byte/row restoration assertions and global CLI
refusal at limits0/1/100. Per engine, its existing printed fault groups include
27 source-grammar cuts,66 source-policy cuts and2 self-link cuts. Other sections
exercise unsupported guild/ship sources, all orphan families, a late lower-ID
commit, interruption, unsigned money/UID boundaries, checked copper overflow,
item lifetime/topology faults, historical/unattributed provenance and exact-ID
operator lookup. The whole script, including all later price-column/history
assertions, reaches its final success marker.

The namespace is `bin/tests/p5-exporter-fixture-20261005/`. All attempts remain:

- `red/`: first harness invocation was rejected by the unchanged disposable
  guard before any fixture assertion. Its source and receipt are retained.
- `red-selected/`: exact guarded execution establishes missing column1054 on
  both engines against the unchanged fixture.
- `green-first/`: the first repair reached further assertions. MySQL identified
  an inline positional orphan-child insert still requiring explicit columns;
  that is repaired. MariaDB's reversible table rename failed with missing
  tablespace194 in the Windows-mounted live data directory.
- `green-linux/`: fresh native Linux daemon storage completes the unchanged
  rename tests and every other section on both engines. The existing fixture
  guard is preserved. After each daemon stops, its database/log artifacts are
  copied into the evidence namespace before temporary storage is removed.

No failed attempt is counted as a pass or replaced. The stopped database artifacts
are qualification fixtures, not production data or an operational backup.

## Curator and remaining gates

The primary should retain the existing full disposable exporter recipe in central
registration and consume this two-file owned fix. Its corruption schema remains
separate from real0056 fresh/upgrade and native capsule classes. No interface or
schema change is requested. This report provides the curator handoff to the
primary's locally maintained notebook; notebook locality is not a blocker.

The public native source still emits EAB1. The primary's reviewed private EAB2
contract enables the next independent dual-version reader work, but no public
native/schema successor is installed or qualified by this slice. The previously
reported shared build initializers, baseline equipment repair, complete
producer/source capture, combined R1–R8, managed restore and retention/service
qualification remain open. Synthetic fixture assertions do not establish those
gates. Inactive behavior, wallet-root item exclusions and the declined inactive
spell change are preserved. No activation, deployment, PR merge, production
mutation, auto-correction or push to experimental-accounting occurred.
