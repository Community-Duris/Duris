# PR #717 final stability and persistence review

Reviewed against PR head `4a42e64fd65e6644a957db5737484c0e1d51887e`,
`experimental-accounting` at `d8c5ed764c30690232088f45898e0a88a0bdb9f6`, and
`luminarimud/duris:master` at `a32d6a4f30e4bb12c22884981eda3f2d48dd5de8`.
The comparison used unmatched source history plus inspection of the current
implementation; matching commit subjects alone were not treated as evidence
that a fix was missing. The scope is stability and persistence, without enabling
accounting or importing gameplay features.

## Fixes applied

| Failure | Change | Regression evidence |
| --- | --- | --- |
| A coin conversion commits after its player's descriptor detaches, updating the wallet but failing to attach the physical pile. | Resolve the retained player body for physical publication using the same lookup as wallet publication. | Real descriptor lookup, linkdead and pending-death bodies, custody, balances, and duplicate receipts. |
| Allocation failure during journal decoding is mistaken for corrupt bytes; compaction then removes a valid save. | Return a retryable journal I/O failure without quarantining or compacting the record. Compaction path allocation also returns through the journal error API. | Allocation sweep through scanning/compaction; journal bytes remain identical and no quarantine is created; normal replay still succeeds. |
| The writer allocates when taking its next job, and allocates again when delivering a completion after journal ACK. The first can terminate the process; the second can silently lose the receipt. | Move the job into inline optional storage and deliver completions through fixed storage bounded by the existing admission limit. | Deny writer allocations after every ACK, fill all slots, verify backpressure, drain and wrap the completion queue, and check exact receipt order. |
| A failed stream allocation returns truncated quest state as a successful serialization. | Refuse bad stream state; also refuse quest mutations if their rollback snapshot could not be captured. | Sweep serializer allocations and verify that a refused rollback capture cannot mutate completion state. |
| A temporary database qualification outage permanently disables telemetry after the retry limit. | Keep retrying qualification with capped backoff. Permanent failures and write-batch retry limits retain their existing behavior. | Twenty failed qualifications followed by recovery, no early attempts, plus existing permanent-failure and batch-limit tests. |
| Critical-command completion lookup allocates after removing the receipt from its queue. | Prepare identity before popping; allocation refusal leaves the receipt queued. Publication ACK refusal preserves the pending operation and its fences. | Inject failures at pulse and ACK lookup; retry the same receipt and preserve publication fences. |
| A committed shop trade loses its pending record when a live projection or physical publication fails. | Retain the original receipt, completed projection stages, and coordinator fences until physical publication and notification finish. Retry safe refusals once per pulse; retain uncertain native effects without repeating them. | Projection and physical fault cases in both compile modes, duplicate receipts, changed receipts, reentry, fresh player bodies, repeated refusals, and an ACK retry without repeating notification. |

