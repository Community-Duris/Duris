# Plan 5 saved-ground handoff payload diagnostics — 2026-10-08

The read-only saved-ground exporter now retains independently calculated complete
SQL payload digests, source-ID digests, metadata coverage/orphans and raw season
state. The reconciler diagnoses current, unretired handoff conflicts without
granting runtime, origin, publication, retirement or duplicate-custody authority.
Full Plan 5, activation and R1–R8 remain open.

## Branch, source and ownership

- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Local and remote branch: `codex/accounting-plan5` only. No branch switch in this
  slice; all seven previous branch tips remain ancestors. Publication/delivery
  records verify the remote head. Nothing is pushed to experimental-accounting.
- Owned base: `75805240bc48001cd2dcaaa9e22fa7dbd6a5a2b0`; fix commit: `a62edf27f7c0915ebdf943ad7847c5670d336a1a`.
- Refreshed/tested primary: `9fe5e2022c18d181dcace44a7388741659d39233`. Latest documentation-only primary is
  `ab56edfc9d7cb84641833cb89d53f71ddbed8217`; six raw document changes and
  the unchanged source/schema comparison are in `primary-refresh.json` and its patch. Native tree `833d3085815b396861ad18a77635412212381e4b` and canonical migration
  tree `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` remain unchanged, with all 64 migrations.
- Final composed reader/test tree: `212e6d80d497c174a291e5a25030d371f1b5239f`; tar SHA256
  `6871de8460efc802bbcea1728352653d0b1e9c8c8df5062b42f4357b863187b1` (`candidate06-source.json`, `candidate06.tar`).
  This overlays the 11 explicitly listed Plan 5 files on the refreshed primary;
  its six other provider tests and original saved recovery journey are unchanged
  dependencies. This private composition is not primary publication/adoption.
- Fresh whole-server matrices use composed tree
  `dd2df7a9bad5a0463f09d985578d6507a79074ef`, archive SHA256
  `24122a144994311990fc42329c0a5cd5c4f2e819e91d6187d4c47458b974474b`.
  The final reader changes only UTC session handling and its focused tests after
  that capture. Final-source replay of all fresh cuts checks exact equality with
  the live observer packets on both canonical engines. Native emitter/fixtures,
  registry and schemas are unchanged.
- Owned implementation/tests: `scripts/economic_sql_audit_snapshot.py`,
  `scripts/reconcile_economy_accounting.py`,
  `tests/async/test_sql_saved_ground_custody_audit.py`,
  `tests/async/run_economic_sql_audit_snapshot_mysql.py`.
  This report and the additive remote curator follow-up are the only doc changes.
  Shared coordinator, contracts, producers, schemas, registry/matrix, activation
  owner and shared notebook are untouched. No shared interface change is needed
  for this slice; separate next-interface requests are below.

## Established defect and completed fix

`defect.json` binds genuine previous native source-tamper cuts 55/56, copied
without alteration from the sealed saved-recovery evidence. Changing the legacy
child's `cost` from 150 to 151 preserves every field selected by the old reader
and the entire handoff table. Both-engine RED replay confirms the old exported
packets are identical, while the final reader reports
`saved_ground_source_payload_digest_mismatch`. The old reader already reported
unqualified authority; this is a missing specific diagnosis, not an old release
clear being reclassified.

The exporter hashes all 33 saved-item columns and all affects/extra-descr columns
in canonical schema order. Framing is independently implemented from raw SQL
bytes: table ordinal and row count, then `N` for NULL or `V<byte-length>:<bytes>`
per cell. IDs are ordered and framed with trailing commas for the source ID
digest. Native mutation helpers are neither called nor imported. Schema order
and text charset must match; SQL collation defines opaque per-cut key groups
using the minimum physical ID. Keys, names, descriptions and metadata text never
appear in the audit packet or exception details.

