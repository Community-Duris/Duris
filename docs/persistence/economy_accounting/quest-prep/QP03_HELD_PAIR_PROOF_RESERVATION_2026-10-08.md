# QP03 original held-pair proof reservation — 2026-10-08

Reserve a bounded **value reader plus captured-agreement assertions**, using
owner-exported original parent/child commands and attachments. Existing pure
decoders make that slice feasible. Existing SQL cuts, public context validation
and journal replay do **not** supply the genuine hold/publication/retirement
observation needed for native acceptance. The specific integration prerequisite
is an export from the existing guarded owners, described below; no such export
API is implemented or claimed by this reservation.

This delivery adds only this document, based on prep
`f023c9fdaec7d15436b2a87ff6669a87efd54045`. The containing sole-document commit
is reported with remote delivery. Primary pin:
`a9807ef2757cf79f5c5132f4753e6c936479c735`; source tree
`833d3085815b396861ad18a77635412212381e4b`; migrations tree
`7e06717b85ea7a5e27a1096fdb9cd9f124bd60c2`. No merge or source replacement was
needed. Original native Goal remains **BLOCKED**. No maintained code/test/driver,
codec, schema, registry or shared-owner edit, DB/server action, native build or
gameplay batch is part of this reservation.

## Reuse and concrete scope

