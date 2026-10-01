# Community spell-up controls and boot-lifetime repeat mode

**Status: implemented in PR 521.** The command is available through the existing
`newbsa` command registration at `LESSER_G` (level 59). The implementation keeps
the legacy default one-shot path intact and adds an allowlisted selection draft,
preview/status controls, and one boot-scoped server-owned repeat job.

## Goal and compatibility

The implementation is in [`community_spellup.c`](../../src/cmd/community_spellup.c),
with the legacy command entry point and one-shot helper in
[`actwiz.c`](../../src/cmd/actwiz.c). `newbsu <player>` is unchanged.

For a staff member who has not edited a draft, bare `newbsa`, `newbsa g`, and
`newbsa e` retain the existing default package, spell level 61, connected-player
filter, level-60 ceiling, faction filter, and creator exclusion. The untouched
default one-shot calls the original `newb_spellup` helper, preserving its spell
order, caster choices, audit lines, per-player messages, and `Enjoy your
blessings.` feedback. The new aggregate report is additional information.

The command remains registered at `LESSER_G`; the implementation also checks the
same level internally before accepting any subcommand. No new permission is
granted. No database tables, persistent schedules, copyover payloads, or spell
mechanic changes are introduced.

## Command interface

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

- `help` prints the complete syntax and the targeting/lifecycle rules.
- `spells` lists the explicit allowlist. Canonical multiword names are parsed
  from the complete remainder after `add` or `remove`; aliases and unique
  prefixes are accepted, while unknown or ambiguous names do not mutate a draft.
- `add` and `remove` change only the caller's in-memory draft. The execution
  order is the fixed registry order, so changing a draft cannot silently reorder
  interactions. Duplicate additions and absent removals are reported as no-ops.
- `reset` restores the twelve-effect default in the caller's draft. It does not
  alter an active repeat job until `update` is explicitly issued.
- `preview` reports the current draft, fixed level, faction scope, and the
  eligible count at that instant. It does not call a spell or mutate a player.
- Bare `newbsa` or `newbsa g|e` applies the current draft once. An empty draft is
  rejected.
- `repeat` validates the current draft, interval, and scope, applies the first
  pass immediately, then schedules later passes. The interval is a strict whole
  number followed by `s`, `m`, or `h`; accepted values are **10 seconds through
  1 hour**, inclusive. Invalid or overflowing values are rejected rather than
  clamped. The normalized interval is shown in status/start feedback.
- `status` reports the active job, stable creator/editor attribution, revision,
  selection, scope, interval, pass progress, next due time, and last run totals.
- `update` atomically publishes the caller's non-empty draft as the next job
  revision. A pass already in progress keeps its immutable selection/scope
  snapshot; the next pass observes the new selection. Interval and scope are
  intentionally unchanged by `update`.
- `stop` cancels future callbacks and reports whether a pass was partial. Effects
  already applied are not reversed.

## Targeting and effect registry

The default selection is exactly:

```text
bless, spirit armor, barkskin, enhance armor, stone skin,
fly, haste, strength, agility, dexterity, accelerated healing, rest
```

`regeneration` is the first additional allowlisted entry. The registry invokes
the existing spell functions at level 61 through narrow adapters; arbitrary spell
names, command strings, scripts, room/object spells, and caller-supplied function
identifiers are not accepted.

Each pass evaluates connected, playing PCs at level 60 or below and excludes the
creator's PID. The faction scope is all, good, or evil. The eligible target list
is capped at 2,048 runtime IDs and processed in slices of 32 targets. Additional
eligible players are counted as truncated in the report instead of causing an
unbounded event callback. Every stored ID is re-resolved and revalidated before
effects are applied, so extraction/reconnect cannot redirect a pass to a reused
character object.

The repeat event stores only creator/editor PIDs and names, selections, revisions,
scope, and runtime IDs. It does not retain a character or descriptor pointer.
When the creator is online, the service checks that the creator still meets the
level-59 authorization before each slice. Logout alone is not revocation, so a
job may continue while its creator is offline. For caster-sensitive effects that
require a live caster, a repeat pass uses the creator while online and the target
as an explicit offline-safe self-caster context while the creator is offline; the
target-context effects always use the target. This avoids impersonating or
retaining a disconnected character.

## Truthful outcomes and feedback

The implementation snapshots the supported affect state before and after each
effect. Reports distinguish applied, refreshed, upgraded, unchanged, blocked, and
failed outcomes. Result displays group effects under color-coded, aligned outcome
labels, omit zero-count groups, add `xN` counts for multi-player passes, and wrap
long groups at 78 visible columns. In particular:

- `rest` distinguishes a new rested affect, a rested-to-well-rested upgrade, and
  a refresh. Its existing duration value of 150 is an affect duration, not a
  promise of 150 wall-clock minutes.
- `regeneration` inspects the normal regeneration affect and leaves the existing
  regenerate-skill and pactum-serpentis compatibility checks in force.
- `accelerated healing` uses its existing compatibility checks. It remains a
  distinct effect from normal regeneration; no stacking or XP rule is invented by
  this command.
- Non-refreshing effects such as an already-present stat buff are reported as
  unchanged rather than falsely counted as newly applied.

One-shot mode preserves the legacy per-player spell messages. Repeat mode sends
bounded aggregate feedback to the authorized operator rather than replaying a
new global broadcast for every target and interval. Lifecycle audit records name
the job, creator/editor, revision, scope, selection, and outcome; they do not
capture raw command strings or unrelated player data.

## Repeat-job lifetime and safety

- There is one active global job. A second `repeat` is rejected instead of
  silently replacing the existing owner. Any authorized controller may inspect,
  update, or stop it.
- A pass is either running or waiting for its next callback; callbacks do not
  overlap. Sliced continuation callbacks use a bounded one-second scheduler gap,
  and the next full pass is scheduled only after the current pass completes, so a
  stalled clock cannot create a catch-up burst.
- A scheduler allocation/rejection stops the job coherently and reports the
  failure to the last available operator. `status` never claims a timer is active
  after scheduling failed.
- `stop` cancels the pending handle when present and causes any already-queued
  callback to return without doing work. A stop during a slice reports partial
  work; completed spell effects remain.
- Cold reboot and copyover call the module reset hook after the event pool is
  rebuilt. Drafts, the active job, and its handles are therefore cleared; no
  schedule is serialized or replayed.

## Verification notes

The implementation was checked with the focused source contract test and with the
repository's available WSL `g++` C++20 warning profile for the new module,
`actwiz.c`, and `new_events.c`. The native `make` invocation is not available in
the Windows shell, and the repository Makefile cannot safely resolve this
OneDrive path with spaces under WSL; the PR validation records that environment
limitation separately from compiler diagnostics.

The remaining full-client journeys (large live recipient set, extraction during a
running pass, reconnect, and scheduler allocation failure) are covered by the
bounded state-machine structure and should be exercised in the server integration
environment before changing the interval or target limits.

## Non-goals

Persistent presets/schedules, arbitrary spell execution, automatic login
blessings, player rewards or compensation, XP rebalance, changing `newbsu`, and
production migrations remain out of scope.
