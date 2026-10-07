# Independent flatfile pages integration — 2026-10-06

The maintained independent reader now durably rotates retained-root and
authority-crosslink pages. It retains pinned range ceilings, observes late lower
arrivals in subsequent ranges, preserves sticky refusals, and lets other buckets
advance without advancing a refused bucket. Native authority remains read-only;
protected checkpoint publication and exclusive operator locking stay outside it.

This complete dependent slice integrates peer0f40bbd9c andbc58072b4. The shared
progress helper preserves bounded regular-file reads, duplicate-field rejection,
exclusive lock ownership and fsync/replace publication. Native framing, original
semantic checks and source/lineage bindings remain required. Partial page reports
do not claim complete reconstruction or release clearance.

## Primary qualification

The full original native script passed in362.135 seconds:28 retained-root and29
authority-page controls,12 lock/authority controls, nine operator-limit controls,
20 valid stores and367 corruption refusals. It also retained1058 metadata and574
command-envelope comparisons,11 accepted native records and ten malformed
envelope refusals. Native/economic inventories and2883 selected source bytes/
modes remained unchanged. Original sanitizer fixture compilation reused no objects.

The qualifier, fixture and sanitized independent reader hashes were respectively
`0a599945e9f9c89709454abd69714c3acd32a545f567110c4bce2bf31429d890`,
`7a51c67892684944eeca63352d91b0c34cfbdab157ce4ce415e706d579ca3717`, and
`1a9e94becd681335b80de868c7a4ae540c2c5e99acee9a9e9c49577c3c2d002d`.

Frozen source transport is9d3fce1d; its only difference from the integrated
b2207029 transport is an observational exception handler in the original test.
It records a refused timestamp/checkpoint before re-raising the same refusal;
no original assertion, native source, flag, limit or timestamp guard changes.
The successful run never entered that handler. Log SHA256:
`12f58cc5101b7b4170d40a00984a7c7b70d81d45afd36e0dbb3e6849c84c521d`.
Protected evidence:
`bin/tests/plan5-flatfile-pages-timestamp-executable-primary-20261006/`.

The first original run refused at timestamp validation; its exact rejected
field/value was not logged and the refusal did not recur in the diagnostic run.
No executed clock regression is inferred. A separate diagnostic attempt failed
because its temporary mount prohibited fixture execution; an executable mount
corrected that test-environment error. Both earlier logs/results remain retained.
The current guard is preserved. Two added focused tests prove invalid timestamp
type/range/order refusal and a modeled backward clock refusing before any native
page or checkpoint mutation; both pass without skips. They do not attribute the
first original refusal. Owned diagnostic containers are absent after completion.

The complete native script and its existing/new output markers are registered
in the central integration manifest. The new timestamp tests are registered in
the fast regression manifest. Changed C++ formatting and normal accounting
contract validation pass; release_ready remains false.

## Remaining work

The separate private producer source, real player journeys, complete native
holdings, baseline/lifecycle/orphan pagination, full backup/service restoration,
R7/R8, activation and measured release workloads remain open. No whole Plan or
release gate is promoted. Existing inactive behavior and the declined spell
path remain unchanged.
