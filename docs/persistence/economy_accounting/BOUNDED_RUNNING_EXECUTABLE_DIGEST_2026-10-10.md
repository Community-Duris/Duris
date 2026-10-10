# Bounded running executable digest - 2026-10-10

The original process executable digest's fixed file buffer still used an
allocating EVP_MD_CTX on first access. The new bounded entry admits its complete
fixed file/SHA workspace before initializing the actual process cache, then
hashes the genuine /proc/self/exe descriptor with SHA256_CTX Init/Update/Final.
Both default and bounded entries share exactly one real retained static cache
and once-only guard. Whichever entry first captures supplies the result for
both, including genuine permanent failure. Null/profile/overflow/reserve
refusal before initialization cannot poison that cache. Default first-use still
uses the byte-identical original EVP capture body and original semantics.

The bounded path preserves the original same-descriptor ELF, regular-file
length, EINTR, exact extent, early EOF, one-byte tail, device/inode/size/time
comparison and close checks. Cached syscall errors survive cleanup; logical
mismatch/SHA failure is EIO. An original cached boolean failure has no retained
cause, so bounded access reports EIO rather than inferring it from stale errno.
Output changes only after a valid shared-cache value exists. Existing bounded
profile and actual width checks refuse ENOTSUP before reading or allocating.
No new wire format, public enum, authority or activation gate is added.

Independent RAW review authenticates all 58 members (ebe1984c), all 47 reference
predecessor members (02b2b723), both whole forward/inverse pairs and current
preimages. All 45 authentic library captures include complete SHA leaves,
installed file/stat headers and GCC13 guard.cc. The original EVP body remains
2154 bytes, SHA256 10f02d8c8d9d57dcd6f9fb745a4e2db6ad31f0f8e1ea476f6727ca85bb3106ca.
A fresh reviewer WSL header read was unavailable: authentication proves the
preserved captures, not live runtime binary correspondence. Final installed
review verifies source formatting/tokens/logical preprocessing, whole inverses,
both genuine preimages and unchanged original body/cache semantics.
Evidence is in tmp/running-artifact-integrated-20261010 and the immutable
tmp/native-running-artifact-digest-owner-20261010 packet. CHARACTERIZATION.md
specifies meaningful cache-order/refusal/same-FD/error cases for the batch.

The two existing source pins refresh; all 458 pins, 931 writer policies and
mapped-site coverage survive, and protected Plan 5 work remains excluded.
No compiler/native/gameplay/persistence/recovery checks ran; these remain
deferred until major-plan readiness. Real constructor/publication/recovery
selection, emitted/transitive/libc/allocator/global memory, mixed-thread behavior
and combined32MiB/R1-R8 qualification remain OPEN. Accounting stays inactive,
admission CLOSED, coverage incomplete and release BLOCKED; goal ACTIVE.
This source milestone does not complete a major plan or native acceptance gate.
