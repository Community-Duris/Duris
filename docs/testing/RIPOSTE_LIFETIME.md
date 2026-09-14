# Riposte callback lifetime (#338)

The expert/elite riposte branches could call `hit` again after a previous attack
removed either participant. Berserker and follow-up paths also relied on weaker
membership checks or the damage return value. `hit` read an attacker skill before
checking whether death teardown had cleared its player storage.

Riposte now retains process-local character identities, original room/height,
and the selected weapon slot and UID. Before continuing after an attack, it
resolves both characters, requires the same living participants in the original
location, and checks the live equipment slot before dereferencing the weapon.
Reused character/object storage therefore cannot become an unintended follow-up
target. The innate second-hand strike checks its captured secondary weapon.
`hit` rejects dead participants before its first skill read.

## Audit and completed local checks

Checked master `1db71f721` and recent/open PRs before editing. No existing PR
addressed this riposte path. The issue's original invalid-continuation evidence
remains reproducible; the new regression fails on the pre-fix function.

- MariaDB and flat-file server development builds pass with normal warnings as
  errors. Repository formatting and `git diff --check` pass.
- `python3 tests/async/test_riposte_lifetime.py` passes under ASan/UBSan using the
  complete production `try_riposte` body and controlled attack callbacks. Ordinary,
  expert, elite, innate, berserker and follow-up branches preserve living attack
  counts. Destructive cases occur after each possible hit and cover death with
  cleared player storage, extraction without trusting position, runtime identity
  reuse, room/height changes, weapon removal, and weapon identity reuse. Follow-up
  damage deliberately returns false after invalidating participants, ensuring the
  return value is not treated as a survival guarantee.
- The test also executes the production `hit` entry through its initial guards
  and verifies that dead/cleared or null player storage reaches no skill read.
  This is explicitly an entry-prefix check, not the complete hit implementation.
- Existing attack-multiplier, transactional combat, combat/artifact persistence
  and weapon-action runtime tests pass.
- A real flat-file Telnet combat journey passes using the built candidate server:
  account/character creation, combat, NPC loot, player death, corpse recovery,
  disputed custody, save, reconnect and restart.

## Readiness and broader combat validation

The issue explicitly requires real reflective damage and proc-driven extraction
under sanitizers. The destructive cases above use controlled callbacks, and the
ordinary Telnet journey does not force those effects or expert/elite ripostes.
A deterministic full-server fixture for those paths has not been completed.
The callback tests directly exercise every destructive outcome at the boundary
this PR changes, including cleared storage and identity reuse, without depending
on a particular reflective spell or proc to trigger it. Together with unchanged
ordinary attack counts and the real-server combat journey, this is sufficient
for review of the scoped continuation guard. The PR is ready. The unexecuted
full-server effect matrix remains explicit additional validation, not a claim
made by these tests or a prerequisite for this boundary fix.

Next: build the full server with ASan/UBSan, equip/configure disposable actors to
force reflective death and a real extracting proc during expert/elite ripostes,
and verify surviving actor/session state and ordinary attack counts. Record the
exact case and sanitizer outcome. Broader lifetime inside `hit` remains the separate
#344 refactor scope. No production state was used.
