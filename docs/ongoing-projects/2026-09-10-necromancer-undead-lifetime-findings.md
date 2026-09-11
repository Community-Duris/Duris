# Necromancer undead lifetime: history and current behavior

Date: 2026-09-10

## Question

Were undead raised by necromancer-type casters (animate dead, raise spectre/wraith/shadow/
vampire/lich, create dracolich, and the theurgist mirror spells) meant to have a limited
lifetime that ends unless the corpse was preserved or embalmed first? Where did that
intent come from, and is it still in the code?

## Short answer

Yes. The limited lifetime is intentional and has been in the code since 2009-2010, long
before the community fork. It is still active today. The mechanism is:

1. A corpse carries a decay timer when it is created at death.
2. Raising an undead reads the remaining decay time off the corpse and uses it as the
   charm duration of the new pet.
3. A separate real-time suicide event is scheduled from the same decay-derived value.
   Because charm duration is stored in ticks while the suicide delay is expressed in
   minutes, the event can fire before the charm affect ends.
4. Preserve and embalm extend the corpse decay timer, so casting them before raising
   gives a longer-lived undead. Nothing extends the lifetime after the undead is raised.
5. Two things make an undead permanent instead: the caster carries a necromancer globe,
   or has the Unholy Alliance innate. Either sets the charm duration to unlimited and no
   suicide event is scheduled.

The reviewed recent commits do not change the decay-to-lifetime formula, the
preserve/embalm timer behavior, or the globe/innate permanence exceptions. Some recent
commits do change adjacent aggro and restore behavior, so this conclusion is limited to
the lifetime rule itself.

## Current mechanics (as of master, 2026-09-10)

### Corpse decay at death

- `src/combat/fight.c:1668` and `:1680` set the decay timer on a fresh corpse from the
  `timer.decay.corpse.npc` / `timer.decay.corpse.pc` properties. The configured defaults
  are 20 real minutes for NPC corpses and 120 real minutes for PC corpses. The 120-minute
  example below therefore applies to a PC corpse or to a deployment that explicitly
  configures the NPC value to 120.
- The timer is an object affect tagged `TAG_OBJ_DECAY` (`src/magic/spells.h:1128`) backed
  by a scheduled event.

### Preserve and embalm

- `spell_preserve` (`src/magic/magic.c:15903`): adds `max(10, level/2)` game hours to the
  decay timer. Immortals remove the timer entirely ("preserved forever").
- `spell_embalm` (`src/magic/magic.c:16662`): adds `max(50, level*2)` game hours. The help
  text calls it the more powerful version of preserve.
- Both spells refuse a corpse that has no decay affect and both persist a PC corpse after
  the change. Mass versions exist for whole-room casting.
- `mummify` (`src/classes/necromancy.c:794`) is a death-trigger helper that calls embalm
  at max level on a corpse; it is not restricted to an NPC-only condition.

### Raising

`raise_undead` in `src/classes/necromancy.c` is the shared body for animate dead, all the
raise-X spells, and the theurgist call-X spells. The lifetime section is at
`src/classes/necromancy.c:701-720`:

- Reads the corpse's remaining decay time and converts pulses to minutes:
  `timeToDecay = obj_affect_time(...) / (60 * 4)`.
- Calls `setup_pet(undead, ch, MAX(4, timeToDecay), PET_NOCASH)`. The third argument
  becomes the charm affect duration.
- If `setup_pet` returns a non-negative duration, it schedules `event_pet_death` for
  `duration + random(1..10) + 1` minutes. The event calls `die()` on the undead.

Dracolich, greater dracolich, titan, avatar, and golem use the same pattern with a
different formula: `timeToDecay / 2 + 6000 / STAT_INDEX(INT)` (for example
`src/classes/necromancy.c:1926`). The golem path adds `number(1, 10)` at
`src/classes/necromancy.c:1936-1940`; the dracolich/greater-dracolich paths are the
ones that must not be described as having that random pad.

### What makes an undead permanent

`setup_pet` (`src/classes/necromancy.c:178`):

- Duration is `-1` (never expires) when the caster is an ordinary NPC that is not itself
  a pet.
- Duration is forced to `-1` when the caster has the Unholy Alliance innate or is holding
  or wielding a necromancer globe (`get_globe` checks those two equipment locations). In
  that case `raise_undead` gets `-1` back and schedules no death event.

