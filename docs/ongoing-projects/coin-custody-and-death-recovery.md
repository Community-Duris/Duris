# Coin custody and death recovery: two-hour recovery plan

Date: 2026-09-06. Priority: P0. Status: previous implementation trajectory stopped;
bounded recovery effort replaces the open-ended plan. No fix deployed.

## Time limit and scope

The effort started at **09:13:32 UTC on 2026-09-06**. The hard stop is
**11:13:32 UTC**, including assessment, implementation, verification, and reporting.
Updating this document, interruptions, failed checks, and incomplete work do not
restart the clock or extend the deadline.

The required outcome is a **complete, reviewable fix for the coin-custody bug and
the resulting player death trap**. Coin put, merge, and get must conserve value
and maintain custody. A rejected corpse transfer must preserve assets durably and
release the player from the death transition, with successful re-entry.

This time limit does not guarantee that the complete fix fits. If the assessment
shows that it cannot fit, report that immediately with concrete reasons. Keep the
requirements intact; do not substitute a partial fix or spend two hours discovering
that the essential work is infeasible.

| Window | Deadline (UTC) | Required work |
| --- | --- | --- |
| Minutes 0–15 | 09:28:32 | Inspect the actual diff and runtime paths; give the feasibility assessment. |
| Minutes 15–90 | 10:43:32 | Implement the smallest complete fix and remove unnecessary changes selectively. |
| Minutes 90–120 | 11:13:32 | Reserve this entire window for the build, focused runtime verification, and final accounting. |

Report progress at minutes **20, 40, 60, 80, and 100**, then deliver the final
accounting by minute **120**. Each report must state progress against the concrete
coin/death/reconnect outcome, actual evidence, remaining failures, and time left.
Do not present foundation-only milestones as near-completion.

## First 15 minutes: audit and feasibility decision

Inspect the working diff, including untracked files, rather than relying on prior
checkpoints. Trace the command, completion, custody, corpse-transfer, terminal-save,
and re-entry paths. Check the existing runtime harnesses and their substitutions.

Classify changes as:

- **Reuse:** working changes necessary for the complete coin/death outcome, with
  evidence identifying what their tests actually prove.
- **Remove:** unnecessary or superseded changes that expand scope without helping
  that outcome. Remove individual hunks; preserve unrelated work.
- **Finish or replace:** unfinished changes that leave a required path incomplete
  or unsafe. Do not retain machinery solely because time was already spent on it.

Give a short assessment by minute 15 stating what can be reused, what should be
removed, the essential remaining edits, and whether implementation realistically
fits before minute 90 while leaving the final 30 minutes for verification. Give an
earlier negative feasibility decision as soon as concrete evidence establishes it.
Identify the missing behavior, integration, or verification that makes it infeasible;
do not silently reduce requirements. Any work continued within the deadline must
be reported honestly as incomplete until the full outcome is proven.

Existing work is a candidate for reuse, not an approved architecture. Prior
checkpoints report atomic coin backend work, command adapters, loader corrections,
and focused tests. The death-record codec and journal work are preparatory;
their passing tests do not prove bounded death recovery. At the scope reset,
the death disposition handlers and their gameplay integration remained unfinished.
Revalidate these facts against the actual checkout before making the decision.

## Implementation constraints

Reuse existing persistence, transactions, terminal-save/journal, and recovery
mechanisms. Add no framework, service, queue, or general-purpose abstraction.
For each proposed storage field, identify the exact information needed for recovery
that existing records cannot preserve; add only that information.

Make necessary implementation decisions autonomously within this scope. Preserve
unrelated work. Never reset or overwrite entire source files containing other
work to remove a subset of changes.

Concentrate on the actual failure. Broader producer audits, unrelated command
rewrites, new infrastructure, production repair campaigns, and deployment work
are not independent deliverables in this effort. Touch adjacent paths only where
necessary to make the requested coin/death behavior complete and avoid introducing
loss, duplication, or a regression.

Do not disable integrity checks, guess-delete ownership rows, permit duplication,
suppress the failure message as a substitute for recovery, or repeatedly retry an
unchanged integrity rejection. Do not deploy or modify production data.

## Essential behavior

### Coin put, merge, get, and bag movement

