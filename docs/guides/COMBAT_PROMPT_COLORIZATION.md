# Combat, prompt and feedback color roles

`toggle color incoming`, `outgoing`, `observed`, `prompt` and `feedback` expose
only the following implemented boundaries. Defaults preserve existing presentation;
`default` or reset restores it. All these channels force Static and exclude word
dictionaries and decorative recipes, including an operator configuration that
requests animation. Global motion-off remains valid for other channels.

| Choice | Implemented boundary | Retained behavior |
| --- | --- | --- |
| incoming | Victim variant in `dam_message` | Recipient rules, terse behavior, authored attack accents, red battle brackets |
| outgoing | Attacker variant in `dam_message` | Existing damage text, authored names/accents, green battle brackets |
| observed | Room observer variant in `dam_message` | Existing room/visibility/terse filters and protected names |
| prompt | Standard `make_prompt` frame, maxima and own HP/mana/vitality prefixes | Numeric formats, resource values, names, position/ward/condition text and existing authored styles |
| feedback | Manual-save success and failure after acknowledgement or timeout | No success before durability; failure/retry/timeout and original-character lifetime checks |

This is not comprehensive adoption of every combat emitter. `raw_damage` direct
non-expanded messages, death messages, spell/skill broadcasts, proc messages and
other combat helper sends retain their prior presentation. Smart infobar cursor
output also remains unchanged. The standard prompt is constructed from player
field-selection flags; it has no arbitrary user-authored template string at this
boundary. Player/entity names and existing styled prompt fields are retained.

Profiles may configure semantic `roles` independently of word recipes:

```json
{
  "policy": "static",
  "base": "cyan",
  "roles": {
    "healthy": "green",
    "caution": "yellow",
    "low": "red",
    "critical": "bright_red",
    "success": "bright_green",
    "failure": "bright_red",
    "hit": "bright_cyan",
    "miss": "gray"
  }
}
```

This object belongs in a profile in the normal versioned configuration. Map it
only to the desired channels. The new role fields are optional and older servers
reject configuration containing them; remove those fields before a binary rollback.
No configuration is enabled automatically.

`dam_message` supplies Hit for positive event damage and Miss otherwise; it never
reads message words to classify an outcome. The original damage calculations,
random adjective selection and existing RNG calls remain unchanged. Authored
styles still win over a configured role. Role changes affect only the recipient's
base foreground and cannot reveal another damage value or target.

Own prompt resources use their existing computed percentages: Healthy at 66% and
above, Caution at 33..65%, Low at 15..32%, and Critical below 15% (including invalid
maxima). A configured role replaces only the caller-owned resource color prefix.
Without that role the original per-resource warning prefix remains. A personal
prompt color changes the frame, maxima and healthy resources; it does not flatten
warning colors. Configured semantic roles take priority over a personal base color.
The actual numbers and the original negative-mana spacing remain identical.

Prompt output still goes directly to the existing queue. Paging, editing, login,
confirmation, transaction deferral, two-line layout, MSP framing, Telnet GA and
WebSocket output use their original control flow. Prompt coloring never advances
cosmetic sequences. Logs retain the existing separate prompt-log representation;
snoop output receives the displayed prompt through its existing path.

Tests in `test_word_output_integration.py` execute real `dam_message`, `make_prompt`,
`act` and queue functions. They compare default/selected visible output, all three
combat recipients, authored names, resource thresholds and values, RNG call counts,
unchanged HP, configurable Hit/Miss/Success/Failure/Critical roles, and auxiliary
prompts. `test_manual_save_feedback_contract.py` verifies success only after ACK,
failure on deadline and lifetime gating. `test_item_movement_prompt_runtime.py`
runs the real ANSI/Telnet/WebSocket and deferred item/currency paths under sanitizers,
including selected prompt colors. Final live visual release evidence is tracked by
#289.