So the "permanent undead" path is globe or innate, not preserve or embalm. Preserve and
embalm only stretch the timer.

### When the charm link breaks early

`charm_broken` (`src/classes/necromancy.c:143`) fires when the pet link is removed for
any reason. If the mob is the generic necropet vnum 1201 or one of the hard-coded pet
vnums, it schedules death one minute later. This is the "necro pets die after 1 min when
charm drops" rule from 2010.

### Deferred raising through the persistence layer

`persistence_defer_corpse_raise` (`src/classes/necromancy.c:652` and the resume path at
`:1370-1440`) is the 2026-08-29 addition. When the corpse is mid-handoff, the raise is
queued and resumed later. The resumed path re-reads the corpse decay timer, calls the
same `setup_pet` formula, and schedules the same death event. The lifetime rule is
unchanged; it is just applied after the handoff drains.

## Timeline from git history

Searches used: `git log --follow` on `src/classes/necromancy.c`, `git log -S` on
`timeToDecay`, `event_pet_death`, `spell_embalm`, `spell_preserve`, and a GitHub issue
and PR search on the community repo. History goes back to the 2005 initial import.

| Date | Commit | Author | What it did |
| --- | --- | --- | --- |
| 2009-08-12 | `be8dd5166` | torgal | Earliest surviving copy of `raise_undead` with the decay-timer-to-charm-duration rule and `event_pet_death`. Moved the tree under `mud/`. |
| 2010-08-02 | `fc70a295a` | Venthix | Theurgist class added. The branch retained the dracolich/titan `timeToDecay / 2 + 6000 / INT` formula and the suicide-pad behavior already present in its parent; the comment says "if the undead will stop being charmed after a bit, also make it suicide 1-10 minutes later". |
| 2010-09-04 | `2cb141675` | Venthix | "Necro pets die after 1 min when charm link drops." Added the necropet vnum table to `charm_broken`. |
| 2013-11-05 | `8157949c5` | Kitana | wipe2013 merge. `MAX(4, timeToDecay)` floor is present for animate dead by this point. Comment style edits only around the timer. |
| 2015-06-25 | `6469a7716` | Lohrr | "Pets now poof if they get conjured aggro." Hostile-on-raise undead now die in 5-10 seconds instead of lingering. |
| 2026-04-06 | `0bae49b7c` | Xanadin | Whole-tree clang-format refactor plus an Unholy Alliance aggro-condition change. It did not change the decay-to-lifetime formula. |
| 2026-06-19 | `1c8f392a2` | xander-l | Replaced fatal guards in necromancy with graceful checks. No lifetime change. |
| 2026-08-04 | `1c1aa9129` | xander-l | Added `DURIS_CORPSE_TRACE` logging around dracolich creation that reports `charm_minutes` and `corpse_remaining_minutes`. Diagnostic only. |
| 2026-08-29 | `ff1d19920` | moshehbenavraham | "Restore recoverable follower raising." Added the deferred-raise path that survives the async corpse handoff. Copies the existing lifetime formulas verbatim. |
| 2026-08-31 | `ea62f2ccd` et al. | moshehbenavraham | Moved sources into `src/classes/`, `src/magic/`, etc. |

No reviewed commit between 2026-06-10 and 2026-09-10 changed the decay-to-lifetime
formula, the preserve or embalm spells, or the globe/innate permanence exceptions.

### GitHub issues and PRs

The search found related terms in issues #63, #69, and #89, but no earlier issue covering
the specific restored-pet lifetime/identity/cap defect described here. The PR search
surfaced only corpse persistence work. The one design-relevant note is in
`docs/records/pr-23-post-merge-review.md`: automatic raising was changed so a player
corpse is not raised while its item ownership handoff is still pending. That gates *when*
a raise can happen, not how long the undead lives.

## Observation: timer units do not line up

This is not a recent regression. It has been this way since at least 2009, but it is
worth knowing when reading player reports.

- `timeToDecay` is computed in real minutes (pulses divided by 240).
- `setup_pet` stores that number as the charm affect duration. Affect durations are
  decremented once per game tick in `affect_update`, and a tick is
  `PULSES_IN_TICK` = 300 pulses = 75 real seconds (`src/core/config.h:93,107`).
- The suicide event is scheduled in real minutes.

With the default 120-minute corpse decay:

