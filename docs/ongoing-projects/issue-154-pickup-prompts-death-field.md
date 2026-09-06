# Issue #154: pickup prompts and NPC Death Field targeting

Updated: 2026-09-06.

Status: **implementation and validation complete; ready for review in PR
[#159](https://github.com/Community-Duris/Duris/pull/159)**. The previously
interrupted standalone retry passed, resolving the only inconclusive validation
result from the full regression gate.

- Issue: [#154 — extra pickup prompt and Death Field missing an engaged player](https://github.com/Community-Duris/Duris/issues/154).
- Branch: [`fix/154-pickup-prompts-death-field`](https://github.com/Community-Duris/Duris/tree/fix/154-pickup-prompts-death-field).
- Pull request: [#159 — Fix pickup prompt ordering and NPC area target pruning](https://github.com/Community-Duris/Duris/pull/159).
- Created from fetched `origin/master` at
  `bf095cf5c57996cbe7732bf1f61a29a61b7700f3`.
- The issue remains open pending review and merge; the PR closes it on merge.

## Completed work

### Deferred pickup prompts

In [`src/net/comm.c`](../../src/net/comm.c), `process_output()` now keeps the
command prompt pending while `item_movement_transaction_player_busy()` reports
an outstanding movement. It continues flushing queued text, including unrelated
asynchronous messages. Once ownership completion publishes its messages, normal
prompt generation resumes. Ownership submission and commit ordering are unchanged.

[`test_item_movement_prompt_runtime.py`](../../tests/async/test_item_movement_prompt_runtime.py)
holds real single-item and bulk movement transactions at the coordinator boundary
for six output passes. It checks successful and failed completion, immediate
rejection, container and floor source fixtures, and unrelated output while pending.
Delayed output is compared byte for byte with synchronous output, with prompt
count and ordering assertions across:

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
protects an NPC caster's eligible melee opponent as well as its explicit spell
target. This addresses the demonstrated group-combat path where a mob aiming at
a different player could randomly skip its melee opponent.

This is a policy choice for **shared NPC area spells**, not a Death Field-only
damage change. Other eligible players still undergo pruning. Player-caster
pruning, altitude/safe-room/death eligibility, and damage defenses are unchanged.
The issue left this policy open; protecting both targets was the implementation
assumption stated during the work and has not received a separate owner response.

[`test_death_field_runtime.py`](../../tests/async/test_death_field_runtime.py)
executes production `MobCastSpell()`, `event_spellcast()`, Death Field, area
selection, `spell_damage()` and ward handling. Checks cover:

- 1,000 completed solo casts reaching damage.
- 1,000 group casts protecting the melee opponent and explicit target while
  still pruning other targets.
- Low configured hit chance and altitude, safe-room and dead-target exclusions.
- Ward absorption, resistance, deflection and area evasion controls.
- 1,000 player-caster casts retaining the existing pruning behavior.

World/event services, damage modifiers, resistance decisions and final raw HP
application are fixtures. The original mob/zone, player defenses and client
transcript were not supplied. The original encounter has therefore **not** been
reproduced; the tests verify the identified group-pruning path and controlled
damage/defense behavior.

## Validation checkpoint

| Command or check | Result |
| --- | --- |
| `make -C src` | Passed. The full gate also rebuilt the maintained targets. |
| `python3 -B tests/async/test_item_movement_prompt_runtime.py` | Passed, including the final expanded container/failure/rejection cases. |
| `python3 -B tests/async/test_death_field_runtime.py` | Passed under ASan/UBSan. |
| `python3 -B tests/async/test_get_all_durable_chain.py` | Passed. |
| `python3 -B tests/async/test_live_item_movement_contract.py` | Passed, 13 checks. |
| `python3 -B tests/async/test_get_item_source_owner.py` | Passed. |
| `python3 -B tests/async/test_item_movement_input_queue.py` | Passed. |
| `python3 -B tests/async/test_telnet_output_runtime.py` | Passed. |
| `make test-db` | Passed using isolated Docker databases, including migration replay, bootstrap equivalence and runtime compatibility. |
| `make test-native` | Passed separately after the full gate stopped. |
| `./scripts/format.sh --check` and `git diff --check` | Passed. |
| New regressions supplied with master's original changed functions | Both failed at the expected assertions: premature output and missing melee-target damage. |
| `make test-all` | **413 passed, 1 failed** in 520.27 seconds; command exited 2. Both new tests passed in this run. |
| `python3 -B tests/async/test_flatfile_combat_journey.py` | Passed on standalone retry, including player death, corpse recovery, save and reconnect. |

### Full-suite retry result

`test_flatfile_combat_journey.py` timed out in `disputed_death()` during the
`reset_coins=True` journey. It sent `hit executioner` and expected
`Your wounds claim you at last`, but received:

```text
You stumble, but recover in time!
```

The transcript continued to show 35/35 HP. That message comes from the unchanged
combat fumble path, which returns before starting combat. The standalone retry
subsequently passed its complete death, corpse recovery, save and reconnect
journey. No combat-journey test or unrelated combat code was changed.

## Review follow-up

1. Have the reviewer assess the shared NPC-area policy. If the original encounter
   needs confirmation, obtain the mob name/vnum and zone, solo/group context,
   melee and explicit targets, altitude, defenses, prompt/terse settings, and a
   transcript with HP before and after Death Field.
No deployment, production migration, or restart of the configured game server
was performed. Existing integration tests used disposable servers and isolated
state. The unrelated untracked `.agents/skills/plan-ablation/` directory is not
part of this branch's changes.
