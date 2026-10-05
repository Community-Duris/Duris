# Native shop checkpoint and admission handoff

The later [the full-player native proof checkpoint](SHOP_FULL_PLAYER_NATIVE_PROOF_2026-10-04.md) supplies the reviewed full-player/native
receipt reader foundation. Actual publication/ACK and qualification remain
unfinished; source pins below retain this earlier admission checkpoint.

2026-10-04. Primary owns this implementation. **Private source integration;
not a completed gameplay route or native qualification.** Published source is
`103fd07c384ac84ba104cbbc0d90167f6b57db5f` after the collector SQL compiler repair.
The private shop inputs below have not been applied to the branch.

## Implemented and independently source-reviewed

The original pending trade now owns selection, keeper forest, actor body,
account/epoch mappings and a held whole-player inventory/level checkpoint.
The native keeper transaction seals actual before/after facts before its first
write. Lost COMMIT replies retain those facts for complete original-image
readback; a readback transaction's rollback never proves the earlier commit.
A fresh unsealed attempt can reset only after confirmed original-session
retirement establishes that no mutation could have occurred.

First keeper checkpoint durability is explicit in a nullable revision on the
existing keeper row. Missing full payloads cannot masquerade as legacy rows once
that checkpoint exists. Source review found an ordinary keeper save could add a
root without payload while retaining the marker. The corrected writer captures
and writes the complete current forest, requires actual same-keeper custody,
rejects foreign physical copies and missing existing payloads, and verifies the
complete resulting image before commit. It does not adopt missing custody.

The v6 producer freezes the actual player save/level and native keeper revision,
wallet/bank cut, epoch/mappings, operation and acceptance time once. Guarded
admission converts its same original player slot to exact command/execution
ownership. No second queue or authority is introduced. Definite synchronous
refusal consumes only that exact execution generation; uncertainty retains it.
Asynchronous never-admitted refusal uses retained exact coordinator evidence.
Physical success callbacks cannot fabricate the private pipeline ACK proof.

Source review also caught allocating intent classification in coordinator refusal
delivery: allocation failure could erase an operation while its player hold
survived. Retention/cache decisions now use allocation-free conservative v6
command classification. Full decoding stays in private cancellation, where a
failure preserves ownership.

| Private source | SHA-256 |
| --- | --- |
| Original domain C | `4ba98a48b01d2d942f0accc8ef08d808a51842c2168e10f82a2f3896b7c5d6f2` |
| Original domain header | `f9216bd4b31bc8c0fb8e338b35e3dd5e55ce93f694de0f9e8cab4d84c2b216ea` |
| Player checkpoint/admission C | `57dfa244b11fd5041b9433f37206881d7e146749db6072931a024948b218a0a7` |
| Player checkpoint/admission header | `8106618076bb721111a6172a4893356c5cfa76a478b1622ea8ad8b15423268e2` |
| Core codec C | `bb9ec52294d4e788e6ad8d9c4ee28760254a8f7a270c136b40a6642436befbd2` |
| Guarded coordinator C | `b41b522764c30c939d8f10c39859bdc0b13a22ba5881b078b9a965e120ed90b1` |
| Guarded coordinator header | `41e40357a553ed010658354098e5064c8915e5a671bc837edb54616c4366a05e` |
| Native keeper SQL C | `6b54581d8b92acc5430f9413e42cc1d27b8d6137ec9044e273396abe9d2bc756` |
| Keeper literal helper C | `a76c4916fa60dca2ddbbe1ebe2d5f698f0e101583123d5f3bfbe7dce080d5740` |
| Keeper literal helper header | `9d6afdf441f4f575f4ecf70e15fb319de56dde4990e1790fbcc3a7da2513978d` |

Exact before copies, receipts and deltas remain in the existing private proposal
directories. Changed-line clang-format18, whitespace and source-pin checks pass.
Independent database review accepted the frozen inputs within these scopes.
No compiler, test, migration, database, service or gameplay execution qualifies
this private shop integration.

## Narrow Plan 5 schema handoff

The private additive 0057 adds
`shopkeepers.runtime_payload_checkpoint_revision BIGINT UNSIGNED NULL DEFAULT NULL`
at ordinal 10 after `updated_at`. Named CHECK
`chk_shopkeeper_runtime_checkpoint_revision` permits NULL or a positive revision
no greater than `shop_revision`. The native full-forest checkpoint updates marker
and shop revision in its same transaction. Ordinary saves preserve completeness;
NULL retains legacy behavior. This marker grants neither custody nor trade authority.

Private 0058 adds
`economic_baseline_witness.command_accepted_at_usec BIGINT UNSIGNED NULL DEFAULT NULL`
at ordinal 10 after `canonical_witness`. NULL retains absent historical evidence;
new native witnesses retain their actual positive original admission time.
No backfill or fabricated acceptance time is permitted. Independent full-command
authentication must consume that original evidence after coherent integration.

All published histories through 0056 and bootstrap definitions are preserved.
Both proposals bind the revised exact 0057 SQL/verifier in all three histories.
Engine fingerprints are still unmeasured: old 0056 fingerprints are historical,
and cannot be promoted to 0057/0058 compatibility evidence. The branch remains
at its published schema until the coherent candidate's original major-plan batch.
Plan 5 may continue all unaffected 0056 audit/restore work in the meantime.

## Remaining implementation

Successful native current-state/receipt and physical publication, original replay
registration, actorless refusal delivery, complete keeper/UID writer fences,
lifecycle closure and gameplay callbacks are still required. An implementation
agent is working on the narrow read-only native publication observers; no result
is claimed here. Submitted successful preparations retain their original holds
instead of falling through legacy flat-cache publication.

Coherent migration measurements, maintained builds, actual player trades and
persistence/recovery checks remain in the requested major-plan qualification.
Other Plans 2–4 writers, activation-owner integration and full R1–R8 acceptance
remain open. Inactive behavior, safety gates and the declined spell path remain
unchanged; no production accounting, data change or deployment is authorized.