Metadata is scanned without a join that hides orphans. A count preflight includes
saved rows, both metadata tables, receipts, modern room payload rows and raw
season rows. The original 100,000-row bound remains; every raw cell is capped at
1 MiB and total framed source bytes at 32 MiB, before buffered text reads. Final
JSON retains the original 32 MiB limit. The consistent read requires all 40
participating tables to exist and use InnoDB.

The reader SELECT verifies UTC formatting and refuses another borrowed timezone
without changing it. The CLI initializes its own connection to `+00:00`, matching
the native runtime contract. NULLs, empty text, multibyte Unicode, embedded NULs,
timestamps and omitted numeric/flag fields have regression coverage.

The new additive diagnostic fields are `saved_ground_payloads` (group ID, three
row counts, ID/payload digests), `saved_ground_metadata_orphans` (fixed table tag,
physical metadata ID and absent item ID), `saved_ground_recovery_state` (all raw
state_id/season_epoch/reset_status rows), `saved_ground_payload_coverage`
(version=1, group/metadata/orphan counts and framed source bytes), and receipt
`destination_key_canonical`. Representation and cardinality checks are strict.
Older packets remain supported with the existing explicit unqualified gates.

Only an active singleton state_id=1 and a matching current-season, unretired
receipt permit current recovery precondition diagnostics. Missing source or
destination, count/root/key conflicts, unknown digests and digest conflicts have
specific exceptions. Source root checks follow native SQL-root and complete-byte
binding; destination UID/room checks follow the native destination contract.
Retired and prior-season payloads may legitimately change or disappear and are
not current corruption findings. Matching digests never remove
`saved_ground_full_runtime_authority_unqualified` or
`saved_ground_history_authority_unqualified`.

## Commands, backends and results

All jobs use pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
network=none, task-specific D: build/evidence mounts and private RAM source/DB
scratch. Python 3.12.3, GCC 13.3, MariaDB 10.11.14 and MySQL 8.0.46 are recorded.
Credentials are exclusively disposable fixture accounts; no production env,
server, database, archive or data is used.

The actual whole SQL server binary has SHA256
`1a7e305e8be3f516ae6778d6eb03fadcb14859e0876df6d279fc1d4bc75d937b`.
It is reused from the earlier 754-unit production SQL Make build, after matching
all 1,318 repository dependencies and 28 external headers against frozen source
and the pinned image. `sql-build-binding.json` contains every hash. This is not a
fresh Make build and no flatfile binary is substituted. No production C/C++
changes require another full build. The focused native SQL oracle is freshly
compiled with ASan/UBSan; its complete g++ command is in `focused/commands.json`.

```text
python3 tests/async/test_sql_saved_ground_custody_audit.py
  (DURIS_RUN_SQL_SAVED_AUDIT=1 for native canonical both-engine coverage)
python3 tests/async/test_reconcile_economy_accounting.py
python3 tests/async/test_economic_sql_audit_origins.py
python3 -u -B tests/async/run_economic_sql_audit_snapshot_mysql.py
  (once per private engine, original explicit disposable socket path contract)
python3 tests/async/run_saved_item_recovery_journey.py /workspace/bin/server [flags]
python3 -u -B /evidence/replay-final/replay_final.py replay-final
```

`focused` runs the original unittest loader for the first three commands: saved
ground 11/11 methods, 164 observations (134 canonical, 30 synthetic marker parity),
zero skips there; reconciler 131/134, origins 55/58. The six opt-in skips are
listed exactly in its terminal record: two near-limit audit budgets, native stake,
and three native origin integrations. They are not promoted to passes. Windows
final invocation passes 10/11 methods with the explicitly Linux/native SQL method
skipped; its performance record exercises 100,000 reverse-ordered rows at depth65.

`whole02` passes both engines, including all 36 missing/MyISAM refusal controls
per engine and the original repeatable-read late writer assertions. SELECT-only
roles and read-only transactions remain enforced. `replay03` passes all 120
previous sealed genuine native cuts, four predecessor RED cuts and eight real SQL
refusal controls (schema layout, cell size, total bytes and borrowed timezone,
each on both engines); preflight failures precede complete buffered payload reads
and leave database data and the borrowed timezone unchanged.

