# Experimental accounting review checkpoint

## Current combined source qualification — 2026-10-05

[Current source and evidence](PLAN1_CURRENT_COMBINED_QUALIFICATION_2026-10-05.md) supersede earlier installation/build status.
Reviewed native/schema61 is installed; both production builds and the reviewed54
both-engine audit pass. Latest original-diagnostic qualification passes both engines; SHOP74
rerun remains in progress. Restore fixture milestone3e93d4cb8 is pushed. Private
quest ownership/root work continues; no full plan, R1–R8, release or activation
completion is claimed. Original requirements and inactive gates stay.

Date: 2026-09-30. Review branch: `Community-Duris/Duris:experimental-accounting`.

**This branch is an implementation checkpoint for public code review.
Production release and accounting activation remain blocked.**

## What is saved

The branch includes the source history after the community `master` baseline,
the `main` save/publication/item-placement refactor, and the current accounting,
save, quest-reward, death-custody, journal quarantine, flatfile receipt, and
staging schema preparation. Focused regression fixtures and migration adapters
are intentional source files. Writer registries and coverage matrices are
maintained, reproducible contract inputs, not runtime dumps.

The [active plan](FINISH_ACCOUNTING_PLAN.md) is the current source of outstanding
work. [Implementation history](FINISH_ACCOUNTING_IMPLEMENTATION_HISTORY.md)
contains dated results from earlier checkpoints; historical passing results
must not be read as qualification of every current route.

## Review priorities

1. Save/journal admission, acknowledgement, replay, quarantine, and PID fences:
   preserve pending durable intent and refuse unsafe publication after failures.
2. Quest XP, item/cash delivery, and spell-effect receipts: exact operation and
   recipient evidence, native transaction boundaries, and recovery after lost
   completion replies.
3. Death/corpse custody and ownership repair: preserve original payloads and
   linked evidence, keep ambiguous cases held, and avoid fabricating historical
   accounting during a future witnessed opening.
4. Immutable migrations: preserve the staging 0045 fork, atomic receipt/state
   recording, persistent advisory locks, and the two exact runtime histories.
5. Accounting coverage: distinguish lexical inventory from executable writer
   proof, and independently reconcile native balances and item custody.

## Known blockers

- The release validator still refuses missing executable writer evidence.
- SQL quest recovery now has fixed-cost native witness reads and complete load
  metrics, qualified on disposable MySQL/MariaDB workloads in the
  [review continuation](REVIEW_CONTINUATION_2026-09-30.md). Integrated concurrent
  publication/save recovery and full gameplay workload qualification remain open.
- Full-world clone login/save and integrated disconnect, database interruption,
  copyover, death/corpse, and compound economic journeys remain open.
- Native monetary/origin/UID audit coverage, flatfile parity, and lifecycle
  qualification remain incomplete.
- Production ownership repair needs two independent verified backups, complete
  protected cross-backup/journal cases, reviewed dispositions, clone rehearsal,
  and a new immutable accounting opening. No repair or activation has occurred.

## Validation and evidence boundaries

Before this packaging checkpoint, both maintained production builds and selected
SQL/flatfile quest crash/recovery journeys passed. Disposable MySQL/MariaDB
migration and session-fault fixtures passed. A privately captured staging
generation passed clone migration, copied native recovery-file validation, and
minimal isolated SQL service boot/shutdown. See
[the public staging preparation summary](../../operations/STAGING_SQL_ROLLOUT_PREP.md)
for the qualified scope and remaining deployment gates.

This checkpoint excludes local environment files, SSH keys, live credentials,
database dumps, player/account records, journals, quarantine archives, case packets,
build binaries, test logs, and scratch qualification reports. New staging
rollout notes retain exact operational coordinates, incident names, and captured
backup identities in protected local evidence. Legacy area-editor build outputs and boot logs have
been removed from the branch's tracked tree and ignored; local copies remain.

Publication checks cover the staged source, new branch history, and artifact
paths. Gitleaks 8.30.1 found no secrets in the staged changes. Manual review of
the history/full-tree findings identified only WebSocket protocol examples and
disposable test constants. The 22 immutable-runner and ten runtime-compatibility
tests passed again on Linux; changed-line formatting and diff checks passed.
The normal accounting validator, generated writer-matrix check, and offline
runtime-schema validator passed; the release validator remains a known failing gate.

Existing public deployment documents and their history already contain server
addresses, service names, and operational paths. This checkpoint does not rewrite
that public history. Those coordinates must be refreshed privately before use.
No live access secret was identified by this publication review. These checks
reduce disclosure risk and do not constitute an independent security audit or
production release approval.
