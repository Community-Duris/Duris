# shady2 builder questions and recommended resolutions

Checkpoint 530, source snapshot `c500ac51b8cd26540943257e7ca4e9807263ef74`.
These questions concern the unlisted 90-record mobile file, its shared
VNUMs and variant fields against active `shady.mob`, and its current typed
consumers. They do not authorize changing, merging, moving, activating, or
archiving either source.

## Explicit builder questions and recommended resolutions

1. **Who owns `areas/mob/shady2.mob` and can decide its source status?**
   Recommended resolution: identify the responsible builder or area owner
   before proposing any change to the file or its identity.
2. **What original task or tool produced the separate `shady2` file?**
   Recommended resolution: recover its editor, export, backup, or import
   provenance instead of assuming it is a duplicate backup.
3. **Was the lack of `shady2` from `areas/AREA` and `areas/qst/AREA`
   intentional?** Recommended resolution: ask manifest and deployment
   owners to state whether the file is intentionally source-only.
4. **Who owns mobile VNUMs #97500–#97589 when both files define them?**
   Recommended resolution: record the intended namespace and keep the
   distinct source records intact until both owners agree.
5. **Is `shady2.mob` a variant set, working copy, historical snapshot, or
   independent source?** Recommended resolution: use provenance evidence
   and builder confirmation; the shared VNUM range alone cannot decide.
6. **Are the different stat and mechanical lines across all 90 shared
   records intentional?** Recommended resolution: have a mobile-data owner
   review the exact field differences before any synchronization.
7. **Are the five differing keyword lists for #97540, #97545, #97562,
   #97580, and #97582 intentional?** Recommended resolution: confirm which
   aliases each consumer needs, preserving both authored variants meanwhile.
8. **Should #97580's differing short, long, and detailed descriptions be
   retained as an additional variant?** Recommended resolution: have the
   author explain its distinct prose and behavior before considering a copy.
9. **Does the active 106-record `shady.mob` set own the live Shady Grove
   identities?** Recommended resolution: document its relationship to the
   90 unlisted IDs and the extra #97590–#97605 records.
10. **Do the 342 active ZON `M` rows across `shady`, `alatorin`, and
    `caertannad` intentionally resolve to active `shady.mob`?** Recommended
    resolution: preserve these current typed VNUM consumers and confirm
    their reset roles with the listed area owners.
11. **Do all 12 active `shady.shp` keeper IDs continue to belong to the
    listed Shady prototypes?** Recommended resolution: keep their keeper
    bindings attached to active Shady while shop ownership is confirmed.
12. **Do the four `shady.qst` headers #97510, #97537, #97545, and #97548
    belong to the active Shady QST family?** Recommended resolution: confirm
    the header-to-mobile ownership and preserve those QST records as typed
    consumers of the active identities.
13. **Does the `shady.story.json` sidecar apply only to active `shady`?**
    Recommended resolution: retain its explicit `source_area: shady` and
    obtain separate narrative provenance before associating it with
    `shady2`.
14. **Are the C/C++ special assignments and helper references meant for
    the active prototypes at the shared numbers?** Recommended resolution:
    preserve the exact VNUM contracts until code and area owners verify any
    intended variant-specific behavior.
15. **Does the `nexus56` use of #97521 remain an OBJ dependency?**
    Recommended resolution: keep it classified as an object stock/reset
    reference, not a same-number mobile consumer.
16. **Does any editor or deployment tool load unlisted `shady2.mob`
    independently of the manifests?** Recommended resolution: check those
    tools and their file-selection rules before concluding that the file is
    unused.
17. **Do generated aggregates, staging inputs, or private exports copy
    either file?** Recommended resolution: record the authoritative
    generation inputs and preserve source and output provenance.
18. **Do saved characters, scripts, or other persisted records rely on
    identities or behavior specific to `shady2`?** Recommended resolution:
    close saved-reference and runtime integration separately from static
    source matches.
19. **Does tracked history contain an owner-approved retirement or
    replacement decision?** Recommended resolution: retain the separate
    file unless an explicit source decision is recovered.
20. **What path decision is approved after the provenance and consumer
    review?** Recommended resolution: keep `shady2.mob` RETAIN IN PLACE and
    separate from `shady.mob` until the owner documents any later decision.
21. **Do quest/story item IDs #97514, #97549, #97570, #97572, and #97582
    target their active Shady OBJ records even though the numbers overlap
    `shady2.mob`?** Recommended resolution: preserve the item-typed Shady OBJ
    contracts, including Alatorin's orcish-forged sword #97549; keep the
    same-number mobile records distinct and do not retarget by numeric match.

See the [shady2 static audit](SHADY2_AUDIT_REVIEW.md).
