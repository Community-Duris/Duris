# Plan 5: maintained SQL collector row comparison build handoff

Branch `codex/accounting-plan5`; worktree
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
Refreshed primary is `9155b623419b960c17f117ed8c87d8f783ce42a1`.
Frozen tested base is `acccc4b816bc43059b38a91c83e0e78adff936e4`.
Native tree is `8352e470e9d32bee2fc84cb90097d0ecfb87ce2d`; migration tree is
`1b0f9a40fef29de409338ba83be015cd3390c9f5`, including canonical 0056.
The result commit and verified remote branch SHA are supplied in delivery.

## Established native compiler defect

Plan 5 next attempted the maintained SQL build needed for managed full-dump,
schema/history/value recovery and real isolated service boot on both engines.
The checkout and all prior artifacts are preserved; fresh explicit build paths
avoid overwriting workspace objects or server binaries.

```sh
make -C src -j2 PERSISTENCE_BACKEND=mariadb \
  OBJDIR=/workspace/bin/tests/plan5-sql-managed-current/objects-sql \
  DMS_BINARY=/workspace/bin/tests/plan5-sql-managed-current/server-sql
```

The build exits 2 in 83.4324473 seconds: 173 compilation commands, 172 completed
objects, one compiler error, and no server executable. The exact failure is:

```text
economy/collector_purchase_publication.c:278:47: error:
comparison of integer expressions of different signedness:
'int' and 'uint32_t' {aka 'unsigned int'} [-Werror=sign-compare]
if (!selected || selected->db_item_id != row)
```

The attempted backend uses SQL/MySQL headers and the maintained C++20 hardening
and warning flags, including `-Wall -Wextra -Wpedantic -Werror`. No warning is
suppressed. The actual error is in `literal_matches`, inside the
`#ifndef __NO_MYSQL__` section at lines 45–291. The preceding qualified flatfile
build does not compile that section and cannot establish SQL compilation.

## Exact fields, invariants and consumers

- `obj_data::db_item_id` is the existing signed `int` in `src/core/structs.h:523`,
  retained for incremental native saves.
- `collector_command_result::materialized_item_id` is the existing `uint32_t`
  in `src/economy/collector_command.h:83`; it is the `row` argument here.
- The accounted repository's native materialization in
  `src/economy/collector_repository.c:1284` reads `mysql_insert_id`, refuses zero
  or values above `INT_MAX`, and only then converts to the result field.
- `collector_purchase_publication_owner::native_publish` calls the comparison
  in its online committed path after the effect callback and singleton census.
  It must prove the exact native row and one-item literal bytes before accepting
  the final runtime registry proof. Failure retains the existing refusal path.
- The public `collector_purchase_publication_attempt` contract in
  `collector_transaction.h` retains original guarded ACK and save-hold release,
  session/current-history revalidation and callback reentry conflict checks.

No shared field, representation, schema, command encoding, return contract or
consumer signature change is requested. Primary owns the actual local comparison
repair, formatting and corresponding shared raw registry/matrix pins.

A narrow diagnostic proposal is:

```cpp
if (!selected || selected->db_item_id < 0 ||
    static_cast<uint32_t>(selected->db_item_id) != row)
    return false;
```

This preserves equality for all valid positive IDs in the repository's bounded
domain and refuses a negative native ID before conversion. Zero equality remains
as before at this local comparison; the upstream successful materialization
requires a nonzero ID. No SQL unsigned row can alias a negative native ID through
the old implicit conversion. The following singleton tree capture, encoded size
and literal byte comparison remain exact. No native row, player snapshot,
ownership/custody, receipt conflict, current projection, publication/ACK or
save-hold check is removed or weakened. Inactive dispatch and the canonical
never-admitted/no-effect path are untouched.

## Executed diagnostic and limits

Docker image `duris-plan5-origin-sql-tools:local` has immutable ID
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
The source checkout is read-only, outputs are confined to `bin/`, and networking
is disabled. The diagnostic recorder alone also writes `tmp/plan5/`. No `.env`,
database daemon, existing runtime, production data or external service is used.

`python3 tmp/plan5/probe-sql-collector-row-comparison.py` extracts the exact
failing translation unit's compiler flags from the maintained build log. It
replaces object compilation with `-fsyntax-only` and writes fresh dependencies.
It runs the unchanged original as a control, then a copied translation unit with
only the proposed condition replaced:

