# Paid QP02 NPC-cost assertion handoff — 2026-10-08

Added a callable paid-QP02 cut-agreement oracle and modeled sensitivity tests.
It compares original native cash, economic effects/postings, source/receipt links
and the selected production contract in a funded-before/consumed-after interval.
It does not authenticate the supplied mapping, frozen projection or physical
publication. The continuing native Goal remains BLOCKED; this finite delivery
neither resumes nor completes it. No DB/server/migration/native build ran.

## Exact commits, files and inputs

| Pin | Value |
| --- | --- |
| Preserved prep parent | `807f5b2323c1b7da12c17b9e07821847807993f6` |
| Code and test commit | `0965b74a8d17ac4690c7039e09640e63031b2ba5` |
| Fetched primary reference | `22489f09b4f780e82e9c8956befc2f341a60a69c` |
| Primary server / migration trees | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`; unchanged from prior blueprint source |
| New `native_quest_cost_checks.py` blob | `7ced2c50d110344787d77a559b5b4cda7a29aaf9` |
| New `test_native_quest_cost_checks.py` blob | `ac96d25ac5d4e04e1cc27988c589ff6cf23d29c7` |
| Reused `quest_cut_checks.py` blob, unchanged | `20ec1fa587ee00cfee1a040236df1dff800cb4ad` |
| Reused `case_data.py` blob, unchanged | `44c08a70031bde27ebb9a0996c2706e93a5cf8d0` |
| Existing capture reader, unchanged | `8a079422a452028e4ff8c5e94f8031fa9b350358` |
| Maintained `scripts/reconcile_economy_accounting.py`, unchanged | `e8b02432ef2b0cede6f8ad488a8309f05437ad4a` |
| Maintained `scripts/economic_restore_evidence.py`, unchanged | `06a2ebeaddd4bf604990b0f6cacf0c842555ccf3` |
| Reused registry | `f3797ec71cd79c7f24fb0f3c015af6ef5ad39754` |

The executable commit adds only the two new files under
`tests/async/quest_accounting_prep/`. The following documentation commit adds
only this handoff; its containing SHA is reported with final remote delivery.
All prior bundles, blueprint, canonical `HANDOFF.md`, existing assertions/capture,
production, shared drivers, schema and registries remain unchanged. Both new
paths were absent at the parent; there is no replaced helper preimage.

Read maintained source through the primary Git pin, because the prep worktree's
older production tree is not an integrated binary. Relevant primary source blobs:

| Source | Git blob / inspected contract |
| --- | --- |
| `src/economy/economic_gameplay_authority.c` | `4afa41bdb718068e4862f422b701fa5ad6953d56`, constructor983–1000: item-plus-fee source is birth operation, birth generation, pre-charge **stock revision**, original completion slot |
| `src/economy/item_transfer_accounting.c` | `b7012f63c3ce96573eaf23c0424d6447f71e74ac`, intent378–445/effects456–531: reason `quest_cost`, writer6, policy1, `quest_action`, mapped wallet plus requirement sink, one exact opposite posting pair per charged requirement |
| `src/world/quest_mobile_native.c` | `6c84016ab5351f00b4a7f22fe8a10db979d1e671`,660–679: frozen cash projection, one mobile/stock advance, original last transition |
| `src/economy/native_quest_cost.c` | `439ceade2196d33ac60e8428a12fda2f19564837`,24–106: maintained denomination-change calculation and one charged cash revision advance |
| `src/economy/native_quest_cost_policy.h` | `2ce45e8bdfc054cd3a50dbec75377a66467199a7`: policy version1, sink44/context0 |
| `src/economy/item_transfer_accounting.h` | `2654c99423c0c989a49ffe8aa7a666f773dbe5cf`: typed item writer6 |

Both numeric-policy headers match the prep headers used by the helper. The new
module reads those existing constants and registered reason/account/source IDs;
it does not add a registry or duplicate a monetary projector/decoder. Native
wallet context12 is the existing `item_owner_type::native_mobile` context. Item
and fee-only source identities differ; this helper accepts only item-plus-fee
QP02, never substitutes the fee-only operation-ID/mobile-revision rule.

## Callable interface and supported interval

```python
from native_quest_cost_checks import assert_paid_qp02