| Quantity | Value |
| --- | --- |
| Charm duration stored | 120 ticks |
| Charm actually lasts | 120 x 75 s = 150 real minutes |
| Suicide event fires | 122 to 131 real minutes after raising |

So in practice the undead dies while still charmed, roughly two hours after being raised.
The "1-10 minutes after charm ends" intent in the comment is not what players see. The
charm expiring first only happens when the corpse timer is short (under about 45
minutes), for example an old corpse or one that was already partly decayed.

Embalm and preserve are the levers players have. A level 50 embalm adds 100 game hours,
which at 75 seconds per game hour is about 125 real minutes on the corpse timer, and
therefore roughly doubles the undead's lifetime if cast before the raise.

## Open questions for the owner

1. Is the current behavior (undead expire about two hours after raise, extended only by
   preserving or embalming the corpse first, made permanent only by globe or innate) the
   intended design going forward? The history says yes; nothing suggests it was ever
   meant to be removed.
2. Should the unit mismatch be fixed so the suicide event genuinely fires after the charm
   ends? Fixing it would lengthen undead lifetime by about 25 percent at the default
   decay and would change balance without a design decision.
3. Do the theurgist call-X spells intentionally share the necromancer lifetime rules?
   They have since the class was added in 2010.

## Where to look if this becomes a task

- `src/classes/necromancy.c:143` `charm_broken`
- `src/classes/necromancy.c:173` `event_pet_death`
- `src/classes/necromancy.c:178` `setup_pet`
- `src/classes/necromancy.c:701` decay-to-lifetime block in `raise_undead`
- `src/classes/necromancy.c:1370` resumed raise path
- `src/magic/magic.c:15903` `spell_preserve`
- `src/magic/magic.c:16662` `spell_embalm`
- `src/combat/fight.c:1668` corpse decay defaults
- `lib/information/help_index` entries EMBALM (line 4586) and PRESERVE (line 11241).
  There is no help entry for ANIMATE DEAD that explains the lifetime.

## Follow-up (2026-09-10): player report of idle uncharmed undead

Reported room state: an `undead corpse` (vnum 1201) with the generic prototype description
plus twelve dracoliches (vnums 3-6), all idle in L'srillizzin with no visible owner. No
earlier GitHub issue covered this specific defect. Filed as
[Community-Duris/Duris#212](https://github.com/Community-Duris/Duris/issues/212).

### Root cause found in code

Pet persistence rebuilds pets from the area prototype and re-charms them, so it does not
preserve every runtime property or every creation-time rule in the sections above.

- Crash-save restore on login: `src/player/player_load_pets.c:106` re-reads the prototype
  by `mob_vnum`; `player_load_pets_commit` re-applies the saved charm duration and does
  not itself schedule `event_pet_death`. A finite restored charm can still expire through
  normal affect processing and invoke `charm_broken`.
- Copyover restore: `src/persistence/copyover.c:1009-1040` re-reads the prototype and
  calls `setup_pet(..., -1, ...)`, so the charm is permanent.
- Autosave checkpoints are `RENT_CRASH` (`src/persistence/persistence_checkpoint.c`), so
  the crash restore path runs after any link loss, reboot, or copyover.

Effects differ by restore path:

1. Prototype-based restoration can lose name, descriptions, level, spell slots,
   alignment, rolls, size, affects, and `ACT_SENTINEL`; the exact fields depend on the
   restore path. Prototype 1201 has act flags `0`, so a copyover-restored undead can
   wander. This matches the report's visible behavior.
2. Crash-save restore preserves and reapplies the current finite charm duration, so the
   lifetime can still expire after relog. Copyover instead calls `setup_pet(..., -1, ...)`,
   making that restored copy permanent and bypassing the original finite lifetime input.
3. Restore lacks the undead-specific `count_undead` / `can_raise_draco` accounting. A
   restored 1201 named `undead corpse` matches no `undead_data` name and may therefore
   be counted incorrectly, but generic restore caps still apply (64 snapshot pets and
   10 copyover pets); this is not literally unbounded.
4. Capture takes every NPC follower in the room, not only `LNK_PET` links.

### Still open

The exact path that accumulated twelve dracoliches is not pinned. Quit and death both
kill pets (`die_follower`, `do_dismiss(CMD_DEATH)`) and both snapshot types clear
`player_pets`. The uncapped restore paths are the only pet-creating code without a spell
check, so a repro across copyover and link-loss cycles is the next step.