Fresh native matrices each run the original default four schema cases plus 17
original invocations: nested replay, malformed/rejected child, concurrent source
and child replacement, eight materialization/publication/ack/retirement faults,
and four payload/topology tamper controls. Original journey assertions, fault
stages and timeouts are preserved. Each engine passes 18 invocations /21 cases,
39 real server processes, 60 read-only SQL cuts, zero skips. Accounting epoch and
lineage tables are empty at every acceptance cut; migration history is 64.
`replay-final` recaptures every fresh native cut on both canonical engines with
the final reader, requires exact packet equality with the live observer, repeats
the RED and eight refusal controls, and passes. This establishes this bounded
diagnostic behavior, not runtime authority or release completion.

- `native02-mariadb`: 830.695903s, 326 observed commands, 39 native processes, 60 cuts; conflicts `{"saved_ground_destination_missing": 2, "saved_ground_destination_payload_digest_mismatch": 2, "saved_ground_source_id_digest_mismatch": 3, "saved_ground_source_payload_digest_mismatch": 7, "saved_ground_source_root_identity_mismatch": 2}`; terminal exit0.
- `native02-mysql`: 922.146193s, 331 observed commands, 39 native processes, 60 cuts; conflicts `{"saved_ground_destination_missing": 2, "saved_ground_destination_payload_digest_mismatch": 2, "saved_ground_source_id_digest_mismatch": 3, "saved_ground_source_payload_digest_mismatch": 7, "saved_ground_source_root_identity_mismatch": 2}`; terminal exit0.
- `focused`: 201.433752s, 1401 observed commands; terminal exit0.
- `whole02`: 159.489545s, 15 observed commands; terminal exit0.
- `replay03`: 42.160711s, 144 observed commands; terminal exit0.
- `replay-final`: 29.420973s, 144 observed commands; terminal exit0.

## Preserved failures and limits

The initial freeze lacked `--add` for newly owned test files; its partial temporary
index remains. `checks` failed on duplicate synthetic season-table creation;
`checks02` exposed empty result tuple/list serialization; `checks03` passed all
164 observations but failed its obsolete 112-count assertion. `checks05` and
`checks06` passed their focused modules then refused the observer's wrong private
socket prefix. `whole` exposed the incompatible SET SESSION query; the final
SELECT guard and `whole02` solve it without loosening query guards. Failed batches
remain failed even when earlier methods passed. `focused` is a separate complete
green batch on final source.

Initial MySQL native observation failed in read-only TCP caching_sha2 auth because
the pinned Python runtime lacks cryptography; its stopped native server and DB
evidence remain. Final observer uses the private Unix socket and the same
SELECT-only role. Initial `replay` omitted the expected cyclic-root diagnostic
and shadowed its source record, so its final source guard/terminal summary failed;
that final guard is not claimed. Docker terminal state, console and raw copies
are retained. Corrected replay batches have independent successful guards.
Initial MariaDB matrix passed on its earlier reader; final matrices and final
reader replay have separate source pins. Minor preparation syntax errors occurred
before execution and are not counted as tests.

No native saved-runtime/public physical capture is installed, no current UID or
value authority is granted, no financial opening is established, and no finding
is auto-corrected. Hydrated prototype/text/coin authority, complete collector and
cross-provider reverse census, native EAB2 installation/recapture, reset and NPC
opening origin, authenticated retention/erasure/backup continuity, populated
both-engine upgrade/rerun, original major-plan gameplay/load/fault journeys and
the primary's tested combined publication remain open. Six opt-in tests above
remain skipped in this slice. Inactive behavior, wallet-root ITEM_MONEY exclusion
and the declined inactive spell path are preserved.

## Narrow primary handoffs and curator delivery

