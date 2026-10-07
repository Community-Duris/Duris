# Plan 5: independent native claim-source consumption and frozen sets — 2026-10-07

`--economic-auction-source-attribution-audit` independently verifies supported
native version-1 source creators, availability at each native claim revision,
whole-row consumers, applied native receipts and frozen cashout source sets.
The established defect preserves verified creators and balanced remaining totals
while assigning a consumed source to an unknown operation. The new reader reports
the unproven consumer and digest. It never recovers, mutates or corrects findings.
Complete producer/backend, origins, R7/R8 and release qualification remain open.

## Source, ownership and delivery

- Local/remote branch: `codex/accounting-plan5`.
- Worktree: `C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
- Base: `8809f8100f0c67d916cf5efe15c929ce794caf15`. The result is the commit containing this report; exact
  result/remote equality and ancestry are authenticated by
  `D:/CodexEvidence/accounting-plan5/bin/flatfile-auction-consumption-delivery-01-20261007/delivery.json`.
- Tested code tree: `bb76713163de959f4bf8ef9683418ddbf373f354`.
- Canonical source archive SHA256: `7da2f65e2e07c3dc877c99969a8c3b6de574b7bf35306443e2b189f8980cfc85`;
  6416 regular bodies/modes and four original links are pinned.
  Publication adds this report and updates the follow-up. Delivery verifies
  every nonpublication body/mode/link against this tested archive.
- Unchanged native tree: `4abb609524a1f1682ea4c190f82d75003c4d679b`. Unchanged migrations 0–62:
  `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`. No native, migration, accounting contract, shared
  coordinator, writer registry/matrix, shared runner or manifest is edited.
- Refreshed primary snapshot: `d81e04b490390f797f286dc99a4446b516a167cc`, native tree
  `eba89be4181caea517005dd93b670c48b15e6187`, migrations 0–64 `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`.
  Its auction repository and three accounting compilers match byte-for-byte.
  `auction_command.c` has the newer v2 context extension. This exact v1 reader
  slice does not qualify that distinct current combined candidate.

All seven earlier alternate tips remain preserved ancestors and their follow-ups
continue on this remote branch. Primary retains shared native/contracts/schema,
coordinator/producer, registry/matrix, activation and combined publication ownership.
These nine owned code/fixture/driver files change:

| Owned file |
| --- |
| `scripts/qualify_flatfile_auction_source_consumption.h` |
| `scripts/qualify_flatfile_auction_source_credit.h` |
| `scripts/qualify_flatfile_restore.cpp` |
| `tests/async/flatfile_auction_consumption_cases.py` |
| `tests/async/flatfile_auction_money_cases.py` |
| `tests/async/flatfile_auction_money_fixture.cpp` |
| `tests/async/flatfile_restore_authority_fixture.cpp` |
| `tests/async/test_flatfile_restore_baseline_markers.py` |
| `tests/async/test_flatfile_restore_economic_authority.py` |

## Reproduction and completed reader fix

Original native accounting compilers create a modeled retained settlement credit
and cashout, while accounting remains inactive. Original source/catalog codecs
retain the successful native receipts and source row consumed by operation 8.
Changing only `consumed_by` to unknown operation 9 and rebuilding the native
envelope hash leaves the old source-credit command exit 0, its creator flag true
and remaining source/claim totals balanced. Recomputing the full frozen consumer
intent with the original money-claim compiler rejects that source set. The red03
run completes two observations, exit 0 and zero skips; every queried input remains
unchanged. Red01/02 missing test-provider links are retained as failed precursors.

The independent reader composes existing authenticated record views and creator
proof under the same authority lock. It stores bounded scalar claim edges,
source rows and receipt proofs. For each immutable claim lifetime it sorts edges
by native revisions, validates revision/value continuity and reconstructs the
available source pool. Baselines observe a value/revision and never mint sources.
Credits enter only at their creating edge. Successful supported v1 writer 9 bids
and writer 13 cashouts must independently match original action, admission, mapping,
fence, metadata, account/result and revision rules. Unsupported negative writers
grant no consumer proof. A zero-auction cashout still has the original operation
derived auction fence; no existing bid/settlement fence rule changes.

Consumers select the native whole prefix ordered by operation bytes and numeric
u16 slot for that lifetime/beneficiary. An oversized next row refuses selection;
the reader never splits it, skips it, or borrows a future credit. Every selected
row must carry that consumer identity. Every consumed historical row requires a
supported successful consumer and canonical selection. Native receipts must be
successful and match SHA256 of the full retained CCM1 command, including EAI and
accepted time, plus the meaningful first 152 zero-item result bytes. Native event
publication and ignored trailing result padding grant no extra authority.

Cashouts debit the whole frozen claim and increment its native revision. The
reader recomputes the original domain-separated digest: the tag
`DURIS-PENDING-CLAIM-SOURCES-V1` without NUL, a u32 count, then each ordered
16-byte operation ID, u16 slot, u32 beneficiary, u64 mapping and u64 amount.
It verifies count 1–4096, total,
immutable lifetime/PID, first original operation/slot and the exact frozen hash.
Supported consumers, cashouts and selected rows have separate counters. Every
finding affects validity; only the first 100 are rendered while aggregates remain
exact. Source, receipt, mapping and timeline storage is bounded; cooperative
budgets, unsafe inputs and pending journals refuse without partial proof.

The query uses no mutation provider or recovery call. Its separate sanitizer
harness includes only owned reader headers; `-MM` contains no `src/` dependency.
Broader origin/cross-epoch/native-holdings/full-R7/release flags remain false even
on healthy modeled cuts. The test report's wider qualification flags remain false;
the seal explicitly records only successful v1 reader checks.

## Exact verification and evidence

All selected runs use immutable canonical archives in private network-none
containers, pinned image
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`,
CPU 2/memory 4 GiB. Source bodies/modes/four links are checked again at terminal
completion. Native source input hashes cover 1304 files; the complete owned audit
family has 30 pins. Artifacts copy to the host before authoritative terminal
completion and sealing. No CI pass is substituted.

