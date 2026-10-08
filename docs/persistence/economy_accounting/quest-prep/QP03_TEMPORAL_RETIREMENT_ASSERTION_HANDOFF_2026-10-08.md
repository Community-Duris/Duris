# QP03 temporal retirement cut assertion handoff — 2026-10-08

Published a callable **modeled cut-consistency** slice for A before retirement,
A explicitly terminal, B observed at a later birth stage, and stable stale-callback
and recovery cuts. It removes the simultaneous-A/B prerequisite in the new API
without modifying the historical `retired` oracle or rewriting capture metadata.
The original native Goal remains **BLOCKED**. This finite delivery is not native
quest execution, physical D qualification, or integrated release completion.

## Commits, candidate and owned files

| Pin | Exact value |
| --- | --- |
| Preserved prep parent | `6366b7ee5a354ce74454a5dc70aeddfd155b1566` |
| New code/test commit | `826c30f20f3fe0cc8bbdb8184f9fa948e87b88a3` |
| Initial reviewed primary reference | `a2faf44cbf9b33bd177148e9b144fad9cacf2679` |
| Latest fetched primary reference for this slice | `4357367798ac045879081a8bdcf861a4699a7ce4` |
| Primary source tree | `833d3085815b396861ad18a77635412212381e4b` |
| Primary migrations tree | `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Research PR678 revision | `55905eac1906cf59405764407f9d22497cccfff3` |
| Original accounting research base | `17c033d69316b21da8598791fc95cae79baa8dc2` |

Only these three new paths belong to this delivery:

- `tests/async/quest_accounting_prep/native_quest_retirement_checks.py`, code blob
  `833b2a5ac9c543468ef46820331b2df3bbc45e26`.
- `tests/async/quest_accounting_prep/test_native_quest_retirement_checks.py`, test
  blob `963f6a98004ae5a2d5aaee7f1f771b5564877825`.
- This handoff, added in the documentation commit following the exact code SHA.
  Its containing commit is reported in the final branch delivery.

Both code paths were absent at the parent. Existing tests/helpers, capture reader,
canonical `HANDOFF.md`, production, shared drivers, migrations and registries
remain byte-identical. No DB, server, migration or native build operation ran.
The two latest primary advances changed documentation, with source/migrations
unchanged. The prep checkout remains its preserved branch; it is not a rebuilt
or merged primary candidate.

## Producer and owner facts checked against the primary pin

QP03 is Auriam16006 in Pinehollow, room16077. Production
`areas/qst/pineholl.qst` block16006 consumes exact kinds16013/16014/16080 and
rewards16015/16075, with D and no coin fee. Loader prepending makes runtime
reward order16075 then16015 and ingredient order16080/16014/16013. Zone reset
creates M16006, cap1, room16077, then G16016 and G16015. The original carried
16015 UID is residual stock; it cannot become a fresh reward by matching VNUM.
Foreign-VNUM descendants of that stock also require retirement evidence.
Actual cash must be observed from the native image, not assumed from the mobile
prototype's denomination/XP line.

| Primary source / blob | Verified boundary |
| --- | --- |
| `areas/qst/pineholl.qst` / `4d9ca3564469a6baa480308e80ab5622fea4e567` | Block16006 at54, Q69, R78–79, GI80–82, D83 |
| `areas/zon/pineholl.zon` / `76965dcc42965027c2f90ea98ab1a85ac313f476` | M258, G259–260; reset producer, not a selected-SQL census |
| `src/world/quest_mobile_native_birth.c` / `cba793daac270b1c9fefd4b818139c5597e75025` | `prepare_mobile`642: real reset invocation664, generated birth operation668, allocator instance671, npc_generation source674–675, original vnum/place/zone676–679, mobile/stock revision1 at680–681; `seal_mobile` capture958–961 uses the birth operation and cash revision1 |
| `src/world/db.c` / `da996337d17da9a0f81016e3f4c077405fd4fdee` | `reset_zone`6938, M limit/force logic7215–7245, actual birth dispatch7255; force overrides mean cap1 is not authenticated chronology |
| `src/world/new_events.c` / `4bab396e1c0f60b080d7ac1f17b0fbb801be3815` | Full-boot reset2091, recovery/copyover exclusion2073–2088 |
| `src/item/item_movement_transaction.c` / `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` | `native_publish`7668, applied/already success7689, successful D refusal7694: original whole-stock/cash/lifetime owner still required |
| `src/world/quest.c` / `4ee99c6c1020250395cba9bdfa3e47f42a642cfe` | Original runtime lookup4867–4872; frozen parent/child cleanup2902–3024 requires exact receipts, original obligation ACK, rollback and successful pair transition |
| `src/persistence/quest_mobile_native_origin_sql.c` / `60a16a46a37f2ac1aca79a9a35f38684de6d142c` | Retained origin lock184 and historical authentication214 precede current mobile lock221; available opaque bytes alone supply no such authentication |

No D implementation, typed D policy/source command or physical owner was added.
The model uses the existing registered generic item-destroy reason for its
terminal example. Acceptance of a registered balanced row set does not establish
that a real D owner authorized that reason, source, counterparty or command.

## Callable contract

```python
from native_quest_retirement_checks import assert_temporal_qp03

