# Falling skill corrections (#339, #340)

## Rules

Safe Fall success halves the computed impact damage, rounding down. The existing
pre-skill minimum of two gives a successful minimum of one. The strict skill
success comparison is unchanged. Water landings, flying/levitating characters,
mount/rider handling and breakable-floor behavior keep their existing rules.

Climb requires the active Climb affect and catches an initiating fall with
`clamp(effective_skill, 0, 100) / 2` percent probability, rounded down. This
implements the existing comment's 50% maximum and prevents skill bonuses above
100 from exceeding it. The effective skill still includes the existing Mental
Anguish restriction. Climb does not recheck once a falling event is underway.

The old Safe Fall left shift doubled damage, contrary to its help and adjacent
survival comment. Halving corrects the protective direction with integer
arithmetic; the historical comments do not specify a more precise percentage.
No other balance formula or random success threshold changes.

## Recent-change check and reproduction

Before editing, checked `origin/master` at `1db71f721` and open/recent PRs. No
existing PR corrected these calculations. Recent item-power changes to
`affects.c` left them unchanged. Both issues reproduce in the production
`falling_char` function compiled with controlled callbacks.

`test_falling_skills.py` fails on master when successful Safe Fall is required to
reduce damage. It passes under ASan/UBSan after the changes. Representative output:

```text
safe fall speed=43: baseline=172 success=86
safe fall speed=250: baseline=1000 success=500
climb skill=1: catches=0/100
climb skill=2: catches=1/100
climb skill=50: catches=25/100
climb skill=100: catches=50/100
climb skill=150: catches=50/100
```

The regression exhausts every 1–100 roll for negative, zero, boundary, ordinary
and above-cap skill values. It covers success/failure, short/long falls, odd
damage rounding, minimum damage, a lethal threshold, water, a breakable floor,
mount/rider damage and dismounting, flight/levitation, active/absent Climb,
Mental Anguish, and initial versus already-scheduled falls.

## Real player journey

```sh
make -C src -j4
make -C src -j4 PERSISTENCE_BACKEND=flatfile DMS_BINARY="$PWD/bin/server/dms_falling"
python3 tests/async/test_falling_skills.py
python3 tests/async/run_falling_skills_journey.py "$PWD/bin/server/dms_falling"
```

The Telnet journey creates a synthetic account and character, saves/quits,
restarts, walks east off a ledge, falls through real events, lands, saves, and
restarts/reloads in the landing room. It uses a private minimal world and flat-file
authority. The offline fixture assigns a Thief so login's real class eligibility
check preserves Safe Fall. Skill 1 always fails the existing strict comparison;
skill 100 can fail on rolls 100/101. Failed rolls may retry up to five times;
increased damage fails immediately. Knockout recovery is handled before retrying
a rejected save command. The large HP fixture makes ordinary damage randomness
and a regeneration tick small relative to the skill effect.

Observed before the fix: 6,205 HP lost without a skill success, then 12,411 with
success; the journey fails explicitly for increased damage. An active Climb
affect at minimum skill also caught the fall on master and prevented landing.
After the fix: one exact-source run saved losses of 6,204 without success, 3,098
with success, and 6,197 with minimum-skill Climb. All landing/restart checks passed.
Both backend builds and the repository format/diff checks passed locally.

These changes do not resolve #342's lethal breakable-floor investigation.
Mount/rider, lethal-threshold and floor checks here use controlled callbacks;
they are not a claim of full-server death/corpse/floor validation. No production
state or database was used. Hosted CI is not the acceptance gate.
