# Plan 5 integration handoff — 2026-10-04

Primary integrated `codex/accounting-plan5` through
`7a78bb065a7979b8dc8ad2ec49295629d6713906` in local merge `432db98be`.
Experimental branch through `f7d26eaa721cd3b675c0b0c65009a2535813f400`
was integrated in `a590fc662`; that incoming delta changes only game news.
No new combined-candidate tests ran. External reports and native input trees
remain branch-specific evidence, not qualification of the primary candidate.
The later Plan5 `fcdb1afd8` restore-coin slice is integrated in `8d1be035d`;
its native tree `d9a9610f3` evidence does not qualify the current combined source.
Latest remote readback shows Plan5 `57d4d99b2` and experimental `f7d26eaa7`.
Canonical capsule restore slice57d4d99b2 is cherry-picked as e5a458f70; its
independent EAI1/EAP1 decoder, SQL restore checks and native decision fixture
do not change primary-owned contracts or migrations. The peer report's
3,026 native decisions and both-engine evidence retain their original inputs;
no combined-candidate qualification ran. Native command-preimage binding and
baseline reconciliation parity remain open. Primary source through6ef8abb88
connects save residence, but keeps epoch ownership disabled pending native
journal/direct writers, reserved replay, lifecycle/census and critical ACK.
Marker-v2 and mandatory `.elr` independent consumers still belong to Plan5.

Independent read-only specialist/architect source reviews found these remaining
gates for the Plan 5 owner. Counterexamples below are predictions from source,
**not executed REDs**; follow the agreed major-plan testing cadence.

1. **Stake retirement provenance.** In `stake_snapshot(True)`, change only
   `account_origins[1]["retired_by"]` to funding `OP`. `audit_accounts` verifies
   committed/same-lineage identity, zero aggregate balance and absent native
   holding, but not the stake's terminal effect/revision/reason. Kind11's mapping
   exemption removes that other cross-check. Bind retirement to exact finite-round
   terminal authority or emit a specific unresolved-retirement exception. Durable
   stake/round authority remains separately required; do not qualify it from the
   synthetic clean fixture.
2. **Retained price metadata conflict.** In `price_snapshot()`, change only
   `native.lineage_realized_prices[0]["reason"]` from21 to22, leaving selected root
   reason21/price3000. The overlapping selected/history comparison checks ID,
   epoch and price only, while the operator view exposes both reasons. Compare
   reason and outcome/result consistently along with price; prepare a focused
   case expecting an exception. The comparison gap predates the new view and
   is now part of its expanded conflict contract.
3. **Empty initialized baseline loss.** The independent qualifier discovers
   books from surviving baseline files or successful receipts. Complete loss
   before the first successful receipt is invisible; revision-zero initialization
   operation/opening remain self-asserted. Plan1 owns the native authority-bound
   per-epoch initialization marker and atomic initialization/retention contract.
   Plan5 owns independent marker enumeration/cross-checks and release evidence.
   Agree a narrow versioned record and explicit pre-marker compatibility policy
   before reader implementation. Missing legacy markers must not be silently
   interpreted as never initialized.

Audited ordering matches native numeric account ordering; S48 grammar and the
33 source-kind restrictions match native accounting policy. Operator coverage
and epoch-bearing retained prices are additive views. The three incoming flatfile
qualifier headers match the inspected native formats and fail closed on malformed
retained structures. Structural consistency does not attest original source
balances/custody: witness digests remain assertions until separately qualified.

Primary retains Plans1–4/shared contracts, registry/matrix and activation ownership.
Plan5 should own fixes to its audit/read-only qualifier files and report immutable
commits plus scope/BEFORE/AFTER/fault witnesses for regular local integration.
Preserve SELECT-only controls, both SQL engines, original budgets, populated
upgrade/retention and full release gates. The snapshot-exception13 and
save-death-conflict4 actual-native matrix adapters remain outstanding. No
activation, production operation or full release readiness follows from this merge.


## Native marker interface now ready for independent consumers

Primary committed native marker-v2 storage in0aa0bceeb; see the exact
[versioned record and compatibility handoff](BASELINE_INITIALIZATION_MARKER_INTERFACE_V2.md).
Plan5 may implement independent catalog-v2 decoding/enumeration and book-loss/key/ID
cross-checks against that contract. Legacy_unknown must remain explicitly unqualified;
no inferred ID, missing-file-as-never or rewritten compatibility evidence.
Primary added source-only lifecycle staged composition in2b4591c21 and retains
native boundary/census, activation and legacy migration ownership.
The native marker is source-reviewed, not executed or qualified. Defer new reader
checks to the agreed major-plan batch, preserving actual BEFORE counterexamples.

