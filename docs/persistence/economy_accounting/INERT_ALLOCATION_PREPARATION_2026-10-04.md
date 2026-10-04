# Inert recovery allocation prerequisites — 2026-10-04

Status: **IMPLEMENTED, SOURCE-REVIEWED, UNQUALIFIED**. No compiler/native/SQL/
gameplay/recovery tests ran. Existing pool growth calls fatal_boot_error on mmap
failure; CREATE panics and MEMCHK allocation exits. A C++ catch cannot make those
paths recoverable, so the opt-in inert recovery owner requires separate primitives.

`mm_try_get` acquires only an existing free slot, zeroes it and updates the ordinary
pool count. Null/exhausted pools return null without growth or metadata changes;
release uses normal mm_release. This deliberately may defer recovery even when the
system could allocate more memory. Normal mm_get/growth behavior is unchanged.

`__try_malloc` rejects zero/invalid arguments and MEMCHK header-size overflow,
returns null on allocation failure and retains the aligned debug header, metadata
and counters expected by normal __free/str_free. MEMCHK>1 prepares its existing log
before accepting storage so cleanup does not first encounter a fatal lazy-log-open
failure. It does not change CREATE, __malloc or ordinary deallocation behavior.
Tag and source-file strings must remain valid for the allocation lifetime.

These are main-thread primitives over initialized live pools/memory infrastructure,
not cross-thread allocator locks. All stages must drain before free_world or
memory-log teardown. Existing dump_mem_log closes mem_log without clearing it;
post-teardown cleanup is outside this contract. MEMCHK>1 already contains getmem's
undeclared p and dump's absent header linkage: that configuration is not build
qualified and those preexisting issues are outside this narrow source slice.

Private BEFORE base8c76443a690675e45efe2f32c3d36deea3dcffff:
`tmp/inert-allocation-before-v1.local/manifest.json`, SHA-256
`7e31544cb603f28c359412fcc7596fa3a4a6c8fdccdac2d3d7ede30968bd62cf`.
Private native fixture preparation is pending; missing BEFORE APIs are unsupported,
not semantic RED. Source review and formatting/diff hygiene are the only checks.

## Source pins

- `src/core/mm.c`: `b47d7cad44b6897cc2d411a26977a4567f0b42b526366694a5850680ddc79ba1`.
- `src/core/mm.h`: `b1cbc8a3813af53764e26d32ab0cc040eddc9754cbb25ffb2f80fdfbf25a612f`.
- `src/core/memory.c`: `b46ebbf02fc3efb7e815c119b4cabbce34c7129c11fb2bfe05d8c58ce557659e`.
- `src/core/prototypes.h`: `d10f6f2844bdd8d1821f99494e1b3d77fd1a9bae247f65cbb402767932c623a5`.

## Remaining owner

The separate move-only literal object stage must own every partial string/node/pool
slot immediately and clean up without extract_obj/free_obj/affect/event callbacks.
It must preserve trusted retained UID and exact four-bit SQL literal strings,
weights, descriptions and saved fields without template conversion, UID issuance,
procedures, RNG, global list/count/cache changes or runtime custody registration.
Unsupported dynamic/procedure/trap/artifact/corpse/money routes refuse before
construction. Current read_object and maintained materializers remain unchanged.
Prepared prototypes, complete current-authority graph/epoch proof, all allocation,
atomic runtime hydration and nonthrowing final enrollment remain separate ownership
gates. These primitives do not establish an inert factory or full cold recovery.