1. Update the shared saved recovery operator handoff's concurrent-child row.
   With migration0037 strict full source binding, same root/count with replaced
   child bytes/ID retains source and receipt, logs source payload conflict and
   deferred retirement, and withholds the cold destination. The previous test
   repair3b51d3b2c3aa54ca610f5e329651085de214c935 and both fresh matrices prove it.
   Consumer: staff recovery guidance. Validation: original concurrent-child,
   source payload tamper and cold refusal controls on both engines. This report
   does not edit that shared protocol document independently.
2. For the next independent consumer, publish the exact private
   `economic_sql_initialized_activation_view.h` and original-codec source packet,
   including owning physical/source2 row spans, initialized world facts, the five
   raw room providers, creation packet and persisted correspondence/all-row raw
   catalog/history. Fields needed from the proposed reset contract are exact
   `root_item_uid`, `birth_operation`, `room_revision`, `canonical_origin` (ZRO1
   original command and TIR48), and nullable original terminal BODY bytes, with
   the RSC2 nine-table original cut and original budgets. Preserve NULL and raw
   bytes; do not substitute normalized/current projections or schema backfill.
   The proposed storage fields are nonzero uint64 root UID, binary16 birth
   operation with restrictive inbox FK, uint64 room revision, and ZRO1 bytes
   capped at524352; nullable terminal BODY retains its separate32 MiB bound.
   Native DTO member names/types are not invented here. Invariants: synchronous
   borrowed lifetime, exact retained initialized world; same actual selected
   owner/session/transaction/slot/save-epoch/closed-outbox/installation/request
   hash/activation evidence before and after, failure latch closed, rollback and
   residual row/cell/byte budget enforcement. Consumers: independent Plan5
   activation, persisted-provider union and writer16 reset-origin readers.
   Tests: compile exact header, both-engine actual owner callback, stale/foreign
   request and changed-boundary refusals, expired reference/latch/rollback,
   native original ZRO1/RSC2/nullable BODY replay, independently authenticated
   writer16/type22 creation parent `(0,0,0)` branch, and current/private combined
   activation qualification. The published primary checkpoint's111-production/
   23-original-fixture/5-private-schema candidate remains uncompiled/unexecuted;
   its header/source packet is not in this checkout. The new const `pc_money` primary comparison report is not independent
   authority; the consumer still checks raw PC cash/revision/shared-bank evidence,
   unloaded holdings and original account context rules independently. The private
   packet SHA256 is7bb4c58150d14dcf57148080127b080219436c4b91575a4addbf12d9c2e97351.
   No shared schema/interface edit or activation waiver is made here.

The additive `PLAN5_REMOTE_FOLLOWUP_2026-10-06.md` entry and this report are
curator-ready. The primary-local shared notebook is explicitly nonblocking under
the user's direction. Curator application/acknowledgement and primary adoption
are not claimed; no cross-chat message was sent. Full goal remains active.

## Sealed evidence

Evidence root: `D:/Dev/Tests/Duris/accounting-plan5/saved-handoff-20261008`. Final raw seal `seal/evidence.json` SHA256
`7551c95736dd57bf2c2b570cbdf7da707d8e1cd897346fac5345b31d1985d253`, 38400 entries,
11575710321 bytes; 16 separately guarded build copies,
zero copied links/reparse points, all Docker jobs terminal. Native POSIX link/mode
inventories and before/after frozen source guards accompany raw cuts, SQL/native
logs, stopped private DB/game copies, commands, source archives and failed runs.
Failure replay's missing final guard is explicitly excluded as above. Full Docker
consoles are retained even where tool display output was truncated.

`analysis.json` binds terminal results; `code-commit.json` binds the owned fix;
`delivery/result.json` records the documentation/publication commit, exact remote
head, seven ancestor checks and clean worktree after push. It is written after
publication, outside the immutable raw seal. No merge, deploy, activation,
production mutation or direct primary push occurs.
