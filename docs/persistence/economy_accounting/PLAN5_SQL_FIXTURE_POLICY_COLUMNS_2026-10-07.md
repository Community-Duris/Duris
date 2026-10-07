# Plan 5: SQL fixture policy columns - 2026-10-07

The disposable SQL audit fixture now declares the two policy digest columns
queried by the current origin reader. The original fixture fails on MariaDB
with error1054 for `p.native_boundary_digest` before its intended audit cases.
The test-only repair adds nullable BINARY(32) `native_boundary_digest` and
`request_digest` columns and names the seven existing installation INSERT
columns explicitly. The historical modeled receipt and unknown-origin
semantics remain unchanged; no digest evidence is invented.

Branch/worktree: `codex/accounting-plan5`, `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. Base `d1e185eaf256f03c171a9acf14e87d0bc1f2e7a5`;
result is the containing commit, recorded in the external delivery packet.
Only `tests/async/run_economic_sql_audit_snapshot_mysql.py` and this report/follow-up are owned. No shared interface request,
native source, canonical migration, producer, registry or activation change.

Exact tested code tree `d9b1136e1615b77ce8251ca7cba79174bd6dc55d`; archive SHA256
`bb5eb9df1f4ed1cbfcb5dc2b77bebde769a00f2112a83cebb48117138edc69af`. Source bodies/modes and four links remain unchanged
through terminal completion. Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migration
tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain unchanged. Primary plan/checkpoint source
is pinned separately as e6e058515f5433a1a3028f80d5d7d672d48b2471.

Both complete original `python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`
invocations pass, zero test controls removed, on private network-disabled
MariaDB and MySQL instances: 14.426850s and 14.588019s.
Versions and SELECT-only reader denials, queries, authority hashes, snapshots,
source guard and stopped-daemon checks are retained in
`D:/CodexEvidence/accounting-plan5/bin/area-coin-fixture-02-20261007/`.
This uses modeled InnoDB DDL; it is not fresh/upgrade canonical schema acceptance.
The first successful run with trailing whitespace is preserved in fixture01;
fixture02 verifies the corrected exact final code. The original missing-column
failure stays under area-coin-red-01-20261007. MySQL was unrun in that fail-fast
prototype; both engines pass the final repaired fixture.

Host command `python -B -m unittest -v test_economic_sql_audit_origins.ItemRevisionTests
test_economic_sql_audit_origins.PartialClaimExportTests test_economic_sql_audit_origins.OriginTests
test_economic_sql_audit_origins.BaselineVersionTests` passes44 methods, zero skips,
with PYTHONPATH=tests/async. Staged whitespace passes. No C/C++ changes require
a new Make build. Native codec execution belongs to the subsequent area-coin
slice; it is not claimed here.

Evidence seal `D:\CodexEvidence\accounting-plan5\bin\fixture-policy-seal-01-20261007\evidence.json`; SHA256 `8166fe91b99b6a095ef828636646c283f006be6e1d94c8745d329157204a6012`; 589 artifacts /
787199958 bytes. Remote result/ancestry and uncommitted area-coin
follow-up preservation are checked in the delivery packet. The report,
follow-up, seal and delivery are the nonblocking curator packet for the
primary-maintained notebook; application and acknowledgement are unclaimed.

The actual area-coin omission is reproduced separately on both engines in
area-coin-red-02-20261007, after this prerequisite repair. It remains separate
work. All full Plan5/R7/R8/current combined, genuine producer/gameplay/recovery,
both-engine canonical schema and release-host backup/retention gates remain.
Accounting inactivity, wallet-root exclusions and the declined inactive spell
change are preserved. No production modification or audit correction occurs.
