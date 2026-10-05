# Baseline component collector linkage — 2026-10-05

The original Plan 1 component recipe failed to link because command admission
calls `collector_purchase_accounting_decode` without its real pure providers.
Append `collector_accounting.c`, `collector_command.c`, `collector_policy.c`
and `collector_codec.c` after the original ordered 25-source prefix. This changes
only `tests/async/test_economic_baseline_adapter.py`. The original test body,
EAB1 reference vectors, both modes, allocation wrappers, compiler and sanitizer
flags, and 120-second native execution limit are unchanged. AST removal of the
four appended source constants reproduces the original module exactly.

## Native evidence

The first original attempt failed after 216.510 seconds at the unresolved
collector decoder. Retained stderr SHA256:
`be7c7424748a4fb7bcf26aafe8e8deb1582bcefcc38d8e216f160bfc73448617`.

The repaired original recipe compiled and executed **both SQL and client-free
native modes successfully**, with AddressSanitizer and UndefinedBehaviorSanitizer.
Total original owner duration was 518.455 seconds inside the unchanged 600-second
outer bound. It exercised baseline preparation, admission refusal, exact replay,
reference bytes, corruption/truncation/order/budget cases and allocation failures.
The pending EAB2 candidate's equipment, position and regenerated-plan cases also
passed; the historical EAB1 controls stayed intact.

| Retained artifact | SHA256 |
| --- | --- |
| SQL-mode native binary | `790c98182cbdb0e3ee4ec40bfacdee4cdce79c1048381fae3f653ea07d961706` |
| Client-free native binary | `a9aa476f423fce864a7c3fd8d7bb4b3eba46b2958779d78b88b7a2e52c5ef516` |
| Original owner stdout | `2cd900bf5ca089e79f9839a6eb0c9fdc7803091dcd3e4f7d261bdc2c48ff1ec1` |
| Frozen 3082-file source manifest | `00da4ead4f127705b13a83b2346318247c8675e800fd0ad543c0246f4bcc08ef` |

Execution used the frozen reviewed EAB2/schema61 candidate over `b754e2962`,
plus this recipe and the literal registry input. Those pending native/schema
changes are **not installed by this commit**. SQL mode here describes the native
build configuration, not a SQL service, gameplay journey or maintained server
build. Private commands, binaries and logs remain under the owned
`bin/tests/plan1-eab2-components-tools-20261005-3900d2bf80f5` attempt. The exact
owned tools runner was removed after the batch returned.

## Remaining original qualification

The next unchanged flatfile owner compiled but failed its initial 17-mutation
assertion after 182.006 seconds; the entire component batch did not pass.
Independent native EAB2 audit on both actual engines reached corruption cases,
then leaked `EvidenceError` from a reserved-position-padding cut instead of its
controlled origin refusal. The genuine historical C05 owner separately failed
with SQL error 1054: the retained v12 source snapshot refers to `item_condition`,
which immutable migration 0043 adds after its required schema35 prefix. Preserve
both failures; do not weaken cases or apply unrecorded later DDL to prefix35.
Use the coherent original C05 introduction source for its next bounded attempt.

The previously measured schema61 fingerprints and fresh full runtime checks on
MySQL 8.0.46 and MariaDB 10.11 passed at their recorded candidate scope. Populated
historical upgrade, complete native baseline/flat recovery, independent audit,
writer/player journeys and all remaining original Plan 1 gates are still open.
This solved recipe linkage issue does not complete Plan 1, R1–R8, coverage or
release. Current inactive behavior, safety gates and activation refusal remain.
