# Plan5 coin-payload source bounds

The SQL exporter now refuses oversized coin payload sources before selecting
their bytes. It checks both stored coin payloads and their mapping-join
projections, and excludes non-coin payloads from both data queries. Actual
SELECT-only MariaDB/MySQL observations establish the previous late refusal and
the corrected boundaries. The snapshot format and native writers are unchanged.

## Branch, exact source and ownership

- Local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`.
- Base: `56d1f79733c7d9360e3d46c5304a39dd0f7ebeea`, normally merging the
  refreshed primary `d0fbb37397dc151686625c9eb2975085e2af016d` over Plan5
  `176b612c8d574211a2f0d736996e4d44fd36aef6`.
- Result: the commit containing this report; the post-publication delivery
  receipt records and verifies its full local/remote SHA.
- Tested source: the full committed public snapshot of that base plus the
  exact three executable-file changes below. Documentation changes do not
  alter those tested executable bytes.

| Tested file | SHA256 |
| --- | --- |
| `scripts/economic_sql_audit_snapshot.py` | `3d7363ed326f9d2d3248bb5d5318df7a0c58b5e58996286b623d8f94f9d92d03` |
| `tests/async/test_economic_sql_uid_scope.py` | `506738a6ff1bfa611778a5acdce6f164f70c6a1f4066293215382b4029e6cf10` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `c5d8a9f81686a91ac0c2d406c412a4c4b8a7850bbdd9eaeaa46fc3e38af627c8` |

Owned files are those three files, `AUDIT_OPERATIONS.md` and this report.
Shared coordinator/contracts/producers/schema/registry/matrix/activation files
are untouched. All earlier branch work and follow-ups remain on this branch.
Native and migration trees remain the recorded current61 trees
`bf7a92a728ad9b5b813626462e56533f8ba39c97` and
`2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d`.

## Established defect and complete reader fix

The original `read_native` mapping query fetched `i.coin_payload` before its
later coin-source aggregate check. Buffered client results therefore contained
oversized payloads before the advertised source limit could refuse them. Both
mapping and native-item queries also fetched payloads belonging to non-coin
items, although those bytes were ignored when building the snapshot.

The two new regression methods fail on the original reader with five failing
subcases: missing bounds, excessive rows, aggregate bytes, individual bytes
and repeated mapping projections all encounter a payload SELECT first.
The original-reader bytes and exact RED log are retained. On each actual SQL
engine, a4 MiB+1 payload is fetched before the old per-item decoder refuses:
4194530 bytes from two payload values, including the unchanged225-byte coin.
The original source-level ordering defect is thereby reproduced independently
of a mock cursor or a release inventory.

The reader moves the existing collection aggregate check ahead of both data
queries, and adds the existing4 MiB decoder ceiling to that preflight. It
separately sums the exact coin payloads selected by the lineage/backend mapping
join, counting repeated joined bytes rather than deduplicating native UIDs.
Both payload projections use a fixed coin-VNUM `CASE` expression so unrelated
payloads return SQL NULL. No limit is relaxed; exact byte-ceiling inputs remain
accepted. All observations stay in the same read-only consistent transaction.
The audit neither imports mutation logic nor repairs a source discrepancy.

## Actual native, SQL and regression evidence

Pinned image:
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Actual Python3.12.3/GCC13.3/MariaDB10.11.14/MySQL8.0.46 are used in one
named container, no network or host ports, read-only root and private RAM
workspace/database filesystems, two CPUs and4 GiB memory. Four private daemons
run sequentially and stop through the original restore database helper.
Neither source `.env` nor production credentials/data are used.

The archive contains6184 regular public source/helper files and the two
committed help-file links. Full archive SHA256:
`b0a65c4a9567fa247049c42556b1c268ec42c4f02fcbbab320a7ea0f85fbd53b`.
Every archived source/helper byte is verified before/after execution; no live
checkout is mounted. The archived original exporter supplies the RED runs.

| Command/scope | Result |
| --- | --- |
| Windows `python -B -m unittest discover -s tests/async -p test_economic_sql_uid_scope.py -v` |11 tests PASS; new methods previously RED;0 skips |
| Frozen Linux `python3 -u -B -m unittest -v test_economic_sql_uid_scope test_ship_coffer_audit test_economic_sql_audit_origins.ItemRevisionTests test_economic_sql_audit_origins.OriginTests test_economic_sql_audit_origins.BaselineVersionTests test_reconcile_economy_accounting.ReconciliationTests` |171 methods PASS,0 skips;24.723342s wrapper |
| Original `tests/async/test_player_item_snapshot_codec.py`, called through its retaining observer | native codec PASS;2.386335s; original C++20/strict-warning recipe, cases and assertions unchanged |
| `python3 -u -B tmp/plan5/run-coin-budget-original-exporter.py`, once per engine | expected RED exit1;4194530 payload bytes fetched before old size refusal |
| `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`, MariaDB | PASS exit0;7.839982s; all six new observations and existing cases |
| Same unchanged entry point, MySQL | PASS exit0;8.387145s; all six new observations and existing cases |

Each engine checks these actual reads:

| Case | Selected payload bytes | Result |
| --- | ---: | --- |
| Individual4 MiB+1 |0 | refused before payload read |
| Stored collection exactly32 MiB |67108864 total across the two32 MiB projections | accepted; valid modeled snapshot-codec strings |
| Stored collection32 MiB+1 |0 | refused before payload read |
| Repeated mapping joins above32 MiB while stored payloads remain below the limit |0 | refused before payload read |
|4 MiB non-coin payload |450, only the unchanged coin's two projections | excluded; diagnostic snapshot unchanged |
| Same non-coin item with a dangling pile mapping |450 | excluded; existing dangling-mapping finding remains |

All twelve new engine observations prove one rollback, cursor closure,
SELECT/transaction-only calls and unchanged item/mapping authority inventories.
The broader existing suite retains its source grammar/policy/original-link,
orphan, interrupted-cut, unsigned-revision, provenance and operator-view checks,
including original all-table preservation assertions. The SQL fixture uses its
explicit minimal modeled DDL; these runs are not canonical migration replay,
native producer admission or complete native capture proof.

The actual native codec binary is retained:699184 bytes, SHA256
`5c981c7316076091614fbfc23d9fef4ac4ed1b6a843c8dcda6f55b190ba337ad`.
Its original compile/execution argument vectors and output are retained.
No production C/C++ code changes; no new full production build is claimed.

## Evidence, metadata and narrow primary handoff

Evidence root:
`D:\CodexEvidence\accounting-plan5\bin\coin-payload-source-budget-20261005`.
Seal: `tmp/plan5/coin-payload-source-budget-evidence.json`, SHA256
`0c72bfac65bcd9e43f91ff25c13ff3c36d0894b6f6eda373f86309ae6c4549e4`.
It verifies42 retained artifacts, original RED sources/logs, actual per-engine
read observations, original native binary/commands, complete source transport,
metadata outputs and authoritative exited0/not-OOM/empty-error container State.
Handle28688 is completed and consumed.

Normal contract validation, `generate_economy_writer_coverage.py --check` and
runtime compatibility validation PASS. `validate_economy_accounting.py --release`
still exits1 for missing executable writer evidence. The initial attempted
`run_integration_matrix.py --check` is an unsupported-argument exit2 with no
test execution; the actual metadata-check command is recorded separately.
No full integration matrix result follows from metadata validity.

Primary-owned registration request: in `tests/regression_manifest.json`, set
`test_economic_sql_uid_scope.py.minimum_cases` from9 to11. Preserve every other
field and all105 existing integration rows. Consumers are regression inventory,
test-entry minimum-case enforcement and the regression runner. The two new
methods are `UidScopeTests.test_coin_payload_bounds_refuse_before_mapping_payload_reads`
and `UidScopeTests.test_mapped_coin_payload_budget_counts_repeated_joined_bytes`.
The invariant is that normal discovery executes both source-read guards without
allowing a reduced suite to satisfy the old nine-case minimum. No producer,
wire, public snapshot, SQL field or schema interface change is requested.

The fix has no selected skips or local delivery blocker. The primary registration
request, full combined qualification, complete native holdings/UID capture,
writer/refusal coverage, active erasure, real world/player journeys and declared
release-host workloads remain gates. Accounting stays inactive; wallet-root item
exclusions and the declined inactive spell path remain. No production mutation,
audit autocorrection, activation, deployment, PR merge or experimental-accounting
push occurs. This report and its delivery receipt form the next notebook-curator
packet for the primary's locally maintained project notebook.
