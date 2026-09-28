# Baseline genesis preparation

`economic_baseline_prepare` is the pure bounded compiler for a future quiesced
cutover owner. It consumes a single batch of already resolved account lifetimes
and complete item forests, and returns an immutable native witness plus an
existing canonical accounting plan. It does not collect native data, establish
quiescence, authorize an operator, persist a baseline, or activate a domain.

## Accounting genesis versus native state

Opening effects describe the initially empty accounting book: ordinary balances
start at zero and become the observed denomination vector, with accounting
revision 0 -> 1. These numbers are **not native mutation revisions**. Each exact
native revision (including zero and UINT64_MAX), balance and source fingerprint
remains in the canonical witness and its domain digest. A cutover owner must
compare them against the frozen native sources and leave those sources unchanged.
A gameplay currency writer must never execute the opening plan.

Each nonzero ordinary holding receives one posting and an exact negative posting
to the designated epoch opening-equity account. Legs are not netted, so their
individual values remain representable even when aggregate supply exceeds int64.
Zero holdings remain explicit ordinary effects with no zero-value postings. An
all-zero batch omits the unused equity effect; an empty batch remains a valid
empty preparation receipt. Opening-account identity is still bound in its intent.

Wallet, shared bank, physical pile, auction escrow, pending claim and treasury
keys are supported. Duplicate lifetime IDs (even across kind/context), malformed
keys, negative balances and copper-value overflow fail. The compiler does not
infer physical identity from a source fingerprint or silently deduplicate aliases;
the owner must prove that each native holding maps to exactly one lifetime and
that batches do not overlap.

Items retain exact UID, owner, root, parent, state and revision. Before and after
snapshots match and there are no item events, child commands or UID allocations.
Complete forests are required; missing ancestors, duplicate UIDs and invalid
custody fail. Active, quarantined and destroyed history remain distinct. Baseline
source metadata makes no claim about historical creation/provenance. Quarantine
is evidence for the future preflight decision, not permission to activate.

## Identity and fingerprints

Writer 4 is baseline batch preparation. Batch operation IDs derive from the stable
preparation ID with domain `0x42415345` and the exact uint64 batch index. The source
event explicitly retains preparation ID, epoch and batch index. Changing a batch
under the same ID changes its hashes; the future receipt owner must reject that
conflict rather than allocate a replacement ID. Per-account/epoch uniqueness must
also hold across different preparation IDs and batch partitions.

Canonical inputs sort holdings by full account key and items by UID. All integers
below are little-endian; digests are raw 32 bytes. SHA-256 tags include their NUL.

- Intent tag: `DURIS-ECONOMIC-BASELINE-INTENT-V1`. The 178-byte input contains u16
  version 1; lineage, epoch and preparation IDs (16 bytes each); u64 operator ID
  and batch index; 40-byte opening key; boundary and coverage digests; u32 holding
  and item counts.
- Domain tag: `DURIS-ECONOMIC-BASELINE-DOMAIN-V1`. Holding records are 112 bytes:
  key, four i64 denominations, u64 native revision and source digest. Item records
  are 82 bytes: u64 UID, u8 owner type, u64 owner ID/context/root/parent/revision,
  u8 custody state and source digest. The counts in the intent bind the split.

Boundary and coverage hashes must be nonzero but are not capabilities or proof
of global consistency. Source digests must bind both native source identity and
its content; the compiler cannot authenticate arbitrary caller-supplied DTOs.
A cutover owner must retain/reverify the complete manifests and witness in the same
authority transaction as the plan and progress receipt.

## Durable witness frame

`economic_baseline_encode` encodes only prepared witnesses. `economic_baseline_decode`
checks the entire bounded frame, regenerates the immutable plan, and requires exact
canonical re-encoding. Moved-from prepared values are not serializable. Both functions
preserve outputs on every error. This is a structural codec, not source authentication:
a changed but valid source fingerprint produces a different plan under the same batch
operation ID and must conflict with the original retained receipt.

The EAB1 frame uses little-endian integers, a 192-byte header, 112-byte holding rows
and 88-byte item rows. There is no native struct padding. The maximum size is 872,144
bytes, which exceeds the 384 KiB critical-command payload limit. A cutover transaction
must retain the full witness alongside its existing canonical plan and original
receipt; it must not truncate a forest or raise gameplay command limits to fit it.
No backend storage path for this frame is activated by the codec.

The preparation plan's intent/domain digests describe the baseline witness, not
an EAI1 command binding. Existing accounting record validation derives metadata
from the attached critical-command intent and requires exact plan agreement.
The baseline command adapter below binds the complete witness through a compact
reference and produces matching EAI1 plan metadata for the common store. The pure
preparation plan must not be inserted directly as an executable accounting record
or bypass that comparison.

| Header offset | Bytes | Field |
| --- | --- | --- |
| 0 | 4 | EAB1 magic |
| 4 | 2 | Version 1 |
| 6 | 2 | Header bytes, 192 |
| 8 | 4 | Exact total byte count |
| 12 | 4 | Reserved, zero |
| 16 / 32 / 48 | 16 each | Lineage / epoch / preparation ID |
| 64 / 72 | 8 each | Operator ID / batch index |
| 80 | 40 | Opening account key |
| 120 / 152 | 32 each | Boundary / coverage digest |
| 184 / 188 | 4 each | Holding / item count |