result = assert_temporal_qp03(
    original_before, original_terminal, replacement_born,
    after_stale_callback, after_cold_recovery,
    original_instance=observed_a,
    replacement_instance=observed_b,
    original_wallet=observed_a_mapping_key,
    replacement_wallet=observed_b_mapping_key,
    terminal_operation=observed_a_terminal_operation,
)
```

At least one stable cut after B birth is required. The tests use two, representing
stale callback and repeated recovery observations; the labels themselves confer
no authenticity. The callable reads inputs without modifying them and raises
the existing `CutError` on missing/inconsistent evidence. There is no new CLI,
capture implementation, owner decoder, projection or fixture runner.

The first two cuts must explicitly select A only; B birth and every stable cut
must select A and B. Actual mobile rows must match the stage's observed IDs. Each
stage validates its original metadata using the existing binding function;
cross-stage common pins are compared separately, never patched to pass a bind.
All cuts share source/binary/schema pins, migration rows, player, lineage, current
epoch and watch selection. Epoch rotation during this sequence is unsupported.
A's retained birth root may belong to a different historical epoch; it is linked
and preserved without falsely passing that root to the current-epoch book check.

The narrow D-only interval starts after separately established quest reward/cost
effects and ends at observed A terminal state. It requires retained original
birth/source/reference and opaque carrier; explicit LIVE→RETIRED image; advanced
mobile clock and, for residual stock, stock clock; zero terminal cash and its
exact cash-revision relationship; one new terminal root/receipt/claim; exact
selected residual UID, kind, topology, revision and custody tombstones/events/
references; and exact wallet denomination effects/postings. No player, reward,
XP, task, obligation or ACK effect may repeat in that interval.

B birth is a separate interval. It requires distinct instance/birth/source and
wallet keys; original reset source shape; live native revision1/stock1/cash1 with
birth operation as last transition; its own new root, receipt, claim and carrier;
fresh selected stock UIDs with creation custody attributed to B's birth operation;
and its own wallet issuance effects. A's terminal image, tombstones and all
historical evidence must survive. B creation is allowed at this stage only;
subsequent stable cuts permit no new roots, postings, debit, reward, stock move,
retirement, history change or modification of A or B.

Both economic intervals deliberately support one explicit native wallet and one
stateless counterparty effect, with one exact posting per effect and balanced
denomination vectors. The modeled terminal counterparty is a registered sink;
B birth uses registered issuance. This narrow assertion is not a semantic D
policy or a promise that an unpublished D implementation will have that layout.
If its genuine observable interval/layout differs, stop the dependent assertion
and review the actual owner rather than synthesizing rows to satisfy this API.

## Scope returned even on agreement

`scope` is `captured temporal-QP03 agreement only`. Every passing result includes
explicit external requirements for:

- Authenticated reset chronology and a complete mobile census. Absence of B in
  selected rows, operation lists or stage labels cannot prove global absence or
  that B was born later. Observed contradictory B evidence in an early cut is
  rejected; coherent selected omission remains insufficient to authenticate time.
- The real quest-D owner, command/policy/source/counterparty authority, and full
  physical retirement. SQL terminal bytes do not establish world extraction.
- Original/replacement wallet mapping authentication and complete literal native
  forest/custody correspondence. The maintained decoder validates the full image
  grammar; this helper checks selected SQL UID transitions and forest **counts**,
  without copying a decoder to authenticate every serialized UID/object field.
- Native holds and exact original parent/child pair ACK/retirement. Receipts and
  canonical-origin bytes are retained opaque carriers, not validated authority.

The current source still refuses successful D publication. Therefore a genuine
native QP03 temporal run remains blocked on its original shared owner. This
oracle cannot turn that refusal into retirement or manufacture a B birth.

## Executed checks and retained evidence

Executed from the preserved prep worktree under WSL Ubuntu-22.04/Python3.10.12:

```powershell
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' -- env TMPDIR=/mnt/d/Dev/Temp TMP=/mnt/d/Dev/Temp TEMP=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B tests/async/quest_accounting_prep/test_native_quest_retirement_checks.py -v
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' -- env TMPDIR=/mnt/d/Dev/Temp TMP=/mnt/d/Dev/Temp TEMP=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B tests/async/quest_accounting_prep/test_quest_cut_checks.py -v
git diff --cached --check
```

PASS: 13 new methods with rejection subcases (0.241s) and all17 unchanged methods
(0.870s); both exited0. Final logs are
`D:\Dev\Temp\qp03-temporal-retirement-20261008\temporal-tests.txt` and
`D:\Dev\Temp\qp03-temporal-retirement-20261008\unchanged-quest-cut-tests.txt`.
They contain modeled unit-test output only and are not committed.

Controls include incomplete/reversed/simultaneous stages; observed early B;
wrong/reused instance, birth, source, clocks and mapping; missing receipt/carrier;
historical mutation/removal, including old birth epoch; foreign-VNUM nested
residual stock and exact tombstone topology; residual cash; scalar-equal damaged
denominations; duplicate/offsetting cash effects; borrowed stock; extra B-birth
reward/debit; and B cash/stock/lifetime/birth changes or repeated roots/effects
across stale/recovery cuts. A passing label/selected-absence control explicitly
asserts the returned external chronology/census requirement. Tests also confirm
the oracle does not mutate cuts and allows a genuine mapping ID numerically
equal to its mobile instance ID, without inferring that mapping.

These are source verification and passing modeled component tests. No native
journey, actual D transition, SQL capture execution or authenticated B reset was
performed in this slice. Earlier journey/ELF evidence retains its original pins.
No major gameplay qualification batch was started.

## Import dependencies and next useful slice

Select compatible existing pack inputs before cherry-picking the new code commit
and following handoff commit. Do not import the prep branch's older production
tree. Runtime/test dependency preimages used here are:

| Existing file | Unchanged blob |
| --- | --- |
| `tests/async/quest_accounting_prep/quest_cut_checks.py` | `20ec1fa587ee00cfee1a040236df1dff800cb4ad` |
| `tests/async/quest_accounting_prep/case_data.py` | `44c08a70031bde27ebb9a0996c2706e93a5cf8d0` |
| `tests/async/quest_accounting_prep/test_quest_cut_checks.py` | `d6c149c4632ecc939fd388c403883149ac5ce2f2` |
| `tests/async/test_economic_sql_canonical_audit.py` | `a3099e06a7a43400ff7c4cba1847c9dade71325c` |
| `tests/async/test_reconcile_economy_accounting.py` | `ff40f81c3498ad41d173bf4ab45ebde1ff48f449` |
| `scripts/reconcile_economy_accounting.py` | `e8b02432ef2b0cede6f8ad488a8309f05437ad4a` |
| `scripts/economic_restore_evidence.py` | `06a2ebeaddd4bf604990b0f6cacf0c842555ccf3` |
| `docs/persistence/economy_accounting/registry.json` | `f3797ec71cd79c7f24fb0f3c015af6ef5ad39754` |

The two maintained decoder scripts and registry match the fetched primary's blobs.
Existing capture reader blob `8a079422a452028e4ff8c5e94f8031fa9b350358` is not a new
import or changed dependency: use it only when genuine isolated owner activity
becomes available. Keep the same explicit watched A residual UIDs, including
foreign descendants, across all cuts so tombstones remain selected. Add B to
`--mobile-instance` at its observed birth stage, retain original operations, and
record authenticated reset chronology/census outside these selected cuts.

The next useful bounded slice is exact original parent/child held and ACK-pair
cut agreement using maintained recovery interfaces, after source/shape inspection
and an explicit owned-path reservation. It must preserve the distinction between
an obligation receipt, frame absence and genuine pair retirement. Actual D owner
and authentic runtime setup remain primary dependencies; this proposed slice
does not authorize production, codec, driver, schema, DB or server changes.
