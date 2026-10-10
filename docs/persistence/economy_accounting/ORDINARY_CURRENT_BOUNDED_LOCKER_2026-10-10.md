# Complete ordinary CURRENT bounded locker reader - 2026-10-10

The full ordinary physical census needs every valid locker and all retained
chests/items, including empty and non-selected entries. The new passive bounded
reader preserves the original complete v1/v2 catalog, framing/magic/digest,
canonical account and expected-name checks, owner kinds, sorted IDs, one public
chest rule, nested item/forest validation and all uniqueness constraints.
Original readers remain unchanged. Both output and actual retained-heap scalar
transfer together after all fallible validation and final authority-lock checks.

Prospective allocation requests and actual capacities cover the complete catalog,
sorted scalar/string-view validation arrays, nested item decoding and canonical
encoding. Reading the item blob through its wire span avoids a duplicate buffer.
Incremental retained-capacity accounting avoids rescanning the whole catalog at
every allocation cut. The complete corrected successor preserves resource errors:
outer allocation failure and nested codec allocation failure return io_error/ENOMEM;
reservation refusal returns ENOBUFS; semantic rejection remains invalid. This
retains the original complete proof rather than treating a budget refusal as
corrupt evidence.

Independent full RAW and exact formatted SOURCE review passed all158 members,
the preserved114-member predecessor, ten full original algorithms, seven current
captures, two complete forward/inverse pairs and the narrow resource correction.
Actual tokens, logical preprocessing, original C prefix and three unrelated WIP
hashes authenticate. The registry now includes the two existing locker source
files: all439 pins authenticate against checkout and disposable proof. All931
writer policies remain unchanged,2853 mapped unique sites, zero new/unmapped
sites. Source pin coverage is not executable writer or accounting completion.

Immutable proposal:tmp/ordinary-current-locker-bounded-complete-resource-retained-owner-20261010.
Manifest:cab02d3489dcf3f0f3a69226d61f4740641bbea508e7a5358ea2db9d24b8df8a.
Evidence:tmp/ordinary-current-locker-integrated-20261010.
Reviewer:/root/ordinary_reference_review.

The companion remains unselected until the complete physical aggregate and real
CURRENT caller join. Native/OpenSSL/library/emitted-frame/global32MiB qualification,
execution/publication/cold recovery/terminal ACK and original R1-R8 release gates
remain OPEN. Native/build/gameplay/persistence/recovery checks remain deferred to
the major-plan batch. Accounting inactive; admission CLOSED; coverage incomplete;
release BLOCKED; goal ACTIVE. No complete plan is claimed by this companion.