Relevant comparison commits include
[retained death-coin publication](https://github.com/Community-Duris/Duris/commit/2b9549ef29ed9ca8200b4aa3ddb27cb260505711),
[allocation-free worker completion](https://github.com/Community-Duris/Duris/commit/304e868a1e270035b3fdc05f66ca34944c0bdad9),
[quest serialization refusal](https://github.com/Community-Duris/Duris/commit/889e1122b0134bc2d82c9688ee2405a9cab149b4),
[critical publication cleanup](https://github.com/Community-Duris/Duris/commit/a66b82b7144b150abe115fb2974d96e9d8e01e21),
[shop publication retention](https://github.com/Community-Duris/Duris/commit/39e665ce936c5ae31d5e875589854b6c6d435ec4), and
[qualification retry](https://github.com/LuminariMUD/Duris/commit/489c17d071dc089e7e08344ba30a0007005c3d8d).
These were adapted to the current code. Accounting admission dispositions,
keeper-cash wire extensions, and accounting providers were not imported.

## Persistence contract assessment

**Adopt bounded-rate recovery for read-only qualification.** A retry limit is
useful when writes might have committed, or when a permanent schema/configuration
failure needs intervention. It is not a useful lifetime limit on reconnecting and
checking ownership/schema after a temporary outage. Qualification may continue
with capped backoff while queue capacity stays bounded and gameplay stays
independent. This narrow policy change is included here.

**Keep durable authority and publication fences.** The single writer already
provides ordered execution. Replacing durable item/wallet decisions with live RAM
and later snapshots, removing recovery journals, or treating logout queue admission
as completed persistence would create a larger crash-loss window. Those changes
are not improvements for this project's stated durability contract. No such
change is included.

**Consider canonical artifact identity and repair of derived ownership metadata
as a separate migration.** The useful part of
[artifact identity/recovery work](https://github.com/LuminariMUD/Duris/commit/53d0da2f76526b136867df98d8c2fbe02dab3176)
and [owner reconciliation](https://github.com/LuminariMUD/Duris/commit/2c70abf1728a5930f345ee5f731ccbbb44c238a9)
is distinguishing the canonical artifact UID from its prototype and repairing
stale artifact location metadata from validated item custody. Recovery should
preserve an existing expiry timer and should not rerun acquisition or soul-switch
side effects.

A sensible contract extension would let validated durable custody repair a
*derived* artifact-owner projection, using an explicit canonical UID, owner
revision checks, and one guarded transaction. It should not permit an arbitrary
live object to claim ownership. This needs a canonical-identity migration,
legacy duplicate reconciliation, and restart tests across inventory, corpses,
lockers, and world recovery. The upstream implementation is intertwined with its
RAM authority changes, so copying it into this patch would not preserve those
conditions. It is a useful follow-up design, not a completed port.

**Keep recovery leases fenced.** Letting a Redis world writer republish after an
expired lease without proving the current owner/epoch can revive a stale writer.
Automatic recovery is desirable, but should reacquire authority and validate its
fence before publishing; the missing-fence shortcut was not imported.

Other reviewed candidates were already covered or did not apply to this branch:
failed SQL replacement leases, shutdown extraction, account guild/ship cleanup
only after committed deletion, and ignoring untracked legacy corpse cash when
counting discarded custody. Luminari's offline-loader change targets a helper
that is absent here. Its raw corpse-save calls accompany its different item
movement authority and are not needed in place of our transactional item moves.

## Validation

- All 40 selected regression scripts passed after updating four stale test
  assertions/fixtures: transaction-owned wallet exclusions, the death diagnostic
  adapter, extracted entry-room routing, and additive schema inventory growth.
  The selection covers currency, journal, worker, snapshots, critical publication,
  death/terminal saves, shop transactions, quest state, telemetry, lockers,
  auctions, and the economy outbox.
- All 74 shop owner/physical fault scenarios passed with AddressSanitizer and
  UndefinedBehaviorSanitizer, across SQL-header and flat-file compile modes.
  Telemetry transport also passed its ThreadSanitizer run.
- Fault-injection regressions reproduced failures on the original code for
  retained-body currency publication, journal decoding, worker delivery, quest
  serialization, critical receipt lookup, and telemetry qualification retry.
- A fresh full `make -C src -j4 BIN_ROOT=/build/bin` passed with GCC 13, C++20,
  fortification, and warnings as errors. The final formatted source was rebuilt
  successfully. All 15 touched C/C++ files passed `scripts/format.sh --check
  --file ...`; `git diff --check` was clean.

## Validation boundaries

Focused executable tests exercise the production worker, codecs, journal,
coordinator, transaction publication, and native shop callbacks with controlled
external effects. Shop tests distinguish a safe retry from an interrupted native
handler whose remaining effects cannot be proven; the latter stays fenced for
recovery instead of reporting delivery or applying effects twice.

The tests do not constitute a live-server burn-in or a new database/restart
qualification of the complete artifact or accounting systems. No production
operations, schema changes, or accounting activation are part of this review.
