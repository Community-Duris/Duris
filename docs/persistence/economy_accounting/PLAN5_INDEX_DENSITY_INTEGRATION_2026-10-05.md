# Independent dense evidence-index integration — 2026-10-05

Peer Plan5 commit9b0816d6f establishes a missing audit rule: matching row counts,
balances and links could still conceal gap/offset numbering. Reader now checks
effects/postings/current-root item references exactly0..count-1 and children
exactly1..count, only after strict type/bounds and matching cardinality. Export
order remains free; original ownership/legacy history positions stay unchanged.
The bounded diagnostic is evidence_index_mismatch with existing root/table
envelope; duplicate and cardinality findings retain their meanings. No mutation,
shared schema/activation or automatic audit repair is introduced.

Three owned blobs imported exactly after current preimages matched9b's parent.
Primary source review confirms count bounds precede range allocation and current
root sequences remain distinct from filtered historical positions. Two AST,
raw SHA and whitespace checks only; no local unit/native/SQL execution.

[Peer frozen-input report](PLAN5_EVIDENCE_INDEX_DENSITY_SLICE_2026-10-05.md)
records117 selected passing tests, two-engine real SQL effect/posting corruption
captures, and synthetic child/item sequence tests. Native probes/budget samples
qualify frozena9f51/e018-era canonical0056 inputs, not this current combined
server or pending NPC/shop producers. Peer explicitly leaves maintained current
builds, migration/producer coherence and full release qualification open.

This integration does not claim zero remaining work, full runtime coverage or
release readiness. Existing inactive behavior and deferred major-plan testing
remain; coverage_complete=false and releaseBLOCKED.
