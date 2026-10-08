# Maintained Smith save profile and original save-body retention

2026-10-08. This milestone applies two independently source-reviewed files to
maintained source, rather than promoting the broader unfinished private candidate.
Smith's private preparation checkpoint previously retained filtered inventory,
level and an ACK revision but lacked the exact original queued save body required
by its persistence participant. The maintained pipeline previously had no Smith
profile. It now has a distinct private SQL Smith profile and retains the original
enqueue/coalescing body in its existing checkpoint slot.

## Behavior

Only the original Smith owner can begin, poll, hold, cancel or observe this profile.
The original runtime generation is resolved before remembered actor dereference.
The profile retains filtered EQ/INV and level, requires the existing matching
captured/completion/durable revision checks, and binds a hold to the original
operation. Cancellation can consume only an unheld preparation checkpoint.

The body is encoded after original receipt merges and installed only after the
queue owns the snapshot. Its allocated capacity, plus the existing slot payload,
is charged under the unchanged shared 32 MiB checkpoint limit. Held observation
checks PID/revision, filtered inventory/level and canonical body round trip before
changing outputs. Other save profiles keep their existing component and body
policies; inactive accounting and the declined inactive spell path are unchanged.

The returned body is the original enqueue/journal capture. It is not proof that
every component in its original mask was applied by that particular worker ACK:
existing receipt-free worker mask narrowing remains unchanged. The profile does
not authenticate a full physical forest, fresh SQL rows, source, native history,
factory effects or publication. It supplies no held release, submission or guarded
ACK capability. No gameplay route or activation gate is opened.

## Source evidence

Reviewed manifest:
`5ea6dcaa7140339fbf41e763fb468b7c874257c71d38728bb0732723393e0e70`.
Independent persistence review passed against the actual maintained preimages,
103 current provider pins, both output bodies and 14 exact forward/inverse deltas.
The port imports no private flat, reset or SQL participant implementation.

Applied source bodies:

- `src/player/player_save_pipeline.h`:
  `ddd0b92c6f29fe9a4caf369171dc0dd253db69e67e09b10cf06c6cc04753a523`.
- `src/player/player_save_pipeline.c`:
  `884282f4ae696c3bf6e11917e3bf8b43801ced3a29f6b6d4103cf9da51c2fe83`.

Root application authenticated current source preimages/providers and exact output
hashes, preserving all three unrelated worktree changes. Source formatting,
token/inverse/hash checks and Git whitespace checks passed. Local evidence is
`bin/tests/smith-maintained-save-profile-primary-20261008/SOURCE-APPLICATION.json`;
the private review packet is
`tmp/smith-maintained-save-profile-primary-20261008/`. These are local evidence,
not committed runtime artifacts.

## Qualification and remaining integration

Compilation and executable testing remain deferred under the user's instruction
until actual major-plan readiness. Required checks then include real queued and
coalesced capture, receipt merges, capacity refusal, stale ACK, actor replacement,
unheld cancellation/held refusal, journal recovery and integrated SQL/flat journeys.
No test execution, database operation, schema change or production action ran for
this milestone.

The larger private candidate remains unpromoted. Root still owns complete Smith
source/native-history/full-physical/save cross-proof, retained recovery budget,
combined SQL/flat admission and commit, actual factory/fee/grant/extraction
publication and guarded ACK. Smith, full Plans 2-4, combined Plan 5, R1-R8 and
release qualification remain incomplete. This source milestone does not change
the historical scope of recorded Plan 1 acceptance.
