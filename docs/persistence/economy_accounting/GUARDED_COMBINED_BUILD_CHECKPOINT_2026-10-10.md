# Guarded combined build checkpoint - 2026-10-10

Frozen published candidate `3049fb7712de7ccd957c61d5d345d2c8a4cf838d`
passes fresh production C++20 builds and links in both SQL and flat-file modes.
The existing inactive flat-file boot preflight also passed. This closes the
combined build failure for that candidate after the separate compiler repairs.

Evidence: `tmp/guarded-candidate-3049fb771-20261010/native/RESULTS.json`,
SHA256 `cc47d3eece9c225b54f88cd756fe860e2f9ff9a49ba2764dfbe0e7aa79e1e6e5`; freeze `3887f698260ce01a454aa05f1a8a77df51e194c272c045fc2930426edf8b3de4`.
Actual g++13.3.0, production warnings-as-errors and original safety flags,
fresh object directories and authentic private linker aliases were used.
Source and installed linker inputs stayed unchanged. No warning suppression,
production data, activation or live-game interactions were used.

- SQL build/link: exit0, binary `3f34b8ee8e174af686d8b557fa95cc2c58e2b7b68f62112c9c53de6dc230ca7b`.
- Flat-file build/link: exit0, binary `ee8e22071d64d32e746ddfd047b0a241889e6e98de6498d6c037b292f996f383`.
- Inactive flat boot: exit0; original client-free health, three boots,
  missing/stale UID authority refusals, exact restoration and clean shutdown.
- SQL service smoke: NOT_RUN; separate disposable schema/service ownership.

This checkpoint predates critical provider `08ef94a4c` and bounded-list provider
`240fc9ef1`. It cannot qualify those newer Source definitions or selecting
consumers. Targeted provider compiler results are recorded separately.
Native runtime/library correspondence, actual accepting first admission,
aggregate32MiB accept/refuse, producer/publication/restart journeys and full
gameplay/persistence/recovery/release qualification remain OPEN.
No complete Plan1-5 or R1-R8 completion is claimed. Accounting inactive,
admission CLOSED, coverage incomplete, release BLOCKED, ongoing goal ACTIVE.
