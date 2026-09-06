# Issue #154: pickup prompts and NPC Death Field targeting

Updated: 2026-09-06.

Status: **implementation and review follow-up complete; ready to merge in PR
[#159](https://github.com/Community-Duris/Duris/pull/159)**.

- Issue: [#154 — extra pickup prompt and Death Field missing an engaged player](https://github.com/Community-Duris/Duris/issues/154).
- Branch: [`fix/154-pickup-prompts-death-field`](https://github.com/Community-Duris/Duris/tree/fix/154-pickup-prompts-death-field).
- Pull request: [#159 — Fix pickup prompt ordering and NPC area target pruning](https://github.com/Community-Duris/Duris/pull/159).
- Created from fetched `origin/master` at
  `bf095cf5c57996cbe7732bf1f61a29a61b7700f3`.
- The issue remains open pending review and merge; the PR closes it on merge.

## Completed work

### Deferred pickup prompts

In [`src/net/comm.c`](../../src/net/comm.c), `process_output()` now keeps an
ordinary command prompt pending while either the item-movement or currency
coordinator reports the player busy. It continues flushing queued text, including
unrelated asynchronous messages. Pager and string-editor prompts are not deferred,
and the leading-newline decision uses the state that would exist after the pending
command prompt. Once completion publishes its messages, normal prompt generation
resumes. Submission and commit ordering are unchanged.

The existing busy predicates intentionally retain their established conflict
scope, including inbound transactions involving the player. That matches the
input gate, which already withholds dependent commands for either side of a
pending transfer. The busy checks remain short-circuited to descriptors with a
pending ordinary prompt, so pager/editor and prompt-free output passes incur no
coordinator scan.

[`test_item_movement_prompt_runtime.py`](../../tests/async/test_item_movement_prompt_runtime.py)
holds real single-item and bulk movement transactions at the coordinator boundary
for six output passes. It checks successful and failed item completion, immediate
rejection, currency completion, container and floor source fixtures, pager/editor
prompts, switched descriptors, and unrelated output while pending. Delayed output
is compared byte for byte with synchronous output, with prompt count and ordering
assertions across:

- Compact and noncompact output.
- Smart, old-smart, both flags, and neither flag.
- One-line and two-line prompts, both in and out of combat.
- Telnet output and WebSocket JSON text serialization.

The completion callback queues fixture pickup text; it does not run the full
`do_get()` command. Existing ownership, bulk-get, source-owner and input-queue
regressions provide complementary command/publication coverage. The prompt
generator and serializers are production code; the transport endpoint is a
fixture, so WebSocket network framing and the reporting player's client are
outside this test's coverage.

### NPC area targeting

In [`src/core/utility.c`](../../src/core/utility.c), shared area pruning now
protects an autonomous NPC caster's eligible melee opponent as well as its explicit
spell target. This addresses the demonstrated group-combat path where a mob aiming
at a different player could randomly skip its melee opponent. Charmed pets and
mob bodies controlled by switched immortals retain the player-controlled pruning
policy. The skip count is capped by the number of unprotected PCs, making loop
termination explicit if additional protected-target rules are added later.

This is a policy choice for **shared autonomous-NPC area spells**, not a Death
Field-only damage change. Other eligible players still undergo pruning.
Player-controlled caster pruning, altitude/safe-room/death eligibility, and damage
defenses are unchanged.

[`test_death_field_runtime.py`](../../tests/async/test_death_field_runtime.py)
executes production `MobCastSpell()`, `event_spellcast()`, Death Field, area
selection, `spell_damage()` and ward handling. Checks cover:

- 1,000 completed solo casts reaching damage.
- 1,000 group casts protecting the melee opponent and explicit target while
  still pruning other targets.
- Low configured hit chance and altitude, safe-room and dead-target exclusions.
- Ward absorption, resistance, deflection and area evasion controls.
- 1,000 casts each for player, charmed, and switched casters retaining the
  player-controlled pruning behavior.

World/event services, damage modifiers, resistance decisions and final raw HP
application are fixtures. The original mob/zone, player defenses and client
transcript were not supplied. The original encounter has therefore **not** been
reproduced; the tests verify the identified group-pruning path and controlled
damage/defense behavior.

## Validation checkpoint

| Command or check | Result |
| --- | --- |
| `make -C src` | Passed with the strict warning profile. |
| `python3 -B tests/async/test_item_movement_prompt_runtime.py` | Passed under ASan/UBSan, including item/currency deferral, ambient bytes, pager/editor prompts, and switched descriptors. |
| `python3 -B tests/async/test_death_field_runtime.py` | Passed under ASan/UBSan, including autonomous, charmed, switched, and player-caster policy. |
| Prompt/currency/custody adjacent focused regressions | Passed. |
| `make test-db` | Passed using isolated Docker databases, including migration replay, bootstrap equivalence and runtime compatibility. |
| `make security-check` and workflow shell/Python syntax checks | Passed. |
| `./scripts/format.sh --check` and `git diff --check` | Passed. |
| `make test-all -j"$(nproc)" TEST_JOBS="$(nproc)"` | **412 passed, 2 failed** in 417.85 seconds; both changed regressions and the native signal suite passed. |
| `python3 -B tests/async/test_flatfile_combat_journey.py` | Passed on standalone retry, including player death, corpse recovery, save and reconnect. |
| `python3 -B tests/async/test_flatfile_chaos_new_character_kit.py` | Failed on a pre-existing creation-grant integrity retry; the same failure reproduced at untouched PR head `55a2afe`. |

### Full-suite retry result

`test_flatfile_combat_journey.py` timed out in `disputed_death()` after the
initiating `hit executioner` randomly returned:

```text
You stumble, but recover in time!
```

The transcript continued to show 35/35 HP. That message comes from the unchanged
combat fumble path, which returns before starting combat. The standalone retry
subsequently passed its complete death, corpse recovery, save and reconnect
journey. `test_flatfile_chaos_new_character_kit.py` separately encountered a
starter-item critical-command integrity retry loop. That result reproduced in a
detached worktree at `55a2afe`, before these review changes, and the prompt is
canceled by the existing creation-grant command block before the reviewed
transaction prompt condition is evaluated. No unrelated Chaos, coordinator, or
journey code was changed.

## Review follow-up

All review details were addressed: the prompt gate was narrowed and extended to
currency, ambient newline behavior was preserved, autonomous-NPC policy was made
explicit and loop-safe, and the regressions now use exact overload signatures and
a shared extraction helper while covering pager/editor and switched paths.

No deployment, production migration, or restart of the configured game server
was performed. Existing integration tests used disposable servers and isolated
state. The repository-local `.agents/skills/plan-ablation/` workflow is included
in this branch at the owner's request.
