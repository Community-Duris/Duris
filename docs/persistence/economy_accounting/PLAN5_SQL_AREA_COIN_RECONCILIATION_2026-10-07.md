# Plan 5: SQL area-coin reconciliation - 2026-10-07

The independent SQL audit reader now includes valid native ITEM_MONEY literals
whose item prototype is not3. The old reader's five prototype3-only selections
drop balances, current pile census and account mappings for area money402013.
Changing only prototype3 to402013 reproduces the omission on both engines.
The native custody decoder accepts any matching native prototype with
ITEM_MONEY type; the existing currency journey uses AREA_COIN_VNUM402013.

The reader uses the native type byte at SQL payload position31 as a bounded
candidate selector. Full decoding still requires exactly one item, exact
UID and native prototype, root position, ITEM_MONEY type, nonnegative four
denominations and exact end-of-payload. The same selector applies before
global and repeated mapping byte bounds, to mapped balances, live pile census
and current pile literals. Existing unrelated noncoin payload exclusions
remain tested. The default two-argument decoder continues to require prototype3.

Branch/worktree: `codex/accounting-plan5`, `C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`. Base `5b1129392dee59ae35a676fd885204f041a00801`;
result is the containing commit and is recorded in the external delivery packet.
Owned files are `scripts/economic_sql_audit_snapshot.py`, `tests/async/run_economic_sql_audit_snapshot_mysql.py`, `tests/async/test_economic_sql_audit_origins.py`, `docs/persistence/economy_accounting/AUDIT_OPERATIONS.md`,
this report and the additive remote follow-up. No shared interface or schema
change is requested. Wire/output fields, native code, canonical migrations,
producer integration, writer registry/matrix and activation are not changed.

Exact tested owned code tree `04c685a1ad5ff2524227cdbe0cdc8b44cd2bc6b4`; archive SHA256
`15efb3d910b48dea1c7441f9f0f2a4c8b67b7c052f7d0f961218f93dc9c5d0f3`. Its native tree `4abb609524a1f1682ea4c190f82d75003c4d679b` and
migration tree `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5` remain unchanged. This work does not
qualify that older native/schema base as the primary combined candidate.
Source bodies, canonical tar modes and four links are authenticated through
terminal completion and matched to the eventual publication excluding only
the new report/follow-up prose. No observation timeout was treated as terminal.

The two-engine negative run at area-coin-red-02-20261007 fails exactly
`prototype-402013-state-1` with the unchanged old exporter. Both failures
are observed; their wrapper exits0 only after verifying expected failure.
The separate fixture prerequisite `5b1129392dee59ae35a676fd885204f041a00801` fixes an earlier missing policy
column failure; its exact evidence is linked from the fixture report.
No failed or earlier archive is relabeled as a final passing source.

Final disposable SQL command on both actual engines:
`python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py`.
MariaDB `10.11.14-MariaDB-0ubuntu0.24.04.1` passes in 15.645040s;
MySQL `8.0.46-0ubuntu0.22.04.4` passes in 17.796420s.
The complete original driver controls remain, with13 added area observations
per engine:402013/402014/INT32_MAX at live/tombstone/quarantine states,
denomination drift, native prototype mismatch, UID mismatch and negative amount.
The ten byte-bound cases include four new area cases: individual above4MiB,
aggregate exactly32MiB, aggregate above32MiB, repeated join above32MiB.
Above-limit payloads are refused before projected full-payload transfer.

SELECT-only capture records queries, one rollback/closed cursor per area cut,
all25 modeled application-table inventories, unchanged authoritative sources,
UPDATE denial1142, exact fixture restoration and original snapshot equality.
Each daemon uses a private socket and skip-networking; its exit and removed
PID file are observed. This is modeled InnoDB DDL, not canonical migration
fresh/upgrade acceptance or a real producer/player/recovery journey.

Focused command `python3 -u -B -m unittest -v
test_economic_sql_audit_origins.ItemRevisionTests
test_economic_sql_audit_origins.CoinPayloadTests
test_economic_sql_audit_origins.OriginTests
test_economic_sql_audit_origins.BaselineVersionTests
test_economic_sql_uid_scope test_reconcile_economy_accounting.ReconciliationTests`
with PYTHONPATH=tests/async passes184 methods, zero skips. It includes exact
native UID/prototype binding, corrupt/nonmoney literals and invalid prototype
representations. `python3 -u -B scripts/validate_economy_accounting.py` passes;
`--release` exits1 as expected, `writer has no executable evidence`.
Staged whitespace passes. No maintained C/C++ change requires a new Make build.

Native proof runs the unchanged original
`python3 -u -B tests/async/test_player_item_snapshot_codec.py` recipe while
retaining its binary/commands, plus an external C++20 codec oracle compiled
with -Wall/-Wextra/-Wpedantic/-Werror, AddressSanitizer and UndefinedBehaviorSanitizer.
The exact commands and GCC version are in retained-native/evidence.json.
Prototypes1,3,402013,402014 andINT32_MAX round-trip exactly between the owned
fixture and actual native decode/encode, with matching UID82, denominations
and type byte. Native-produced bytes are independently decoded by the fixed
reader. All five runs and original codec pass with no sanitizer findings.

The first native run uses the exact owned archive and is preserved separately.
Four compiler headers differ from current primary: `src/item/item_transfer_command.h`, `src/persistence/critical_command.h`, `src/world/quest_mobile_native_binding.h`, `src/world/db.h`.
The second run executes the actual primary `e6e058515f5433a1a3028f80d5d7d672d48b2471` archive, native tree
`d149afce4392056ee92afdde2c42dfd22880a5c3`, migrations `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`,
archive SHA256 `13e90d1ceb9edbfd33e7f2fe3389ce2ae4fa8abcbb54f32365a5c13f90c1a2ab`, and the owned reader archive.
All42 tracked compiler inputs are checked against primary Git bodies.
Both source trees' bodies/modes/links remain unchanged. This proves the codec
wire agreement; it does not qualify any producer or the entire combined native.

Evidence directories under D:/CodexEvidence/accounting-plan5/bin/:
- area-coin-refresh-01-20261007: primary plans/checkpoint and interfaces.
- area-coin-red-02-20261007: both-engine demonstrated omission.
- area-coin-green-01-20261007: full final SQL/pure/validator controls.
- area-coin-native-01-20261007: retained original owned native codec/oracle.
- area-coin-native-current-01-20261007: retained exact current primary codec/oracle.

Seal `D:\CodexEvidence\accounting-plan5\bin\area-coin-seal-01-20261007\evidence.json`; SHA256 `4cca6c20dd484cfe325aa75c5b3daeab0d81a775cd0960cd30745d095ba545f0`; 490 artifacts /
800077190 bytes. The report, follow-up, seal and exact remote
delivery are the nonblocking curator packet for the primary-maintained notebook;
notebook application/acknowledgement are unclaimed. All seven alternate tips
must remain ancestors of the same published codex/accounting-plan5 branch.

The type-byte selector establishes candidate coverage for present valid
literals. Missing/unclassifiable area payloads and complete native money
classification remain gaps; complete=false and release refusal are retained.
Full Plan5 native holdings/treasury/UID/provenance/retention coverage, actual
producer/player ACK and cold recovery journeys, both-engine current canonical
schema, full R7/R8, release-host backup/restore/retention and tested combined
candidate gates remain. Prior primary-owned runner source-list requests remain
separate handoffs; no new interface change is needed for this fix. Inventory
coverage and synthetic passing checks do not establish release completion.
Accounting inactivity, wallet-root exclusions and the declined inactive spell
change remain preserved. No audit auto-correction or production change occurs.
