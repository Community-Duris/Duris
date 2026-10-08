# Optional replacement world-quest task assertion — 2026-10-08

Added a small explicit four-cut assertion for the reviewed stale-map reservation:
captured original A, source-shaped reset, replacement B before original completion,
and B afterwards. It checks all fourteen task fields, XP and literal history at
the B comparison while permitting wallet denomination/revision changes. Its
result is **captured task/history/XP agreement only**; it never reports financial
restitution, native command execution or owner authentication as proven.

## Exact bundle and source pins

| Input / delivery | Exact pin |
| --- | --- |
| Preserved prep parent / reviewed service boundary | `04731d7e4a0893c5ceb9afaae8c92407d4871717` |
| New helper and focused test | `0aeeed7f92611947dd59f272c5799bd6fe752d0d` |
| Fetched public primary | `78d71393ee625a975b66d583b447577267e3616c` |
| Prior public source pin, unchanged source/schema | `257190ac149a86af59b1c3c2fe321abb382cd8ef` |
| Public source / migrations trees | `833d3085815b396861ad18a77635412212381e4b` / `7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2` |
| Helper Git blob | `8a8a641fb131da580083dbce5bebd705fac8bbbe` |
| Focused test Git blob | `ccf56d7ea82d606ef78554f38ae62e6d39d36c7c` |

The code commit adds only:

- `tests/async/quest_accounting_prep/world_quest_task_checks.py`
- `tests/async/quest_accounting_prep/test_world_quest_task_checks.py`

The following documentation commit adds only this handoff. Its exact SHA is the
containing commit, obtainable with `git log -1 --format=%H -- docs/persistence/economy_accounting/quest-prep/WORLD_QUEST_TASK_ASSERTION_HANDOFF_2026-10-08.md`.
Import the compatible helper/test bundle first. No adopted-primary or private
candidate API is inferred. Source-only private reports remain unexecuted and
unavailable here. The continuing actual native Goal remains **BLOCKED**.

## Existing overlap and domain evidence

The maintained `quest_cut_checks.refunded` compares the entire player row and
requires original debit/restitution/root linkage. Its modeled dynamic fixture
only has active/start/target/map-bought task fields. That accepted contract cannot
describe A→reset→replacement B or a legitimately canonicalized wallet by comparing
the whole original player row. No existing helper supplied this four-cut,
full-fourteen-field agreement. Neither `refunded` nor any accepted helper, capture,
default case, CLI or oracle was changed or automatically opted into this helper.

Reuse `capture_quest_cut.TASK_COLUMNS` directly and maintained `row`, `rows`,
`require`, `bind` and `CutError`. The focused fixture extends maintained
`test_quest_cut_checks.empty_cut` with the omitted actual capture columns. The
old fixture and its historical metadata pins stay unchanged. Such modeled
metadata are observations, not authenticated source/binary/schema proof.

Seven exact public files were inspected/exported and authenticated against both
public revisions. These source facts determine the bounds and stage predicates:

| Public source | Git blob | Used fact |
| --- | --- | --- |
| `src/core/structs.h` | `5ba85524e8ceee7ae085fc6637b501d528a41fe0` | Native PID and all fourteen task members are int; quest fields1344–1357. |
| `src/core/defines.h` | `7d5909d88a47176db88fb5ae71d8afd468ce038f` | FIND_AND_KILL1 / FIND_AND_ASK2, lines1393–1394. |
| `src/player/player_load_materialize.c` | `2ec4341c3b6ab4e768f7435958d2884f57306ffc` | Exact fourteen int32 task destinations588–600. |
| `src/player/player_snapshot_capture.c` | `9aacf75b74b45f1d2f0911e7325b9d7a3125921b` | Same ordered fourteen captured fields199–207. |
| `migrations/bootstrap_multithread_safe.sql` | `8b32494408010d12cc9a2064bb56974e0df28786` | Signed INT task columns1024–1037, SQL signed BIGINT exp985; SQL pid unsigned INT932, narrowed to positive native int for this player slice. |
| `src/world/world_quest.c` | `63b518d8d445dccea94b1748234ac6e1c8b46fc1` | Strict next-start119, reset127, share eligibility740 and installation841–869. |
| `lib/duris.properties` | `b284a15f2608a61edcd43595b60c3dfb938e66c8` | Sharing remains disabled at2360. |

