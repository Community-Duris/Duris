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

The `lethal` variant verifies the actual death and observes the intact ice floor
and corpse in the impact room. It currently fails the expected account-menu
release: the corpse operation reports `rejected_preserved error=74`, then
`death_disposition_failed outcome=2 wallet=0` repeats while the dead character
remains held. The same failure occurs on an independently rebuilt unchanged
master baseline, which also prints the old false shattering message. This is a
concrete integration blocker, not evidence that this guard introduces the stall.
The fixture's elevated offline level/HP and synthetic state may contribute;
attribution of the corpse rejection remains unresolved. Do not waive this test
or claim the full lethal journey passed. Its assertion stays in the manual
journey so the blocker is reproducible.

Both local backend builds and formatting passed. An initial baseline comparison
was invalidated by restored source mtimes allowing cached candidate objects; it
was replaced by a baseline rebuild that explicitly touched all runtime units
changed in this sweep. Exact PR rebuilds use that same rule. No production state
was accessed. GitHub CI is not the blocker.

## Draft disposition

The downstream death guards contain the originally observed continuation under
the tested callbacks, and the impact narration was inaccurate. The narrow guard
and text correction are ready for review, but #342 stays open and this PR stays
draft until the real lethal custody/release failure is attributed and resolved,
corpse contents/restart are verified, and full-server NPC/mount/rider sanitizer
coverage is completed. No original memory-safety incident has been established.
