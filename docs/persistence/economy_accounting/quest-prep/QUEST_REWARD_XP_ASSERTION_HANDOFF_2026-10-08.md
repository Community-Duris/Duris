# Optional captured group quest XP agreement — 2026-10-08

Added one optional pure assertion for declared original frozen XP awards and
per-recipient SQL-shaped observations. It compares exact original-operation,
PID, reward-slot, frozen-input amount and application flags. It checks owner
mask/ACK implications only inside the supplied owner observation. It neither
joins independently timed cuts into a common snapshot nor authenticates their
decoder, owner, timeline, saved progression or native lifecycle.

## Exact delivery and candidate pins

| Input / delivery | Exact pin |
| --- | --- |
| Preserved prep parent / accepted completion boundary | `d4d048c29ce8cc05ddf7c820fe6384cbd7ed014d` |
| New helper/test code commit | `306008c1b51560bd5bcad943d9fac5d026603432` |
| New helper / test Git blobs | `09a43636c63fc776cebf6f9f83682b8aa9528a9c` / `d5f21493b44e6864c03263e7a7e6da67eb995df1` |
| Fetched public accounting | `07c0e0398e0123ad9232e162cd1d1df08fd38a6d` |
| Compared preceding public candidate | `f679ee312baccbe077267aedd36fceaa2f97b14f` |
| Unchanged public source / migration / test trees | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` / `a9559094ab4b9d184de97cb0ac135ed146467b73` |

Code commit adds only `tests/async/quest_accounting_prep/quest_reward_xp_checks.py`
and `tests/async/quest_accounting_prep/test_quest_reward_xp_checks.py`. The next
commit adds only this handoff; its exact SHA is the containing commit, obtainable
with `git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/QUEST_REWARD_XP_ASSERTION_HANDOFF_2026-10-08.md`.
Import compatible code first, then this document. No accepted helper, capture,
decoder, legacy solo policy, shared driver, production, schema, registry, finish
plan, canonical HANDOFF or Plan 5 file changed. No automatic opt-in or new CLI.
Private `d9eaa45b/143production/77C` reports remain source-only, unavailable and
unexecuted here; the new public documentation supplies no new quest API.
The actual continuing native Goal remains **BLOCKED**.

## Provider and shape grounding

Reuse the [accepted completion boundary](QUEST_REWARD_COMPLETION_AUTHORITY_BOUNDARY_2026-10-08.md),
existing Kord/legacy evidence and existing group component coverage. This delivery
does not repeat recipes, effective-XP policy, group gameplay or ACK/cold journeys.

- Maintained [continuation terms/decoder](https://github.com/Community-Duris/Duris/blob/07c0e0398e0123ad9232e162cd1d1df08fd38a6d/src/item/quest_reward_continuation.h#L41)
  have64 reward slots and64 **total** XP tuples. Version5/6 validate positive
  uint32 PIDs, bounded signed-int reward/amounts, unique PID/slot tuples and a
  complete award for every credited PID at every XP slot. The decoder explicitly
  requires `party_size == credited_count` at243/442; this is source-supported,
  not an assumed gameplay identity. With a nonempty full XP export, distinct award
  PIDs therefore recover that cardinality. No new `credited_count` JSON field or
  wire decoder is introduced.
- Existing owned `read_quest_continuation.cpp` exports exactly version, owner PID,
  mobile VNUM, original level, party size and the **full** PID/index/amount award
  list through `quest_reward_continuation_decode`. Existing `legacy_xp.decode`
  invokes that adapter. Its solo `kord` assertion remains unchanged and separate.
- Maintained [original admission](https://github.com/Community-Duris/Duris/blob/07c0e0398e0123ad9232e162cd1d1df08fd38a6d/src/persistence/critical_command_repository.c#L799)
  creates entitlement rows only for version5+ credited groups larger than one;
  this new assertion deliberately supports **group version5/6**, not solo/version4.
  SQL [entitlement identity/storage](https://github.com/Community-Duris/Duris/blob/07c0e0398e0123ad9232e162cd1d1df08fd38a6d/migrations/immutable/0048_quest_xp_entitlement.sql#L3)
  is original operation/PID/slot with unsigned INT PID/slot/amount. Amount is
  further constrained by the decoder's positive signed-int reward domain.
- Existing `capture_quest_cut.py` selects obligations by owner PID and entitlements
  by selected recipient PID. It returns integer0/1 derived flags, continuation
  hex and unsigned64 owner XP mask. It can include unrelated original operations.
  Each cut starts its own read-only consistent snapshot; cuts do not share a
  transaction. The helper uses that existing selection shape without changing it.
- Maintained [exact original reader](https://github.com/Community-Duris/Duris/blob/07c0e0398e0123ad9232e162cd1d1df08fd38a6d/src/persistence/quest_reward_obligation_repository.c#L835)
  binds full original tuples and owner mask in one real transaction. Its ACK
  validation includes all recipients and economic effects. This optional helper
  copies only the **local owner** XP-mask/ACK implication that the available owner
  cut can express; it does not replace that reader or extend per-PID captures.

Ten exact public bodies and thirteen unchanged prep/provider dependencies are
exported with Git blob IDs/SHA256s in
`D:\Dev\Temp\quest-reward-xp-assertion-20261008\source-manifest.json`.
Nineteen checked excerpts ground the array/domain/coverage, insertion, SQL ACK,
mask, capture and decoder-export facts. Modeled fixture metadata retain the
existing builder's historical `275df7f626e12cb396a22da34317a4e7f355e9a1` pin;
those invented observations are not evidence from the new public candidate.

## Explicit optional API

```python
from legacy_xp import decode
from quest_reward_xp_checks import assert_frozen_xp_agreement

