# Typed SQL bank admission and replay

Status: focused local qualification passed on `codex/474-phase5-bank-admission`, based on
[PR #603](https://github.com/Community-Duris/Duris/pull/603). Partial #476/#480 delivery.

This increment reuses the bank-only transport portion of `a89fa8f18`. The
coordinator accepts an optional pure extension validator, shared by fresh submit,
publication-retaining submit and durable journal replay. Existing callers default
to no extension support. Failed initialization and shutdown clear registration.

The bank validator regenerates the typed frozen intent from its retained lineage,
epoch, wallet/bank lifetimes and canonical command, requiring exact byte equality.
It neither authorizes a new mutation nor consults current activation state. The
SQL owner still verifies retained receipts first and checks current authority only
for new operations. Frozen commands are not renormalized after binding.

SQL startup pairs this validator with the existing pooled root. The pool admits
only legacy commands and structurally valid bank envelopes, and retains existing
connection replacement and original-ID reconciliation after ambiguous commits.
The direct bank owner performs the transactional checks described in
[SQL_BANK.md](SQL_BANK.md). Unsupported nested SQL paths remain closed.

Flat-file startup registers no extension validator. Its current owner has no
accounting transaction support, so accounting journals stop initialization without
being applied or checkpointed. No coin adapter, flat-file dispatcher, gameplay
producer, baseline or activation is imported by this change.

## Verification boundary

- Pure transport tests in both compilation modes exercise explicit registration,
  malformed/mismatched bank intent, unsupported types, same-ID attachment/conflict,
  unresolved fences and durable replay. Owner doubles do not prove native storage.
- SQL pool tests use real disposable database connections for fresh apply, replay,
  lost commit reply and replacement-connection reconciliation. Retired authority
  replay must preserve the original receipt and exactly one financial posting.
- Existing coordinator and default-closed mixed-journal tests protect legacy
  behavior and refusal without forwarding/checkpointing.
- Both server builds qualify startup registration and linkage. A complete gameplay
  wallet/bank journey still requires baseline, selected-backend storage and final
  native publication/save acknowledgement.

Local results: all checks above passed, including MySQL 8.0.46 and MariaDB
10.11.14 pooled bank ASan/UBSan runs, both full server builds, timestamp-zero
admission and explicit coordinator publication acknowledgement. Hosted checks and
review remain pending. Native gameplay publication is not established by these tests.
