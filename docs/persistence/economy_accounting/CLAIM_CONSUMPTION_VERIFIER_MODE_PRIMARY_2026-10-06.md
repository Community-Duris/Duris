# Canonical0062 verifier executable mode — 2026-10-06

The original raw-Git Linux restore runner failed on both private engines with
PermissionError while migration_runner.py directly executed the new0062
verifier. Git had recorded100644. This repair records100755, preserving the
sealed verifier blob and SHA2566693a797f7d4a259f412f0ac54d2324ac34e9612a5dcdac298aa459afdbf29ed.
No migration SQL, verifier content, historical immutable files or manifests change.
Earlier schema/build results remain evidence for their recorded transport;
they did not prove this raw-Git executable-mode requirement.

The complete original test_restore_economic_coin_effects.py now passes with
zero skips in 549.370s on a raw-Git archive plus the separately
integrated read-only NPC successor and this mode repair. Both fresh engines
explicitly report migration_head=0062_economic_pending_claim_consumption:
MariaDB10.11.14 and MySQL8.0.46. Original native SQL/flatfile fixtures compile
with cache off;32 native cases and3026 independent decoder decisions agree.
Both engines pass84 retained canonical cuts,72 refusals and51 full-entry cuts,
including45 native-NPC cuts and259-root second-page validation. SELECT-only
capture preserves authority. Original flags, providers and deadlines remain.

The original summary JSON still contains a historical hardcoded schema_head61
label; the setup's executed head query/assertion and engine lines prove62.
This report does not treat the stale summary label as current schema evidence.
The first failure and retry remain separately retained under
bin/tests/plan5-sql-native-mobile-primary-20261006 and
bin/tests/plan5-sql-native-mobile-primary-retry-20261006.
Retry archive SHA2560bac35b5818bdf5414216c32477244bb69f0e67961a65669046ab1c836d03fc1; log SHA256a12e0a61cd5f0ce1f758f9f1faafda37c80247d75f9888f0a6bcd7f645961e67.
The source archive records every file mode and hash; all inputs remain unchanged.

The NPC reader changes are a separate commit with the same combined qualification.
This mode repair solves packaging; full producer/cold-recovery/activation and
R1–R8 qualification remain open. Accounting remains inactive and release BLOCKED.