# Future consumer: actual captured owner literal and one actual cut per peer.
original = "<exact original 32-character lowercase operation hex>"
literal = next(q["continuation"] for q in owner_cut["obligations"]
               if q["offering_operation_id"] == original)
decoded = decode(literal)  # existing maintained adapter; external to pure check
agreement = assert_frozen_xp_agreement(original, decoded, owner_cut, peer_cuts)
```

`peer_cuts` must be a list containing exactly one cut for every decoded peer,
excluding the owner cut. Order is irrelevant. Each selected player PID must be a
positive uint32 and exactly agree with its metadata; this is SQL/decoded identity
agreement, not proof that a uint32 value fits a live native signed-int actor.
The helper requires version5/6, nonempty complete group/slot coverage, at most64
total awards, slots0–63 and amounts1–INT32_MAX. It checks exact Python integers:
bool, float, string, NULL, zero where positive, negative and overflow refuse.

Reuse maintained `rows`, `row`, `require`, `bind` and `CutError`. Normalize only
the explicitly validated differing selected PID for cross-recipient binding;
case/source/binary/schema/lineage/epoch/mobile selection/watched kinds and
migration observations must agree. Typed JSON comparisons also refuse bool/int
metadata conflation. Native mode requires the existing native label. Explicit
`legacy=True` requires maintained legacy-no-epoch observations and still returns
only group tuple agreement. Neither label authenticates an execution mode.

Filter all obligation/entitlement observations by the explicit selected operation.
Require one owner obligation, no foreign original-owner obligation in a peer's
PID-selected table, and every expected original tuple exactly once in its own
recipient cut. Missing, extra, duplicate, wrong-PID/slot/amount or conflicting
selected-original rows refuse. Well-formed unrelated operation IDs are excluded;
their other columns/effects are not validated by this helper. Captured tables
retain the existing2048-row bound. Original literal must be nonempty lowercase
hex within the maintained8192-byte continuation bound; checking its representation
does not decode or authenticate it.

In the **same supplied owner observation**, XP mask must equal the bits of applied
owner entitlements; an observed owner ACK1 requires all expected local owner XP
slots applied. Pending ACK0 remains allowed when owner's XP is fully applied.
Peer application flags are reported without any cross-cut ACK implication. A
newer ACKed owner cut plus an older unapplied peer cut **passes local agreement**;
the independently timed observations do not establish a contradictory schedule.
A caller-supplied coherence/schedule label changes no proof flag.

The result reports original operation, expected tuples, each local application
set and observed owner mask/ACK. `owner_authenticated`,
`decoded_terms_authenticated`, `common_time_snapshot_proven`, `ack_order_proven`,
`effective_xp_proven`, `save_completion_proven`, `native_journey_proven` and
`retirement_proven` are always **false**. Inputs remain unmodified on success and
refusal. Missing/malformed evidence raises existing `CutError`.

JSON alone does not bind the export to the supplied literal. In particular this
smaller export omits reward count/types and the credited-PID vector. Coherently
deleting a whole XP slot from both an invented export and all invented cuts can
still pass declared agreement. The forged-pack test demonstrates that limit and
all false authority flags. Authentic decoder execution, source/world census,
retained participant/custody and real common-time observation remain external.
No source absence is promoted to global absence. No effective-XP formula, receipt
authentication, save/ACK ordering, economic evidence or pair lifecycle is inferred.

## Executed checks and remaining work

From the existing prep worktree, the bounded proof runner actually executed:

```powershell
python -B D:\Dev\Temp\quest-reward-xp-assertion-20261008\verify.py
```

It records these commands with45-second subprocess deadlines and bounded outputs:

```powershell
python -B tests/async/quest_accounting_prep/test_quest_reward_xp_checks.py
wsl.exe -d Ubuntu-22.04 --cd '/mnt/c/Users/alexa/.codex/worktrees/accounting-quest-prep/NewDuris Max' --exec env TMPDIR=/mnt/d/Dev/Temp PYTHONDONTWRITEBYTECODE=1 PYTHONPATH=tests/async/quest_accounting_prep:tests/async python3 -B -m unittest test_quest_cut_checks.QuestCutTests.test_legacy_complete_still_requires_exact_custody_and_xp test_quest_cut_checks.QuestCutTests.test_dynamic_refund_needs_exact_debit_and_once_only_restitution
git diff --check
```

Results: **23 new modeled tests PASS**; **two unchanged compatibility controls
PASS**. New cases cover two/three recipients, multiple slots/manual amounts,
mixed flags, versions5/6, bounds including slot63/uint64 mask/64 total awards,
tuple and metadata refusals, local ACK limits, unrelated-operation coexistence,
unmodified inputs, missing evidence and coherent fabrication. The unchanged
legacy control verifies exact custody and missing solo XP evidence refusal;
it does not run the decoder or re-execute a Kord journey. Refund control retains
its original financial/whole-player contract. The first Windows-Python combined
compatibility attempt had one existing `fcntl` import error and one refund pass;
the identical unchanged controls subsequently passed under installed Ubuntu WSL.
No compatibility code was changed to hide that platform requirement.

Source manifest, anchor excerpts, bounded test output/commands and verification
are on D:. Publication receipt records exact code/document/remote commits, owned
hashes and clean state; artifact index seals the proof files. Temporary manifest
discovery corrected two script locations and one variable token before source
checks passed; these were evidence-tool corrections, not quest failures.

No SQL, build, server, journal, native or broad batch ran. Genuine group setup,
authentic original native admission/custody, owner-coherent multi-recipient
observations, actual XP/save receipts and ACK/cold/pair qualification remain with
existing shared owners. Final qualification uses the integrated primary candidate
at its major-batch boundary. This optional input assertion does not resolve those
blockers or reopen the accepted solo/refund/task/pair bundles.
