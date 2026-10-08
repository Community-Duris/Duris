# Native auction creator proof and zero-fee credit - 2026-10-07

Status: solved and verified on the maintained candidate. Both maintained production
builds, ten original contracts, focused sanitizer regression, writer contracts and
the unchanged original MySQL8/MariaDB10.11 SQL component pass. Whole Plan4,
physical gameplay, activation and release qualification remain open.

## Failure and fix

The original native BID184 buyout executes and records successfully, then retained
verification returns EILSEQ. Its creator lookup requires payload version1 although
the stored genuine native creator uses version2. The seller-credit check also
requires a closing-fee sink even when the original canonical producer charges zero.

The lookup reads and binds the actual version. Legacy version1 keeps its existing
validation. Version2 borrows the existing complete native creator/root/receipt,
canonical intent/plan/digest, indexed financial, source/outbox and original listing
forest proof. The helper uses the caller's existing SQL session and owns no
transaction lifecycle. Missing fee sink is accepted only when the authenticated
seller credit equals the entire final price. Positive-fee identity, amount, sign,
overflow and fee-plus-credit checks remain enforced. Flatfile returns ENOTSUP.

Three source files change: `economic_sql_auction_claim_endpoint.c`,
`economic_sql_auction_retained.c` and its header. Their private preimages equal the
maintained raw bytes at `65730c44362866a1f86bc268693014716fe21547`.
The new `tests/async/test_auction_retained_seller_fee.py` extracts the current
owning verifier subsection at execution and links the genuine producer and the
original C++ bid suite. It adds zero-fee and malformed-credit controls with the
original strict sanitizer flags and180-second compile/30-second execution limits.

## Evidence

Both private original753-provider production builds and nine contracts pass.
The unchanged original auction component passes on MySQL8 and MariaDB10.11:
native LIST181, BID182/183, BID184 buyout/apply/replays at revision12, and BID185
stale-save refusal followed by payout/apply/replays at revision13. Original source,
parent/descendant, multiroot, corruption, durable rejection and no-effects controls
remain. Execution times are12.46 and10.352 seconds respectively; no timeout.

The focused test establishes a failing original zero-fee predicate after the
existing positive-fee controls, then passes the fix, including malformed seller
amount, negative posting and wrong positive-fee sink refusals. Earlier real link
failures are retained and are not counted as the failing behavioral test.

Primary independently authenticates the complete terminal export:1,343 raw native
members with modes,205 sealed artifacts and the ordered engine/receipt markers.
Archive: `e9189378b89df211306d46800de2d4a26e0486abe6dbb3a7d962110a65aba264`.
Authentication: `e185caf839ba45284a6282dedbf96cf7b159e582c1b9eb41bf2c8348502be512`.
The application manifest and protected-WIP checks are in
`tmp/auction-maintained-creator-fix-primary-20261007/APPLIED.json`.
No resources were deleted and no production service or live game was used.

Writer refresh preserves all925 route policies/backend evidence and the exact
2,900-occurrence census. It only refreshes changed-source pins and derived matrix
anchors. Coverage remains false and release remains BLOCKED.

Both maintained original753-provider builds and full links pass: MariaDB26.87
seconds and flatfile18.48 seconds, each with five fresh units and748 authenticated
reused objects. All ten original contracts pass. The focused regression passes
on current maintained source under ASan/UBSan in59.18 seconds. Primary independently
authenticates6,422 frozen raw source bodies/modes, both1,508-member native caches,
original dependency/compile/link/toolchain identity, current production/migrations/
focused test and protected WIP. Build handoff:
`009662d05a9dac2d4501162f9a9b6d375466695f3b2cd76e91e9fcf6df8066ed`.
Evidence: `bin/tests/auction-maintained-native-build-primary-20261007/`.
All four current writer contract commands pass separately.

The unchanged original SQL component also passes on this exact maintained
composition: MySQL8 exit0/10.957 seconds and MariaDB10.11 exit0/10.042 seconds.
Each engine's full stdout equals the successful original, including all94 ordered
controls. Primary independently authenticates1,343 exported raw members/modes,
including the actual stopped disposable SQL datadirs and component object/ELF.
Archive: `fb94f225657797dbf17c269254ae85c50812e9c0773f5b26018d1a6adef75727`.
Authentication: `7ffea211e32b6745b152701152a2e249c600e4c2e9b9682c3f05903e0932991f`.
Handoff: `a88a0051ea6afa9b610ab362202fe66113a33913cd4dd2a2ec1b1de966a4eb84`.
Evidence: `bin/tests/auction-maintained-sql-component-primary-20261007/`.
`SOURCE-AND-FIXTURE-PROVENANCE.json` distinguishes the unchanged private corrected
driver/foundation/helper from21 maintained inputs and records their exact paths,
hashes and controller invocation. These database fixtures remain private local
evidence; the focused regression is the added maintained test. The first controller
mountpoint failure occurred before Python or SQL and remains retained separately.

## Remaining qualification

The solved issue is ready for its separate commit and normal milestone push.
The native SQL component
does not prove physical player publication, all compound-domain journeys, full
accounting completion or activation readiness. The separate private coin stream
passes its warm full-payload/ACK proof. An earlier full cold boot omitted the retained
coin UID because inert staging rejected the genuine zero-description literal;
the independent correction now restores the same UID and full payload on both
MariaDB cold scans. Its second normal shutdown exceeds the original30-second
limit, so full restart/MySQL qualification remains open while that failure is
diagnosed on the combined source. The fresh SHOP route is another independent
remaining Plan4 stream.
