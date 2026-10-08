# Native mobile retained cash image — 2026-10-05

The native NPC economic image retained identity, lifetime and ordered literal
stock but omitted the actual rolled wallet. `convertMob` draws all four money
denominations even with `apply_mob_gold=false`; reloading that NPC cannot prove
or restore its original cash. This source milestone retains the missing native
economic values without adding a finite NPC treasury or full NPC-stat ledger.

## Implemented source contract

An explicit v2 image carries the native four signed denominations in existing
copper/silver/gold/platinum order and a positive original cash revision. Each
amount must fit the existing nonnegative native int representation. Its 256-byte
overhead adds exactly 40 bytes to v1's 216-byte framing; existing total-size,
canonical item order, little-endian, length and SHA-256 checks apply. The separate
148-byte reference format remains version1 and byte-identical.

Historical v1 images decode/reencode with cash absent, preserving their exact
bytes. Absence means unknown, never zero. The original capture overload remains
compatible; the cash-aware overload requires an explicit retained revision from
the actual economic owner and records the native wallet after original conversion
and complete unfiltered stock capture. Refusal leaves caller output unchanged.

Both SQL and flat-file native participants validate the same pure cash transition
policy before writes: new birth is live with cash/mobile revision1; known cash
cannot disappear; unchanged denominations preserve cash revision and cannot
roll mobile revision backward; changed denominations advance both revisions
exactly once with overflow refusal. Retired cash must be zero. Historical unknown
cash cannot be silently adopted or rewritten through these ordinary participants.
This policy does not authenticate original reset/spawn source, custody, admission
or publication. The original typed transition owner still supplies those proofs.

Version2 writes require the corresponding upgraded native reader. No production
format activation, existing-record backfill, migration application or game-data
mutation occurs. Current inactive loader and accounting safety gates are unchanged.

## Source verification and remaining work

Independent persistence source review, exact raw current preimages, frozen six-file
candidate pins, changed-line clang-format18 fixed points, version/size/source
closure and complete unchanged source census are recorded in the private proposal
and installation receipts. All existing writer policies/backend evidence remain
unchanged. Inventory and source checks do not establish runtime qualification.

No compilation, tests, native SQL/flat execution, service, gameplay, persistence
or recovery run occurred. Those checks remain at original major-plan readiness,
including v1/v2 round trips/damage/limits, native revision refusal and both-backend
readback/retry, and original cash/stock restoration without reroll or duplicate
issuance. This is one original native economic-image prerequisite, not completion
of native birth, reset generation, lifecycle, restore, Plan3 or R1–R8.

The next native integration must supply the actual retained reset/spawn source
owner before RNG, bounded original birth/ordered-stock commands, explicit durable
reference recording/restoration and retirement/death cash consumption coupled to
the existing reward issuance. Current reset refusal remains until qualification.

In parallel, the coherent0057–0060 schema source chain is prepared privately;
current engine fingerprints remain explicitly unmeasured. Cold SHOP still needs
original replay registration, native rebind/publication and guarded ACK. Release
and activation remain BLOCKED.
