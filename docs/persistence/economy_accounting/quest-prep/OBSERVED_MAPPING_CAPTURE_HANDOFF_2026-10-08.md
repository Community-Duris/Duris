# Explicit observed mapping capture — 2026-10-08

Implemented optional mapping-lifetime and lineage-head observations in the
existing SELECT-only quest cut reader. Paid QP02 and original-A/later-B QP03 can
carry actual observed mapping IDs between cuts without substituting mobile UIDs
or manually assembling those rows. This delivery is **reader/query unit evidence
only**: SQL execution, native journeys, owner authentication and journal
qualification remain false. The continuing native Goal remains BLOCKED.

## Exact bundle and source pins

| Pin | Exact value |
| --- | --- |
| Preserved prep parent / reviewed fee component handoff | `20970bf1e273df61e84b7e3c90aa61c9def15679` |
| Mapping reader code and tests | `2e85d59b1457e3587a713af98af78bc6907ac4ce` |
| Fetched public primary candidate | `20510d07da21759396bbe8775150f1d9aafc4233` |
| Public source tree | `833d3085815b396861ad18a77635412212381e4b` |
| Public migrations tree; source observation, not applied schema | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Original accounting base, historical only | `17c033d69316b21da8598791fc95cae79baa8dc2` |
| Reader preimage / result Git blobs | `8a079422a452028e4ff8c5e94f8031fa9b350358` / `60c14c667563f3ec263959519e4b06b95d5c470c` |
| Test preimage / result Git blobs | `5d4aa65a80cb2d779a9b45ab0daeb3e39b47e462` / `c8a6897a4035264698db370495c90afeed59d86f` |

The code commit changes only
`tests/async/quest_accounting_prep/capture_quest_cut.py` and
`tests/async/quest_accounting_prep/test_capture_quest_cut.py`. The next commit adds
only this handoff; its exact SHA is the containing commit, obtainable with
`git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/OBSERVED_MAPPING_CAPTURE_HANDOFF_2026-10-08.md`
and reported with remote publication. Import the compatible owned commit, not
the prep worktree's older production tree. No production, migration, assertion
oracle, registry, shared driver, finish plan, canonical HANDOFF or Plan 5 file
changed. Prior fee failures and qualification evidence remain intact.

The public source and migration trees equal the previously qualified public
`cda8aa6f65c72121e92d7c07efae7e91164d1050` trees. This is a source comparison;
neither candidate was built or booted for this reader delivery. Private primary
NBC4/NMB4/source candidates remain unavailable and unqualified here.

## Maintained contract traced before implementation

All following blobs were read from public candidate `20510d07da21759396bbe8775150f1d9aafc4233`,
retained in D: evidence and hashed in the receipt:

| Source | Git blob | Relevant contract |
| --- | --- | --- |
| `migrations/economy_accounting.sql` | `a1d65bc8dc54d4426b6a8e3f7687636fd75fd7db` | Lines25–32 define lineage, nullable active epoch and revision. Lines34–64 define all11 mapping columns, active/retired lifetime constraints, and creating/retiring FKs to `critical_operation_inbox`. Mapping IDs are SQL BIGINT UNSIGNED. |
| `src/persistence/economic_accounting_repository.c` | `22ee155be9a2445f08129209127eca02d7740bcc` | `economic_sql_lock_authority` requires an existing transaction/session, locks the lineage head and mapping IDs, then validates epoch/lineage/kind/context/backend/locator/native/active/revision facts. The reader does not call or reproduce this authority path. |
| `src/persistence/quest_mobile_published_world_sql.c` | `a42ec60cba96cfb2a7ddb355295a7bb10c714f67` | `observe_mapping` reads all11 columns and verifies the authenticated native wallet origin, birth operation and lifetime. Supplying the same numeric IDs to this reader does not perform those checks. |

The existing reader already discovers historical operation IDs from native
births, items and obligations, then reads available item references, accounting
operations, inbox receipts, effects, postings and source claims. Mapping creating
and non-NULL retiring IDs now join that same bounded historical observation.
Absent rows stay absent. No synthetic receipt or historical proof is produced.

## Optional interface and literal output

CLI: repeatable `--mapping-id <id> [<id> ...]`.
Python: optional keyword-only `capture(..., mapping_ids=())`.

- IDs must be strict Python integers in `1..UINT64_MAX`; bool, float, string,
  NULL, zero, negative and overflow refuse. Unlike native item UIDs, SQL mapping
  IDs do not use the native allocator's reserved upper sentinel. The CLI follows
  the existing integer-parser convention.
- At most2,048 distinct IDs; duplicates, including across repeated CLI options,
  refuse before SQL connection/cursor creation. Iterables stop at first excess.
  Bindings and selection metadata are sorted.
- `account_mappings` selects exactly the supplied IDs using bound parameters.
  It projects `mapping_id,lineage,account_kind,context_id,backend_kind,locator_kind,`
  `native_id,active_native_id,creating_operation_id,retiring_operation_id,revision`.
  There is no lineage/backend/locator/active filter or native-UID inference.
  Original, retired, replacement and foreign rows retain their literal facts.
- `lineage_head` selects `lineage,active_epoch,revision` for the requested capture
  lineage, in the same RR read-only transaction and cursor. It is not a separate
  head query for every foreign mapping row. Missing head is `[]`; NULL or an
  epoch different from capture metadata is preserved, not coerced or accepted
  as active authority. Existing explicit epoch-row eligibility remains unchanged.
- Nonempty selection adds `meta.watched_mapping_ids`, `meta.missing_mapping_ids`
  and `meta.mapping_observation_authenticated=false`. Missing IDs are not
  reconstructed from mobile IDs, effect keys or other lifetimes. The existing
  `meta.authority` label remains unchanged; it is not mapping authentication.
