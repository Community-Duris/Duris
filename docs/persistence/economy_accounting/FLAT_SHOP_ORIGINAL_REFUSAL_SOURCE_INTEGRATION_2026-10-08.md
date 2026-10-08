# Original flat SHOP refusal: integrated source

2026-10-08. Private candidate `6641083bd523f9acebbd9618f5ef01816ef40fc2f12635a4b0f03242581d1ac5`
at `tmp/lifecycle-flat-original-refusal-plan5-candidate-primary-20261008/candidate`
composes the full live flat refusal with the retained coordinator cancellation
ordering repair on the [complete cold owner and driver](FULL_FLAT_COLD_DRIVER_SOURCE_INTEGRATION_2026-10-08.md).
It preserves all189 selected paths, including150 production files,23 original
fixtures,five schema inputs and11 audit/restore paths. Existing81 C providers and
Makefile registration remain unchanged.

## Failure and source fix

The original coordinator erased the operation and fences before calling a hold
consumer that can refuse. The reviewed successor retains the exact operation,
original refusal and fences until consumption actually succeeds. Callback and
consumption remain outside the coordinator mutex; the original operation and
in-flight guards protect the interval. Failure clears only temporary ACK state.
This change is restricted to SHOP cancellation, following the existing auction
ordering. Packet979cae67/sourcecaab280 passed independent source review.

The live client-free flat preparation also rejected genuine never-admitted
publication after an attempted native source checkpoint. Its separate old
cancellation callback could not prove full original produced cleanup; the
generic offline path could reach that weaker owner. Packetfd8d29/source431333a
now invokes the authentic retained flat publisher. It requires original
coordinator refusal authority, exact command/reservation/slot/source-root cut,
both genuine receipt absences, frozen complete trade BEFORE, full native money,
pet and world topology, and owner-wide current/foreign custody proof.

The ready native checkpoint remains refused-trade BEFORE; it is neither reset
nor reissued. All new retained allocations are charged once before transfer.
Only the actual uniquely detached complete produced tree may be extracted.
Started precedes extraction; normal return is retained before post-census.
Initially absent produced copies without that returned marker and started but
unreturned extraction stay closed. A fresh full census and source/custody cut
must pass again before the original coordinator may consume its hold.
Notification is a separate retained continuation: disappearance cannot erase it
or manufacture a returned notification. Unknown absent-player UID-zero pets
still refuse. Original execution, SQL, inactive and declined spell paths stay.

## Actual evidence and remaining gates

Independent review, formatting/token equivalence,213 refusal dependency hashes
and13 exact forward/inverse hunks pass. Coordinator source adds120 dependency
records; its nine exact hunks and raw outside-function preservation pass.
Root composition verifies every selected file and protected WIP hash and the
explicit compatible coordinator predecessor7370415-to-successorcaab280 upgrade.
Across the five source packets joined after the immutable-forest checkpoint,
580 dependency records are authenticated; this count is not unique dependencies.
The inherited12/1643 source slices, Plan5's44 Python dependencies,108 prior
central rows plus three offline rows and29 added methods retain their scopes.

No compiler,unit,native,SQL,gameplay,persistence,recovery or performance execution
ran. Source acceptance and composition do not prove a solved runtime issue or
full accounting completion. Maintained implementation and coverage/activation
gates are not promoted by this documentation milestone. Original budgets stay.
Major-plan qualification remains deferred until the relevant candidate is ready.
Shared keeper first-checkpoint policy and retained original boot/warm AFTER
provenance remain open,along with remaining producers,opening/activation and
combinedPlan5/R1-R8 qualification. Original Plan1 acceptance retains its recorded
historical scope; Plans2-4/fullrelease remain incomplete. The goal stays active.
