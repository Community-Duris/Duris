# Independent child-operation identity audit integration

The independent audit previously accepted zero, malformed, incorrectly derived
or reused child operation IDs as clean. The SQL exporter omitted the existing
domain, discriminator, relationship and nullable receipt identity needed to
check those IDs independently. Six exact blobs from peer
`50495fdd2fdf43ef5c513e6eb95f543a886b2711` are now integrated after confirming
their maintained preimages against `d90381b54` (allowing only checkout CRLF).

The reader derives each child identity from its original parent, domain and
discriminator, checks canonical IDs and reuse across the captured rows, and
reports absent evidence instead of assuming an older projection is complete.
The SELECT-only exporter supplies the original stored fields. No mutation
codec, schema, producer, coordinator or activation policy changes in this fix.
The exact peer [qualification report](PLAN5_CHILD_IDENTITY_QUALIFICATION_2026-10-05.md)
records 220 selected passing methods, including native SQL/flat components and
private MySQL/MariaDB fault cuts. Its frozen native tree and canonical migration
sequence 56 are its evidence boundary; these results do not qualify the current
combined server, complete parent-plan authentication or actual gameplay.

## Central qualification registration

The new mixed test owner is explicitly manual: a whole-module default run skips
its native and budget classes. Three matrix entries separately select all eight
pure methods, the one native/private SQL method and the one bounded workload
method. Each entry retains the existing no-skip outcome requirement and
900-second outer limit. The native entry composes the unchanged owner through
`run_plan5_child_identity_matrix.py`, which reads and hash-checks both original
`d90381b54` Git preimages before writing them to a fresh short nonce namespace
`bin/cid-<token>/n`. Its path is retained in the row transcript; this avoids
the default nested report path exceeding the database's Unix socket limit.
Missing history, reused output or hash disagreement refuses execution.
It does not reuse a probe or weaken the owner's explicit opt-in.

The inventory now contains 915 owners and 95 matrix rows. All 92 preceding rows,
their budgets and the existing engine policy are unchanged. The new owner's
one-second scheduling estimate is an unmeasured default; it is not a native
runtime claim. Exact peer blobs, Python AST, complete inventory, required-owner
coverage and unchanged preceding rows were checked. No local regression,
native compilation, SQL, service or recovery execution was performed: major-plan
testing remains deferred as requested.

Original parent EAP1/detail and child-receipt authentication, actual native
capture/producers, cold publication and ACK, coherent migrations, combined
builds and journeys, writer evidence and full R1–R8 remain open. Accounting
activation and release remain blocked; inactive behavior and the declined
inactive spell path are preserved.
