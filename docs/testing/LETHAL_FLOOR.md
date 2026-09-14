# Breakable-floor lethal impact (#342)

The floor branch used to announce shattering/continued falling, call `damage`,
then invoke a dispel and request another fall even when damage reported death.
The real dispel already rejects a dead caster and the scheduler rejects ordinary
work for a dead actor. This patch makes that existing continuation policy
explicit at the caller and reports impact without claiming the floor shattered.
Death owns corpse placement and actor release; the dead actor does not destroy
the floor. Surviving actors still dispel it and continue falling as before.

## Tests and observed outcome

`python3 tests/async/test_lethal_floor.py` compiles the full production falling
function under ASan/UBSan. Controlled callbacks exercise lethal/nonlethal PC/NPC
and mounted impacts. It asserts no post-lethal dispel/event call, while living
impacts retain damage, one dispel and one continuation. This is callback coverage,
not proof of full-server NPC/rider destruction under sanitizers.

`python3 tests/async/run_lethal_floor_journey.py /absolute/server nonlethal`
creates two disposable Telnet accounts. An offline fixture makes one staff and
sets the other's HP; the staff player casts a real wall of ice down across the
test shaft. Three falling steps reach the required speed above 43. The real
movement, impact, spell dispel, wall removal, final landing and player save pass.

The `lethal` variant verifies death, the intact floor and corpse in the impact
room, account-menu release, re-entry, real corpse looting, save, restart and
retained recovered item IDs/death count. Normal carry limits apply: the fixture
recovers 11 of its 27 starting items and verifies their exact identities. Minimal
mode deliberately skips corpse restoration at startup, so the live journey
loots before restart; production corpse-restore tests cover catalog loading.

The earlier account-menu release failure (`rejected_preserved error=74`) occurred
on both candidate and independently rebuilt unchanged master. It was traced to
first-corpse creation before a world-item catalog exists, now tracked by #359.
Dropping loot before dying had hidden this defect in other combat journeys.
The first-corpse fix passes independently, and the combined local build supplies
it to this journey. This PR is stacked on that fix so its reviewed diff remains
the lethal-continuation change. No production DB copy is required.

Both local backend builds and formatting passed. An initial baseline comparison
was invalidated by restored source mtimes allowing cached candidate objects; it
was replaced by a baseline rebuild that explicitly touched all runtime units
changed in this sweep. Exact PR rebuilds use that same rule. No production state
was accessed. GitHub CI is not the blocker.

## Disposition

The downstream death guards contain the originally observed continuation under
the tested callbacks, and the impact narration was inaccurate. The scoped guard
and text correction are ready for review after local lethal/nonlethal journeys
and both backend builds. PC/NPC and mount callback cases run under ASan/UBSan;
a full-server NPC/mount/rider destruction matrix remains additional world-hazard
coverage rather than a prerequisite for this caller-level policy correction.
No original memory-safety incident has been established or claimed fixed.
