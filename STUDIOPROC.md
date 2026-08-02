# studioproc — builder-authored procs

This document is for reviewers and for whoever maintains this next. It
explains what was added, why each piece is shaped the way it is, and what
was deliberately left out. The builder-facing grammar reference is
`src/howto_trg.txt`; this is the design and the reasoning behind it.

## The problem

A proc today means a C function in `specs.*.c` plus an assignment in
`specs.assign.c`. That is a code change, a rebuild and a reboot, which
means it is not something a zone author can do. The practical consequence
is that almost every builder-written mob stands still and autoattacks, and
anything more interesting gets smuggled in through the object system — the
canonical example being a boss carrying an invisible, no-pickup weapon
purely so that it has an attack it can call its own.

The engine is not actually missing the dispatch. `mob_index[].func.mob`,
`obj_index[].func.obj` and `world[].funct` already deliver commands,
combat rounds, ticks, death and decay to prototype procs. What a builder
cannot do is *fill those slots* without editing C.

## The approach

`studioproc_boot()` reads one data file, `areas/world.trg`, at boot and
binds a single generic C proc per target type to every vnum named in it.
After that, the engine's own dispatch does all the work — there is no
scripting VM, no interpreter thread, and no new dispatch machinery.

**The rule that governs everything here: the C is primitives only, and a
proc is data.** The module implements fourteen actions, nineteen
conditions and sixteen events. Behaviours are composed from those in the
data file. A new behaviour is a new record, never a new C function; if
something cannot be composed, the correct fix is to generalise a
primitive rather than add a one-off.

That rule was tested before the code was written, not asserted after.
Roughly forty concrete behaviours — breath weapons, gaze attacks, sweep,
skill rotations, enrage, multi-phase bosses, death curses, healers that
cast on their own allies, thieves, guards, spawner objects, attack
riders, set bonuses, portals, entry traps, toll gates, shifting mazes,
timed passages — were mapped against the primitive set. Thirty-five
compose from data alone; four already exist in the engine
(`ACT_SCAVENGER`, mobpatrol, the `AGGR_` words); none required new C.

The payoff is that behaviour arrives with the zone, and installing a
zone stops being a code change. The cost is one new data file in
`areas/` and a large module.

## Why one event covers a dozen

`special()` (interp.c:1808) delivers *every* verb to a bound proc. So
`CMD <verb>` is one event rather than twelve: on-look, on-open,
on-touch, on-wear and on-flee are all `CMD <that verb>`. Adding a verb
to the engine adds a trigger type for free, with no change here.

Three things the engine never dispatches anywhere needed one call each:

| hook | why nothing existing works |
|---|---|
| speech (`actcomm.c`) | `special()` runs *before* the command, so a reply issued from there prints above the player's own "You say", and returning TRUE swallows the say entirely. The hook runs after the words land, so a reply reads as a reply. |
| give (`actobj.c`) | `special(CMD_GIVE)` fires pre-transfer with the raw argument string. The mob cannot tell which object it was handed, and "give all" never resolves. This runs post-transfer with the actual object. |
| kill (`fight.c`) | `CMD_DEATH` tells the victim. Nothing anywhere tells the killer that it killed — `nq_char_death()` exists at nq.c:845 with zero call sites. |

Placement is load-bearing in each case. The give hook is the last
statement of `do_give()` because a GIVE trigger may purge the mob; it
sits beside the existing `nq_action_check()`. The kill hook runs before
the `ACT_SPEC_DIE` block because a mob's hand-written death proc can
extract the mob and return early, which would skip it.

## Footprint

Seventeen added lines across five existing files. Nothing removed,
nothing reformatted.

| file | + | what |
|---|---|---|
| `Makefile` | 1 | one object, after `specs.library.o` |
| `db.c` | 7 | include, a 4-line comment, and `studioproc_boot()` in `boot_db()` |
| `actcomm.c` | 3 | include, blank, the speech hook |
| `actobj.c` | 3 | include, blank, the give hook |
| `fight.c` | 3 | include, blank, the kill hook |

The `db.c` ordering is a real constraint and the comment in the code says
so: after `assign_spell_pointers()`, because the parser resolves spell
names through `spells[]`; before `ne_init_events()`, which asks every
bound room proc whether it wants a periodic tick.

Every anchor predates 2020, so future merges carry five one-liners.

New files: `src/studioproc.c` (the parser, binder, dispatch and
primitives), `src/studioproc.h` (the public surface — boot, the three
generic procs, the three hook entry points), and `src/howto_trg.txt`
(the builder reference, written in the shape of `howto_add.txt`).

## Reading order

`studioproc.c` is one file on purpose — one thing to review, one thing to
revert — and it is big. A path through it that works:

1. the header comment
2. `data model` (~line 161)
3. `the generic dispatch` (~1273)
4. `the four hook entry points` (~1559)
5. `parser` (~1796)
6. `boot` (~2940)

The primitives read independently after that: `conditions` (~528), `the
attack primitive` (~742, whose damage formula is written out in English
above the code), `the 'do' primitive` (~856), `per-instance state`
(~270).

## Safety

All of these are enforced in C and none are optional.

- Caps of 24 actions, 8 conditions and 32 triggers per record.
- A per-trigger re-entrancy latch and a recursion depth ceiling of 4, so
  `do` cannot recurse away.
- `do` refuses any command with a non-zero `minimum_level` or the
  grantable flag, and refuses below the command's minimum position.
- Every damage path caps a single firing at 80% of the victim's current
  hp and passes `RAWDAM_NOKILL`. Triggers wound; they do not execute.
