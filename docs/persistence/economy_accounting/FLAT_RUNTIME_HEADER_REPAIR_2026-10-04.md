# Flat runtime replay-ownership header repair

2026-10-04, source parent b96c85c176a4e19a75822b9b0edd5e0328d84d91.

Plan5's maintained flatfile build at frozen05b2f7b3e inputs failed in
flatfile_economic_runtime.c:56 because current_ownership_epoch was not declared
by the included player_save_execution_guard.h. Its refreshed collector TU passed
compilation. The exact failed build and diagnostic-only injected-header success
are retained in PLAN5_V3_SERVICE_BUILD_HANDOFF_2026-10-04.md; neither is a passing
combined server build or managed service journey.

The primary replaces only that include with player_save_replay_ownership.h.
This header already includes the lower execution guard and supplies the existing
mutex-protected observer. No shutdown predicate or observer behavior changes.
Enabled or poisoned nonzero epochs still prevent release, as do a foreign process,
an absent current-thread lifecycle guard and initialized/running/admission-worker/
append/shutdown-refused coordinator state. Only the original clean closed owner
can release its runtime projection.

Read-only source review confirms exact API visibility and unchanged predicates.
The source candidate SHA-256 is
0361c613879b7ea4bfd4c5dca5fe441a1ea77558753a87db88cdc72a2434e6b9.
The existing registry candidate pin and generated matrix are updated together;
no writer classification or executable proof is added. Source inventory retains
886 rows,2843 occurrences/2784 mapped unique sites and zero unmapped sites.
Census completeness and coverage remain false; release remains BLOCKED.
Both maintained backend builds and original ownership/lifecycle shutdown checks,
followed by Plan5's managed v3 two-boot journey, remain at major-plan readiness.
No primary native compilation/test/service execution is claimed by this repair.

## Existing SQL baseline receipt handoff

Independent source review confirms EAB1 and SQL witness/root storage lack the
original accepted_at_usec. Inbox/lifecycle hashes are one-way; completed generic
WAL frames are removed and production baseline installation need not append them.
Reuse existing economic_baseline_witness with a nullable-for-historical
command_accepted_at_usec uint64 field; new native writes retain the actual nonzero
admitted value atomically with witness/root/openings/source/inbox. Exact replay
compares retained present values and never rewrites them. Historical NULL remains
explicit missing full-preimage evidence; no SQL timestamp backfill or fabricated
time is permitted. Known zero/mismatch refuses. Native incoming-command receipt
checking remains compatible with old unknown rows without promoting audit proof.

Primary owns native transaction and additive migration after pending0057, with
fresh schema, guarded retry, real both-engine metadata fingerprints and existing
compatibility/lifecycle registration kept coherent. Plan5 owns independent full
376-byte CCM1 command_hash and known nine-byte keys_hash verification, selected/
retained damage cuts and cold dump/import. Flat ELR already retains the command
and admission time and needs no equivalent field. This closes the existing
baseline receipt requirement; it adds no store or release gate. Implementation
and combined qualification of this timestamp handoff remain unfinished.

Fresh-bootstrap ordering must retain the historical0032 witness shape and add
this field through its new migration. Pre-adding it before immutable0032 runs
would invalidate that historical exact metadata check. Current-schema mutation
oracles must use the new current verifier, while original0032 checks remain
historical. No immutable predecessor, runner bypass or fabricated history is
permitted. This ordering is source-reviewed; engine execution remains pending.
