# Plan 5: native coin payload shared row budget - 2026-10-07

The independent SQL decoder now enforces the native codec's shared 8,192-row
budget across the item, dynamic affects, extra descriptions and all their spell
rows. The old reader bounds each vector separately and accepts a total of 8,193
rows that the actual native reader refuses. At 8,192 rows the full payload still
decodes and round-trips exactly. This prevents an audit holdings snapshot from
adopting a literal that the native authority cannot decode.

Branch/worktree: `codex/accounting-plan5`, `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. Base `629da9ecdc846518ce8afbf275dd1a5d152997fd`;
result is the containing commit, with its exact SHA in the external delivery
packet. Owned files are the independent decoder, its existing SQL fixture and
pure tests, AUDIT_OPERATIONS.md, this report and the additive remote follow-up.
No shared interface/schema change is requested. The wire/output formats,
native providers, canonical migrations, producers, shared runners, writer
registry/matrix and activation ownership remain with the primary agent.

Exact tested owned code tree `a43c8be7c0bce48dab57b1e67bb2c47c6d160cfb`; archive SHA256
`eb808188f0481becd296c81ae63011435980fe7f10873b9bb513e16229258753`. Native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and migrations
`1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain unchanged. Publication adds only this report,
operator guidance and remote follow-up prose to that frozen tested tree.
This does not qualify the older native base as the integrated candidate.

The refreshed primary source is `0fd938ce2fbae7b7e675346d60b30b3867bea0f7`, tree `181c390086c835f6a0ec92d29364f5175526bb18`,
native tree `833d3085815b396861ad18a77635412212381e4b`, migration tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`;
archive SHA256 `4ebbd97e08c30223ef577db219a83df5b433c590524d2c151fd8e0f53c8678bd`. Its unchanged original
`python3 -u -B tests/async/test_flatfile_accounting_coin.py` passes with all 30
cold-reader groups and the original split/replay/restart/recovery assertions.
The original providers, flags and 120-second native timeout remain exact.
This is native component evidence, not combined or real gameplay acceptance.

The negative codec run uses the old exporter at the base: 24 observations across
prototype3 and area prototype402013, including 12 native refusals that the old
audit incorrectly accepts. The fixed exporter agrees on all 24 observations:
12 healthy/at-limit payloads and12 above-limit refusals. Complete payloads,
hashes, commands and retained native binaries are in the external evidence.
The private C++20 oracle links the actual current primary snapshot codec,
with GCC 13.3.0, -Wall/-Wextra/-Wpedantic/-Werror and Address/UndefinedBehavior sanitizers.
Successful native decode/encode must reproduce the exact input bytes; each
refusal must match the expected native result. The unchanged original
`python3 -u -B tests/async/test_player_item_snapshot_codec.py` also passes.
Independent product code imports no native recovery or mutation API.

The final disposable command on each engine is
`python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`:
MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` passes in 15.649435s;
MySQL `8.0.46-0ubuntu0.22.04.4` passes in 22.172547s.
Each full driver retains its prior controls, 13 area observations and 10 byte
bound observations, and adds 24 row-budget cuts. Shared counts include the item,
the exact/above affect and description limits, spell vectors, affects mixed
with descriptions/spells, and spell rows split across descriptions. Every cut
records SELECT-only queries, exactly one rollback/closed cursor, all modeled
application-table inventories unchanged and refusal without partial output.
The original fixture is restored exactly. Private skip-networking sockets and
daemon/PID-file cleanup are observed. This is modeled InnoDB DDL, not canonical
fresh/upgrade, native producer, backup/restore or real player qualification.

The focused command is `python3 -u -B -m unittest -v
test_economic_sql_audit_origins.ItemRevisionTests
test_economic_sql_audit_origins.CoinPayloadTests
test_economic_sql_audit_origins.OriginTests
test_economic_sql_audit_origins.BaselineVersionTests
test_economic_sql_uid_scope
test_reconcile_economy_accounting.ReconciliationTests` with
PYTHONPATH=tests/async: 186 methods pass, zero skips. The earlier broader
invocation ran 204 methods with six guarded native/operational integration
skips; it is retained as that scope and never described as zero-skip evidence.
The first wrapper attempt subsequently fails on duplicate environment keys
before SQL capture; the next wrapper fails the existing private-socket path
guard. Both terminal failures and their helpers remain in the seal. The final
attempt corrects only the observer environment/socket path, retains the actual
driver guard, and does not repeat already passing pure/native tests.

Normal `python3 -u -B scripts/validate_economy_accounting.py` passes.
Its `--release` invocation exits1 as expected: writer has no executable evidence.
Staged whitespace passes. No maintained C/C++ file changed, so no new Make
build is claimed or required for this Python-only fix.

Compiler dependency closure authenticates 42 tracked current-primary
inputs for both codec binaries, plus the exact generated private oracle. The
native coin suite closure authenticates 174 tracked inputs and
1 generated inputs. Canonical source bodies, tar modes
and four links remain unchanged through actual terminal completion and artifact
copy. Earlier failures are retained; no observation timeout is completion.

Evidence root: `D:/CodexEvidence/accounting-plan5/bin/`. The exact packet is
`coin-row-budget-seal-01-20261007/evidence.json`, SHA256 `269ede4ce45e47b4ecc3d46fa63c8c78db3a5067939b9661728f2131e2d569b0`,
covering 1,462 artifact entries/1411950110 bytes. Its
compiler-inputs.json links the actual primary closures. Per-command timings,
log hashes and terminal source guards are retained in each phase directory.
Supplemental `coin-row-budget-artifact-binding-01-20261007/binding.json`
(SHA256 `17cd9763369c5e5c7674e5c924ed0db66ca64790ef389be678ce23386621da55`)
authenticates all 727 retained native files, the generated compiler input, all
48 negative/fixed payload inputs and the exact compiler version.
Final publication and all seven old-tip ancestry checks are recorded in
`coin-row-budget-delivery-01-20261007/delivery.json`.

Full independent flatfile pile/custody/owner-literal and UID-history comparison
remains open, as do unsupported/unknown origins, all other money domains,
current combined canonical SQL qualification on both engines, real journeys,
release-host latency/growth/backup/restore/retention and R1-R8. Wallet-root item
exclusions, inactive behavior and the declined inactive spell-path change stay
intact. No activation, production data change, correction, deployment or merge
occurred. The project notebook remains locally maintained by the primary and
nonblocking per the user; this is an additive curator-ready handoff. Notebook
application, acknowledgement and cross-chat notification are not claimed.
