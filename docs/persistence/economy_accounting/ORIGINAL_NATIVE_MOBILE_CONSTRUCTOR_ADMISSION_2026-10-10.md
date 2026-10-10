# Original native mobile constructor admission — 2026-10-10

The native mobile constructor still used fatal pool/allocation and unbounded
capture helpers in its bounded caller chain. This change supplies warm NBC2
capture and frozen NBC2/3 replay through the complete original loader body,
with prospective pool, NPC allocation, parser/cache, diagnostic and fixed-SHA
admission. Existing unbounded callers retain their original default selection.

Real pool requests use mm_try_reserve_free_slot/mm_try_get; NPC allocation uses
the actual nonfatal allocator and MEMCHK header. Exact original tilde/string
cache parsing, RNG/time/fallback, conversion and witness streams remain.
The caller refreshes genuine current globals; detached NPC storage stays with
the actual stage until publication. Original strong output transfer survives.

Two independent source-review corrections distinguish unsupported profile,
invalid input, overflow, reserve refusal and allocation failure using existing
errno categories. The first refusal survives cleanup. Actual metadata getters
and allocator calls isolate stale errno; corrupt storage reports EIO rather
than an inherited ENOMEM/ENOBUFS. Successful calls restore prior errno where
required. The original default constructor behavior remains unchanged.

Independent RAW and final installed source review authenticates the complete
75f2a69a -> 464418e3 -> 4cfad962 chain (70/74/74 members), 23 current repository
dependencies, 29 captured library files, two whole inverses, genuine b7e5da182
preimages and explicit newline aliases. Formatting preserves all phase-two
tokens and logical preprocessing. All458 existing pins authenticate, with two
hash updates/no additions. All931 policies remain unchanged. 56 writer sites,
56 census coordinates and18 definition lines relocate to identical original
source text; all2853 unique sites remain mapped. Protected Plan5 WIP is intact.
Evidence: tmp/mobile-constructor-integrated-20261010 and
tmp/mobile-constructor-reviewed-root-join-20261010.

Actual warm factory, cold publisher, complete item construction, adoption and
mixed startup integration remain unfinished. Retained stdio/sscanf/conversion,
emitted/transitive/caller storage and combined native32MiB qualification remain
OPEN. No compiler/native/gameplay/persistence/recovery test ran: execution is
deferred to major-plan readiness. Inactive accounting and CLOSED admission
remain. Coverage is incomplete; release BLOCKED; goal ACTIVE. This source
milestone completes no major plan or release gate.
