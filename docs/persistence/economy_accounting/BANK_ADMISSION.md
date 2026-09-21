# Typed bank admission and replay

Status: SQL admission was delivered in PR #604 on PR #603. The current flat-file
follow-up is based on [PR #607](https://github.com/Community-Duris/Duris/pull/607)
at `790665585`. These are partial #476/#478/#480 components; gameplay activation
and restart-safe publication remain open.

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

The flat-file follow-up on PR #607 now pairs the same bank-only validator with
`flatfile_accounting_apply_selected`. Schema-1 commands keep the selected legacy
repository path and context. Schema-2 bank commands keep their original envelope
and go to the typed native bank owner, using an explicit root or the configured
flat-file root. Unsupported schema-2 types return retryable ENOTSUP without legacy
fallback. The coordinator refuses unsupported durable envelopes during startup
without checkpointing them. Default callers without registration still refuse
accounting. No gameplay producer, baseline or activation is enabled.

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

## Flat-file publication boundary

Native bank effects and retained receipts can be reconciled exactly once through
the coordinator after restart. This does not prove restart-safe live publication.
`enqueue_replayed` currently value-initializes `retain_until_publication` to false;
the journal does not persist that opt-in flag. A replayed command may therefore
checkpoint after its durable owner result without waiting for a live publication
acknowledgement. Fresh `submit_for_publication` still holds fences until explicit
acknowledgement. The native dispatcher tests distinguish these behaviors.

Restoring the required publication/save acknowledgement across restart is an open
requirement of the wallet/bank gameplay deliverable, before activation. The
standalone bank owner and this dispatcher must not be reported as a complete
player journey or as completing #474, #478, #479 or #480.

## Flat-file focused evidence

The dispatcher ASan/UBSan test passes in SQL and client-free compilation modes,
including configured/explicit roots, full result and failure-stage forwarding,
and refusal of unsupported families without legacy fallback. The native
coordinator/bank sanitizer journey proves assigned timestamp preservation,
exact-command retained replay after committed-but-unacknowledged shutdown,
unchanged balances/revisions, fresh acknowledgement retirement, and durable
unsupported-work refusal without execution or checkpoint. Its explicit replay
auto-retirement assertion records the publication gap above, not completion of
that requirement. Existing pure admission tests pass in both modes, and the
legacy flat-file gates still reject schema-2 calls without native file changes.
The source census and changed C++ formatting pass. Full build/boot evidence is
reported with the increment's PR.
