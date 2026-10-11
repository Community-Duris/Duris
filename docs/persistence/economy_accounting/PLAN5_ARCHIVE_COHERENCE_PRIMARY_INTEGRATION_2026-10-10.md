# Frozen backup archive coherence integrated - 2026-10-10

Backup capture now records the copied migration inventory and frozen schema
digest. Before writing metadata, syncing, publishing or pruning generations,
backup verifies that the staged schema and migration bytes still match their
capture. Drift refuses publication and preserves the previous generation.

Migration archive validation checks all six complete selectors, the declared
payload checksums, and exact archive membership. The primary archive retains
140 inputs and780 checksum references across nine authentic history heads.
Existing restore verification reaches these checks before candidate creation
and after qualification; no restore source edit is necessary. Only legacy
generations without a runtime schema profile may omit the archive. A marked
profile or existing archive requires authentic contents. The schema64 fallback,
unmeasured/null-fingerprint schema65 refusal and immutable history checks remain.

The narrow a3b04e5d peer slice changes backup and adds three exact peer tests:
staged migration drift refusal, invalid archive despite consistent outer
inventory, and legacy archive absence. All eight previously integrated history
and runtime-profile test bodies are unchanged. Independent162-member Source
review and actual installed byte/AST/inverse review passed. Current migration
tree and five dependency controls remain unchanged; protected WIP is preserved.

No tests, DB, native, backup/restore or release qualification executed for this
milestone. Affected suites remain in the major-plan batch. Registry refresh
preserves931 policies,2911 census occurrences,2853 mapped sites and zero new
writers; inventory is not accounting completion. Plans2-4/fullPlan5/R1-R8 and
native gameplay/persistence/recovery/activation/release qualification remain
OPEN. Original Plan1 acceptance keeps its recorded scope. Accounting inactive,
admission CLOSED, coverage incomplete, release BLOCKED, goal ACTIVE.
