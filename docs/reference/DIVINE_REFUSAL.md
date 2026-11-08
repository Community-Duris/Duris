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
| `pets.divine_refusal.percent` | 10 | 0–100 | Chance per eligible dispatch attempt outside an active refusal window. Fractional values are retained with 0.01-percentage-point roll granularity; zero consumes no random draw. |
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

## Authored allegiance and content

The second-phase authored rows live in the server-side JSON file
`lib/misc/divine_refusal.json`. This is deliberately separate from
`lib/duris.properties`: the properties file is a numeric key/value store and
cannot safely represent a per-vnum dictionary of patrons and messages.

The current schema is:

```json
{
  "version": 1,
  "revision": 1,
  "entries": {
    "66026": {
      "patron": "Garl",
      "message": "{patron} has forbidden it. I will not act against that warning.",
      "enabled": true,
      "percent": 10
    }
  }
}
```

Each entry is keyed by the exact NPC template vnum. `patron` and `message` are
optional so a row can only override policy, but an authored line requires both
fields and a patron in the reviewed canonical catalog. `{patron}` is the only
supported placeholder. `enabled` and `percent` inherit the shared properties
when absent; an entry-level `enabled: false` disables the pilot for that exact
vnum, while an entry-level `enabled: true` cannot bypass the global master
switch. An entry-level percentage never changes the existing refusal decision
or its retry state; it only supplies that template's effective chance.

The shipped pilot intentionally contains only two reviewed Garl cleric rows:

| Vnum | Template evidence | Patron evidence | Intended rate |
| ---: | --- | --- | --- |
| 66026 | `areas/mob/ashrumite.mob` names it `guildguard cleric`; its `PG` class mask is `32`, the loader value for `CLASS_CLERIC` (`BIT_6`). | The lore says the guildguard would rather pray in the temple, wears Garl's clerical robe, and wears Garl's symbol. | Inherits `pets.divine_refusal.percent` (shipped 10%). |
| 66031 | The same mob file names it `guildmaster cleric`; its `PG` class mask is `32` (`CLASS_CLERIC`). | Its lore explicitly describes the holy symbol of Garl and calls it a cleric of Garl. | Inherits `pets.divine_refusal.percent` (shipped 10%). |

These rows do not make an NPC summonable, charmable, owned, or orderable. The
existing runtime eligibility check still requires a live PC master, the normal
charm/ownership relationship, a cleric NPC, and (by default) a summoner master.
The rows are therefore authored lore for templates that can enter the ordinary
`spell_charm_person`/`setup_pet` control path, not a new control mechanism. New
rows should be added only after the same class, controllability, and direct
patron-lore review; priest- or temple-named mobs are not mass-tagged.

The content parser bounds the file, entry count, vnums, patron names, and
messages; rejects unknown fields, duplicate keys, control characters, invalid
percentages, unsupported placeholders, and malformed JSON. Unknown or missing
patrons, missing message fields, and any other incomplete row render the
generic refusal line. Authored `$` characters are escaped before entering the
existing speech-styled `act()` renderer, so content cannot become an `act`
format directive. The pilot uses no `do_say`, `mobsay`, command, or magic-door
path.

The file is loaded at boot and by the existing `properties reload` boundary.
An invalid or unreadable revision is logged and leaves the last valid snapshot
active; there is no partial publication. Each `order` dispatch borrows one
immutable snapshot, so a reload cannot produce mixed rows within a group order.

## Rollout and rollback

Keep the master switch at zero until the proposed chance and duration have been
reviewed in disposable gameplay. Enabling this setting is a balance decision,
not part of deploying the code. Set `pets.divine_refusal.enabled=0` and reload
properties for immediate rollback. Disabling does not replay rejected orders or
alter unrelated waits; an unexpired per-NPC deadline remains dormant and can
resume if the switch is re-enabled before it naturally expires.
