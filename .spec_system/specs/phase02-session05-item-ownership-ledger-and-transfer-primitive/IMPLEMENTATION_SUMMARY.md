# Implementation Summary

Durable items now have database-reserved collision-free identities, one typed current
owner, a per-item revision, and an immutable operation ledger. A bounded transfer locks
both owner revisions and the complete indexed item subtree, then commits all custody
rows, revisions, ledger events, exact inbox result, and outbox notification atomically.

Creation, ordinary custody transfer, and terminal destruction are explicit states.
Missing, duplicate, stale, cyclic, incomplete, and overflowed declarations fail closed.
The guarded legacy importer captures only unambiguous player, corpse, locker, and floor
evidence and quarantines conflicts or opaque auction blobs for explicit repair.

Project version: `1.81.34`