[RECOVERY_BATCH.md](RECOVERY_BATCH.md#L284) already requests the actual
`retire_completed` observation: original pair before, exact original receipts and
obligation/ACK in one transaction, confirmed rollback, then actual guarded
`transition_pair` return. Keep that request and the original missing-parent,
mismatched-command/receipt, unacknowledged-XP and started-unreturned controls.
Do not reproduce its journey inventory. Earlier results and native/legacy ELF
pins retain their original classifications; no prior legacy reward-move or
historical ACK result is promoted to native pair retirement here.

Reuse `quest_cut_checks.bind`, `held`, `replay`, `acknowledged`, history/book
patterns and the existing SELECT-only capture. `held` proves a stable accepted,
unacknowledged SQL cut; `acknowledged` requires a historical acknowledged row.
Neither sees original command bodies, live save holds, coordinator uncertainty,
physical release or the pair-transition return. The new temporal QP03 oracle
explicitly leaves these requirements external. Its A/B tests stay unchanged.

Reuse `read_quest_continuation.cpp`'s bounded read/decode/print pattern. That
adapter decodes only reward terms; it cannot decode the recovery attachment or
identify the original parent/child pair. Use maintained native context/command
decoders for that new input, rather than extending a legacy XP assumption.

**Proposed next implementation paths, absent at this reservation's parent:**

1. `tests/async/quest_accounting_prep/read_native_quest_pair.cpp`: bounded passive
   reader of two owner-exported command/attachment bodies and their observed
   envelope revisions/phases. No journal, SQL, world, source allocator or owner
   capability is linked or invoked.
2. `tests/async/quest_accounting_prep/native_quest_pair_checks.py`: callable
   stable-held and terminal-retirement captured-agreement assertions over the
   reader's results, unchanged SQL cuts and the genuine owner observation.
3. `tests/async/quest_accounting_prep/test_native_quest_pair_checks.py`: focused
   negative controls for those added assertions/adapter bounds, reusing existing
   modeled value fixtures where applicable. Keep modeled provenance explicit.

These paths and interface sketches are **proposed**, not implemented or approved
production hooks. First implement only original terminal QP03 correlation; omit
prefix-successor mutation, generic journal tools, new schemas and generalized
recovery machinery. QP03 uses an original v12 acceptance parent and v12 consumption
child, exact original Auriam16006 binding, disappearing completion and its frozen
quest-offering continuation. Paid/fee-only v14/version6 must not be silently
treated as this route. Preserve its source exception in any later extension.

## What the maintained interfaces actually establish

All source links below use the pinned primary, not the older prep source tree.

| Maintained boundary | Actual guarantee and limit |
| --- | --- |
| [Context carrier/decoders](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/world/native_quest_recovery_context.h#L12) | Pointer-free original inventories, receipt, publication stage/steps, literal branches and original child command; latest child carrier retained separately. Canonical decoding binds every original command byte, with strong unchanged-output refusal. No source, SQL, physical or ACK authority. |
| [Pair validator774](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/world/native_quest_recovery_context.c#L774) | Both revisions nonzero/phase2; distinct operations; parent's exact next-child command; child decoded publication stage/receipt and optional parent link. Null successor accepts quest-offering shape or terminal rejection. It does not authenticate the stage/receipt against real execution or authorize completion. |
| [Private continuation owner](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/world/native_quest_frozen_continuation.h#L17) and [private coordinator methods](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_coordinator.h#L316) | Actual context borrowing, parent selection and transition are friend-restricted. A prep adapter cannot call them or manufacture a friend capability. Copy-parent refuses missing/ambiguous/unsuitable original state. |
| [Save publication5627–5718](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/player/player_save_pipeline.c#L5627) | Exact frozen command, original inventory/save revision, held generation/runtime binding, worker/revision/fence checks, actual native publication and publication census before/after. It alone forwards the guarded publication ACK. |
| [Guarded publication ACK3677 onward](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_coordinator.c#L3677) and [hold consumption3316](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/player/player_save_pipeline.c#L3316) | Journal phase2 transition must return OK; exact-generation hold consumption occurs outside coordinator lock. Only a successful consumption marks `native_physical_released` at3884–3890. A phase2 frame or callback-entered flag alone is insufficient. |
| [Operation-held probe324](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/player/player_save_execution_guard.h#L324) | Mutex-protected local boolean only. No complete original body, PID/generation/save/SQL/physical correlation or historical retirement proof. Never inspect/mutate `detail::holds` from the adapter. |
| [Retirement2902–3026](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/world/quest.c#L2902) | Game-thread/current/unpoisoned/no-pending state; actual copied parent; structural pair check; live handoff1 parent must reach actual handoff2. Once an actual attempt starts, both complete preimages remain pinned. |
| [Original native receipt verifier4979](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_repository.c#L4979) | On the caller's active transaction, authenticates original command/key identity, committed native receipt, native root and same reconnect-disabled session. Selected inbox fields alone do not duplicate this verifier. |
| [Exact obligation reader718](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/quest_reward_obligation_repository.c#L718) | Original PID/operation/literal continuation, inbox, native item/cash witnesses and complete original XP set; acknowledged rows require all original economic/XP receipts. Reads only, strong unchanged-output refusal, bounded query metrics. |
| [Cleanup proof](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/player/player_sql_transaction_cleanup.h#L13) | Explicit `finish`, same session, rollback-confirmed, zero cleanup error, idle-verified disposition. Destructor fallback is not successful reuse proof or an ACK. |
| [Coordinator transition2902](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_coordinator.c#L2902) | Exact phase2 commands/attachments/revisions; both operations present, physically released, no pending publication checkpoint or ACK uncertainty; registered structural validator; retained-budget/lifecycle/generation pins across journal I/O. Context uncertainty may retry the same exact pair. Successful journal result alone still requires the guarded post-I/O identity checks. |
| [Journal pair1097](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_journal.c#L1097), [rewrite579](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_journal.c#L579) | Both exact original phase2 frame bodies must exist; parent replacement/deletion and child deletion share one rewrite. Full expected parent/child and complete attempted postimage are retained on uncertain rename. No parent-child, SQL, ACK or world authority follows from this layer. |
| [Uncertain confirmation462–502](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_journal.c#L462) | Confirms exact attempted pair/successor/retirement mode and **whole** postimage size/bytes, followed by directory fsync/close. Two missing selected frames do not establish that complete postimage. |

The decisive ordering in `retire_completed`: parent/child exact receipt reads at
2979–2985; version6 substitutes the actual native-fee verifier for the child
receipt; exact acknowledged obligation2997–3003; same-session confirmed cleanup
3010–3015; then `terminal_pair_attempted` and actual transition3020–3023. Only
success sets `retired`3024 and calls cleanup3025. Its already-retired fast path
is a local latch from that prior success, not a second journal attempt.

## Minimal future input/output contract

The proposed reader consumes **existing canonical bytes**, not journal frames:
parent and child `critical_command_encode` bodies, their exact context attachment
bytes, and owner-observed nonzero revisions/phases. Use maintained
`critical_command_decode`, `item_transfer_command_decode_payload`,
`native_quest_recovery_context_decode` and, for a terminal phase2 pair,
`native_quest_recovery_pair_context_valid(parent, child, nullptr)`. Decode original
quest terms through `quest_reward_continuation_decode`; never derive commands or
attachments from current native/player images. Re-encode only to compare exact
canonical bytes with the supplied command; no encoder may repair an input.

Bound each command by `CRITICAL_COMMAND_MAX_ENCODED_BYTES`512KiB and attachment
by `CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES`32MiB, read at most the two
selected envelopes, reject truncation/trailing data and failed canonical decode,
and keep process output bounded (proposed64KiB). Read immutable owner-exported
regular files only, with before/after hashes and no symlink following; never
accept a journal directory as input. No bytes or private player/journal payloads
belong in Git. A path/hash is not export authenticity.

Output should expose only correlation fields needed by the assertions: original
operation/PID/native-instance/birth binding; command/attachment byte counts and
digests; phase/revision/payload version; handoff and child-operation linkage;
frozen completion/continuation digest; publication-stage/step states; complete
receipt outcome/error/failure-stage/durable-revision/result-size and result-array
digest; and structural-pair result. Retain the actual private input bytes for
maintained comparison. Label output `original-pair value correlation only`.
Decoded `physically_proven` is a carrier value, not new physical proof.

**Additional owner observation prerequisite — desired evidence, not an existing
API or a caller authority token:** obtain immutable observations from the actual
save/publication, frozen-continuation, coordinator and journal owners. They must
bind the selected bodies/digests to one isolated run, source/binary/schema pins,
lineage/epoch, process/coordinator/hold generations and exact ordered attempt.
Required contents and cuts are:

| Cut | Genuine observation needed | Proposed assertion |
| --- | --- | --- |
| H1/H2: stable accepted-but-held | Original parent and child envelopes; child's actual save hold PID/generation/frozen command/save revision; owner uncertainty/poison/pending states; accepted SQL receipt and original unacknowledged obligation. A lost reply may leave the context receipt absent: record it, retain hold, never infer publication. | Existing `bind`/`held` plus unchanged exact retained envelopes/hold identity in the quiescent same-process interval. No economic or physical replay permission. A phase1 child is decoded but must not pass the phase2 pair validator. |
| P: publication ACK/release | Actual native owner's physical BEFORE/AFTER proof and publication census; matching guarded completion; original phase1→phase2 checkpoint result; actual exact-generation hold consumption result; coordinator's resulting physical-release/uncertainty state. | Correlate original bytes/revisions and receipt with the successful owner path. Distinguish publication ACK from later quest reward-obligation ACK. A phase2 frame with failed hold consumption remains unresolved. |
| A: immediately before terminal attempt | Both actual phase2 preimages; refreshed original parent/handoff; exact parent and child verification results and full retained receipt comparisons; original continuation and exact acknowledged obligation including complete XP/economic masks; same SQL session and explicit cleanup proof. | Require real non-fee QP03 checks, both successful historical native receipts, obligation ACK and successful cleanup. No inference from `acknowledged_at` alone. Current reward custody may have legitimately advanced. |
| J: actual attempt/uncertainty | Same pinned pair and generations; null successor/terminal mode; journal return; whole attempted postimage byte count/digest and original journal-owner confirmation; guarded coordinator return, retirement latch and uncertainty disposition. | On failure/uncertainty retain both original preimages, do not claim retirement or repeat rewards. A rename may leave both frames absent while the attempt is still uncertain. Compare full attempted postimage through owner evidence, never reconstruct it from selected frames. |
| T: successful terminal return/repeat/cold | Journal OK **and** guarded coordinator true **and** actual original owner retired latch/cleanup; event provenance tied to the same attempt. Preserve this success observation for later comparison, original SQL history and reward UID. | Retirement agreement requires the captured successful path, not later frame absence. A repeated local latch return is not a new mutation. Cold restart does not recreate an erased state/latch or authenticate historical success; retain the genuine prior observation. |

No current export exposes this combined evidence. In particular the public
operation-held boolean, journal health counters and publication-ACK trace do not
export original held bodies/generations, exact pair postimage or the private
retirement result. Do not replace those missing observations with accepting
stubs, synthesized booleans, GDB-written state or a reconstructed command.
Ordinary cross-process generations may change on genuine cold rebind; require
the actual owner rebind proof, not equality with an old process's generation.

Proposed Python callables are `assert_qp03_held_pair(...)` over H1/H2, and
`assert_qp03_pair_retirement(...)` over A/J/T plus original SQL cuts. These names
are reservations only. The terminal SQL interval begins after original ACK and
uses `replay` to forbid second item/money/XP/history/obligation effects; pair
cleanup must not republish or repossess a legitimately moved reward. A successful
captured-agreement result must still return external requirements for export
authenticity, actual owner execution, D physical/census proof and chronology.
An internally consistent forged report cannot authenticate its own provenance.

## Offline journal decision and remaining blockers

Do **not** build an offline journal parser in the owned adapter.
`scan_bounded`/native frame decode are private. Public
[`critical_command_journal_init`718–770](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/persistence/critical_command_journal.c#L718)
uses mkdir and `O_WRONLY|O_APPEND|O_CREAT`; replay1106 requires initialized state,
updates health/calls callbacks, and refuses active append/rewrite uncertainty.
Even initialization on a copied journal is not a strictly read-only scanner.
Opening the live journal, calling sync/transition/replay to settle uncertainty,
or duplicating its frame/CRC format is outside this slice and unnecessary.

The primary must first supply an owner-observed export of the exact original
canonical command/attachment values and guarded outcomes. A private, quiescent
owner-generated copy may be evidence input later, but the adapter must consume
the existing exported value bodies, not initialize a journal against it. If a
read-only whole-journal/postimage export is needed, its maintained implementation
belongs to the journal owner and requires a separate reservation; Plan5 retains
independent canonical audit/restore. No new shared API is claimed available.

Genuine native QP03 H/P/A/J/T captures remain unavailable: successful D publication
still refuses at [native_publish7694](https://github.com/Community-Duris/Duris/blob/a9807ef2757cf79f5c5132f4753e6c936479c735/src/item/item_movement_transaction.c#L7694)
until the original whole-stock/cash/lifetime owner exists. Authentic original
reset birth/custody, full literal forest/world census, delayed original callbacks,
chronology and same-owner SQL/ACK observations are prerequisites. Current SQL
capture selects original inbox/obligation/XP/economic/native rows, but neither
command bodies nor private live holds/pair disposition. Its selected XP rows are
also not a substitute for the exact reader's complete original entitlement set.

## Focused qualification reserved for review

Reuse existing pure-context roundtrip/command-binding/receipt/pair/prefix controls
in `native_quest_recovery_context_test.cpp`; do not duplicate them as new tests.
Its maintained runner links15 actual source files under unchanged C++20,
ASan/UBSan, warnings-as-errors and20s runtime deadline. Treat that as a canonical
provider starting point, not proof that the unimplemented reader links; use
`native_build_artifacts` and genuine needed providers, never owner stubs. The
legacy continuation reader's canonical providers are another existing reference.
Resolve the new reader's actual link closure during its separately reviewed
implementation and preserve applicable compiler/sanitizer/deadline requirements.
Keep new qualification artifacts on D:. The existing context runner requires
`bin/tests` under its composed candidate; use a D: scratch candidate rather than
weakening that path guard or moving an existing checkout.

New sensitivity must target the proposed observation joins: missing parent/child
or original bytes; swapped operation/birth/PID; wrong revision/phase/handoff;
command-bound attachment damage; unreturned publication steps; missing/lost
receipt; phase2 without consumed exact hold; ACKed SQL without owner verification;
incomplete original XP/economic receipt mask; changed SQL session or failed
rollback/cleanup; changed pair/postimage across uncertain retry; journal OK with
coordinator false; false retired latch; missing frames without captured success;
duplicate terminal/economic effects; and legitimate later reward move preserved.
Include bounded-input/trailing-data/unchanged-output controls and report the
modeled provenance of any observation fixtures. Generic journal-append uncertainty
tests use type::test with unavailable native capability stubs that abort; they
do not cover this native pair. The existing MySQL publication harness proves its
SQL cut component, not live save-hold consumption or original pair retirement.

The next implementable slice, after coordinator review, is the three owned files
above for pure exported-value decoding and modeled observation-join sensitivity.
It can proceed without a real D owner only with explicit captured-agreement
limits. Genuine H/P/A/J/T qualification must wait for the exact owner export and
original runtime prerequisites, then use the integrated primary at its normal
major-batch boundary. Do not introduce another acceptance milestone or repeat
unchanged broad journeys while those inputs are absent.

## Frozen dependency preimages and executed source proof

Every primary path below is frozen by `a9807ef2` and source tree833d above;
full SHA is in the opening pin. These are source/dependency preimages, not new
implementation results. The pinned runner fixes its15 linked source paths/flags;
the primary source tree fixes all their maintained headers/providers.

| Primary path | Git blob |
| --- | --- |
| `src/world/native_quest_recovery_context.h` | `d3e3b42bd558a9a92aa0592afbc2183554c48d73` |
| `src/world/native_quest_recovery_context.c` | `1ece381de10406d224bcce76c8e648e5fea108e8` |
| `src/world/native_quest_frozen_continuation.h` | `5c1809f8bb86d5fc2196a5d3374df7bc9877ac9c` |
| `src/world/quest.c` | `4ee99c6c1020250395cba9bdfa3e47f42a642cfe` |
| `src/persistence/critical_command_journal.h` | `7d63bf95051a8244205b7cdefda4b2e8ff488634` |
| `src/persistence/critical_command_journal.c` | `a64124327c421ff928597619600597b534fbc600` |
| `src/persistence/critical_command_coordinator.h` | `0c9d9a9293a63635220097b5b2bd8d7d0c25f5bd` |
| `src/persistence/critical_command_coordinator.c` | `43a5b4f15c9171703e77b89b84bbc0c6f3a49b16` |
| `src/player/player_save_pipeline.h` | `e1c2d87094c2eeaa120ceff65406ac7f7b7d5baa` |
| `src/player/player_save_pipeline.c` | `c308866ae0d569677bf0cec40d000739f95475ac` |
| `src/player/player_save_execution_guard.h` | `1dfdb7767c13f31d701007b9445158de1562a136` |
| `src/player/player_sql_transaction_cleanup.h` | `7f9d5ae325a0c4687eee305bc56546b2c790adc3` |
| `src/persistence/critical_command_repository.h` | `7074bc389bcb9216e6c6cf8b6d20287722c8154c` |
| `src/persistence/critical_command_repository.c` | `1f69092f0b3070d665952ec8c8638c1ec5d09b00` |
| `src/persistence/quest_reward_obligation_repository.h` | `e0d36d28c5c7f63f6a3592d6ea019f65818e72d2` |
| `src/persistence/quest_reward_obligation_repository.c` | `05f178549c7f4204711f88005cea78bdda2d76b3` |
| `src/persistence/critical_command.h` | `c1262dd3bd7f604b2de56cf23fcf64e15abb51c9` |
| `src/item/item_transfer_command.h` | `f1d8a67d565133eeb45b50135dd82af82f50c439` |
| `src/item/quest_reward_continuation.h` | `34431765330fe5afeabc314e061705f58e7f1524` |
| `src/item/item_movement_transaction.c` | `9d2ce19fd843eef7ce2bda7c41348471b5a2864b` |
| `tests/async/test_native_quest_recovery_context.py` | `c8ca1aeb936ef0f36c10829159494ef814672636` |
| `tests/async/native_quest_recovery_context_test.cpp` | `6209343c9b07bb143f3df1f1e087fcddbabf79a8` |
| `tests/async/test_critical_command_journal_uncertain.py` | `d5d19544124254ebda26a403f83fa2b02c121918` |
| `tests/async/test_critical_command_journal_faults.py` | `b196f7d7e92df40a687ad33fe61cc648fab42d27` |
| `tests/async/test_publication_ack_checkpoint.py` | `e55aea09b78ac185eed79e96913ec79784e5703a` |
| `tests/async/native_quest_publication_mysql_harness.cpp` | `c92476bfb905eb7cc2a1987c501a2b05f3be5976` |
| `tests/async/run_native_quest_publication_mysql.sh` | `acd6adee49fa1f46bf12941f2abf1c598ee544b7` |
| `tests/async/native_build_artifacts.py` | `a1b5c1b6a4b96930425384e69f830b1e136244f6` |

| Existing prep dependency at `f023c9fd` | Git blob |
| --- | --- |
| `tests/async/quest_accounting_prep/read_quest_continuation.cpp` | `bcf43cea2ac1dfdb256b1cceaa143cb4ec9e5338` |
| `tests/async/quest_accounting_prep/legacy_xp.py` | `57f4f3eacf0426fca0ac9db1529c8fcf43f8a5a3` |
| `tests/async/quest_accounting_prep/capture_quest_cut.py` | `8a079422a452028e4ff8c5e94f8031fa9b350358` |
| `tests/async/quest_accounting_prep/quest_cut_checks.py` | `20ec1fa587ee00cfee1a040236df1dff800cb4ad` |
| `tests/async/quest_accounting_prep/native_quest_retirement_checks.py` | `833b2a5ac9c543468ef46820331b2df3bbc45e26` |
| `tests/async/quest_accounting_prep/case_data.py` | `44c08a70031bde27ebb9a0996c2706e93a5cf8d0` |
| `docs/persistence/economy_accounting/quest-prep/RECOVERY_BATCH.md` | `f48781be2f671c9a5021699f29a75e049c53b7bd` |
| `docs/persistence/economy_accounting/quest-prep/RESULTS.md` | `553423a4f7e3c88ed2c64a114c1675ac5068b78c` |

Executed `git fetch origin experimental-accounting`, direct `git show`/`git grep`
source reads and:

```powershell
python D:\Dev\Temp\qp03-held-pair-reservation-20261008\verify_reservation.py
python D:\Dev\Temp\qp03-held-pair-reservation-20261008\check_doc.py
git diff --cached --check
```

The private verification script reads immutable public Git objects only. Final
PASS:49 public source/dependency entries (including all15 maintained component
compiler inputs),21 exact line anchors; no component/native execution. An initial
source-check attempt correctly refused an off-by-one init anchor717; direct Git
lookup corrected it to718, then the complete verification passed. No source
contract was weakened. Retained source-only manifest:
`D:\Dev\Temp\qp03-held-pair-reservation-20261008\source-proof.json`, SHA256
`db804a549fe73ebb453896c5f0b7c5dd97aa7a93eb981e0802f5f04ca4f02e7c`.
Verification script SHA256:
`32c6bb47b16279b653a752eaad152ed392f2254df894199e9e3efdd90726aa58`.
Neither artifact contains journal/player input bytes or is committed.
The final document check also verifies all36 declared blob entries,18 pinned
source links and absence of all3 proposed implementation paths at the parent;
its source-only receipt is retained beside the manifest as `doc-check.json`.

Import this sole document independently after the reviewed temporal bundle.
Select compatible existing prep helpers before any later adapter implementation;
do not import the older prep production tree. Coordinator review precedes that
maintained implementation. The concrete missing export and physical D capability
remain shared-owner prerequisites, not a reason to fabricate successful evidence.