- Only opted-in captures add the two tables to the InnoDB engine gate. Empty or
  omitted selection preserves default queries, bindings, dependency tables and
  output. Existing legacy-no-epoch capture remains available with empty selection;
  nonempty mapping input explicitly refuses on that path before connecting.

All SELECT projections retain per-query2,048-row/16MiB byte limits and the final
16MiB aggregate JSON limit. Existing obligation/current-image/birth-origin BLOB
preflights remain. On opted-in captures, the final historical operation selection
also preflights aggregate inbox `result_payload` bytes/count before fetching
receipt bodies, including its bytes in the cumulative native BLOB budget with
hex expansion. Excess bounds refuse the whole cut; there is no truncation or
accepted partial result. Lifetime IDs can increase historical selection enough
to refuse an oversized cut; the reader grants no larger allowance.

The same transaction encloses engine checks, epoch eligibility, mappings, head,
remaining rows, BLOB preflights and historical receipts. Confirmed rollback,
transaction-close verification, cursor/connection cleanup, exclusive output,
disposable loopback/schema/binary/source guards and connector timeouts
(connect5s/read10s/write5s) remain. The reader acquires no owner locks and cannot
keep an authority borrow alive after its rollback.

Future integration syntax, **not executed against SQL in this delivery**:

```text
python -B tests/async/quest_accounting_prep/capture_quest_cut.py <existing-verified-disposable-capture-options> --mapping-id <observed-original-wallet-mapping-id> <observed-replacement-wallet-mapping-id>
```

Use IDs observed through the actual owning integration session. Selection is not
authentication, a complete lifetime census, current cash evidence or permission
to replay, publish, acknowledge or retire.

## Focused qualification and retained evidence

Environment: Windows Python3.12.10; TEMP/TMP explicitly `D:\Dev\Temp`, bytecode
disabled. No DB connection, server, migration, native journey or C++ build ran.
Every SQL statement exercised here used the existing modeled cursor seam.

```powershell
Set-Location 'C:\Users\alexa\.codex\worktrees\accounting-quest-prep\NewDuris Max'
$env:TEMP='D:\Dev\Temp'
$env:TMP='D:\Dev\Temp'
$env:PYTHONDONTWRITEBYTECODE='1'
python -B tests/async/quest_accounting_prep/test_capture_quest_cut.py -v
python -B D:\Dev\Temp\quest-observed-mapping-20261008\qualify.py
python -B D:\Dev\Temp\quest-observed-mapping-20261008\seal.py
git diff --cached --check
```

Results: **32/32 updated reader tests PASS** (exit0; retained final run0.272s),
including all original criteria. The exact unchanged original19-test body was
separately replayed against the updated reader: **19/19 PASS** (exit0,0.133s).
Each qualification subprocess has a60s controller deadline. Six old/new default
comparisons matched both output and full query/binding sequences: QP02, QP03,
mobile filter, explicit item history, explicit operation and inactive legacy.
Staged whitespace checks passed.

New coverage includes equal numeric native/mapping IDs without inference;
original/retired/replacement and foreign lineage/backend/locator literals;
missing IDs and missing/NULL/different-epoch heads; exact projections/bindings;
lifetime operation union and absent receipts; malformed/duplicate/overflow/
maximum/bounded-iterator selections; row, byte, aggregate and cumulative receipt
BLOB budgets; opted-in missing/MyISAM tables; mapping/head/receipt read failures;
rollback failure and uncleared transaction; repeatable CLI; legacy refusal;
connection deadlines; disposable/binary guards and exclusive output. Existing
guard tests were extended to both selections instead of copying a second suite.

Evidence directory: `D:\Dev\Temp\quest-observed-mapping-20261008`. It retains
source excerpts, exact original test/reader bodies, stdout/stderr, generated
modeled observations/query bindings, qualification/seal scripts and receipt.
`artifact-index.json` hashes14 payload files; the index itself is additional.
Qualification verified source unchanged and the seal verified committed bodies
exactly equal the tested bodies.

| Artifact | SHA-256 |
| --- | --- |
| Committed/executed reader | `910d80aad12fe281d119082715d4d04ee1f9d73b93cca212cc68200ff0ae4545` |
| Committed/executed test | `76c001723a73b65d5a6339a1d8ac04f24a1f3c40ac8aeac4db1b1a7679d3b5f0` |
| `receipt.json` | `752d615ab0bc757cfaccaeb1618cdc11cfe148a9440046776404c9b37bdbacfd` |
| `artifact-index.json` | `a4949021eca175ba6f45d975b515c888e82c6b67903564ed140029867ce52731` |

## Integration disposition and next boundary

The optional reader change is reviewable without a new production hook or schema.
It removes manual mapping/head collection for paid QP02 and QP03 original-A/
later-B comparisons. Accepted oracles are unchanged. No hypothetical balance
table or private role grammar was introduced.

Still unavailable: genuine owner-authenticated same-cut cash/mapping borrow;
live funding, held inventory/save/publication setup; retained parent/child and
historical SQL proof; ACK, D ownership, paired retirement and cold recovery
execution; current complete world/custody census; private primary source. These
remain integration/evidence dependencies, not reader defects resolved by these
unit results. Raw mapping/head/receipt rows cannot replace the owning validator,
native physical observations or same-session cleanup.

At this handoff, the authorized independent reader preparation is complete;
dependent native execution remains blocked. Preserve the major-batch cadence
and qualify native acceptance only with the actual integrated primary candidate.
Do not resume or complete the continuing native Goal on this component result.
