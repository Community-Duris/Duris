# Native auction retained proof: current private progress

The private native-v2 historical/current-cut integration now passes the original
LIST, settlement, partial item-pickup and rejected-receipt replay assertions.
The next original legacy LIST control still returns retryable EAGAIN. This is
partial component progress; the source is not integrated into maintained
production files or qualified for release.

## Qualified build and measured cases

Seven auction repository/retained/bid/money-claim/settlement inputs change over
the previously authenticated full producer source. Native-v2 non-item commands
retain their original executor and authenticated source cut; v1 execution and
root-only settlement events preserve their owners. Before/after membership and
outside parent references participate in globally ordered UID locking. No
declined spell-path change, inactive behavior or safety gate is altered.

Full source archive:
`d2be1e34f35c60301e212acc18fbf2e37f173b6fae9360a2a0c4e84679b4ba42`.
Manifest: `55d06a6ce0f7fcc865d87dd6ca9d25a32ac9947540ffe320272ae90bd16d4234`.
Both original strict 753-provider production builds and nine contracts pass.
Primary authenticates all 6,367 source members, 23 artifacts and both complete
1,508-member object/dependency caches. Build handoff:
`5a0cee4aff12054ed2032d2c9d14a1b5b8cdc7845aa75ef9042104cbd3d2c574`.

The original full-tree SQL component then observes:

- LIST151, expired FINALIZE152 and partial PICKUP153/154 apply with outcome 0,
  error 0 and 320 result bytes. Their original receipt/history checks pass.
- Early FINALIZE155 is durably rejected with outcome 4, EAGAIN11 and 320 result
  bytes; no-effect assertions pass.
- Exact apply and reconcile replay of that rejection both return outcome 4,
  error11, stage0, revision0 and 320 bytes. Complete result/history equality
  assertions pass.
- The next original v1 LIST171 returns outcome2, error11 and no result bytes;
  the original fresh-apply assertion fails. MariaDB component cases remain
  unrun under the original MySQL-first fail-fast ordering.

The replay helper originally required success replay for every receipt. The
existing repository contract and separate original SQL fixtures require durable
rejected receipts to replay as terminal failure. A source-reviewed helper repair
selects that exact outcome from the original receipt's error, preserving all
byte/history comparisons and adding failure-stage equality. It accepts neither
outcome interchangeably. No production replay behavior is changed by this helper.

The subsequent original LIST171 diagnostic observes schema2/payload1, valid
envelope/frozen classification and successful typed decode. Item, wallet and
bank expected/current revisions match (6/6, 9/9, 10/10). A retained custody
row is present and claimed. The actual apply still returns retryable error11
with zero result bytes; connection mysql_errno observed afterward is zero.
This excludes the earlier false-classification guard, but does not identify
the internal failing predicate. Source review finds a historical unique UID
and unconditional listing insert. Rollback can clear the internal SQL error;
diagnostic SELECTs can perturb last-error state and timing. No SQL1062, index
change, historical-row deletion, new UID or authority repair is claimed.
Primary authenticates its terminal seal
`9271e89347c8bcee02482057066fcbc0eab5bc8414cd948a749da444c660b6de`,
all 97 artifacts and 542 native members. Exact cleanup/absence passes with
receipt `8407f96b0d1e1eeff31f96b9fc5fe5adab948d6f63ff42f2f6f0bc965a695ebc`.

## Evidence and limits

Private runtime packet:
`tmp/auction-native-nonitem-replay-oracle-launch-primary-20261007`.
Terminal seal `7b22ba6549a4d83e89c50793c3f20a94d9998fac4b5fe3001823cdacb48f833e`;
native archive `ee7b607193285a9d79db50d53bb9dbfdd41d89e1a43ee28709a55ab4524dc175`.
Primary authenticates all 89 evidence files and 542 native archive members,
including raw hashes and modes. Exact owned terminal cleanup and resource
absence pass; cleanup receipt:
`80491020d3914c9b92e58105ac9038bfaa14c5f0ce6755b8ae882c284675dfb6`.

Original providers, fixture assertions, schema63, immutable migrations0–62,
daemon helpers, case order and budgets remain. No retry or deadline increase.
These are modeled full-provider SQL component cases, not actual player journeys.
Both-engine legacy/v2 completion, private producer integration, pending-journal
recovery, shared Plans1–4 integration and release remain open. Plan1's original
independent deliverable remains complete within its recorded scope; future
shared changes retain their existing owning major-plan qualification.