result = assert_paid_qp02(
    funded_before, consumed_after,
    original_instance=observed_original_instance,
    native_wallet=observed_original_mapped_account_key,
    cost_operation=observed_original_cost_operation,
    reward_vnum=19010,
    fee=10000,
)
```

This is a callable oracle, not a new runner or CLI. Select the exact QP02 recipe
by reward VNUM and explicit fee: gloves19010/C10000/four19006 roots, shirt19008/
C2000/two roots, shoes19007/C1000/one root. Unpaid backpack, another recipe or a
different fee refuses. Loader slots are gloves0, shirt2, shoes3. Those branch
terms come from the existing production QST helper; catalog sorting is not
runtime order.

The interval begins after any player-to-NPC funding has settled and after item
acceptance, with eligible roots already owned by the original recipient. It ends
after original consumption/cost projections agree, before reward issuance or
ACK advancement. The unacknowledged original reward obligation may and must be
created; publishing that obligation's rewards belongs to a later interval.
If an integrated owner offers no observable interval with these conditions,
the journey must report that boundary unavailable, not relax the oracle or
fabricate a stop. Native SQL commit and physical publication remain separate
observations, even when their captured projections eventually agree.

Checks implemented:

- Existing `bind`, `mobile`, `new_rows`, `book`, value arithmetic and maintained
  mobile/account/source decoders are reused. Candidate/schema/lineage/epoch
  bindings agree; native-shaped current epoch rows must exist. Missing fields
  raise `CutError`; input cuts are not modified.
- Original19005 reset-born live instance remains the same immutable birth.
  Mobile, stock and V2 cash revisions advance exactly once; the last transition
  is the explicit original cost operation. Replacement/other observed mobile
  rows remain unchanged. The reset source has actual npc-generation shape.
- Available original birth root/source/committed receipt/claim and nonempty
  opaque carrier are present, retained unchanged. Historical birth epoch may
  differ from the current charge epoch. No canonical-origin authentication is
  inferred from the opaque carrier or successful birth row predicates.
- One new original cost root/receipt/source claim, maintained reason/writer/
  policy/compiler, exact item-cost action source, and exact item/child counts.
  All prior immutable evidence stays unchanged; missing/duplicate roots or
  altered historical evidence refuse.
- Exactly two new effects: explicit original wallet key in the current lineage,
  wallet kind/context12, and the registered requirement sink. Mapping authority
  ID is never derived from instance ID, nor required to differ numerically.
  Wallet before/after denomination vectors and cash revisions match the native
  images; sink vectors/revisions follow the existing zero projection contract.
- Exactly two new root postings, exact account/event/line/child identities,
  wallet denominations equal native delta, sink denominations exactly opposite,
  checked copper delta exactly minus the selected fee, balanced root. Different
  denominations with equal scalar value, duplicates and offsetting extra charges
  refuse when they disagree with these captured vectors. Native cost range is
  retained. No second reconciler or sign/change algorithm is introduced.
- Player row/currency/history/affects/XP stay unchanged. No extra reward item or
  unrelated root appears. Legitimate selected19006 roots do retire with original
  native-to-destruction events/references; residual stock items stay unchanged.
  Consumption is not incorrectly required to preserve the whole recipient image.

## Proof limits and required original-owner inputs

The returned scope is `captured paid-QP02 agreement only`, with
`mapping_projection_publication_authentication` explicitly still required.
This helper cannot authenticate the account mapping to the mobile from current
capture fields. An account key supplied by a caller is an observation, not a
capability. Birth carrier/result bytes are checked only for availability and
row links; they are not decoded into invented authority. Actor metadata,
full command/intent/plan bytes and same-session locking are not in this cut.

The full journey must retain the genuine original command/payload, original
wallet mapping and same-cut lifetime/lineage validation, frozen cost projection
before/after vectors and revisions, exact attempted requirement slots/amounts/
outcomes, original branch/triggering acceptance/selected UID forest, full typed
intent/plan/result correlation and physical publication/held-owner evidence.
Only the maintained projector and original owner can prove the chosen change
vector. A deliberately coherent scalar-equivalent alternate vector across
images/effects/postings passes *consistency*; a unit test demonstrates that limit
and checks the returned requirement for external proof. Do not label that pass
as projector or native acceptance. Do not use this oracle as an authorization
predicate for mutation/recovery.

Current paid four-hide reachability, active numeric-GIVE refusal, authentic
initialization/acquisition/mapping and physical publication/ACK remain the
dependencies recorded in the completed QP02/QP03 blueprint and review. Prior
funding requires its own player-to-NPC operation/receipt/vector assertions.
The existing `static_complete` player-fee oracle is byte-identical and has not
been relabeled or weakened.

## Executed validation

Windows development command, with `TEMP/TMP=D:\Dev\Temp` and
`PYTHONDONTWRITEBYTECODE=1`:

```text
python -B tests/async/quest_accounting_prep/test_native_quest_cost_checks.py -v
```

Initial8-method and later9-method development runs passed on Python3.12.10.
Final source then tightened item-reference/root-count predicates; the following
WSL execution qualifies the exact committed code:

```powershell
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' -- env TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B tests/async/quest_accounting_prep/test_native_quest_cost_checks.py -v
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' -- env TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 python3 -B tests/async/quest_accounting_prep/test_quest_cut_checks.py -v
```

PASS:9 new modeled-oracle methods and17 unchanged quest-assertion methods on
Python3.10.12. Sensitivity subcases cover original/replacement, equal numeric
mapping/instance IDs, wrong mapping/context, missing cuts, V1/incorrect clocks,
source fields/policy/root/receipt/claim, cash/revision/denomination mismatch,
sink corruption, duplicate/offsetting charges, player debit/XP/task, reward/ACK,
residual stock and item references. Explicit modeled denomination change and
historical birth epoch controls pass. No SQL, accepting native stub, server,
migration, native compiler/build or genuine authority was used.

`git diff --check`, staged diff check and unchanged production/capture/assertion
helper comparisons passed. Tests use in-memory modeled cuts; no credentials,
logs, world/player state or binaries are committed. Existing jobs and evidence
remain in place; new scratch environment points to D:.

## Import and next bounded candidate

Select the compatible existing owned pack/dependencies above before this
additive code commit; primary does not contain that whole pack by implication.
Import code `0965b74a8d17ac4690c7039e09640e63031b2ba5`, then this handoff,
and run the new test on the actual selected candidate. Do not import the prep
branch's older production tree or unrelated fixes. Final native qualification
remains at the integrated primary major-batch boundary.

Next proposed slice, requiring a separate reservation: a temporal QP03 oracle
over original-A before/terminal cuts and genuine later-B birth/stable-replay
cuts, reusing existing `mobile`/`book`/history helpers. It would remove the
current simultaneous-A/B seam without implementing D or authenticating births.
Only a modeled consistency portion is independently executable today; the real
D result/service body, mapping/physical retirement, later reset/adoption and
held journal interfaces remain unavailable. No such helper is included here.
