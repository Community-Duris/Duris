# Community spell-up controls and boot-lifetime repeat mode

**Status: design proposal only; not implemented.** All new syntax below is proposed.
This document does not enable a timer, change spells, or authorize deployment.

## Goal

Extend `newbsa` so an authorized immortal can inspect and customize a useful spell
bundle while online, cast it once, or repeat it until stopped or the server
restarts. Make the selected effects, targeting, lifecycle and actual results
clear without requiring an external client timer.

Keep the first implementation small: one supported spell registry, per-immortal
in-memory selections, one server-owned repeat job, and bounded feedback.

## Existing behavior and compatibility

Source baseline: `5b2f406400c78ec24edca9c81c48c9666fad4224`.

- [`newb_spellup` and its command handlers](../../src/cmd/actwiz.c) invoke a
  hardcoded bundle at spell-level argument 61. `newbsa` accepts no faction filter,
  `g`, or `e`; it visits playing descriptors, excludes the caster and includes
  characters at level 60 or below. The helper logs each recipient and sends
  `Enjoy your blessings.`; the command reports a recipient count, not a count of
  effects successfully applied.
- [`interp.c`](../../src/cmd/interp.c) registers `CMD_NEWBSA` and `CMD_NEWBSU` at
  `LESSER_G` (59), using the existing command-grant mechanism. Preserve that
  authorization contract; do not grant permissions as part of implementation.
- [`spell_rest`](../../src/magic/spells.c) grants rested, upgrades rested to
  well-rested, or refreshes well-rested. The affected duration is set to 150 in
  each branch. Do not describe that number as wall-clock minutes without tracing
  affect timing.
- [`gain_exp`](../../src/world/limits.c) applies the rested/well-rested multipliers
  of 1.5/2 before subsequent applicable modifiers and caps, except resurrection.
  Repeating the default bundle can therefore maintain an XP benefit.
- [`spell_regeneration` and `spell_accel_healing`](../../src/magic/magic.c) are
  distinct effects. Both reject the regenerate skill or pactum serpentis; they
  do not directly reject one another. Verify actual combined regeneration
  behavior before documenting stacking or recommending a combined preset.

Preserve existing `newbsu <player>` behavior in this first slice. Preserve the
current `newbsa`, `newbsa g` and `newbsa e` defaults for immortals who have not
explicitly edited their selection. Do not broaden recipient eligibility,
change spell power, or silently change existing spell restrictions.

## Proposed command interface

```text
newbsa help
newbsa spells
newbsa add regeneration
newbsa remove rest
newbsa reset
newbsa preview [g|e]
newbsa [g|e]
newbsa repeat <interval> [g|e]
newbsa status
newbsa update
newbsa stop
```

- `help`: show syntax, current/default selection behavior and schedule lifetime.
- `spells`: show the caller's selected bundle and the supported additions. Show
  canonical multiword names; parse the remainder after `add`/`remove` as the name.
  Reject unknown or ambiguous names with suggestions and no mutation.
- `add`/`remove`: update only the caller's draft selection. Adding an existing
  entry or removing an absent entry reports a no-op. Keep canonical execution
  order rather than making user entry order silently change interactions.
- `reset`: restore the legacy default bundle in the caller's draft, not in an
  already running job. Drafts are memory-only and reset on logout/restart.
- `preview`: display the caller's exact draft, fixed spell level, scope, current
  eligible-recipient count and effect caveats. It must have no gameplay effect.
  Counts describe the preview instant, not guaranteed future recipients.
- Bare `newbsa` with an optional faction: cast the caller's selected bundle once.
- `repeat`: validate the complete draft, interval and scope; create a job from an
  immutable selection snapshot; cast immediately and schedule further passes.
  No-argument scope is all racewars, not just good and evil. Reject an empty
  selection. An existing active job is not silently replaced.
- `status`: show active job identity, creator, last editor, revision, spells,
  scope, interval, next pass, last result and stop reason when available.
- `update`: explicitly copy the caller's validated draft into the active job for
  the next pass. Preserve its interval/scope and announce the new revision. A
  running pass finishes its old snapshot. Changing interval/scope initially uses
  `stop` followed by a new `repeat`, rather than extra configuration commands.
- `stop`: cancel future work and report whether a pass had already started.
  Already-applied effects remain; stopping is not a dispel or reward rollback.

Use explicit human interval units, such as `10m`, with strict overflow/range
validation. Display the normalized interval. Establish a conservative bounded
minimum and maximum before implementation; do not silently clamp invalid input.
All control and detailed-status commands require the existing staff authority.

## Default bundle and regeneration

The initial selection is exactly:

```text
bless, spirit armor, barkskin, enhance armor, stone skin,
fly, haste, strength, agility, dexterity, accelerated healing, rest
```

The first additional supported entry is `regeneration`. It must invoke the
existing spell's behavior through a reviewed adapter, not invent a stronger
regeneration effect or remove incompatible effects. Retain the initial spell
level of 61; arbitrary spell levels are out of scope.

Use an explicit allowlist with canonical names, help text, application adapters
and outcome semantics. Do not expose every spell function merely because it is
marked nonaggressive. Do not accept arbitrary command strings, scripts, room or
object spells, or caller-supplied function identifiers.

## Preview and feedback

Illustrative proposed preview, not output from an implemented command:

```text
Community spell-up — PREVIEW ONLY
Targets: connected players, level 60 or below; faction: all
Caster: excluded; spell level: 61
Selected: bless, spirit armor, barkskin, enhance armor, stone skin,
          fly, haste, strength, agility, dexterity,
          accelerated healing, rest, regeneration
Rest: grants rested, upgrades rested to well-rested, or refreshes well-rested.
      This affects XP gains.
Regeneration: normal compatibility checks apply.
Nothing has been cast. To cast this selection now, enter: newbsa
```