- Focused corrective command: `python3 -u -B /evidence/invocation.py`, calling
  `run_checks(Path('/workspace'), Path('/workspace/bin/tests/auction-consumption'), fixture=base/'economic-fixture', operator=base/'qualify', native_fixture=base/'native')`.
  Exit 0, 33.137096s, 58 observations,
  zero skips. Original typed native compilers/codecs and a separate ASAN/UBSAN
  harness compare the independent reader. Strict C++20 builds, command lines,
  dependency proof, executable hashes, case cuts and content-addressed bodies are
  retained under `flatfile-auction-consumption-green-08-20261007/retained-native-tests/auction-consumption/`.
  The independent sanitizer harness is rebuilt. Three authenticated binaries
  are reused from green07 after proving all 6415 other bodies/modes/four links,
  all native/owned C++/headers/builders and generated independent C++ identical.
  The sole test change corrects the plan-offset adjustment from 16 to 24: the original ID belongs at plan 56, intent 80. Binary
  hashes are checked before and after; the read-only reused mount and exact
  invocation are retained in `compatible-native-reuse.json`/`invocation.py`.
  Original runs use the direct script command with `--native-source /workspace`
  and `--artifacts /workspace/bin/tests/auction-consumption`; the final full driver
  invokes the corrected module directly with its own native binaries.
- Controls cover full/two-credit cashout, reversed operation chronology, whole-row
  partial re-bids, later credit with a lower operation ID, balanced marker swaps,
  unproven consumers, missing/altered native receipts, rebound frozen digest/count/
  clock/original/slot/writer, exact 100-finding aggregation, 32768 source/receipt
  bounds and one-over refusals, three pending journals, private modes, links/FIFOs,
  native writer exclusion, four audit budgets, missing catalogs and native refusal
  of a modeled forbidden whole-prefix skip. Generic immutable hashes/source claims
  are fully rebound before semantic controls; findings never correct them.

| Selected full-driver command | Exit | Duration |
| --- | --- | --- |
| `python3 -u -B -m unittest -v test_backup_review_remediations` | 0 | 0.716704s |
| `python3 -u -B tests/async/test_flatfile_restore_economic_authority.py` | 0 | 639.501885s |
| `python3 -u -B -m unittest -v test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration` | 0 | 211.581290s |

The full native driver has 20 healthy stores / 367 corruption refusals, 28 history,
50 native-domain, 38 wallet/bank, 116 auction-value, 50 source-balance,
61 creator and
58 consumption observations.
Backup review has 18 methods. Managed integration has one selected method, two
actual isolated service boots and two retained generations, with all 30 audit
pins and unchanged read-only pending-source refusal. It uses
`DURIS_RUN_BACKUP_INTEGRATION=1` and
`DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/auction-consumption-managed`.
Selected methods have zero skips. These modeled/native reader and isolated boot
checks do not establish genuine admitted producer/player journeys.

Maintained Make: `make -C src -j2 BUILD_PROFILE=production PERSISTENCE_BACKEND=flatfile BIN_ROOT=/workspace/bin/tests/money-history-build/bin OBJDIR=/workspace/bin/tests/money-history-build/objects DMS_BINARY=/workspace/bin/tests/money-history-build/server`, exit 0,
275.324326s, all 740 providers freshly compiled,
zero reused without compilation and 740 byte-identical
objects. All 1290 dependency inputs match.
Server SHA256 `f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4` matches the managed boot's authenticated
native origin. Its raw compiler log and source/dependency provenance are retained.
No native source changes were required. The Make archive SHA256 is
`382bc158cd82e79c1a32b5cea1f03a138f44010518bc55120c50ad97f7a67757` (green07). The final corrected test archive
differs only by that one Python test-control offset. Every native/C++ and all 1290
Make dependency inputs are byte-for-byte identical to the final archive; the
seal proves this compatibility without relabeling the Make source or rerunning
unchanged C++ solely for a corrected Python fixture control.