Holding rows use the 112-byte domain layout above. Item rows contain UID at byte 0,
owner type/state at bytes 8/9, six zero reserved bytes, then owner ID, owner context,
root UID, parent UID and native revision (five u64 values), then the 32-byte source
digest. This padded durable layout differs deliberately from the 82-byte domain-hash
input. Holdings must already be in full-key order and items in UID order. Decoder
count bounds and exact size checks occur before allocating or reading any row.

## Bounds and verification

One batch has at most 3,071 ordinary holdings (reserving one plan account for
equity), 6,142 postings and 6,000 item witnesses. Sorting/deduplication and existing
forest validation are bounded; failures preserve the prior output. No game-loop,
I/O, crash-recovery or whole-import latency qualification follows from this compiler.

`python3 tests/async/test_economic_baseline_adapter.py` runs both compilation modes
under ASan/UBSan. An independent Python encoder supplies intent/domain reference
hashes. Cases cover all six holding kinds, exact opposite postings, input-order
invariance, empty/zero holdings, maximum native revisions, unchanged active/
quarantined/destroyed item custody, nested forests, overflow, duplicated lifetime/
UID refusals, missing ancestors, changed boundary/coverage/source fingerprints,
plan roundtrip, maximum-size batches and every observed allocation failure. Codec
coverage adds independent Python reference bytes, every fixture truncation, header/
reserved/count/row mutations, noncanonical ordering, moved-from refusal, a maximum
combined holdings/item frame and allocation failure during both encode and decode.

Still required: native SQL/flatfile cutover owners; complete source collection and
preflight; a durable quiesced maintenance boundary; stable lifetime allocation; resumable unique
openings and progress; atomic receipts/witness storage; per-domain activation,
boot/writer compatibility, safe pause, and disposable crash/restart rehearsals.
Gameplay admission still refuses baseline commands.

## Durable command binding

`economic_baseline_command_build` creates accounting schema 2 command type 20
(`economic_baseline`), payload version 1, from an immutable prepared witness.
Existing command type numbers 1-19 are unchanged. The operation ID remains the
BASE-derived preparation/batch ID. The caller supplies a nonzero acceptance time
and must retain that time in the original receipt on retry.

The 48-byte EBC1 payload is little endian:

| Offset | Bytes | Meaning |
| --- | --- | --- |
| 0 | 4 | `EBC1` |
| 4 | 2 | version 1 |
| 6 | 2 | size 48 |
| 8 | 4 | exact complete EAB1 witness size |
| 12 | 4 | zero reserved |
| 16 | 32 | SHA-256 of the complete EAB1 witness, with no prefix |

The command uses operator-repair source, recovery deadline, no expected native
revision entries and the single system fence `0x45434f4e42415345`. This fence
serializes baseline work with itself; it does not freeze gameplay. Complete
native revisions remain in EAB1 and must be compared under the cutover boundary.
The EAI1 intent uses the prepared operation metadata and empty writer facts;
its ordinary command/domain hashes bind EBC1, which binds the complete witness.

`economic_baseline_command_plan` regenerates the exact command from the retained
witness, refuses any mismatch, then derives EAI1 plan metadata while preserving
all prepared opening effects and unchanged item snapshots. Common accounting
record validation accepts that bound plan and continues to refuse the pure
preparation plan with its custom witness hashes. Replay must load the original
witness, regenerate the bound plan, and compare the original receipt's plan and
command, including acceptance time. Looking up current state as a replacement
witness is forbidden. Receipt equality, not the binding digest alone, detects a
changed acceptance time (the established digest uses a sentinel time).

Structural freezing and binding now use envelope validation independently of
legacy execution support. Schema-1 projections remain the binding preimage so
existing command hashes do not change. Both schema-1 and schema-2 baseline
commands refuse legacy execution; accounting coordinator admission stays closed
until an implemented lifecycle owner can enforce all cutover invariants.

This is command/record integration, not durable baseline execution. The lifecycle
owner must still atomically retain EAB1, the bound plan and receipt, enforce
account/epoch and item coverage uniqueness, prove frozen sources and complete
domain coverage, and make activation progress restart-safe on both backends.

## Current incremental evidence

On the follow-up to PR #608, both SQL and client-free ASan/UBSan modes pass.
Each mode exercises 75 preparation, 1 encode, 78 decode, 10 command-build and
45 bound-plan allocation failures while preserving outputs. Independent reference
witness/payload bytes, maximum frames, malformed/truncated/canonical inputs,
complete item witnesses, zero/maximum native revisions and DURECR2 record
roundtrips pass. Real coordinator tests refuse fresh, publication-retaining and
durable baseline commands with no execution or checkpoint and retain the exact
journal command. These are refusal tests, not baseline persistence qualification.

Existing plan/intent reference tests pass in both modes with all 13 golden
fixtures unchanged. Existing bank admission tests in both modes, mixed-journal
refusal, 26 contract tests, source census and changed C++ formatting also pass.
Full server build evidence is recorded with this increment's PR. No native
source-capture, cutover, activation or player journey is qualified here.
