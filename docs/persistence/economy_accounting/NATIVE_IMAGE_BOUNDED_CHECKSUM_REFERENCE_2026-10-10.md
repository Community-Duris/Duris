# Native image bounded checksum and reference selection - 2026-10-10

The existing bounded image encode/decode still called unbounded reference
providers and one-shot SHA256. Their hidden allocations prevented those paths
from satisfying the intended prospective resource contract. Both complete image
bodies now select the real typed bounded reference codec and fixed SHA256_CTX
Init/Update/Final checksum helpers with authentic full SHA/compression/memory/
cleanse/comparison source inventories.

Original v1/v2 framing, source/cash rules, whole item forest validation and
canonical comparison survive, including every strong output transfer tail.
Typed reference failures propagate unchanged. Checksum checked-add overflow is
limit_exceeded; its profile/reserve refusal is allocation_failure; real bad
hash/digest is invalid_value. Inherited image admissions keep their original
unsupported_version and limit_exceeded results. No blanket result normalization
or new errno contract is introduced. Actual decoded item heap capacities replace
their forecast after decode; actual candidate/blob retention accompanies hashing.

Independent narrow RAW review authenticates all 52 sealed members under
aa8654ae, complete C forward/inverse, unchanged native header, full original
unbounded prefix and capture suffix, and exact reviewed reference/library copies.
The sole repository dependency advance is the published ce203882 reference
header's three comment lines, proved explicitly rather than silently repinned.
Final installed review verifies formatting/tokens/logical preprocessing and
whole inverses. Evidence: tmp/native-image-integrated-20261010 and
tmp/native-image-reviewed-root-join-20261010. The unchanged header is a dependency,
not a source change. Only one existing source pin refreshes; 458 pins, 931 writer
policies and complete mapped-site coverage survive. Protected Plan 5 work stays
excluded from the commit.

This closes the inherited image checksum/reference-provider source dependency
of the previous witness milestone. It does not qualify the full original
container/libc allocation profile, emitted/transitive storage or combined 32MiB
bound. Actual constructor/factory/publication/cold-adoption/ROOT and complete
backend/recovery/R1-R8 qualification remain OPEN. No compiler/native/gameplay/
persistence/recovery tests ran; those remain deferred to major-plan readiness.
Accounting stays inactive, admission CLOSED, coverage incomplete and release
BLOCKED; the goal remains ACTIVE. No major plan or runtime gate is complete.
