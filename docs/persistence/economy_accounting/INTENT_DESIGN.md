# Frozen intent and envelope integration contract

Status: codecs and rejection gates implemented for #476; typed writer facts,
capabilities and transactional execution remain incomplete. No gameplay
accounting is enabled. Preserve the implemented binding tag
`DURIS-ECONOMIC-COMMAND-V1` including its NUL delimiter.

## Envelope compatibility

Existing builders remain on `CRITICAL_COMMAND_SCHEMA_VERSION = 1`. Accounting schema 2 appends `accounting_intent` bytes to the command value.
Schema 1 requires an empty intent and retains exactly its current encoding.
Schema 2 keeps the existing header, keys, revisions and domain payload, followed
by a 4-byte little-endian intent length and at most 8,192 intent bytes. Preserve
384 KiB domain payload and 3,003 keys/revisions: maximum encoded size is
`52 + 3003*40 + 393216 + 4 + 8192 = 521584`, below 512 KiB. Unknown schemas,
empty schema-2 intent and noncanonical/extra bytes fail closed.

## Intent v1 layout

The codec uses a 256-byte header and at most 7,936 bytes of writer-specific typed facts.
Little-endian integers; all reserved bytes zero.

| Offset | Bytes | Field |
| --- | --- | --- |
| 0 | 4 | EAI1 magic |
| 4 | 2 | Version 1 |
| 6 | 2 | Header size 256 |
| 8 | 4 | Total size |
| 12 | 4 | Writer ID |
| 16 | 4 | Policy version |
| 20 | 4 | Compiler version |
| 24 | 2 | Reason |
| 26 | 1 | Actor kind |
| 27 | 1 | Source-present flag |
| 28 | 2 | Facts version |
| 30 | 2 | Reserved |
| 32 | 16 | Lineage |
| 48 | 16 | Epoch |
| 64 | 16 | Root operation ID |
| 80 | 16 | Original operation ID or zero |
| 96 | 8 | Actor ID |
| 104 | 4 | Facts size |
| 108 | 4 | Reserved |
| 112 | 48 | Source event or zeros |
| 160 | 32 | Immutable command-binding digest |
| 192 | 32 | Typed domain-payload digest |
| 224 | 32 | Reserved |
| 256 | Variable | Canonical typed facts |

Freeze from immutable admission decisions; resolve current balances and realized
revisions only under authority locks. Large item lists already in the domain
payload are referenced through the binding, not duplicated into intent. Writer
facts must retain source generation/slot, beneficiary, selected reward/cost,
original expense or other decisions not otherwise recoverable from the payload.
A writer number or balanced plan is not authorization. Typed adapters must prove
source entitlement and exact complete agreement with the mutation they execute.

The NUL-terminated tags `DURIS-ECONOMIC-DOMAIN-V1` and
`DURIS-ECONOMIC-INTENT-V1` separate domain-payload and intent hashes. The former
covers 2-byte command type, 2-byte payload version, 4-byte payload size and exact
payload bytes; the latter covers the complete canonical intent. The plan digest
already hashes its complete canonical bytes. The command-binding helper projects
schema 1 with acceptance timestamp 1; schema-2 binding explicitly clears its intent in that projection without
downgrading actual execution or receipt bytes. `economic_intent_verify_binding`
also requires the supplied decoded intent to encode exactly to the attached
command intent, preventing a caller from substituting another facts record.

## Execution gates and retained replay

`critical_command_envelope_valid` checks bounded wire structure for schemas 1/2.
It deliberately does not interpret writer-specific fact bytes. The economy
codec checks intent structure and command binding; typed adapters must still
validate and authorize the facts. `critical_command_valid` remains legacy-only
through `critical_command_legacy_execution_supported`. Thus current coordinator
admission, journal replay and repository validity checks reject schema 2.
All public command-bearing SQL mutation helpers also check the shared legacy
predicate before touching their connection or output arguments. The pooled root
entrypoint also rejects before client-library/thread initialization or pool
acquisition. This predicate
does not impose new timestamp requirements on existing compound child calls.
No path strips an extension to invoke a legacy writer.

When adding transactional accounting, replace these closed execution gates only
with typed adapters that verify and atomically retain accounting evidence. Audit
all direct entrypoints as well as top-level dispatch. The immutable command
codec alone is not support for economic execution.

Coordinator `worker_loop` checkpoints terminal failures as well as successes.
Unsupported durable schema-2 records therefore must stop replay initialization
while retaining the journal, rather than return terminal ENOTSUP. Reject fresh
unsupported requests before admission writes. When accounting execution is
available, do structural decoding/binding, then exact retained-receipt lookup,
then current typed policy/authority preparation only for a new operation.
Committed replay returns the original outcome/plan even after policy/state changes.

Relevant entrypoints: `critical_command_coordinator_submit`, `enqueue_replayed`,
`critical_command_repository_apply`, `critical_command_repository_reconcile`,
`flatfile_critical_command_repository_apply_selected`, and direct repository APIs.

Current executable coverage includes independent Python schema-1/schema-2 and
intent reference bytes/digests, full maximum-size envelopes, timestamp
invariance, changed attached intent identity, malformed lengths/reserved fields,
all reference truncations, direct SQL helper rejection, and unsupported durable
replay with no authority apply callback or checkpoint. Mixed journals in either
order retain exact file bytes, and legacy-only replay still completes. A replay
observer may restore temporary legacy restitution state before a later unsupported
record stops initialization; production startup failure aborts that state and
clears coordinator fences. This is not an authority apply callback. Both fresh
submission APIs reject unsupported envelopes before admission. Existing
coordinator/admission and journal-fault tests also pass. Same-ID attachment during supported accounting execution and backend
storage/recovery journeys remain pending transactional integration.