All task values must be actual Python int in signed32 range; bool, float, string,
NULL and overflow refuse. Do not impose a universal nonnegative bound: reset
zone is -1, and literal signed storage agreement is distinct from valid live
zone/target authority. XP uses the captured SQL signed-bigint domain, not an
invented XP award cap. PID must be a positive native signed32 int and agree
with the cut's strictly typed metadata identity.

The original and B-before task must be active1, unfinished0, known task type1/2
and map-not-bought0. Original start is positive; reset preserves it and exactly
clears the other fields as the source does, including zone-1. B strictly advances
start and retains map room0 after sharing. Nothing authenticates the donor,
target/zone validity, clock, map availability or actual share execution from
these values alone. Coherent invented observations can satisfy agreement.

## Explicit optional API and comparison scope

```python
from world_quest_task_checks import assert_replacement_task_stable

agreement = assert_replacement_task_stable(
    actual_A_request_cut,
    actual_post_reset_cut,
    actual_B_before_original_completion_cut,
    actual_B_after_original_completion_cut,
)
```

All cuts must select one same observed player, bind to QP07 and retain consistent
candidate/schema/lineage/epoch/capture-selection metadata under the existing
binding rules. Native-shaped cuts require the native label. Explicit
`legacy=True` instead requires the maintained no-epoch legacy binding and returns
the same limited agreement; it cannot qualify active/native execution.

Exactly all fourteen B task fields must match before/after original completion.
B XP also matches, and the full captured history lists match as literal JSON
rows, including order/count/content and integer/float/bool distinctions. Dictionary
key order can differ. Missing history, malformed rows and NaN refuse. History
grammar/authenticity is not reimplemented. Task changes from A through reset/share
are accepted. Earlier XP/history observations are outside the B comparison;
allowing differences there does not claim reset/share itself awards XP or writes
history. The caller must obtain appropriate quiescent B cuts and prove attribution
with the actual command/owner timeline.

Wallet denominations, revisions, currency/inbox/operation rows and receipt links
are deliberately outside this task assertion. Canonicalizing10000 copper into
10 platinum passes task agreement, as can a changed net balance; neither is a
financial PASS. The unchanged `refunded` contract still rejects a changed whole
player row. Compose this helper with genuine owner finance/receipt assertions
when those are available; do not weaken `refunded` or relabel this helper as a
replacement financial oracle.

No extra item/affect check is required to establish task agreement. Capture
already supplies selected item and player-affect rows for a future journey's
separate effect/custody checks; this helper neither compares them nor claims a
complete census. Timer-driven affect changes also require an actual timeline.
The test explicitly permits arbitrary unvalidated wallet/currency/item/affect
changes while keeping all financial/native proof flags false.

Return fields name the observed PID, A/B starts, all fourteen task columns and:

- `scope="captured replacement task/history/XP agreement only"`
- `owner_authenticated=false`, `native_journey_proven=false`
- `financial_restitution_proven=false`
- Explicit external proof requirements for authentic command ordering/map
  availability/positive sharing/eligibility/delay; original request and PID/runtime
  lifetime/native giver birth/source/custody; debit/receipt and once-only
  restitution or retained obligation; physical publication, items/affects/GMCP,
  task/history/XP save, hold/ACK/replay/cold and retirement.

This adds no serialized context, operation identity, delay export or owner API.
Inputs are preserved on success and refusal; missing/malformed observations
raise maintained `CutError`.

## Focused qualification and retained evidence

