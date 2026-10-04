# Critical publication cleanup preparation — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. No compiler, test,
AST, native/SQL/gameplay/recovery run or milestone push accompanies this slice.

## Contract and scope

Publication ACK durably retires the original critical journal frame before it
removes the operation's fences and retains an optional bounded completion cache.
Those final cleanup steps must not depend on temporary string allocation or let
optional cache allocation escape after the original frame is gone.

The previous nine-byte entity keys normally use libstdc++ small-string storage,
and `critical_command_encode` already catches its allocation failures. Therefore
this is an explicit cleanup/portability guarantee, **not a demonstrated native
failing-before/passing-after reproduction** on the supported compiler.

`src/persistence/critical_command_coordinator.c` now uses transparent string-view
lookup with an explicit nine-byte stack key, preserving embedded NUL/high bytes,
in fence and active-key cleanup. ACK identity allocation fails before marking
checkpoint-in-flight or invoking the journal. The whole optional cache function
is noexcept and contains encoding/cache admission failures; FIFO insertion failure
undoes only its new map entry before byte charge. Earlier bounded cache evictions
may remain. Cache visibility follows successful durable checkpoint. Failed
checkpoint retains the original operation, receipt and fences for retry. Lifecycle,
wire/public APIs and terminal native execution behavior are preserved by source
review; they await runtime qualification.

## Preparation and evidence limits

Final source SHA-256: `a51194beec1e397aed8355f3f45d9f111b45740acdab8b701667a99d0b46ee40`.
Independent architect review covered candidate `64249ddef749d0714912131fe7a86f2cb172ff853f19235a8736ab870ef51280`;
the only later change clarified a comment about the publication ACK path.

Ignored private owner: `tmp/critical-publication-cleanup-prepared-v1/`.
Original BEFORE manifest: `afd816a53556f4f7fb73ff52e225e1626f5dce7008bbcb01c8542f4c3a79523f`.
Full consumed BEFORE closure receipt: `5b49608600bcf55a65d266218d11881cc3b5c5efdf952574a425125226356c6e`.
Final candidate manifest: `bb993a34143ee79e757c70351e63096132d18575d1e3180581ba2cac84abad60`.
Harness: `87c85e70e1fbb71adfeed467d875e1ab12466cfb939cdff20f2b7eb1cd6e1351`.

Seven unexecuted groups cover preidentity allocation, postcheckpoint 64-ordinal
allocation sweep, failed-checkpoint retry, concurrent ACK/read/cutover, shutdown
overlap, cache eviction bounds, and binary entity IDs with overlapping successor
fences/foreign active-key preservation. Postcheckpoint instrumentation must observe
actual command-copy and map/FIFO fault witnesses; a sweep with no witnessed fault
is not failure containment evidence. Suggested 300-second compile/30-second case
limits are unmeasured private proposals. Existing maintained limits remain intact.
Only formatting and `git diff --check` ran. Qualification waits for the major-plan
batch and must retain exact native/frame/receipt/lifecycle gates.
