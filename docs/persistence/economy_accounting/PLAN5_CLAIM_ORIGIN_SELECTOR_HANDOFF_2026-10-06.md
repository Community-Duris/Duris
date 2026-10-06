# Primary handoff: authenticate the opening-origin selector

The published 0062 contract distinguishes historical NULL
`economic_baseline_witness.claim_origin_version` from new version1 openings.
Plan 5's retained-allocation reader validates declared version1 slots/counts,
original account keys and amounts, including fully consumed origins. That
component check does not establish how the original root authenticates which
origin policy applies. Primary's new opening producer remains private; no defect
in that unpublished producer is asserted here.

## Exact fields and missing proof

The field needing an authoritative derivation is
`economic_baseline_witness.claim_origin_version`. Its schema permits NULL or1;
its value alone is outside the authenticated EAB1/EAB2 bytes. The existing
original command/root chain binds `canonical_witness` through its digest,
`critical_operation_inbox.command_hash`, the EAI1 command binding/domain digest,
and EAP1 intent/domain digests. The published witness grammar has no explicit
claim-origin-policy selector.

Please publish the exact retained selector used by the new producer: field
name, wire/capture version, byte layout, authenticated parent and derivation of
the SQL field. If the existing private candidate already retains it in its
versioned original command or frozen source-capture contract, identify those
exact fields/bytes; no additional representation is requested unnecessarily.
If no authenticated selector exists, the required logical field is
`claim_origin_version` in that original versioned contract, sealed by the
existing command/witness/root chain. Primary owns the format and compatibility
decision; Plan 5 will not independently change a shared wire or schema.

Invariant: SQL origin metadata must match the policy authenticated by the
original retained root. A new version1 root cannot acquire historical unknown
coverage by changing this SQL field to NULL. A historical NULL root cannot be
promoted by adding a flag or reconstructed source rows. Current native balances,
schema head, observation time and inferred producer inventory are not selectors.

The exact original PID proof also needs the new producer's published digest
contract. Relevant native EAB holding fields are `account`, `native_revision`
and `source_digest`, plus retained mapping `native_id` and source
`beneficiary_pid`, `amount`, `source_operation_id` and `source_slot`. Table
consistency binds the mapping/PID pair but does not itself seal the original PID.
The current independent decoder exposes `account` as `account_key` and
`native_revision` as `revision`. It validates but does not expose the retained
32-byte `source_digest` at holding offset80 (the holding is112 bytes).
The currently published source-capture contract for `auction_money_pickups`
is order `pid`, columns `pid,money,claim_revision`; its ESD1 definition and ESR1
row digest frame raw canonical cells. Existing lifecycle holdings retain the
source-row digest and native revision. Please identify whether the new claim
holding retains this exact digest preimage or a new versioned contribution,
with a native reference vector. That allows Plan 5 to recompute original PID /
amount / revision independently from frozen evidence, without consulting later
pickup values or importing mutation/provider implementations.

## Established interface observation

Two explicitly modeled historical EAB1 claim witnesses have identical original
intent, plan, witness and inbox command hash. With no source rows, NULL retains
unknown historical coverage; changing only the SQL field to1 correctly refuses
with `restore_economic_baseline_claim_origin_mismatch`. The observation is in
`tmp/plan5/claim-origin-selector-observation.json`, with these capsule SHA256s:

- EAI1: `0a9a7b3d6ae378685083203d0bd99d104157f1bf35dbc75b8ce6f7bb1e324722`;
- EAP1: `356a45de69173e90e7b4ab89e15d660de3d733bbd3a26ae52311631d4a8b1366`;
- EAB1: `dea3b7ce50879f5acfefc27979c3c918ff0c39de4d4fcfbd4819032a4c76cacf`;
- inbox command hash: `81b1414b566f1c438e02152ef1979995dc45fae07f3933753422aa6ebe507693`.

This demonstrates that the published metadata choice is not derived by the
current reader from those frozen bytes. It is a synthetic compatibility
observation, not a new-producer failure, complete capture or release result.

## Consumers and required tests

Consumers are the restore/canonical audit baseline path, independent origin
reader, snapshot exporter/reconciler and the primary's full opening/cold-recovery
and activation verifier. The authentic selector must be shared with these
consumers before complete new-opening qualification.

Required reference and corruption checks:

1. A genuine new opening with positive and zero claim holdings derives version1
   from its original authenticated contract; exact slots/counts remain bound.
2. Changing SQL1 to NULL and dropping origins from that same root refuses,
   including after every original amount has legitimately been consumed.
3. Historical NULL original bytes remain historical, with unknown coverage;
   flag/source insertion cannot reinterpret them as newly proven origins.
4. Changing mapping PID and beneficiary together, while retaining the original
   holding source digest, refuses. A legitimate source-row digest vector binds
   original PID, amount and native revision independently on both engines.
5. Original replay, lost reply and cold recovery preserve the selector and
   immutable origins while allowing authenticated whole/partial consumption.

No backfill, new acceptance scope or activation is requested. Plan 5's current
canonical reports keep source capture and release qualification false. This
handoff supplies the primary's local notebook curator packet and leaves the
independent retained-allocation work free to proceed.

## Independent snapshot follow-up

Owned snapshot fix `1047e8c48cb9214d1c3fd76984472201ca80a4fd` closes the separate
partial-consumption omission, with its [exact qualification](PLAN5_PARTIAL_CLAIM_SNAPSHOT_QUALIFICATION_2026-10-06.md)
on the expected remote `codex/accounting-plan5`. It adds no shared schema/wire
and does not change the selector/PID request above. Selected-epoch projections
remain bound to original EAP1; auxiliary historical capsules require the
separate canonical SQL audit under release quiescence.

Latest primary observed is `26d7b66b86e1a38d09430386be257a065fceacd5`. Its shared
integration progress records a private ESR1 claim PID/money/revision comparison
and leaves the independently usable published reference and generalized marker
downgrade proof open. This is primary-reported private progress, not an
independently qualified new producer result. The requested exact reference,
invariants, consumers and genuine tests remain pending; independent Plan5 work
and the primary's locally maintained notebook remain nonblocking.
