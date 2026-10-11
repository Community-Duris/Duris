# Frozen runtime profile integration - 2026-10-10

Operator backups now bind the actual schema64/65 profile to the generation's
frozen runtime contract, dump-table coverage and pre/capture/post verification.
The database table count selects the verifier profile; it does not qualify the
database by itself. Profile drift refuses publication and preserves prior
generations. Restore propagates the same frozen profile and schema through its
verification and database-qualification calls before and after replay.

An older SQL64 generation remains supported when the checkout adds the nested65
profile: only that additive nested object is stripped for64 compatibility, while
all original64 contract fields must still match. Exact integer64/65 profile
selection is enforced. Schema65 remains unmeasured with null two-engine
fingerprints and is refused before verifier dispatch. No gate is relaxed.

Backup also archives all140 current primary migration inputs: six authentic
selectors and134 distinct apply/verify payloads. Content checksums, generation
inventory and existing capacity limits remain enforced. Archival capture does
not execute migrations; the separately reviewed archive-coherence enforcement
slice still awaits integration. Current immutable migration/runtime controls
and the previously integrated exact-history test methods are unchanged.

This adopts only the narrow b68c4527 profile slice with three new helpers, five
peer tests and two fixture adaptations. Older metadata/scheduling/drill changes,
strict report validation and room reconciliation hooks are excluded. Independent
167-member Source review verified exact current preimages,15 reversible edits,
peer helper/test correspondence,780 checksum references, nine authentic history
heads and230/231 table lists. Independent final Source/AST fidelity also passed.
No tests, database, runtime/native or restore qualification executed here;
affected suites remain in the major-plan batch on the combined candidate.

The registry/matrix Source refresh preserves931 writer policies,2911 census
occurrences,2853 mapped sites and zero new/unmapped writers. Inventory is not
completion. Full backup/restore evidence, native gameplay/persistence/recovery,
writer/activation ownership, Plans2-4/fullPlan5/R1-R8 and release remain OPEN.
Accounting inactive, admission CLOSED, coverage incomplete, release BLOCKED,
goal ACTIVE.
