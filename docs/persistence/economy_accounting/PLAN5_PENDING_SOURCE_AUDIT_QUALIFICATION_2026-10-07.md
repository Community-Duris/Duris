# Plan 5 managed source-audit ordering qualification

The original managed lifecycle journey seeded a pending authority transaction
and then expected the online read-only economic audit to accept its source.
The audit correctly refused. This separate slice repairs the stale test ordering
while preserving that reader safety boundary and the complete managed recovery,
boot, receipt and retention assertions. The full method now passes.

## Exact source and ownership

Work and remote publication remain `codex/accounting-plan5`, worktree
`C:/Users/alexa/.codex/worktrees/accounting-plan5/NewDuris Max`.
Base is `d2e979a9ebdf5ec7905aaa6fa3e55385ec6cf149`, the separately committed/published
[source-provenance repair](PLAN5_AUDIT_INPUT_PROVENANCE_QUALIFICATION_2026-10-07.md).
Result and remote SHAs, canonical source comparison and all seven earlier tips
are bound by `flatfile-pending-audit-delivery-01-20261007/delivery.json` under
`D:/CodexEvidence/accounting-plan5/bin/`.

Tested archive SHA256 is `401dd5e712928694d89a72058471250313463afa92c9f3c4b2daabf8d792876b`. Exactly one code
payload overlays the base: `tests/async/test_persistence_backup_integration.py`.
This report and the owned remote follow-up are publication files. All other
code payloads, original modes and four links remain unchanged. Native tree is
`4abb609524a1f1682ea4c190f82d75003c4d679b`; migration tree is `1d041c8bc27cfc2b2bfdc8095b6c1348ac3a15c5`, canonical
source0062. No shared schema, coordinator, contract, producer, runner,
registry/matrix or activation file changes; no shared interface request remains.
Latest refreshed primary is `36e8f6ad78ef027851c1da809acca7570dcd95d6`, with a distinct native tree. This
qualification does not qualify the primary's combined candidate.

## Established failure and complete scoped repair

On the preceding frozen source, the original managed method fails at its initial
`audit(live)`, before capture or server boot, with `BackupError:subprocess_failed`.
Its original source deliberately contains `.critical-authority-transaction`.
The `--economic-evidence-audit` route acquires the independent read-only authority
lock, whose `no_pending()` refuses that unresolved transaction. The original
failure log and `completed:false` certificate remain intact in
`flatfile-audit-inputs-full-01-20261007`; their seal SHA256 is
`0b058da8044679d9bf7fc9ab3c344411a558164cce481bb8033396fea8c2b613`. This is a stale qualification
expectation, not a reason to weaken the online audit or recover its source.

The journey now checks the healthy inactive source first, then seeds its original
WALs and pending transaction. A direct native audit must return1, empty stdout
and exactly `native_restore_qualification_failed` on stderr. Complete retained
state metadata/content and both source journal inventories must remain unchanged.
The transaction stays pending for the actual backup manager to capture.

All prior capture, transport, required-file discovery, candidate recovery,
service boot, cold restart, dedupe and pruning assertions remain. Only the manager's
fresh copied `ISOLATED_RESTORE` candidate performs recovery. The private outcome
certificate adds `pending_source_online_audit_refused_read_only:true`; existing
fields and all14 source-guard hashes remain. No audit findings are corrected.

## Original commands and executed outcomes

The immutable image is
`sha256:13d9e3ccbd77e8e4432f3f2647c54ccfbdd7c83a83585346b5527077c2f1e32a`.
Execution is network-disabled, two CPUs/4GiB, fresh3GiB workspace and2GiB
temporary tmpfs. SYS_ADMIN and unconfined seccomp support the original private
tmpfs and isolated user/network/PID namespaces. No production environment,
credentials, existing database or game is supplied. Native cache is off;
qualifier and both fixture builds compile fresh with their original C++20 strict
and ASan/UBSan recipes. Original recovery/boot/process deadlines remain intact.
The new source-refusal probe has a45-second outer limit over the original
30-second audit budget.

