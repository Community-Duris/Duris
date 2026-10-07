# Independent coin exporter: primary integration - 2026-10-07

The maintained SQL audit reader now shares the native codec's 8,192-row budget
across the item, affects, descriptions and all spell vectors. It also includes
area-specific ITEM_MONEY prototypes and binds each decoded literal to its native
row's UID and prototype. Output remains `sql_partial`, `complete=false`.

Before import, primary reproduced the old reader accepting 8,193 total rows
while accepting the valid 8,192-row boundary. It also reproduced rejection of a
valid prototype402013 money literal. Native source independently establishes
the cumulative bound in `src/player/player_snapshot_codec.c` and
`PLAYER_SNAPSHOT_MAX_ROWS=8192` in `src/player/player_snapshot.h`. This local
failure witness did not execute the native decoder.

Primary imported precisely the three Python code blobs and dedicated report
from Plan5 commit `cffe05a0352a3d482fe9a2b48bf4861db834252c`; these blobs are
unchanged at the refreshed peer tip `2e02e9ce10472320a33cd98239aa9244fb1404b2`:

| Path | Git blob |
| --- | --- |
| `scripts/economic_sql_audit_snapshot.py` | `b14841eb46af7287e91ac946dce3949066a80f4e` |
| `tests/async/run_economic_sql_audit_snapshot_mysql.py` | `8a4d0ab887263e1c331d21f52859ed6f920817b2` |
| `tests/async/test_economic_sql_audit_origins.py` | `cf8eba159d3168a47be656b2aa961b270e113f1f` |
| `PLAN5_COIN_PAYLOAD_SHARED_ROW_BUDGET_2026-10-07.md` | `d36beae5d30809492c660885f557a5a3fd1c8ab0` |

Integration base `5826195dd7365ea8e482545de21b770ea0da3a73` retains native tree
`833d3085815b396861ad18a77635412212381e4b` and migration tree
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`, matching the peer's refreshed
current-primary codec input. No C/C++ source, schema, shared wire format,
coordinator, activation contract or writer disposition changes in this slice.

## Actual local qualification

With `PYTHONPATH=tests/async`, the original focused `python -u -B -m unittest -v`
command ran `ItemRevisionTests`, `CoinPayloadTests`, `OriginTests`,
`BaselineVersionTests` from `test_economic_sql_audit_origins`,
`test_economic_sql_uid_scope`, and
`test_reconcile_economy_accounting.ReconciliationTests`.
All **186 methods pass, zero skips**, exit0, 69.031 seconds. Boundary tests include
shared totals across descriptions, exact/above limits, corrupt payloads,
UID/prototype mismatch, noncanonical prototypes and area denominations.

Normal `scripts/validate_economy_accounting.py` passes, exit0, 5.734 seconds.
Its `--release` invocation retains expected exit1: writer has no executable
evidence. Exact terminal commands and log hashes are retained in
`bin/tests/plan5-coin-budget-integration-primary-20261007/RESULT.json`;
preimages, failure witness and blob pins in
`tmp/plan5-coin-budget-integration-primary-20261007/PREIMPORT.json`.
Protected SHOP harness changes and the unrelated untracked Plan5 report remain
byte-identical. No new production Make is required for these Python-only changes.

The [peer report](PLAN5_COIN_PAYLOAD_SHARED_ROW_BUDGET_2026-10-07.md) separately
records native codec comparisons and modeled MySQL/MariaDB SELECT-only capture.
Its external `D:/CodexEvidence` artifacts are unavailable here and were not
rehash-authenticated by primary. Those reported checks do not become local
canonical upgrade, producer, gameplay, backup/restore or release evidence.

Larger independent custody/world reader slices have missing provider closure and
remain pending. Plans2-4, joint Plan5, source-complete activation, actual player
journeys and full R1-R8 remain open. All926 writer policies,
`coverage_complete=false`, inactive behavior and the declined spell path remain.
