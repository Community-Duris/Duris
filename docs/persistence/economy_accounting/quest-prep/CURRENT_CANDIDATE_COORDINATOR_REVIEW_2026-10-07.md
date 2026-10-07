# Current quest preparation: coordinator checkpoint - 2026-10-07

Reviewed prep revision `36bfef3c9e9a97b5dd94fbf620ffe2f21e02a8d1` preserves the
published producer candidate `275df7f626e12cb396a22da34317a4e7f355e9a1` through
history-preserving merge `1fe9f048fe8f08d8bc27c13c632c6b3758b5198e`.
Compatibility coverage is published at
`7429e4f21d83d9bd8d07c5447bd33b13e2f5ec0c`.

The coordinator exported the exact Git revision to a separate ignored snapshot
and independently ran:

```text
python3 tests/async/quest_accounting_prep/test_native_selectors.py --acceptance
```

All four QP01/QP02/QP05/QP06 components pass with the published compiler flags and
unchanged assertions. Actual `src/world/quest.c` SHA256:
`52ff1d3db3aa77a56f78a0167dc487ba6e2ae2d325934421c31bf96c7c124e4b`.
The selector coverage now follows actual native cash-cost availability, funded
recipe precedence, shortage fallback, unreadable-cash refusal and duplicate/item
selection. It uses constructed NPC stock and cash references and stops before
custody/payload capture. This is independent component proof; it does not qualify
genuine active native authority, SQL admission, rewards, ACK or retirement.

The coordinator inspected the owned capture-reader change against schema63.
The snapshot requires the retained-origin table to be InnoDB, bounds combined
canonical-origin/image/continuation BLOB material before fetching/hex expansion,
captures the actual birth operation/publication revision/origin bytes, and
includes those operations in the existing receipt/accounting evidence queries.
The existing read-only consistent transaction, identity checks, row/byte limits
and rollback remain intact. Original birth bytes and the current mobile image
remain separate. Only the maintained native owner validates reconstruction
authority; this reader neither interprets the origin envelope nor grants it.
Absent origin rows stay absent. This review does not claim an executed populated
native-origin SQL cut.

The worker's reserved next run is the original QP06 SQL Kord XP-ACK/later-move
journey using a fresh source/ELF and schema64 in a disposable loopback database.
The bounded legacy-no-epoch scope is appropriate: no invented epoch, birth,
independent-route verifier or reference rows, and native origins remain empty.
Preserve production prototypes, specials, fault/deadline calibration, reward
UIDs, ACK and original cold-state checks. Current build/run pins and terminal
results remain pending. Missing inspector providers must be supplied from the
existing maintained sources in the owned invocation with original controls;
the shared provider-manifest dependency remains an explicit primary handoff.

This checkpoint neither adopts an optional production patch nor reopens a
completed primary area. Historical blanket paid-refusal QP02 is superseded;
QP07's optional guard handoff and its separate native-refund requirements remain.
Both ongoing workstream Goals and coordination remain active under the broader
primary Plans1-5/original R1-R8 completion boundary.