- Typed damage goes through `spell_damage()`, so every existing
  resistance, shield and globe check still applies.
- Trusted characters are never damaged. Ordinary `block`s DO apply to
  them - a maze that transfers and then blocks must suppress the
  original move for everyone, or an immortal walks a broken maze - but
  data can never suppress a privileged command: a `block` fired by a
  trigger on a command with a non-zero `minimum_level` (or the
  grantable flag) is ignored, the same test `do` applies in the other
  direction. `goto` always works.
- Actions on a dead or extracted target are skipped. `purge` must be
  last and is refused outright for DEATH, HPBELOW and DAMAGED.
- **A vnum that already has a hand-written C proc keeps it.** The C proc
  runs first and a TRUE return means the data never runs, so existing
  content — Tiamat, for one — behaves exactly as it does now.
- FIGHT, PULSE and DEATH triggers require `ACT_SPEC` on the mob (plus
  `ACT_SPEC_DIE` for DEATH), because that is the engine's own gate
  (mobact.c:5735, fight.c:2566).

A malformed record logs its zone, vnum and line to `logs/log/status` and
is skipped, so a bad `.trg` cannot stop a boot. The file is read and
never written. The kill switch is renaming `areas/world.trg` and
rebooting.

## Per-instance state

Counters, cooldowns and latches are per *instance*, not per prototype —
"has enraged", "phase 2" — which is what makes a boss genuinely stateful
rather than random.

They are stored as ordinary affects flagged `AFFTYPE_STORE`, which is
the engine's own "used to store data only" idiom (structs.h:125; used
today in epic.c:123, nanny.c:2465, new_skills.c:549,
specs.object.c:7949). Choosing that over a pointer-keyed side table buys
three things: state dies exactly when the instance does, there is no
`free_char()` hook to add and no lifetime to get wrong, and the state is
visible in `stat` while debugging.

Object state rides an instance extra-description, which is how proclib
parameters already persist. Room state is a flat array sized at boot.

## Threading

Every entry point runs on the main game thread. The module holds its
state without locks because nothing else can reach it — the SQL,
persistence and locker workers only ever consume sealed strings and
never touch `P_char`, `P_obj` or `world[]`.

That is a claim rather than a guarantee, so it is enforced instead of
asserted: `studioproc_boot()` latches `pthread_self()`, and every
dispatch entry point returns immediately on any other thread. If a
future refactor moves combat off the main loop, this subsystem goes
quiet rather than racing.

## What already exists, and why this is not that

"Don't we already have this" is the right first question, so it was
asked before the code was written.

The tree has three quest-shaped data systems — `quest.c` (`.qst`:
keyword→message, give→reward on quester mobs), `nq.c` (XML quests, which
already hook say at actcomm.c:1127/1209 and give at actobj.c:2855) and
`world_quest.c` — plus `mobpatrol.c`, `ACT_SCAVENGER`, the `AGGR_`
aggression words, an `ITEM_SPAWNER` constant (defines.h:138) with no
implementation behind it, and `mob_prog_data` structs (structs.h:2107-2116)
with no implementation anywhere, which suggests a mobprog layer was
intended once and never built.

None of them can express a boss phase, a per-instance counter, a
conditional exit, a room damage-over-time, or a mob that casts on its own
allies. The honest overlap is give and speech flavour on quester mobs,
which `.qst` already covers well — and for that, `.qst` remains the right
tool. This subsystem touches and replaces none of them.

## Deliberately not included

- **On-crit riders.** Nothing is dispatched when a critical is
  confirmed. The smallest fix is a `CMD_MELEE_CRIT` pseudo-command
  beside `CMD_MELEE_HIT` — two lines in `hit()`. Left out because that
  trade, in that function, should be the maintainer's call.
- **A fix for `events.c:1348`**, where `room_event()` passes a room vnum
  while every other room-proc call site passes the real index.
  `studioproc_room()` accepts both rather than carrying an unrelated
  change into this PR.
- **`spells.h` changes.** The three affect type ids (2198-2200) come
  from the unused top of the `skills[]` index space
  (`MAX_AFFECT_TYPES + 1` = 2201; the `TAG_` list currently ends at
  2126). Nothing reserves them, so if that list ever grows past 2197 they
  collide silently. Three explicit `TAG_`-style constants would be
  strictly safer and is three lines — say the word.
- **Toolchain integration for `world.trg`.** It is appended by hand
  rather than produced by `areas/src`, which is also why area
  regeneration leaves it alone. Having `make_all` concatenate
  `trg/*.trg` per zone would be a better long-term shape and is a small
  follow-up.

## Verifying it yourself

Build from a pristine `git archive` of the branch: `cd src && make -j16`.
Expect exit 0 and 225 objects, with your warning set unchanged.

Boot with no `areas/world.trg` present — the entire feature is one line
in the status log, `STUDIOPROC: no areas/world.trg, proc engine idle.`,
and nothing else changes.

Then give it content. Create `logs/log` if it does not exist (`logit()`
gives up quietly when the directory is missing), and write
`areas/world.trg` using any mob vnum you have to hand:

```
#<mobvnum> M
T CMD east
if !carrying <objvnum>
say You do not pass without the sigil.
block
~
S
#~
```

Boot, and the status log reports `STUDIOPROC: 1 records, 1 triggers,
1 bound (1 mob, 0 obj, 0 room), 0 counters.` Walk a mortal east past the
mob with and without the object. Then break the file on purpose and boot
again: the parse error names the zone, vnum and line, the record is
skipped, and the boot completes.

`src/howto_trg.txt` has the full grammar and the authoring rules.