Normal accounting validation, 55 writer coverage contracts, changed-line C++
format checks, Python AST and staged whitespace pass. `--release` intentionally
exits 1 for missing writer executable evidence. Inventory has 920 entries and
registers these controls through the existing nonmanual native authority driver;
inventory alone grants no completion. SQL changes are absent and no disposable
SQL engine was selected for this native-only reader slice. Both-engine current
candidate/SQL R8 qualification remains an explicit gate, not a selected-test skip.

Retained failed attempts, never promoted to passing qualification:

- `flatfile-auction-consumption-red-01-20261007`: exit 1, 75.993383s; incomplete qualification retained.
- `flatfile-auction-consumption-red-02-20261007`: exit 1, 195.209970s; incomplete qualification retained.
- `flatfile-auction-consumption-green-01-20261007`: exit 1, 134.935885s; incomplete qualification retained.
- `flatfile-auction-consumption-green-02-20261007`: exit 1, 217.549970s; incomplete qualification retained.
- `flatfile-auction-consumption-green-03-20261007`: exit 1, 221.203795s; incomplete qualification retained.
- `flatfile-auction-consumption-green-04-20261007`: exit 1, 228.365763s; incomplete qualification retained.
- `flatfile-auction-consumption-green-05-20261007`: exit 1, 222.351448s; incomplete qualification retained.
- `flatfile-auction-consumption-green-06-20261007`: exit 1, 227.555495s; incomplete qualification retained.
- `flatfile-auction-consumption-green-07-20261007`: exit 1, 239.167943s; incomplete qualification retained.
- `flatfile-auction-consumption-full-01-20261007`: exit 1, 672.826662s; incomplete qualification retained.

Evidence root: `D:/CodexEvidence/accounting-plan5/bin/`.
Seal: `flatfile-auction-consumption-seal-01-20261007/evidence.json`;
SHA256 `cdb8db4a73010f34dbacef8f37e39cd1a8269a9e85214bef0c3f6ac1aa4bef86`. It binds 10904 artifacts/5076126626 bytes,
the original red/failed/final runs, source cuts, native comparison/proposal,
maintained Make, full drivers, gates, primary refresh and creator prerequisite.
Remote delivery adds exact result SHA, seven preserved tips and body/mode/link
equivalence. Every compiled/generated/log/database/fixture artifact remains
outside committed source under `bin/` or the external evidence root.

## Narrow primary build handoff

The unmodified standalone qualifier on primary `d81e04b490390f797f286dc99a4446b516a167cc` fails to
link, exit 1/64.364125s. Its item payload parser
references these absent providers in the maintained qualifier's source list:

| Existing provider | Required functions |
| --- | --- |
| `src/item/lockpick_retirement_continuation.c` | `lockpick_retirement_payload_valid` |
| `src/economy/native_quest_cost.c` | `native_quest_cost_projection_encode/decode` |
| `src/economy/native_quest_coin_give.c` | `native_quest_coin_give_project/encode/decode` |

Proposed narrow change for the primary/current integration owner: add the three
existing provider names to the maintained `scripts/build_restore_qualifier.py`
source list, and qualify its actual current-source consumers. The standalone
builder and fixtures that iterate `SOURCES` are consumers. Accounting fields,
schemas, wire tuples, payload invariants and inactive behavior change by zero.
Historical source inputs must remain correctly attributed; these new providers
do not exist on this slice's older native tree. No speculative fallback is added.

The proposal is concrete and tested: append only these names to `q.SOURCES` in
memory on the exact archived primary. Strict C++20/NoMySQL/GC build exits 0 in
65.230313s; binary SHA256
`09de607ffba20dd0964331563a9d957a502dc3e5f8b44fe7e7d25e10c494f179`. Every source body/mode/link remains
unchanged. Original failure and successful proposal source archive SHA256
`18993e4191248c30048ed7b3c357bb799603879b553b2e09cca5461fba5f80ab`, all 6431
regular bodies/four links and commands are retained in
`flatfile-auction-consumption-primary-qualifier-02-20261007/` and
`flatfile-auction-consumption-primary-qualifier-proposal-01-20261007/`.
No maintained shared file is edited; no functional combined-candidate pass is
claimed from this link-only proposal. Primary should rerun its current native
authority/managed/retention checks after applying the provider change and this
reader slice, then publish its measured combined candidate.

## Remaining gates and curator packet

Full Plan 5/R7/R8/release remain incomplete: current combined v2/native/schema
qualification, complete independent holdings/origins/cross-epoch/UID comparison,
actual empty-source-catalog producer creation, genuine admitted writer/player/
publication/ACK/cold-recovery journeys, both engines and release-host backup,
restore and retention evidence. Born-lifetime/closed-removal/current-holding
findings remain findings. No older 0055 or isolated reader pass promotes a newer
candidate. Primary retains activation ownership.

The report, follow-up, seal and remote delivery are the curator packet for the
primary-maintained local notebook. Notebook application, acknowledgement and
cross-chat notification are not claimed and are not blockers. AI_CONTEXT.md
remains unavailable after the recorded searches; required tracked plans/current
checkpoint are read and pinned. This work preserves accounting inactivity,
wallet-root item exclusions and the declined inactive spell-path change.
No production data, deployment, activation, merge or audit autocorrection occurs.
