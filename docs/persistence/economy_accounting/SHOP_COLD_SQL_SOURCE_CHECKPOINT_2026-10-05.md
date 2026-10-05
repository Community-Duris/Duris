# SHOP cold SQL source checkpoint

The four-file private source slice now supplies the missing actual-value player
reader and original-transaction full-forest authentication. It is reviewed and
frozen locally; maintained executable sources are not replaced by this slice.
The immutable accepted receipt SHA-256 is
`e55f608f5f210c084f01e4a0155a4a9e47af1eb7149b414c525bd1292c07b3ae`.

| Private candidate | Exact source SHA-256 |
| --- | --- |
| `economic_sql_shop_trade_transaction.c` | `85313b3af581f14c134cf8d5fbeb9c5e5d17b100e42e0ed1981c1b947b0c2e79` |
| `economic_sql_shop_trade_transaction.h` | `8096c5ff415b9fde2c2d0205418204e08f94da09a4e7f2cc7871e70275c2274b` |
| `shop_item_runtime_payload.c` | `de7b7ee2712b1130059d8bcee801a2c245d1d847834056d1de33e61a6ff6b863` |
| `shop_item_runtime_payload.h` | `51eba38c026d8f090aab047e040de7e773958bf6f58beb166f55add60e48d376` |

Cold recovery cannot manufacture full player item values from retained UIDs.
The new UID-span reader uses actual canonical sidecars, complete physical rows
and current custody. It reconstructs only stored standalone parent indices from
matched parent identities. The existing value-span reader compares its entire
canonical body through the same implementation, preserving actual string policy,
slots, properties, topology, count, foreign-child and duplicate-copy checks.

For v8, the SQL adapter acquires authority, player/bank, keeper and all four
sorted owner revisions, then the complete sorted custody union before physical
reads. All four persisted manifest forests, selected/stock/target identities,
keeper routes and active root/parent references participate. Canonical player
and keeper values authenticate the original BEFORE and prospective AFTER phase.
AFTER validation follows native mutation, item events and revision advancement
inside the same original transaction, before parent receipt/outbox completion.
It never invokes historical sealed-receipt validation before that receipt exists.

Independent source review established and corrected two concrete omissions:
keeper physical copies in foreign stores, and extra active keeper-context or
foreign root/parent custody references ignored by narrower image readers.
The original broad cut now validates exact phase keeper membership/ownership.
AFTER rereads only its retained bounded UID/gap-lock scope, including initially
absent produced UIDs. It validates closure without acquiring a newly discovered
scope after physical mutation. Tombstones and legitimate pre-production absence
remain allowed. Failed AFTER proof takes the parent's rollback path; it cannot
become a business-denial receipt or ACK.

The persistence specialist accepted this bounded correction and the reader's
source contracts. Changed-line clang18 formatting, exact preimages and pins
passed. No compilation, tests, SQL, services, native gameplay or restoration
executed. The legacy precommit owner sequence remains in the no-manifest branch.
Three private runtime enrollment selectors accept explicit `(6,7,8)` only with
the frozen v8 unchanged result contract.

Next integration must assemble the command/producer/runtime/link dependencies
and coherent schema chain. Original cold startup rebind, actorless physical
publication, retained native operation ownership, guarded ACK and SQL/flat
major-plan qualification remain unfinished. Existing inactive behavior and
safety gates stay intact. These source findings neither qualify current branch
execution nor satisfy full accounting completion; R1–R8 remain open.
