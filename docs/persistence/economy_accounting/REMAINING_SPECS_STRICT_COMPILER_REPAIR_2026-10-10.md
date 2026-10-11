# Restore bounded object library strict compilation - 2026-10-10

The object-library bounded formatter now uses an annotated variadic measure/render implementation with two genuine va_list cursors. This removes the four strict format-truncation failures while preserving literal call-site format checking, the measured allocation, ordinary parser results, cleanup and budget ownership. The existing bounded producer receives only its required binding-stage friend capability. A failed or inconsistent second render now refuses with EINVAL/EOVERFLOW after cleanup; this exceptional correction is explicit, with the existing ENOBUFS priority retained. Accounting remains inactive and admission CLOSED; the declined inactive spell path is unchanged.

Independent RAW37 and actual formatted Source reviews passed. Fresh GNU13.3 strict production specs/specs.library.o compiled in both mariadb and flatfile outputs from published72af773 plus the exact two reviewed RAW files. Evidence: `tmp/svc10/native/RESULTS.json`, SHA256 `f495352a106b93e73537118aee01dcb40922fb8a3ec6a0710fcb43ffe30de4a8`; freeze `2388591fa62d947b8a4d69db57b2b4c2c6790441afd4480b5c0214dc186bfce7`. The earlier ten-object and specs20 batches remain failed evidence; their outcomes are not overwritten. The two header writer sites moved112->116 and176->180 with their actual statements unchanged. Whole combined compilation/link/boot and gameplay, persistence, recovery and host qualification remain OPEN.

Registry478 source pins,931 unchanged writer policies,2911 census occurrences,
2853 mapped sites and zero new/unmapped writers are source inventory only.
All unowned pins and three protected WIP files match the parent and disk.
Relevant strict objects compiled in both backends in a frozen private composition.
Full build/link/boot and gameplay/persistence/recovery checks remain OPEN.
Full selecting host, complete historical birth
joins, native32MiB and full Plans1-5/R1-R8/release qualification remain OPEN.
No major plan completion is claimed. Accounting inactive, admission CLOSED,
coverage incomplete, release BLOCKED, goal ACTIVE.
