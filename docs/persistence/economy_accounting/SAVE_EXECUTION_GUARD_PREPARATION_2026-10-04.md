# Ordinary save execution guard preparation — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. Production cold ordinary
SQL-drop restoration remains unwired. No compiler/native/SQL/gameplay/recovery
checks or milestone push follow from this source preparation.

## Established missing ownership and implemented boundary

Before this slice, the pipeline restored hold affected save admission only.
Already admitted workers, direct ordinary repository calls, journal replay and
independent ordinary/exact checkpoints could apply or retire that PID's frames.
A callback-only guard would end before the actual checkpoint and leave a race.

The private header-only leaf owner `player_save_execution_guard.h` now supplies
move-only independent counted permits. Registration begins only with all earlier
execution owners drained; it remains restricted to the prepared/no-execution
phase. The pipeline owns exact original encoded-command equality, PID/operation
and the matching guard generation. Identical registration preserves generation;
conflicts/capacity/execution refuse. Malformed internal identity poisons admission.
The generation is an execution-owner identity, not a fabricated native revision.

Worker permits span callback, actual journal ACK, terminal/failure handling and
completion staging. Held jobs take the existing exact-request parking path with
no callback, ACK, failure retry or completion. Permit allocation/capacity refusal
uses retryable ENOMEM rather than a false held state with no future release wake.
Replay allocates token storage and admits every distinct PID before its first
callback, retaining those permits through both aggregate checkpoint exits.
Held PIDs contribute no callback/proof; original same-PID ordinary/exact proof
withdrawal and unrelated-PID checkpoint behavior remain intact by source review.
Independent ordinary/exact checkpoints and valid-PID appends also require permits.
Invalid append PID inputs retain original validation/result/counter precedence.

Direct ordinary SQL apply is guarded before native execution. The pooled outer
owner starts before borrowing and lasts through ambiguous-COMMIT replacement,
readback, disposal and release. Direct borrowed apply alone ends on return:
retire_required cleanup requires a caller's separate outer permit through actual
retirement before claiming that stronger boundary. Recovery/death-conflict outer
mutators and other direct writers remain outside this slice. A held PID gets
hold refusal before malformed snapshot/null-session validation; no SQL or durable
revision is claimed. Ordinary unheld validation behavior remains unchanged.

The closed ordinary phase counts permits without allocation. Prepared registration
maps are bounded independently of the journal's new-write admission limit; the
reader still scans all legacy bytes. Shutdown discards resident holds only after
all owners drain. Failed drain sets sticky shutdown_incomplete, so preparation
cannot reopen against another journal merely when the token later disappears;
an explicit successful shutdown retry is required. Generations never reset.

## Source preparation and remaining gates

Actual BEFORE base: `432db98be5a1e956503c4cee71a5223a68aa49f3`.
Ignored selected-source BEFORE manifest:
`tmp/save-execution-guard-before-v1.local/manifest.json`,
SHA-256 `9e62ac79c6c728a666e0161b95d2b0bc7d5467b92b6c9cb4614b4ac55c6ac74a`.
Private native-owner preparation: `tmp/save-execution-guard-prepared-v1/`.
Prepare actual worker/journal and actual pipeline families separately; a controlled
apply callback is not native SQL proof. Missing new headers/APIs on BEFORE are
unsupported cases, never semantic REDs. Original maintained oracles/budgets remain.

Replay execution ownership is staged before SQL, **not the entire post-SQL path**:
proof accumulation, checkpoint token vectors and typed ACK encoding still allocate.
Refusal before journal retirement retains frames; qualify those faults explicitly.
Source reviewers covered leaf locking/count/moves, worker/replay/checkpoint lifetime,
sticky shutdown, ordinary SQL cleanup/pool ordering and preserved proof semantics.
Primary corrected reviewed invalid-append and shutdown lifecycle gaps. Formatting
and diff hygiene are the only executed checks.

This does not establish a clean mutation census, exceptional frame reconciliation,
critical-publication ACK reservation, reliable deferred wake, actor-independent
native room publication, hydration safety, all native mutation exclusion or the
production prepare→restore→start sequence. The existing acknowledged callback is
not a census authorization. Keep those callers disabled until the complete
ownership chain and major-plan qualification pass. Historical restored-hydration
component evidence is not qualification of this newer restricted phase contract;
its future actual owner must register before start and retain every original oracle.
R1–R8, current writer reanchoring, coverage_complete=False and release=BLOCKED remain.

## Prepared source input pins

- `src/player/player_save_execution_guard.h`: `85d8d47d6e8090e425784772d862a97a0108f5b9522a7242c33394c37b9ec62f`.
- `src/player/player_save_pipeline.c`: `ccc654e00046957a5b3c1e4f6a11214084561460eb107dd08a0e2eb5ff8c2c4a`.
- `src/player/player_save_pipeline.h`: `a513ba80d9465463ef7118be8987400d9032c800ca2320e31860f4a75bbb0f2f`.
- `src/player/player_save_worker.c`: `caaa0d2fe0b85b1b14d0a14845823fe8792f0443d5c9540a7d4cf753f9669dc9`.
- `src/player/player_save_journal.c`: `a73adc2749281342c7c87fd576c89eccea5d07223f4dc2956287ef77d95d8020`.
- `src/player/player_save_journal.h`: `23b4e2397bfbe35b2845b5ab6bb5ea68729c0dca3aa7f10c02c3e78f57c77a85`.
- `src/player/player_snapshot_repository.c`: `273e7364edce38876c6109844decc3cbc1a5a6f73e0cafb2f89b199550a7fbbc`.
- `src/player/player_snapshot_repository.h`: `38d00c1cac85e5f4109e606282a4f0d31e30575d56177463a8d69ec82f2d1aa1`.


## Final private preparation receipt (unexecuted)

`tmp/save-execution-guard-prepared-v1-final.local.json` raw SHA-256:
`1dc308447ecef824903638148d288cba38932becc9de3435bf46aa415f11ac7a`;
LF-normalized `c6d63065de04c74ce6b17ce649f0c2e1bdb66a753fa53eed543f4e3644c26719`.
Immutable source archives are BEFORE432db98be and AFTER80692d52b, not the later
combined candidate. Leaf35 cases pin534 AFTER inputs and retain300-second compile/
120-second runtime bounds; pipeline14 cases pin555 AFTER inputs and retain600/120.
Existing19 leaf and9 pipeline baseline cases are preserved. Link/interposer/fault
calibration and timings remain unmeasured. Controlled apply is not SQL proof;
manual restored registration is not production critical-ACK/census/wake recovery.
Missing new BEFORE APIs are unsupported exit78, not RED or pass. No execution ran.