```sh
PYTHONPATH=/workspace/tests/async PYTHONDONTWRITEBYTECODE=1 \
python3 -u -B -m unittest -v test_backup_review_remediations
DURIS_RUN_BACKUP_INTEGRATION=1 \
DURIS_PLAN5_LIFECYCLE_BACKUP_ARTIFACTS=/workspace/bin/tests/pending-audit-managed \
python3 -u -B -m unittest -v \
  test_persistence_backup_integration.FlatfileLifecycleRecoveryIntegration
```

Native environment retains `ASAN_OPTIONS=detect_leaks=1:halt_on_error=1` and
`UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1`.

| Original command | Actual result | Seconds |
| --- | --- | ---: |
| Backup review regressions |18 methods pass,0 failures/errors/skips |4.978525 |
| Complete managed lifecycle method |1 method passes,0 failures/errors/skips |197.121101 |

The healthy source audit accepts; the pending source audit refuses read-only.
The actual backup/restore managers preserve the pending native transaction and
journals in the generation, recover the fresh candidate, drain both journals
and complete two actual isolated server boots. Original critical dedupe and
native player-revision/status assertions pass. Cold restart preserves old
inactive receipts and validates the UID high-water advance. Policy rotation
retains two generations and prunes only the unretained oldest generation.
Both manifest loss/corruption cases, a checksum-valid corrupt receipt and a
required receipt missing before capture refuse before boot. Source/generation
preservation and all14 terminal source-guard comparisons pass. The final native
certificate has `completed:true`, one full outcome and `full_R8_qualified:false`.

The server actually booted is retained binary SHA256
`f7f0066bc23663a63c42250e81afad31e7b18e18435959103cd5980f979febf4` from
`flatfile-namespace-flatfile-build-01-20261007`, its original740 fresh units and
archive `2eba999163a7a773ebf3a02fde95f4b2639ec92d96e212bdd826885370d5a36c`. Its exact native
tree/payloads/modes and binary hash match this candidate and are bound through
the prior seals. No fresh production build is claimed for a Python-only test
ordering change. The earlier lifecycle131/marker69 commands retain their
preceding slice attribution; none of their consumed inputs changed, and they
are not relabeled as newly executed here.

## Evidence, remaining gates and curator packet

Protected directories are `flatfile-pending-audit-full-01-20261007` and
`flatfile-pending-audit-gates-01-20261007`, under
`D:/CodexEvidence/accounting-plan5/bin/`. They retain exact archives, command/env
records, complete logs, fresh qualifier/fixture binaries, managed native
certificate, two actual service logs and cold-restart state delta. Source bytes
are checked before and after execution. The full process and artifact-copy
wrapper have exited before sealing.

Seal `flatfile-pending-audit-seal-01-20261007/evidence.json`, SHA256 `6504b112017748acc82be45f4e22fb8bdee8e614eab2eb4967873e3112f08151`,
binds 27 artifacts/571,400,062 bytes and references
the preceding genuine failed journey without altering it. Separate preflight
and delivery receipts compare every nonpublication payload/mode/link against
the canonical result and verify the remote SHA plus preserved ancestry.

`git diff --check`, normal accounting validation and all55 writer contracts pass.
Release validation still correctly exits1 for `writer has no executable evidence`.
No C/C++ formatting/build or SQL database method is repeated: this change owns
only flatfile qualification ordering, with unchanged native/SQL/schema inputs.
No selected method is skipped. Earlier SQL results keep their original source
attribution; this does not constitute a current MariaDB/MySQL qualification.

The stale managed ordering is resolved. The evidence covers modeled inactive
native-codec origin history; source capture and lifecycle installation are not
executed. Full current holdings/UID reconciliation, whole forward/orphan closure,
native producer/recovery/ACK, both-backend genuine gameplay, release-host budgets,
complete trusted backup/replica/retention/erasure proof, R7/R8 and the primary's
published tested combined candidate remain open. This component pass is not
release completion. No new engineering blocker or shared request remains.

Accounting stays inactive. Wallet-root item exclusions and the declined inactive
spell path remain exact. No production mutation, source recovery, audit finding
correction, activation, deployment, PR merge or independent experimental-accounting
push occurs. This report, the remote follow-up and receipts are the curator packet
for the primary's locally maintained notebook. Notebook upkeep is nonblocking;
its application, direct notification and primary acknowledgement are not claimed.
