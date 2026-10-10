# Complete bounded WORLD/SHOP resource reporting - 2026-10-10

The ordinary full physical aggregate could not distinguish resource refusal from
corrupt catalog data: WORLD's boolean decode/validation helpers swallowed bad_alloc
and nested codec failures, while SHOP's bounded decoder did likewise and its
reader replaced the outcome with EBADMSG. Complete sizing paths also lost their
resource origin. The selected bounded all-world/shop readers now use full private
semantic and sizing companions carrying an explicit typed resource state.

Allocation exceptions and codec allocation_failure remain io_error/ENOMEM;
checked arithmetic, admission and applicable codec resource limits remain
io_error/ENOBUFS; unsupported storage policy remains io_error/ENOTSUP. Actual
malformed or unknown wire versions stay invalid. No errno inference follows an
already-flattened boolean result, and no allocating custom exception is introduced.

Every original WORLD v1/v2/v3 corpse/saved/room/name/money/order/global UID/item
forest and SHOP v1/v2 legacy cash/affects/equipment/expected item count/global UID
law survives. Existing unbounded/shared helpers and headers remain unchanged.
Prospective profiles retain full original requests and actual capacities; actual
new state/reference/closure/enum carriers and larger WORLD decoder objects are
admitted. Outputs and WORLD retained-heap scalar transfer only after final
borrowed-lock authentication, with no fallible callback afterward.

Independent full RAW and actual formatted SOURCE review passed all118 members,
the complete51-member predecessor plus its manifest, seven current captures,
twelve full original algorithms, full two-file forward/inverses and ten unchanged
shared/legacy helper bodies. Full phase-2 tokens and logical preprocessing,
headers and three protected WIP files authenticate. All439 source pins match
actual checkout/proof; registry and matrix each change only two source-pin values.
All931 policies/routes,2911 occurrences and2853 mapped unique sites remain unchanged;
zero new/unmapped sites. These are complete finite source checks, not native tests.

Proposal:tmp/ordinary-world-shop-complete-resource-retained-owner-20261010.
Manifest:80aeb7b236f0389bb3d37e1ab327ca76ab87157035cb45dd7dae0296ada56c91.
Evidence:tmp/ordinary-world-shop-resource-integrated-20261010.
Reviewer:/root/ordinary_authority_review.

This closes the identified WORLD/SHOP resource-reporting source dependency of the
proposed aggregate. Full PLAYER/PET/AUCTION/COLLECTOR installation, aggregate and
genuine CURRENT transaction/caller remain unfinished. Existing command/receipt
enum APIs intentionally retain their original acceptance/refusal categories;
native qualification must measure them without assuming allocator-origin detail
the API does not provide. No new error enum is required by this source review.
Execution/publication/cold recovery/terminal ACK and native/library/allocator/
emitted/global32MiB/R1-R8 qualification remain OPEN. Native/build/gameplay/
persistence/recovery testing stays deferred to the major-plan batch. Accounting
inactive; admission CLOSED; coverage incomplete; release BLOCKED; goal ACTIVE.
