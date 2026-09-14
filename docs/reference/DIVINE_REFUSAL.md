# Divine refusal for ordered cleric pets

The divine-refusal pilot is a default-off reliability modifier for manually
ordered, charmed NPC clerics. It runs only in the ordinary `order` command. It
does not affect autonomous NPC behavior, uncontrolled clerics, player
characters, following, or direct spell execution.

## Configuration

All values are numeric entries in `lib/duris.properties` and take effect after
the normal `properties reload` command.

| Key | Shipped value | Effective range | Meaning |
| --- | ---: | ---: | --- |
| `pets.divine_refusal.enabled` | 0 | off/on at 0.5 | Master switch. Missing or non-finite values preserve legacy behavior. |
| `pets.divine_refusal.summoner_only` | 1 | off/on at 0.5 | When enabled, only a PC with the summoner class can receive refusals. Non-finite values fail closed to summoner-only. |
| `pets.divine_refusal.percent` | 10 | 0–100 | Chance per eligible dispatch attempt outside an active refusal window. Zero consumes no random draw. |
| `pets.divine_refusal.retry_lock_seconds` | 4 | greater than 0, capped at 60 | Per-live-NPC refusal window. Invalid non-positive or non-finite values disable the gate. |

`abort` and `flee` are always exempt. Blank or unknown commands and commands the
existing casting/item-action gate would reject do not consume a random draw.
The decision occurs before the requested command handler or special procedure,
so refusal consumes no spell mana, item charge, or action-specific resource.

An initial refusal gives the controlling player one normal combat-round command
wait. Retrying during that pet's active window does not reroll, extend the
window, execute the rejected command, or repeat the room message. A group order
evaluates each eligible pet independently but charges the controller at most one
normal combat-round wait.

The deadline belongs to the live NPC instance. It survives movement, transfer,
and re-charm of that instance, but is intentionally not persisted across
extraction/recreation, copyover, or restart.

## Rollout and rollback

Keep the master switch at zero until the proposed chance and duration have been
reviewed in disposable gameplay. Enabling this setting is a balance decision,
not part of deploying the code. Set `pets.divine_refusal.enabled=0` and reload
properties for immediate rollback. Disabling does not replay rejected orders or
alter unrelated waits; an unexpired per-NPC deadline remains dormant and can
resume if the switch is re-enabled before it naturally expires.