## Immutable lifecycle receipt interface for independent consumers

Primary's source-only native installer now retains
`economic-evidence/lifecycle-<original-operation-id>.elr`; see
[the exact source preparation and limits](LIFECYCLE_RETAINED_RECEIPT_PREPARATION_2026-10-04.md).
It is mandatory evidence for new completed lifecycle installs, committed in the
same authenticated authority bundle as mappings, baseline/reservations and epoch
selection. Original-ID retry precedes selected-epoch rejection and native capture.
Native format source is local commit `012c32e24` on `codex/accounting-review-fixes`
in `.worktrees/accounting-review-fixes`, source-reviewed but unqualified/unpushed.
Use that exact source/contract for the local narrow interface handoff; do not treat
the published experimental branch as already containing these new prerequisites.
Historical retry never reselects the original epoch or reconstructs original mapping
metadata from current mapping rows. Old completed installations lacking this record
fail closed; no implicit compatibility migration is supplied.

The private v1 envelope is48 bytes: eight-byte `DURELR\0\0` magic, little-endian
version1/body length and SHA256 body digest. The body stores exact original request,
opening, resolved coverage, baseline identity/revision, lineage and epoch provenance,
ordered full original mapping snapshots, original native source descriptors, encoded
baseline command, canonical witness and command-bound plan. Bounds and exact field
order live in the native codec; Plan5 should implement an independent reader from
that contract, preserving malformed/count/name/blob/canonical refusal.

Native decode cross-binds original locator/PID/order, balances, revisions and source
fingerprints to the separately retained baseline witness and ordered coverage.
Original mapping revision/create/last metadata remains explicitly authoritative in
the immutable `.elr` frame; its checksum is not an independent authentication proof
against coordinated rewriting. Alias names are private and do not identify durable
economic accounts. Do not invent retention/governance policy or expose raw aliases.

Current Plan5 authority/baseline qualifiers ignore this filename family;
`scripts/validate_data_lifecycle.py` also lacks its participant. Plan5 owns mandatory
receipt discovery and independent baseline/epoch/source cross-checks, missing/corrupt
history refusal, backup/restore/export and erasure/retention consumers/evidence.
Primary owns native format/producer, shared registry and central registration.
Return narrow interface needs and immutable owned commits for regular integration.

The38 prepared lifecycle cases remain declarative and unexecuted. Source review
is accepted; no compiler/native/flatfile recovery or broad release result follows.
Keep major-plan testing cadence, original budgets and all R1–R8 gates. Native
boundary authority, complete holdings/items, activation and compatibility remain
primary prerequisites. New format evidence is not full Plan1 or Plan5 completion.

Primary follow-on source95d03d02f connects reserved fresh journal replay and
separate callback/checkpoint scopes, refusing enabled missing/corrupt namespaces.
Ownership stays disabled; complete native callers/census, serial lifecycle, actual
revisit/startup, critical ACK and major-plan qualification remain required. No
new compiler/tests/native/SQL/gameplay/services or qualified push occurred.

## Native SQL baseline claim integration

Peer `17b824618cb1ba6c20f8a2feebde8d65d0e559eb` is integrated in `ad504f5c0`.
[Its report](PLAN5_NATIVE_SQL_BASELINE_CLAIMS_SLICE_2026-10-04.md) records the
actual native owner fixture, exact-ID replay, both-engine independent claim
reconciliation and retained damage diagnostics at its consumed inputs. Fixture
holdings/items remain absent and the exported authority is explicitly partial;
this does not qualify native world capture, initialization/cutover or the combined
source. Command preimage binding, marker-v2/`.elr` consumers and full release
qualification remain Plan5 handoffs.

Primary registered `native_sql_baseline_claim_audit` and
`native_sql_baseline_origin_audit` in `tests/integration_manifest.json`, with the
peer's explicit opt-in environment and required native cases. Both component
owners run their MySQL/MariaDB pair internally, so each is registered once rather
than duplicated for every outer engine. Required-case/skip rejection remains in
the existing harness. Each new row uses 900 seconds; no existing budget changes.
The release host must provide the peer report's native compiler/client/daemon
prerequisites. This is registration only: no case, build, compiler, SQL service or
qualification was executed by primary; the major-plan test batch remains deferred.
