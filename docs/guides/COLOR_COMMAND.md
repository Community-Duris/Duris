# Choose your display colors

Start with `toggle color`. It lists your settings, channels, previews, reset commands,
and motion setting. Enter a channel alone to see what it affects and the valid choices.
The written labels stay readable even when a sample uses a different foreground.

| Command | Result |
| --- | --- |
| `toggle color tell` | Current choice and available colors |
| `toggle color tell bright` | Complete bright color names and an example |
| `toggle color tell bright cyan` | Old and new choice, styled sample, restore hint |
| `toggle color tell default` | Restore the original channel presentation |
| `toggle color preview` | Preview every available channel |
| `toggle color preview tell` | Preview the current tell presentation |
| `toggle color preview tell bright cyan` | Try a sample without changing settings |
| `toggle color reset` | Explain reset-one and reset-all |
| `toggle color reset tell` | Restore tells and replies |
| `toggle color reset all` | Restore every channel and motion to default |
| `toggle color room` | Explain default, static, and animated room prose |
| `toggle color room animated` | Select animation for configured room prose |
| `toggle color motion off` | Restrict decorative effects to a stable frame |
| `toggle color motion on` | Allow selected/configured animation on new eligible output |

Chat choices are say, tell, guild, shout, yell, whisper, ask, and petition. `reply`
is an alias for tell; `gcc` is an alias for guild. Channel abbreviations must be
unambiguous: `t` selects tell, while `s` lists say and shout. Case and repeated
whitespace do not matter. Color names must be complete; `purple` suggests magenta
without applying it. Trailing text such as `tell bright cyan extra` changes nothing.

Default retains authored colors. It does not mean monochrome. Use `default`, not
an ambiguous channel-level `off`. A repeated choice is a no-op with a sample and
does not request another save. Preview commands never save or advance animation.

Changes apply to future output and report **save pending** after admission to the
normal character checkpoint. If admission fails, previous settings remain active.
See [persistence behavior](OUTPUT_PREFERENCES.md) for recovery and durability.

Room animation requires a configured room profile. If configuration is absent,
the command says so and retains authored output. Static output and motion-off do
not change over time. The `room` setting concerns long prose; it does not alter
the room title or maps. Other presentation branches are added as their output
paths are adopted. Channel definitions, aliases, choices, and previews are shared
by the parser and its discovery text.

The compiled command walkthrough is maintained in
`tests/async/color_command_harness.cpp`; run `python3 tests/async/test_color_command.py`.
It covers every command above, incomplete and invalid input, aliases, ambiguity,
actual rendered previews, immutability, resets, no-op and failed save admission.