| Input | Result | Elapsed |
| --- | --- | ---: |
| Unchanged actual original | Exit 1, same signedness error | 1.444982 s |
| Copied diagnostic candidate | Exit 0, strict syntax pass | 1.893923 s |

Original raw SHA-256 is
`7ca24ec6b64fb1a1a12517d39efa26bf39629f01b475ac972c507a98a9800f36`.
Copied candidate SHA-256 is
`51478b2dfadb97c5aa910735a0337238e8aee964a3150b97fe80b0f9912ba9d1`.
The original source remains byte-for-byte unchanged. Full commands, stdout/stderr,
dependency outputs and timings are recorded in `diagnostic.json` and its logs.
The copied diagnostic is not a repaired maintained server, executed publication,
native regression pass, database qualification or release result.

The early full-build stop left 543 translation units unreached. To establish
the remaining syntax scope without repeating the original 173 attempts,
`python3 tmp/plan5/check-unreached-sql-translation-units.py` obtains all 716
maintained commands through a fresh-path `make -n`, excludes those 173 sources,
and runs the remaining commands with the same SQL/strict flags and
`-fsyntax-only`, fresh logs/dependencies and two workers. All 543 pass in
217.875374 seconds; no additional syntax error is found. All 1,490 native and
migration inputs are checked unchanged before and after the scan. The complete
command/result/source-hash list is `unreached-syntax/results.json`.
This is syntax evidence only: code-generation/optimizer warnings, object
compilation, linking and a running SQL server remain unqualified for those
543 units. The known failing original is excluded from this separate scan.

Primary validation after the real repair must keep strict flags and cover valid
matching IDs through `INT_MAX`, mismatches, negative native IDs, upstream zero
refusal, and literal/custody disagreement. The actual SQL collector successful
publication, historical retry, callback conflict and held-ACK/save replay checks
remain required on both engines. Existing focused collector fixture wrappers do
not explicitly compile `collector_purchase_publication.c`; their isolated passes
cannot substitute for the failing maintained translation unit and real route.

## Evidence, skipped execution and curator handoff

Owned tracked file: this report only. No native/server or shared coordinator,
contract, producer, schema, registry/matrix or central registration is edited.
The failed build is terminal and is not rerun on unchanged source.

Evidence root is `bin/tests/plan5-sql-managed-current/`: complete maintained log,
command/timing and build metrics, 172 objects and dependency outputs, original and
candidate source copies, two strict syntax logs/dependencies, diagnostic metadata,
543 unreached-unit strict syntax results/logs/dependencies, copied drivers/recorder
and frozen tree entries. The planned copied-checkout input
list freezes 3,124 tracked source/helper/runtime entries; no copied checkout or
SQL restore test is claimed executed. Final manifest is
`tmp/plan5/sql-managed-current-evidence.json`.
It verifies all 1,254 native and 236 migration inputs, all 3,124 frozen planned
qualification inputs, and all 28,904 protected prior evidence files unchanged
after the completed diagnostics. It records 1,450 fresh evidence files.
Manifest SHA-256 is
`2ceeee2b9b169c543794e3aef6b4e4e8b1db1459b97960a28b332a99f1ea3c9b`.

Both existing managed SQL cases
`PersistenceRecoveryIntegration.test_mariadb_full_dump_schema_history_values_and_isolated_service_boot`
and `test_mysql_full_dump_schema_history_values_and_isolated_service_boot`
are not started because the selected maintained SQL server was not produced.
No disposable database is created in this slice, and no build/database gate is
waived. Fresh managed SQL backup/import/service qualification can proceed after
primary publishes the actual repair, on a newly frozen base and evidence path.

The prior [managed v3 flatfile qualification](PLAN5_V3_MANAGED_RESTORE_QUALIFICATION_2026-10-04.md)
and [both-engine baseline fence qualification](PLAN5_SQL_BASELINE_KEYS_HASH_SLICE_2026-10-04.md)
retain their own exact sources and scope. Pending 0057/0058, original baseline
admission-time command authentication, real source capture/install, producer
journeys, complete workload/coverage and combined release gates remain open.

Primary maintains the shared notebook locally through the curator workflow,
per the user's clarification; this exact report supplies the handoff and notebook
access is not a blocker. Accounting stays inactive. Wallet-root item exclusions
and the declined inactive spell-path behavior are preserved. No activation,
audit correction, production mutation, deployment, PR merge or direct push to
`experimental-accounting` occurs.
