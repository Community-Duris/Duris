# Schema65 lifecycle integration - 2026-10-10

The current branch now protects zone_reset_item_birth_origin as reconciliation
and replay evidence, includes original0065 in the default lifecycle scanner, and
requires its retention protection. Existing281 rows, global policies and the
original0063 origin row are preserved. The new row retains season/terminal
evidence, leaves controller/export/retention decisions pending, and excludes
canonical_origin and terminal_publication_context from personal export. Its
schema dependency is the original critical_operation_inbox FK.

Only the independently reviewed three-file lifecycle slice from completed
ca24bb1b2/fetched73e11477 is adopted over the current immutable migrations.
The peer migration tree is not adopted. The existing lifecycle fixture now
includes62/63/65 and the focused source-coverage/refusal checks; its23 methods
are not executed here under the major-plan test deferral.

Current Source metadata checks passed: lifecycle231 database tables/51 other
stores/42 Redis surfaces, destructive_rules_enabled=false; default runtime
profile remains valid at230 tables; explicit schema65 refuses with exit2 because
its fingerprints await actual two-engine measurement. That refusal is expected
and does not establish native compatibility. The earlier peer23/23 Python result
belongs to its frozen source composition, not this combined candidate.

Registry479 pins,931 unchanged writer policies,2911 census occurrences and2853
mapped sites with zero new/unmapped writers remain inventory only. One fixture
pin is added; all476 unowned pins, immutable migration/runtime controls and
protected WIP match the parent/disk. Native database/build/gameplay/recovery,
complete writer/activation and R1-R8/release qualification remain OPEN.
Accounting inactive, admission CLOSED, coverage incomplete, release BLOCKED,
goal ACTIVE.
