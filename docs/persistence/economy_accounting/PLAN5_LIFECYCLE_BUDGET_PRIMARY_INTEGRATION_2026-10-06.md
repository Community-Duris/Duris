# Lifecycle audit maximum read budget integration — 2026-10-06

The one-shot independent flatfile audit now admits the supported maximum native
lifecycle fixture within a bounded 16,384 physical-read budget. Its previous
2,048-read default was below the measured workload. Byte (128 MiB), directory
entry (8,192), cooperative time (30 seconds), decoder, read-lock and corruption
checks remain. Repeated cache reads still count. Retained-root and authority
pages retain their explicit 64-read/32 MiB budgets.

This integrates peer `8a137a688` over maintained `01c5ebf36`. The current paging
header is preserved; only the default read count and its explanatory comment
change. The lifecycle regression is the exact peer version. All previous
acceptance/refusal cases remain, with measured maximum-budget assertions added.
No native producer, schema, activation, writer registry or production data changes.

## Primary native qualification

Frozen raw-source archive:
`4cb64cdb2c1d6cc53d49c8e1465242ce48156599044818a850ea6ab9bc1efb2f`.
All 2,884 selected source members are authenticated and remain byte/mode exact.
The two unrelated SHOP harness edits and restore-session note are excluded.

The original registered lifecycle script runs without build cache in pinned image
`sha256:4994cc50a09a4acd40462ff412f3c3df21fc7c23a3f298f7fa725f0e2a350fb3`,
with no network/services or maintained runtime mounts. Original sanitizer flags,
fixture decoding, restore checks and read-only state comparisons are retained.
It passes 131 cases: nine accepted, 122 refused, zero skips, exit zero, in
427.235 seconds. The maximum fixture verifies one linked lifecycle receipt with
9,574 physical reads, 19,639,289 bytes and 2,675 directory entries. Those reads
exceed the old default and fit every current bound. Full-state byte, mode, link,
inode, size and timestamp comparisons remain mandatory.

Primary binds the three exported actual native ELF binaries and all 2,223 native
artifacts. Protected evidence is
`bin/tests/plan5-lifecycle-budget-primary-integration-20261006/`;
`PRIMARY-QUALIFICATION.json` records the scope and observations.
Artifact pins SHA256:
`7c585810acf980886c0500e2ab44a44dd883fd6f25f5cacbb26ebac7a9f47fbb`.
Log SHA256:
`8a9c41372394f248749d60d1e752e4194f33d2aa5d9193e2b638c6c60cb516dd`.
The owned container is absent; temporary state used ephemeral executable mounts.
Formatting, whitespace and normal accounting contract validation pass;
release_ready remains false. Existing central registration runs the complete
script and its added measured-budget assertions.

## Qualification boundary

The fixture uses genuine native codecs/common-baseline records but modeled
holdings. It does not execute source capture, lifecycle install, activation or
player routes. It does not qualify growing history, hard I/O latency, a release
host, complete native holdings or Plan5/R7/R8/release completion. Durable lifecycle,
baseline/orphan pagination and shared producer/cold-fault work remain open.
Existing inactive-accounting behavior and the declined spell path stay unchanged.