Use the existing transaction machinery so wallet changes, the final pile payload,
and custody changes commit consistently. A new pile receives custody; a merge
preserves its UID and durably stores the combined amount. Full pickup retires the
consumed pile's custody; partial pickup preserves its UID and exact remainder.
Moving the bag must retain correct parent/root relationships without a phantom
coin descendant.

Before commit, failure leaves source money and objects unchanged. Resolve an
uncertain commit by its existing operation identity before retrying. Publish the
committed result exactly once, including across disconnect/re-entry; no speculative
refund or second debit. Preserve existing capacity and permission rules and reject
arithmetic overflow without truncation or loss.

### Death and rejected corpse transfers

Use existing durable recovery mechanisms to preserve the final character state,
death consequences, corpse identity/location, wallet disposition, complete affected
item payloads/UIDs, and unresolved custody evidence. Identify any genuinely missing
record fields during the audit rather than assuming a new record design is needed.

An integrity rejection must leave disputed assets durably recoverable outside the
player's active inventory, release the player to the account menu, and permit normal
re-entry and unrelated gameplay. With the local durable journal available, the
acceptance target is release within **two seconds of terminal item rejection**.
Measure it. A serial transfer chain or repeated unchanged rejection must not keep
the player dead indefinitely.

Death consequences, wallet conversion, corpse creation, and item disposition must
apply at most once through retries and reconnects. Preserve the entire wallet;
clamping denominations and then clearing all cash is unacceptable. A missing live
corpse must not permit an empty terminal save to discard untransferred assets.
Ambiguous custody remains preserved for specific repair, not guessed away.

If both the database and durable journal are unwritable, report and test that
storage failure boundary honestly. Preserve live state; do not claim durable
recovery or silently leave an individual player trapped as ordinary failure handling.

## Final 30 minutes: build and focused runtime verification

Stop expanding implementation at minute 90. Run formatting checks and
`make -C src`, then verify the actual sequence with synthetic characters and
isolated storage:

```text
put coins in bag
merge more coins into the same pile
get coins from bag
move bag
die
reconnect
```

Exercise real command, completion, death, and re-entry paths. Assert money totals,
pile amounts and UIDs, persisted payloads, custody/root/parent records, corpse
contents, death consequences, and successful re-entry. A hand-built repository
command or a source-contract assertion is supplemental evidence, not proof of the
complete gameplay sequence.

Inject the original corpse-transfer rejection: a captured subtree and its custody
rows disagree, producing `EMSGSIZE`. Verify that the integrity check still rejects
it, affected payloads and ownership evidence survive durably, death exits within
the stated bound, and reconnect does not duplicate assets or trap the player.
Check persistence after restart/replay where required to substantiate preservation.

Check **MariaDB and flatfile** to the extent needed to substantiate the claims.
Record which backend and real paths each test exercised, any fixtures or mocks,
and all failures or checks that could not run. Include focused failure-before-commit,
replay, and capacity checks where necessary to establish conservation and recovery.
A passing build or isolated helper test cannot justify a broad success claim.

The final window stays reserved for build and verification. Diagnose failures and
report them; do not use a failure to extend the deadline or begin another design.

## Hard stop and final accounting

At **11:13:32 UTC**, stop work. Do not continue open-ended execution because a goal
remains active or requirements are unfinished. Leave a reviewable diff and report:

- Completed fixes, retained changes, and changes selectively removed.
- Exact build/test commands, actual results, backend coverage, measured recovery
  time, and evidence for the required gameplay sequence.
- Remaining failures, incomplete paths, and unverified behavior.
- Deployment readiness, including an explicit statement if the fix is not ready.

Declare success only if the requested coin/death behavior is actually proven.
Otherwise state that the effort ended incomplete. No deployment or production-data
modification is authorized by this plan.

## Incident evidence to preserve

The original inspected revision was `06c623d784322adf243cf1465af2076d4868610f`.
Feric's death repeatedly produced `corpse/rejected_preserved`, error 90 (`EMSGSIZE`):
the command described 74 items while the authoritative subtree initially returned
75. A coin-object row was present, but the exact omitted UID was not captured;
that identity must not be guessed when interpreting or repairing the evidence.

The owner confirmed that **a staff character resurrected Feric**. His recovery was
staff-assisted, not automatic. Successful login does not establish that the original
inventory was preserved or that the underlying defect was repaired.