Windows Python3.12.10, bytecode disabled, TEMP/TMP on D:. Commands actually run
from the existing prep worktree:

```powershell
$env:TEMP='D:\Dev\Temp'
$env:TMP='D:\Dev\Temp'
$env:PYTHONDONTWRITEBYTECODE='1'
python -B tests/async/quest_accounting_prep/test_world_quest_task_checks.py -v
python -B tests/async/quest_accounting_prep/test_quest_cut_checks.py QuestCutTests.test_dynamic_refund_needs_exact_debit_and_once_only_restitution -v
python -B D:\Dev\Temp\world-quest-task-assertion-20261008\qualify.py
python -B D:\Dev\Temp\world-quest-task-assertion-20261008\seal.py 0aeeed7f92611947dd59f272c5799bd6fe752d0d
git diff --cached --check
```

**19/19 focused modeled tests PASS**, final retained unittest runtime0.194s.
**The unchanged original refund test PASS**, runtime0.002s, preserving all three
fee fixtures and their mutation/linkage controls. Its original file is byte-for-
byte unchanged. The external qualification controller ran those two exact
entry points with60-second deadlines, exit0/0. Staged whitespace checks passed.
The initial focused run also passed; the final frozen run includes the explicit
map-availability external-proof text and retained output.

Sensitivity covers same-target/newer B; independent mutation of every final task
field; every missing field at every stage; bool/float/string/NULL/container and
both signed32 overflow directions for every field/stage; reset values and lost
watermark; nonadvancing/replaced attempt; active/completed/type/map conditions;
wrong/duplicate player and binding; XP missing/types/signed64 bounds/mutation;
history missing/extra/removed/literal type/content changes; legitimate reset/share
differences; canonical wallets; explicit legacy mode; input preservation. Signed
storage endpoint examples and coherent forged pins pass only with all authority/
native/financial flags false. These are modeled oracle controls, not gameplay.

Evidence root: `D:\Dev\Temp\world-quest-task-assertion-20261008`.
Qualification records all seven public blobs/SHA-256, twelve unchanged imported
code/data dependencies, interpreter, exact commands/deadlines, before/after
hashes, modeled cuts/result and retained stdout/stderr. Seal proves the committed
helper/test bodies equal the qualified bodies. `artifact-index.json` hashes19
payload files, with the index additional.

| Artifact | SHA-256 |
| --- | --- |
| Qualified/committed helper | `2c139da4f492a8a4cb15291b8062a35d2da3468ae06083c049a1ee73f09b5342` |
| Qualified/committed focused test | `2c24192d43f9c4ae4d49eae7bcda097ec0273a5a880ce7b3735d81f969712698` |
| `qualification-receipt.json` | `8ad6ff217283201ed9f357bdc45d463e8c1267bb2e54c05baae9a416c11d994c` |
| `artifact-index.json` | `0ce10f24d16662381cd80b842c55751f75778678ea116256a8699f104fcaf842` |

No capture/production/shared driver/schema/registry/Plan5/finish-plan/canonical
HANDOFF changes; no DB/client/server/journal/C++ build/native journey or unchanged
broad batch ran. Earlier financial and runtime failures remain intact.

## Remaining integration boundary

Preserve the four native prerequisites in
[the reviewed service boundary](WORLD_QUEST_SERVICE_AUTHORITY_BOUNDARY_2026-10-08.md):
genuine service admission/lifecycle; original-operation delay/release;
retained attempt/PID/runtime/receipt across business/save/ACK/cold; and original
debit-linked restitution or a durable outstanding obligation. Default shares0
remains unavailable. Public active generic bartender spend and generic credit
refusals are not resolved by this assertion; the conditional birth hook/special
restriction is not bypassed.

The three-file optional preparation bundle is reviewable at this handoff.
Reassess native execution when those actual contracts and an integrated candidate
are published. Preserve major-batch qualification cadence. This modeled result
does not resume or complete the actual blocked native Goal.