Help and status must prominently distinguish draft and active bundles.
After an edit during an active job, say that `newbsa update` is needed to publish
that edit to the job.

Each run should summarize players considered/eligible and effects applied,
refreshed, upgraded, unchanged, skipped or failed, with compact typed reasons.
An eligible recipient is not proof that every spell applied. Preserve reasons
such as incompatible effects, invalid/dead target or target becoming unavailable.
Do not infer success by counting calls to void spell functions or parsing ANSI
messages. Adapt only the supported spells narrowly to expose truthful outcomes,
without changing other callers' behavior or creating a generic spell refactor.

Aggregate scheduled caster feedback instead of replaying every per-spell caster
message. Keep useful player effect messages/legacy one-shot feedback; avoid a
new global broadcast every interval. Detailed staff inspection is bounded and
private. Lifecycle audit records name the authorized creator/editor, selection
revision and start/update/stop outcome; they exclude raw commands and unrelated
player data. Observational telemetry being off must not break scheduling or
staff status. Avoid unbounded per-recipient log amplification.

## Repeat-job lifetime and safety

- Permit one active global job initially. Any currently authorized controller
  may inspect, update or stop it; report the existing owner rather than silently
  overwriting it when another immortal attempts `repeat`.
- Continue after the creator logs out. Store stable attribution, not a raw
  character/descriptor pointer. Eligibility continues to exclude the original
  creator's character; editing a job does not silently change that exclusion.
- Re-evaluate connected recipients and faction/level rules on each pass. New
  arrivals receive the next scheduled pass, not a separate login-triggered cast.
  Revalidate each recipient before applying effects; stale runtime identities
  must never target a different character after reconnect or extraction.
- Use the existing bounded event mechanism, explicit cancellation and monotonic
  deadlines. No overlapping passes, unbounded descriptor work in one pulse, or
  burst of catch-up executions after a stall. Large passes need bounded slices
  and a stable run revision, with exact-once handling within that pass.
- Stop invalid/revoked jobs safely and expose the reason. A creator's logout is
  not revocation. Define a bounded authorization-revocation check before claiming
  this behavior; never perform synchronous database lookups per target.
- Clear the job and drafts on cold reboot or copyover. No database persistence,
  serialized copyover job, or automatic restart is included. Stop cancels pending
  callbacks and prevents remaining not-yet-applied effects; no completed effect
  is reversed. Report partial work if stopping during a sliced pass.
- A scheduling/allocation failure must be visible and leave a coherent stopped
  state, not a status display claiming a nonexistent timer is active.

The current helper uses the immortal as caster for some spells and the recipient
as caster for others. A disconnected-owner job cannot simply reissue commands as
that character or replace every caster with the recipient. Define reviewed
per-spell application contexts and prove the intended caster-sensitive behavior.
Do not retain extracted characters or create a broadly privileged impersonation
mechanism. This is an implementation gate, not a solved property of this draft.

## Implementation slices and ownership

1. **Selection and truthful one-shot:** explicit registry, default-equivalence
   fixtures, narrow outcome adapters, parser, help, preview and editable drafts.
2. **Repeat lifecycle:** one boot-scoped service, safe casting context, bounded
   passes, immutable active revisions, update/stop/disconnect/copyover behavior.
3. **Verification and polish:** aggregate result reporting, lifecycle audit,
   actual-client journeys and operator documentation matching implemented syntax.

Likely touchpoints are `src/cmd/actwiz.c`, command registration/help, the selected
spell routines and a narrowly scoped scheduler module. Read current master and
active claims before allocating files. Coordinate shared `interp.c` work rather
than broadening another branch. Do not modify production, unrelated mechanics,
telemetry activation or balance policy in these slices.

## Acceptance and verification checklist

- [ ] Legacy default spells, caster-sensitive behavior, level, filters and
      single-player command are preserved by executable comparisons.
- [ ] Help, spell listing and preview exactly match the registry; preview and
      invalid edits have no effects; multiword/unknown/duplicate entries behave
      predictably; all entry points enforce authorization.
- [ ] Regeneration apply/refresh/block and accelerated-healing interaction are
      tested against the real spell functions. Rest grant/upgrade/refresh and XP
      implications are explicit, with no unrelated modifier changes.
- [ ] Staff selection edits do not mutate the job before update; update is atomic
      between run revisions; two controllers cannot start duplicate jobs.
- [ ] Tests exercise immediate first run, interval bounds, scheduling failure,
      clock stalls, bounded large recipient sets, extraction/reconnect, owner
      logout, authorization revocation, stop during a pass and duplicate callback
      defense. No dangling pointers, overlapping passes or catch-up storms.
- [ ] Restart/copyover clears schedules and does not replay them. Stopping leaves
      already-applied effects intact and reports partial/complete work honestly.
- [ ] Result totals reconcile to actual per-effect outcomes. Detailed output and
      memory/log growth are bounded; no raw command capture or public hidden-state
      disclosure is introduced.
- [ ] Run focused production-function and scheduler tests, relevant builds, and
      isolated multi-client gameplay journeys. Use local evidence rather than
      relying on GitHub CI or a live production experiment.

Before implementation, settle interval limits, exact safe offline-caster
semantics and the bounded revocation-check mechanism. These choices must not be
filled in by silently changing spell behavior or permission rules.

## Non-goals

Persistent presets/schedules, arbitrary spell execution, automatic login
blessings, player rewards/compensation, XP rebalance, and production activation.
A later UX pass may add named presets if the simpler selection workflow proves
insufficient; they are not prerequisites for this first slice.
