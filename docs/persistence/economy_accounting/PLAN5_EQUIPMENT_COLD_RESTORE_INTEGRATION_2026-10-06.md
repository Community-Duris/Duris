# Plan5 cold-restore slice and primary registration integration — 2026-10-06

The [equipment cold-restore qualification](PLAN5_EQUIPMENT_COLD_RESTORE_QUALIFICATION_2026-10-06.md)
is committed separately as `8a9b4c844c130b4d28910d6a97f59528cb84f1ec`, over
`f0b6bb3e4472e09aa7b52cbc73e75336d00bcda3`. It adds maintained read-only audit
coverage to the original native baseline recipe without changing production
implementation, native contracts or schema. Its whole original native recipe
passes on MariaDB10.11.14 and MySQL8.0.46 with fresh SQL/client-free sanitizer
builds, zero skips, eight equipment captures, four extra cold dump/imports and24
bounded CLI observations. The report preserves its modeled live positions,
partial-capture findings and all earlier failed attempts.

Refreshed primary `c00f1868174ea3481ec39b4f533f0063a5b2a658` is normal-merged as
`03f18626ef24b38614bc85400ed0b8e3193550ef`. No conflicts or independent edits to
primary-owned files occur. The merged native/migration/scripts/tests differ
from the qualified slice only in the primary's new equipment manifest entry.
All3080 remaining code/test input hashes match the successful frozen archive.
Native tree `bf7a92a728ad9b5b813626462e56533f8ba39c97` and migrations
`2eb9da7bf64bcd86e05f85d2f4bdf60ef113962d` remain exact canonical61.
The owned follow-up commit containing this report changes documentation only;
its result SHA and remote delivery are authenticated after publication.

The primary imported the earlier Plan5 equipment fix byte-exactly, and added its
original seven-case fast/unittest manifest entry. All existing regression fields,
including UID minimum11, remain identical. Complete discovery now accepts918
entries. All105 integration rows are unchanged, including the original
`native_sql_baseline_claim_audit` owner, required case/marker, self-SQL provider
and900-second central timeout. No further registration change is requested for
the new helper, which executes inside that original recipe before its final
qualification marker.

Actual post-merge Windows commands, with protected outputs under
`D:\CodexEvidence\accounting-plan5\bin\equipment-cold-primary-integration-20261006`:

```text
python -B tests/run_regression_tests.py --profile fast --match test_item_equipment_reconciliation --jobs 1 --report D:\CodexEvidence\accounting-plan5\bin\equipment-cold-primary-integration-20261006\central-equipment.json
python -B tests/run_test_entry.py tests/async/test_economic_sql_uid_scope.py unittest D:\CodexEvidence\accounting-plan5\bin\equipment-cold-primary-integration-20261006\central-uid.json 11
python -B scripts/validate_economy_accounting.py
python -B scripts/generate_economy_writer_coverage.py --check
python -B scripts/validate_runtime_compatibility.py
python -B scripts/validate_economy_accounting.py --release
```

The central runner executes all seven equipment methods; the original adapter
executes all11 affected UID methods. All18 methods pass, zero skips; retained
case IDs account for every collected case. Other entries excluded by the narrow
match were not executed and are not reported as passing. Complete inventory
acceptance is discovery evidence only. Normal, matrix and runtime metadata exit0;
release validation exits1 for missing executable writer evidence. It remains a
release gate. No fresh native/database run is claimed after this metadata-only
merge; the exact qualified bytes support composition at their original scope.

`tmp/plan5/equipment-cold-primary-integration.json` seals11 new artifacts, all
source correspondence, exact commands, manifest comparison and inventory.
SHA256 `9deec31546051bc69de8d39ff582e107e43c8cc8c2146de8312e9e9b3738b6e9`.
It also rechecks every one of149 previously sealed native artifacts against
`tmp/plan5/equipment-cold-restore-evidence.json`, SHA256
`80491d900fd6752872b8532e354dedb1522dbf9124936a90307799e2797eea5d`.
The post-publication delivery receipt additionally checks raw case records,
owned-file hashes, all earlier branch heads, clean worktree and exact remote tip.
Protected logs/dumps are retained locally and are not committed.

The earlier seven-method registration handoff is closed by the primary. No
shared interface or schema request is added here. A reviewed commit-watermark
contract remains necessary before implementing resumable audit sweeps; IDs or
timestamps alone do not establish a complete committed cut. Full native
holdings/UID/world authority, real producer/player journeys, final combined
backup/restore/retention/corruption/lifecycle and erasure evidence, flatfile
equipment parity, and original mixed-root release-host budget measurements
remain unqualified. This slice does not establish a complete capture, service
boot, managed backup generation, writer coverage, activation or release.

The worktree stays
`C:\Users\alexa\.codex\worktrees\accounting-plan5\NewDuris Max`, branch
`codex/accounting-plan5`. Earlier work and follow-ups are preserved on this same
branch through normal ancestry; all seven prior tips are verified by the delivery
receipt. Only that remote branch is published. Accounting stays inactive;
wallet-root exclusions and the declined inactive spell-path change remain.
Primary private birth candidates gain no native/release evidence from this
metadata integration. The primary's locally maintained shared notebook is
nonblocking, per the user. These owned reports and receipts supply the curator
handoff; shared notebook/checkpoint edits remain with the primary. Independent
Plan5 work can continue; full Plan/R1–R8 gates remain open.
